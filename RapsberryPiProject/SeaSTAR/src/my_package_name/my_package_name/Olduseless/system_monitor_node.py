#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class SystemMonitorNode(Node):
    def __init__(self):
        super().__init__("system_monitor_node")

        self.event_pub = self.create_publisher(
            String,
            "/starfy/event",
            10
        )

        # Last heartbeat times
        self.last_uart_status_time = None
        self.last_gamecontroller_status_time = None
        self.last_camerafake_status_time = None

        # Online/offline tracking to avoid spamming repeated events
        self.uart_alive = False
        self.gamecontroller_alive = False
        self.camerafake_alive = False

        # Timeouts
        self.uart_timeout_sec = 1.5
        self.gamecontroller_timeout_sec = 1.5
        self.camerafake_timeout_sec = 1.5

        self.uart_status_sub = self.create_subscription(
            String,
            "/status/uart_bridge",
            self.uart_status_callback,
            10
        )

        self.gamecontroller_status_sub = self.create_subscription(
            String,
            "/status/gamecontroller",
            self.gamecontroller_status_callback,
            10
        )

        self.camerafake_status_sub = self.create_subscription(
            String,
            "/status/camerafake",
            self.camerafake_status_callback,
            10
        )

        self.watchdog_timer = self.create_timer(
            0.2,
            self.check_watchdogs
        )

        self.get_logger().info("system_monitor_node started")

    def publish_event(self, event_name):
        msg = String()
        msg.data = event_name
        self.event_pub.publish(msg)
        self.get_logger().info(f"Published event: {event_name}")

    def uart_status_callback(self, msg):
        self.last_uart_status_time = self.get_clock().now()
        self.uart_alive = True

    def gamecontroller_status_callback(self, msg):
        self.last_gamecontroller_status_time = self.get_clock().now()
        self.gamecontroller_alive = True

    def camerafake_status_callback(self, msg):
        self.last_camerafake_status_time = self.get_clock().now()
        self.camerafake_alive = True

    def check_watchdogs(self):
        now = self.get_clock().now()

        if self.last_uart_status_time is not None:
            age_sec = (now - self.last_uart_status_time).nanoseconds / 1e9
            if age_sec > self.uart_timeout_sec and self.uart_alive:
                self.uart_alive = False
                self.get_logger().error(f"UART watchdog timeout ({age_sec:.2f}s)")
                self.publish_event("DISCONNECTED")

        if self.last_gamecontroller_status_time is not None:
            age_sec = (now - self.last_gamecontroller_status_time).nanoseconds / 1e9
            if age_sec > self.gamecontroller_timeout_sec and self.gamecontroller_alive:
                self.gamecontroller_alive = False
                self.get_logger().error(f"Gamecontroller watchdog timeout ({age_sec:.2f}s)")
                self.publish_event("GAMECONTROLLER_DISCONNECTED")

        if self.last_camerafake_status_time is not None:
            age_sec = (now - self.last_camerafake_status_time).nanoseconds / 1e9
            if age_sec > self.camerafake_timeout_sec and self.camerafake_alive:
                self.camerafake_alive = False
                self.get_logger().error(f"Camerafake watchdog timeout ({age_sec:.2f}s)")
                self.publish_event("CAMERA_OFF")


def main(args=None):
    rclpy.init(args=args)
    node = SystemMonitorNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass

    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()