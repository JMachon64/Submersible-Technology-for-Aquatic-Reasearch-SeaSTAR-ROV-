#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32MultiArray


class TelemetryMonitor(Node):

    def __init__(self):

        super().__init__("telemetry_monitor")


        self.env_sub = self.create_subscription(
            Float32MultiArray,
            "/uart/telemetry_environmental",
            self.environmental_callback,
            10
        )

        self.pos_sub = self.create_subscription(
            Float32MultiArray,
            "/uart/telemetry_positional",
            self.positional_callback,
            10
        )

        self.pow_sub = self.create_subscription(
            Float32MultiArray,
            "/uart/telemetry_power",
            self.power_callback,
            10
        )

        self.get_logger().info("Telemetry Monitor Started")

    def environmental_callback(self, msg):

        if len(msg.data) < 3:
            self.get_logger().warn("Invalid environmental packet")
            return

        temp = msg.data[0]
        depth = msg.data[1]
        pressure = msg.data[2]
        time = msg.data[3]

        self.get_logger().info(
            f"[ENV] "
            f"Temp: {temp:.3f}C     |"
            f"Depth: {depth:.3f}m   | "
            f"Press: {pressure:.3f}Pa |"
            f"Time: {time:.3f}"
        )

    def positional_callback(self, msg):

        if len(msg.data) < 3:
            self.get_logger().warn("Invalid positional packet")
            return

        roll = msg.data[0]
        pitch = msg.data[1]
        yaw = msg.data[2]
        time = msg.data[3]

        self.get_logger().info(
            f"[POS] "
            f"Roll: {roll:.3f}      | "
            f"Pitch: {pitch:.3f}    | "
            f"Yaw: {yaw:.3f}        |" 
            f"Time: {time:.3f}"
        )
    def power_callback(self, msg):

        if len(msg.data) < 3:
            self.get_logger().warn("Invalid positional packet")
            return

        voltage = msg.data[0]
        current = msg.data[1]
        power = msg.data[2]
        time = msg.data[3]

        self.get_logger().info(
            f"[POW] "
            f"Voltage: {voltage:.3f}    |"
            f"Current: {current:.3f}   | "
            f"Power: {power:.3f}       | "
            f"Time: {time:.3f}"
        )


def main(args=None):

    rclpy.init(args=args)

    node = TelemetryMonitor()

    try:
        rclpy.spin(node)

    except KeyboardInterrupt:
        pass

    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()