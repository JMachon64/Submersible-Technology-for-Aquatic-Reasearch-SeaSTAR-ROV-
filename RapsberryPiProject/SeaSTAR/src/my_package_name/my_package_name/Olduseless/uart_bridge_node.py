#!/usr/bin/env python3
import threading
import struct
import rclpy
from rclpy.node import Node
from std_msgs.msg import String, Int16MultiArray


class UARTBridgeNode(Node):
    def __init__(self):

        super().__init__('uart_bridge_node')

        self.connected = False

        # Functional event publisher to FSM
        self.event_pub = self.create_publisher(
            String,
            '/starfy/event',
            10
        )

        # Node-health publisher
        self.status_pub = self.create_publisher(
            String,
            '/status/uart_bridge',
            10
        )

        # Subscribers
        self.heartbeat_sub = self.create_subscription(
            String,
            '/uart_tasks/heartbeat',
            self.heartbeat_callback,
            10
        )

        self.thruster_sub = self.create_subscription(
            Int16MultiArray,
            '/thruster/commands',
            self.thruster_callback,
            10
        )

        # Publish node liveness every 0.5 s
        self.status_timer = self.create_timer(
            0.5,
            self.publish_status
        )

        self.get_logger().info("uart_bridge_node started")
        self.get_logger().info("Press:")
        self.get_logger().info("  c = toggle UART connected/disconnected")
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

    def safe_disconnect(self, reason: str = ""):
        if self.connected:
            self.connected = False
            if reason:
                self.get_logger().error(f"UART DISCONNECTED: {reason}")
            else:
                self.get_logger().error("UART DISCONNECTED")
            self.publish_event("DISCONNECTED")

    def keyboard_loop(self):
        while rclpy.ok():
            try:
                key = input().strip().lower()
            except EOFError:
                self.safe_disconnect("stdin closed")
                rclpy.shutdown()
                break

            if key == 'c':
                self.connected = not self.connected

                if self.connected:
                    self.get_logger().info("UART CONNECTED")
                    self.publish_event("CONNECTED")
                else:
                    self.get_logger().info("UART DISCONNECTED")
                    self.publish_event("DISCONNECTED")

            elif key == 'q':
                self.get_logger().info("Exiting uart_bridge_node...")
                self.safe_disconnect("user quit")
                rclpy.shutdown()
                break

    def heartbeat_callback(self, msg: String):
        try:
            if not self.connected:
                self.get_logger().info("Heartbeat received but UART is disconnected")
                return

            self.get_logger().info(f"Heartbeat received: {msg.data}")

            # later:
            # convert msg.data timestamp into payload
            # build ping packet
            # send to serial

        except Exception as e:
            self.safe_disconnect(f"heartbeat callback error: {e}")
            raise

    def thruster_callback(self, msg: Int16MultiArray):
        try:
            if not self.connected:
                self.get_logger().info("Thruster command received but UART is disconnected")
                return

            if len(msg.data) != 5:
                self.get_logger().error(
                    f"Expected 5 thruster values, got {len(msg.data)}"
                )
                return

            j1x, j1y, j2x, j2y, trig = msg.data

            payload = struct.pack('<hhhhb', j1x, j1y, j2x, j2y, trig)

            self.get_logger().info(
                f"Thruster values: {[j1x, j1y, j2x, j2y, trig]} | "
                f"payload_len={len(payload)} | payload_hex={payload.hex()}"
            )

            # later:
            # packet = self.build_packet(packet_id=0x03, payload=payload)
            # self.serial.write(packet)

        except Exception as e:
            self.safe_disconnect(f"thruster callback error: {e}")
            raise


def main(args=None):
    rclpy.init(args=args)
    node = UARTBridgeNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.safe_disconnect("keyboard interrupt")
    except Exception as e:
        node.safe_disconnect(f"fatal error: {e}")
        raise
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()