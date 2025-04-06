# Content/Python/init_unreal.py - Using UE settings integration
import unreal
import importlib.util
import os
import sys
from utils import logging as log

def initialize_socket_server():
    """
    Initialize the socket server if auto-start is enabled in UE settings
    """
    auto_start = False
    
    # Get settings from UE settings system
    try:
        # First get the class reference
        settings_class = unreal.load_class(None, '/Script/GenerativeAISupport.GenerativeAISupportSettings')
        if settings_class:
            # Get the settings object using the class reference
            settings = unreal.get_default_object(settings_class)
            
            # Log available properties for debugging
            log.log_info(f"Settings object properties: {dir(settings)}")
            
            # Check if auto-start is enabled
            if hasattr(settings, 'auto_start_socket_server'):
                auto_start = settings.auto_start_socket_server
                log.log_info(f"Socket server auto-start setting: {auto_start}")
            else:
                log.log_warning("auto_start_socket_server property not found in settings")
                # Try alternative property names that might exist
                for prop in dir(settings):
                    if 'auto' in prop.lower() or 'socket' in prop.lower() or 'server' in prop.lower():
                        log.log_info(f"Found similar property: {prop}")
        else:
            log.log_error("Could not find GenerativeAISupportSettings class")
    except Exception as e:
        log.log_error(f"Error reading UE settings: {e}")
        log.log_info("Falling back to disabled auto-start")

    # Auto-start if configured
    if auto_start:
        log.log_info("Auto-starting Unreal Socket Server...")

        # Find our plugin's Python directory
        plugin_python_path = None
        for path in sys.path:
            if "GenerativeAISupport/Content/Python" in path:
                plugin_python_path = path
                break

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
