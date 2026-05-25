# File: NavigationCamera_Stream.py
#
#
#



#!/usr/bin/env python3

import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from gi.repository import Gst
from NavigationCamera_Config import *

class CameraStreamer:

    def __init__(self, logger, pipeline, tee, condition, state):

        #constructor 
        self.logger = logger #prints
        self.pipeline = pipeline #GStreamer 
        self.tee = tee #split the camera feed to record and stream
        self.condition = condition #notification of new frame condition
        self.state = state #shared state

        self.stream_bin = None
        self.tee_src_pad = None

        #create the http server on its own thread

        self.http = ThreadingHTTPServer((HOST, PORT), self.browser())

    def attach(self):

        #attach the livestream branch onto the GStreamer pipeline
        self.stream_bin = Gst.Bin.new("stream_bin")

        #queue seperates branch from main stream and 
        #appsink makes python recieve individual frames 
        queue = Gst.ElementFactory.make("queue", "stream_queue")
        appsink = Gst.ElementFactory.make("appsink", "stream_sink")

        #these functions let python listen to the frames from the stream
        #it does not accumulate frames 
        appsink.set_property("emit-signals", True)
        appsink.set_property("sync", False)
        appsink.set_property("max-buffers", 1)
        appsink.set_property("drop", True)

        #build and connect 
        self.stream_bin.add(queue) # queue buffer to seperate from main pipeline
        self.stream_bin.add(appsink)

        #get one frame at a time from the stream gstreamer to python
        queue.link(appsink)


        sink_pad = queue.get_static_pad("sink")
        ghost_pad = Gst.GhostPad.new("sink", sink_pad)

        self.stream_bin.add_pad(ghost_pad)

        self.pipeline.add(self.stream_bin)
        self.stream_bin.sync_state_with_parent()

        # link output connection from branch splitter 
        self.tee_src_pad = self.tee.request_pad_simple("src_%u")
        stream_sink_pad = self.stream_bin.get_static_pad("sink")

        self.tee_src_pad.link(stream_sink_pad)

        #whenever a new frame arrives call function
        appsink.connect("new-sample", self.new_sample)

    # This function gets called everytime gstreamer makes a 
    # new frame data from the livestream available. 
    def new_sample(self, sink):

        #store the frame 
        sample = sink.emit("pull-sample")

        #egde case for error and production of an unusable frame 
        if sample is None:
            return Gst.FlowReturn.ERROR

        #get raw frame returns two 
        buffer = sample.get_buffer()
        success, content =  buffer.map(Gst.MapFlags.READ)

        if not success:
            return Gst.FlowReturn.ERROR

        #copy the jpeg frame into python memory, then free up the buffer.
        jpeg = bytes(content.data)
        buffer.unmap(content)

        # wrap JPEG inoto MJPEG HTTP frame format becuase
        # browsers expect it in this format
        part = (
            b"--" + BOUNDARY_TEXT.encode() + b"\r\n"
            b"Content-Type: image/jpeg\r\n"
            b"Content-Length: " + str(len(jpeg)).encode() + b"\r\n\r\n"
            + jpeg +
            b"\r\n"
        )

        #Save the frame in case the recorder script needs it for 
        # pictures. Update with self.condition to avoid race conditions
        with self.condition:
            self.state["frame"] = part
            self.state["part_id"] += 1
            self.state["last_frame_time"] = time.time()
            self.condition.notify_all()

        return Gst.FlowReturn.OK

    #Start the http stream to link 
    def start_http(self):
        self.http.serve_forever()

    #end the stream 
    def stop_http(self):
        self.http.shutdown()
        self.http.server_close()

    #this function deals with serving the stream onto the browser
    def browser(self):

        streamer = self

        class CameraHTTPServer(BaseHTTPRequestHandler):
            #If the bowser requests to viewt the feed serve the camera stream 

            def do_GET(self):
                if self.path == CAMERA_PATH:
                    self.serve_camera()
                else:
                    self.send_error(404)


            def serve_camera(self):
                self.send_response(200) #success status code

                # let browser know its a mjpeg stream, not cached  and allow streams from other
                # pages to embedd onto the UI
                self.send_header("Content-Type",f"multipart/x-mixed-replace; boundary={BOUNDARY_TEXT}")
                self.send_header("Cache-Control", "no-store")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()

                #track frames coming through
                last_id = 0

                try:
                    while streamer.state["running"]:
                        with streamer.condition:
                            streamer.condition.wait(timeout=2.0) #wait for a new frame 

                            if streamer.state["frame"] is None:
                                continue

                            if last_id == streamer.state["part_id"]: #prevent duplicate frames 
                                continue


                            last_id = streamer.state["part_id"]
                            part = streamer.state["frame"]
                        #sends the frame into the browser 
                        self.wfile.write(part)
                        self.wfile.flush()

                #when not actively viewing the browser safelyclose the stream
                except (BrokenPipeError, ConnectionResetError):
                    streamer.logger.info("Client disconnected")

        return CameraHTTPServer