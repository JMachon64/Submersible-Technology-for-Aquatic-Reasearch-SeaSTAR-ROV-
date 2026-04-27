import rclpy
from rclpy.node import Node
from std_msgs.msg import String

BOOT = "BOOT"
IDLE = "IDLE"
ACTIVE = "ACTIVE"
FAILURE = "FAILURE"


class SeaSTAR_FSM_Node(Node):
    def __init__(self):
        super().__init__("mission_fsm_node")

        self.state = BOOT

        self.camera_active = False
        self.uart_connected = False
        self.gamecontroller_connected = False

        self.state_pub = self.create_publisher(
            String,
            "/starfy/state",
            10
        )

        self.event_sub = self.create_subscription(
            String,
            "/starfy/event",
            self.event_callback,
            10
        )

        self.publish_state()
        self.get_logger().info("mission_fsm_node started")

    def publish_state(self):
        msg = String()
        msg.data = self.state
        self.state_pub.publish(msg)
        self.get_logger().info(f"State -> {self.state}")

    def all_systems_ready(self):
        return (
            self.camera_active and
            self.uart_connected and
            self.gamecontroller_connected
        )

    def event_callback(self, msg):
        event = msg.data
        old_state = self.state

        self.get_logger().info(f"Received event: {event}")

        if event == "CAMERA_ACTIVE":
            self.camera_active = True
        elif event == "CAMERA_OFF":
            self.camera_active = False
        elif event == "CONNECTED":
            self.uart_connected = True
        elif event == "DISCONNECTED":
            self.uart_connected = False
        elif event == "GAMECONTROLLER_CONNECTED":
            self.gamecontroller_connected = True
        elif event == "GAMECONTROLLER_DISCONNECTED":
            self.gamecontroller_connected = False

        self.get_logger().info(
            "Status: "
            f"camera_active={self.camera_active}, "
            f"uart_connected={self.uart_connected}, "
            f"gamecontroller_connected={self.gamecontroller_connected}"
        )

        if self.state == BOOT:
            if self.all_systems_ready():
                self.state = IDLE

        elif self.state == IDLE:
            if event == "START_MISSION":
                self.state = ACTIVE
            elif event in ["CAMERA_OFF", "DISCONNECTED", "GAMECONTROLLER_DISCONNECTED"]:
                self.state = FAILURE

        elif self.state == ACTIVE:
            if event == "END_MISSION":
                self.state = IDLE
            elif event in ["CAMERA_OFF", "DISCONNECTED", "GAMECONTROLLER_DISCONNECTED"]:
                self.state = FAILURE

        elif self.state == FAILURE:
            if self.all_systems_ready():
                self.state = IDLE

        if self.state != old_state:
            self.publish_state()
        else:
            self.get_logger().info(
                f"Ignored event '{event}' while in state {self.state}"
            )


def main(args=None):
    rclpy.init(args=args)
    node = SeaSTAR_FSM_Node()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass

    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()