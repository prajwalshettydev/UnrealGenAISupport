import argparse
import os
import socket
import subprocess
import sys
import time


def _port_is_open(host: str, port: int, timeout_seconds: float = 0.5) -> bool:
    try:
        with socket.create_connection((host, port), timeout=timeout_seconds):
            return True
    except OSError:
        return False


def _wait_for_port_to_close(host: str, port: int, timeout_seconds: float, poll_interval: float = 0.5) -> bool:
    deadline = time.time() + max(timeout_seconds, 0.5)
    while time.time() < deadline:
        if not _port_is_open(host, port):
            return True
        time.sleep(poll_interval)
    return not _port_is_open(host, port)


def _build_editor_command(editor_path: str, project_file_path: str) -> list:
    normalized_editor_path = os.path.abspath(editor_path)
    normalized_project_path = os.path.abspath(project_file_path)

    if sys.platform == "darwin" and normalized_editor_path.endswith(".app"):
        return ["open", "-a", normalized_editor_path, "--args", normalized_project_path]

    return [normalized_editor_path, normalized_project_path]


def _spawn_editor(command: list) -> subprocess.Popen:
    popen_kwargs = {
        "stdin": subprocess.DEVNULL,
        "stdout": subprocess.DEVNULL,
        "stderr": subprocess.DEVNULL,
    }

    if os.name == "nt":
        detached_process = 0x00000008
        create_new_process_group = 0x00000200
        popen_kwargs["creationflags"] = detached_process | create_new_process_group
    else:
        popen_kwargs["start_new_session"] = True

    return subprocess.Popen(command, **popen_kwargs)


def main() -> int:
    parser = argparse.ArgumentParser(description="Wait for Unreal Editor to exit, then relaunch it.")
    parser.add_argument("--editor-path", required=True)
    parser.add_argument("--project-file-path", required=True)
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", required=True, type=int)
    parser.add_argument("--wait-for-port-close-timeout", type=float, default=30.0)
    args = parser.parse_args()

    if not os.path.exists(args.editor_path):
        raise FileNotFoundError(f"Editor executable was not found: {args.editor_path}")

    if not os.path.exists(args.project_file_path):
        raise FileNotFoundError(f"Project file was not found: {args.project_file_path}")

    port_closed = _wait_for_port_to_close(args.host, args.port, args.wait_for_port_close_timeout)
    if not port_closed:
        return 1

    command = _build_editor_command(args.editor_path, args.project_file_path)
    _spawn_editor(command)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
