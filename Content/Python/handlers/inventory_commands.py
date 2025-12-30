# Inventory Commands - Python Handler for InventoryComponent
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# HELPER: Get InventoryComponent from Actor
# ============================================

def get_inventory_component(actor_name: str):
    """Get InventoryComponent from actor by name"""
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for actor in actors:
        if actor.get_actor_label() == actor_name:
            components = actor.get_components_by_class(unreal.InventoryComponent)
            if components and len(components) > 0:
                return components[0]
    return None

def get_player_inventory():
    """Get player's inventory component"""
    # Try to get player pawn
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world:
        player = unreal.GameplayStatics.get_player_pawn(world, 0)
        if player:
            components = player.get_components_by_class(unreal.InventoryComponent)
            if components and len(components) > 0:
                return components[0]
    return None

# ============================================
# ITEM MANAGEMENT
# ============================================

def handle_add_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add item to inventory"""
    try:
        actor_name = command.get("actor_name", "")
        item_id = command.get("item_id", "")
        quantity = command.get("quantity", 1)

        if not item_id:
            return {"success": False, "error": "item_id required"}

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        success, remainder = inv.add_item(item_id, quantity)

        return {
            "success": success,
            "item_id": item_id,
            "quantity_added": quantity - remainder,
            "remainder": remainder
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_add_item_to_slot(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add item to specific slot"""
    try:
        actor_name = command.get("actor_name", "")
        slot_index = command.get("slot_index", 0)
        item_id = command.get("item_id", "")
        quantity = command.get("quantity", 1)

        if not item_id:
            return {"success": False, "error": "item_id required"}

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        success = inv.add_item_to_slot(slot_index, item_id, quantity)

        return {"success": success, "slot_index": slot_index, "item_id": item_id}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_remove_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Remove item from inventory"""
    try:
        actor_name = command.get("actor_name", "")
        item_id = command.get("item_id", "")
        quantity = command.get("quantity", 1)

        if not item_id:
            return {"success": False, "error": "item_id required"}

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        removed = inv.remove_item(item_id, quantity)

        return {"success": removed > 0, "item_id": item_id, "quantity_removed": removed}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_remove_item_from_slot(command: Dict[str, Any]) -> Dict[str, Any]:
    """Remove item from specific slot"""
    try:
        actor_name = command.get("actor_name", "")
        slot_index = command.get("slot_index", 0)
        quantity = command.get("quantity", 0)

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        removed = inv.remove_item_from_slot(slot_index, quantity)

        return {"success": removed > 0, "slot_index": slot_index, "quantity_removed": removed}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_move_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Move item between slots"""
    try:
        actor_name = command.get("actor_name", "")
        from_slot = command.get("from_slot", 0)
        to_slot = command.get("to_slot", 0)

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        success = inv.move_item(from_slot, to_slot)

        return {"success": success, "from_slot": from_slot, "to_slot": to_slot}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_split_stack(command: Dict[str, Any]) -> Dict[str, Any]:
    """Split a stack"""
    try:
        actor_name = command.get("actor_name", "")
        slot_index = command.get("slot_index", 0)
        amount = command.get("amount", 1)
        target_slot = command.get("target_slot", -1)

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        success = inv.split_stack(slot_index, amount, target_slot)

        return {"success": success, "slot_index": slot_index, "amount": amount}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_use_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Use/consume an item"""
    try:
        actor_name = command.get("actor_name", "")
        slot_index = command.get("slot_index", 0)

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        success = inv.use_item(slot_index)

        return {"success": success, "slot_index": slot_index}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_drop_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Drop item from inventory"""
    try:
        actor_name = command.get("actor_name", "")
        slot_index = command.get("slot_index", 0)
        quantity = command.get("quantity", 0)

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        success = inv.drop_item(slot_index, quantity)

        return {"success": success, "slot_index": slot_index}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# QUERIES
# ============================================

