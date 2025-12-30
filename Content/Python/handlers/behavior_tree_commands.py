"""
Behavior Tree / AI Commands Handler
Wrapper for GenAIUtils C++ functions
AI Controllers, Behavior Trees, Blackboard, Navigation, Perception, EQS
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
# AI CONTROLLER MANAGEMENT
# ============================================

def handle_list_ai_controllers(command: Dict[str, Any]) -> Dict[str, Any]:
    """List all AI Controllers in the level"""
    try:
        log.log_command("list_ai_controllers", "Listing all AI controllers")
        result = unreal.GenAIUtils.list_ai_controllers()
        return _parse_json_response(result)

    except Exception as e:
        log.log_error(f"Error listing AI controllers: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_ai_controller_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get AI Controller info for an actor"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenAIUtils.get_ai_controller_info(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_set_ai_controller(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set AI Controller class for an actor"""
    try:
        actor_name = command.get("actor_name")
        controller_class_path = command.get("controller_class_path")

        if not actor_name or not controller_class_path:
            return {"success": False, "error": "actor_name and controller_class_path required"}

        result = unreal.GenAIUtils.set_ai_controller(actor_name, controller_class_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# BEHAVIOR TREES
# ============================================

def handle_list_behavior_trees(command: Dict[str, Any]) -> Dict[str, Any]:
    """List all Behavior Tree assets"""
    try:
        directory_path = command.get("directory_path", "/Game")
        result = unreal.GenAIUtils.list_behavior_trees(directory_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_behavior_tree_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get Behavior Tree info"""
    try:
        behavior_tree_path = command.get("behavior_tree_path")
        if not behavior_tree_path:
            return {"success": False, "error": "behavior_tree_path required"}

        result = unreal.GenAIUtils.get_behavior_tree_info(behavior_tree_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_set_behavior_tree(command: Dict[str, Any]) -> Dict[str, Any]:
    """Assign Behavior Tree to AI Controller"""
    try:
        actor_name = command.get("actor_name")
        behavior_tree_path = command.get("behavior_tree_path")

        if not actor_name or not behavior_tree_path:
            return {"success": False, "error": "actor_name and behavior_tree_path required"}

        result = unreal.GenAIUtils.set_behavior_tree(actor_name, behavior_tree_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_run_bt(command: Dict[str, Any]) -> Dict[str, Any]:
    """Run Behavior Tree on AI (PIE only)"""
    try:
        actor_name = command.get("actor_name")
        behavior_tree_path = command.get("behavior_tree_path")

        if not actor_name or not behavior_tree_path:
            return {"success": False, "error": "actor_name and behavior_tree_path required"}

        result = unreal.GenAIUtils.run_behavior_tree(actor_name, behavior_tree_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_stop_bt(command: Dict[str, Any]) -> Dict[str, Any]:
    """Stop Behavior Tree on AI (PIE only)"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenAIUtils.stop_behavior_tree(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# BLACKBOARD
# ============================================

def handle_list_blackboards(command: Dict[str, Any]) -> Dict[str, Any]:
    """List all Blackboard assets"""
    try:
        directory_path = command.get("directory_path", "/Game")
        result = unreal.GenAIUtils.list_blackboards(directory_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_blackboard_keys(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get Blackboard keys"""
    try:
        blackboard_path = command.get("blackboard_path")
        if not blackboard_path:
            return {"success": False, "error": "blackboard_path required"}

        result = unreal.GenAIUtils.get_blackboard_keys(blackboard_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_bb_value(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get blackboard value (PIE only)"""
    try:
        actor_name = command.get("actor_name")
        key_name = command.get("key_name")

        if not actor_name or not key_name:
            return {"success": False, "error": "actor_name and key_name required"}

        result = unreal.GenAIUtils.get_blackboard_value(actor_name, key_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_set_bb_value(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set blackboard value (PIE only)"""
    try:
        actor_name = command.get("actor_name")
        key_name = command.get("key_name")
        value = command.get("value", "")

        if not actor_name or not key_name:
            return {"success": False, "error": "actor_name and key_name required"}

        result = unreal.GenAIUtils.set_blackboard_value(actor_name, key_name, str(value))
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# NAVIGATION
# ============================================

def handle_is_location_navigable(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if a location is navigable"""
    try:
        location = command.get("location", [0, 0, 0])
        loc = unreal.Vector(location[0], location[1], location[2])

        result = unreal.GenAIUtils.is_location_navigable(loc)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_find_nav_path(command: Dict[str, Any]) -> Dict[str, Any]:
    """Find path between two points"""
    try:
        start_location = command.get("start_location", [0, 0, 0])
        end_location = command.get("end_location", [0, 0, 0])

        start = unreal.Vector(start_location[0], start_location[1], start_location[2])
        end = unreal.Vector(end_location[0], end_location[1], end_location[2])

        result = unreal.GenAIUtils.find_path(start, end)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_random_navigable_point(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get random navigable point in radius"""
    try:
        origin = command.get("origin", [0, 0, 0])
        radius = command.get("radius", 1000.0)

        origin_vec = unreal.Vector(origin[0], origin[1], origin[2])

        result = unreal.GenAIUtils.get_random_navigable_point(origin_vec, radius)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_project_point_to_navigation(command: Dict[str, Any]) -> Dict[str, Any]:
    """Project point to navigation"""
    try:
        location = command.get("location", [0, 0, 0])
        query_extent = command.get("query_extent", [100, 100, 250])

        loc = unreal.Vector(location[0], location[1], location[2])
        extent = unreal.Vector(query_extent[0], query_extent[1], query_extent[2])

        result = unreal.GenAIUtils.project_point_to_navigation(loc, extent)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# AI MOVEMENT
# ============================================

def handle_ai_move_to_location(command: Dict[str, Any]) -> Dict[str, Any]:
    """Move AI to location (PIE only)"""
    try:
        actor_name = command.get("actor_name")
        target_location = command.get("target_location", [0, 0, 0])
        acceptance_radius = command.get("acceptance_radius", 50.0)

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        target = unreal.Vector(target_location[0], target_location[1], target_location[2])

        result = unreal.GenAIUtils.move_to_location(actor_name, target, acceptance_radius)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_ai_move_to_actor(command: Dict[str, Any]) -> Dict[str, Any]:
    """Move AI to actor (PIE only)"""
    try:
        actor_name = command.get("actor_name")
        target_actor_name = command.get("target_actor_name")
        acceptance_radius = command.get("acceptance_radius", 50.0)

        if not actor_name or not target_actor_name:
            return {"success": False, "error": "actor_name and target_actor_name required"}

        result = unreal.GenAIUtils.move_to_actor(actor_name, target_actor_name, acceptance_radius)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_stop_ai_movement(command: Dict[str, Any]) -> Dict[str, Any]:
    """Stop AI movement (PIE only)"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenAIUtils.stop_movement(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_movement_status(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get AI movement status (PIE only)"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenAIUtils.get_movement_status(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# AI PERCEPTION
# ============================================

def handle_get_perception_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get perception component info"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenAIUtils.get_perception_info(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_perceived_actors(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get currently perceived actors (PIE only)"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenAIUtils.get_perceived_actors(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_configure_sight_sense(command: Dict[str, Any]) -> Dict[str, Any]:
    """Configure sight sense"""
    try:
        actor_name = command.get("actor_name")
        sight_radius = command.get("sight_radius", 3000.0)
        lose_sight_radius = command.get("lose_sight_radius", 3500.0)
        peripheral_vision_angle = command.get("peripheral_vision_angle", 90.0)

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenAIUtils.configure_sight_sense(
            actor_name, sight_radius, lose_sight_radius, peripheral_vision_angle
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_configure_hearing_sense(command: Dict[str, Any]) -> Dict[str, Any]:
    """Configure hearing sense"""
    try:
        actor_name = command.get("actor_name")
        hearing_range = command.get("hearing_range", 3000.0)

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenAIUtils.configure_hearing_sense(actor_name, hearing_range)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# EQS (Environment Query System)
# ============================================

def handle_list_eqs_queries(command: Dict[str, Any]) -> Dict[str, Any]:
    """List EQS queries"""
    try:
        directory_path = command.get("directory_path", "/Game")
        result = unreal.GenAIUtils.list_eqs_queries(directory_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_run_eqs_query(command: Dict[str, Any]) -> Dict[str, Any]:
    """Run EQS query (PIE only)"""
    try:
        actor_name = command.get("actor_name")
        query_path = command.get("query_path")

        if not actor_name or not query_path:
            return {"success": False, "error": "actor_name and query_path required"}

        result = unreal.GenAIUtils.run_eqs_query(actor_name, query_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# SMART OBJECTS
# ============================================

def handle_list_smart_objects(command: Dict[str, Any]) -> Dict[str, Any]:
    """List Smart Objects in level"""
    try:
        result = unreal.GenAIUtils.list_smart_objects()
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_find_smart_objects(command: Dict[str, Any]) -> Dict[str, Any]:
    """Find Smart Objects matching criteria"""
    try:
        actor_name = command.get("actor_name")
        tag_query = command.get("tag_query", "")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenAIUtils.find_smart_objects(actor_name, tag_query)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# CROWD & AVOIDANCE
# ============================================

def handle_get_avoidance_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get avoidance info for actor"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenAIUtils.get_avoidance_info(actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_set_avoidance_settings(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set avoidance settings"""
    try:
        actor_name = command.get("actor_name")
        enable_avoidance = command.get("enable_avoidance", True)
        avoidance_weight = command.get("avoidance_weight", 0.5)

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        result = unreal.GenAIUtils.set_avoidance_settings(
            actor_name, enable_avoidance, avoidance_weight
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# COMMAND REGISTRY
# ============================================

BEHAVIOR_TREE_COMMANDS = {
    # AI Controller
    "list_ai_controllers": handle_list_ai_controllers,
    "get_ai_controller_info": handle_get_ai_controller_info,
    "set_ai_controller": handle_set_ai_controller,

    # Behavior Trees
    "list_behavior_trees": handle_list_behavior_trees,
    "get_behavior_tree_info": handle_get_behavior_tree_info,
    "set_behavior_tree": handle_set_behavior_tree,
    "run_bt": handle_run_bt,
    "stop_bt": handle_stop_bt,

    # Blackboard
    "list_blackboards": handle_list_blackboards,
    "get_blackboard_keys": handle_get_blackboard_keys,
    "get_bb_value": handle_get_bb_value,
    "set_bb_value": handle_set_bb_value,

    # Navigation
    "is_location_navigable": handle_is_location_navigable,
    "find_nav_path": handle_find_nav_path,
    "get_random_navigable_point": handle_get_random_navigable_point,
    "project_point_to_navigation": handle_project_point_to_navigation,

    # AI Movement
    "ai_move_to_location": handle_ai_move_to_location,
    "ai_move_to_actor": handle_ai_move_to_actor,
    "stop_ai_movement": handle_stop_ai_movement,
    "get_movement_status": handle_get_movement_status,

    # Perception
    "get_perception_info": handle_get_perception_info,
    "get_perceived_actors": handle_get_perceived_actors,
    "configure_sight_sense": handle_configure_sight_sense,
    "configure_hearing_sense": handle_configure_hearing_sense,

    # EQS
    "list_eqs_queries": handle_list_eqs_queries,
    "run_eqs_query": handle_run_eqs_query,

    # Smart Objects
    "list_smart_objects": handle_list_smart_objects,
    "find_smart_objects": handle_find_smart_objects,

    # Avoidance
    "get_avoidance_info": handle_get_avoidance_info,
    "set_avoidance_settings": handle_set_avoidance_settings,
}


def get_behavior_tree_commands():
    """Return all available behavior tree/AI commands"""
    return BEHAVIOR_TREE_COMMANDS
