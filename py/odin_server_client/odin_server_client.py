import os
import socket
import sys
import threading
from typing import Callable, Iterable, Optional

import flatbuffers

from .flatbuffers.FB import CmdOutput as FB_CmdOutput
from .flatbuffers.FB import Input as FB_Input
from .flatbuffers.FB import MessageType as FB_MessageType
from .flatbuffers.FB import Output as FB_Output
from .flatbuffers.FB import Spikes as FB_Spikes


class OdinServerClientError(RuntimeError):
    pass


class OdinServerError(OdinServerClientError):
    pass


class OdinConnectionClosedError(OdinServerClientError):
    pass


class OdinServerClient:
    def __init__(self, host: str, port: int = 5001,
                 log_handler: Optional[Callable[[int, str], None]] = None,
                 spikes_handler: Optional[Callable[[int, Iterable[int]], None]] = None):
        self.host = host
        self.port = port
        self.log_handler = log_handler
        self.spikes_handler = spikes_handler

        self.sock: Optional[socket.socket] = None
        self.running = False
        self.recv_thread: Optional[threading.Thread] = None

        self._condition = threading.Condition()
        self._send_lock = threading.Lock()

        self._next_handle = 1
        self._pending_cmd_outputs = {}
        self._last_error: Optional[BaseException] = None

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def connect(self, timeout: float = 5.0):
        if self.running:
            return
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(timeout)
            self.sock.connect((self.host, self.port))
            self.sock.settimeout(None)
            self.running = True
            self.recv_thread = threading.Thread(target=self._receive_loop, daemon=True)
            self.recv_thread.start()
        except Exception as exc:
            self.running = False
            if self.sock is not None:
                try:
                    self.sock.close()
                except Exception:
                    pass
                self.sock = None
            raise OdinServerClientError(f"Failed to connect: {exc}") from exc

    def close(self):
        self.running = False
        socket_ref = self.sock
        self.sock = None
        if socket_ref is not None:
            try:
                socket_ref.shutdown(socket.SHUT_RDWR)
            except Exception:
                pass
            try:
                socket_ref.close()
            except Exception:
                pass
        thread_ref = self.recv_thread
        if thread_ref is not None and thread_ref.is_alive():
            thread_ref.join(timeout=1.0)
        with self._condition:
            self._condition.notify_all()

    def send_command(self, command, wait_for_output: bool = False, timeout: Optional[float] = None):
        handles = self.send_commands([command], wait_for_outputs=wait_for_output, timeout=timeout)
        return handles[0]

    def send_commands(self, commands: Iterable, wait_for_outputs: bool = False, timeout: Optional[float] = None):
        cmd_list = list(commands)
        if not cmd_list:
            return []

        with self._condition:
            for cmd in cmd_list:
                if getattr(cmd, "handle", 0) == 0:
                    cmd.handle = self._next_handle
                    self._next_handle += 1

        input_t = FB_Input.InputT(commands=cmd_list)
        builder = flatbuffers.Builder(2048)
        input_offset = input_t.Pack(builder)
        builder.Finish(input_offset)
        self._send_input(builder.Output())

        if not wait_for_outputs:
            return [cmd.handle for cmd in cmd_list]

        return [self.wait_for_handle(cmd.handle, timeout=timeout) for cmd in cmd_list]

    def wait_for_handle(self, handle: int, timeout: Optional[float] = None):
        with self._condition:
            ok = self._condition.wait_for(
                lambda: handle in self._pending_cmd_outputs or self._last_error is not None or not self.running,
                timeout=timeout,
            )

            if handle in self._pending_cmd_outputs:
                return self._pending_cmd_outputs.pop(handle)

            if self._last_error is not None:
                raise OdinServerClientError(f"Receiver loop failed: {self._last_error}") from self._last_error

            if not ok:
                raise TimeoutError(f"Timed out waiting for response handle={handle}")

            raise OdinConnectionClosedError("Connection closed while waiting for command response")

    def _send_input(self, payload: bytes):
        if self.sock is None or not self.running:
            raise OdinConnectionClosedError("Client is not connected")
        try:
            with self._send_lock:
                # print(f"len={len(payload)}")
                self.sock.sendall(len(payload).to_bytes(4, "little") + payload)
        except Exception as exc:
            self._set_receiver_error(exc)
            raise OdinServerClientError(f"Failed to send input: {exc}") from exc

    def _receive_loop(self):
        try:
            while self.running:
                size_raw = self._recv_exact(4)
                if size_raw is None:
                    break
                payload_size = int.from_bytes(size_raw, "little")
                if payload_size <= 0:
                    continue
                payload = self._recv_exact(payload_size)
                if payload is None:
                    break
                output = FB_Output.Output.GetRootAs(payload, 0)
                self._process_output(output)
        except Exception as exc:
            self._set_receiver_error(exc)
        finally:
            self.running = False
            with self._condition:
                self._condition.notify_all()

    def _recv_exact(self, byte_count: int) -> Optional[bytes]:
        if self.sock is None:
            return None
        chunks = bytearray()
        while len(chunks) < byte_count and self.running:
            data = self.sock.recv(byte_count - len(chunks))
            if not data:
                return None
            chunks.extend(data)
        return bytes(chunks) if len(chunks) == byte_count else None

    def _process_output(self, output):
        with self._condition:
            msg = output.Message()
            if msg is not None:
                msg_type = msg.Type()
                text = (msg.Text() or b"").decode(errors="replace")
                if self.log_handler is not None:
                    self.log_handler(msg_type, text)
                else:
                    print(text, end="")
                if msg_type == FB_MessageType.MessageType.ERROR:
                    self._last_error = OdinServerError(text)

            cmd_output_obj = output.CmdOutput()
            if cmd_output_obj is not None:
                cmd_output_t = FB_CmdOutput.CmdOutputT.InitFromObj(cmd_output_obj)
                self._pending_cmd_outputs[cmd_output_t.handle] = cmd_output_t

            spikes_obj = output.Spikes()
            if spikes_obj is not None:
                spikes_t = FB_Spikes.SpikesT.InitFromObj(spikes_obj)
                try:
                    if self.spikes_handler is not None:
                        self.spikes_handler(spikes_t.coreId, spikes_t.words)
                    else:
                        print(f"Spikes(core={spikes_t.coreId}) words={list(spikes_t.words) if spikes_t.words is not None else []}")
                except Exception:
                    pass

            self._condition.notify_all()

    def _set_receiver_error(self, exc: BaseException):
        with self._condition:
            if self._last_error is None:
                self._last_error = exc
            self._condition.notify_all()
