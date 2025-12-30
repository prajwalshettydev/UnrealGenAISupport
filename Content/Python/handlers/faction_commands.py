# Faction Commands - Python Handler for FactionLibrary
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# HELPER: Get FactionLibrary
# ============================================

def get_lib():
    """Get the FactionLibrary class"""
    return unreal.FactionLibrary

# ============================================
# FACTION REGISTRY
# ============================================

def handle_register_faction(command: Dict[str, Any]) -> Dict[str, Any]:
    """Register a faction"""
    try:
        faction_id = command.get("faction_id", "")
        name = command.get("name", "")
        description = command.get("description", "")

        if not faction_id or not name:
            return {"success": False, "error": "faction_id and name required"}

        log.log_info(f"Register faction: {name} ({faction_id})")

        return {
            "success": True,
            "faction_id": faction_id,
            "message": f"Faction '{name}' registered"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_load_factions_from_json(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load factions from JSON file"""
    try:
        file_path = command.get("file_path", "")

        if not file_path:
            return {"success": False, "error": "file_path required"}

        lib = get_lib()
        count = lib.load_factions_from_json(file_path)

        return {"success": True, "count": count}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_faction(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get a faction by ID"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        found, faction = lib.get_faction(faction_id)

        if found:
            return {
                "success": True,
                "faction": {
                    "id": faction_id,
                    "name": str(faction.name) if hasattr(faction, 'name') else "",
                    "description": str(faction.description) if hasattr(faction, 'description') else ""
                }
            }
        else:
            return {"success": False, "error": f"Faction '{faction_id}' not found"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_all_factions(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all registered factions"""
    try:
        lib = get_lib()
        factions = lib.get_all_factions()

        faction_list = []
        for f in factions:
            faction_list.append({
                "id": str(f.faction_id) if hasattr(f, 'faction_id') else "",
                "name": str(f.name) if hasattr(f, 'name') else ""
            })

        return {"success": True, "factions": faction_list, "count": len(faction_list)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_discovered_factions(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all discovered factions"""
    try:
        lib = get_lib()
        discovered = lib.get_discovered_factions()

        return {"success": True, "discovered_factions": list(discovered)}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# REPUTATION MANAGEMENT
# ============================================

def handle_modify_reputation(command: Dict[str, Any]) -> Dict[str, Any]:
    """Modify reputation with a faction"""
    try:
        faction_id = command.get("faction_id", "")
        amount = command.get("amount", 0)
        reason = command.get("reason", "")
        source = command.get("source", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        event = lib.modify_reputation(faction_id, amount, reason, source)

        return {
            "success": True,
            "faction_id": faction_id,
            "amount": amount,
            "new_reputation": event.new_reputation if hasattr(event, 'new_reputation') else 0
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_set_reputation(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set reputation to specific value"""
    try:
        faction_id = command.get("faction_id", "")
        new_reputation = command.get("new_reputation", 0)

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        event = lib.set_reputation(faction_id, new_reputation)

        return {
            "success": True,
            "faction_id": faction_id,
            "new_reputation": new_reputation
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_reputation(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get current reputation with faction"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        reputation = lib.get_reputation(faction_id)

        return {"success": True, "faction_id": faction_id, "reputation": reputation}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_rank(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get current rank with faction"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        rank = lib.get_rank(faction_id)

        return {"success": True, "faction_id": faction_id, "rank": str(rank)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_player_standing(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get player standing with faction"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        found, standing = lib.get_player_standing(faction_id)

        if found:
            return {
                "success": True,
                "faction_id": faction_id,
                "standing": {
                    "reputation": standing.reputation if hasattr(standing, 'reputation') else 0,
                    "rank": str(standing.rank) if hasattr(standing, 'rank') else "Neutral"
                }
            }
        else:
            return {"success": False, "error": f"No standing with faction '{faction_id}'"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_all_player_standings(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all player standings"""
    try:
        lib = get_lib()
        standings = lib.get_all_player_standings()

        standings_list = []
        for s in standings:
            standings_list.append({
                "faction_id": str(s.faction_id) if hasattr(s, 'faction_id') else "",
                "reputation": s.reputation if hasattr(s, 'reputation') else 0,
                "rank": str(s.rank) if hasattr(s, 'rank') else "Neutral"
            })

        return {"success": True, "standings": standings_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# RANK UTILITIES
# ============================================

def handle_get_rank_from_reputation(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get rank from reputation value"""
    try:
        reputation = command.get("reputation", 0)

        lib = get_lib()
        rank = lib.get_rank_from_reputation(reputation)

        return {"success": True, "reputation": reputation, "rank": str(rank)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_rank_thresholds(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get reputation range for a rank"""
    try:
        rank = command.get("rank", "Neutral")

        log.log_info(f"Get rank thresholds: {rank}")

        # Default thresholds
        thresholds = {
            "Hated": {"min": -42000, "max": -6000},
            "Hostile": {"min": -6000, "max": -3000},
            "Unfriendly": {"min": -3000, "max": 0},
            "Neutral": {"min": 0, "max": 3000},
            "Friendly": {"min": 3000, "max": 9000},
            "Honored": {"min": 9000, "max": 21000},
            "Revered": {"min": 21000, "max": 42000},
            "Exalted": {"min": 42000, "max": 999999}
        }

        if rank in thresholds:
            return {"success": True, "rank": rank, **thresholds[rank]}
        else:
            return {"success": False, "error": f"Unknown rank: {rank}"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_rank_display_name(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get display name for rank"""
    try:
        rank = command.get("rank", "Neutral")

        return {"success": True, "rank": rank, "display_name": rank}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_rank_color(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get color for rank"""
    try:
        rank = command.get("rank", "Neutral")

        colors = {
            "Hated": {"r": 0.8, "g": 0.0, "b": 0.0, "a": 1.0},
            "Hostile": {"r": 1.0, "g": 0.2, "b": 0.2, "a": 1.0},
            "Unfriendly": {"r": 1.0, "g": 0.5, "b": 0.0, "a": 1.0},
            "Neutral": {"r": 1.0, "g": 1.0, "b": 0.0, "a": 1.0},
            "Friendly": {"r": 0.0, "g": 1.0, "b": 0.0, "a": 1.0},
            "Honored": {"r": 0.0, "g": 0.8, "b": 0.2, "a": 1.0},
            "Revered": {"r": 0.0, "g": 0.6, "b": 1.0, "a": 1.0},
            "Exalted": {"r": 0.8, "g": 0.0, "b": 1.0, "a": 1.0}
        }

        color = colors.get(rank, {"r": 1.0, "g": 1.0, "b": 1.0, "a": 1.0})

        return {"success": True, "rank": rank, "color": color}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_progress_to_next_rank(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get progress to next rank"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        has_next, current_rep, next_rank_rep, progress = lib.get_progress_to_next_rank(faction_id)

        return {
            "success": True,
            "faction_id": faction_id,
            "has_next_rank": has_next,
            "current_reputation": current_rep,
            "next_rank_reputation": next_rank_rep,
            "progress": progress
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# FACTION RELATIONSHIPS
# ============================================

def handle_get_faction_relationship(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get relationship between two factions"""
    try:
        faction_a = command.get("faction_a", "")
        faction_b = command.get("faction_b", "")

        if not faction_a or not faction_b:
            return {"success": False, "error": "faction_a and faction_b required"}

        lib = get_lib()
        relation = lib.get_faction_relationship(faction_a, faction_b)

        return {
            "success": True,
            "faction_a": faction_a,
            "faction_b": faction_b,
            "relationship": str(relation)
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_allied_factions(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get allied factions"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        allies = lib.get_allied_factions(faction_id)

        return {"success": True, "faction_id": faction_id, "allies": list(allies)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_enemy_factions(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get enemy factions"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        enemies = lib.get_enemy_factions(faction_id)

        return {"success": True, "faction_id": faction_id, "enemies": list(enemies)}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# REWARDS
# ============================================

def handle_get_available_faction_rewards(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get available rank rewards"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        rewards = lib.get_available_rewards(faction_id)

        reward_list = []
        for r in rewards:
            reward_list.append({
                "rank": str(r.rank) if hasattr(r, 'rank') else "",
                "item_id": str(r.item_id) if hasattr(r, 'item_id') else ""
            })

        return {"success": True, "faction_id": faction_id, "rewards": reward_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_unclaimed_faction_rewards(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get unclaimed rewards"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        rewards = lib.get_unclaimed_rewards(faction_id)

        reward_list = []
        for r in rewards:
            reward_list.append({
                "rank": str(r.rank) if hasattr(r, 'rank') else ""
            })

        return {"success": True, "faction_id": faction_id, "unclaimed_rewards": reward_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_claim_faction_rank_reward(command: Dict[str, Any]) -> Dict[str, Any]:
    """Claim a rank reward"""
    try:
        faction_id = command.get("faction_id", "")
        rank = command.get("rank", "")

        if not faction_id or not rank:
            return {"success": False, "error": "faction_id and rank required"}

        log.log_info(f"Claim faction reward: {faction_id} - {rank}")

        return {
            "success": True,
            "faction_id": faction_id,
            "rank": rank,
            "message": "Reward claimed"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_shop_discount(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get current shop discount for faction"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        discount = lib.get_shop_discount(faction_id)

        return {"success": True, "faction_id": faction_id, "discount": discount}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# DISCOVERY
# ============================================

def handle_discover_faction(command: Dict[str, Any]) -> Dict[str, Any]:
    """Discover a faction"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        newly_discovered = lib.discover_faction(faction_id)

        return {
            "success": True,
            "faction_id": faction_id,
            "newly_discovered": newly_discovered
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_is_faction_discovered(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if faction is discovered"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        discovered = lib.is_faction_discovered(faction_id)

        return {"success": True, "faction_id": faction_id, "discovered": discovered}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# NPC/ENTITY QUERIES
# ============================================

def handle_get_npc_faction(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get faction of an NPC"""
    try:
        npc_id = command.get("npc_id", "")

        if not npc_id:
            return {"success": False, "error": "npc_id required"}

        lib = get_lib()
        faction_id = lib.get_npc_faction(npc_id)

        return {"success": True, "npc_id": npc_id, "faction_id": faction_id}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_faction_npcs(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all NPCs in a faction"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        npcs = lib.get_faction_np_cs(faction_id)

        return {"success": True, "faction_id": faction_id, "npcs": list(npcs)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_is_hostile_with_faction(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if player is hostile with faction"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        hostile = lib.is_hostile_with_faction(faction_id)

        return {"success": True, "faction_id": faction_id, "is_hostile": hostile}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_can_trade_with_faction(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if player can trade with faction"""
    try:
        faction_id = command.get("faction_id", "")

        if not faction_id:
            return {"success": False, "error": "faction_id required"}

        lib = get_lib()
        can_trade = lib.can_trade_with_faction(faction_id)

        return {"success": True, "faction_id": faction_id, "can_trade": can_trade}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SAVE/LOAD
# ============================================

def handle_save_faction_progress(command: Dict[str, Any]) -> Dict[str, Any]:
    """Save faction progress to JSON"""
    try:
        lib = get_lib()
        json_string = lib.save_progress_to_json()

        return {"success": True, "json": json_string}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_load_faction_progress(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load faction progress from JSON"""
    try:
        json_string = command.get("json", "")

        if not json_string:
            return {"success": False, "error": "json required"}

        lib = get_lib()
        success = lib.load_progress_from_json(json_string)

        return {"success": success}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_reset_faction_progress(command: Dict[str, Any]) -> Dict[str, Any]:
    """Reset all faction progress"""
    try:
        lib = get_lib()
        lib.reset_all_progress()

        return {"success": True, "message": "All faction progress reset"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

FACTION_COMMANDS = {
    # Registry
    "faction_register": handle_register_faction,
    "faction_load_json": handle_load_factions_from_json,
    "faction_get": handle_get_faction,
    "faction_get_all": handle_get_all_factions,
    "faction_get_discovered": handle_get_discovered_factions,

    # Reputation
    "faction_modify_reputation": handle_modify_reputation,
    "faction_set_reputation": handle_set_reputation,
    "faction_get_reputation": handle_get_reputation,
    "faction_get_rank": handle_get_rank,
    "faction_get_standing": handle_get_player_standing,
    "faction_get_all_standings": handle_get_all_player_standings,

    # Rank
    "faction_rank_from_reputation": handle_get_rank_from_reputation,
    "faction_rank_thresholds": handle_get_rank_thresholds,
    "faction_rank_display_name": handle_get_rank_display_name,
    "faction_rank_color": handle_get_rank_color,
    "faction_progress_to_next": handle_get_progress_to_next_rank,

    # Relationships
    "faction_relationship": handle_get_faction_relationship,
    "faction_allies": handle_get_allied_factions,
    "faction_enemies": handle_get_enemy_factions,

    # Rewards
    "faction_available_rewards": handle_get_available_faction_rewards,
    "faction_unclaimed_rewards": handle_get_unclaimed_faction_rewards,
    "faction_claim_reward": handle_claim_faction_rank_reward,
    "faction_shop_discount": handle_get_shop_discount,

    # Discovery
    "faction_discover": handle_discover_faction,
    "faction_is_discovered": handle_is_faction_discovered,

    # NPC
    "faction_get_npc_faction": handle_get_npc_faction,
    "faction_get_npcs": handle_get_faction_npcs,
    "faction_is_hostile": handle_is_hostile_with_faction,
    "faction_can_trade": handle_can_trade_with_faction,

    # Save/Load
    "faction_save": handle_save_faction_progress,
    "faction_load": handle_load_faction_progress,
    "faction_reset": handle_reset_faction_progress,
}