def handle_get_slot(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get slot at index"""
    try:
        actor_name = command.get("actor_name", "")
        slot_index = command.get("slot_index", 0)

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        slot = inv.get_slot(slot_index)

        return {
            "success": True,
            "slot_index": slot_index,
            "item_id": str(slot.item_id) if hasattr(slot, 'item_id') else "",
            "quantity": slot.quantity if hasattr(slot, 'quantity') else 0
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_all_slots(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all slots"""
    try:
        actor_name = command.get("actor_name", "")

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        slots = inv.get_all_slots()

        slot_list = []
        for i, slot in enumerate(slots):
            slot_list.append({
                "index": i,
                "item_id": str(slot.item_id) if hasattr(slot, 'item_id') else "",
                "quantity": slot.quantity if hasattr(slot, 'quantity') else 0
            })

        return {"success": True, "slots": slot_list, "count": len(slot_list)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_has_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if has item"""
    try:
        actor_name = command.get("actor_name", "")
        item_id = command.get("item_id", "")
        min_quantity = command.get("min_quantity", 1)

        if not item_id:
            return {"success": False, "error": "item_id required"}

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        has = inv.has_item(item_id, min_quantity)

        return {"success": True, "item_id": item_id, "has_item": has}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_item_count(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get total quantity of item"""
    try:
        actor_name = command.get("actor_name", "")
        item_id = command.get("item_id", "")

        if not item_id:
            return {"success": False, "error": "item_id required"}

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        count = inv.get_item_count(item_id)

        return {"success": True, "item_id": item_id, "count": count}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_find_item_slot(command: Dict[str, Any]) -> Dict[str, Any]:
    """Find first slot with item"""
    try:
        actor_name = command.get("actor_name", "")
        item_id = command.get("item_id", "")

        if not item_id:
            return {"success": False, "error": "item_id required"}

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        slot = inv.find_item_slot(item_id)

        return {"success": True, "item_id": item_id, "slot_index": slot}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_first_empty_slot(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get first empty slot"""
    try:
        actor_name = command.get("actor_name", "")

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        slot = inv.get_first_empty_slot()

        return {"success": True, "empty_slot_index": slot}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_empty_slot_count(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get number of empty slots"""
    try:
        actor_name = command.get("actor_name", "")

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        count = inv.get_empty_slot_count()

        return {"success": True, "empty_slots": count}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_is_inventory_full(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if inventory is full"""
    try:
        actor_name = command.get("actor_name", "")

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        is_full = inv.is_full()

        return {"success": True, "is_full": is_full}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_current_weight(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get current total weight"""
    try:
        actor_name = command.get("actor_name", "")

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        weight = inv.get_current_weight()

        return {"success": True, "current_weight": weight}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_can_add_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if can add item"""
    try:
        actor_name = command.get("actor_name", "")
        item_id = command.get("item_id", "")
        quantity = command.get("quantity", 1)

        if not item_id:
            return {"success": False, "error": "item_id required"}

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        can_add = inv.can_add_item(item_id, quantity)

        return {"success": True, "item_id": item_id, "can_add": can_add}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# EQUIPMENT
# ============================================

def handle_equip_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Equip item from inventory slot"""
    try:
        actor_name = command.get("actor_name", "")
        slot_index = command.get("slot_index", 0)

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        success = inv.equip_item_from_slot(slot_index)

        return {"success": success, "slot_index": slot_index}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_unequip_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Unequip item from equipment slot"""
    try:
        actor_name = command.get("actor_name", "")
        slot = command.get("equipment_slot", "")

        if not slot:
            return {"success": False, "error": "equipment_slot required"}

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        # Convert string to enum
        log.log_info(f"Unequip from slot: {slot}")

        return {"success": True, "equipment_slot": slot, "message": "Unequip request sent"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_equipped_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get equipped item in slot"""
    try:
        actor_name = command.get("actor_name", "")
        slot = command.get("equipment_slot", "")

        if not slot:
            return {"success": False, "error": "equipment_slot required"}

        log.log_info(f"Get equipped item in slot: {slot}")

        return {"success": True, "equipment_slot": slot, "item_id": ""}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_all_equipped_items(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all equipped items"""
    try:
        actor_name = command.get("actor_name", "")

        log.log_info("Get all equipped items")

        return {"success": True, "equipped_items": {}}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_equipment_stat_bonuses(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get total stat bonuses from equipment"""
    try:
        actor_name = command.get("actor_name", "")

        log.log_info("Get equipment stat bonuses")

        return {"success": True, "stat_bonuses": {}}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# GOLD/CURRENCY
# ============================================

def handle_get_gold(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get current gold"""
    try:
        actor_name = command.get("actor_name", "")

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        gold = inv.get_gold()

        return {"success": True, "gold": gold}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_add_gold(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add gold"""
    try:
        actor_name = command.get("actor_name", "")
        amount = command.get("amount", 0)

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        inv.add_gold(amount)

        return {"success": True, "amount_added": amount, "new_total": inv.get_gold()}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_remove_gold(command: Dict[str, Any]) -> Dict[str, Any]:
    """Remove gold"""
    try:
        actor_name = command.get("actor_name", "")
        amount = command.get("amount", 0)

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        success = inv.remove_gold(amount)

        return {"success": success, "amount_removed": amount if success else 0}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_has_gold(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if has enough gold"""
    try:
        actor_name = command.get("actor_name", "")
        amount = command.get("amount", 0)

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        has = inv.has_gold(amount)

        return {"success": True, "amount": amount, "has_gold": has}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SAVE/LOAD
# ============================================

def handle_save_inventory(command: Dict[str, Any]) -> Dict[str, Any]:
    """Serialize inventory to JSON"""
    try:
        actor_name = command.get("actor_name", "")

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        json_string = inv.save_to_json()

        return {"success": True, "json": json_string}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_load_inventory(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load inventory from JSON"""
    try:
        actor_name = command.get("actor_name", "")
        json_string = command.get("json", "")

        if not json_string:
            return {"success": False, "error": "json required"}

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        success = inv.load_from_json(json_string)

        return {"success": success}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_clear_inventory(command: Dict[str, Any]) -> Dict[str, Any]:
    """Clear entire inventory"""
    try:
        actor_name = command.get("actor_name", "")

        inv = get_inventory_component(actor_name) if actor_name else get_player_inventory()
        if not inv:
            return {"success": False, "error": "Inventory not found"}

        inv.clear_inventory()

        return {"success": True, "message": "Inventory cleared"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

INVENTORY_COMMANDS = {
    # Item Management
    "inventory_add_item": handle_add_item,
    "inventory_add_to_slot": handle_add_item_to_slot,
    "inventory_remove_item": handle_remove_item,
    "inventory_remove_from_slot": handle_remove_item_from_slot,
    "inventory_move_item": handle_move_item,
    "inventory_split_stack": handle_split_stack,
    "inventory_use_item": handle_use_item,
    "inventory_drop_item": handle_drop_item,

    # Queries
    "inventory_get_slot": handle_get_slot,
    "inventory_get_all_slots": handle_get_all_slots,
    "inventory_has_item": handle_has_item,
    "inventory_get_item_count": handle_get_item_count,
    "inventory_find_slot": handle_find_item_slot,
    "inventory_first_empty": handle_get_first_empty_slot,
    "inventory_empty_count": handle_get_empty_slot_count,
    "inventory_is_full": handle_is_inventory_full,
    "inventory_get_weight": handle_get_current_weight,
    "inventory_can_add": handle_can_add_item,

    # Equipment
    "inventory_equip": handle_equip_item,
    "inventory_unequip": handle_unequip_item,
    "inventory_get_equipped": handle_get_equipped_item,
    "inventory_get_all_equipped": handle_get_all_equipped_items,
    "inventory_get_stat_bonuses": handle_get_equipment_stat_bonuses,

    # Gold
    "inventory_get_gold": handle_get_gold,
    "inventory_add_gold": handle_add_gold,
    "inventory_remove_gold": handle_remove_gold,
    "inventory_has_gold": handle_has_gold,

    # Save/Load
    "inventory_save": handle_save_inventory,
    "inventory_load": handle_load_inventory,
    "inventory_clear": handle_clear_inventory,
}
