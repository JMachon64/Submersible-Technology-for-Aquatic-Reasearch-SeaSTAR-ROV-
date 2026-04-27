#!/usr/bin/env python3

import struct

import rclpy
from rclpy.node import Node
from std_msgs.msg import Int16MultiArray, UInt8MultiArray, String

from Uart.UartProtocol import *

class ControlLoopNode(Node):
    def __init__(self):
        super().__init__("ControlLoop_node")

        self.thruster_values = [0, 0, 0, 0, 0]

        self.last_log_time = 0.0
        self.current_state = "ACTIVE"

        self.next_command_seq = 0

        self.controller_stream = self.create_subscription(
            Int16MultiArray,
            "/thruster/commands",
            self.thruster_callback,
            10
        )

        self.state_sub = self.create_subscription(String, "/SeaSTAR/state", self.state_callback, 10)

        self.event_sub = self.create_subscription(String, "/mission/control", self.event_command_callback, 10)

        self.thruster_pub = self.create_publisher(UInt8MultiArray, "/uart/thruster_tx_request", 10)

        self.command_pub = self.create_publisher( UInt8MultiArray, "/uart/command_tx_request", 10)

        self.event_pub = self.create_publisher( UInt8MultiArray, "/uart/event_tx_request", 10)

        # 100 Hz control loop
        self.control_timer = self.create_timer(0.005, self.control_loop)

        self.get_logger().info("ControlLoop_node initialized at 100 Hz")

    def get_next_command_seq(self):
        seq = self.next_command_seq
        self.next_command_seq = (self.next_command_seq + 1) & 0xFFFFFFFF
        return seq

    def state_callback(self, msg: String):
        previous_state = self.current_state
        self.current_state = msg.data.strip().upper()

        blocked_now = self.current_state in ("BOOT", "IDLE", "FAILURE")
        blocked_before = previous_state in ("BOOT", "IDLE", "FAILURE")

        if blocked_now and not blocked_before:
            self.publish_thruster_packet([0, 0, 0, 0, 0])

            if self.current_state == "FAILURE":
                self.publish_event_packet(ID_MISSION_FAILURE)

            self.get_logger().warn(f"Thrusters blocked in state={self.current_state}, sent zero command")

    def event_command_callback(self, msg: String):
        command = msg.data.strip().upper()

        if command == "COLLECT_WATER_SAMPLE":
            self.publish_command_packet(ID_COLLECT_WATER_SAMPLE)
            self.get_logger().info("Sent COLLECT_WATER_SAMPLE packet request")

        elif command == "START_MISSION":
            self.publish_event_packet(ID_START_MISSION)
            self.get_logger().info("Sent START_MISSION packet request")

        elif command == "END_MISSION":
            self.publish_event_packet(ID_END_MISSION)
            self.get_logger().info("Sent END_MISSION packet request")

        elif command == "FAILURE":
            self.publish_event_packet(ID_MISSION_FAILURE)
            self.get_logger().warn("Sent FAILURE_EVENT packet request")

        else:
            self.get_logger().warn(f"Unknown mission command: {command}")

    def publish_command_packet(self, packet_id: int):
        seq = self.get_next_command_seq()

        # Payload = uint32 sequence number
        payload = struct.pack("<I", seq)

        packet_msg = UInt8MultiArray()
        packet_msg.data = [packet_id] + list(payload)

        self.command_pub.publish(packet_msg)

        self.get_logger().info(f"Published command packet id=0x{packet_id:02X}, seq={seq}")

    def publish_event_packet(self, packet_id: int):
        seq = self.get_next_command_seq()

        # Payload = uint32 sequence number
        payload = struct.pack("<I", seq)

        packet_msg = UInt8MultiArray()
        packet_msg.data = [packet_id] + list(payload)

        self.event_pub.publish(packet_msg)

        self.get_logger().info(
            f"Published event packet id=0x{packet_id:02X}, seq={seq}"
        )

    def thruster_callback(self, msg: Int16MultiArray):
        if len(msg.data) != 5:
            self.get_logger().warn(f"Expected 5 thruster values, got {len(msg.data)}")
            return

        self.thruster_values = [
            max(-1000, min(1000, int(v * 10))) for v in msg.data
        ]

    def publish_thruster_packet(self, values):
        payload = struct.pack("<hhhhh", *values)

        packet_msg = UInt8MultiArray()
        packet_msg.data = [ID_THRUSTER_INPUT] + list(payload)

        self.thruster_pub.publish(packet_msg)

    def control_loop(self):
        now = self.get_clock().now().nanoseconds / 1e9

        if self.current_state in ("BOOT", "IDLE", "FAILURE"):
            if now - self.last_log_time >= 1.0:
                self.get_logger().warn(
                    f"Thruster output blocked because state={self.current_state}"
                )
                self.last_log_time = now
            return

        self.publish_thruster_packet(self.thruster_values)

        if now - self.last_log_time >= 1.0:
            self.get_logger().info(
                f"TX 100Hz = {self.thruster_values} | state={self.current_state}"
            )
            self.last_log_time = now


def main(args=None):
    rclpy.init(args=args)
    node = ControlLoopNode()

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