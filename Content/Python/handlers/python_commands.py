import textwrap

import unreal
import sys
from typing import Dict, Any
import os
import uuid
import time
import traceback

# Assuming a logging module similar to your example
from utils import logging as log


def execute_script(script_file, output_file, error_file, status_file):
    """Execute a Python script with output and error redirection."""
    with open(output_file, 'w') as output_file_handle, open(error_file, 'w') as error_file_handle:
        original_stdout = sys.stdout
        original_stderr = sys.stderr
        sys.stdout = output_file_handle
        sys.stderr = error_file_handle

        success = True
        try:
            with open(script_file, 'r') as f:
                exec(f.read())
        except Exception as e:
            traceback.print_exc()
            success = False
        finally:
            sys.stdout = original_stdout
            sys.stderr = original_stderr

        with open(status_file, 'w') as f:
            f.write('1' if success else '0')


def get_log_line_count():
    """
    Get the current line count of the Unreal log file
    """
    try:
        log_path = os.path.join(unreal.Paths.project_log_dir(), "Unreal.log")
        if os.path.exists(log_path):
            with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
                return sum(1 for _ in f)
        return 0
    except Exception as e:
        log.log_error(f"Error getting log line count: {str(e)}")
        return 0


def get_recent_unreal_logs(start_line=None):
    """
    Retrieve recent Unreal Engine log entries to provide context for errors
    
    Args:
        start_line: Optional line number to start from (to only get new logs)
        
    Returns:
        String containing log entries or None if logs couldn't be accessed
    """
    try:
        log_path = os.path.join(unreal.Paths.project_log_dir(), "Unreal.log")
        if os.path.exists(log_path):
            with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
                if start_line is None:
                    # Legacy behavior - get last 20 lines
                    lines = f.readlines()
                    return "".join(lines[-20:])
                else:
                    # Skip to the starting line
                    for i, _ in enumerate(f):
                        if i >= start_line - 1:
                            break
                    
                    # Get all new lines
                    new_lines = f.readlines()
                    return "".join(new_lines) if new_lines else "No new log entries generated"
        return None
    except Exception as e:
        log.log_error(f"Error getting recent logs: {str(e)}")
        return None


