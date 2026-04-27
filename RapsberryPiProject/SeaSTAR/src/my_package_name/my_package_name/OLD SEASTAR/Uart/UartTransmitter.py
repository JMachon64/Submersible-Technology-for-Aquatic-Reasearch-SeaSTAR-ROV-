# tx_manager.py

import queue
import struct
import threading
import time

from UartProtocol import *
from UartDriver import UARTDriver


class TXManager:
    def __init__(self, driver: UARTDriver, logger=None):
        self.driver = driver
        self.logger = logger

        self.next_hello_time = time.monotonic()

        # Ping/Pong RTT tracking
        self.next_ping_seq = 0
        self.pending_pings = {}
        self.ping_lock = threading.Lock()

        # Latest thruster command only
        self.latest_thruster = None
        self.thruster_lock = threading.Lock()

        # Reliable command queue
        self.command_tx_queue = queue.Queue()

        self.next_cmd_seq = 0
        self.pending_command_acks = {}
        self.command_ack_lock = threading.Lock()

        self.COMMAND_ACK_TIMEOUT = 0.20
        self.COMMAND_MAX_RETRIES = 3

        # Stats
        self.thruster_tx_count = 0
        self.command_tx_count = 0
        self.command_retry_count = 0
        self.ping_tx_count = 0
        self.last_tx_report = time.monotonic()

        # TX thread
        self.tx_thread_running = False
        self.tx_thread = None

    def send_hello(self):
        now = time.monotonic()

        if not self.driver.uart_ok or self.driver.comm_ok or now < self.next_hello_time:
            return

        if self.driver.send_packet(ID_PI_HELLO, HELLO_PAYLOAD):
            self.next_hello_time = now + HELLO_PERIOD

            if self.logger:
                self.logger.info("TX PI_HELLO")

    def send_ping(self):
        if not self.driver.link_ok():
            return

        seq = self.next_ping_seq
        self.next_ping_seq = (self.next_ping_seq + 1) & 0xFFFFFFFF

        payload = struct.pack("<I", seq)

        if self.driver.send_packet(ID_PING, payload):
            with self.ping_lock:
                self.pending_pings[seq] = time.monotonic()

            self.ping_tx_count += 1

            if self.logger:
                self.logger.info(f"TX PING seq={seq}")

    def mark_pong_received(self, seq: int):
        now = time.monotonic()

        with self.ping_lock:
            sent_time = self.pending_pings.pop(seq, None)

        if sent_time is None:
            if self.logger:
                self.logger.warn(f"RX PONG unknown seq={seq}")
            return

        rtt_ms = (now - sent_time) * 1000.0

        if self.logger:
            self.logger.info(f"RX PONG seq={seq} | RTT={rtt_ms:.3f} ms")

    def set_thruster_command(self, payload: bytes):
        if len(payload) != 10:
            if self.logger:
                self.logger.warn(f"Invalid thruster payload length: {len(payload)}")
            return

        vals = struct.unpack("<hhhhh", payload)

        with self.thruster_lock:
            self.latest_thruster = vals

    def queue_packet(self, pkt_id: int, payload: bytes = b""):
        seq = self.next_cmd_seq
        self.next_cmd_seq = (self.next_cmd_seq + 1) & 0xFFFFFFFF

        full_payload = struct.pack("<I", seq) + payload

        item = {
            "pkt_id": pkt_id,
            "payload": full_payload,
            "seq": seq,
            "retries": 0,
            "last_send": 0.0,
        }

        self.command_tx_queue.put(item)

        if self.logger:
            self.logger.info(f"Queued command id=0x{pkt_id:02X}, seq={seq}")

    def start_tx_thread(self):
        if self.tx_thread_running:
            return

        self.tx_thread_running = True
        self.tx_thread = threading.Thread(target=self.tx_loop, daemon=True)
        self.tx_thread.start()

    def tx_loop(self):
        next_time = time.monotonic()

        while self.tx_thread_running:
            if self.driver.link_ok():
                sent_this_cycle = False

                # 1) Reliable command retries have highest priority
                if self.resend_timed_out_commands():
                    sent_this_cycle = True

                # 2) New command/event packets
                if not sent_this_cycle:
                    try:
                        item = self.command_tx_queue.get_nowait()

                        if self.driver.send_packet(item["pkt_id"], item["payload"]):
                            item["last_send"] = time.monotonic()

                            with self.command_ack_lock:
                                self.pending_command_acks[item["seq"]] = item

                            self.command_tx_count += 1
                            sent_this_cycle = True

                            if self.logger:
                                self.logger.info(
                                    f"TX COMMAND id=0x{item['pkt_id']:02X}, seq={item['seq']}"
                                )

                    except queue.Empty:
                        pass

                # 3) Thruster stream, latest command only
                if not sent_this_cycle:
                    self.send_latest_thruster()

            self.print_status()
            self.cleanup_stale_pings()

            next_time += THRUSTER_TX_PERIOD
            sleep_time = next_time - time.monotonic()
            time.sleep(sleep_time if sleep_time > 0 else 0)

    def send_latest_thruster(self):
        with self.thruster_lock:
            vals = self.latest_thruster

        if vals is None:
            return

        payload = struct.pack("<hhhhh", *vals)

        if self.driver.send_packet(ID_THRUSTER_INPUT, payload):
            self.thruster_tx_count += 1

    def resend_timed_out_commands(self) -> bool:
        now = time.monotonic()

        with self.command_ack_lock:
            for seq, item in list(self.pending_command_acks.items()):
                if now - item["last_send"] < self.COMMAND_ACK_TIMEOUT:
                    continue

                if item["retries"] >= self.COMMAND_MAX_RETRIES:
                    del self.pending_command_acks[seq]

                    if self.logger:
                        self.logger.error(
                            f"COMMAND ACK FAILED id=0x{item['pkt_id']:02X}, seq={seq}"
                        )

                    return False

                if self.driver.send_packet(item["pkt_id"], item["payload"]):
                    item["retries"] += 1
                    item["last_send"] = now
                    self.command_retry_count += 1

                    if self.logger:
                        self.logger.warn(
                            f"RESENT COMMAND id=0x{item['pkt_id']:02X}, "
                            f"seq={seq}, retry={item['retries']}"
                        )

                    return True

        return False

    def mark_ack_received(self, seq: int):
        now = time.monotonic()

        with self.command_ack_lock:
            item = self.pending_command_acks.pop(seq, None)

        if item is None:
            if self.logger:
                self.logger.warn(f"Received ACK for unknown command seq={seq}")
            return

        rtt_ms = (now - item["last_send"]) * 1000.0

        if self.logger:
            self.logger.info(
                f"COMMAND ACK id=0x{item['pkt_id']:02X}, "
                f"seq={seq}, RTT={rtt_ms:.3f} ms"
            )

    def cleanup_stale_pings(self):
        now = time.monotonic()
        cutoff = now - 2.0

        with self.ping_lock:
            stale = [seq for seq, t in self.pending_pings.items() if t < cutoff]

            for seq in stale:
                del self.pending_pings[seq]

                if self.logger:
                    self.logger.warn(f"PING TIMEOUT seq={seq}")

    def print_status(self):
        now = time.monotonic()

        if now - self.last_tx_report < 1.0:
            return

        with self.thruster_lock:
            latest = self.latest_thruster

        with self.command_ack_lock:
            pending_cmds = len(self.pending_command_acks)

        with self.ping_lock:
            pending_pings = len(self.pending_pings)

        queued_cmds = self.command_tx_queue.qsize()

        if self.logger:
            self.logger.info(
                f"THRUSTER TX RATE = {self.thruster_tx_count} Hz | "
                f"CMD TX = {self.command_tx_count} | "
                f"CMD RETRY = {self.command_retry_count} | "
                f"PING TX = {self.ping_tx_count} | "
                f"latest={latest} | "
                f"pending_cmd_acks={pending_cmds} | "
                f"pending_pings={pending_pings} | "
                f"queued_cmds={queued_cmds}"
            )

        self.thruster_tx_count = 0
        self.command_tx_count = 0
        self.command_retry_count = 0
        self.ping_tx_count = 0
        self.last_tx_report = now

    def stop(self):
        self.tx_thread_running = False

        if self.tx_thread is not None and self.tx_thread.is_alive():
            self.tx_thread.join(timeout=1.0)