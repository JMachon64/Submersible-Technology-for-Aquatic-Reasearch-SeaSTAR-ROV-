# ---------------------------------------------------------------------------
# File: FSM_node.py
# Author: Jose Machon
#
# ---------------------------------------------------------------------------

#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class FSMNode(Node):
    def __init__(self):
        super().__init__("FSM_node")


        self.state = "BOOT"


        self.event_sub = self.create_subscription(
            String,
            "/SeaSTAR/event",
            self.event_callback,
            10
        )

        self.state_pub = self.create_publisher(
            String,
            "/SeaSTAR/state",
            10
        )

        # Publish state periodically (for visibility / robustness)
        self.state_timer = self.create_timer(0.1, self.publish_state)

        self.get_logger().info("FSM Node Started in BOOT")

    def event_callback(self, msg):
        event = msg.data

        self.get_logger().info(f"EVENT RECEIVED: {event}")

        if self.state == "BOOT":

            if event == "UART_COMMUNICATION_OK":
                self.transition_to("IDLE")

            elif event == "FAILURE" or "LEAK" in event:
                self.transition_to("FAILURE")

        elif self.state == "IDLE":

            if event == "START_MISSION":
                self.transition_to("ACTIVE")

            elif event == "UART_DISCONNECTED":
                self.transition_to("BOOT")

            elif event == "FAILURE" or "LEAK" in event:
                self.transition_to("FAILURE")

        elif self.state == "ACTIVE":

            if event == "END_MISSION":
                self.transition_to("IDLE")

            elif event == "UART_DISCONNECTED":
                self.transition_to("BOOT")

            elif event == "FAILURE" or "LEAK" in event:
                self.transition_to("FAILURE")

        elif self.state == "FAILURE":

            if event == "RESET":
                self.transition_to("BOOT")

    def transition_to(self, new_state):
        if new_state != self.state:
            self.get_logger().warn(f"STATE CHANGE: {self.state} -> {new_state}")
            self.state = new_state

    def publish_state(self):
        msg = String()
        msg.data = self.state
        self.state_pub.publish(msg)


def main(args=None):
    rclpy.init(args=args)

    node = FSMNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass

    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()