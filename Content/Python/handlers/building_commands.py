# Building Placement Commands - Python Handler for BuildingPlacementLibrary
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# TERRAIN ANALYSIS
# ============================================

def handle_find_flat_area(command: Dict[str, Any]) -> Dict[str, Any]:
    """Find a flat area suitable for building placement"""
    try:
        center_x = command.get("x", 0.0)
        center_y = command.get("y", 0.0)
        center_z = command.get("z", 0.0)
        search_radius = command.get("radius", 500.0)
        max_slope = command.get("max_slope", 15.0)
        num_samples = command.get("num_samples", 9)

        center = unreal.Vector(center_x, center_y, center_z)

        result = unreal.BuildingPlacementLibrary.find_flat_area(
            unreal.EditorLevelLibrary.get_editor_world(),
            center,
            search_radius,
            max_slope,
            num_samples
        )

        return {
            "success": True,
            "found": result.b_found_flat_area,
            "location": {
                "x": result.location.x,
                "y": result.location.y,
                "z": result.location.z
            },
            "ground_normal": {
                "x": result.ground_normal.x,
                "y": result.ground_normal.y,
                "z": result.ground_normal.z
            },
            "average_height": result.average_height,
            "max_slope_angle": result.max_slope_angle,
            "height_variation": result.height_variation
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_check_building_footprint(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if a building footprint can be placed at location"""
    try:
        loc_x = command.get("x", 0.0)
        loc_y = command.get("y", 0.0)
        loc_z = command.get("z", 0.0)
        size_x = command.get("size_x", 500.0)
        size_y = command.get("size_y", 500.0)
        size_z = command.get("size_z", 300.0)
        rotation_yaw = command.get("rotation", 0.0)
        max_slope = command.get("max_slope", 15.0)

        location = unreal.Vector(loc_x, loc_y, loc_z)
        footprint_size = unreal.Vector(size_x, size_y, size_z)
        rotation = unreal.Rotator(0, rotation_yaw, 0)

        result = unreal.BuildingPlacementLibrary.check_building_footprint(
            unreal.EditorLevelLibrary.get_editor_world(),
            location,
            footprint_size,
            rotation,
            max_slope,
            []  # No actors to ignore
        )

        blocking_actors = []
        for actor in result.blocking_actors:
            blocking_actors.append(actor.get_actor_label())

        return {
            "success": True,
            "can_place": result.b_can_place,
            "has_collision": result.b_has_collision,
            "too_steep": result.b_too_steep,
            "in_water": result.b_in_water,
            "blocking_actors": blocking_actors,
            "fail_reason": result.fail_reason
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_ground_height(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get ground height at a specific location"""
    try:
        x = command.get("x", 0.0)
        y = command.get("y", 0.0)

        location = unreal.Vector(x, y, 10000.0)  # Start high

        height, normal = unreal.BuildingPlacementLibrary.get_ground_height(
            unreal.EditorLevelLibrary.get_editor_world(),
            location
        )

        return {
            "success": True,
            "height": height,
            "normal": {
                "x": normal.x,
                "y": normal.y,
                "z": normal.z
            }
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_ground_heights_grid(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get multiple ground heights in a grid pattern"""
    try:
        center_x = command.get("x", 0.0)
        center_y = command.get("y", 0.0)
        center_z = command.get("z", 0.0)
        grid_width = command.get("grid_width", 100.0)
        grid_depth = command.get("grid_depth", 100.0)
        resolution = command.get("resolution", 5)

        center = unreal.Vector(center_x, center_y, center_z)
        grid_size = unreal.Vector2D(grid_width, grid_depth)

        heights, avg_height, height_var = unreal.BuildingPlacementLibrary.get_ground_heights_grid(
            unreal.EditorLevelLibrary.get_editor_world(),
            center,
            grid_size,
            resolution
        )

        return {
            "success": True,
            "heights": list(heights),
            "average_height": avg_height,
            "height_variation": height_var
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# BUILDING PLACEMENT
# ============================================

def handle_place_building(command: Dict[str, Any]) -> Dict[str, Any]:
    """Place a building on terrain with automatic ground alignment"""
    try:
        blueprint_path = command.get("blueprint_path", "")
        x = command.get("x", 0.0)
        y = command.get("y", 0.0)
        z = command.get("z", 0.0)
        rotation = command.get("rotation", 0.0)
        align_to_slope = command.get("align_to_slope", False)

        if not blueprint_path:
            return {"success": False, "error": "blueprint_path required"}

        # Load building class
        building_class = unreal.EditorAssetLibrary.load_blueprint_class(blueprint_path)
        if not building_class:
            return {"success": False, "error": f"Could not load blueprint: {blueprint_path}"}

        location = unreal.Vector(x, y, z)
        rot = unreal.Rotator(0, rotation, 0)

        success, spawned_building = unreal.BuildingPlacementLibrary.place_building_on_terrain(
            unreal.EditorLevelLibrary.get_editor_world(),
            building_class,
            location,
            rot,
            align_to_slope
        )

        if success and spawned_building:
            final_loc = spawned_building.get_actor_location()
            return {
                "success": True,
                "actor_name": spawned_building.get_actor_label(),
                "location": {
                    "x": final_loc.x,
                    "y": final_loc.y,
                    "z": final_loc.z
                }
            }
        else:
            return {"success": False, "error": "Failed to place building"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# UTILITY
# ============================================

def handle_find_nearby_buildings(command: Dict[str, Any]) -> Dict[str, Any]:
    """Find nearby buildings/structures"""
    try:
        x = command.get("x", 0.0)
        y = command.get("y", 0.0)
        z = command.get("z", 0.0)
        radius = command.get("radius", 1000.0)
        tag = command.get("tag", "")

        location = unreal.Vector(x, y, z)
        tag_name = unreal.Name(tag) if tag else unreal.Name()

        buildings = unreal.BuildingPlacementLibrary.find_nearby_buildings(
            unreal.EditorLevelLibrary.get_editor_world(),
            location,
            radius,
            tag_name
        )

        building_list = []
        for b in buildings:
            loc = b.get_actor_location()
            building_list.append({
                "name": b.get_actor_label(),
                "class": b.get_class().get_name(),
                "location": {"x": loc.x, "y": loc.y, "z": loc.z}
            })

        return {"success": True, "buildings": building_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_is_in_water(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if location is in water"""
    try:
        x = command.get("x", 0.0)
        y = command.get("y", 0.0)
        z = command.get("z", 0.0)

        location = unreal.Vector(x, y, z)

        in_water = unreal.BuildingPlacementLibrary.is_in_water(
            unreal.EditorLevelLibrary.get_editor_world(),
            location
        )

        return {"success": True, "in_water": in_water}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

BUILDING_COMMANDS = {
    # Terrain Analysis
    "building_find_flat_area": handle_find_flat_area,
    "building_check_footprint": handle_check_building_footprint,
    "building_get_ground_height": handle_get_ground_height,
    "building_get_ground_heights_grid": handle_get_ground_heights_grid,

    # Placement
    "building_place": handle_place_building,

    # Utility
    "building_find_nearby": handle_find_nearby_buildings,
    "building_is_in_water": handle_is_in_water,
}
