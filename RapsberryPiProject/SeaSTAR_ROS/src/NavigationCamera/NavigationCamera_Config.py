# ---------------------------------------------------------------------------
#  File: NavigationCamera_Config.py
# ---------------------------------------------------------------------------
HOST = "192.168.2.2"
PORT = 8090

STREAM_VIDEO_DEVICE = "/dev/video0"
FRAMERATE = "30"
VIDEO_WIDTH = "1920"
VIDEO_HEIGHT = "1080"
BOUNDARY_TEXT = "ffmpeg"
BOUNDARY_BYTES = b"--" + BOUNDARY_TEXT.encode()
STREAM_TIMEOUT_SECONDS = 2
# File/folder settings
RECORDINGS_DIRECTORY = "/home/starfy/SeaSTAR_ROS/src/MissionRecordingLogs"
PHOTO_DIRECTORY = "/home/starfy/SeaSTAR_ROS/src/MissionPhotoLogs"
CAMERA_PATH = "/NavigationCamera"


RECORDING_NAME_FORMAT = "NavigationCamera_Stream_DATE:%Y%m%d_TIME:%H%M%S.mp4"
PHOTO_NAME_FORMAT = "NavigationCamera_Photo_DATE:%Y%m%d_TIME:%H%M%S.jpg"

DATE_TIME = "%Y-%m-%d %H:%M:%S"

# Recording overlay settings
OVERLAY_FONT = "Sans 24"
OVERLAY_SHADED_BACKGROUND = True
OVERLAY_HALIGNMENT = "right"
OVERLAY_VALIGNMENT = "top"

# H.264 encoder settings
ENCODER_TUNE = "zerolatency"
ENCODER_SPEED = "ultrafast"
ENCODER_BITRATE = 8000

# Photo timestamp settings
PHOTO_FONT = "DejaVuSans.ttf"
PHOTO_FONT_SIZE = 32
PHOTO_PADDING = 12
PHOTO_TEXT_BOX_PADDING = 8
PHOTO_JPEG_QUALITY = 90

