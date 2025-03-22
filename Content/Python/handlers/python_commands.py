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

        if success:
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

        destructive_keywords = ["delete", "save", "quit", "exit", "restart"]
        is_destructive = any(keyword in cmd.lower() for keyword in destructive_keywords)

        if is_destructive and not force:
            log.log_warning("Potentially destructive command detected")
            return {
                "success": False,
                "error": ("This command may involve destructive actions (e.g., deleting or saving). "
                          "Please confirm with 'Yes, execute it' or set force=True.")
            }

        world = unreal.EditorLevelLibrary.get_editor_world()
        unreal.SystemLibrary.execute_console_command(world, cmd)
        log.log_result("execute_unreal_command", True, f"Command '{cmd}' executed")
        return {"success": True, "output": f"Command '{cmd}' executed successfully"}

    except Exception as e:
        log.log_error(f"Error handling execute_unreal_command: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}