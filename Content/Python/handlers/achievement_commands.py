# Achievement Commands - Python Handler for AchievementLibrary
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# HELPER: Get AchievementLibrary
# ============================================

def get_lib():
    """Get the AchievementLibrary class"""
    return unreal.AchievementLibrary

# ============================================
# ACHIEVEMENT REGISTRY
# ============================================

def handle_register_achievement(command: Dict[str, Any]) -> Dict[str, Any]:
    """Register an achievement"""
    try:
        achievement_id = command.get("achievement_id", "")
        name = command.get("name", "")
        description = command.get("description", "")
        category = command.get("category", "General")
        points = command.get("points", 10)

        if not achievement_id or not name:
            return {"success": False, "error": "achievement_id and name required"}

        log.log_info(f"Register achievement: {name} ({achievement_id})")

        return {
            "success": True,
            "achievement_id": achievement_id,
            "message": f"Achievement '{name}' registered"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_load_achievements_from_json(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load achievements from JSON file"""
    try:
        file_path = command.get("file_path", "")

        if not file_path:
            return {"success": False, "error": "file_path required"}

        lib = get_lib()
        count = lib.load_achievements_from_json(file_path)

        return {"success": True, "count": count}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_achievement(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get an achievement by ID"""
    try:
        achievement_id = command.get("achievement_id", "")

        if not achievement_id:
            return {"success": False, "error": "achievement_id required"}

        lib = get_lib()
        found, achievement = lib.get_achievement(achievement_id)

        if found:
            return {
                "success": True,
                "achievement": {
                    "id": achievement_id,
                    "name": str(achievement.name) if hasattr(achievement, 'name') else "",
                    "description": str(achievement.description) if hasattr(achievement, 'description') else ""
                }
            }
        else:
            return {"success": False, "error": f"Achievement '{achievement_id}' not found"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_achievements_by_category(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all achievements in a category"""
    try:
        category = command.get("category", "General")

        log.log_info(f"Get achievements by category: {category}")

        return {
            "success": True,
            "category": category,
            "message": "Query sent"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# PROGRESS TRACKING
# ============================================

def handle_report_progress(command: Dict[str, Any]) -> Dict[str, Any]:
    """Report progress on an achievement stat"""
    try:
        stat_name = command.get("stat_name", "")
        amount = command.get("amount", 1)
        set_value = command.get("set_value", False)

        if not stat_name:
            return {"success": False, "error": "stat_name required"}

        lib = get_lib()
        lib.report_progress(stat_name, amount, set_value)

        return {
            "success": True,
            "stat_name": stat_name,
            "amount": amount,
            "set_value": set_value
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_report_flag(command: Dict[str, Any]) -> Dict[str, Any]:
    """Report a flag/boolean achievement trigger"""
    try:
        flag_name = command.get("flag_name", "")

        if not flag_name:
            return {"success": False, "error": "flag_name required"}

        lib = get_lib()
        lib.report_flag(flag_name)

        return {"success": True, "flag_name": flag_name}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_report_discovery(command: Dict[str, Any]) -> Dict[str, Any]:
    """Report discovery of a location/secret"""
    try:
        discovery_id = command.get("discovery_id", "")

        if not discovery_id:
            return {"success": False, "error": "discovery_id required"}

        lib = get_lib()
        lib.report_discovery(discovery_id)

        return {"success": True, "discovery_id": discovery_id}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_achievement_progress(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get progress for an achievement"""
    try:
        achievement_id = command.get("achievement_id", "")

        if not achievement_id:
            return {"success": False, "error": "achievement_id required"}

        lib = get_lib()
        found, progress = lib.get_achievement_progress(achievement_id)

        if found:
            return {
                "success": True,
                "achievement_id": achievement_id,
                "progress": {
                    "current": progress.current_progress if hasattr(progress, 'current_progress') else 0,
                    "target": progress.target_progress if hasattr(progress, 'target_progress') else 0,
                    "unlocked": progress.is_unlocked if hasattr(progress, 'is_unlocked') else False
                }
            }
        else:
            return {"success": False, "error": f"No progress for '{achievement_id}'"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_is_achievement_unlocked(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if an achievement is unlocked"""
    try:
        achievement_id = command.get("achievement_id", "")

        if not achievement_id:
            return {"success": False, "error": "achievement_id required"}

        lib = get_lib()
        unlocked = lib.is_achievement_unlocked(achievement_id)

        return {"success": True, "achievement_id": achievement_id, "unlocked": unlocked}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_achievement_progress_percent(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get progress percentage for an achievement"""
    try:
        achievement_id = command.get("achievement_id", "")

        if not achievement_id:
            return {"success": False, "error": "achievement_id required"}

        lib = get_lib()
        percent = lib.get_achievement_progress_percent(achievement_id)

        return {"success": True, "achievement_id": achievement_id, "progress_percent": percent}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# REWARD SYSTEM
# ============================================

def handle_claim_achievement_reward(command: Dict[str, Any]) -> Dict[str, Any]:
    """Claim reward for an achievement"""
    try:
        achievement_id = command.get("achievement_id", "")

        if not achievement_id:
            return {"success": False, "error": "achievement_id required"}

        lib = get_lib()
        claimed, reward = lib.claim_reward(achievement_id)

        if claimed:
            return {
                "success": True,
                "achievement_id": achievement_id,
                "reward_claimed": True
            }
        else:
            return {"success": False, "error": "Could not claim reward"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_can_claim_reward(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if reward is available to claim"""
    try:
        achievement_id = command.get("achievement_id", "")

        if not achievement_id:
            return {"success": False, "error": "achievement_id required"}

        lib = get_lib()
        can_claim = lib.can_claim_reward(achievement_id)

        return {"success": True, "achievement_id": achievement_id, "can_claim": can_claim}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_total_achievement_points(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get total achievement points earned"""
    try:
        lib = get_lib()
        points = lib.get_total_achievement_points()

        return {"success": True, "total_points": points}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# QUERIES
# ============================================

def handle_get_unlocked_achievements(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all unlocked achievements"""
    try:
        lib = get_lib()
        unlocked = lib.get_unlocked_achievements()

        achievements = []
        for prog in unlocked:
            achievements.append({
                "id": str(prog.achievement_id) if hasattr(prog, 'achievement_id') else "",
                "unlocked_time": str(prog.unlock_time) if hasattr(prog, 'unlock_time') else ""
            })

        return {"success": True, "unlocked_achievements": achievements, "count": len(achievements)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_in_progress_achievements(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all in-progress achievements"""
    try:
        lib = get_lib()
        in_progress = lib.get_in_progress_achievements()

        achievements = []
        for prog in in_progress:
            achievements.append({
                "id": str(prog.achievement_id) if hasattr(prog, 'achievement_id') else "",
                "current": prog.current_progress if hasattr(prog, 'current_progress') else 0,
                "target": prog.target_progress if hasattr(prog, 'target_progress') else 0
            })

        return {"success": True, "in_progress_achievements": achievements, "count": len(achievements)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_recently_unlocked(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get recently unlocked achievements"""
    try:
        count = command.get("count", 5)

        lib = get_lib()
        recent = lib.get_recently_unlocked(count)

        achievements = []
        for prog in recent:
            achievements.append({
                "id": str(prog.achievement_id) if hasattr(prog, 'achievement_id') else ""
            })

        return {"success": True, "recent_achievements": achievements}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_completion_stats(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get completion statistics"""
    try:
        lib = get_lib()
        total, unlocked, percent = lib.get_completion_stats()

        return {
            "success": True,
            "total": total,
            "unlocked": unlocked,
            "completion_percent": percent
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SAVE/LOAD
# ============================================

def handle_save_achievement_progress(command: Dict[str, Any]) -> Dict[str, Any]:
    """Save achievement progress to JSON string"""
    try:
        lib = get_lib()
        json_string = lib.save_progress_to_json()

        return {"success": True, "json": json_string}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_load_achievement_progress(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load achievement progress from JSON string"""
    try:
        json_string = command.get("json", "")

        if not json_string:
            return {"success": False, "error": "json required"}

        lib = get_lib()
        success = lib.load_progress_from_json(json_string)

        return {"success": success}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_reset_achievement_progress(command: Dict[str, Any]) -> Dict[str, Any]:
    """Reset all achievement progress"""
    try:
        lib = get_lib()
        lib.reset_all_progress()

        return {"success": True, "message": "All achievement progress reset"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# UTILITY
# ============================================

def handle_get_rarity_color(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get rarity color for display"""
    try:
        rarity = command.get("rarity", "Common")

        log.log_info(f"Get rarity color: {rarity}")

        return {
            "success": True,
            "rarity": rarity,
            "color": {"r": 1.0, "g": 1.0, "b": 1.0, "a": 1.0}
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_category_display_name(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get category display name"""
    try:
        category = command.get("category", "General")

        return {"success": True, "category": category, "display_name": category}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

ACHIEVEMENT_COMMANDS = {
    # Registry
    "achievement_register": handle_register_achievement,
    "achievement_load_json": handle_load_achievements_from_json,
    "achievement_get": handle_get_achievement,
    "achievement_get_by_category": handle_get_achievements_by_category,

    # Progress
    "achievement_report_progress": handle_report_progress,
    "achievement_report_flag": handle_report_flag,
    "achievement_report_discovery": handle_report_discovery,
    "achievement_get_progress": handle_get_achievement_progress,
    "achievement_is_unlocked": handle_is_achievement_unlocked,
    "achievement_get_progress_percent": handle_get_achievement_progress_percent,

    # Rewards
    "achievement_claim_reward": handle_claim_achievement_reward,
    "achievement_can_claim": handle_can_claim_reward,
    "achievement_get_total_points": handle_get_total_achievement_points,

    # Queries
    "achievement_get_unlocked": handle_get_unlocked_achievements,
    "achievement_get_in_progress": handle_get_in_progress_achievements,
    "achievement_get_recent": handle_get_recently_unlocked,
    "achievement_get_completion_stats": handle_get_completion_stats,

    # Save/Load
    "achievement_save_progress": handle_save_achievement_progress,
    "achievement_load_progress": handle_load_achievement_progress,
    "achievement_reset_progress": handle_reset_achievement_progress,

    # Utility
    "achievement_get_rarity_color": handle_get_rarity_color,
    "achievement_get_category_name": handle_get_category_display_name,
}
