#!/usr/bin/env python3
# ---------------------------------------------------------------------------
# File: UartManager_node.py
# Author: Jose Machon
#
# Function:
# Top-level UART manager node for SeaSTAR
# Handles:
#   - UART driver init
#   - RX parsing
#   - TX management
#   - ROS interface (events + status)
# ---------------------------------------------------------------------------

import rclpy
from rclpy.node import Node
from std_msgs.msg import String

import time

from UartDriver import UartDriver
from UartReceiver import PacketParser
from UartTransmitter import TXManager


class UartManagerNode(Node):
    def __init__(self):
        super().__init__("UartManager_node")


        self.event_pub = self.create_publisher(
            String,
            "/SeaSTAR/event",
            10
        )

        self.status_pub = self.create_publisher(
            String,
            "/system_status/uart_communication",
            10
        )

        self.driver = UartDriver()
        self.parser = PacketParser(self.driver)
        self.tx_manager = TXManager(self.driver)

        self.uart_ok = False
        self.comm_ok = False

        self.last_rx_time = time.monotonic()

        self.hello_timer = self.create_timer(0.5, self.hello_callback)
        self.ping_timer = self.create_timer(1.0, self.ping_callback)
        self.rx_timer = self.create_timer(0.005, self.rx_callback)
        self.timeout_timer = self.create_timer(0.1, self.timeout_callback)

        # TX manager loop (thrusters, queued packets)
        self.tx_timer = self.create_timer(0.005, self.tx_callback)

        self.get_logger().info("UART Manager Node Started")

    def hello_callback(self):
        if not self.comm_ok:
            self.tx_manager.send_hello()

    def ping_callback(self):
        if self.comm_ok:
            self.tx_manager.send_ping()

    def rx_callback(self):
        packet = self.driver.receive_packet()

        if packet is not None:
            self.last_rx_time = time.monotonic()

            event = self.parser.parse(packet)

            # If parser returns event → publish
            if event:
                msg = String()
                msg.data = event
                self.event_pub.publish(msg)

            # Update connection status
            if self.parser.comm_established:
                self.comm_ok = True

            if self.driver.uart_ok:
                self.uart_ok = True


    def timeout_callback(self):
        now = time.monotonic()

        # Communication timeout
        if now - self.last_rx_time > 4.0:
            if self.comm_ok:
                self.get_logger().warn("UART COMM TIMEOUT")

            self.comm_ok = False

        # Publish system status
        status_msg = String()

        if self.uart_ok and self.comm_ok:
            status_msg.data = "UART_COMMUNICATION_OK"
        elif self.uart_ok:
            status_msg.data = "UART_LINK_ONLY"
        else:
            status_msg.data = "UART_DISCONNECTED"

        self.status_pub.publish(status_msg)

    def tx_callback(self):
        if self.comm_ok:
            self.tx_manager.process()


def main(args=None):
    rclpy.init(args=args)

    node = UartManagerNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass

    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()