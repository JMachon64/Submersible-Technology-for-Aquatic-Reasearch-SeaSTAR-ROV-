#!/usr/bin/env python3

import signal
import threading

import rclpy
from rclpy.node import Node
from std_msgs.msg import String

from NavigationCamera_Pipeline import CameraPipeline


class NavigationCameraManagerNode(Node):
    def __init__(self):
        super().__init__("NavigationCameraManager_node")

        self.system_status_pub = self.create_publisher(
            String,
            "/system_status/navigation_camera",
            10
        )

        self.command_sub = self.create_subscription(
            String,
            "/navigation_camera/command",
            self.handle_camera_command,
            10
        )

        self.camera_pipeline = CameraPipeline(self.get_logger())
        self.camera_pipeline.start()

        self.last_status = None
        self.status_timer = self.create_timer(0.5, self.publish_status)

        self.keyboard_thread = threading.Thread(
            target=self.keyboard_loop,
            daemon=True
        )
        self.keyboard_thread.start()

        self.get_logger().info("NavigationCameraManager_node initialized")
        self.get_logger().info("Keyboard controls: s=start rec, x=stop rec, p=photo, q=quit")

    def publish_status(self):
        status = "STREAMING" if self.camera_pipeline.is_active() else "OFFLINE"

        msg = String()
        msg.data = status
        self.system_status_pub.publish(msg)

        if status != self.last_status:
            self.get_logger().info(f"Published navigation camera status: {status}")
            self.last_status = status

    def handle_camera_command(self, msg):
        command = msg.data.strip().upper()

        if command in ["START RECORDING", "START_RECORDING", "START_RECORDING_VIDEO"]:
            self.camera_pipeline.start_recording()

        elif command in ["STOP RECORDING", "STOP_RECORDING", "STOP_RECORDING_VIDEO"]:
            self.camera_pipeline.stop_recording()

        elif command in ["TAKE PHOTO", "TAKE_PHOTO", "CAPTURE_PHOTO"]:
            self.camera_pipeline.take_photo()

        else:
            self.get_logger().warning(f"Unknown camera command: {command}")

    def keyboard_loop(self):
        while rclpy.ok():
            try:
                key = input("Camera command [s=start, x=stop, p=photo, q=quit]: ").strip().lower()

                if key == "s":
                    self.get_logger().info("Keyboard: START RECORDING")
                    self.camera_pipeline.start_recording()

                elif key == "x":
                    self.get_logger().info("Keyboard: STOP RECORDING")
                    self.camera_pipeline.stop_recording()

                elif key == "p":
                    self.get_logger().info("Keyboard: TAKE PHOTO")
                    self.camera_pipeline.take_photo()

                elif key == "q":
                    self.get_logger().info("Keyboard: QUIT")
                    rclpy.shutdown()
                    break

                else:
                    self.get_logger().info("Unknown key. Use s, x, p, or q.")

            except EOFError:
                break

    def destroy_node(self):
        self.get_logger().info("Shutting down NavigationCameraManager_node...")
        self.camera_pipeline.stop_pipeline()
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