#!/usr/bin/env python3
# ---------------------------------------------------------------------------
#  File: NavigationCamera_Pipeline.py
#  Author: Jose Machon
#  Date: 4/2/2026 1:20pm
#
#  Function: 
#  Handles all high level managing of the nav camera, check status and inits 
#  all needed functions in the camera pipeline 
# 
# ---------------------------------------------------------------------------

import subprocess # enables FFmpeg to run concurrently
import threading  # multi threading library 
import time       # for timestamping
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer # Web toolchain

from NavigationCamera_Config import *

# Node creation
class CameraPipeline:

    def __init__(self, logger):
        self.logger = logger

        self.last_frame_time = 0.0 # keep track of frame timing

        # INIT 
        self.lock = threading.Lock()
        self.cond = threading.Condition(self.lock)

        self.running = False
        self.proc = None

        self.latest_part = None
        self.part_id = 0

        # CREATE HTTP SERVER
        self.httpd = ThreadingHTTPServer((HOST, PORT), self._make_handler())

        # THREAD FOR HTTP SERVER
        self.http_thread = threading.Thread(target=self.httpd.serve_forever, daemon=True)

        # THREAD FOR FFMPEG READING
        self.ffmpeg_thread = threading.Thread(target=self._ffmpeg_worker, daemon=True)

    # This is where the pipeline starts 
    # it marks the pipeline as active 
    # it also starts both threads 
    def start(self):
        self.running = True

        self.http_thread.start()
        self.ffmpeg_thread.start()

        self.logger.info(f"Camera feed endpoint: http://{HOST}:{PORT}/NavigationCamera")

    # Tells manager node if camera is currently 
    # active returns true with string for
    # active status
    def is_active(self):
        with self.lock:
            return self.last_frame_time > 0 and (time.time() - self.last_frame_time < 2.0)

    # shutsdown the camera pipeline by 
    # shutting down HTTP server, closing 
    # port and ends ffmpeg, if ffmpeg
    # refuses to stop it kills the process
    def stop(self):

        self.running = False

        try:
            self.httpd.shutdown()
            self.httpd.server_close()
        except Exception as e:
            self.logger.warning(f"HTTP shutdown warning: {e}")

        if self.proc is not None and self.proc.poll() is None:
            self.proc.terminate()

            try:
                self.proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.proc.kill()

        with self.cond:
            self.cond.notify_all()

    # Starts the ffmpeg stream by calling the config file 
    # for commands on streaming configurations
    # waits for frames to begin sending until marking
    # the system active
    def _start_ffmpeg(self):

        if self.proc is not None and self.proc.poll() is None:
            return

        # start the stream command
        cmd = make_ffmpeg_cmd()
        self.logger.info("Starting FFmpeg: " + " ".join(cmd))

        # captures camera output
        self.proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            bufsize=0
        )

    # this function is called when MJPEG frame is found
    # stores the frame, counts, and marks camera active
    # notifies browser thread of frame acquisition
    def _publish_part(self, part):
        with self.cond:
            self.latest_part = part
            self.part_id += 1
            self.last_frame_time = time.time()
            self.cond.notify_all()

    # this function actually reads ffmpeg frames, it 
    # owns the thread, runs until shutdown, starts
    # ffmpeg if not already active. it also creates a buffer
    # and collects frame packets
    def _ffmpeg_worker(self):
        while self.running:
            self._start_ffmpeg()

            buffer = b""
            proc = self.proc

            if proc is None or proc.stdout is None:
                time.sleep(1.0)
                continue

            try:
                while self.running:
                    chunk = proc.stdout.read(4096)

                    if not chunk:
                        break

                    buffer += chunk

                    while True:
                        first = buffer.find(BOUNDARY_BYTES)

                        if first < 0:
                            break

                        second = buffer.find(
                            BOUNDARY_BYTES,
                            first + len(BOUNDARY_BYTES)
                        )

                        if second < 0:
                            break

                        part = buffer[first:second]
                        buffer = buffer[second:]
                        
                        # PUBLISH FRAME
                        self._publish_part(part)

            except Exception as e:
                self.logger.warning(f"FFmpeg exception: {e}")

            finally:
                if self.running:
                    self.logger.warning("FFmpeg stopped; restarting in 1 second")
                    time.sleep(1.0)

    # This function creates the HTTP class 
    # this is what actually serves the stream to a browser
    # so that the UI can simply embed it onto the whole UI
    def _make_handler(self):

        pipeline = self

        class MJPEGHandler(BaseHTTPRequestHandler):

            # this runs whenever a request to view the feed
            # is detected. It sends the images separated by a 
            # boundary, keeps updating images captured by the 
            # camera. 
            def do_GET(self):
                if self.path != "/NavigationCamera":
                    self.send_error(404)
                    return

                self.send_response(200)
                self.send_header(
                    "Content-Type",
                    f"multipart/x-mixed-replace; boundary={BOUNDARY_TEXT}"
                )

                # do not cache frames, low latency prioritized
                self.send_header("Cache-Control", "no-store")

                # grants other UI web to access feed
                self.send_header("Access-Control-Allow-Origin", "*")

                # end serving to reserve another frame
                self.end_headers()

                last_id = 0

                try:
                    while pipeline.running:
                        with pipeline.cond:
                            pipeline.cond.wait(timeout=2.0)

                            if pipeline.latest_part is None:
                                continue

                            if last_id == pipeline.part_id:
                                continue

                            last_id = pipeline.part_id
                            part = pipeline.latest_part

                        self.wfile.write(part)
                        self.wfile.flush()

                except (BrokenPipeError, ConnectionResetError):
                    pipeline.logger.info("Client disconnected from MJPEG stream")

        return MJPEGHandler