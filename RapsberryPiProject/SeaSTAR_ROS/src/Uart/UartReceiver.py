# ---------------------------------------------------------------------------
#  File: PacketRecieve.py
# ---------------------------------------------------------------------------

#!/usr/bin/env python3
import struct
import time
from std_msgs.msg import Bool, Float32MultiArray, String
from UartProtocol import *

class PacketParser:

    def __init__(self, driver, logger, leak_pub, envs_pub, pos_pub, pow_pub, curr_pub, event_pub, sample_pub):

        self.tx       = None
        self.driver   = driver
        self.logger   = logger
        self.leak_pub = leak_pub
        self.envs_pub = envs_pub
        self.pos_pub  = pos_pub
        self.pow_pub  = pow_pub
        self.curr_pub = curr_pub
        self.event_pub = event_pub
        self.sample_pub = sample_pub

    def handle_packet(self, ID: int, payload: bytes):

        if ID == ID_STM32_HELLO:

            if len(payload) != 4:
                if self.logger:
                    self.logger.warn(f"HELLO PACKET INVALID LENGTH = {len(payload)}")
                return False

            if not self.driver.comm_ok:
                self.driver.comm_ok = True
                self.driver.publish_event("UART_COMMUNICATION_ESTABLISHED")
                self.driver.publish_status()

            if self.logger:
                self.logger.info(f"RX STM32_HELLO | payload={payload.hex()}")
            return True

        elif ID == ID_PONG:

            if len(payload) != 4:
                if self.logger:
                    self.logger.warn(f"PONG PACKET INVALID LENGTH = {len(payload)}")
                return False

            value = int.from_bytes(payload, "little")

            if self.tx is not None:
                self.tx.mark_pong_received(value)
            else:
                if self.logger:
                    self.logger.info(f"RX PONG = 0x{value:08X}")

            return True

        elif ID == ID_PACKET_ACKNOWLEDGMENT:

            if len(payload) != 4:
                if self.logger:
                    self.logger.warn(f"ACK PACKET INVALID LENGTH = {len(payload)}")

                return False
            seq = struct.unpack("<I", payload)[0]
            if self.tx is not None:
                self.tx.mark_ack_received(seq)

            return True

        elif ID == ID_POSITIONAL_TELEMETRY:

            if len(payload) != 12:
                if self.logger:
                    self.logger.warn(f"POSITIONAL PACKET INVALID LENGTH = {len(payload)}")
                return False

            roll, pitch, yaw, timestamp_ms = struct.unpack("<hhh2xI", payload)
            realtime_timestamp = self.timestamper(timestamp_ms)
            msg = Float32MultiArray()
            msg.data = [roll / 1000.0 , pitch / 1000.0 , yaw / 1000.0 , realtime_timestamp]

            self.pos_pub.publish(msg)

            return True

        elif ID == ID_ENVIRONMENTAL_TELEMETRY:

            
            if len(payload) != 16:
                if self.logger:
                    self.logger.warn(f"ENV PACKET INVALID LENGTH = {len(payload)}")
                return False

            temp, depth, pressure_pa, timestamp_ms = struct.unpack("<iiiI", payload)
            realworld_time = time.time()

            realtime_timestamp = self.timestamper(timestamp_ms)
            msg = Float32MultiArray()

            msg.data = [temp / 1000.0 , depth / 1000.0, pressure_pa, realtime_timestamp]
            self.envs_pub.publish(msg)

            return True

        elif ID == ID_POWER_STATUS_TELEMETRY:

            if len(payload) != 12:
                if self.logger:
                    self.logger.warn(f"ENV PACKET INVALID LENGTH = {len(payload)}")
                return False

            voltage_mv, current_ma, power_mw, timestamp_ms = struct.unpack("<hhh2xI", payload)
            
            realtime_timestamp = self.timestamper(timestamp_ms)
            msg = Float32MultiArray()

            msg.data = [voltage_mv /  1000.0, current_ma / 1000.0, power_mw / 1000.0, realtime_timestamp]

            self.pow_pub.publish(msg)

            return True

        elif ID == ID_LEAK_DETECTED:

            self.logger.info(f"LEAK HAS BEEN DETECTED")

            msg = Bool()
            msg.data = True

            self.leak_pub.publish(msg)
            return True
        
        elif ID == ID_SAMPLE_COLLECTED:
            if len(payload) != 16:
                if self.logger:
                    self.logger.warn(f"ENV PACKET INVALID LENGTH = {len(payload)}")
                return False

            if self.logger:
                self.logger.info("WATER SAMPLE COMPLETE")

            msg = String()
            msg.data = "SAMPLE_COLLECTED"
            self.event_pub.publish(msg)

            temp, depth, pressure_pa, timestamp_ms = struct.unpack("<iiiI", payload)
            realworld_time = time.time()

            realtime_timestamp = self.timestamper(timestamp_ms)
            msg = Float32MultiArray()

            msg.data = [temp / 1000.0 , depth / 1000.0, pressure_pa, realtime_timestamp]
            self.sample_pub.publish(msg)

            return True

        else:
            if self.logger:
                self.logger.warn(
                    f"RX UNKNOWN PACKET | id=0x{ID:02X} | payload={payload.hex()}"
                )

            return False


    def timestamper(self, timestamp_ms):
        
        if not hasattr(self, "stm32toms"):
            self.stm32toms = timestamp_ms
            self.pitoreal = time.time()

        elapsed_ms = timestamp_ms - self.stm32toms
    
        return elapsed_ms / 1000.0