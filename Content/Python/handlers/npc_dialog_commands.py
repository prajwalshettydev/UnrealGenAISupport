"""
NPC Dialog Commands Handler
Vollstaendiges NPC Dialog System

Commands:
- configure_npc_dialog: Dialog-Component konfigurieren
- start_npc_dialog: Dialog starten
- end_npc_dialog: Dialog beenden
- send_npc_message: Nachricht an NPC senden
- set_npc_mood: Mood setzen
- modify_npc_reputation: Reputation aendern
- get_npc_dialog_state: Dialog-State abfragen
- add_dialog_choice: Choice hinzufuegen
- set_dialog_choices: Alle Choices setzen
- is_npc_server_running: Server-Status pruefen
- spawn_npc_on_ground: NPC spawnen (am Boden!)
- find_ground_position: Boden finden
"""

import unreal
from typing import Dict, Any, List
from utils import logging as log


def get_actor_by_name(actor_name: str):
    """Helper: Find actor by name or label"""
    actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_sub.get_all_level_actors()

    for actor in actors:
        if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
            return actor
    return None


def get_dialog_component(actor):
    """Helper: Get NPCDialogComponent from actor"""
    components = actor.get_components_by_class(unreal.NPCDialogComponent)
    if components:
        return components[0]
    return None


