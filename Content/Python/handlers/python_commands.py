import unreal
import sys
import io
from typing import Dict, Any

# Assuming a logging module similar to your example
from utils import logging as log

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

        log.log_command("execute_python", f"Script: {script[:50]}...")  # Truncate for brevity

        # Check for potentially destructive actions
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

        # Capture output and execute the script
        old_stdout = sys.stdout
        sys.stdout = output = io.StringIO()
        try:
            exec(script)  # Execute the script in Unreal's Python environment
            script_output = output.getvalue().strip()
            log.log_result("execute_python", True, f"Script executed with output: {script_output}")
            return {"success": True, "output": script_output}
        finally:
            sys.stdout = old_stdout

    except Exception as e:
        log.log_error(f"Error executing Python script: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}