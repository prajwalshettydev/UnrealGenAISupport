import unreal
import json
from typing import Dict, Any, List, Tuple, Union

from utils import unreal_conversions as uc
from utils import logging as log


def handle_modify_object(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a modify_object command
    
    Args:
        command: The command dictionary containing:
            - actor_name: Name of the actor to modify
            - property_type: Type of property to modify (material, position, rotation, scale)
            - value: Value to set for the property
            
    Returns:
        Response dictionary with success/failure status
    """
    try:
        # Extract parameters
        actor_name = command.get("actor_name")
        property_type = command.get("property_type")
        value = command.get("value")

        if not actor_name or not property_type or value is None:
            log.log_error("Missing required parameters for modify_object")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("modify_object", f"Actor: {actor_name}, Property: {property_type}")

        # Use the C++ utility class
        gen_actor_utils = unreal.GenActorUtils

        # Handle different property types
        if property_type == "material":
            # Set the material
            material_path = value
            success = gen_actor_utils.set_actor_material_by_path(actor_name, material_path)
            if success:
                log.log_result("modify_object", True, f"Material of {actor_name} set to {material_path}")
                return {"success": True}
            log.log_error(f"Failed to set material for {actor_name}")
            return {"success": False, "error": f"Failed to set material for {actor_name}"}

        elif property_type == "position":
            # Set position
            try:
                vec = uc.to_unreal_vector(value)
                success = gen_actor_utils.set_actor_position(actor_name, vec)
                if success:
                    log.log_result("modify_object", True, f"Position of {actor_name} set to {vec}")
                    return {"success": True}
                log.log_error(f"Failed to set position for {actor_name}")
                return {"success": False, "error": f"Failed to set position for {actor_name}"}
            except ValueError as e:
                log.log_error(str(e))
                return {"success": False, "error": str(e)}

        elif property_type == "rotation":
            # Set rotation
            try:
                rot = uc.to_unreal_rotator(value)
                success = gen_actor_utils.set_actor_rotation(actor_name, rot)
                if success:
                    log.log_result("modify_object", True, f"Rotation of {actor_name} set to {rot}")
                    return {"success": True}
                log.log_error(f"Failed to set rotation for {actor_name}")
                return {"success": False, "error": f"Failed to set rotation for {actor_name}"}
            except ValueError as e:
                log.log_error(str(e))
                return {"success": False, "error": str(e)}

        elif property_type == "scale":
            # Set scale
            try:
                scale = uc.to_unreal_vector(value)
                success = gen_actor_utils.set_actor_scale(actor_name, scale)
                if success:
                    log.log_result("modify_object", True, f"Scale of {actor_name} set to {scale}")
                    return {"success": True}
                log.log_error(f"Failed to set scale for {actor_name}")
                return {"success": False, "error": f"Failed to set scale for {actor_name}"}
            except ValueError as e:
                log.log_error(str(e))
                return {"success": False, "error": str(e)}

        else:
            log.log_error(f"Unknown property type: {property_type}")
            return {"success": False, "error": f"Unknown property type: {property_type}"}

    except Exception as e:
        log.log_error(f"Error modifying object: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}

def handle_edit_component_property(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to edit a component property in a Blueprint or scene actor.
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint
            - component_name: Name of the component
            - property_name: Name of the property to edit
            - value: New value as a string
            - is_scene_actor: Boolean flag for scene actor (optional, default False)
            - actor_name: Name of the scene actor (required if is_scene_actor is True)
                
    Returns:
        Dictionary with success/failure status, message, and optional suggestions
    """
    try:
        blueprint_path = command.get("blueprint_path", "")
        component_name = command.get("component_name")
        property_name = command.get("property_name")
        value = command.get("value")
        is_scene_actor = command.get("is_scene_actor", False)
        actor_name = command.get("actor_name", "")

        if not component_name or not property_name or value is None:
            log.log_error("Missing required parameters for edit_component_property")
            return {"success": False, "error": "Missing required parameters"}

        if is_scene_actor and not actor_name:
            log.log_error("Actor name required for scene actor editing")
            return {"success": False, "error": "Actor name required for scene actor"}

        # Log detailed information about what we're trying to do
        log_msg = f"Blueprint: {blueprint_path}, Component: {component_name}, Property: {property_name}, Value: {value}"
        if is_scene_actor:
            log_msg += f", Actor: {actor_name}"
        log.log_command("edit_component_property", log_msg)

        # Call the C++ implementation
        node_creator = unreal.GenObjectProperties
        result = node_creator.edit_component_property(blueprint_path, component_name, property_name, value, is_scene_actor, actor_name)

        # Parse the result - CHANGED: Convert JSON string to dict
        try:
            parsed_result = json.loads(result)
            log.log_result("edit_component_property", parsed_result["success"],
                           parsed_result.get("message", parsed_result.get("error", "No message")))
            # CHANGED: Return the parsed dict instead of the original JSON string
            return parsed_result
        except json.JSONDecodeError:
            # If the result is not valid JSON, wrap it in a JSON object
            log.log_warning(f"Invalid JSON result from C++: {result}")
            return {"success": False, "error": f"Invalid response format: {result}"}

    except Exception as e:
        log.log_error(f"Error editing component property: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_add_component_with_events(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to add a component to a Blueprint with overlap events if applicable.
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint
            - component_name: Name of the new component
            - component_class: Class of the component (e.g., "BoxComponent")
                
    Returns:
        Response dictionary with success/failure status, message, and event GUIDs
    """
    try:
        blueprint_path = command.get("blueprint_path")
        component_name = command.get("component_name")
        component_class = command.get("component_class")

        if not blueprint_path or not component_name or not component_class:
            log.log_error("Missing required parameters for add_component_with_events")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("add_component_with_events", f"Blueprint: {blueprint_path}, Component: {component_name}, Class: {component_class}")

        node_creator = unreal.GenBlueprintUtils
        result = node_creator.add_component_with_events(blueprint_path, component_name, component_class)

        import json
        parsed_result = json.loads(result)
        log.log_result("add_component_with_events", parsed_result["success"], parsed_result.get("message", parsed_result.get("error", "No message")))
        return parsed_result

    except Exception as e:
        log.log_error(f"Error adding component with events: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}

def handle_create_game_mode(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        game_mode_path = command.get("game_mode_path")
        pawn_blueprint_path = command.get("pawn_blueprint_path")
        base_class = command.get("base_class", "GameModeBase")

        if not game_mode_path or not pawn_blueprint_path:
            return {"success": False, "error": "Missing game_mode_path or pawn_blueprint_path"}

        node_creator = unreal.GenActorUtils
        result = node_creator.create_game_mode_with_pawn(game_mode_path, pawn_blueprint_path, base_class)
        return json.loads(result)
    except Exception as e:
        return {"success": False, "error": str(e)}
