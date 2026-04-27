# ---------------------------------------------------------------------------
#  File: PacketRecieve.py
# ---------------------------------------------------------------------------

import struct
import time

from UartProtocol import *


class PacketParser:
    def __init__(self, driver, logger=None):
        self.tx = None
        self.driver = driver
        self.logger = logger

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

            if self.tx is not None and self.tx.last_ping_send_time is not None:
                rtt_ms = (time.monotonic() - self.tx.last_ping_send_time) * 1000.0
                if self.logger:
                    self.logger.info(f"RX PONG = 0x{value:08X} | RTT={rtt_ms:.3f} ms")
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

            if self.tx is None:
                if self.logger:
                    self.logger.warn(f"RX ACK seq={seq}, but TX manager is not connected")
                return False

            self.tx.mark_ack_received(seq)
            return True

        elif ID == ID_POSITIONAL_TELEMETRY:
            if len(payload) != 16:
                if self.logger:
                    self.logger.warn(f"POSITIONAL PACKET INVALID LENGTH = {len(payload)}")
                return False

            # Fill in later when positional packet format is finalized.
            return True

        elif ID == ID_ENVIRONMENTAL_TELEMETRY:
            if len(payload) != 16:
                if self.logger:
                    self.logger.warn(f"ENV PACKET INVALID LENGTH = {len(payload)}")
                return False

            temp, depth, pressure_pa, timestamp_ms = struct.unpack("<iiiI", payload)

            if self.logger:
                self.logger.info(
                    f"Temperature: {temp}, Depth: {depth}, "
                    f"Pressure_PA: {pressure_pa}, Time: {timestamp_ms}"
                )

            return True

        elif ID == ID_MISSION_FAILURE:
            if len(payload) != 4:
                if self.logger:
                    self.logger.warn(f"FAILURE PACKET INVALID LENGTH = {len(payload)}")
                return False

            failure_code = struct.unpack("<I", payload)[0]

            if self.logger:
                self.logger.warn(f"RX MISSION FAILURE | code={failure_code}")

            self.driver.publish_event("MISSION_FAILURE")
            return True

        else:
            if self.logger:
                self.logger.warn(
                    f"RX UNKNOWN PACKET | id=0x{ID:02X} | payload={payload.hex()}"
                )
            return False