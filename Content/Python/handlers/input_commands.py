"""
Input Commands Handler
Handles input action and key binding related commands

Commands:
- get_input_action_key: Get the key binding for an input action from an IMC
- get_blueprint_events: Get all events in a Blueprint's event graph
- has_input_action_binding: Check if a Blueprint handles a specific input action
"""

import unreal
import json
from typing import Dict, Any, List, Optional

from utils import logging as log


def handle_get_input_action_key(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Get the key binding for an input action from an Input Mapping Context

    Args:
        command: The command dictionary containing:
            - imc_path: Path to the InputMappingContext asset
            - action_name: Name of the input action to find (e.g., "IA_Interact")

    Returns:
        Response dictionary with:
            - success: bool
            - key: The primary key bound to this action
            - modifiers: List of modifier keys (Shift, Ctrl, Alt)
            - all_mappings: All mappings for this action (if multiple)
    """
    try:
        imc_path = command.get("imc_path")
        action_name = command.get("action_name")

        if not imc_path or not action_name:
            log.log_error("Missing required parameters for get_input_action_key")
            return {"success": False, "error": "Missing imc_path or action_name"}

        log.log_command("get_input_action_key", f"IMC: {imc_path}, Action: {action_name}")

        # Load the InputMappingContext asset
        imc = unreal.load_asset(imc_path)
        if not imc:
            log.log_error(f"Failed to load InputMappingContext: {imc_path}")
            return {"success": False, "error": f"Failed to load IMC: {imc_path}"}

        # Check if it's an InputMappingContext
        if not isinstance(imc, unreal.InputMappingContext):
            log.log_error(f"Asset is not an InputMappingContext: {imc_path}")
            return {"success": False, "error": f"Asset is not an InputMappingContext"}

        # Get all mappings from the IMC (use get_editor_property, not get_mappings)
        mappings = imc.get_editor_property('mappings')

        found_mappings = []
        for mapping in mappings:
            # Get the input action
            input_action = mapping.get_editor_property("action")
            if input_action:
                ia_name = input_action.get_name()
                # Check if this is the action we're looking for
                if ia_name == action_name or action_name in ia_name:
                    # Get the key - it's a struct, get key_name property
                    key_struct = mapping.get_editor_property("key")
                    try:
                        key_name = str(key_struct.get_editor_property("key_name")) if key_struct else "Unknown"
                    except:
                        key_name = str(key_struct) if key_struct else "Unknown"

                    # Get modifiers
                    modifiers = []
                    trigger_modifiers = mapping.get_editor_property("modifiers")
                    if trigger_modifiers:
                        for mod in trigger_modifiers:
                            modifiers.append(mod.get_class().get_name())

                    # Get triggers
                    triggers = []
                    trigger_list = mapping.get_editor_property("triggers")
                    if trigger_list:
                        for trigger in trigger_list:
                            triggers.append(trigger.get_class().get_name())

                    found_mappings.append({
                        "action": ia_name,
                        "key": key_name,
                        "modifiers": modifiers,
                        "triggers": triggers
                    })

        if found_mappings:
            # Return the first mapping as primary, all mappings in array
            primary = found_mappings[0]
            log.log_result("get_input_action_key", True,
                          f"Found {len(found_mappings)} mapping(s) for {action_name}: {primary['key']}")
            return {
                "success": True,
                "key": primary["key"],
                "modifiers": primary["modifiers"],
                "triggers": primary["triggers"],
                "all_mappings": found_mappings
            }
        else:
            log.log_result("get_input_action_key", False, f"No mappings found for {action_name}")
            return {"success": False, "error": f"No mappings found for action: {action_name}"}

    except Exception as e:
        log.log_error(f"Error getting input action key: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_blueprint_events(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Get all events in a Blueprint's event graph

    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset

    Returns:
        Response dictionary with:
            - success: bool
            - events: List of event names found in the Blueprint
            - event_details: Detailed info about each event
    """
    try:
        blueprint_path = command.get("blueprint_path")

        if not blueprint_path:
            log.log_error("Missing required parameter blueprint_path")
            return {"success": False, "error": "Missing blueprint_path"}

        log.log_command("get_blueprint_events", f"Blueprint: {blueprint_path}")

        # Load the Blueprint
        blueprint = unreal.load_asset(blueprint_path)
        if not blueprint:
            log.log_error(f"Failed to load Blueprint: {blueprint_path}")
            return {"success": False, "error": f"Failed to load Blueprint: {blueprint_path}"}

        events = []
        event_details = []

        # Try to use GenBlueprintNodeCreator to get all nodes
        try:
            node_creator = unreal.GenBlueprintNodeCreator
            nodes_json = node_creator.get_all_nodes_in_graph(blueprint_path, "EventGraph")

            if nodes_json:
                nodes = json.loads(nodes_json)
                for node in nodes:
                    node_class = node.get("class", "")
                    node_title = node.get("title", "")

                    # Filter for event nodes
                    if "Event" in node_class or "K2Node_Event" in node_class or "InputAction" in node_class:
                        event_name = node_title or node_class
                        if event_name not in events:
                            events.append(event_name)
                            event_details.append({
                                "name": event_name,
                                "class": node_class,
                                "guid": node.get("guid", ""),
                                "position": node.get("position", {})
                            })
        except Exception as inner_e:
            log.log_warning(f"Could not use GenBlueprintNodeCreator: {str(inner_e)}")

        # Alternative: Check for common events by trying to get their GUIDs
        if not events:
            common_events = ["BeginPlay", "Tick", "EndPlay", "ActorBeginOverlap", "ActorEndOverlap"]
            try:
                gen_bp_utils = unreal.GenBlueprintUtils
                for event_name in common_events:
                    guid = gen_bp_utils.get_node_guid(blueprint_path, "EventGraph", event_name, "")
                    if guid:
                        events.append(event_name)
                        event_details.append({
                            "name": event_name,
                            "class": f"K2Node_Event_{event_name}",
                            "guid": guid
                        })
            except Exception as inner_e:
                log.log_warning(f"Could not check common events: {str(inner_e)}")

        log.log_result("get_blueprint_events", True, f"Found {len(events)} events in {blueprint_path}")
        return {
            "success": True,
            "events": events,
            "event_details": event_details,
            "blueprint_path": blueprint_path
        }

    except Exception as e:
        log.log_error(f"Error getting blueprint events: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_has_input_action_binding(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Check if a Blueprint handles a specific input action

    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - action_name: Name of the input action to check (e.g., "IA_Interact")

    Returns:
        Response dictionary with:
            - success: bool
            - has_binding: bool - whether the Blueprint handles this input action
            - event_types: List of event types found (Started, Completed, Triggered, etc.)
            - node_guids: GUIDs of the input action nodes found
    """
    try:
        blueprint_path = command.get("blueprint_path")
        action_name = command.get("action_name")

        if not blueprint_path or not action_name:
            log.log_error("Missing required parameters for has_input_action_binding")
            return {"success": False, "error": "Missing blueprint_path or action_name"}

        log.log_command("has_input_action_binding", f"Blueprint: {blueprint_path}, Action: {action_name}")

        # Load the Blueprint
        blueprint = unreal.load_asset(blueprint_path)
        if not blueprint:
            log.log_error(f"Failed to load Blueprint: {blueprint_path}")
            return {"success": False, "error": f"Failed to load Blueprint: {blueprint_path}"}

        has_binding = False
        event_types = []
        node_guids = []

        # Use GenBlueprintNodeCreator to get all nodes and search for InputAction events
        try:
            node_creator = unreal.GenBlueprintNodeCreator
            nodes_json = node_creator.get_all_nodes_in_graph(blueprint_path, "EventGraph")

            if nodes_json:
                nodes = json.loads(nodes_json)
                for node in nodes:
                    node_class = node.get("class", "")
                    node_title = node.get("title", "")

                    # Check if this is an InputAction node for our action
                    if "InputAction" in node_class or "EnhancedInputAction" in node_class:
                        # Check if the action name matches
                        if action_name in node_title or action_name in str(node):
                            has_binding = True
                            node_guids.append(node.get("guid", ""))

                            # Try to determine event type from title or pins
                            if "Started" in node_title:
                                if "Started" not in event_types:
                                    event_types.append("Started")
                            if "Completed" in node_title:
                                if "Completed" not in event_types:
                                    event_types.append("Completed")
                            if "Triggered" in node_title:
                                if "Triggered" not in event_types:
                                    event_types.append("Triggered")
                            if "Ongoing" in node_title:
                                if "Ongoing" not in event_types:
                                    event_types.append("Ongoing")
                            if "Canceled" in node_title:
                                if "Canceled" not in event_types:
                                    event_types.append("Canceled")

                            # If no specific type found, add generic
                            if not event_types:
                                event_types.append("Triggered")

        except Exception as inner_e:
            log.log_warning(f"Could not search nodes: {str(inner_e)}")

        result_msg = f"Blueprint {'has' if has_binding else 'does not have'} binding for {action_name}"
        log.log_result("has_input_action_binding", True, result_msg)

        return {
            "success": True,
            "has_binding": has_binding,
            "event_types": event_types,
            "node_guids": node_guids,
            "action_name": action_name,
            "blueprint_path": blueprint_path
        }

    except Exception as e:
        log.log_error(f"Error checking input action binding: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_list_all_input_actions(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    List all input actions in an Input Mapping Context

    Args:
        command: The command dictionary containing:
            - imc_path: Path to the InputMappingContext asset

    Returns:
        Response dictionary with list of all input actions and their bindings
    """
    try:
        imc_path = command.get("imc_path")

        if not imc_path:
            log.log_error("Missing required parameter imc_path")
            return {"success": False, "error": "Missing imc_path"}

        log.log_command("list_all_input_actions", f"IMC: {imc_path}")

        # Load the InputMappingContext asset
        imc = unreal.load_asset(imc_path)
        if not imc:
            log.log_error(f"Failed to load InputMappingContext: {imc_path}")
            return {"success": False, "error": f"Failed to load IMC: {imc_path}"}

        if not isinstance(imc, unreal.InputMappingContext):
            log.log_error(f"Asset is not an InputMappingContext: {imc_path}")
            return {"success": False, "error": f"Asset is not an InputMappingContext"}

        # Get all mappings (use get_editor_property, not get_mappings)
        mappings = imc.get_editor_property('mappings')

        actions = {}
        for mapping in mappings:
            input_action = mapping.get_editor_property("action")
            if input_action:
                ia_name = input_action.get_name()
                key_struct = mapping.get_editor_property("key")
                try:
                    key_name = str(key_struct.get_editor_property("key_name")) if key_struct else "Unknown"
                except:
                    key_name = str(key_struct) if key_struct else "Unknown"

                if ia_name not in actions:
                    actions[ia_name] = {
                        "name": ia_name,
                        "keys": [],
                        "path": input_action.get_path_name()
                    }

                actions[ia_name]["keys"].append(key_name)

        action_list = list(actions.values())
        log.log_result("list_all_input_actions", True, f"Found {len(action_list)} input actions in {imc_path}")

        return {
            "success": True,
            "actions": action_list,
            "total_count": len(action_list),
            "imc_path": imc_path
        }

    except Exception as e:
        log.log_error(f"Error listing input actions: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}
