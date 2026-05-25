# File: NavigationCamera_Pipeline.py
#
#
#

#!/usr/bin/env python3
import os
import time
import threading

# GStreamer python libraries 
import gi
gi.require_version("Gst", "1.0")
from gi.repository import Gst, GLib

#import all extra camera scripts and settings/config
from NavigationCamera_Config import *

#import all classes
from NavigationCamera_Stream   import CameraStreamer
from NavigationCamera_Recorder import CameraRecorder

class CameraPipeline:

    def __init__(self, logger):

        self.logger = logger

        #initialize gstreamer 
        Gst.init(None)

        self.pipeline = None
        self.tee = None

        #shared state for the camera architecture 
        self.state = {"running": False, "frame": None, "part_id": 0, "last_frame_time": 0.0,}

        #share frames between threads 
        self.condition = threading.Condition()

        #save video and pictures to their own directories on the PI
        os.makedirs(RECORDINGS_DIRECTORY, exist_ok=True)
        os.makedirs(PHOTO_DIRECTORY, exist_ok=True)

        #GLib loop in the background on its own thread 
        self.glib_loop = GLib.MainLoop()
        self.glib_thread = threading.Thread(
            target=self.glib_loop.run,
            daemon=True
        )


        self.streamer = None
        self.recorder = None
        self.http_thread = None

    def start(self):

        self.state["running"] = True

        #build pipeline camera + jpeg cap and tee
        self.build_pipeline()

        # http streaming branch variables
        self.streamer = CameraStreamer(
            self.logger,
            self.pipeline,
            self.tee,
            self.condition,
            self.state
        )

        #attach stream branch to tee
        self.streamer.attach()

        self.recorder = CameraRecorder(
            self.logger,
            self.pipeline,
            self.tee
        )

        #create photo and recorder helper
        self.http_thread = threading.Thread(
            target=self.streamer.start_http,
            daemon=True
        )

        # run the server for camera on its own thread
        self.http_thread.start()
        self.glib_thread.start()

        # start pipeline
        self.pipeline.set_state(Gst.State.PLAYING)

        self.logger.info(f"Camera feed: http://{HOST}:{PORT}/NavigationCamera")
        self.logger.info(f"Recordings:  http://{HOST}:{PORT}/MssionRecording_Logs")

    def stop_pipeline(self):

        self.state["running"] = False #tell other loops that stream stopped

        #if actively recording, safely stop recording 
        if self.recorder and self.recorder.recording:
            self.recorder.stop_recording()

        # shut fown gstreamer
        if self.pipeline:
            self.pipeline.set_state(Gst.State.NULL)

        self.glib_loop.quit()

        #stop the http server 
        if self.streamer:
            self.streamer.stop_http()

        with self.condition:
            self.condition.notify_all()

    # Check arrival of camera frames, and if timeout exceeds two seconds camera has failed
    def is_active(self):

        return (self.state["last_frame_time"] > 0 and time.time() - self.state["last_frame_time"] < STREAM_TIMEOUT_SECONDS)

    #call recorder function 
    def start_recording(self):
        if self.recorder:
            self.recorder.start_recording()
    #call stop recorder  function 
    def stop_recording(self):
        if self.recorder:
            self.recorder.stop_recording()


    def take_photo(self):

        # exit if helper functions are ot operational yet
        if not self.recorder:
            self.logger.warning("Photo failed: recorder not initialized")
            return None

        #copy frame 
        with self.condition:

            frame = self.state["frame"]

        #send the frame into the helper function in NavigationCamera_Recorder.py 
        # to save the image
        return self.recorder.take_photo(frame)

    #v412src reads from USB camera the GStreamer pipeline string to 
    #initialize the stream with the configurations set in Config file
    #tee nae = t creates stream/ recording branches
    # at specified resolution and formatting settings

    def build_pipeline(self):

        pipeline_description = f"""
            v4l2src device={STREAM_VIDEO_DEVICE}
            !
            image/jpeg,width={VIDEO_WIDTH},height={VIDEO_HEIGHT},framerate={FRAMERATE}/1
            !
            tee name=t
        """

        self.pipeline = Gst.parse_launch(pipeline_description)
        self.tee = self.pipeline.get_by_name("t")


