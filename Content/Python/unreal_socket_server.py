import os
import socket
import json
import importlib
import unreal
import threading
import time
from typing import Dict, Any, Tuple, List, Optional

# Import handlers
from handlers import basic_commands, actor_commands, blueprint_commands, python_commands
from handlers import ui_commands
from utils import logging as log
from utils import (
    logging as log_module,
)  # kept for importlib.reload() in reload_handlers
from command_registry import registry
from utils import unreal_conversions
from job_manager import tick as _job_tick

# Global queues and state
command_queue = []
response_dict = {}
command_status: Dict[int, str] = {}

COMMAND_STATUS_PENDING = "pending"
COMMAND_STATUS_RUNNING = "running"
COMMAND_STATUS_COMPLETED = "completed"
COMMAND_STATUS_CANCELLED = "cancelled"

_response_lock = threading.Lock()

# P0-3: Cancellation set for timed-out commands.
# Socket thread writes (on timeout); game thread reads (in process_commands).
# Both accesses are protected by _cancel_lock.
cancelled_ids: set = set()
_cancel_lock = threading.Lock()


def _set_command_status(command_id: int, status: str):
    with _response_lock:
        command_status[command_id] = status


def _get_command_status(command_id: int) -> Optional[str]:
    with _response_lock:
        return command_status.get(command_id)


def _take_response(command_id: int) -> Optional[Dict[str, Any]]:
    with _response_lock:
        response = response_dict.pop(command_id, None)
        if response is not None:
            command_status.pop(command_id, None)
        return response


def _store_response(command_id: int, response: Dict[str, Any]):
    with _response_lock:
        response_dict[command_id] = response
        command_status[command_id] = COMMAND_STATUS_COMPLETED


def _mark_command_cancelled(command_id: int) -> bool:
    with _response_lock:
        if command_status.get(command_id) != COMMAND_STATUS_PENDING:
            return False
        command_status[command_id] = COMMAND_STATUS_CANCELLED
        return True


def _mark_command_running(command_id: int) -> bool:
    with _response_lock:
        if command_status.get(command_id) == COMMAND_STATUS_CANCELLED:
            command_status.pop(command_id, None)
            return False
        command_status[command_id] = COMMAND_STATUS_RUNNING
        return True


def _wait_for_response(
    command_id: int, timeout: Optional[float], poll_interval: float = 0.1
) -> Optional[Dict[str, Any]]:
    start_time = time.monotonic()
    while True:
        response = _take_response(command_id)
        if response is not None:
            return response

        if timeout is not None and (time.monotonic() - start_time) >= timeout:
            return None

        time.sleep(poll_interval)


def _get_running_timeout_seconds() -> Optional[float]:
    raw_timeout = os.environ.get("UNREAL_RUNNING_COMMAND_TIMEOUT", "").strip()
    if not raw_timeout:
        return None

    try:
        timeout = float(raw_timeout)
    except ValueError:
        log.log_warning(
            f"Invalid UNREAL_RUNNING_COMMAND_TIMEOUT '{raw_timeout}' — waiting indefinitely for running commands"
        )
        return None

    return timeout if timeout > 0 else None


def _await_command_response(
    command_id: int,
    queue_timeout: float = 10.0,
    poll_interval: float = 0.1,
    running_timeout: Optional[float] = None,
) -> Dict[str, Any]:
    response = _wait_for_response(
        command_id, timeout=queue_timeout, poll_interval=poll_interval
    )
    if response is not None:
        return response

    if _mark_command_cancelled(command_id):
        with _cancel_lock:
            cancelled_ids.add(command_id)
        log.log_warning(
            f"Command {command_id} timed out before execution started — marked for cancellation"
        )
        return {"success": False, "error": "Command timed out before execution started"}

    status = _get_command_status(command_id)
    if status == COMMAND_STATUS_RUNNING:
        log.log_warning(
            f"Command {command_id} exceeded queue timeout after execution started; waiting for completion"
        )
        response = _wait_for_response(
            command_id, timeout=running_timeout, poll_interval=poll_interval
        )
        if response is not None:
            return response
        return {
            "success": False,
            "error": "Command is still running after the extended wait; do not retry automatically",
        }

    response = _take_response(command_id)
    if response is not None:
        return response

    return {"success": False, "error": "Command completed without returning a response"}