def handle_execute_python(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to execute a Python script in Unreal Engine
    
    Args:
        command: The command dictionary containing:
            - script: The Python code to execute as a string
            - force: Optional boolean to bypass safety checks (default: False)
            
    Returns:
        Response dictionary with success/failure status and output if successful
    """
    try:
        script = command.get("script")
        force = command.get("force", False)

        if not script:
            log.log_error("Missing required parameter for execute_python: script")
            return {"success": False, "error": "Missing required parameter: script"}

        log.log_command("execute_python", f"Script: {script[:50]}...")

        # Get log line count before execution
        log_start_line = get_log_line_count()

        destructive_keywords = [
            "unreal.EditorAssetLibrary.delete_asset",
            "unreal.EditorLevelLibrary.destroy_actor",
            "unreal.save_package",
            "os.remove",
            "shutil.rmtree",
            "file.write",
            "unreal.EditorAssetLibrary.save_asset"
        ]
        is_destructive = any(keyword in script for keyword in destructive_keywords)

        if is_destructive and not force:
            log.log_warning("Potentially destructive script detected")
            return {
                "success": False,
                "error": ("This script may involve destructive actions (e.g., deleting or saving files) "
                          "not explicitly requested. Please confirm with 'Yes, execute it' or set force=True.")
            }

        temp_dir = os.path.join(unreal.Paths.project_saved_dir(), "Temp", "PythonExec")
        if not os.path.exists(temp_dir):
            os.makedirs(temp_dir)

        script_file = os.path.join(temp_dir, f"script_{uuid.uuid4().hex}.py")
        output_file = os.path.join(temp_dir, "output.txt")
        error_file = os.path.join(temp_dir, "error.txt")
        status_file = os.path.join(temp_dir, "status.txt")

        # Normalize user script indentation and write to file
        dedented_script = textwrap.dedent(script).strip()
        with open(script_file, 'w') as f:
            f.write(dedented_script)

        # Execute using the wrapper
        execute_script(script_file, output_file, error_file, status_file)
        time.sleep(0.5)  # Allow execution to complete

        output = ""
        error = ""
        success = False

        if os.path.exists(output_file):
            with open(output_file, 'r') as f:
                output = f.read()
        if os.path.exists(error_file):
            with open(error_file, 'r') as f:
                error = f.read()
        if os.path.exists(status_file):
            with open(status_file, 'r') as f:
                success = f.read().strip() == "1"

        for file in [script_file, output_file, error_file, status_file]:
            if os.path.exists(file):
                os.remove(file)

        # Enhanced error handling for common Unreal API issues
        if not success and error:
            if "set_world_location() required argument 'sweep'" in error:
                error += "\n\nHINT: The set_world_location() method requires a 'sweep' parameter. Try: set_world_location(location, sweep=False)"
            elif "set_world_location() required argument 'teleport'" in error:
                error += "\n\nHINT: The set_world_location() method requires 'teleport' parameter. Try: set_world_location(location, sweep=False, teleport=False)"
            elif "set_actor_location() required argument 'teleport'" in error:
                error += "\n\nHINT: The set_actor_location() method requires a 'teleport' parameter. Try: set_actor_location(location, sweep=False, teleport=False)"

            # Get only new log entries
            recent_logs = get_recent_unreal_logs(log_start_line)
            if recent_logs:
                error += "\n\nNew Unreal logs during execution:\n" + recent_logs

        if success:
            # Get only new log entries for successful execution as well
            recent_logs = get_recent_unreal_logs(log_start_line)
            if recent_logs:
                output += "\n\nNew Unreal logs during execution:\n" + recent_logs
                
            log.log_result("execute_python", True, f"Script executed with output: {output}")
            return {"success": True, "output": output}
        else:
            log.log_error(f"Script execution failed with error: {error}")
            return {"success": False, "error": error if error else "Execution failed without specific error", "output": output}

    except Exception as e:
        log.log_error(f"Error handling execute_python: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}
    finally:
        for file in [script_file, output_file, error_file, status_file]:
            if os.path.exists(file):
                try:
                    os.remove(file)
                except:
                    pass


def handle_execute_unreal_command(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to execute an Unreal Engine console command
    
    Args:
        command: The command dictionary containing:
            - command: The Unreal Engine console command to execute
            - force: Optional boolean to bypass safety checks (default: False)
            
    Returns:
        Response dictionary with success/failure status and output if successful
    """
    script_file = None
    output_file = None
    error_file = None
    
    try:
        cmd = command.get("command")
        force = command.get("force", False)

        if not cmd:
            log.log_error("Missing required parameter for execute_unreal_command: command")
            return {"success": False, "error": "Missing required parameter: command"}

        if cmd.strip().lower().startswith("py "):
            log.log_error("Attempted to run a Python script with execute_unreal_command")
            return {
                "success": False,
                "error": ("Use 'execute_python' command to run Python scripts instead of 'execute_unreal_command' with 'py'. "
                          "For example, send {'type': 'execute_python', 'script': 'your_code_here'}.")
            }

        log.log_command("execute_unreal_command", f"Command: {cmd}")

        # Get log line count before execution
        log_start_line = get_log_line_count()

        destructive_keywords = ["delete", "save", "quit", "exit", "restart"]
        is_destructive = any(keyword in cmd.lower() for keyword in destructive_keywords)

        if is_destructive and not force:
            log.log_warning("Potentially destructive command detected")
            return {
                "success": False,
                "error": ("This command may involve destructive actions (e.g., deleting or saving). "
                          "Please confirm with 'Yes, execute it' or set force=True.")
            }

        # Execute the command
        world = unreal.EditorLevelLibrary.get_editor_world()
        unreal.SystemLibrary.execute_console_command(world, cmd)
        
        # Add a short delay to allow logs to be captured
        time.sleep(1.0)  # Slightly longer delay to ensure logs are written
        
        # Get new log entries generated during command execution
        recent_logs = get_recent_unreal_logs(log_start_line)
        
        output = f"Command '{cmd}' executed successfully"
        if recent_logs:
            output += "\n\nRelated Unreal logs:\n" + recent_logs
            
        log.log_result("execute_unreal_command", True, f"Command '{cmd}' executed")
        return {"success": True, "output": output}
        
    except Exception as e:
        # Get new log entries to provide context for the error
        recent_logs = get_recent_unreal_logs(log_start_line) if 'log_start_line' in locals() else None
        error_msg = f"Error executing command: {str(e)}"
        
        if recent_logs:
            error_msg += "\n\nUnreal logs around the time of error:\n" + recent_logs
            
        log.log_error(f"Error handling execute_unreal_command: {str(e)}", include_traceback=True)
        return {"success": False, "error": error_msg}
