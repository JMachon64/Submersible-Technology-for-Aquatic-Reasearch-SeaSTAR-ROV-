# ---------------------------------------------------------------------------
#  File: NavigationCameraManager_node.py
#  Author: Jose Machon
#  Date: 4/2/2026 1:20pm
#
#  Function: 
#  Handles all high level maning of the nav camera, check status and inits 
#  all needed functions in the camera pipeline 
# 
# ---------------------------------------------------------------------------

#!/usr/bin/env python3

#  Imports 
import signal

import rclpy
from rclpy.node import Node
from std_msgs.msg import String

from NavigationCamera_Pipeline import CameraPipeline

#  Node object 
class NavigationCameraManagerNode(Node):
    def __init__(self):
        super().__init__("NavigationCameraManager_node")

        self.system_status_pub = self.create_publisher(
            String,
            "/system_status/navigation_camera",
            10
        )

        self.camera_pipeline = CameraPipeline(self.get_logger())
        self.camera_pipeline.start()

        self.last_status = None

        self.status_timer = self.create_timer(0.5, self.publish_status)

        self.get_logger().info("NavigationCameraManager_node initialized")

    def publish_status(self):
        status = "STREAMING" if self.camera_pipeline.is_active() else "OFFLINE"

        msg = String()
        msg.data = status
        self.system_status_pub.publish(msg)

        if status != self.last_status:
            self.get_logger().info(
                f"Published navigation camera status: {status}"
            )
            self.last_status = status


    def destroy_node(self):
        self.get_logger().info("Shutting down NavigationCameraManager_node...")
        self.camera_pipeline.stop()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = NavigationCameraManagerNode()

    def handle_signal(signum, frame):
        if rclpy.ok():
            rclpy.shutdown()

    signal.signal(signal.SIGINT, handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()

        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main() 