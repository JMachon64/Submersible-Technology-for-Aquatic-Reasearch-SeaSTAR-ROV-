#!/usr/bin/env python3

import threading
import rclpy
from rclpy.node import Node
from std_msgs.msg import String, Int16MultiArray


class GameControllerNode(Node):
    def __init__(self):
        super().__init__("gamecontroller_node")

        self.state = "BOOT"
        self.thrusters_enabled = False
        self.controller_connected = False

        self.state_sub = self.create_subscription(
            String,
            "/starfy/state",
            self.state_callback,
            10
        )

        self.thruster_pub = self.create_publisher(
            Int16MultiArray,
            "/thruster/command",
            10
        )

        self.event_pub = self.create_publisher(
            String,
            "/starfy/event",
            10
        )
        self.status_pub = self.create_publisher(
            String,
            '/status/gamecontroller',
            10
        )

        self.status_timer = self.create_timer(
            0.5,
            self.publish_status
        )
        self.timer = self.create_timer(0.02, self.control_loop)  # 50 Hz

        self.key_thread = threading.Thread(target=self.keyboard_loop, daemon=True)
        self.key_thread.start()

        self.get_logger().info("gamecontroller_node started")
        self.get_logger().info("Press:")
        self.get_logger().info("  g = toggle game controller connected/disconnected")
        self.get_logger().info("  q = quit")

    def publish_event(self, event_name):
        msg = String()
        msg.data = event_name
        self.event_pub.publish(msg)
        self.get_logger().info(f"Published event: {event_name}")
        
    def publish_status(self):
        msg = String()
        msg.data = "ALIVE"
        self.status_pub.publish(msg)

    def state_callback(self, msg):
        old_enabled = self.thrusters_enabled
        self.state = msg.data

        self.thrusters_enabled = (
            self.state == "ACTIVE" and self.controller_connected
        )

        if old_enabled != self.thrusters_enabled:
            self.get_logger().info(
                f"Thrusters enabled = {self.thrusters_enabled} "
                f"(state={self.state}, controller_connected={self.controller_connected})"
            )

    def keyboard_loop(self):
        while rclpy.ok():
            key = input().strip().lower()

            if key == "g":
                self.controller_connected = not self.controller_connected

                if self.controller_connected:
                    self.get_logger().info("Game controller CONNECTED")
                    self.publish_event("GAMECONTROLLER_CONNECTED")
                else:
                    self.get_logger().info("Game controller DISCONNECTED")
                    self.publish_event("GAMECONTROLLER_DISCONNECTED")

                old_enabled = self.thrusters_enabled
                self.thrusters_enabled = (
                    self.state == "ACTIVE" and self.controller_connected
                )

                if old_enabled != self.thrusters_enabled:
                    self.get_logger().info(
                        f"Thrusters enabled = {self.thrusters_enabled} "
                        f"(state={self.state}, controller_connected={self.controller_connected})"
                    )

            elif key == "q":
                self.get_logger().info("Exiting gamecontroller_node...")
                rclpy.shutdown()
                break

    def control_loop(self):
        if not self.thrusters_enabled:
            return

        # Example values
        j1x = 100
        j1y = 200
        j2x = -150
        j2y = 50
        trig = 1   # keep as int16 here for ROS transport simplicity

        msg = Int16MultiArray()
        msg.data = [j1x, j1y, j2x, j2y, trig]
        self.thruster_pub.publish(msg)

        self.get_logger().info(f"Published thruster command: {list(msg.data)}")


def main(args=None):
    rclpy.init(args=args)
    node = GameControllerNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()