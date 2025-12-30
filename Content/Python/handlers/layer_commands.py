"""
Layer Commands Handler
Nutzt LayersSubsystem fuer Actor-Organisation

Commands:
- create_layer: Layer erstellen
- delete_layer: Layer loeschen
- rename_layer: Layer umbenennen
- add_actor_to_layer: Actor zu Layer hinzufuegen
- remove_actor_from_layer: Actor aus Layer entfernen
- get_actors_from_layer: Alle Actors eines Layers
- set_layer_visibility: Layer ein/ausblenden
- get_all_layers: Alle Layer auflisten
- add_actors_to_layer_by_pattern: Mehrere Actors nach Pattern zu Layer
"""

import unreal
from typing import Dict, Any, List
from utils import logging as log


def get_layers_subsystem():
    """Get the LayersSubsystem"""
    return unreal.get_editor_subsystem(unreal.LayersSubsystem)


def get_actor_subsystem():
    """Get the EditorActorSubsystem"""
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def handle_create_layer(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Create a new layer

    Args:
        command: Dictionary with:
            - layer_name: Name of the layer to create
    """
    try:
        layer_name = command.get("layer_name")

        if not layer_name:
            return {"success": False, "error": "layer_name required"}

        log.log_command("create_layer", f"Creating layer: {layer_name}")

        layers = get_layers_subsystem()
        layers.create_layer(layer_name)

        log.log_result("create_layer", True, f"Layer '{layer_name}' created")
        return {"success": True, "message": f"Layer '{layer_name}' created", "layer_name": layer_name}

    except Exception as e:
        log.log_error(f"Error creating layer: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_delete_layer(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Delete a layer

    Args:
        command: Dictionary with:
            - layer_name: Name of the layer to delete
    """
    try:
        layer_name = command.get("layer_name")

        if not layer_name:
            return {"success": False, "error": "layer_name required"}

        log.log_command("delete_layer", f"Deleting layer: {layer_name}")

        layers = get_layers_subsystem()
        layers.delete_layer(layer_name)

        log.log_result("delete_layer", True, f"Layer '{layer_name}' deleted")
        return {"success": True, "message": f"Layer '{layer_name}' deleted"}

    except Exception as e:
        log.log_error(f"Error deleting layer: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_rename_layer(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Rename a layer

    Args:
        command: Dictionary with:
            - old_name: Current layer name
            - new_name: New layer name
    """
    try:
        old_name = command.get("old_name")
        new_name = command.get("new_name")

        if not old_name or not new_name:
            return {"success": False, "error": "old_name and new_name required"}

        log.log_command("rename_layer", f"Renaming layer: {old_name} -> {new_name}")

        layers = get_layers_subsystem()
        success = layers.rename_layer(old_name, new_name)

        if success:
            log.log_result("rename_layer", True, f"Layer renamed to '{new_name}'")
            return {"success": True, "message": f"Layer renamed to '{new_name}'", "new_name": new_name}
        else:
            return {"success": False, "error": f"Failed to rename layer '{old_name}'"}

    except Exception as e:
        log.log_error(f"Error renaming layer: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_add_actor_to_layer(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Add an actor to a layer

    Args:
        command: Dictionary with:
            - actor_name: Name/label of the actor
            - layer_name: Name of the layer
    """
    try:
        actor_name = command.get("actor_name")
        layer_name = command.get("layer_name")

        if not actor_name or not layer_name:
            return {"success": False, "error": "actor_name and layer_name required"}

        log.log_command("add_actor_to_layer", f"Adding {actor_name} to layer {layer_name}")

        # Find the actor
        actor_sub = get_actor_subsystem()
        actors = actor_sub.get_all_level_actors()

        target_actor = None
        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                target_actor = actor
                break

        if not target_actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        # Add to layer
        layers = get_layers_subsystem()
        success = layers.add_actor_to_layer(target_actor, layer_name)

        if success:
            log.log_result("add_actor_to_layer", True, f"Added {actor_name} to {layer_name}")
            return {"success": True, "message": f"Added '{actor_name}' to layer '{layer_name}'"}
        else:
            return {"success": False, "error": f"Failed to add actor to layer"}

    except Exception as e:
        log.log_error(f"Error adding actor to layer: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_remove_actor_from_layer(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Remove an actor from a layer

    Args:
        command: Dictionary with:
            - actor_name: Name/label of the actor
            - layer_name: Name of the layer
    """
    try:
        actor_name = command.get("actor_name")
        layer_name = command.get("layer_name")

        if not actor_name or not layer_name:
            return {"success": False, "error": "actor_name and layer_name required"}

        log.log_command("remove_actor_from_layer", f"Removing {actor_name} from layer {layer_name}")

        # Find the actor
        actor_sub = get_actor_subsystem()
        actors = actor_sub.get_all_level_actors()

        target_actor = None
        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                target_actor = actor
                break

        if not target_actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        # Remove from layer
        layers = get_layers_subsystem()
        success = layers.remove_actor_from_layer(target_actor, layer_name)

        if success:
            log.log_result("remove_actor_from_layer", True, f"Removed {actor_name} from {layer_name}")
            return {"success": True, "message": f"Removed '{actor_name}' from layer '{layer_name}'"}
        else:
            return {"success": False, "error": f"Failed to remove actor from layer"}

    except Exception as e:
        log.log_error(f"Error removing actor from layer: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_actors_from_layer(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Get all actors in a layer

    Args:
        command: Dictionary with:
            - layer_name: Name of the layer
    """
    try:
        layer_name = command.get("layer_name")

        if not layer_name:
            return {"success": False, "error": "layer_name required"}

        log.log_command("get_actors_from_layer", f"Getting actors from layer: {layer_name}")

        layers = get_layers_subsystem()
        actors = layers.get_actors_from_layer(layer_name)

        actor_list = []
        for actor in actors:
            actor_list.append({
                "name": actor.get_name(),
                "label": actor.get_actor_label(),
                "class": actor.get_class().get_name(),
                "location": [
                    actor.get_actor_location().x,
                    actor.get_actor_location().y,
                    actor.get_actor_location().z
                ]
            })

        log.log_result("get_actors_from_layer", True, f"Found {len(actor_list)} actors")
        return {
            "success": True,
            "layer_name": layer_name,
            "actors": actor_list,
            "count": len(actor_list)
        }

    except Exception as e:
        log.log_error(f"Error getting actors from layer: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_set_layer_visibility(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Set layer visibility

    Args:
        command: Dictionary with:
            - layer_name: Name of the layer
            - visible: True/False
    """
    try:
        layer_name = command.get("layer_name")
        visible = command.get("visible", True)

        if not layer_name:
            return {"success": False, "error": "layer_name required"}

        log.log_command("set_layer_visibility", f"Setting {layer_name} visibility to {visible}")

        layers = get_layers_subsystem()
        layers.set_layer_visibility(layer_name, visible)

        status = "visible" if visible else "hidden"
        log.log_result("set_layer_visibility", True, f"Layer {layer_name} is now {status}")
        return {
            "success": True,
            "message": f"Layer '{layer_name}' is now {status}",
            "layer_name": layer_name,
            "visible": visible
        }

    except Exception as e:
        log.log_error(f"Error setting layer visibility: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_all_layers(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Get all layers in the level
    """
    try:
        log.log_command("get_all_layers", "Getting all layers")

        layers = get_layers_subsystem()
        all_layers = layers.get_all_layers()

        layer_list = []
        for layer in all_layers:
            layer_list.append(str(layer))

        log.log_result("get_all_layers", True, f"Found {len(layer_list)} layers")
        return {
            "success": True,
            "layers": layer_list,
            "count": len(layer_list)
        }

    except Exception as e:
        log.log_error(f"Error getting all layers: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_add_actors_to_layer_by_pattern(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Add multiple actors to a layer by name pattern

    Args:
        command: Dictionary with:
            - pattern: Pattern to match actor names (e.g., "NPC", "Quest", "Enemy")
            - layer_name: Name of the layer
            - create_layer: If True, create layer if it doesn't exist (default: True)
    """
    try:
        pattern = command.get("pattern")
        layer_name = command.get("layer_name")
        create_layer = command.get("create_layer", True)

        if not pattern or not layer_name:
            return {"success": False, "error": "pattern and layer_name required"}

        log.log_command("add_actors_to_layer_by_pattern", f"Adding actors matching '{pattern}' to layer {layer_name}")

        layers = get_layers_subsystem()
        actor_sub = get_actor_subsystem()

        # Create layer if needed
        if create_layer:
            try:
                layers.create_layer(layer_name)
            except:
                pass  # Layer might already exist

        # Find matching actors
        all_actors = actor_sub.get_all_level_actors()
        added_count = 0
        added_actors = []

        for actor in all_actors:
            actor_label = actor.get_actor_label()
            actor_name = actor.get_name()

            if pattern in actor_label or pattern in actor_name:
                try:
                    layers.add_actor_to_layer(actor, layer_name)
                    added_count += 1
                    added_actors.append(actor_label)
                except:
                    pass

        log.log_result("add_actors_to_layer_by_pattern", True, f"Added {added_count} actors to {layer_name}")
        return {
            "success": True,
            "message": f"Added {added_count} actors matching '{pattern}' to layer '{layer_name}'",
            "layer_name": layer_name,
            "pattern": pattern,
            "added_count": added_count,
            "added_actors": added_actors[:20]  # Limit to first 20 for response size
        }

    except Exception as e:
        log.log_error(f"Error adding actors by pattern: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


# ============================================================================
# COMMAND REGISTRY
# ============================================================================

LAYER_COMMANDS = {
    "create_layer": handle_create_layer,
    "delete_layer": handle_delete_layer,
    "rename_layer": handle_rename_layer,
    "add_actor_to_layer": handle_add_actor_to_layer,
    "remove_actor_from_layer": handle_remove_actor_from_layer,
    "get_actors_from_layer": handle_get_actors_from_layer,
    "set_layer_visibility": handle_set_layer_visibility,
    "get_all_layers": handle_get_all_layers,
    "add_actors_to_layer_by_pattern": handle_add_actors_to_layer_by_pattern,
}


def get_layer_commands():
    """Return all available layer commands"""
    return LAYER_COMMANDS
