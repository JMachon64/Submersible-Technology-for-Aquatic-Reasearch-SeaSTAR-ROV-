# ---------------------------------------------------------------------------
# File: UartManager_node.py
# Author: Jose Machon
#
# Function:
# Top-level UART manager node for SeaSTAR
# ---------------------------------------------------------------------------

#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import String, UInt8MultiArray, Float32MultiArray
import struct
import time

from std_msgs.msg import Bool
from UartDriver import UARTDriver
from UartReceiver import PacketParser
from UartTransmitter import TXManager
from UartProtocol import *

class UartManagerNode(Node):

    def __init__(self):

        super().__init__("UartManager_node")

        self.event_pub  = self.create_publisher(String, "/SeaSTAR/event",                    10)
        self.status_pub = self.create_publisher(String, "/system_status/uart_communication", 10)

        self.thrusterupdate = self.create_subscription(UInt8MultiArray, "/uart/thruster_tx_request", self.thruster_callback, 10)
        self.commandupdate  = self.create_subscription(UInt8MultiArray, "/uart/command_tx_request",  self.command_callback,  10)

        self.leak_pub  =  self.create_publisher(Bool,                "/uart/leak_sensor",              10)
        self.envs_pub  =  self.create_publisher(Float32MultiArray,   "/uart/telemetry_environmental",  10)
        self.pos_pub   =  self.create_publisher(Float32MultiArray,   "/uart/telemetry_positional",     10)
        self.pow_pub   =  self.create_publisher(Float32MultiArray,   "/uart/telemetry_power",          10)
        self.curr_pub  =  self.create_publisher(Bool,                "/uart/current_spike",            10)
        self.sample_pub = self.create_publisher(Float32MultiArray,   "/uart/sample_telemetry",          10)

        self.uart_ok = False
        self.comm_ok = False

        self.last_rx_time = time.monotonic()

        self.driver = UARTDriver(
            on_event = self.publish_uart_event,
            on_status= self.handle_uart_status,
            logger   = self.get_logger()
        )

        self.tx_manager = TXManager(
            self.driver,
            self.get_logger()
        )

        self.parser = PacketParser(

            self.driver,
            self.get_logger(),
            self.leak_pub,
            self.envs_pub,
            self.pos_pub,
            self.pow_pub,
            self.curr_pub,
            self.event_pub,
            self.sample_pub

        )

        self.parser.tx = self.tx_manager
        self.driver.open_serial()
        self.tx_manager.start_tx_thread()
        self.uart_ok = self.driver.uart_ok
        self.comm_ok = self.driver.comm_ok

        # TIMERS 
        self.hello_timer     = self.create_timer(HELLO_PERIOD,                 self.hello_callback)
        self.ping_timer      = self.create_timer(PING_PERIOD,                  self.ping_callback)
        self.rx_timer        = self.create_timer(RX_PERIOD,                    self.rx_callback)
        self.timeout_timer   = self.create_timer(COMMUNICATION_TIMEOUT_PERIOD, self.timeout_callback)

        self.get_logger().info("UART Manager Node Started")

    def publish_uart_event(self, event_text):

        msg = String()
        msg.data = event_text
        self.event_pub.publish(msg)
        self.get_logger().info(f"Published UART event: {event_text}")

    def handle_uart_status(self, connected):

        self.uart_ok = self.driver.uart_ok
        self.comm_ok = self.driver.comm_ok

    def hello_callback(self):

        if not self.comm_ok:
            self.tx_manager.send_hello()

    def ping_callback(self):

        if self.comm_ok:
            self.tx_manager.send_ping()

    def thruster_callback(self, msg):

        packetID = msg.data[0]
        payload = bytes(msg.data[1:])

        self.tx_manager.set_thruster_command(payload)

    def command_callback(self, msg):
        
        packetID = msg.data[0]
        payload = bytes(msg.data[1:])
        self.get_logger().info(f"Published UART event: {packetID}")
        self.tx_manager.queue_packet(packetID, payload)

    def rx_callback(self):

        packet = self.driver.read_packet()

        if packet is None:
            return

        self.last_rx_time = time.monotonic()

        packetID, payload = packet
        self.parser.handle_packet(packetID, payload)

        self.uart_ok = self.driver.uart_ok
        self.comm_ok = self.driver.comm_ok

    def timeout_callback(self):
        now = time.monotonic()

        if now - self.last_rx_time > 4.0:
            if self.comm_ok:
                
                self.get_logger().warn("UART COMM TIMEOUT")
                self.publish_uart_event("UART_COMMUNICATION_LOST")

            self.comm_ok = False
            self.driver.comm_ok = False

        status_msg = String()

        if self.uart_ok and self.comm_ok:
            status_msg.data = "UART_CONNECTED"
        elif self.uart_ok:
            status_msg.data = "UART_LINK_ONLY"
        else:
            status_msg.data = "UART_DISCONNECTED"

        self.status_pub.publish(status_msg)

    def destroy_node(self):
        self.driver.stop()
        super().destroy_node()


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
