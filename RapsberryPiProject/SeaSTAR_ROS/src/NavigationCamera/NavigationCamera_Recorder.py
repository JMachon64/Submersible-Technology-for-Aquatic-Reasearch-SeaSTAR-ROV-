# File: NavigationCamera_Recorder.py
#
#
#
#
#!/usr/bin/env python3

import os
import time

from gi.repository import Gst

# These libraries are needed to overlay datetime on recorded frames

from PIL import Image, ImageDraw, ImageFont
from io import BytesIO
from datetime import datetime

from NavigationCamera_Config import *

class CameraRecorder:

    def __init__(self, logger, pipeline, tee):

        # constructor 
        self.logger = logger
        self.pipeline = pipeline
        self.tee = tee

        self.recording = False
        self.record_bin = None
        self.recording_path = None

    def start_recording(self):

        #if self.recording marked already exit because its already recording

        if self.recording:
            self.logger.info("Recording already active")
            return

        #Create unique name file and save it to recording directory
        filename = time.strftime(RECORDING_NAME_FORMAT)
        self.recording_path = os.path.join(RECORDINGS_DIRECTORY, filename)

        self.logger.info(f"Starting MP4 recording: {self.recording_path}")

        #create container branchg for recording
        self.record_bin = Gst.Bin.new("record_bin")

        queue = Gst.ElementFactory.make("queue", "record_queue") #seperate recording branch
        jpegdec = Gst.ElementFactory.make("jpegdec", "record_jpegdec") # decode mjpeg frames into video
        convert1 = Gst.ElementFactory.make("videoconvert", "record_convert1") # convert video format
        clockoverlay = Gst.ElementFactory.make("clockoverlay", "record_clock") # overlay timestamp
        convert2 = Gst.ElementFactory.make("videoconvert", "record_convert2") # encode into H.264
        x264enc = Gst.ElementFactory.make("x264enc", "record_x264")  # encode into H.264
        h264parse = Gst.ElementFactory.make("h264parse", "record_h264parse")# format for mp4
        mp4mux = Gst.ElementFactory.make("mp4mux", "record_mp4mux") # package as mp4
        filesink = Gst.ElementFactory.make("filesink", "record_filesink") # writes mp4 to disk


        #this checks if any of the commands above failed, if any are missing recording cannot start
        elements = [queue, jpegdec, convert1, clockoverlay, convert2, x264enc, h264parse, mp4mux, filesink]
        if not all(elements):
            self.logger.warning("Failed to create recording elements")
            self.record_bin = None
            return
            
        filesink.set_property("location", self.recording_path)
        clockoverlay.set_property("time-format", DATE_TIME)
        clockoverlay.set_property("font-desc", OVERLAY_FONT)
        clockoverlay.set_property("shaded-background", OVERLAY_SHADED_BACKGROUND)
        clockoverlay.set_property("halignment", OVERLAY_HALIGNMENT)
        clockoverlay.set_property("valignment", OVERLAY_VALIGNMENT)

        #encoder configurations for ideal low latency stream
        x264enc.set_property("tune", ENCODER_TUNE)
        x264enc.set_property("speed-preset", ENCODER_SPEED)
        x264enc.set_property("bitrate", ENCODER_BITRATE)

        for element in elements:
            self.record_bin.add(element)

        # connect all elements as a recording branch 
        queue.link(jpegdec)
        jpegdec.link(convert1)
        convert1.link(clockoverlay)
        clockoverlay.link(convert2)
        convert2.link(x264enc)
        x264enc.link(h264parse)
        h264parse.link(mp4mux)
        mp4mux.link(filesink)

    # create the input for recording bin as one element 
        sink_pad = queue.get_static_pad("sink")
        ghost_pad = Gst.GhostPad.new("sink", sink_pad)
        self.record_bin.add_pad(ghost_pad)

        # add branch to main pipeline and start 
        self.pipeline.add(self.record_bin)
        self.record_bin.sync_state_with_parent()

        tee_src_pad = self.tee.request_pad_simple("src_%u")
        record_sink_pad = self.record_bin.get_static_pad("sink")

        result = tee_src_pad.link(record_sink_pad)

        if result != Gst.PadLinkReturn.OK:
            self.logger.warning(f"Failed to link recording branch: {result}")
            self.record_bin.set_state(Gst.State.NULL)
            self.pipeline.remove(self.record_bin)
            self.record_bin = None
            return

        self.record_bin.tee_src_pad = tee_src_pad
        self.recording = True

    def stop_recording(self):

        # egde case 
        if not self.recording:
            self.logger.info("Recording not active")
            return

        self.logger.info("Stopping recording")

        # get connection to main branch 
        tee_src_pad = getattr(self.record_bin, "tee_src_pad", None)
        record_sink_pad = self.record_bin.get_static_pad("sink")

        # disconnect the branch from main pipeline
        if tee_src_pad and record_sink_pad:
            tee_src_pad.unlink(record_sink_pad)
            self.tee.release_request_pad(tee_src_pad)

        self.logger.info("Finalizing MP4 file...")

        #send end of stream to the MP4 muxer 
        self.record_bin.send_event(Gst.Event.new_eos())


        bus = self.pipeline.get_bus()

        # wait for 5 seconds for the mp4 file to finalize 
        timeout = time.time() + 5.0 

        while time.time() < timeout:
            msg = bus.timed_pop_filtered(
                100 * Gst.MSECOND,
                Gst.MessageType.EOS
            )

            if msg:
                break

        # stops and removes recording branch 
        self.record_bin.set_state(Gst.State.NULL)
        self.pipeline.remove(self.record_bin)

        self.logger.info(f"Saved recording: {self.recording_path}")
         
        # reset the states 
        self.record_bin = None
        self.recording = False
        self.recording_path = None


    def take_photo(self, latest_part):

        #set file name and save to photo directory 
        filename = time.strftime(PHOTO_NAME_FORMAT)
        filepath = os.path.join(PHOTO_DIRECTORY, filename)

        # if no frame was able to be stored return 
        if latest_part is None:
            self.logger.warning("Photo failed: no frame available yet")
            return None

        #store the binary start and end of the picture  JPEG 
        start = latest_part.find(b"\xff\xd8")
        end = latest_part.find(b"\xff\xd9")

        #if they are not vaild or able to be found then NOT a valid frame 
        if start < 0 or end < 0:
            self.logger.warning("Photo failed: JPEG markers not found")
            return None

        # extract and frame the image
        jpg = latest_part[start:end + 2]

        #open jpeg and prepare to put the timestamp on it
        image = Image.open(BytesIO(jpg)).convert("RGB")

        #draw timestamp and save
        self.draw_timestamp(image)
        image.save(filepath, "JPEG", quality=PHOTO_JPEG_QUALITY)
        self.logger.info(f"Saved timestamped photo: {filepath}")

        return filepath


    def draw_timestamp(self, image):

        draw = ImageDraw.Draw(image)

        # write the date time on the image
        timestamp = datetime.now().strftime(DATE_TIME)

        # timestamp drawing.
        font = ImageFont.truetype(PHOTO_FONT, PHOTO_FONT_SIZE)

        padding = PHOTO_PADDING
        bbox = draw.textbbox((0, 0), timestamp, font=font)
        text_w = bbox[2] - bbox[0]
        text_h = bbox[3] - bbox[1]

        x = image.width - text_w - padding
        y = padding
        draw.rectangle(
        [
                    x - PHOTO_TEXT_BOX_PADDING,
                    y - PHOTO_TEXT_BOX_PADDING,
                    x + text_w + PHOTO_TEXT_BOX_PADDING,
                    y + text_h + PHOTO_TEXT_BOX_PADDING,],fill=(0, 0, 0)
            )

        draw.text((x, y), timestamp, fill=(255, 255, 255), font=font)