def handle_configure_npc_dialog(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Configure NPCDialogComponent on an actor

    Args:
        command: Dictionary with:
            - actor_name: Name of the NPC actor
            - npc_id: Unique NPC ID (for AI personality)
            - display_name: Display name for UI
            - title: Title/role (e.g., "Blacksmith")
            - portrait_path: Path to portrait texture
            - faction_id: Faction ID
            - shop_id: Shop ID if merchant
            - can_trade: Can trade with player
            - greeting_message: First message when dialog starts
            - farewell_message: Message when dialog ends
            - interaction_range: Dialog range (default 300)
    """
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actor = get_actor_by_name(actor_name)
        if not actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        log.log_command("configure_npc_dialog", f"Configuring {actor_name}")

        # Get or add component
        dialog_comp = get_dialog_component(actor)
        if not dialog_comp:
            # Add component if not exists
            dialog_comp = actor.add_component_by_class(unreal.NPCDialogComponent)
            if not dialog_comp:
                return {"success": False, "error": "Failed to add NPCDialogComponent"}

        # Configure
        if "npc_id" in command:
            dialog_comp.set_editor_property("npcid", command["npc_id"])
        if "display_name" in command:
            dialog_comp.set_editor_property("display_name", unreal.Text(command["display_name"]))
        if "title" in command:
            dialog_comp.set_editor_property("title", unreal.Text(command["title"]))
        if "portrait_path" in command:
            dialog_comp.set_editor_property("portrait_path", command["portrait_path"])
        if "faction_id" in command:
            dialog_comp.set_editor_property("faction_id", command["faction_id"])
        if "shop_id" in command:
            dialog_comp.set_editor_property("shop_id", command["shop_id"])
        if "can_trade" in command:
            dialog_comp.set_editor_property("b_can_trade", command["can_trade"])
        if "greeting_message" in command:
            dialog_comp.set_editor_property("greeting_message", unreal.Text(command["greeting_message"]))
        if "farewell_message" in command:
            dialog_comp.set_editor_property("farewell_message", unreal.Text(command["farewell_message"]))
        if "interaction_range" in command:
            dialog_comp.set_editor_property("interaction_range", float(command["interaction_range"]))

        log.log_result("configure_npc_dialog", True, "Configured")
        return {
            "success": True,
            "message": f"Configured NPCDialogComponent on '{actor_name}'",
            "actor_name": actor_name
        }

    except Exception as e:
        log.log_error(f"Error configuring NPC dialog: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_start_npc_dialog(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Start dialog with an NPC

    Args:
        command: Dictionary with:
            - actor_name: Name of the NPC
            - player_actor_name: Name of player actor (optional)
    """
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actor = get_actor_by_name(actor_name)
        if not actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        dialog_comp = get_dialog_component(actor)
        if not dialog_comp:
            return {"success": False, "error": "No NPCDialogComponent on actor"}

        # Find player actor if specified
        player_actor = None
        if command.get("player_actor_name"):
            player_actor = get_actor_by_name(command["player_actor_name"])

        success = dialog_comp.start_dialog(player_actor)

        if success:
            return {"success": True, "message": f"Started dialog with '{actor_name}'"}
        else:
            return {"success": False, "error": "Failed to start dialog"}

    except Exception as e:
        log.log_error(f"Error starting dialog: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_end_npc_dialog(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    End dialog with an NPC

    Args:
        command: Dictionary with:
            - actor_name: Name of the NPC
    """
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actor = get_actor_by_name(actor_name)
        if not actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        dialog_comp = get_dialog_component(actor)
        if not dialog_comp:
            return {"success": False, "error": "No NPCDialogComponent on actor"}

        dialog_comp.end_dialog()

        return {"success": True, "message": f"Ended dialog with '{actor_name}'"}

    except Exception as e:
        log.log_error(f"Error ending dialog: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_send_npc_message(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Send a message to an NPC (via DialogComponent)

    Args:
        command: Dictionary with:
            - actor_name: Name of the NPC
            - message: Message text
            - async: Send asynchronously (default: False)
    """
    try:
        actor_name = command.get("actor_name")
        message = command.get("message")
        use_async = command.get("async", False)

        if not actor_name or not message:
            return {"success": False, "error": "actor_name and message required"}

        actor = get_actor_by_name(actor_name)
        if not actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        dialog_comp = get_dialog_component(actor)
        if not dialog_comp:
            return {"success": False, "error": "No NPCDialogComponent on actor"}

        if use_async:
            dialog_comp.send_message_async(message)
        else:
            dialog_comp.send_message(message)

        return {
            "success": True,
            "message": f"Sent message to '{actor_name}'",
            "sent_message": message
        }

    except Exception as e:
        log.log_error(f"Error sending message: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_set_npc_mood(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Set NPC mood

    Args:
        command: Dictionary with:
            - actor_name: Name of the NPC
            - mood: Mood string (neutral, happy, sad, angry, surprised, thoughtful, afraid, suspicious)
    """
    try:
        actor_name = command.get("actor_name")
        mood_str = command.get("mood", "neutral").lower()

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actor = get_actor_by_name(actor_name)
        if not actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        dialog_comp = get_dialog_component(actor)
        if not dialog_comp:
            return {"success": False, "error": "No NPCDialogComponent on actor"}

        # Map string to enum
        mood_map = {
            "neutral": unreal.NPCMood.NEUTRAL,
            "happy": unreal.NPCMood.HAPPY,
            "sad": unreal.NPCMood.SAD,
            "angry": unreal.NPCMood.ANGRY,
            "surprised": unreal.NPCMood.SURPRISED,
            "thoughtful": unreal.NPCMood.THOUGHTFUL,
            "afraid": unreal.NPCMood.AFRAID,
            "suspicious": unreal.NPCMood.SUSPICIOUS
        }

        mood = mood_map.get(mood_str, unreal.NPCMood.NEUTRAL)
        dialog_comp.set_mood(mood)

        return {
            "success": True,
            "message": f"Set mood of '{actor_name}' to {mood_str}",
            "mood": mood_str
        }

    except Exception as e:
        log.log_error(f"Error setting mood: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_modify_npc_reputation(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Modify NPC reputation

    Args:
        command: Dictionary with:
            - actor_name: Name of the NPC
            - amount: Reputation change (+/-)
    """
    try:
        actor_name = command.get("actor_name")
        amount = command.get("amount", 0)

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actor = get_actor_by_name(actor_name)
        if not actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        dialog_comp = get_dialog_component(actor)
        if not dialog_comp:
            return {"success": False, "error": "No NPCDialogComponent on actor"}

        dialog_comp.modify_reputation(int(amount))
        new_rep = dialog_comp.get_reputation()

        return {
            "success": True,
            "message": f"Modified reputation of '{actor_name}' by {amount}",
            "new_reputation": new_rep
        }

    except Exception as e:
        log.log_error(f"Error modifying reputation: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_npc_dialog_state(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Get current dialog state of an NPC

    Args:
        command: Dictionary with:
            - actor_name: Name of the NPC
    """
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actor = get_actor_by_name(actor_name)
        if not actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        dialog_comp = get_dialog_component(actor)
        if not dialog_comp:
            return {"success": False, "error": "No NPCDialogComponent on actor"}

        state = dialog_comp.get_dialog_state()
        npc_info = dialog_comp.get_npc_info()

        return {
            "success": True,
            "actor_name": actor_name,
            "is_active": dialog_comp.is_dialog_active(),
            "is_waiting": dialog_comp.is_waiting_for_response(),
            "mood": str(dialog_comp.get_current_mood()),
            "reputation": dialog_comp.get_reputation(),
            "npc_id": str(npc_info.npcid) if hasattr(npc_info, 'npcid') else "",
            "display_name": str(npc_info.display_name) if hasattr(npc_info, 'display_name') else "",
            "has_quest": npc_info.b_has_quest_available if hasattr(npc_info, 'b_has_quest_available') else False,
            "can_trade": npc_info.b_can_trade if hasattr(npc_info, 'b_can_trade') else False
        }

    except Exception as e:
        log.log_error(f"Error getting dialog state: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_add_dialog_choice(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Add a dialog choice to NPC

    Args:
        command: Dictionary with:
            - actor_name: Name of the NPC
            - choice_id: Unique ID for choice
            - display_text: Text shown to player
            - choice_type: Type (normal, trade, quest, threaten, persuade, goodbye, back)
            - is_available: Is choice available (default: True)
            - required_item_id: Required item ID (for trade)
            - required_item_quantity: Required quantity
            - required_gold: Required gold amount
    """
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actor = get_actor_by_name(actor_name)
        if not actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        dialog_comp = get_dialog_component(actor)
        if not dialog_comp:
            return {"success": False, "error": "No NPCDialogComponent on actor"}

        # Create choice struct
        choice = unreal.DialogChoice()
        choice.choice_id = command.get("choice_id", "")
        choice.display_text = unreal.Text(command.get("display_text", ""))
        choice.b_is_available = command.get("is_available", True)

        # Choice type mapping
        type_str = command.get("choice_type", "normal").lower()
        type_map = {
            "normal": unreal.DialogChoiceType.NORMAL,
            "trade": unreal.DialogChoiceType.TRADE,
            "quest": unreal.DialogChoiceType.QUEST,
            "threaten": unreal.DialogChoiceType.THREATEN,
            "persuade": unreal.DialogChoiceType.PERSUADE,
            "goodbye": unreal.DialogChoiceType.GOODBYE,
            "back": unreal.DialogChoiceType.BACK
        }
        choice.choice_type = type_map.get(type_str, unreal.DialogChoiceType.NORMAL)

        if "required_item_id" in command:
            choice.required_item_id = command["required_item_id"]
        if "required_item_quantity" in command:
            choice.required_item_quantity = int(command["required_item_quantity"])
        if "required_gold" in command:
            choice.required_gold = int(command["required_gold"])

        dialog_comp.add_choice(choice)

        return {
            "success": True,
            "message": f"Added choice to '{actor_name}'",
            "choice_id": choice.choice_id
        }

    except Exception as e:
        log.log_error(f"Error adding choice: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_clear_dialog_choices(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Clear all dialog choices from NPC

    Args:
        command: Dictionary with:
            - actor_name: Name of the NPC
    """
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actor = get_actor_by_name(actor_name)
        if not actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        dialog_comp = get_dialog_component(actor)
        if not dialog_comp:
            return {"success": False, "error": "No NPCDialogComponent on actor"}

        dialog_comp.clear_choices()

        return {"success": True, "message": f"Cleared choices from '{actor_name}'"}

    except Exception as e:
        log.log_error(f"Error clearing choices: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_is_npc_server_running(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Check if NPC AI Server is running
    """
    try:
        is_running = unreal.NPCChatLibrary.is_npc_server_running()
        return {
            "success": True,
            "is_running": is_running,
            "server_port": 9879,
            "server_host": "192.168.178.73"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_spawn_npc_on_ground(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Spawn NPC properly on ground (no floating!)

    Args:
        command: Dictionary with:
            - blueprint_path: Path to NPC Blueprint
            - location: [X, Y, Z] - Z will be adjusted
            - rotation: [Pitch, Yaw, Roll] (optional)
            - actor_label: Label for spawned actor
    """
    try:
        blueprint_path = command.get("blueprint_path")
        location = command.get("location", [0, 0, 0])
        rotation = command.get("rotation", [0, 0, 0])
        actor_label = command.get("actor_label", None)

        if not blueprint_path:
            return {"success": False, "error": "blueprint_path required"}

        log.log_command("spawn_npc_on_ground", f"Spawning {blueprint_path}")

        # Load blueprint class
        bp = unreal.EditorAssetLibrary.load_blueprint_class(blueprint_path)
        if not bp:
            return {"success": False, "error": f"Blueprint not found: {blueprint_path}"}

        # Use NPCChatLibrary's ground spawn
        loc = unreal.Vector(location[0], location[1], location[2])
        rot = unreal.Rotator(rotation[0], rotation[1], rotation[2])

        world = unreal.EditorLevelLibrary.get_editor_world()

        # Use the C++ function
        success, spawned_actor = unreal.NPCChatLibrary.spawn_actor_on_ground(world, bp, loc, rot)

        if success and spawned_actor:
            if actor_label:
                spawned_actor.set_actor_label(actor_label)

            final_loc = spawned_actor.get_actor_location()
            log.log_result("spawn_npc_on_ground", True, f"Spawned at Z={final_loc.z}")

            return {
                "success": True,
                "message": f"Spawned NPC on ground",
                "actor_label": actor_label or spawned_actor.get_actor_label(),
                "location": [final_loc.x, final_loc.y, final_loc.z]
            }
        else:
            return {"success": False, "error": "Failed to spawn actor on ground"}

    except Exception as e:
        log.log_error(f"Error spawning NPC: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_find_ground_position(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Find ground position below a location

    Args:
        command: Dictionary with:
            - location: [X, Y, Z] starting position
            - max_trace_distance: How far down to trace (default: 10000)
    """
    try:
        location = command.get("location", [0, 0, 0])
        max_dist = command.get("max_trace_distance", 10000)

        world = unreal.EditorLevelLibrary.get_editor_world()
        loc = unreal.Vector(location[0], location[1], location[2])

        success, ground_loc = unreal.NPCChatLibrary.find_ground_position(world, loc, max_dist)

        if success:
            return {
                "success": True,
                "ground_location": [ground_loc.x, ground_loc.y, ground_loc.z],
                "height_difference": location[2] - ground_loc.z
            }
        else:
            return {"success": False, "error": "No ground found"}

    except Exception as e:
        log.log_error(f"Error finding ground: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_adjust_actor_to_ground(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Adjust actor to stand properly on ground

    Args:
        command: Dictionary with:
            - actor_name: Name of the actor
            - snap_to_ground: Move to ground (default: True)
            - fix_rotation: Set pitch/roll to 0 (default: True)
    """
    try:
        actor_name = command.get("actor_name")
        snap = command.get("snap_to_ground", True)
        fix_rot = command.get("fix_rotation", True)

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actor = get_actor_by_name(actor_name)
        if not actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        success = unreal.NPCChatLibrary.adjust_actor_to_ground(actor, snap, fix_rot)

        if success:
            loc = actor.get_actor_location()
            return {
                "success": True,
                "message": f"Adjusted '{actor_name}' to ground",
                "new_location": [loc.x, loc.y, loc.z]
            }
        else:
            return {"success": False, "error": "Failed to adjust actor"}

    except Exception as e:
        log.log_error(f"Error adjusting actor: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_reset_npc_conversation(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Reset NPC conversation history

    Args:
        command: Dictionary with:
            - actor_name: Name of the NPC
    """
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actor = get_actor_by_name(actor_name)
        if not actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        dialog_comp = get_dialog_component(actor)
        if not dialog_comp:
            return {"success": False, "error": "No NPCDialogComponent on actor"}

        dialog_comp.reset_conversation()

        return {"success": True, "message": f"Reset conversation with '{actor_name}'"}

    except Exception as e:
        log.log_error(f"Error resetting conversation: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_fix_all_floating_npcs(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Fix ALL floating NPCs in the level - drop them from above onto ground

    Method: Place high up, fix rotation (upright), then drop to ground via line trace

    Args:
        command: Dictionary with optional:
            - fix_rotation: Also fix rotation (default: True)
            - drop_height: How high to place before dropping (default: 500)
            - actor_class_filter: Only fix actors of this class (default: all with mesh)
    """
    try:
        fix_rotation = command.get("fix_rotation", True)
        drop_height = command.get("drop_height", 500.0)
        class_filter = command.get("actor_class_filter", "")

        log.log_command("fix_all_floating_npcs", f"Dropping all NPCs from {drop_height} units above ground")

        world = unreal.EditorLevelLibrary.get_editor_world()
        if not world:
            return {"success": False, "error": "No editor world"}

        all_actors = unreal.EditorLevelLibrary.get_all_level_actors()

        fixed_count = 0
        skipped_count = 0
        failed_count = 0
        fixed_actors = []

        for actor in all_actors:
            actor_name = actor.get_actor_label()
            actor_class = actor.get_class().get_name()

            # Skip non-NPC actors (lights, cameras, volumes, etc.)
            skip_classes = [
                "Light", "Camera", "Volume", "Trigger", "NavMesh",
                "PlayerStart", "SkyLight", "DirectionalLight", "PointLight",
                "SpotLight", "ExponentialHeightFog", "AtmosphericFog",
                "SkyAtmosphere", "VolumetricCloud", "PostProcessVolume",
                "LevelSequenceActor", "Landscape", "StaticMeshActor"
            ]

            should_skip = False
            for skip_class in skip_classes:
                if skip_class.lower() in actor_class.lower():
                    should_skip = True
                    break

            if should_skip:
                skipped_count += 1
                continue

            # Apply class filter if specified
            if class_filter and class_filter.lower() not in actor_class.lower():
                skipped_count += 1
                continue

            # Check if actor has a mesh component (likely an NPC/character)
            has_mesh = False
            components = actor.get_components_by_class(unreal.SkeletalMeshComponent)
            if components and len(components) > 0:
                has_mesh = True
            else:
                components = actor.get_components_by_class(unreal.StaticMeshComponent)
                if components and len(components) > 0:
                    has_mesh = True

            if not has_mesh:
                skipped_count += 1
                continue

            # Try to fix this actor - DROP METHOD
            try:
                old_loc = actor.get_actor_location()
                old_z = old_loc.z
                current_rot = actor.get_actor_rotation()

                # Step 1: Move actor HIGH UP first
                high_loc = unreal.Vector(old_loc.x, old_loc.y, old_loc.z + drop_height)
                actor.set_actor_location(high_loc, False, False)

                # Step 2: Drop to ground using line trace
                success = unreal.NPCChatLibrary.adjust_actor_to_ground(actor, True, False)

                # Step 3: Fix rotation AFTER landing (so they don't roll!)
                if fix_rotation and success:
                    upright_rot = unreal.Rotator(0.0, current_rot.yaw, 0.0)
                    actor.set_actor_rotation(upright_rot, False)

                # Step 4: Disable physics so they STAY put (no more rolling pinguins!)
                if success:
                    # Try to disable physics on mesh components
                    skel_meshes = actor.get_components_by_class(unreal.SkeletalMeshComponent)
                    for mesh in skel_meshes:
                        if mesh:
                            mesh.set_simulate_physics(False)

                    static_meshes = actor.get_components_by_class(unreal.StaticMeshComponent)
                    for mesh in static_meshes:
                        if mesh:
                            mesh.set_simulate_physics(False)

                if success:
                    new_z = actor.get_actor_location().z
                    height_diff = old_z - new_z

                    fixed_count += 1
                    fixed_actors.append({
                        "name": actor_name,
                        "class": actor_class,
                        "old_z": round(old_z, 2),
                        "new_z": round(new_z, 2),
                        "dropped_from": round(old_z + drop_height, 2),
                        "adjusted_by": round(height_diff, 2)
                    })

                    log.log_info(f"Dropped '{actor_name}': Z {old_z:.1f} -> high {old_z + drop_height:.1f} -> ground {new_z:.1f}")
                else:
                    skipped_count += 1

            except Exception as actor_error:
                failed_count += 1
                log.log_warning(f"Failed to drop '{actor_name}': {str(actor_error)}")

        log.log_result("fix_all_floating_npcs", True,
                      f"Dropped {fixed_count}, skipped {skipped_count}, failed {failed_count}")

        return {
            "success": True,
            "fixed_count": fixed_count,
            "skipped_count": skipped_count,
            "failed_count": failed_count,
            "fixed_actors": fixed_actors,
            "message": f"Dropped {fixed_count} actors from {drop_height} units high onto ground"
        }

    except Exception as e:
        log.log_error(f"Error dropping NPCs: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_fix_actors_by_name(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Fix specific actors by name pattern

    Args:
        command: Dictionary with:
            - name_pattern: Pattern to match actor names (e.g., "NPC_", "Character")
            - fix_rotation: Also fix rotation (default: True)
    """
    try:
        name_pattern = command.get("name_pattern", "")
        fix_rotation = command.get("fix_rotation", True)

        if not name_pattern:
            return {"success": False, "error": "name_pattern required"}

        log.log_command("fix_actors_by_name", f"Fixing actors matching: {name_pattern}")

        all_actors = unreal.EditorLevelLibrary.get_all_level_actors()

        fixed_count = 0
        fixed_actors = []

        for actor in all_actors:
            actor_name = actor.get_actor_label()

            if name_pattern.lower() in actor_name.lower():
                try:
                    old_z = actor.get_actor_location().z

                    success = unreal.NPCChatLibrary.adjust_actor_to_ground(actor, True, fix_rotation)

                    if success:
                        new_z = actor.get_actor_location().z
                        fixed_count += 1
                        fixed_actors.append({
                            "name": actor_name,
                            "old_z": round(old_z, 2),
                            "new_z": round(new_z, 2)
                        })

                except Exception as e:
                    log.log_warning(f"Failed to fix '{actor_name}': {str(e)}")

        return {
            "success": True,
            "fixed_count": fixed_count,
            "pattern": name_pattern,
            "fixed_actors": fixed_actors,
            "message": f"Fixed {fixed_count} actors matching '{name_pattern}'"
        }

    except Exception as e:
        log.log_error(f"Error fixing actors: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


# ============================================================================
# COMMAND REGISTRY
# ============================================================================

NPC_DIALOG_COMMANDS = {
    "configure_npc_dialog": handle_configure_npc_dialog,
    "start_npc_dialog": handle_start_npc_dialog,
    "end_npc_dialog": handle_end_npc_dialog,
    "send_npc_message": handle_send_npc_message,
    "set_npc_mood": handle_set_npc_mood,
    "modify_npc_reputation": handle_modify_npc_reputation,
    "get_npc_dialog_state": handle_get_npc_dialog_state,
    "add_dialog_choice": handle_add_dialog_choice,
    "clear_dialog_choices": handle_clear_dialog_choices,
    "is_npc_server_running": handle_is_npc_server_running,
    "spawn_npc_on_ground": handle_spawn_npc_on_ground,
    "find_ground_position": handle_find_ground_position,
    "adjust_actor_to_ground": handle_adjust_actor_to_ground,
    "reset_npc_conversation": handle_reset_npc_conversation,
    # Batch ground fixing
    "fix_all_floating_npcs": handle_fix_all_floating_npcs,
    "fix_actors_by_name": handle_fix_actors_by_name,
}


def get_npc_dialog_commands():
    """Return all available NPC dialog commands"""
    return NPC_DIALOG_COMMANDS
