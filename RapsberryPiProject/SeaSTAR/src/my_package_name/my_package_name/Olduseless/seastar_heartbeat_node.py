#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from datetime import datetime
from zoneinfo import ZoneInfo
from std_msgs.msg import String


class SeaStarHeartbeatNode(Node):
    def __init__(self):
        super().__init__('seastar_heartbeat_node')

        self.pub = self.create_publisher(String, 
            '/uart_tasks/heartbeat', 
            10
        )

        self.state_sub = self.create_subscription(
            String,
            '/starfy/state',
            self.state_callback,
            10
        )

        self.current_state = "BOOT"
        self.heartbeat_enabled = False

        self.timer = self.create_timer(1.0, self.send_heartbeat)

        self.get_logger().info("Heartbeat node started")

    def state_callback(self, msg):
        old_enabled = self.heartbeat_enabled
        self.current_state = msg.data

        # Enable heartbeat once outside BOOT
        self.heartbeat_enabled = (self.current_state != "BOOT")

        if old_enabled != self.heartbeat_enabled:
            self.get_logger().info(
                f"Heartbeat enabled = {self.heartbeat_enabled} (state={self.current_state})"
            )

    def send_heartbeat_pulse(self):
        if not self.heartbeat_enabled:
            return

        now_ca = datetime.now(ZoneInfo("America/Los_Angeles"))
        timestamp_str = now_ca.strftime("%Y-%m-%d %H:%M:%S.%f %Z")

        msg = String()
        msg.data = timestamp_str
        self.pub.publish(msg)

        self.get_logger().info(f"Published heartbeat timestamp: {timestamp_str}")

def main(args=None):
    rclpy.init(args=args)
    node = SeaStarHeartbeatNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()