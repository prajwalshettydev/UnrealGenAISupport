# SkillTree Commands - Python Handler for SkillTreeLibrary
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# SKILL TREE REGISTRY
# ============================================

def handle_load_skill_trees(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load skill trees from JSON file"""
    try:
        file_path = command.get("file_path", "")

        if not file_path:
            return {"success": False, "error": "file_path required"}

        count = unreal.SkillTreeLibrary.load_skill_trees_from_json(file_path)

        return {"success": True, "trees_loaded": count}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_skill_tree(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get skill tree by ID"""
    try:
        tree_id = command.get("tree_id", "")

        if not tree_id:
            return {"success": False, "error": "tree_id required"}

        found, tree = unreal.SkillTreeLibrary.get_skill_tree(tree_id)

        if not found:
            return {"success": False, "error": f"Tree not found: {tree_id}"}

        nodes = []
        for n in tree.nodes:
            nodes.append({
                "id": n.skill_id,
                "name": n.name,
                "type": str(n.node_type),
                "tier": n.tier,
                "max_rank": n.max_rank
            })

        return {
            "success": True,
            "tree": {
                "id": tree.tree_id,
                "name": tree.name,
                "description": str(tree.description),
                "category": str(tree.category),
                "icon": tree.icon_path,
                "node_count": len(nodes),
                "nodes": nodes
            }
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_all_skill_trees(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all skill trees"""
    try:
        trees = unreal.SkillTreeLibrary.get_all_skill_trees()

        tree_list = []
        for t in trees:
            tree_list.append({
                "id": t.tree_id,
                "name": t.name,
                "category": str(t.category),
                "node_count": len(t.nodes)
            })

        return {"success": True, "trees": tree_list, "count": len(tree_list)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_skill_trees_by_category(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get skill trees by category"""
    try:
        category = command.get("category", "CLASS")

        category_enum = getattr(unreal.ESkillTreeCategory, category.upper(), None)
        if category_enum is None:
            return {"success": False, "error": f"Unknown category: {category}"}

        trees = unreal.SkillTreeLibrary.get_skill_trees_by_category(category_enum)

        tree_list = [{"id": t.tree_id, "name": t.name} for t in trees]

        return {"success": True, "trees": tree_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_skill_trees_for_class(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get skill trees available for class"""
    try:
        class_name = command.get("class_name", "Warrior")
        level = command.get("level", 1)

        trees = unreal.SkillTreeLibrary.get_skill_trees_for_class(class_name, level)

        tree_list = [{"id": t.tree_id, "name": t.name} for t in trees]

        return {"success": True, "trees": tree_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_skill_node(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get skill node by ID"""
    try:
        skill_id = command.get("skill_id", "")

        if not skill_id:
            return {"success": False, "error": "skill_id required"}

        found, node = unreal.SkillTreeLibrary.get_skill_node(skill_id)

        if not found:
            return {"success": False, "error": f"Skill not found: {skill_id}"}

        prereqs = [p for p in node.prerequisites]

        return {
            "success": True,
            "skill": {
                "id": node.skill_id,
                "name": node.name,
                "description": str(node.description),
                "type": str(node.node_type),
                "tier": node.tier,
                "max_rank": node.max_rank,
                "point_cost": node.point_cost,
                "required_level": node.required_level,
                "icon": node.icon_path,
                "prerequisites": prereqs
            }
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SKILL UNLOCK & MANAGEMENT
# ============================================

def handle_can_unlock_skill(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if player can unlock a skill"""
    try:
        skill_id = command.get("skill_id", "")
        player_skills = command.get("player_skills", {})
        player_level = command.get("player_level", 1)
        attributes = command.get("attributes", {})

        if not skill_id:
            return {"success": False, "error": "skill_id required"}

        # Build player skill data
        skill_data = unreal.PlayerSkillData()
        skill_data.available_points = player_skills.get("available_points", 0)
        skill_data.total_points_spent = player_skills.get("total_points_spent", 0)

        # Build attribute map
        attr_map = {}
        for attr_name, value in attributes.items():
            attr_enum = getattr(unreal.EAttributeType, attr_name.upper(), None)
            if attr_enum:
                attr_map[attr_enum] = value

        can_unlock, reason = unreal.SkillTreeLibrary.can_unlock_skill(
            skill_id, skill_data, player_level, attr_map
        )

        return {
            "success": True,
            "can_unlock": can_unlock,
            "reason": str(reason) if not can_unlock else ""
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_unlock_skill(command: Dict[str, Any]) -> Dict[str, Any]:
    """Unlock/upgrade a skill"""
    try:
        skill_id = command.get("skill_id", "")
        player_skills = command.get("player_skills", {})
        player_level = command.get("player_level", 1)
        attributes = command.get("attributes", {})

        if not skill_id:
            return {"success": False, "error": "skill_id required"}

        skill_data = unreal.PlayerSkillData()
        skill_data.available_points = player_skills.get("available_points", 0)
        skill_data.total_points_spent = player_skills.get("total_points_spent", 0)

        attr_map = {}
        for attr_name, value in attributes.items():
            attr_enum = getattr(unreal.EAttributeType, attr_name.upper(), None)
            if attr_enum:
                attr_map[attr_enum] = value

        result = unreal.SkillTreeLibrary.unlock_skill(
            skill_id, skill_data, player_level, attr_map
        )

        return {
            "success": result.b_success,
            "new_rank": result.new_rank,
            "points_spent": result.points_spent,
            "fail_reason": result.fail_reason
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_skill_status(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get skill unlock status"""
    try:
        skill_id = command.get("skill_id", "")
        player_skills = command.get("player_skills", {})
        player_level = command.get("player_level", 1)
        attributes = command.get("attributes", {})

        if not skill_id:
            return {"success": False, "error": "skill_id required"}

        skill_data = unreal.PlayerSkillData()
        skill_data.available_points = player_skills.get("available_points", 0)

        attr_map = {}
        for attr_name, value in attributes.items():
            attr_enum = getattr(unreal.EAttributeType, attr_name.upper(), None)
            if attr_enum:
                attr_map[attr_enum] = value

        status = unreal.SkillTreeLibrary.get_skill_status(
            skill_id, skill_data, player_level, attr_map
        )

        return {"success": True, "status": str(status)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_skill_rank(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get current rank in skill"""
    try:
        skill_id = command.get("skill_id", "")
        player_skills = command.get("player_skills", {})

        if not skill_id:
            return {"success": False, "error": "skill_id required"}

        skill_data = unreal.PlayerSkillData()
        # Copy unlocked skills
        for sid, rank in player_skills.get("unlocked_skills", {}).items():
            skill_data.unlocked_skills[sid] = rank

        rank = unreal.SkillTreeLibrary.get_skill_rank(skill_id, skill_data)

        return {"success": True, "rank": rank}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_is_skill_maxed(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if skill is at max rank"""
    try:
        skill_id = command.get("skill_id", "")
        player_skills = command.get("player_skills", {})

        if not skill_id:
            return {"success": False, "error": "skill_id required"}

        skill_data = unreal.PlayerSkillData()
        for sid, rank in player_skills.get("unlocked_skills", {}).items():
            skill_data.unlocked_skills[sid] = rank

        maxed = unreal.SkillTreeLibrary.is_skill_maxed(skill_id, skill_data)

        return {"success": True, "is_maxed": maxed}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SKILL POINTS
# ============================================

def handle_add_skill_points(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add skill points"""
    try:
        player_skills = command.get("player_skills", {})
        points = command.get("points", 1)

        skill_data = unreal.PlayerSkillData()
        skill_data.available_points = player_skills.get("available_points", 0)

        unreal.SkillTreeLibrary.add_skill_points(skill_data, points)

        return {
            "success": True,
            "new_available_points": skill_data.available_points
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_available_points(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get available skill points"""
    try:
        player_skills = command.get("player_skills", {})

        skill_data = unreal.PlayerSkillData()
        skill_data.available_points = player_skills.get("available_points", 0)

        points = unreal.SkillTreeLibrary.get_available_skill_points(skill_data)

        return {"success": True, "available_points": points}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_total_points_spent(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get total points spent"""
    try:
        player_skills = command.get("player_skills", {})

        skill_data = unreal.PlayerSkillData()
        skill_data.total_points_spent = player_skills.get("total_points_spent", 0)

        total = unreal.SkillTreeLibrary.get_total_points_spent(skill_data)

        return {"success": True, "total_spent": total}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_points_in_tree(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get points spent in tree"""
    try:
        tree_id = command.get("tree_id", "")
        player_skills = command.get("player_skills", {})

        if not tree_id:
            return {"success": False, "error": "tree_id required"}

        skill_data = unreal.PlayerSkillData()
        for tree, points in player_skills.get("points_per_tree", {}).items():
            skill_data.points_per_tree[tree] = points

        points = unreal.SkillTreeLibrary.get_points_spent_in_tree(tree_id, skill_data)

        return {"success": True, "points_in_tree": points}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_points_for_level(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get skill points for level"""
    try:
        level = command.get("level", 1)

        points = unreal.SkillTreeLibrary.get_skill_points_for_level(level)

        return {"success": True, "points": points}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# RESPEC & RESET
# ============================================

def handle_respec_all(command: Dict[str, Any]) -> Dict[str, Any]:
    """Respec all skills"""
    try:
        player_skills = command.get("player_skills", {})
        player_gold = command.get("player_gold", 0)
        respec_cost = command.get("respec_cost", 100)

        skill_data = unreal.PlayerSkillData()
        skill_data.available_points = player_skills.get("available_points", 0)
        skill_data.total_points_spent = player_skills.get("total_points_spent", 0)

        result = unreal.SkillTreeLibrary.respec_all_skills(skill_data, player_gold, respec_cost)

        return {
            "success": result.b_success,
            "points_refunded": result.points_refunded,
            "gold_spent": result.gold_spent,
            "new_gold": result.new_gold,
            "fail_reason": result.fail_reason
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_respec_tree(command: Dict[str, Any]) -> Dict[str, Any]:
    """Respec single tree"""
    try:
        tree_id = command.get("tree_id", "")
        player_skills = command.get("player_skills", {})
        player_gold = command.get("player_gold", 0)
        respec_cost = command.get("respec_cost", 50)

        if not tree_id:
            return {"success": False, "error": "tree_id required"}

        skill_data = unreal.PlayerSkillData()

        result = unreal.SkillTreeLibrary.respec_tree(tree_id, skill_data, player_gold, respec_cost)

        return {
            "success": result.b_success,
            "points_refunded": result.points_refunded,
            "gold_spent": result.gold_spent
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_calculate_respec_cost(command: Dict[str, Any]) -> Dict[str, Any]:
    """Calculate respec cost"""
    try:
        player_skills = command.get("player_skills", {})
        player_level = command.get("player_level", 1)

        skill_data = unreal.PlayerSkillData()
        skill_data.total_points_spent = player_skills.get("total_points_spent", 0)

        cost = unreal.SkillTreeLibrary.calculate_respec_cost(skill_data, player_level)

        return {"success": True, "respec_cost": cost}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# ATTRIBUTE BONUSES
# ============================================

def handle_calculate_bonuses(command: Dict[str, Any]) -> Dict[str, Any]:
    """Calculate all attribute bonuses from skills"""
    try:
        player_skills = command.get("player_skills", {})

        skill_data = unreal.PlayerSkillData()
        for sid, rank in player_skills.get("unlocked_skills", {}).items():
            skill_data.unlocked_skills[sid] = rank

        bonuses = unreal.SkillTreeLibrary.calculate_attribute_bonuses(skill_data)

        bonus_dict = {}
        for attr, value in bonuses.flat_bonuses.items():
            bonus_dict[str(attr)] = {"flat": value, "percent": bonuses.percent_bonuses.get(attr, 0.0)}

        return {"success": True, "bonuses": bonus_dict}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_attribute_bonus(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get total bonus for specific attribute"""
    try:
        attribute = command.get("attribute", "STRENGTH")
        player_skills = command.get("player_skills", {})
        base_value = command.get("base_value", 100.0)

        attr_enum = getattr(unreal.EAttributeType, attribute.upper(), None)
        if attr_enum is None:
            return {"success": False, "error": f"Unknown attribute: {attribute}"}

        skill_data = unreal.PlayerSkillData()
        for sid, rank in player_skills.get("unlocked_skills", {}).items():
            skill_data.unlocked_skills[sid] = rank

        bonus = unreal.SkillTreeLibrary.get_attribute_bonus(attr_enum, skill_data, base_value)

        return {"success": True, "bonus": bonus}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# ACTIVE SKILLS
# ============================================

def handle_get_unlocked_active_skills(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all unlocked active skills"""
    try:
        player_skills = command.get("player_skills", {})

        skill_data = unreal.PlayerSkillData()
        for sid, rank in player_skills.get("unlocked_skills", {}).items():
            skill_data.unlocked_skills[sid] = rank

        skills = unreal.SkillTreeLibrary.get_unlocked_active_skills(skill_data)

        skill_list = []
        for s in skills:
            skill_list.append({
                "id": s.skill_id,
                "name": s.name,
                "type": str(s.node_type),
                "icon": s.icon_path
            })

        return {"success": True, "active_skills": skill_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_skill_effect(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get skill effect at current rank"""
    try:
        skill_id = command.get("skill_id", "")
        rank = command.get("rank", 1)

        if not skill_id:
            return {"success": False, "error": "skill_id required"}

        effect = unreal.SkillTreeLibrary.get_skill_effect_at_rank(skill_id, rank)

        return {
            "success": True,
            "effect": {
                "damage": effect.damage,
                "healing": effect.healing,
                "duration": effect.duration,
                "cooldown": effect.cooldown,
                "mana_cost": effect.mana_cost,
                "stamina_cost": effect.stamina_cost
            }
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_can_use_skill(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if active skill is usable"""
    try:
        skill_id = command.get("skill_id", "")
        rank = command.get("rank", 1)
        current_mana = command.get("current_mana", 100.0)
        current_stamina = command.get("current_stamina", 100.0)
        current_cooldown = command.get("current_cooldown", 0.0)

        if not skill_id:
            return {"success": False, "error": "skill_id required"}

        can_use = unreal.SkillTreeLibrary.can_use_active_skill(
            skill_id, rank, current_mana, current_stamina, current_cooldown
        )

        return {"success": True, "can_use": can_use}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# TREE ANALYSIS
# ============================================

def handle_get_connected_skills(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all skills connected to a skill"""
    try:
        skill_id = command.get("skill_id", "")

        if not skill_id:
            return {"success": False, "error": "skill_id required"}

        skills = unreal.SkillTreeLibrary.get_connected_skills(skill_id)

        skill_list = [{"id": s.skill_id, "name": s.name} for s in skills]

        return {"success": True, "connected_skills": skill_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_unlock_path(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get path to unlock a skill"""
    try:
        skill_id = command.get("skill_id", "")
        player_skills = command.get("player_skills", {})

        if not skill_id:
            return {"success": False, "error": "skill_id required"}

        skill_data = unreal.PlayerSkillData()
        for sid, rank in player_skills.get("unlocked_skills", {}).items():
            skill_data.unlocked_skills[sid] = rank

        path = unreal.SkillTreeLibrary.get_unlock_path(skill_id, skill_data)

        return {"success": True, "path": list(path)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_points_to_reach(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get total points needed to reach skill"""
    try:
        skill_id = command.get("skill_id", "")
        player_skills = command.get("player_skills", {})

        if not skill_id:
            return {"success": False, "error": "skill_id required"}

        skill_data = unreal.PlayerSkillData()

        points = unreal.SkillTreeLibrary.get_points_to_reach_skill(skill_id, skill_data)

        return {"success": True, "points_needed": points}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_skills_by_tag(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get skills by tag"""
    try:
        tag = command.get("tag", "")

        if not tag:
            return {"success": False, "error": "tag required"}

        skills = unreal.SkillTreeLibrary.get_skills_by_tag(tag)

        skill_list = [{"id": s.skill_id, "name": s.name, "type": str(s.node_type)} for s in skills]

        return {"success": True, "skills": skill_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# UI HELPERS
# ============================================

def handle_get_status_color(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get color for skill status"""
    try:
        status = command.get("status", "LOCKED")

        status_enum = getattr(unreal.ESkillUnlockStatus, status.upper(), unreal.ESkillUnlockStatus.LOCKED)

        color = unreal.SkillTreeLibrary.get_status_color(status_enum)

        return {
            "success": True,
            "color": {"r": color.r, "g": color.g, "b": color.b, "a": color.a}
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_skill_description(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get skill description with current rank values"""
    try:
        skill_id = command.get("skill_id", "")
        rank = command.get("rank", 0)

        if not skill_id:
            return {"success": False, "error": "skill_id required"}

        desc = unreal.SkillTreeLibrary.get_skill_description_with_values(skill_id, rank)

        return {"success": True, "description": str(desc)}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SAVE/LOAD
# ============================================

def handle_save_skill_progress(command: Dict[str, Any]) -> Dict[str, Any]:
    """Save skill progress to JSON"""
    try:
        player_skills = command.get("player_skills", {})

        skill_data = unreal.PlayerSkillData()
        skill_data.available_points = player_skills.get("available_points", 0)
        skill_data.total_points_spent = player_skills.get("total_points_spent", 0)
        for sid, rank in player_skills.get("unlocked_skills", {}).items():
            skill_data.unlocked_skills[sid] = rank

        json_str = unreal.SkillTreeLibrary.save_skill_progress_to_json(skill_data)

        return {"success": True, "json": json_str}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_load_skill_progress(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load skill progress from JSON"""
    try:
        json_str = command.get("json", "")

        if not json_str:
            return {"success": False, "error": "json required"}

        loaded, skill_data = unreal.SkillTreeLibrary.load_skill_progress_from_json(json_str)

        if not loaded:
            return {"success": False, "error": "Failed to load progress"}

        unlocked = {}
        for sid, rank in skill_data.unlocked_skills.items():
            unlocked[sid] = rank

        return {
            "success": True,
            "player_skills": {
                "available_points": skill_data.available_points,
                "total_points_spent": skill_data.total_points_spent,
                "unlocked_skills": unlocked
            }
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# PRESETS & BUILDS
# ============================================

def handle_get_recommended_skills(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get recommended skills for class"""
    try:
        class_name = command.get("class_name", "Warrior")
        level = command.get("level", 1)

        skills = unreal.SkillTreeLibrary.get_recommended_skills(class_name, level)

        return {"success": True, "recommended_skills": list(skills)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_apply_preset(command: Dict[str, Any]) -> Dict[str, Any]:
    """Apply skill preset/build"""
    try:
        skill_ids = command.get("skill_ids", [])
        player_skills = command.get("player_skills", {})
        player_level = command.get("player_level", 1)
        attributes = command.get("attributes", {})

        skill_data = unreal.PlayerSkillData()
        skill_data.available_points = player_skills.get("available_points", 0)

        attr_map = {}
        for attr_name, value in attributes.items():
            attr_enum = getattr(unreal.EAttributeType, attr_name.upper(), None)
            if attr_enum:
                attr_map[attr_enum] = value

        success = unreal.SkillTreeLibrary.apply_skill_preset(
            skill_ids, skill_data, player_level, attr_map
        )

        return {"success": success}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

SKILLTREE_COMMANDS = {
    # Registry
    "skilltree_load": handle_load_skill_trees,
    "skilltree_get": handle_get_skill_tree,
    "skilltree_get_all": handle_get_all_skill_trees,
    "skilltree_get_by_category": handle_get_skill_trees_by_category,
    "skilltree_get_for_class": handle_get_skill_trees_for_class,
    "skilltree_get_node": handle_get_skill_node,

    # Unlock
    "skilltree_can_unlock": handle_can_unlock_skill,
    "skilltree_unlock": handle_unlock_skill,
    "skilltree_status": handle_get_skill_status,
    "skilltree_get_rank": handle_get_skill_rank,
    "skilltree_is_maxed": handle_is_skill_maxed,

    # Points
    "skilltree_add_points": handle_add_skill_points,
    "skilltree_available_points": handle_get_available_points,
    "skilltree_total_spent": handle_get_total_points_spent,
    "skilltree_points_in_tree": handle_get_points_in_tree,
    "skilltree_points_for_level": handle_get_points_for_level,

    # Respec
    "skilltree_respec_all": handle_respec_all,
    "skilltree_respec_tree": handle_respec_tree,
    "skilltree_respec_cost": handle_calculate_respec_cost,

    # Bonuses
    "skilltree_calculate_bonuses": handle_calculate_bonuses,
    "skilltree_attribute_bonus": handle_get_attribute_bonus,

    # Active Skills
    "skilltree_active_skills": handle_get_unlocked_active_skills,
    "skilltree_skill_effect": handle_get_skill_effect,
    "skilltree_can_use": handle_can_use_skill,

    # Analysis
    "skilltree_connected": handle_get_connected_skills,
    "skilltree_unlock_path": handle_get_unlock_path,
    "skilltree_points_to_reach": handle_get_points_to_reach,
    "skilltree_by_tag": handle_get_skills_by_tag,

    # UI
    "skilltree_status_color": handle_get_status_color,
    "skilltree_description": handle_get_skill_description,

    # Save/Load
    "skilltree_save": handle_save_skill_progress,
    "skilltree_load": handle_load_skill_progress,

    # Presets
    "skilltree_recommended": handle_get_recommended_skills,
    "skilltree_apply_preset": handle_apply_preset,
}
