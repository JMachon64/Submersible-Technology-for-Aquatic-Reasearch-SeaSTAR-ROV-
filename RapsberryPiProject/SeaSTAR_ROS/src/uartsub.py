import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32MultiArray, Float32, Bool
import random


class UISensorBridge(Node):
    def __init__(self):
        super().__init__("ui_sensor_bridge")

        self.uart_sub = self.create_subscription(
            Float32MultiArray,
            "/uart/sensors",
            self.uart_callback,
            10
        )

        self.depth_pub = self.create_publisher(Float32, "/sensors/depth", 10)
        self.temp_pub = self.create_publisher(Float32, "/sensors/temp", 10)
        self.pressure_pub = self.create_publisher(Float32, "/sensors/pressure", 10)
        self.leak_pub = self.create_publisher(Bool, "/sensors/leak", 10)
        self.voltage_pub = self.create_publisher(Float32, "/power/voltage", 10)
        self.current_pub = self.create_publisher(Float32, "/power/current", 10)
        self.power_pub = self.create_publisher(Float32, "/power/watts", 10)
        self.fake_timer = self.create_timer(1.0, self.publish_fake_data)
        self.get_logger().info("UI Sensor Bridge started.")
        self.get_logger().info("Subscribing to /uart/sensors")
        self.get_logger().info("Publishing UI sensor and power topics.")

    def uart_callback(self, msg):
        """
        Expected /uart/sensors format:
        data[0] = depth in meters
        data[1] = temperature in Celsius
        data[2] = pressure in Pascals
        data[3] = leak, 0 = no leak, 1 = leak
        data[4] = voltage in Volts
        data[5] = current in Amps
        """

        if len(msg.data) < 6:
            self.get_logger().warn("UART sensor message does not have enough data.")
            return

        depth = float(msg.data[0])
        temp = float(msg.data[1])
        pressure_pa = float(msg.data[2])
        leak = bool(msg.data[3])
        voltage = float(msg.data[4])
        current = float(msg.data[5])

        self.publish_to_ui(depth, temp, pressure_pa, leak, voltage, current)

    def publish_fake_data(self):
        depth = random.uniform(0.2, 5.0)
        temp = random.uniform(18.0, 25.0)

        # Fake pressure in Pascals
        pressure_pa = random.uniform(100000.0, 103000.0)

        leak = False

        # Fake power data
        voltage = random.uniform(11.0, 12.6)  # Volts
        current = random.uniform(0.5, 5.0)    # Amps

        self.publish_to_ui(depth, temp, pressure_pa, leak, voltage, current)

    def publish_to_ui(self, depth, temp, pressure_pa, leak, voltage, current):
        depth_msg = Float32()
        depth_msg.data = float(depth)

        temp_msg = Float32()
        temp_msg.data = float(temp)

        pressure_msg = Float32()
        pressure_msg.data = float(pressure_pa) / 101325.0  # Pa to atm

        leak_msg = Bool()
        leak_msg.data = bool(leak)

        voltage_msg = Float32()
        voltage_msg.data = float(voltage)

        current_msg = Float32()
        current_msg.data = float(current)

        power_msg = Float32()
        power_msg.data = float(voltage) * float(current)

        self.depth_pub.publish(depth_msg)
        self.temp_pub.publish(temp_msg)
        self.pressure_pub.publish(pressure_msg)
        self.leak_pub.publish(leak_msg)

        self.voltage_pub.publish(voltage_msg)
        self.current_pub.publish(current_msg)
        self.power_pub.publish(power_msg)

        self.get_logger().info(
            f"Depth: {depth_msg.data:.2f} m | "
            f"Temp: {temp_msg.data:.2f} C | "
            f"Pressure: {pressure_msg.data:.3f} atm | "
            f"Leak: {leak_msg.data} | "
            f"Voltage: {voltage_msg.data:.2f} V | "
            f"Current: {current_msg.data:.2f} A | "
            f"Power: {power_msg.data:.2f} W"
        )


def main(args=None):
    rclpy.init(args=args)
    node = UISensorBridge()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
