import unreal
from typing import Dict, Any
from utils import logging as log
import json # Import json for parsing C++ response

# Lazy load the C++ utils class to avoid issues during Unreal init
_widget_gen_utils = None
def get_widget_gen_utils():
    global _widget_gen_utils
    if _widget_gen_utils is None:
        try:
            # Attempt to load the class generated from C++
            # Make sure your C++ class is compiled and accessible to Python
            _widget_gen_utils = unreal.GenWidgetUtils()
        except Exception as e:
            log.log_error(f"Failed to get unreal.WidgetGenUtils: {e}. Ensure C++ module is compiled and loaded.")
            raise  # Re-raise to signal failure
    return _widget_gen_utils

def handle_add_widget_to_user_widget(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handles adding a widget component to a User Widget Blueprint.
    """
    try:
        widget_gen_utils = get_widget_gen_utils()
        user_widget_path = command.get("user_widget_path")
        widget_type = command.get("widget_type")
        widget_name = command.get("widget_name")
        parent_name = command.get("parent_widget_name", "") # Default to empty string if not provided

        log.log_command("add_widget_to_user_widget", f"Path: {user_widget_path}, Type: {widget_type}, Name: {widget_name}, Parent: {parent_name}")

        if not all([user_widget_path, widget_type, widget_name]):
            return {"success": False, "error": "Missing required arguments: user_widget_path, widget_type, widget_name"}

        # Call the C++ function
        response_str = widget_gen_utils.add_widget_to_user_widget(user_widget_path, widget_type, widget_name, parent_name)

        # Parse the JSON response from C++
        response_json = json.loads(response_str)
        log.log_result("add_widget_to_user_widget", response_json.get("success", False), response_json.get("message") or response_json.get("error"))
        return response_json

    except Exception as e:
        log.log_error(f"Error in handle_add_widget_to_user_widget: {str(e)}", include_traceback=True)
        return {"success": False, "error": f"Python Handler Error: {str(e)}"}

def handle_edit_widget_property(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handles editing a property of a widget inside a User Widget Blueprint.
    """
    try:
        widget_gen_utils = get_widget_gen_utils()
        user_widget_path = command.get("user_widget_path")
        widget_name = command.get("widget_name")
        property_name = command.get("property_name")
        value_str = command.get("value") # Value is expected as a string

        log.log_command("edit_widget_property", f"Path: {user_widget_path}, Widget: {widget_name}, Property: {property_name}, Value: {value_str}")

        if not all([user_widget_path, widget_name, property_name, value_str is not None]):
            return {"success": False, "error": "Missing required arguments: user_widget_path, widget_name, property_name, value"}

        # Call the C++ function
        response_str = widget_gen_utils.edit_widget_property(user_widget_path, widget_name, property_name, value_str)

        # Parse the JSON response from C++
        response_json = json.loads(response_str)
        log.log_result("edit_widget_property", response_json.get("success", False), response_json.get("message") or response_json.get("error"))
        return response_json

    except Exception as e:
        log.log_error(f"Error in handle_edit_widget_property: {str(e)}", include_traceback=True)
        return {"success": False, "error": f"Python Handler Error: {str(e)}"}