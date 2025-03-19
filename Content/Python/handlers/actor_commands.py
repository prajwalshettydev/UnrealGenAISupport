import unreal
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