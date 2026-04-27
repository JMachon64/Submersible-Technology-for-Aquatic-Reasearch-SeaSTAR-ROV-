
HOST = "192.168.2.2"
PORT = 8090

VIDEO_DEVICE = "/dev/video0"

FRAMERATE = "30"
VIDEO_SIZE = "1920x1080"
INPUT_FORMAT = "mjpeg"

STATUS_OFFLINE = "offline"
STATUS_ACTIVE = "active"

BOUNDARY_TEXT = "ffmpeg"
BOUNDARY_BYTES = b"--" + BOUNDARY_TEXT.encode()


def make_ffmpeg_cmd():
    return [
        "ffmpeg",
        "-hide_banner",
        "-loglevel", "warning",
        "-thread_queue_size", "2",
        "-f", "v4l2",
        "-input_format", INPUT_FORMAT,
        "-framerate", FRAMERATE,
        "-video_size", VIDEO_SIZE,
        "-i", VIDEO_DEVICE,
        "-an",
        "-c:v", "copy",
        "-f", "mpjpeg",
        "-"
    ]