"""Low-level socket transport to Unreal Engine.

Both mcp_server.py and tools/*.py import send_to_unreal from here,
avoiding circular imports.

P3-T1: Two protocol versions controlled by SOCKET_PROTOCOL_VERSION env var.
  v1 (default): raw JSON, no framing — backwards compatible
  v2:           length-prefixed framing [2-byte magic 0xABCD][4-byte BE length][payload]
                + 10s read timeout + TRANSPORT_TIMEOUT error code
"""
import json
import os
import socket
import struct
import sys

# P3-T1: Protocol version control
_PROTOCOL_VERSION = os.environ.get("SOCKET_PROTOCOL_VERSION", "v1").strip().lower()
_FRAME_MAGIC = b'\xab\xcd'
_DEFAULT_TIMEOUT = float(os.environ.get("UNREAL_SOCKET_TIMEOUT", "10.0"))

_negotiated_protocol: str = ""  # cached after first successful negotiation; "" = not yet negotiated


def _negotiate_protocol(sock) -> str:
    """Attempt v1/v2 protocol negotiation. Returns agreed version or 'v1' on failure."""
    try:
        probe = json.dumps({"type": "protocol_negotiate", "client_supports": ["v1", "v2"]})
        sock.sendall((probe + "\n").encode("utf-8"))
        sock.settimeout(3.0)
        data = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
            try:
                resp = json.loads(data.decode("utf-8"))
                return resp.get("agreed_version", "v1")
            except json.JSONDecodeError:
                continue
    except Exception:
        pass
    return "v1"


def _recv_exact(sock: socket.socket, n: int) -> bytes:
    """Read exactly n bytes from socket, raising on short read."""
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError(f"Socket closed after {len(buf)}/{n} bytes")
        buf += chunk
    return buf


def _send_v2(sock: socket.socket, payload: bytes) -> None:
    """Send with framing: magic(2) + length(4 BE) + payload."""
    header = _FRAME_MAGIC + struct.pack(">I", len(payload))
    sock.sendall(header + payload)


def _recv_v2(sock: socket.socket) -> bytes:
    """Receive framed message. Returns raw payload bytes."""
    header = _recv_exact(sock, 6)
    if header[:2] != _FRAME_MAGIC:
        raise ValueError(f"Invalid frame magic: {header[:2].hex()}")
    length = struct.unpack(">I", header[2:])[0]
    return _recv_exact(sock, length)


def send_to_unreal(command):
    global _negotiated_protocol
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            unreal_port = int(os.environ.get("UNREAL_PORT", "9877"))

            if _PROTOCOL_VERSION == "v2":
                # P3-T1: framing + timeout
                s.settimeout(_DEFAULT_TIMEOUT)
                s.connect(('localhost', unreal_port))
                if not _negotiated_protocol:
                    _negotiated_protocol = _negotiate_protocol(s)
                payload = json.dumps(command).encode('utf-8')
                _send_v2(s, payload)
                response_bytes = _recv_v2(s)
                return json.loads(response_bytes.decode('utf-8'))
            else:
                # v1: legacy raw JSON (default, backwards compatible)
                s.connect(('localhost', unreal_port))
                if not _negotiated_protocol:
                    _negotiated_protocol = _negotiate_protocol(s)
                json_str = json.dumps(command)
                s.sendall(json_str.encode('utf-8'))

                buffer_size = 8192
                response_data = b""
                while True:
                    chunk = s.recv(buffer_size)
                    if not chunk:
                        break
                    response_data += chunk
                    try:
                        json.loads(response_data.decode('utf-8'))
                        break
                    except json.JSONDecodeError:
                        continue

                if response_data:
                    return json.loads(response_data.decode('utf-8'))
                else:
                    return {"success": False, "error": "No response received"}

        except socket.timeout:
            print(f"[transport] TRANSPORT_TIMEOUT after {_DEFAULT_TIMEOUT}s", file=sys.stderr)
            return {
                "success": False,
                "error_code": "TRANSPORT_TIMEOUT",
                "error": f"Socket read timeout after {_DEFAULT_TIMEOUT}s — UE may be busy",
            }
        except Exception as e:
            print(f"Error sending to Unreal: {e}", file=sys.stderr)
            return {"success": False, "error": str(e)}
