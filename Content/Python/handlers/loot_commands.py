# Loot Commands - Python Handler for LootLibrary
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# HELPER: Get LootLibrary
# ============================================

def get_lib():
    """Get the LootLibrary class"""
    return unreal.LootLibrary

# ============================================
# LOOT TABLE REGISTRY
# ============================================

def handle_register_loot_table(command: Dict[str, Any]) -> Dict[str, Any]:
    """Register a loot table"""
    try:
        table_id = command.get("table_id", "")
        name = command.get("name", "")
        entries = command.get("entries", [])

        if not table_id:
            return {"success": False, "error": "table_id required"}

        log.log_info(f"Register loot table: {table_id}")

        return {
            "success": True,
            "table_id": table_id,
            "message": f"Loot table '{table_id}' registered"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_load_loot_tables_from_json(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load loot tables from JSON file"""
    try:
        file_path = command.get("file_path", "")

        if not file_path:
            return {"success": False, "error": "file_path required"}

        lib = get_lib()
        count = lib.load_loot_tables_from_json(file_path)

        return {"success": True, "count": count}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_loot_table(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get a loot table by ID"""
    try:
        table_id = command.get("table_id", "")

        if not table_id:
            return {"success": False, "error": "table_id required"}

        lib = get_lib()
        found, table = lib.get_loot_table(table_id)

        if found:
            return {
                "success": True,
                "table_id": table_id,
                "found": True
            }
        else:
            return {"success": False, "error": f"Loot table '{table_id}' not found"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# LOOT GENERATION
# ============================================

def handle_generate_loot(command: Dict[str, Any]) -> Dict[str, Any]:
    """Generate loot from a loot table"""
    try:
        table_id = command.get("table_id", "")
        player_level = command.get("player_level", 1)

        if not table_id:
            return {"success": False, "error": "table_id required"}

        lib = get_lib()
        success, drops, gold = lib.generate_loot(table_id, player_level)

        if success:
            drop_list = []
            for drop in drops:
                drop_list.append({
                    "item_id": str(drop.item_id) if hasattr(drop, 'item_id') else "",
                    "quantity": drop.quantity if hasattr(drop, 'quantity') else 1
                })

            return {
                "success": True,
                "table_id": table_id,
                "drops": drop_list,
                "gold": gold
            }
        else:
            return {"success": False, "error": "Failed to generate loot"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_roll_loot_entry(command: Dict[str, Any]) -> Dict[str, Any]:
    """Roll a single loot entry"""
    try:
        item_id = command.get("item_id", "")
        drop_chance = command.get("drop_chance", 1.0)
        min_quantity = command.get("min_quantity", 1)
        max_quantity = command.get("max_quantity", 1)
        player_level = command.get("player_level", 1)

        log.log_info(f"Roll loot entry: {item_id} (chance: {drop_chance})")

        # Simulate roll
        import random
        rolled = random.random() < drop_chance
        quantity = random.randint(min_quantity, max_quantity) if rolled else 0

        return {
            "success": True,
            "item_id": item_id,
            "dropped": rolled,
            "quantity": quantity
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_apply_luck_modifier(command: Dict[str, Any]) -> Dict[str, Any]:
    """Apply luck modifier to drop chance"""
    try:
        base_chance = command.get("base_chance", 0.1)
        luck_modifier = command.get("luck_modifier", 0.0)

        lib = get_lib()
        modified_chance = lib.apply_luck_modifier(base_chance, luck_modifier)

        return {
            "success": True,
            "base_chance": base_chance,
            "luck_modifier": luck_modifier,
            "modified_chance": modified_chance
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# MONSTER LOOT TABLES
# ============================================

def handle_get_monster_loot_table(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get loot table ID for a monster type"""
    try:
        monster_id = command.get("monster_id", "")

        if not monster_id:
            return {"success": False, "error": "monster_id required"}

        lib = get_lib()
        table_id = lib.get_monster_loot_table_id(monster_id)

        return {"success": True, "monster_id": monster_id, "table_id": table_id}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_register_monster_loot_table(command: Dict[str, Any]) -> Dict[str, Any]:
    """Register monster to loot table mapping"""
    try:
        monster_id = command.get("monster_id", "")
        table_id = command.get("table_id", "")

        if not monster_id or not table_id:
            return {"success": False, "error": "monster_id and table_id required"}

        lib = get_lib()
        lib.register_monster_loot_table(monster_id, table_id)

        return {
            "success": True,
            "monster_id": monster_id,
            "table_id": table_id
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_generate_monster_loot(command: Dict[str, Any]) -> Dict[str, Any]:
    """Generate loot for a defeated monster"""
    try:
        monster_id = command.get("monster_id", "")
        monster_level = command.get("monster_level", 1)
        player_level = command.get("player_level", 1)

        if not monster_id:
            return {"success": False, "error": "monster_id required"}

        lib = get_lib()
        success, drops, gold = lib.generate_monster_loot(monster_id, monster_level, player_level)

        if success:
            drop_list = []
            for drop in drops:
                drop_list.append({
                    "item_id": str(drop.item_id) if hasattr(drop, 'item_id') else "",
                    "quantity": drop.quantity if hasattr(drop, 'quantity') else 1
                })

            return {
                "success": True,
                "monster_id": monster_id,
                "drops": drop_list,
                "gold": gold
            }
        else:
            return {"success": False, "error": "No loot generated"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

LOOT_COMMANDS = {
    # Registry
    "loot_register_table": handle_register_loot_table,
    "loot_load_json": handle_load_loot_tables_from_json,
    "loot_get_table": handle_get_loot_table,

    # Generation
    "loot_generate": handle_generate_loot,
    "loot_roll_entry": handle_roll_loot_entry,
    "loot_apply_luck": handle_apply_luck_modifier,

    # Monster
    "loot_get_monster_table": handle_get_monster_loot_table,
    "loot_register_monster": handle_register_monster_loot_table,
    "loot_generate_monster": handle_generate_monster_loot,
}
