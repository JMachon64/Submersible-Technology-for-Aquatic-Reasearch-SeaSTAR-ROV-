import rclpy
from rclpy.node import Node
from std_msgs.msg import String
from rosbridge_msgs.msg import ConnectedClients


class UIStatusNode(Node):
    def __init__(self):
        super().__init__("ui_status_node")

        self.ui_status_pub = self.create_publisher(
            String,
            "/system_status/ui",
            10
        )

        self.connected_clients_sub = self.create_subscription(
            ConnectedClients,
            "/connected_clients",
            self.connected_clients_callback,
            10
        )

    def connected_clients_callback(self, msg):
        status_msg = String()

        if len(msg.clients) > 0:
            status_msg.data = "UI_CONNECTED"
        else:
            status_msg.data = "UI_DISCONNECTED"

        self.ui_status_pub.publish(status_msg)


def main(args=None):
    rclpy.init(args=args)
    node = UIStatusNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()