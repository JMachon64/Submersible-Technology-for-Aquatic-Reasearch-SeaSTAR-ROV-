# ---------------------------------------------------------------------------
# File: UartProtocol.py
# Author: Jose Machon
# Date: 4/10/2026 10:23pm
#
# Function:
# Holds UART protocol constants, packet IDs, timing constants,
# CRC calculation, and packet-building helpers for SeaSTAR UART communication.
#
# ---------------------------------------------------------------------------

import struct

HEAD = 0xCC

# TX: PI -> STM32
ID_PI_HELLO = 0x10
ID_PING = 0x01
ID_THRUSTER_INPUT = 0x03
ID_START_MISSION = 0x04
ID_END_MISSION = 0x05
ID_SET_MISSION_SAMPLING_RATE = 0x15
ID_SET_MISSION_MAXIMUM_SPEED = 0x07
ID_COLLECT_WATER_SAMPLE = 0x21

# TX: STM32 -> PI
ID_STM32_HELLO = 0x11
ID_PONG = 0x02
ID_ENVIRONMENTAL_TELEMETRY = 0x08
ID_POSITIONAL_TELEMETRY = 0x09
ID_PACKET_ACKNOWLEDGMENT = 0x99
ID_MISSION_FAILURE = 0x22
ID_LEAK_DETECTED = 0x23


PORT = "/dev/ttyS0"
BAUD = 115200

RX_PERIOD = 0.005                  # serial poll interval, 200 Hz
COMMUNICATION_TIMEOUT_PERIOD = 0.1 # how often to check for comm timeout
COMM_TIMEOUT = 4.0                 # seconds of silence before declaring link lost
THRUSTER_TX_PERIOD = 0.005         # thruster update rate, 200 Hz


HELLO_PERIOD = 0.5
PING_PERIOD = 1.0


PING_VALUE = 0x0043CD28
HELLO_PAYLOAD = bytes([0xAA, 0xBB, 0xCC, 0xDD])

def crc16(data: bytes) -> int:
    crc = 0xFFFF

    for b in data:
        crc ^= b << 8

        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF

    return crc

def build_packet(pkt_id: int, payload: bytes = b"") -> bytes:
    if payload is None:
        payload = b""

    if len(payload) > 255:
        raise ValueError("Payload too large. LEN field is only 1 byte.")

    body = bytes([len(payload), pkt_id]) + payload
    crc = crc16(body)

    return bytes([HEAD]) + body + struct.pack("<H", crc)