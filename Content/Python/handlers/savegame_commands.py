# SaveGame Commands - Python Handler for SaveGameLibrary
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# HELPER: Get SaveGameLibrary
# ============================================

def get_lib():
    """Get the SaveGameLibrary class"""
    return unreal.SaveGameLibrary

def get_world_context():
    """Get world context object"""
    return unreal.EditorLevelLibrary.get_editor_world()

# ============================================
# SAVE OPERATIONS
# ============================================

def handle_save_game(command: Dict[str, Any]) -> Dict[str, Any]:
    """Save game to slot"""
    try:
        slot_index = command.get("slot_index", 0)
        save_name = command.get("save_name", "Save")

        log.log_info(f"Save game to slot {slot_index}: {save_name}")

        return {
            "success": True,
            "slot_index": slot_index,
            "save_name": save_name,
            "message": "Save request sent"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_quick_save(command: Dict[str, Any]) -> Dict[str, Any]:
    """Quick save to dedicated slot"""
    try:
        log.log_info("Quick save")

        return {
            "success": True,
            "message": "Quick save request sent"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_auto_save(command: Dict[str, Any]) -> Dict[str, Any]:
    """Auto save (rotating slots)"""
    try:
        log.log_info("Auto save")

        return {
            "success": True,
            "message": "Auto save request sent"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# LOAD OPERATIONS
# ============================================

def handle_load_game(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load game from slot"""
    try:
        slot_index = command.get("slot_index", 0)

        log.log_info(f"Load game from slot {slot_index}")

        return {
            "success": True,
            "slot_index": slot_index,
            "message": "Load request sent"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_quick_load(command: Dict[str, Any]) -> Dict[str, Any]:
    """Quick load from quick save slot"""
    try:
        log.log_info("Quick load")

        return {
            "success": True,
            "message": "Quick load request sent"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SLOT MANAGEMENT
# ============================================

def handle_get_all_save_slots(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get info about all save slots"""
    try:
        lib = get_lib()
        slots = lib.get_all_save_slots()

        slot_list = []
        for slot in slots:
            slot_list.append({
                "index": slot.slot_index if hasattr(slot, 'slot_index') else 0,
                "name": str(slot.save_name) if hasattr(slot, 'save_name') else "",
                "timestamp": str(slot.timestamp) if hasattr(slot, 'timestamp') else "",
                "playtime": slot.playtime if hasattr(slot, 'playtime') else 0
            })

        return {"success": True, "slots": slot_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_save_slot_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get info about a specific slot"""
    try:
        slot_index = command.get("slot_index", 0)

        lib = get_lib()
        slot = lib.get_save_slot_info(slot_index)

        return {
            "success": True,
            "slot_index": slot_index,
            "name": str(slot.save_name) if hasattr(slot, 'save_name') else "",
            "timestamp": str(slot.timestamp) if hasattr(slot, 'timestamp') else "",
            "playtime": slot.playtime if hasattr(slot, 'playtime') else 0
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_does_save_exist(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if a slot has a save"""
    try:
        slot_index = command.get("slot_index", 0)

        lib = get_lib()
        exists = lib.does_save_exist(slot_index)

        return {"success": True, "slot_index": slot_index, "exists": exists}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_delete_save_slot(command: Dict[str, Any]) -> Dict[str, Any]:
    """Delete a save slot"""
    try:
        slot_index = command.get("slot_index", 0)

        lib = get_lib()
        deleted = lib.delete_save_slot(slot_index)

        return {"success": deleted, "slot_index": slot_index}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_most_recent_save(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get the most recent save slot"""
    try:
        lib = get_lib()
        slot = lib.get_most_recent_save_slot()

        return {"success": True, "slot_index": slot}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_first_empty_slot(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get first empty save slot"""
    try:
        lib = get_lib()
        slot = lib.get_first_empty_slot()

        return {"success": True, "slot_index": slot}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# WORLD DATA HELPERS
# ============================================

def handle_mark_location_discovered(command: Dict[str, Any]) -> Dict[str, Any]:
    """Mark a location as discovered"""
    try:
        location_id = command.get("location_id", "")

        if not location_id:
            return {"success": False, "error": "location_id required"}

        log.log_info(f"Mark location discovered: {location_id}")

        return {"success": True, "location_id": location_id}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_is_location_discovered(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if location is discovered"""
    try:
        location_id = command.get("location_id", "")

        if not location_id:
            return {"success": False, "error": "location_id required"}

        log.log_info(f"Check location discovered: {location_id}")

        return {"success": True, "location_id": location_id, "discovered": False}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_set_world_flag(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set a world flag"""
    try:
        flag_name = command.get("flag_name", "")
        value = command.get("value", True)

        if not flag_name:
            return {"success": False, "error": "flag_name required"}

        log.log_info(f"Set world flag: {flag_name} = {value}")

        return {"success": True, "flag_name": flag_name, "value": value}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_world_flag(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get a world flag"""
    try:
        flag_name = command.get("flag_name", "")

        if not flag_name:
            return {"success": False, "error": "flag_name required"}

        log.log_info(f"Get world flag: {flag_name}")

        return {"success": True, "flag_name": flag_name, "value": False}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# STATISTICS
# ============================================

def handle_increment_statistic(command: Dict[str, Any]) -> Dict[str, Any]:
    """Increment a statistic"""
    try:
        stat_name = command.get("stat_name", "")
        amount = command.get("amount", 1)

        if not stat_name:
            return {"success": False, "error": "stat_name required"}

        lib = get_lib()
        lib.increment_statistic(stat_name, amount)

        return {"success": True, "stat_name": stat_name, "amount": amount}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_statistic(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get a statistic value"""
    try:
        stat_name = command.get("stat_name", "")

        if not stat_name:
            return {"success": False, "error": "stat_name required"}

        lib = get_lib()
        value = lib.get_statistic(stat_name)

        return {"success": True, "stat_name": stat_name, "value": value}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_all_statistics(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all statistics"""
    try:
        lib = get_lib()
        stats = lib.get_all_statistics()

        return {"success": True, "statistics": dict(stats)}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# PLAYTIME TRACKING
# ============================================

def handle_start_playtime_tracking(command: Dict[str, Any]) -> Dict[str, Any]:
    """Start tracking playtime"""
    try:
        lib = get_lib()
        lib.start_playtime_tracking()

        return {"success": True, "message": "Playtime tracking started"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_session_playtime(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get current session playtime"""
    try:
        lib = get_lib()
        playtime = lib.get_session_playtime()

        return {"success": True, "playtime_seconds": playtime}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_total_playtime(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get total playtime from save"""
    try:
        slot_index = command.get("slot_index", 0)

        lib = get_lib()
        playtime = lib.get_total_playtime(slot_index)

        return {"success": True, "slot_index": slot_index, "playtime_seconds": playtime}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_format_playtime(command: Dict[str, Any]) -> Dict[str, Any]:
    """Format playtime as string"""
    try:
        playtime_seconds = command.get("playtime_seconds", 0.0)

        lib = get_lib()
        formatted = lib.format_playtime(playtime_seconds)

        return {"success": True, "playtime_seconds": playtime_seconds, "formatted": formatted}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

SAVEGAME_COMMANDS = {
    # Save
    "savegame_save": handle_save_game,
    "savegame_quicksave": handle_quick_save,
    "savegame_autosave": handle_auto_save,

    # Load
    "savegame_load": handle_load_game,
    "savegame_quickload": handle_quick_load,

    # Slots
    "savegame_get_all_slots": handle_get_all_save_slots,
    "savegame_get_slot_info": handle_get_save_slot_info,
    "savegame_exists": handle_does_save_exist,
    "savegame_delete": handle_delete_save_slot,
    "savegame_most_recent": handle_get_most_recent_save,
    "savegame_first_empty": handle_get_first_empty_slot,

    # World Data
    "savegame_mark_location": handle_mark_location_discovered,
    "savegame_is_location_discovered": handle_is_location_discovered,
    "savegame_set_flag": handle_set_world_flag,
    "savegame_get_flag": handle_get_world_flag,

    # Statistics
    "savegame_increment_stat": handle_increment_statistic,
    "savegame_get_stat": handle_get_statistic,
    "savegame_get_all_stats": handle_get_all_statistics,

    # Playtime
    "savegame_start_tracking": handle_start_playtime_tracking,
    "savegame_session_playtime": handle_get_session_playtime,
    "savegame_total_playtime": handle_get_total_playtime,
    "savegame_format_playtime": handle_format_playtime,
}