class CommandDispatcher:
    """
    Dispatches commands to appropriate handlers based on command type
    """

    HANDLER_MODULES = [
        basic_commands,
        actor_commands,
        blueprint_commands,
        python_commands,
        ui_commands,
    ]
    UTILITY_MODULES = [unreal_conversions, log_module]

    def __init__(self):
        self._reload_revision = 0
        self._build_handler_map()

    def _compose_handler_map(self) -> Dict[str, Any]:
        """Build the next command -> handler dispatch table without mutating live state."""
        builtins = {
            "handshake": self._handle_handshake,
            "reload_handlers": self._handle_reload,
        }
        registry_table = registry.build_dispatch_table()

        conflicts = set(builtins) & set(registry_table)
        if conflicts:
            raise RuntimeError(f"Handler name conflicts with built-ins: {conflicts}")

        return {**builtins, **registry_table}

    def _build_handler_map(self):
        """(Re)build the command -> handler dispatch table.

        Built-in commands (handshake, reload_handlers) are registered here.
        All other commands come from the @registry.command() decorators on
        handler functions — imported modules auto-register at import time.
        """
        self.handlers = self._compose_handler_map()
        builtins_count = 2
        log.log_info(
            f"CommandDispatcher: {len(self.handlers)} commands registered "
            f"({registry.count} from registry + {builtins_count} built-in)"
        )

    def _handle_reload(self, command: Dict[str, Any]) -> Dict[str, Any]:
        """Hot-reload handler (and optionally utility) modules, then rebuild dispatch table.
        Atomic: only rebuilds if ALL modules reload successfully."""
        include_utils = command.get("include_utils", False)
        modules_to_reload = []
        if include_utils:
            modules_to_reload += self.UTILITY_MODULES
        modules_to_reload += self.HANDLER_MODULES

        previous_registry = registry.snapshot()
        previous_handlers = dict(self.handlers)
        expected_command_names = set(previous_registry["commands"].keys())
        reloaded = []
        errors = []
        registry.clear()  # P0-4: Allow re-registration of all commands during reload
        for mod in modules_to_reload:
            try:
                importlib.reload(mod)
                reloaded.append(mod.__name__)
            except Exception as e:
                errors.append(f"{mod.__name__}: {e}")

        next_handlers = None
        if not errors:
            try:
                registry.validate(expected_names=expected_command_names)
                next_handlers = self._compose_handler_map()
            except Exception as e:
                errors.append(str(e))

        if errors:
            registry.restore(previous_registry)
            self.handlers = previous_handlers
            return {
                "success": False,
                "reloaded": reloaded,
                "errors": errors,
                "note": "Reload failed — restored previous command registry and handler map",
            }

        self.handlers = next_handlers
        self._reload_revision += 1
        log.log_info(
            f"Handler reload complete (rev {self._reload_revision}): {reloaded}"
        )
        return {
            "success": True,
            "reloaded": reloaded,
            "revision": self._reload_revision,
            "handler_count": len(self.handlers),
        }

    def dispatch(self, command: Dict[str, Any]) -> Dict[str, Any]:
        """Dispatch command to appropriate handler"""
        command_type = command.get("type")
        if command_type not in self.handlers:
            return {"success": False, "error": f"Unknown command type: {command_type}"}

        try:
            handler = self.handlers[command_type]
            return handler(command)
        except Exception as e:
            log.log_error(f"Error processing command: {str(e)}")
            return {"success": False, "error": str(e)}

    def _handle_handshake(self, command: Dict[str, Any]) -> Dict[str, Any]:
        """Built-in handler for handshake command.
        NOTE: This runs on the socket thread (not game thread) for speed.
        Do NOT call any unreal.* APIs here — they require the main thread."""
        message = command.get("message", "")
        log.log_info(f"Handshake received: {message}")

        connection_info = {
            "status": "Connected",
            "timestamp": time.time(),
            "session_id": f"UE-{int(time.time())}",
        }

        return {
            "success": True,
            "message": f"Received: {message}",
            "connection_info": connection_info,
        }


# Create global dispatcher instance
dispatcher = CommandDispatcher()


def process_commands(delta_time=None):
    """Process commands on the main thread.  Also advances the job queue."""
    # Advance one job step per tick (non-blocking — job runs on this call)
    _job_tick(delta_time)

    if not command_queue:
        return

    command_id, command = command_queue.pop(0)

    # P0-3: Skip commands that timed out on the socket thread.
    with _cancel_lock:
        was_cancelled = command_id in cancelled_ids
        if was_cancelled:
            cancelled_ids.discard(command_id)

    if was_cancelled:
        log.log_warning(
            f"Skipping cancelled command {command_id}: {command.get('type')}"
        )
        _take_response(command_id)
        with _response_lock:
            command_status.pop(command_id, None)
        return

    if not _mark_command_running(command_id):
        log.log_warning(
            f"Skipping command {command_id} after late cancellation: {command.get('type')}"
        )
        return

    log.log_info(f"Processing command on main thread: {command}")

    try:
        response = dispatcher.dispatch(command)
        _store_response(command_id, response)
    except Exception as e:
        log.log_error(f"Error processing command: {str(e)}", include_traceback=True)
        _store_response(command_id, {"success": False, "error": str(e)})


