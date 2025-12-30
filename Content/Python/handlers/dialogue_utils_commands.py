"""
Dialogue Utils Commands Handler
Wrapper for GenDialogueUtils C++ functions
NPC Dialog Component configuration via GenUtils
"""

import unreal
import json
from typing import Dict, Any
from utils import logging as log


def _parse_json_response(json_str: str) -> Dict[str, Any]:
    """Parse JSON string response from C++ GenUtils"""
    try:
        return json.loads(json_str)
    except json.JSONDecodeError as e:
        return {"success": False, "error": f"JSON parse error: {str(e)}"}


# ============================================
# COMPONENT MANAGEMENT
# ============================================

def handle_list_dialog_actors(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Find all actors with NPCDialogComponent in the level

    Returns:
        JSON array of actors with dialog components
    """
    try:
        log.log_command("list_dialog_actors", "Finding all dialog actors")
        result = unreal.GenDialogueUtils.list_dialog_actors()
        return _parse_json_response(result)

    except Exception as e:
        log.log_error(f"Error listing dialog actors: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_dialog_config(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get dialog component configuration for an actor"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.get_dialog_config(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_set_dialog_config(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set dialog component configuration on an actor"""
    try:
        actor_name = command.get("actor_name")
        npc_id = command.get("npc_id", "")
        display_name = command.get("display_name", "")
        title = command.get("title", "")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.set_dialog_config(
            actor_name, npc_id, display_name, title
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_dialog_component(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add NPCDialogComponent to an actor"""
    try:
        actor_name = command.get("actor_name")
        npc_id = command.get("npc_id", "")
        display_name = command.get("display_name", "")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        log.log_command("add_dialog_component", f"Adding to {actor_name}")
        result = unreal.GenDialogueUtils.add_dialog_component(
            actor_name, npc_id, display_name
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_remove_dialog_component(command: Dict[str, Any]) -> Dict[str, Any]:
    """Remove NPCDialogComponent from an actor"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.remove_dialog_component(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# GREETING & FAREWELL
# ============================================

def handle_set_greeting(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set greeting message for NPC"""
    try:
        actor_name = command.get("actor_name")
        greeting_text = command.get("greeting_text", "")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.set_greeting(actor_name, greeting_text)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_set_farewell(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set farewell message for NPC"""
    try:
        actor_name = command.get("actor_name")
        farewell_text = command.get("farewell_text", "")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.set_farewell(actor_name, farewell_text)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# QUICK OPTIONS
# ============================================

def handle_get_quick_options(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get quick dialog options for an NPC"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.get_quick_options(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_quick_option(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add a quick dialog option to an NPC"""
    try:
        actor_name = command.get("actor_name")
        display_text = command.get("display_text", "")
        message = command.get("message", "")
        hotkey = command.get("hotkey", 1)

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.add_quick_option(
            actor_name, display_text, message, hotkey
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_clear_quick_options(command: Dict[str, Any]) -> Dict[str, Any]:
    """Clear all quick options for an NPC"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.clear_quick_options(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# TRADE & SHOP
# ============================================

def handle_set_trade_config(command: Dict[str, Any]) -> Dict[str, Any]:
    """Configure trade settings for NPC"""
    try:
        actor_name = command.get("actor_name")
        can_trade = command.get("can_trade", False)
        shop_id = command.get("shop_id", "")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.set_trade_config(actor_name, can_trade, shop_id)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# MOOD & REPUTATION
# ============================================

def handle_set_npc_mood_genutils(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set NPC mood via GenDialogueUtils"""
    try:
        actor_name = command.get("actor_name")
        mood_name = command.get("mood_name", "Neutral")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.set_mood(actor_name, mood_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_set_npc_reputation(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set NPC reputation"""
    try:
        actor_name = command.get("actor_name")
        reputation = command.get("reputation", 0)

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.set_reputation(actor_name, int(reputation))
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# PORTRAIT & VISUAL
# ============================================

def handle_set_npc_portrait(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set portrait image path"""
    try:
        actor_name = command.get("actor_name")
        portrait_path = command.get("portrait_path", "")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.set_portrait(actor_name, portrait_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_set_npc_faction(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set faction ID"""
    try:
        actor_name = command.get("actor_name")
        faction_id = command.get("faction_id", "")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.set_faction(actor_name, faction_id)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# RUNTIME CONTROL (PIE)
# ============================================

def handle_start_dialog_genutils(command: Dict[str, Any]) -> Dict[str, Any]:
    """Start dialog with NPC (in PIE mode)"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.start_dialog(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_end_dialog_genutils(command: Dict[str, Any]) -> Dict[str, Any]:
    """End dialog with NPC (in PIE mode)"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.end_dialog(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_send_message_genutils(command: Dict[str, Any]) -> Dict[str, Any]:
    """Send message to NPC (in PIE mode)"""
    try:
        actor_name = command.get("actor_name")
        message = command.get("message", "")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenDialogueUtils.send_message(actor_name, message)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# EXPORT/IMPORT
# ============================================

def handle_export_all_dialog_configs(command: Dict[str, Any]) -> Dict[str, Any]:
    """Export all dialog configurations to JSON"""
    try:
        result = unreal.GenDialogueUtils.export_all_dialog_configs()
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_list_moods(command: Dict[str, Any]) -> Dict[str, Any]:
    """List available moods"""
    try:
        result = unreal.GenDialogueUtils.list_moods()
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_list_choice_types(command: Dict[str, Any]) -> Dict[str, Any]:
    """List choice types"""
    try:
        result = unreal.GenDialogueUtils.list_choice_types()
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# COMMAND REGISTRY
# ============================================

DIALOGUE_UTILS_COMMANDS = {
    # Component Management
    "gu_list_dialog_actors": handle_list_dialog_actors,
    "gu_get_dialog_config": handle_get_dialog_config,
    "gu_set_dialog_config": handle_set_dialog_config,
    "gu_add_dialog_component": handle_add_dialog_component,
    "gu_remove_dialog_component": handle_remove_dialog_component,

    # Greeting & Farewell
    "gu_set_greeting": handle_set_greeting,
    "gu_set_farewell": handle_set_farewell,

    # Quick Options
    "gu_get_quick_options": handle_get_quick_options,
    "gu_add_quick_option": handle_add_quick_option,
    "gu_clear_quick_options": handle_clear_quick_options,

    # Trade
    "gu_set_trade_config": handle_set_trade_config,

    # Mood & Reputation
    "gu_set_mood": handle_set_npc_mood_genutils,
    "gu_set_reputation": handle_set_npc_reputation,

    # Portrait & Visual
    "gu_set_portrait": handle_set_npc_portrait,
    "gu_set_faction": handle_set_npc_faction,

    # Runtime (PIE)
    "gu_start_dialog": handle_start_dialog_genutils,
    "gu_end_dialog": handle_end_dialog_genutils,
    "gu_send_message": handle_send_message_genutils,

    # Export/Import
    "gu_export_all_dialog_configs": handle_export_all_dialog_configs,
    "gu_list_moods": handle_list_moods,
    "gu_list_choice_types": handle_list_choice_types,
}


def get_dialogue_utils_commands():
    """Return all available dialogue utils commands"""
    return DIALOGUE_UTILS_COMMANDS
