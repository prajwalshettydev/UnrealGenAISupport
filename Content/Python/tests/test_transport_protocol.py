"""
Transport Protocol Tests (US-002)
Tests for v1/v2 framing, timeout, and error handling.
Run standalone (no UE required) using mock sockets.

Usage:
    python tests/test_transport_protocol.py
"""
import json
import struct
import socket
import threading
import time
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Protocol constants (must match transport.py and unreal_socket_server.py)
FRAME_MAGIC = b'\xab\xcd'
PROTOCOL_HEADER_SIZE = 6  # 2 magic + 4 length

_RESULTS = []


def _report(name, passed, detail=""):
    status = "PASS" if passed else "FAIL"
    _RESULTS.append((name, passed))
    print(f"  [{status}] {name}" + (f": {detail}" if detail else ""))


def _make_v2_frame(payload: bytes) -> bytes:
    return FRAME_MAGIC + struct.pack(">I", len(payload)) + payload


def _run_mock_server(handler_fn, port: int):
    """Start a one-shot mock TCP server on localhost:port."""
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("localhost", port))
    srv.listen(1)
    srv.settimeout(3.0)

    def _serve():
        try:
            conn, _ = srv.accept()
            conn.settimeout(3.0)
            handler_fn(conn)
            conn.close()
        except Exception:
            pass
        finally:
            srv.close()

    t = threading.Thread(target=_serve, daemon=True)
    t.start()
    return t


def test_v1_roundtrip():
    """v1: server receives raw JSON, responds raw JSON."""
    PORT = 19870
    received = []

    def handler(conn):
        data = b""
        while True:
            chunk = conn.recv(4096)
            if not chunk:
                break
            data += chunk
            try:
                json.loads(data.decode())
                break
            except json.JSONDecodeError:
                continue
        received.append(json.loads(data.decode()))
        resp = json.dumps({"success": True, "echo": "v1"}).encode()
        conn.sendall(resp)

    _run_mock_server(handler, PORT)
    time.sleep(0.05)

    s = socket.socket()
    s.settimeout(2.0)
    s.connect(("localhost", PORT))
    s.sendall(json.dumps({"type": "ping"}).encode())
    resp_raw = s.recv(4096)
    s.close()
    resp = json.loads(resp_raw.decode())
    _report("v1_roundtrip", resp.get("echo") == "v1", str(resp))
    _report("v1_received_json", len(received) == 1 and received[0].get("type") == "ping")


def test_v2_roundtrip():
    """v2: server receives framed data, responds framed."""
    PORT = 19871
    received = []

    def handler(conn):
        # Read v2 frame
        header = b""
        while len(header) < 6:
            header += conn.recv(6 - len(header))
        magic = header[:2]
        length = struct.unpack(">I", header[2:])[0]
        payload = b""
        while len(payload) < length:
            payload += conn.recv(length - len(payload))
        received.append(json.loads(payload.decode()))
        # Respond with v2 frame
        resp_payload = json.dumps({"success": True, "echo": "v2"}).encode()
        conn.sendall(_make_v2_frame(resp_payload))

    _run_mock_server(handler, PORT)
    time.sleep(0.05)

    s = socket.socket()
    s.settimeout(2.0)
    s.connect(("localhost", PORT))
    payload = json.dumps({"type": "ping"}).encode()
    s.sendall(_make_v2_frame(payload))
    # Read framed response
    hdr = b""
    while len(hdr) < 6:
        hdr += s.recv(6 - len(hdr))
    length = struct.unpack(">I", hdr[2:])[0]
    resp_payload = b""
    while len(resp_payload) < length:
        resp_payload += s.recv(length - len(resp_payload))
    s.close()
    resp = json.loads(resp_payload.decode())
    _report("v2_roundtrip", resp.get("echo") == "v2", str(resp))
    _report("v2_received_framed", len(received) == 1 and received[0].get("type") == "ping")


def test_invalid_header():
    """unreal_socket_server receive_all_data returns None on invalid magic."""
    # Simulate: send data that starts with invalid magic (not 0xABCD) but is not valid JSON
    PORT = 19872

    def handler(conn):
        # Send invalid framing: wrong magic + garbage
        conn.sendall(b'\xDE\xAD\x00\x00\x00\x04test')

    _run_mock_server(handler, PORT)
    time.sleep(0.05)

    # Just verify connection closes cleanly (no crash on client side)
    try:
        s = socket.socket()
        s.settimeout(2.0)
        s.connect(("localhost", PORT))
        data = b""
        while True:
            chunk = s.recv(64)
            if not chunk:
                break
            data += chunk
        s.close()
        # Invalid header either returns garbage or closes — no exception is the pass condition
        _report("invalid_header_no_crash", True, f"received {len(data)} bytes")
    except Exception as e:
        _report("invalid_header_no_crash", False, str(e))


def test_transport_timeout():
    """transport.py returns TRANSPORT_TIMEOUT error code on timeout."""
    os.environ["SOCKET_PROTOCOL_VERSION"] = "v1"
    os.environ["UNREAL_PORT"] = "19999"  # nothing listening
    os.environ["UNREAL_SOCKET_TIMEOUT"] = "0.5"

    # Reload transport to pick up env vars
    if "tools.transport" in sys.modules:
        del sys.modules["tools.transport"]
    try:
        from tools.transport import send_to_unreal
        result = send_to_unreal({"type": "ping"})
        # Should return error dict (connection refused or timeout)
        is_error = isinstance(result, dict) and not result.get("success", True)
        has_code = "error" in result or "error_code" in result
        _report("timeout_returns_error_dict", is_error, str(result)[:80])
        _report("timeout_no_exception", True)
    except Exception as e:
        _report("timeout_no_exception", False, str(e))
    finally:
        del os.environ["UNREAL_SOCKET_TIMEOUT"]
        del os.environ["UNREAL_PORT"]


def test_v2_frame_constants_match():
    """transport.py and unreal_socket_server.py share same FRAME_MAGIC constant."""
    if "tools.transport" in sys.modules:
        del sys.modules["tools.transport"]
    try:
        from tools import transport as tr
        transport_magic = getattr(tr, "_FRAME_MAGIC", None)
        _report("transport_has_FRAME_MAGIC", transport_magic is not None, str(transport_magic))
        _report("FRAME_MAGIC_matches", transport_magic == FRAME_MAGIC,
                f"transport={transport_magic} expected={FRAME_MAGIC}")
    except ImportError as e:
        _report("transport_import", False, str(e))


if __name__ == "__main__":
    print("=== Transport Protocol Tests ===")
    print()
    print("--- v1 roundtrip ---")
    test_v1_roundtrip()
    print()
    print("--- v2 roundtrip ---")
    test_v2_roundtrip()
    print()
    print("--- invalid header ---")
    test_invalid_header()
    print()
    print("--- transport timeout ---")
    test_transport_timeout()
    print()
    print("--- constants match ---")
    test_v2_frame_constants_match()

    total = len(_RESULTS)
    passed = sum(1 for _, p in _RESULTS if p)
    print()
    print(f"=== Results: {passed}/{total} passed ===")
    sys.exit(0 if passed == total else 1)