def receive_all_data(conn, buffer_size=4096):
    """
    Receive all data from socket until complete JSON is received

    Args:
        conn: Socket connection
        buffer_size: Initial buffer size for receiving data

    Returns:
        Decoded complete data
    """
    data = b""
    while True:
        try:
            # Receive chunk of data
            chunk = conn.recv(buffer_size)
            if not chunk:
                break

            data += chunk

            # Try to parse as JSON to check if we received complete data
            try:
                json.loads(data.decode("utf-8"))
                # If we get here, JSON is valid and complete
                return data.decode("utf-8")
            except json.JSONDecodeError as json_err:
                # Check if the error indicates an unterminated string or incomplete JSON
                if "Unterminated string" in str(json_err) or "Expecting" in str(
                    json_err
                ):
                    # Need more data, continue receiving
                    continue
                else:
                    # JSON is malformed in some other way, not just incomplete
                    log.log_error(
                        f"Malformed JSON received: {str(json_err)}",
                        include_traceback=True,
                    )
                    return None

        except socket.timeout:
            # Socket timeout, return what we have so far
            log.log_warning("Socket timeout while receiving data")
            return data.decode("utf-8")
        except Exception as e:
            log.log_error(f"Error receiving data: {str(e)}", include_traceback=True)
            return None

    return data.decode("utf-8")


def socket_server_thread():
    """Socket server running in a separate thread"""
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    port = int(os.environ.get("UNREAL_PORT", "9877"))
    server_socket.bind(("localhost", port))
    server_socket.listen(1)
    log.log_info(f"Unreal Engine socket server started on port {port}")

    command_counter = 0

    while True:
        try:
            conn, addr = server_socket.accept()
            # Set a timeout to prevent hanging
            conn.settimeout(5)  # 5-second timeout

            # Receive complete data, handling potential incomplete JSON
            data_str = receive_all_data(conn)

            if data_str:
                try:
                    command = json.loads(data_str)
                    log.log_info(f"Received command: {command}")

                    # For handshake, we can respond directly from the thread
                    if command.get("type") == "handshake":
                        response = dispatcher.dispatch(command)
                        conn.sendall(json.dumps(response).encode())
                    else:
                        # For other commands, queue them for main thread execution
                        command_id = command_counter
                        command_counter += 1
                        _set_command_status(command_id, COMMAND_STATUS_PENDING)
                        command_queue.append((command_id, command))

                        response = _await_command_response(
                            command_id,
                            queue_timeout=10.0,
                            poll_interval=0.1,
                            running_timeout=_get_running_timeout_seconds(),
                        )
                        conn.sendall(json.dumps(response).encode())
                except json.JSONDecodeError as json_err:
                    log.log_error(
                        f"Error parsing JSON: {str(json_err)}", include_traceback=True
                    )
                    error_response = {
                        "success": False,
                        "error": f"Invalid JSON: {str(json_err)}",
                    }
                    conn.sendall(json.dumps(error_response).encode())
            else:
                # No data or error receiving data
                error_response = {
                    "success": False,
                    "error": "No data received or error parsing data",
                }
                conn.sendall(json.dumps(error_response).encode())

            conn.close()
        except Exception as e:
            log.log_error(f"Error in socket server: {str(e)}", include_traceback=True)
            try:
                # Try to close the connection if it's still open
                conn.close()
            except:
                pass


# Register tick function to process commands on main thread
def register_command_processor():
    """Register the command processor with Unreal's tick system"""
    unreal.register_slate_post_tick_callback(process_commands)
    log.log_info("Command processor registered")


# Initialize the server
def initialize_server():
    """Initialize and start the socket server"""
    # Start the server thread
    thread = threading.Thread(target=socket_server_thread)
    thread.daemon = True
    thread.start()
    log.log_info("Socket server thread started")

    # Register the command processor on the main thread
    register_command_processor()

    log.log_info("Unreal Engine AI command server initialized successfully")
    log.log_info("Available commands:")
    log.log_info("  - Basic: handshake, spawn, create_material, modify_object")
    log.log_info(
        "  - Blueprint: create_blueprint, add_component, add_variable, add_function, add_node, connect_nodes, compile_blueprint, spawn_blueprint, add_nodes_bulk, connect_nodes_bulk"
    )


# Auto-start the server when this module is imported
initialize_server()
