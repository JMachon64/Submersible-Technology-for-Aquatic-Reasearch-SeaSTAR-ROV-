
import rclpy
from rclpy.node import Node #creates ros node.
from std_msgs.msg import Float32MultiArray, String, Float32, Bool, UInt8MultiArray

from functools import partial
#-----------------------------------------------------------------------------------

#!/usr/bin/env python3
from Uart.UartProtocol import *

class FSM(Node):
    def __init__(self):
        super().__init__("FSM_node")

        self.state = "BOOT"
        self.previous_state = "IDLE"
        self.failure_sent = False

        self.state_pub = self.create_publisher(String, "SeaSTAR/state", 10)
        self.event_pub = self.create_publisher( UInt8MultiArray, "/uart/command_tx_request", 10)

        self.event_sub         = self.create_subscription(String, "SeaSTAR/event", self.event_callback, 10)
        self.status_pub        = self.create_subscription(String, "/system_status/uart_communication", partial(self.status_callback, "UART"), 10)
        self.status_pub        = self.create_subscription(String, "/system_status/navigation_camera", partial(self.status_callback, "CAMERA"), 10)
        self.commandupdate     = self.create_subscription(String, "/mission/control", self.command_callback,10)
        self.leak_pub          = self.create_subscription(Bool,   "/uart/leak_sensor", partial(self.status_callback, "LEAK"), 10)
        self.status_pub        = self.create_subscription(String,  "/system_status/ui", partial(self.status_callback, "UI"), 10)
        self.status_pub        = self.create_subscription(String,  "/system_status/controller", partial(self.status_callback, "CONTROLLER"), 10)

        self.state_timer = self.create_timer(2, self.publish_state)

        self.uart = False
        self.camera = False
        self.dry = True
        self.ui = False
        self.controller = False

        self.publish_state()

    def event_callback(self,msg):

        event = msg.data
        self.get_logger().info(f"current state: {self.state} | {event}")
        self.state_machine(event)

    def state_machine(self, event):

        if self.state == "BOOT":
            if self.uart and self.camera and self.dry and self.ui:
                self.update_state("IDLE")

        elif self.state == "IDLE":
            
            if event == "START_MISSION" and self.controller:
                self.update_state("ACTIVE")

            elif event == "COLLECT_WATER_SAMPLE":

                self.previous_state = self.state
                self.update_state("SAMPLING") 

            elif not self.uart or not self.camera or not self.dry or not self.ui:
                if not self.camera or not self.dry:
                    self.failure_sent = True
                    self.publish_packet(ID_PI_FAILURE)
                self.update_state("FAILURE")

        elif self.state == "ACTIVE":
            
            if event == "END_MISSION":
                self.update_state("IDLE")

            elif event == "COLLECT_WATER_SAMPLE": 
    
                self.previous_state = self.state
                self.update_state("SAMPLING")

            elif not self.uart or not self.camera or not self.dry or not self.ui or not self.controller:
                if not self.camera or not self.dry or not self.ui or not self.controller:
                    self.failure_sent = True
                    self.publish_packet(ID_PI_FAILURE)
                self.update_state("FAILURE")


        elif self.state == "SAMPLING":

            if event == "SAMPLE_COLLECTED": #SAMPLE COLLECTED WILL COME FROM UART PACKET 
                self.update_state(self.previous_state)

            elif not self.uart or not self.camera or not self.dry or not self.ui:
                if not self.camera or not self.dry or not self.ui:
                    self.failure_sent = True
                    self.publish_packet(ID_PI_FAILURE)
                self.update_state("FAILURE")

        elif self.state == "FAILURE":

            if self.uart and self.camera and self.dry and self.ui and self.controller:
                if self.failure_sent == True:
                    self.failure_sent = False
                    self.publish_packet(ID_PI_RECOVERY)
                self.update_state("IDLE")

    def update_state(self, new_state):

        if self.state == new_state:
            return 
    
        self.get_logger().info(f"STATE CHANGED: {self.state} -> {new_state}")
        
        self.state = new_state
        self.publish_state()

    def publish_state(self):

        msg = String()
        msg.data = self.state
        self.state_pub.publish(msg)

    def status_callback(self, source, msg):

        if source == "UART":
            status = msg.data.strip()
            self.uart = (status == "UART_CONNECTED")
        elif source == "CAMERA":
            status = msg.data.strip()
            self.camera = (status == "STREAMING")
        elif source == "LEAK":
            leak_detected = msg.data
            self.dry = not leak_detected
        elif source == "UI":
            status = msg.data.strip()
            self.ui = (status == "UI_CONNECTED")
        elif source == "CONTROLLER":
            status = msg.data.strip()
            self.controller = (status == "CONTROLLER_CONNECTED")
        self.state_machine("STATUS_UPDATE")
            
    def publish_packet(self, packet_ID):
        msg = UInt8MultiArray()
        msg.data = [packet_ID]
        self.event_pub.publish(msg)
        
    def command_callback(self, msg):
        command = msg.data.strip()
        self.state_machine(command)

#-----------------------------------------------------------------------------------
def main(args=None):
    rclpy.init(args=args)

    node = FSM()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:

        rclpy.shutdown()
#-----------------------------------------------------------------------------------
if __name__ == "__main__":
    main()
#-----------------------------------------------------------------------------------