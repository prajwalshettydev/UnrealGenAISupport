import socket
import json
import unreal
import threading
import time
from typing import Dict, Any, Tuple, List, Optional

# Import handlers
from handlers import basic_commands, actor_commands, blueprint_commands, python_commands
from handlers import ui_commands
from utils import logging as log

# Global queues and state
command_queue = []
response_dict = {}


class CommandDispatcher:
    """
    Dispatches commands to appropriate handlers based on command type
    """
    def __init__(self):
        # Register command handlers
        self.handlers = {
            "handshake": self._handle_handshake,

            # Basic object commands
            "spawn": basic_commands.handle_spawn,
            "create_material": basic_commands.handle_create_material,
            "modify_object": actor_commands.handle_modify_object,
            "take_screenshot": basic_commands.handle_take_screenshot,

            # Blueprint commands
            "create_blueprint": blueprint_commands.handle_create_blueprint,
            "add_component": blueprint_commands.handle_add_component,
            "add_variable": blueprint_commands.handle_add_variable,
            "add_function": blueprint_commands.handle_add_function,
            "add_node": blueprint_commands.handle_add_node,
            "connect_nodes": blueprint_commands.handle_connect_nodes,
            "compile_blueprint": blueprint_commands.handle_compile_blueprint,
            "spawn_blueprint": blueprint_commands.handle_spawn_blueprint,
            "delete_node": blueprint_commands.handle_delete_node,
            
            # Getters
            "get_node_guid": blueprint_commands.handle_get_node_guid,
            "get_all_nodes": blueprint_commands.handle_get_all_nodes,
            "get_node_suggestions": blueprint_commands.handle_get_node_suggestions,
            
            
            # Bulk commands
            "add_nodes_bulk": blueprint_commands.handle_add_nodes_bulk,
            "connect_nodes_bulk": blueprint_commands.handle_connect_nodes_bulk,
            
            # Python and console
            "execute_python": python_commands.handle_execute_python,
            "execute_unreal_command": python_commands.handle_execute_unreal_command,
            
            # New
            "edit_component_property": actor_commands.handle_edit_component_property,
            "add_component_with_events": actor_commands.handle_add_component_with_events,
            
            # Scene
            "get_all_scene_objects": basic_commands.handle_get_all_scene_objects,
            "create_project_folder": basic_commands.handle_create_project_folder,
            "get_files_in_folder": basic_commands.handle_get_files_in_folder,
            
            # Input
            "add_input_binding": basic_commands.handle_add_input_binding,

            # --- NEW UI COMMANDS ---
            "add_widget_to_user_widget": ui_commands.handle_add_widget_to_user_widget,
            "edit_widget_property": ui_commands.handle_edit_widget_property,
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
        """Built-in handler for handshake command"""
        message = command.get("message", "")
        log.log_info(f"Handshake received: {message}")
        
        # Get Unreal Engine version
        engine_version = unreal.SystemLibrary.get_engine_version()
        
        # Add connection and session information
        connection_info = {
            "status": "Connected",
            "engine_version": engine_version,
            "timestamp": time.time(),
            "session_id": f"UE-{int(time.time())}"
        }
        
        return {
            "success": True, 
            "message": f"Received: {message}",
            "connection_info": connection_info
        }


# Create global dispatcher instance
dispatcher = CommandDispatcher()


def process_commands(delta_time=None):
    """Process commands on the main thread"""
    if not command_queue:
        return

    command_id, command = command_queue.pop(0)
    log.log_info(f"Processing command on main thread: {command}")

    try:
        response = dispatcher.dispatch(command)
        response_dict[command_id] = response
    except Exception as e:
        log.log_error(f"Error processing command: {str(e)}", include_traceback=True)
        response_dict[command_id] = {"success": False, "error": str(e)}


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
                json.loads(data.decode('utf-8'))
                # If we get here, JSON is valid and complete
                return data.decode('utf-8')
            except json.JSONDecodeError as json_err:
                # Check if the error indicates an unterminated string or incomplete JSON
                if "Unterminated string" in str(json_err) or "Expecting" in str(json_err):
                    # Need more data, continue receiving
                    continue
                else:
                    # JSON is malformed in some other way, not just incomplete
                    log.log_error(f"Malformed JSON received: {str(json_err)}", include_traceback=True)
                    return None
            
        except socket.timeout:
            # Socket timeout, return what we have so far
            log.log_warning("Socket timeout while receiving data")
            return data.decode('utf-8')
        except Exception as e:
            log.log_error(f"Error receiving data: {str(e)}", include_traceback=True)
            return None
    
    return data.decode('utf-8')


def socket_server_thread():
    """Socket server running in a separate thread"""
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('localhost', 9877))
    server_socket.listen(1)
    log.log_info("Unreal Engine socket server started on port 9877")

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
                        command_queue.append((command_id, command))

                        # Wait for the response with a timeout
                        timeout = 10  # seconds
                        start_time = time.time()
                        while command_id not in response_dict and time.time() - start_time < timeout:
                            time.sleep(0.1)

                        if command_id in response_dict:
                            response = response_dict.pop(command_id)
                            conn.sendall(json.dumps(response).encode())
                        else:
                            error_response = {"success": False, "error": "Command timed out"}
                            conn.sendall(json.dumps(error_response).encode())
                except json.JSONDecodeError as json_err:
                    log.log_error(f"Error parsing JSON: {str(json_err)}", include_traceback=True)
                    error_response = {"success": False, "error": f"Invalid JSON: {str(json_err)}"}
                    conn.sendall(json.dumps(error_response).encode())
            else:
                # No data or error receiving data
                error_response = {"success": False, "error": "No data received or error parsing data"}
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
    log.log_info("  - Blueprint: create_blueprint, add_component, add_variable, add_function, add_node, connect_nodes, compile_blueprint, spawn_blueprint, add_nodes_bulk, connect_nodes_bulk")

# Auto-start the server when this module is imported
initialize_server()

