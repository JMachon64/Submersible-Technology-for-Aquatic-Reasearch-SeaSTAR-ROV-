# ---------------------------------------------------------------------------
#  File: UartDriver.py
# ---------------------------------------------------------------------------

import struct
import threading
import time
import serial

from UartProtocol import *


class UARTDriver:
    def __init__(self, on_event=None, on_status=None, logger=None):
        self.on_event = on_event or (lambda text: None)
        self.on_status = on_status or (lambda connected: None)
        self.logger = logger

        self.ser = None
        self.serial_lock = threading.Lock()

        self.uart_ok = False
        self.comm_ok = False
        self.last_rx_time = None

        self.packet_handler = None

    def link_ok(self):
        return self.uart_ok and self.comm_ok

    def publish_status(self):
        self.on_status(self.link_ok())

    def publish_event(self, text: str):
        self.on_event(text)

    def open_serial(self):
        try:
            ser = serial.Serial(PORT, BAUD, timeout=0.01)
            time.sleep(0.1)
            ser.reset_input_buffer()
            ser.reset_output_buffer()

            with self.serial_lock:
                self.ser = ser

            self.uart_ok = True
            self.comm_ok = False
            self.last_rx_time = None

            if self.logger:
                self.logger.info(f"FOUND UART ON {PORT} @ {BAUD}")

            self.publish_event("UART_DETECTED")

        except Exception as e:
            with self.serial_lock:
                self.ser = None

            self.uart_ok = False
            self.comm_ok = False
            self.last_rx_time = None

            if self.logger:
                self.logger.error(f"FAILED TO OPEN SERIAL PORT: {e}")

        self.publish_status()

    def close_serial(self, reason=""):
        with self.serial_lock:
            if self.ser is not None:
                try:
                    self.ser.close()
                except Exception:
                    pass

                self.ser = None

        self.uart_ok = False
        self.comm_ok = False
        self.last_rx_time = None

        if reason and self.logger:
            self.logger.warn(f"UART closed: {reason}")

        self.publish_status()

    def send_packet(self, pkt_id: int, payload: bytes = b"") -> bool:
        if not self.uart_ok:
            return False

        try:
            packet = build_packet(pkt_id, payload)

            with self.serial_lock:
                if self.ser is None:
                    return False

                self.ser.write(packet)

            return True

        except Exception as e:
            if self.logger:
                self.logger.error(f"FAILED TO SEND PACKET: {e}")

            self.close_serial("send failure")
            return False

    def read_packet(self):
        try:
            with self.serial_lock:
                if self.ser is None:
                    return None

                head = self.ser.read(1)

                if not head:
                    return None

                if head[0] != HEAD:
                    return None

                hdr = self.ser.read(2)

                if len(hdr) != 2:
                    return None

                length, pkt_id = hdr

                payload = self.ser.read(length)
                crc_bytes = self.ser.read(2)

        except Exception as e:
            if self.logger:
                self.logger.error(f"FAILED TO READ UART: {e}")

            self.close_serial("read failure")
            return None

        if len(payload) != length or len(crc_bytes) != 2:
            return None

        rx_crc = struct.unpack("<H", crc_bytes)[0]
        calc_crc = crc16(bytes([length, pkt_id]) + payload)

        if rx_crc != calc_crc:
            if self.logger:
                self.logger.warn(
                    f"INVALID CRC, id=0x{pkt_id:02X}, "
                    f"rx=0x{rx_crc:04X}, calc=0x{calc_crc:04X}"
                )
            return None

        return pkt_id, payload

    def parse_packet(self, pkt_id: int, payload: bytes):
        self.last_rx_time = time.monotonic()

        if self.packet_handler is None:
            if self.logger:
                self.logger.warn(
                    f"NO PACKET HANDLER SET | "
                    f"id=0x{pkt_id:02X} | payload={payload.hex()}"
                )
            return False

        return self.packet_handler(pkt_id, payload)

    def poll_serial(self):
        if not self.uart_ok:
            return

        for _ in range(8):
            packet = self.read_packet()

            if packet is None:
                break

            self.parse_packet(*packet)

    def check_timeout(self):
        if not self.comm_ok or self.last_rx_time is None:
            return

        elapsed = time.monotonic() - self.last_rx_time

        if elapsed > COMM_TIMEOUT:
            if self.logger:
                self.logger.warn(
                    f"UART communication timeout | elapsed_time={elapsed:.3f}s"
                )

            self.comm_ok = False
            self.last_rx_time = None

            self.publish_event("UART_COMMUNICATION_LOST")
            self.publish_status()

            with self.serial_lock:
                if self.ser is not None:
                    try:
                        self.ser.reset_input_buffer()
                        self.ser.reset_output_buffer()
                    except Exception:
                        pass

    def stop(self):
        self.close_serial("PROGRAM TERMINATED")