# Crafting Commands - Python Handler for CraftingLibrary
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# RECIPE REGISTRY
# ============================================

def handle_load_recipes(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load recipes from JSON file"""
    try:
        file_path = command.get("file_path", "")

        if not file_path:
            return {"success": False, "error": "file_path required"}

        count = unreal.CraftingLibrary.load_recipes_from_json(file_path)

        return {"success": True, "recipes_loaded": count}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_recipe(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get recipe by ID"""
    try:
        recipe_id = command.get("recipe_id", "")

        if not recipe_id:
            return {"success": False, "error": "recipe_id required"}

        found, recipe = unreal.CraftingLibrary.get_recipe(recipe_id)

        if not found:
            return {"success": False, "error": f"Recipe not found: {recipe_id}"}

        return {
            "success": True,
            "recipe": {
                "id": recipe.id,
                "name": recipe.name,
                "description": recipe.description,
                "category": str(recipe.category),
                "station": str(recipe.station),
                "difficulty": str(recipe.difficulty),
                "required_level": recipe.required_level,
                "crafting_time": recipe.crafting_time,
                "xp_gain": recipe.xp_gain,
                "gold_cost": recipe.gold_cost,
                "hidden": recipe.hidden
            }
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_all_recipes(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all registered recipes"""
    try:
        recipes = unreal.CraftingLibrary.get_all_recipes()

        recipe_list = []
        for r in recipes:
            recipe_list.append({
                "id": r.id,
                "name": r.name,
                "category": str(r.category),
                "station": str(r.station),
                "difficulty": str(r.difficulty),
                "required_level": r.required_level
            })

        return {"success": True, "recipes": recipe_list, "count": len(recipe_list)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_recipes_by_category(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get recipes by category"""
    try:
        category = command.get("category", "WEAPONS")

        category_enum = getattr(unreal.ECraftingCategory, category.upper(), None)
        if category_enum is None:
            return {"success": False, "error": f"Unknown category: {category}"}

        recipes = unreal.CraftingLibrary.get_recipes_by_category(category_enum)

        recipe_list = [{"id": r.id, "name": r.name} for r in recipes]

        return {"success": True, "recipes": recipe_list, "count": len(recipe_list)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_recipes_by_station(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get recipes by crafting station"""
    try:
        station = command.get("station", "ANVIL")

        station_enum = getattr(unreal.ECraftingStation, station.upper(), None)
        if station_enum is None:
            return {"success": False, "error": f"Unknown station: {station}"}

        recipes = unreal.CraftingLibrary.get_recipes_by_station(station_enum)

        recipe_list = [{"id": r.id, "name": r.name, "difficulty": str(r.difficulty)} for r in recipes]

        return {"success": True, "recipes": recipe_list, "count": len(recipe_list)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_search_recipes(command: Dict[str, Any]) -> Dict[str, Any]:
    """Search recipes by name or tag"""
    try:
        search_term = command.get("search_term", "")

        if not search_term:
            return {"success": False, "error": "search_term required"}

        recipes = unreal.CraftingLibrary.search_recipes(search_term)

        recipe_list = [{"id": r.id, "name": r.name, "category": str(r.category)} for r in recipes]

        return {"success": True, "recipes": recipe_list, "count": len(recipe_list)}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# CRAFTING OPERATIONS
# ============================================

def handle_can_craft(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if player can craft recipe"""
    try:
        recipe_id = command.get("recipe_id", "")
        inventory = command.get("inventory", {})
        gold = command.get("gold", 0)
        skills = command.get("skills", [])
        known_recipes = command.get("known_recipes", [])
        station = command.get("station", "ANVIL")

        if not recipe_id:
            return {"success": False, "error": "recipe_id required"}

        station_enum = getattr(unreal.ECraftingStation, station.upper(), unreal.ECraftingStation.ANVIL)

        # Build skills array
        skill_array = []
        for s in skills:
            skill = unreal.CraftingSkill()
            skill.category = getattr(unreal.ECraftingCategory, s.get("category", "WEAPONS").upper(), unreal.ECraftingCategory.WEAPONS)
            skill.level = s.get("level", 1)
            skill.xp = s.get("xp", 0)
            skill_array.append(skill)

        # Build known recipes
        known = unreal.KnownRecipes()
        known.recipe_ids = known_recipes

        can_craft, reason = unreal.CraftingLibrary.can_craft(
            recipe_id, inventory, gold, skill_array, known, station_enum
        )

        return {
            "success": True,
            "can_craft": can_craft,
            "reason": str(reason) if not can_craft else ""
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_craft_item(command: Dict[str, Any]) -> Dict[str, Any]:
    """Craft an item"""
    try:
        recipe_id = command.get("recipe_id", "")
        inventory = command.get("inventory", {})
        gold = command.get("gold", 0)
        skills = command.get("skills", [])
        known_recipes = command.get("known_recipes", [])
        station = command.get("station", "ANVIL")
        luck_bonus = command.get("luck_bonus", 0.0)

        if not recipe_id:
            return {"success": False, "error": "recipe_id required"}

        station_enum = getattr(unreal.ECraftingStation, station.upper(), unreal.ECraftingStation.ANVIL)

        # Build skills array
        skill_array = []
        for s in skills:
            skill = unreal.CraftingSkill()
            skill.category = getattr(unreal.ECraftingCategory, s.get("category", "WEAPONS").upper(), unreal.ECraftingCategory.WEAPONS)
            skill.level = s.get("level", 1)
            skill.xp = s.get("xp", 0)
            skill_array.append(skill)

        known = unreal.KnownRecipes()
        known.recipe_ids = known_recipes

        result = unreal.CraftingLibrary.craft_item(
            recipe_id, inventory, gold, skill_array, known, station_enum, luck_bonus
        )

        outputs = []
        for o in result.output_items:
            outputs.append({"item_id": o.item_id, "quantity": o.quantity})

        return {
            "success": True,
            "crafting_success": result.b_success,
            "is_critical": result.b_is_critical,
            "xp_gained": result.xp_gained,
            "gold_spent": result.gold_spent,
            "output_items": outputs,
            "fail_reason": result.fail_reason
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_max_craftable(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get max craftable quantity"""
    try:
        recipe_id = command.get("recipe_id", "")
        inventory = command.get("inventory", {})
        gold = command.get("gold", 0)

        if not recipe_id:
            return {"success": False, "error": "recipe_id required"}

        max_qty = unreal.CraftingLibrary.get_max_craftable_quantity(recipe_id, inventory, gold)

        return {"success": True, "max_quantity": max_qty}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_calculate_success_chance(command: Dict[str, Any]) -> Dict[str, Any]:
    """Calculate success chance for recipe"""
    try:
        recipe_id = command.get("recipe_id", "")
        skills = command.get("skills", [])
        station_bonus = command.get("station_bonus", 0.0)

        if not recipe_id:
            return {"success": False, "error": "recipe_id required"}

        found, recipe = unreal.CraftingLibrary.get_recipe(recipe_id)
        if not found:
            return {"success": False, "error": f"Recipe not found: {recipe_id}"}

        skill_array = []
        for s in skills:
            skill = unreal.CraftingSkill()
            skill.category = getattr(unreal.ECraftingCategory, s.get("category", "WEAPONS").upper(), unreal.ECraftingCategory.WEAPONS)
            skill.level = s.get("level", 1)
            skill_array.append(skill)

        chance = unreal.CraftingLibrary.calculate_success_chance(recipe, skill_array, station_bonus)

        return {"success": True, "success_chance": chance}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# RECIPE LEARNING
# ============================================

def handle_learn_recipe(command: Dict[str, Any]) -> Dict[str, Any]:
    """Learn a recipe"""
    try:
        recipe_id = command.get("recipe_id", "")
        known_recipes = command.get("known_recipes", [])

        if not recipe_id:
            return {"success": False, "error": "recipe_id required"}

        known = unreal.KnownRecipes()
        known.recipe_ids = known_recipes

        learned = unreal.CraftingLibrary.learn_recipe(recipe_id, known)

        return {
            "success": True,
            "learned": learned,
            "known_recipes": list(known.recipe_ids)
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_is_recipe_known(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if recipe is known"""
    try:
        recipe_id = command.get("recipe_id", "")
        known_recipes = command.get("known_recipes", [])

        if not recipe_id:
            return {"success": False, "error": "recipe_id required"}

        known = unreal.KnownRecipes()
        known.recipe_ids = known_recipes

        is_known = unreal.CraftingLibrary.is_recipe_known(recipe_id, known)

        return {"success": True, "is_known": is_known}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SKILL MANAGEMENT
# ============================================

def handle_get_skill_level(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get skill level for category"""
    try:
        category = command.get("category", "WEAPONS")
        skills = command.get("skills", [])

        category_enum = getattr(unreal.ECraftingCategory, category.upper(), None)
        if category_enum is None:
            return {"success": False, "error": f"Unknown category: {category}"}

        skill_array = []
        for s in skills:
            skill = unreal.CraftingSkill()
            skill.category = getattr(unreal.ECraftingCategory, s.get("category", "WEAPONS").upper(), unreal.ECraftingCategory.WEAPONS)
            skill.level = s.get("level", 1)
            skill.xp = s.get("xp", 0)
            skill_array.append(skill)

        level = unreal.CraftingLibrary.get_skill_level(category_enum, skill_array)

        return {"success": True, "level": level}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_add_skill_xp(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add XP to crafting skill"""
    try:
        category = command.get("category", "WEAPONS")
        xp_amount = command.get("xp_amount", 0)
        skills = command.get("skills", [])

        category_enum = getattr(unreal.ECraftingCategory, category.upper(), None)
        if category_enum is None:
            return {"success": False, "error": f"Unknown category: {category}"}

        skill_array = []
        for s in skills:
            skill = unreal.CraftingSkill()
            skill.category = getattr(unreal.ECraftingCategory, s.get("category", "WEAPONS").upper(), unreal.ECraftingCategory.WEAPONS)
            skill.level = s.get("level", 1)
            skill.xp = s.get("xp", 0)
            skill_array.append(skill)

        success, new_level, leveled_up = unreal.CraftingLibrary.add_skill_xp(
            category_enum, xp_amount, skill_array
        )

        return {
            "success": success,
            "new_level": new_level,
            "leveled_up": leveled_up
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_create_default_skills(command: Dict[str, Any]) -> Dict[str, Any]:
    """Create default crafting skills"""
    try:
        skills = unreal.CraftingLibrary.create_default_skills()

        skill_list = []
        for s in skills:
            skill_list.append({
                "category": str(s.category),
                "level": s.level,
                "xp": s.xp
            })

        return {"success": True, "skills": skill_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# MATERIALS
# ============================================

def handle_get_required_materials(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get required materials for recipe"""
    try:
        recipe_id = command.get("recipe_id", "")

        if not recipe_id:
            return {"success": False, "error": "recipe_id required"}

        materials = unreal.CraftingLibrary.get_required_materials(recipe_id)

        mat_list = []
        for m in materials:
            mat_list.append({
                "item_id": m.item_id,
                "quantity": m.quantity,
                "optional": m.optional
            })

        return {"success": True, "materials": mat_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_has_required_materials(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if player has required materials"""
    try:
        recipe_id = command.get("recipe_id", "")
        inventory = command.get("inventory", {})

        if not recipe_id:
            return {"success": False, "error": "recipe_id required"}

        has_mats, missing = unreal.CraftingLibrary.has_required_materials(recipe_id, inventory)

        missing_list = []
        for m in missing:
            missing_list.append({
                "item_id": m.item_id,
                "quantity": m.quantity
            })

        return {
            "success": True,
            "has_materials": has_mats,
            "missing": missing_list
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# STATIONS
# ============================================

def handle_get_station_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get crafting station info"""
    try:
        station = command.get("station", "ANVIL")

        station_enum = getattr(unreal.ECraftingStation, station.upper(), None)
        if station_enum is None:
            return {"success": False, "error": f"Unknown station: {station}"}

        info = unreal.CraftingLibrary.get_station_info(station_enum)

        return {
            "success": True,
            "station": {
                "name": str(info.display_name),
                "description": str(info.description),
                "bonus": info.crafting_bonus,
                "speed_multiplier": info.speed_multiplier
            }
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SAVE/LOAD
# ============================================

def handle_save_crafting_progress(command: Dict[str, Any]) -> Dict[str, Any]:
    """Save crafting progress to JSON"""
    try:
        skills = command.get("skills", [])
        known_recipes = command.get("known_recipes", [])

        skill_array = []
        for s in skills:
            skill = unreal.CraftingSkill()
            skill.category = getattr(unreal.ECraftingCategory, s.get("category", "WEAPONS").upper(), unreal.ECraftingCategory.WEAPONS)
            skill.level = s.get("level", 1)
            skill.xp = s.get("xp", 0)
            skill_array.append(skill)

        known = unreal.KnownRecipes()
        known.recipe_ids = known_recipes

        json_str = unreal.CraftingLibrary.save_crafting_progress_to_json(skill_array, known)

        return {"success": True, "json": json_str}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_load_crafting_progress(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load crafting progress from JSON"""
    try:
        json_str = command.get("json", "")

        if not json_str:
            return {"success": False, "error": "json required"}

        loaded, skills, known = unreal.CraftingLibrary.load_crafting_progress_from_json(json_str)

        if not loaded:
            return {"success": False, "error": "Failed to load progress"}

        skill_list = []
        for s in skills:
            skill_list.append({
                "category": str(s.category),
                "level": s.level,
                "xp": s.xp
            })

        return {
            "success": True,
            "skills": skill_list,
            "known_recipes": list(known.recipe_ids)
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

CRAFTING_COMMANDS = {
    # Registry
    "crafting_load_recipes": handle_load_recipes,
    "crafting_get_recipe": handle_get_recipe,
    "crafting_get_all": handle_get_all_recipes,
    "crafting_get_by_category": handle_get_recipes_by_category,
    "crafting_get_by_station": handle_get_recipes_by_station,
    "crafting_search": handle_search_recipes,

    # Operations
    "crafting_can_craft": handle_can_craft,
    "crafting_craft": handle_craft_item,
    "crafting_max_quantity": handle_get_max_craftable,
    "crafting_success_chance": handle_calculate_success_chance,

    # Learning
    "crafting_learn": handle_learn_recipe,
    "crafting_is_known": handle_is_recipe_known,

    # Skills
    "crafting_get_skill_level": handle_get_skill_level,
    "crafting_add_xp": handle_add_skill_xp,
    "crafting_default_skills": handle_create_default_skills,

    # Materials
    "crafting_get_materials": handle_get_required_materials,
    "crafting_has_materials": handle_has_required_materials,

    # Stations
    "crafting_station_info": handle_get_station_info,

    # Save/Load
    "crafting_save": handle_save_crafting_progress,
    "crafting_load": handle_load_crafting_progress,
}
