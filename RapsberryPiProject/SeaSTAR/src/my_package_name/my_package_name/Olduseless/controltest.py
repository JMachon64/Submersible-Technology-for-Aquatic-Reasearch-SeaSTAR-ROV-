#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import threading


class ControlTestNode(Node):
    def __init__(self):
        super().__init__('controltest_node')

        self.pub = self.create_publisher(String, 
            '/starfy/event', 
            10
        )
        
        self.get_logger().info("Control Test Node Started")
        self.get_logger().info("Press:")
        self.get_logger().info("  s = START_MISSION")
        self.get_logger().info("  e = END_MISSION")
        self.get_logger().info("  q = quit")

        # run keyboard input in separate thread
        self.thread = threading.Thread(target=self.keyboard_loop, daemon=True)
        self.thread.start()

    def publish_event(self, event_str):
        msg = String()
        msg.data = event_str
        self.pub.publish(msg)
        self.get_logger().info(f"Published event: {event_str}")

    def keyboard_loop(self):
        while rclpy.ok():
            key = input().strip().lower()

            if key == 's':
                self.publish_event("START_MISSION")

            elif key == 'e':
                self.publish_event("END_MISSION")

            elif key == 'q':
                self.get_logger().info("Exiting control node...")
                rclpy.shutdown()
                break


def main(args=None):
    rclpy.init(args=args)
    node = ControlTestNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass

    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()