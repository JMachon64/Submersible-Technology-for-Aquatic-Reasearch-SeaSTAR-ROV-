#!/usr/bin/env python3

import threading
import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class CameraFakeNode(Node):
    def __init__(self):
        super().__init__("camerafake_node")

        self.event_pub = self.create_publisher(String, 
            "/starfy/event", 
            10
        )
        self.status_pub = self.create_publisher(
            String,
            '/status/camerafake',
            10
        )

        self.status_timer = self.create_timer(
            0.5,
            self.publish_status
        )
        self.camera_on = False

        self.get_logger().info("camerafake_node started")
        self.get_logger().info("Press:")
        self.get_logger().info("  c = toggle camera state")
        self.get_logger().info("  q = quit")

        self.key_thread = threading.Thread(target=self.keyboard_loop, daemon=True)
        self.key_thread.start()

    def publish_event(self, event_name: str):
        msg = String()
        msg.data = event_name
        self.event_pub.publish(msg)
        self.get_logger().info(f"Published event: {event_name}")
    def publish_status(self):
        msg = String()
        msg.data = "ALIVE"
        self.status_pub.publish(msg)
    def keyboard_loop(self):
        while rclpy.ok():
            key = input().strip().lower()

            if key == "c":
                self.camera_on = not self.camera_on

                if self.camera_on:
                    self.get_logger().info("Camera toggled ON")
                    self.publish_event("CAMERA_ACTIVE")
                else:
                    self.get_logger().info("Camera toggled OFF")
                    self.publish_event("CAMERA_OFF")

            elif key == "q":
                self.get_logger().info("Exiting camerafake_node...")
                rclpy.shutdown()
                break

            else:
                self.get_logger().info("Unknown key. Use 'c' to toggle, 'q' to quit.")


def main(args=None):
    rclpy.init(args=args)
    node = CameraFakeNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass

    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()