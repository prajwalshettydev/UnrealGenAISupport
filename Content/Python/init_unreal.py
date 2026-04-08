# Content/Python/init_unreal.py - Using UE settings integration
import unreal
import importlib.util
import os
import sys
import subprocess
import atexit
from utils import logging as log

# Global process handle for MCP server
mcp_server_process = None


def shutdown_mcp_server():
    """Shutdown the MCP server process when Unreal Editor closes"""
    global mcp_server_process
    if mcp_server_process:
        log.log_info("Shutting down MCP server process...")
        try:
            mcp_server_process.terminate()
            mcp_server_process = None
            log.log_info("MCP server process terminated successfully")
        except Exception as e:
            log.log_error(f"Error terminating MCP server: {e}")


def start_mcp_server():
    """Start the external MCP server process"""
    global mcp_server_process
    try:
        # Find our plugin's Python directory
        plugin_python_path = None
        for path in sys.path:
            if "GenerativeAISupport/Content/Python" in path:
                plugin_python_path = path
                break

        if not plugin_python_path:
            log.log_error("Could not find plugin Python path")
            return False

        # Get the mcp_server.py path
        mcp_server_path = os.path.join(plugin_python_path, "mcp_server.py")

        if not os.path.exists(mcp_server_path):
            log.log_error(f"MCP server script not found at: {mcp_server_path}")
            return False

        # Start the MCP server as a separate process
        python_exe = sys.executable
        log.log_info(f"Starting MCP server using Python: {python_exe}")
        log.log_info(f"MCP server script path: {mcp_server_path}")

        # Create a detached process that will continue running
        # even if Unreal crashes (we'll handle proper shutdown with atexit)
        popen_kwargs = {
            "stdout": subprocess.PIPE,
            "stderr": subprocess.PIPE,
            "text": True,
        }
        if sys.platform == "win32":
            popen_kwargs["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP
        else:
            # On macOS/Linux, start_new_session detaches the child process
            # so it doesn't receive signals from the parent's process group
            popen_kwargs["start_new_session"] = True

        mcp_server_process = subprocess.Popen(
            [python_exe, mcp_server_path], **popen_kwargs
        )

        log.log_info(f"MCP server started with PID: {mcp_server_process.pid}")

        # Register cleanup handler to ensure process is terminated when Unreal exits
        atexit.register(shutdown_mcp_server)

        return True
    except Exception as e:
        log.log_error(f"Error starting MCP server: {e}")
        return False


def initialize_socket_server():
    """
    Initialize the socket server if auto-start is enabled in UE settings
    """
    auto_start = False
    settings_path = "/Script/GenerativeAISupportEditor.GenerativeAISupportSettings"

    # Get settings from UE settings system
    try:
        settings_class = unreal.load_class(None, settings_path)
        if settings_class:
            settings = unreal.get_default_object(settings_class)
            if hasattr(settings, "reload_config"):
                settings.reload_config()
            elif hasattr(settings, "load_config"):
                settings.load_config()

            if hasattr(settings, "get_editor_property"):
                auto_start = bool(
                    settings.get_editor_property("auto_start_socket_server")
                )
            else:
                auto_start = bool(getattr(settings, "auto_start_socket_server", False))

            log.log_info(
                f"Loaded auto-start setting from {settings_path}: {auto_start}"
            )
        else:
            log.log_error(
                f"Could not find GenerativeAISupportSettings class at {settings_path}"
            )
    except Exception as e:
        log.log_error(f"Error reading UE settings: {e}")
        log.log_info("Falling back to disabled auto-start")

    # Auto-start if configured
    if auto_start:
        log.log_info("Auto-starting Unreal Socket Server...")

        # Start Unreal Socket Server
        try:
            # Find our plugin's Python directory
            plugin_python_path = None
            for path in sys.path:
                if "GenerativeAISupport/Content/Python" in path:
                    plugin_python_path = path
                    break

            if plugin_python_path:
                server_path = os.path.join(
                    plugin_python_path, "unreal_socket_server.py"
                )

                if os.path.exists(server_path):
                    # Import and execute the server module
                    spec = importlib.util.spec_from_file_location(
                        "unreal_socket_server", server_path
                    )
                    server_module = importlib.util.module_from_spec(spec)
                    spec.loader.exec_module(server_module)
                    log.log_info("Unreal Socket Server started successfully")

                    # Now start the MCP server
                    if start_mcp_server():
                        log.log_info("Both servers started successfully")
                    else:
                        log.log_error("Failed to start MCP server")
                else:
                    log.log_error(f"Server file not found at: {server_path}")
            else:
                log.log_error("Could not find plugin Python path")
        except Exception as e:
            log.log_error(f"Error starting socket server: {e}")
    else:
        log.log_info("Unreal Socket Server auto-start is disabled")


# Run initialization when this script is loaded
initialize_socket_server()
