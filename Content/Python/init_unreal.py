# Content/Python/init_unreal.py - Simplified config-only version
import unreal
import importlib.util
import os
import sys
import json
from utils import logging as log

def initialize_socket_server():
    """
    Initialize the socket server if auto-start is enabled in config
    """
    # Use config file approach
    config_path = None
    auto_start = False

    # Find our plugin's Python directory
    plugin_python_path = None
    for path in sys.path:
        if "GenerativeAISupport/Content/Python" in path:
            plugin_python_path = path
            config_path = os.path.join(plugin_python_path, "socket_server_config.json")
            break

    # Try to read config file
    if config_path and os.path.exists(config_path):
        try:
            with open(config_path, 'r') as f:
                config = json.load(f)
                auto_start = config.get("auto_start_socket_server", False)
        except Exception as e:
            unreal.log_error(f"Error reading config file: {e}")

    # Auto-start if configured
    if auto_start:
        unreal.log("Auto-starting Unreal Socket Server...")

        if plugin_python_path:
            server_path = os.path.join(plugin_python_path, "unreal_socket_server.py")

            if os.path.exists(server_path):
                try:
                    # Import and execute the server module
                    spec = importlib.util.spec_from_file_location("unreal_socket_server", server_path)
                    server_module = importlib.util.module_from_spec(spec)
                    spec.loader.exec_module(server_module)
                    log.log_info("Unreal Socket Server started successfully")
                except Exception as e:
                    log.log_error(f"Error starting socket server: {e}")
            else:
                log.log_error(f"Server file not found at: {server_path}")
        else:
            log.log_error("Could not find plugin Python path")
    else:
        log.log_info("Unreal Socket Server auto-start is disabled")

# Run initialization when this script is loaded
initialize_socket_server()