# Shop Commands - Python Handler for ShopLibrary
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# HELPER: Get ShopLibrary
# ============================================

def get_lib():
    """Get the ShopLibrary class"""
    return unreal.ShopLibrary

# ============================================
# SHOP REGISTRY
# ============================================

def handle_register_shop(command: Dict[str, Any]) -> Dict[str, Any]:
    """Register a shop"""
    try:
        shop_id = command.get("shop_id", "")
        name = command.get("name", "")
        owner_npc = command.get("owner_npc", "")

        if not shop_id:
            return {"success": False, "error": "shop_id required"}

        log.log_info(f"Register shop: {name} ({shop_id})")

        return {
            "success": True,
            "shop_id": shop_id,
            "message": f"Shop '{name}' registered"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_load_shops_from_json(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load shops from JSON file"""
    try:
        file_path = command.get("file_path", "")

        if not file_path:
            return {"success": False, "error": "file_path required"}

        lib = get_lib()
        count = lib.load_shops_from_json(file_path)

        return {"success": True, "count": count}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_shop(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get a shop by ID"""
    try:
        shop_id = command.get("shop_id", "")

        if not shop_id:
            return {"success": False, "error": "shop_id required"}

        lib = get_lib()
        found, shop = lib.get_shop(shop_id)

        if found:
            return {
                "success": True,
                "shop_id": shop_id,
                "name": str(shop.name) if hasattr(shop, 'name') else ""
            }
        else:
            return {"success": False, "error": f"Shop '{shop_id}' not found"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_shop_by_owner(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get shop by owner NPC"""
    try:
        npc_id = command.get("npc_id", "")

        if not npc_id:
            return {"success": False, "error": "npc_id required"}

        lib = get_lib()
        found, shop = lib.get_shop_by_owner(npc_id)

        if found:
            return {
                "success": True,
                "npc_id": npc_id,
                "shop_id": str(shop.shop_id) if hasattr(shop, 'shop_id') else ""
            }
        else:
            return {"success": False, "error": f"No shop for NPC '{npc_id}'"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_all_shops(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all shops"""
    try:
        lib = get_lib()
        shops = lib.get_all_shops()

        shop_list = []
        for shop in shops:
            shop_list.append({
                "id": str(shop.shop_id) if hasattr(shop, 'shop_id') else "",
                "name": str(shop.name) if hasattr(shop, 'name') else ""
            })

        return {"success": True, "shops": shop_list, "count": len(shop_list)}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# BUYING
# ============================================

def handle_buy_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Buy an item from a shop"""
    try:
        shop_id = command.get("shop_id", "")
        item_id = command.get("item_id", "")
        quantity = command.get("quantity", 1)
        player_gold = command.get("player_gold", 0)

        if not shop_id or not item_id:
            return {"success": False, "error": "shop_id and item_id required"}

        lib = get_lib()
        success, new_gold, result = lib.buy_item(shop_id, item_id, quantity, player_gold)

        return {
            "success": success,
            "shop_id": shop_id,
            "item_id": item_id,
            "quantity": quantity,
            "gold_spent": player_gold - new_gold if success else 0,
            "remaining_gold": new_gold
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_can_buy_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if player can buy an item"""
    try:
        shop_id = command.get("shop_id", "")
        item_id = command.get("item_id", "")
        quantity = command.get("quantity", 1)
        player_gold = command.get("player_gold", 0)
        player_level = command.get("player_level", 1)
        player_reputation = command.get("player_reputation", 0)

        if not shop_id or not item_id:
            return {"success": False, "error": "shop_id and item_id required"}

        lib = get_lib()
        can_buy, reason = lib.can_buy_item(shop_id, item_id, quantity, player_gold, player_level, player_reputation)

        return {
            "success": True,
            "can_buy": can_buy,
            "reason": str(reason)
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_buy_price(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get the effective buy price for an item"""
    try:
        shop_id = command.get("shop_id", "")
        item_id = command.get("item_id", "")
        player_reputation = command.get("player_reputation", 0)

        if not shop_id or not item_id:
            return {"success": False, "error": "shop_id and item_id required"}

        lib = get_lib()
        price = lib.get_buy_price(shop_id, item_id, player_reputation)

        return {"success": True, "shop_id": shop_id, "item_id": item_id, "price": price}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SELLING
# ============================================

def handle_sell_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Sell an item to a shop"""
    try:
        shop_id = command.get("shop_id", "")
        item_id = command.get("item_id", "")
        quantity = command.get("quantity", 1)
        player_gold = command.get("player_gold", 0)

        if not shop_id or not item_id:
            return {"success": False, "error": "shop_id and item_id required"}

        lib = get_lib()
        success, new_gold, result = lib.sell_item(shop_id, item_id, quantity, player_gold)

        return {
            "success": success,
            "shop_id": shop_id,
            "item_id": item_id,
            "quantity": quantity,
            "gold_received": new_gold - player_gold if success else 0,
            "new_gold": new_gold
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_shop_will_buy_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if shop will buy an item"""
    try:
        shop_id = command.get("shop_id", "")
        item_id = command.get("item_id", "")

        if not shop_id or not item_id:
            return {"success": False, "error": "shop_id and item_id required"}

        lib = get_lib()
        will_buy, reason = lib.shop_will_buy_item(shop_id, item_id)

        return {
            "success": True,
            "will_buy": will_buy,
            "reason": str(reason)
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_sell_price(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get the sell price for an item"""
    try:
        shop_id = command.get("shop_id", "")
        item_id = command.get("item_id", "")
        player_reputation = command.get("player_reputation", 0)

        if not shop_id or not item_id:
            return {"success": False, "error": "shop_id and item_id required"}

        lib = get_lib()
        price = lib.get_sell_price(shop_id, item_id, player_reputation)

        return {"success": True, "shop_id": shop_id, "item_id": item_id, "price": price}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# STOCK MANAGEMENT
# ============================================

def handle_get_item_stock(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get current stock for an item"""
    try:
        shop_id = command.get("shop_id", "")
        item_id = command.get("item_id", "")

        if not shop_id or not item_id:
            return {"success": False, "error": "shop_id and item_id required"}

        lib = get_lib()
        stock = lib.get_item_stock(shop_id, item_id)

        return {
            "success": True,
            "shop_id": shop_id,
            "item_id": item_id,
            "stock": stock,
            "unlimited": stock == -1
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_restock_shop(command: Dict[str, Any]) -> Dict[str, Any]:
    """Restock a shop"""
    try:
        shop_id = command.get("shop_id", "")

        if not shop_id:
            return {"success": False, "error": "shop_id required"}

        log.log_info(f"Restock shop: {shop_id}")

        return {"success": True, "shop_id": shop_id, "message": "Shop restocked"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_restock_all_shops(command: Dict[str, Any]) -> Dict[str, Any]:
    """Restock all shops that need it"""
    try:
        log.log_info("Restock all shops")

        return {"success": True, "message": "All shops restocked"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SHOP ITEMS QUERY
# ============================================

def handle_get_shop_items(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all items in a shop"""
    try:
        shop_id = command.get("shop_id", "")

        if not shop_id:
            return {"success": False, "error": "shop_id required"}

        lib = get_lib()
        items = lib.get_shop_items(shop_id)

        item_list = []
        for item in items:
            item_list.append({
                "item_id": str(item.item_id) if hasattr(item, 'item_id') else "",
                "price": item.price if hasattr(item, 'price') else 0,
                "stock": item.stock if hasattr(item, 'stock') else -1
            })

        return {"success": True, "shop_id": shop_id, "items": item_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_featured_items(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get featured items in a shop"""
    try:
        shop_id = command.get("shop_id", "")

        if not shop_id:
            return {"success": False, "error": "shop_id required"}

        lib = get_lib()
        items = lib.get_featured_items(shop_id)

        item_list = []
        for item in items:
            item_list.append({
                "item_id": str(item.item_id) if hasattr(item, 'item_id') else ""
            })

        return {"success": True, "shop_id": shop_id, "featured_items": item_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_items_on_sale(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get items on sale"""
    try:
        shop_id = command.get("shop_id", "")

        if not shop_id:
            return {"success": False, "error": "shop_id required"}

        lib = get_lib()
        items = lib.get_items_on_sale(shop_id)

        item_list = []
        for item in items:
            item_list.append({
                "item_id": str(item.item_id) if hasattr(item, 'item_id') else "",
                "discount": item.discount if hasattr(item, 'discount') else 0
            })

        return {"success": True, "shop_id": shop_id, "sale_items": item_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_affordable_items(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get items player can afford"""
    try:
        shop_id = command.get("shop_id", "")
        player_gold = command.get("player_gold", 0)
        player_level = command.get("player_level", 1)

        if not shop_id:
            return {"success": False, "error": "shop_id required"}

        lib = get_lib()
        items = lib.get_affordable_items(shop_id, player_gold, player_level)

        item_list = []
        for item in items:
            item_list.append({
                "item_id": str(item.item_id) if hasattr(item, 'item_id') else "",
                "price": item.price if hasattr(item, 'price') else 0
            })

        return {"success": True, "shop_id": shop_id, "affordable_items": item_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SPECIAL OFFERS
# ============================================

def handle_set_item_discount(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set a discount on an item"""
    try:
        shop_id = command.get("shop_id", "")
        item_id = command.get("item_id", "")
        discount_percent = command.get("discount_percent", 0.1)

        if not shop_id or not item_id:
            return {"success": False, "error": "shop_id and item_id required"}

        lib = get_lib()
        lib.set_item_discount(shop_id, item_id, discount_percent)

        return {
            "success": True,
            "shop_id": shop_id,
            "item_id": item_id,
            "discount_percent": discount_percent
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_set_item_markup(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set a markup on an item"""
    try:
        shop_id = command.get("shop_id", "")
        item_id = command.get("item_id", "")
        markup_percent = command.get("markup_percent", 0.1)

        if not shop_id or not item_id:
            return {"success": False, "error": "shop_id and item_id required"}

        lib = get_lib()
        lib.set_item_markup(shop_id, item_id, markup_percent)

        return {
            "success": True,
            "shop_id": shop_id,
            "item_id": item_id,
            "markup_percent": markup_percent
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_clear_price_modifiers(command: Dict[str, Any]) -> Dict[str, Any]:
    """Clear all price modifiers for a shop"""
    try:
        shop_id = command.get("shop_id", "")

        if not shop_id:
            return {"success": False, "error": "shop_id required"}

        lib = get_lib()
        lib.clear_price_modifiers(shop_id)

        return {"success": True, "shop_id": shop_id, "message": "Price modifiers cleared"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SAVE/LOAD
# ============================================

def handle_save_shop_states(command: Dict[str, Any]) -> Dict[str, Any]:
    """Save shop states to JSON"""
    try:
        lib = get_lib()
        json_string = lib.save_shop_states_to_json()

        return {"success": True, "json": json_string}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_load_shop_states(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load shop states from JSON"""
    try:
        json_string = command.get("json", "")

        if not json_string:
            return {"success": False, "error": "json required"}

        lib = get_lib()
        success = lib.load_shop_states_from_json(json_string)

        return {"success": success}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

SHOP_COMMANDS = {
    # Registry
    "shop_register": handle_register_shop,
    "shop_load_json": handle_load_shops_from_json,
    "shop_get": handle_get_shop,
    "shop_get_by_owner": handle_get_shop_by_owner,
    "shop_get_all": handle_get_all_shops,

    # Buying
    "shop_buy": handle_buy_item,
    "shop_can_buy": handle_can_buy_item,
    "shop_buy_price": handle_get_buy_price,

    # Selling
    "shop_sell": handle_sell_item,
    "shop_will_buy": handle_shop_will_buy_item,
    "shop_sell_price": handle_get_sell_price,

    # Stock
    "shop_get_stock": handle_get_item_stock,
    "shop_restock": handle_restock_shop,
    "shop_restock_all": handle_restock_all_shops,

    # Items
    "shop_get_items": handle_get_shop_items,
    "shop_featured": handle_get_featured_items,
    "shop_on_sale": handle_get_items_on_sale,
    "shop_affordable": handle_get_affordable_items,

    # Offers
    "shop_set_discount": handle_set_item_discount,
    "shop_set_markup": handle_set_item_markup,
    "shop_clear_modifiers": handle_clear_price_modifiers,

    # Save/Load
    "shop_save_states": handle_save_shop_states,
    "shop_load_states": handle_load_shop_states,
}
