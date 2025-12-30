"""
Kismet Editor Utilities Commands Handler
Nuetzliche Blueprint-Editor Funktionen

Commands:
- create_blueprint_from_actor: Blueprint aus bestehendem Actor erstellen
- is_actor_valid_for_level_script: Kann Actor in Level Script?
- get_default_object: Default Object einer Klasse holen
"""

import unreal
from typing import Dict, Any
from utils import logging as log


def handle_create_blueprint_from_actor(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Create a Blueprint from an existing Actor in the level

    Args:
        command: Dictionary with:
            - actor_name: Name/label of the actor to convert
            - blueprint_path: Path for the new Blueprint (e.g., "/Game/Generated/BP_FromActor")
            - replace_actor: If True, replace actor with BP instance (default: False)
    """
    try:
        actor_name = command.get("actor_name")
        blueprint_path = command.get("blueprint_path")
        replace_actor = command.get("replace_actor", False)

        if not actor_name or not blueprint_path:
            return {"success": False, "error": "actor_name and blueprint_path required"}

        log.log_command("create_blueprint_from_actor", f"Creating BP from {actor_name}")

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

        # Create Blueprint from Actor
        bp = unreal.KismetEditorUtilities.create_blueprint_from_actor(
            blueprint_path,
            target_actor,
            replace_actor
        )

        if bp:
            log.log_result("create_blueprint_from_actor", True, f"Created {blueprint_path}")
            return {
                "success": True,
                "message": f"Created Blueprint from '{actor_name}'",
                "blueprint_path": blueprint_path,
                "replaced_actor": replace_actor
            }
        else:
            return {"success": False, "error": "Failed to create Blueprint"}

    except Exception as e:
        log.log_error(f"Error creating blueprint from actor: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_is_actor_valid_for_level_script(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Check if an actor can be referenced in Level Blueprint

    Args:
        command: Dictionary with:
            - actor_name: Name/label of the actor
    """
    try:
        actor_name = command.get("actor_name")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

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

        is_valid = unreal.KismetEditorUtilities.is_actor_valid_for_level_script(target_actor)

        return {
            "success": True,
            "actor_name": actor_name,
            "valid_for_level_script": is_valid
        }

    except Exception as e:
        log.log_error(f"Error checking actor validity: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_compile_blueprint_by_path(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Compile a Blueprint by path using KismetEditorUtilities

    Args:
        command: Dictionary with:
            - blueprint_path: Path to the Blueprint
    """
    try:
        blueprint_path = command.get("blueprint_path")

        if not blueprint_path:
            return {"success": False, "error": "blueprint_path required"}

        log.log_command("compile_blueprint_by_path", f"Compiling {blueprint_path}")

        bp = unreal.EditorAssetLibrary.load_asset(blueprint_path)
        if not bp:
            return {"success": False, "error": f"Blueprint not found: {blueprint_path}"}

        unreal.KismetEditorUtilities.compile_blueprint(bp)

        log.log_result("compile_blueprint_by_path", True, "Compiled")
        return {
            "success": True,
            "message": f"Compiled Blueprint: {blueprint_path}"
        }

    except Exception as e:
        log.log_error(f"Error compiling blueprint: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


# ============================================================================
# COMMAND REGISTRY
# ============================================================================

KISMET_UTILITIES_COMMANDS = {
    "create_blueprint_from_actor": handle_create_blueprint_from_actor,
    "is_actor_valid_for_level_script": handle_is_actor_valid_for_level_script,
    "compile_blueprint_by_path": handle_compile_blueprint_by_path,
}


def get_kismet_utilities_commands():
    """Return all available kismet utilities commands"""
    return KISMET_UTILITIES_COMMANDS
