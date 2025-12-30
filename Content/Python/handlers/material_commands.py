"""
Material Editing Commands Handler
Erweiterte Material-Manipulation

Commands:
- set_material_scalar_param: Scalar Parameter setzen
- set_material_vector_param: Vector/Color Parameter setzen
- set_material_texture_param: Texture Parameter setzen
- get_material_info: Material Infos abfragen
- recompile_material: Material neu kompilieren
- set_actor_material: Material auf Actor setzen (WICHTIG!)
"""

import unreal
from typing import Dict, Any, List
from utils import logging as log


def handle_set_material_scalar_param(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Set a scalar parameter on a Material Instance

    Args:
        command: Dictionary with:
            - material_path: Path to Material Instance
            - param_name: Parameter name
            - value: Float value
    """
    try:
        material_path = command.get("material_path")
        param_name = command.get("param_name")
        value = command.get("value")

        if not material_path or not param_name or value is None:
            return {"success": False, "error": "material_path, param_name, and value required"}

        log.log_command("set_material_scalar_param", f"Setting {param_name}={value}")

        mat = unreal.EditorAssetLibrary.load_asset(material_path)
        if not mat:
            return {"success": False, "error": f"Material not found: {material_path}"}

        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            mat, param_name, float(value)
        )

        log.log_result("set_material_scalar_param", True, "Parameter set")
        return {
            "success": True,
            "message": f"Set {param_name} = {value}",
            "material_path": material_path
        }

    except Exception as e:
        log.log_error(f"Error setting scalar param: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_set_material_vector_param(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Set a vector/color parameter on a Material Instance

    Args:
        command: Dictionary with:
            - material_path: Path to Material Instance
            - param_name: Parameter name
            - value: [R, G, B, A] or [X, Y, Z, W]
    """
    try:
        material_path = command.get("material_path")
        param_name = command.get("param_name")
        value = command.get("value")

        if not material_path or not param_name or not value:
            return {"success": False, "error": "material_path, param_name, and value required"}

        log.log_command("set_material_vector_param", f"Setting {param_name}")

        mat = unreal.EditorAssetLibrary.load_asset(material_path)
        if not mat:
            return {"success": False, "error": f"Material not found: {material_path}"}

        # Convert to LinearColor
        color = unreal.LinearColor(
            r=float(value[0]) if len(value) > 0 else 0,
            g=float(value[1]) if len(value) > 1 else 0,
            b=float(value[2]) if len(value) > 2 else 0,
            a=float(value[3]) if len(value) > 3 else 1
        )

        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            mat, param_name, color
        )

        log.log_result("set_material_vector_param", True, "Parameter set")
        return {
            "success": True,
            "message": f"Set {param_name} = {value}",
            "material_path": material_path
        }

    except Exception as e:
        log.log_error(f"Error setting vector param: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_set_material_texture_param(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Set a texture parameter on a Material Instance

    Args:
        command: Dictionary with:
            - material_path: Path to Material Instance
            - param_name: Parameter name
            - texture_path: Path to texture asset
    """
    try:
        material_path = command.get("material_path")
        param_name = command.get("param_name")
        texture_path = command.get("texture_path")

        if not material_path or not param_name or not texture_path:
            return {"success": False, "error": "material_path, param_name, and texture_path required"}

        log.log_command("set_material_texture_param", f"Setting {param_name}")

        mat = unreal.EditorAssetLibrary.load_asset(material_path)
        if not mat:
            return {"success": False, "error": f"Material not found: {material_path}"}

        texture = unreal.EditorAssetLibrary.load_asset(texture_path)
        if not texture:
            return {"success": False, "error": f"Texture not found: {texture_path}"}

        unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
            mat, param_name, texture
        )

        log.log_result("set_material_texture_param", True, "Texture set")
        return {
            "success": True,
            "message": f"Set {param_name} to {texture_path}",
            "material_path": material_path
        }

    except Exception as e:
        log.log_error(f"Error setting texture param: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_material_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Get information about a material

    Args:
        command: Dictionary with:
            - material_path: Path to Material
    """
    try:
        material_path = command.get("material_path")

        if not material_path:
            return {"success": False, "error": "material_path required"}

        log.log_command("get_material_info", f"Getting info for {material_path}")

        mat = unreal.EditorAssetLibrary.load_asset(material_path)
        if not mat:
            return {"success": False, "error": f"Material not found: {material_path}"}

        # Get statistics
        stats = unreal.MaterialEditingLibrary.get_statistics(mat)

        # Get used textures
        textures = unreal.MaterialEditingLibrary.get_used_textures(mat)
        texture_list = [str(t.get_path_name()) for t in textures]

        # Get expression count
        expr_count = unreal.MaterialEditingLibrary.get_num_material_expressions(mat)

        return {
            "success": True,
            "material_path": material_path,
            "expression_count": expr_count,
            "used_textures": texture_list,
            "texture_count": len(texture_list)
        }

    except Exception as e:
        log.log_error(f"Error getting material info: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_recompile_material(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Recompile a material

    Args:
        command: Dictionary with:
            - material_path: Path to Material
    """
    try:
        material_path = command.get("material_path")

        if not material_path:
            return {"success": False, "error": "material_path required"}

        log.log_command("recompile_material", f"Recompiling {material_path}")

        mat = unreal.EditorAssetLibrary.load_asset(material_path)
        if not mat:
            return {"success": False, "error": f"Material not found: {material_path}"}

        unreal.MaterialEditingLibrary.recompile_material(mat)

        log.log_result("recompile_material", True, "Recompiled")
        return {
            "success": True,
            "message": f"Recompiled material: {material_path}"
        }

    except Exception as e:
        log.log_error(f"Error recompiling material: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_set_actor_material(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Set material on an actor's mesh component - THE IMPORTANT ONE!

    Args:
        command: Dictionary with:
            - actor_name: Name/label of the actor
            - material_path: Path to material asset
            - slot_index: Material slot index (default: 0)
            - component_name: Optional specific component name
    """
    try:
        actor_name = command.get("actor_name")
        material_path = command.get("material_path")
        slot_index = command.get("slot_index", 0)
        component_name = command.get("component_name", None)

        if not actor_name or not material_path:
            return {"success": False, "error": "actor_name and material_path required"}

        log.log_command("set_actor_material", f"Setting material on {actor_name}")

        # Find the actor
        actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        actors = actor_sub.get_all_level_actors()

        target_actor = None
        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                target_actor = actor
                break

        if not target_actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        # Load material
        material = unreal.EditorAssetLibrary.load_asset(material_path)
        if not material:
            return {"success": False, "error": f"Material not found: {material_path}"}

        # Find mesh component
        components = target_actor.get_components_by_class(unreal.StaticMeshComponent)
        if not components:
            components = target_actor.get_components_by_class(unreal.SkeletalMeshComponent)

        if not components:
            return {"success": False, "error": "No mesh component found on actor"}

        # Use specific component or first one
        target_component = None
        if component_name:
            for comp in components:
                if comp.get_name() == component_name:
                    target_component = comp
                    break
        else:
            target_component = components[0]

        if not target_component:
            return {"success": False, "error": f"Component not found: {component_name}"}

        # Set material
        target_component.set_material(slot_index, material)

        log.log_result("set_actor_material", True, f"Material set on slot {slot_index}")
        return {
            "success": True,
            "message": f"Set material on '{actor_name}' slot {slot_index}",
            "material_path": material_path,
            "slot_index": slot_index
        }

    except Exception as e:
        log.log_error(f"Error setting actor material: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_create_material_instance(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Create a Material Instance from a parent material

    Args:
        command: Dictionary with:
            - parent_material_path: Path to parent material
            - instance_name: Name for the new instance
            - save_path: Path to save (default: /Game/Materials)
    """
    try:
        parent_path = command.get("parent_material_path")
        instance_name = command.get("instance_name")
        save_path = command.get("save_path", "/Game/Materials")

        if not parent_path or not instance_name:
            return {"success": False, "error": "parent_material_path and instance_name required"}

        log.log_command("create_material_instance", f"Creating {instance_name}")

        parent = unreal.EditorAssetLibrary.load_asset(parent_path)
        if not parent:
            return {"success": False, "error": f"Parent material not found: {parent_path}"}

        # Create Material Instance
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.MaterialInstanceConstantFactoryNew()

        instance_path = f"{save_path}/{instance_name}"
        mat_instance = asset_tools.create_asset(
            instance_name,
            save_path,
            unreal.MaterialInstanceConstant,
            factory
        )

        if mat_instance:
            # Set parent
            unreal.MaterialEditingLibrary.set_material_instance_parent(mat_instance, parent)
            unreal.EditorAssetLibrary.save_asset(instance_path)

            log.log_result("create_material_instance", True, f"Created {instance_path}")
            return {
                "success": True,
                "message": f"Created Material Instance: {instance_name}",
                "instance_path": instance_path,
                "parent_path": parent_path
            }
        else:
            return {"success": False, "error": "Failed to create material instance"}

    except Exception as e:
        log.log_error(f"Error creating material instance: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


# ============================================================================
# COMMAND REGISTRY
# ============================================================================

MATERIAL_COMMANDS = {
    "set_material_scalar_param": handle_set_material_scalar_param,
    "set_material_vector_param": handle_set_material_vector_param,
    "set_material_texture_param": handle_set_material_texture_param,
    "get_material_info": handle_get_material_info,
    "recompile_material": handle_recompile_material,
    "set_actor_material": handle_set_actor_material,
    "create_material_instance": handle_create_material_instance,
}


def get_material_commands():
    """Return all available material commands"""
    return MATERIAL_COMMANDS
