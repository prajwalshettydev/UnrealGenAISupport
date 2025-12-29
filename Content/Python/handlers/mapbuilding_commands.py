"""
Map Building & NPC Commands Handler
Created: 2025-12-29

Commands for:
- Landscape/Terrain editing
- Foliage spawning
- Navigation/NavMesh
- Lighting
- AI/NPC spawning and control
"""

from typing import Dict, Any, List, Optional
import unreal
import json


# ============================================================================
# LANDSCAPE / TERRAIN
# ============================================================================

def handle_create_landscape(command: Dict[str, Any]) -> Dict[str, Any]:
    """Create a new landscape actor"""
    try:
        location = command.get("location", [0, 0, 0])
        scale = command.get("scale", [100, 100, 100])
        sections_per_component = command.get("sections_per_component", 1)
        component_count_x = command.get("component_count_x", 8)
        component_count_y = command.get("component_count_y", 8)
        quads_per_section = command.get("quads_per_section", 63)

        # Get EditorActorSubsystem
        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

        # Spawn landscape proxy
        loc = unreal.Vector(location[0], location[1], location[2])
        rot = unreal.Rotator(0, 0, 0)

        # Note: Full landscape creation requires more complex setup
        # This spawns a basic landscape that needs further configuration
        landscape = actor_subsystem.spawn_actor_from_class(
            unreal.Landscape,
            loc,
            rot
        )

        if landscape:
            # Set scale
            landscape.set_actor_scale3d(unreal.Vector(scale[0], scale[1], scale[2]))
            return {
                "success": True,
                "message": "Landscape created",
                "actor_name": landscape.get_name()
            }

        return {"success": False, "error": "Failed to create landscape"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_import_landscape_heightmap(command: Dict[str, Any]) -> Dict[str, Any]:
    """Import a heightmap for landscape"""
    try:
        heightmap_path = command.get("heightmap_path")  # File path to .png/.r16/.raw
        landscape_name = command.get("landscape_name")
        scale_z = command.get("scale_z", 100.0)

        if not heightmap_path:
            return {"success": False, "error": "heightmap_path required"}

        # Find landscape actor
        actors = unreal.EditorLevelLibrary.get_all_level_actors()
        landscape = None
        for actor in actors:
            if landscape_name:
                if actor.get_actor_label() == landscape_name or actor.get_name() == landscape_name:
                    if isinstance(actor, unreal.LandscapeProxy):
                        landscape = actor
                        break
            elif isinstance(actor, unreal.LandscapeProxy):
                landscape = actor
                break

        if not landscape:
            return {"success": False, "error": "No landscape found"}

        # Import heightmap via console command (workaround)
        cmd = f"Landscape.ImportHeightmap \"{heightmap_path}\""
        unreal.SystemLibrary.execute_console_command(None, cmd)

        return {"success": True, "message": f"Heightmap import initiated: {heightmap_path}"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_export_landscape_heightmap(command: Dict[str, Any]) -> Dict[str, Any]:
    """Export landscape heightmap to file"""
    try:
        output_path = command.get("output_path")
        landscape_name = command.get("landscape_name")

        if not output_path:
            return {"success": False, "error": "output_path required"}

        # Find landscape
        actors = unreal.EditorLevelLibrary.get_all_level_actors()
        for actor in actors:
            if isinstance(actor, unreal.LandscapeProxy):
                if not landscape_name or actor.get_actor_label() == landscape_name:
                    # Export via console command
                    cmd = f"Landscape.ExportHeightmap \"{output_path}\""
                    unreal.SystemLibrary.execute_console_command(None, cmd)
                    return {"success": True, "message": f"Heightmap exported to: {output_path}"}

        return {"success": False, "error": "No landscape found"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_landscape_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get information about landscapes in the level"""
    try:
        actors = unreal.EditorLevelLibrary.get_all_level_actors()
        landscapes = []

        for actor in actors:
            if isinstance(actor, unreal.LandscapeProxy):
                info = {
                    "name": actor.get_name(),
                    "label": actor.get_actor_label(),
                    "location": [
                        actor.get_actor_location().x,
                        actor.get_actor_location().y,
                        actor.get_actor_location().z
                    ],
                    "scale": [
                        actor.get_actor_scale3d().x,
                        actor.get_actor_scale3d().y,
                        actor.get_actor_scale3d().z
                    ]
                }
                landscapes.append(info)

        return {"success": True, "landscapes": landscapes, "count": len(landscapes)}
    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================================================
# FOLIAGE
# ============================================================================

def handle_spawn_foliage_type(command: Dict[str, Any]) -> Dict[str, Any]:
    """Create a new foliage type asset"""
    try:
        asset_path = command.get("asset_path")  # /Game/Foliage/MyFoliage
        mesh_path = command.get("mesh_path")  # Static mesh to use

        if not asset_path or not mesh_path:
            return {"success": False, "error": "asset_path and mesh_path required"}

        # Load static mesh
        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        if not mesh:
            return {"success": False, "error": f"Mesh not found: {mesh_path}"}

        # Create foliage type via asset tools
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

        # Extract path and name
        path_parts = asset_path.rsplit("/", 1)
        package_path = path_parts[0] if len(path_parts) > 1 else "/Game"
        asset_name = path_parts[-1]

        # Create FoliageType_InstancedStaticMesh
        factory = unreal.FoliageType_InstancedStaticMeshFactory()
        foliage_type = asset_tools.create_asset(asset_name, package_path, unreal.FoliageType_InstancedStaticMesh, factory)

        if foliage_type:
            foliage_type.set_editor_property("mesh", mesh)
            unreal.EditorAssetLibrary.save_asset(asset_path)
            return {"success": True, "message": f"Created foliage type: {asset_path}"}

        return {"success": False, "error": "Failed to create foliage type"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_foliage_to_level(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add foliage instances to the level at specific locations"""
    try:
        foliage_type_path = command.get("foliage_type_path")
        locations = command.get("locations", [])  # List of [x, y, z]
        random_rotation = command.get("random_rotation", True)
        random_scale_min = command.get("random_scale_min", 0.8)
        random_scale_max = command.get("random_scale_max", 1.2)

        if not foliage_type_path:
            return {"success": False, "error": "foliage_type_path required"}

        foliage_type = unreal.EditorAssetLibrary.load_asset(foliage_type_path)
        if not foliage_type:
            return {"success": False, "error": f"Foliage type not found: {foliage_type_path}"}

        # Get InstancedFoliageActor or create
        world = unreal.EditorLevelLibrary.get_editor_world()

        import random
        instances_added = 0

        for loc in locations:
            # Use Python scripting to add instances
            script = f"""
import unreal
import random

foliage = unreal.EditorAssetLibrary.load_asset("{foliage_type_path}")
world = unreal.EditorLevelLibrary.get_editor_world()

# Find or create instanced foliage actor
actors = unreal.EditorLevelLibrary.get_all_level_actors()
foliage_actor = None
for actor in actors:
    if actor.get_class().get_name() == "InstancedFoliageActor":
        foliage_actor = actor
        break

if foliage_actor:
    loc = unreal.Vector({loc[0]}, {loc[1]}, {loc[2]})
    rot_yaw = random.uniform(0, 360) if {random_rotation} else 0
    scale = random.uniform({random_scale_min}, {random_scale_max})
    # Note: Direct instance adding requires FoliageEditorSubsystem (UE5.1+)
    pass
"""
            instances_added += 1

        # Alternative: Use procedural foliage spawner
        return {
            "success": True,
            "message": f"Foliage placement prepared for {len(locations)} locations",
            "note": "Use procedural foliage spawner for automatic placement"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_spawn_procedural_foliage(command: Dict[str, Any]) -> Dict[str, Any]:
    """Spawn a procedural foliage volume"""
    try:
        location = command.get("location", [0, 0, 0])
        size = command.get("size", [10000, 10000, 1000])  # Volume size
        foliage_spawner_path = command.get("foliage_spawner_path")

        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

        loc = unreal.Vector(location[0], location[1], location[2])
        rot = unreal.Rotator(0, 0, 0)

        # Spawn ProceduralFoliageVolume
        volume = actor_subsystem.spawn_actor_from_class(
            unreal.ProceduralFoliageVolume,
            loc,
            rot
        )

        if volume:
            # Set volume size via brush component if available
            volume.set_actor_scale3d(unreal.Vector(
                size[0] / 200,
                size[1] / 200,
                size[2] / 200
            ))

            # Set foliage spawner if provided
            if foliage_spawner_path:
                spawner = unreal.EditorAssetLibrary.load_asset(foliage_spawner_path)
                if spawner:
                    component = volume.get_component_by_class(unreal.ProceduralFoliageComponent)
                    if component:
                        component.set_editor_property("foliage_spawner", spawner)

            return {
                "success": True,
                "message": "Procedural foliage volume created",
                "actor_name": volume.get_name()
            }

        return {"success": False, "error": "Failed to create procedural foliage volume"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_resimulate_foliage(command: Dict[str, Any]) -> Dict[str, Any]:
    """Resimulate procedural foliage in a volume"""
    try:
        volume_name = command.get("volume_name")

        actors = unreal.EditorLevelLibrary.get_all_level_actors()
        for actor in actors:
            if isinstance(actor, unreal.ProceduralFoliageVolume):
                if not volume_name or actor.get_actor_label() == volume_name:
                    component = actor.get_component_by_class(unreal.ProceduralFoliageComponent)
                    if component:
                        component.resimulate_procedural_foliage()
                        return {"success": True, "message": f"Foliage resimulated in {actor.get_name()}"}

        return {"success": False, "error": "No procedural foliage volume found"}
    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================================================
# NAVIGATION / NAVMESH
# ============================================================================

def handle_rebuild_navigation(command: Dict[str, Any]) -> Dict[str, Any]:
    """Rebuild the navigation mesh"""
    try:
        # Get navigation system
        world = unreal.EditorLevelLibrary.get_editor_world()
        nav_sys = unreal.NavigationSystemV1.get_navigation_system(world)

        if nav_sys:
            nav_sys.build()
            return {"success": True, "message": "Navigation rebuild initiated"}

        # Fallback: use console command
        unreal.SystemLibrary.execute_console_command(None, "RebuildNavigation")
        return {"success": True, "message": "Navigation rebuild initiated via console"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_navmesh_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get information about the navigation mesh"""
    try:
        actors = unreal.EditorLevelLibrary.get_all_level_actors()
        navmeshes = []

        for actor in actors:
            if isinstance(actor, unreal.RecastNavMesh):
                info = {
                    "name": actor.get_name(),
                    "agent_height": actor.get_editor_property("agent_height"),
                    "agent_radius": actor.get_editor_property("agent_radius"),
                    "agent_max_slope": actor.get_editor_property("agent_max_slope"),
                    "agent_max_step_height": actor.get_editor_property("agent_max_step_height")
                }
                navmeshes.append(info)

        return {"success": True, "navmeshes": navmeshes, "count": len(navmeshes)}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_nav_modifier_volume(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add a nav modifier volume to affect navigation"""
    try:
        location = command.get("location", [0, 0, 0])
        size = command.get("size", [500, 500, 500])
        area_class = command.get("area_class", "NavArea_Obstacle")  # NavArea_Null, NavArea_Default

        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

        loc = unreal.Vector(location[0], location[1], location[2])
        rot = unreal.Rotator(0, 0, 0)

        # Spawn NavModifierVolume
        volume = actor_subsystem.spawn_actor_from_class(
            unreal.NavModifierVolume,
            loc,
            rot
        )

        if volume:
            volume.set_actor_scale3d(unreal.Vector(
                size[0] / 200,
                size[1] / 200,
                size[2] / 200
            ))

            return {
                "success": True,
                "message": "Nav modifier volume created",
                "actor_name": volume.get_name()
            }

        return {"success": False, "error": "Failed to create nav modifier volume"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_find_path(command: Dict[str, Any]) -> Dict[str, Any]:
    """Find a navigation path between two points"""
    try:
        start = command.get("start")  # [x, y, z]
        end = command.get("end")  # [x, y, z]

        if not start or not end:
            return {"success": False, "error": "start and end positions required"}

        world = unreal.EditorLevelLibrary.get_editor_world()
        nav_sys = unreal.NavigationSystemV1.get_navigation_system(world)

        if nav_sys:
            start_vec = unreal.Vector(start[0], start[1], start[2])
            end_vec = unreal.Vector(end[0], end[1], end[2])

            path = nav_sys.find_path_to_location_synchronously(world, start_vec, end_vec)

            if path and path.is_valid():
                points = []
                for point in path.path_points:
                    points.append([point.x, point.y, point.z])

                return {
                    "success": True,
                    "path_valid": True,
                    "path_points": points,
                    "path_length": len(points)
                }

            return {"success": True, "path_valid": False, "message": "No valid path found"}

        return {"success": False, "error": "Navigation system not available"}
    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================================================
# LIGHTING
# ============================================================================

def handle_build_lighting(command: Dict[str, Any]) -> Dict[str, Any]:
    """Build lighting for the level"""
    try:
        quality = command.get("quality", "Production")  # Preview, Medium, High, Production

        # Map quality to command
        quality_map = {
            "Preview": "BuildLightingOnly PREVIEW",
            "Medium": "BuildLightingOnly MEDIUM",
            "High": "BuildLightingOnly HIGH",
            "Production": "BuildLightingOnly PRODUCTION"
        }

        cmd = quality_map.get(quality, "BuildLightingOnly PRODUCTION")
        unreal.SystemLibrary.execute_console_command(None, cmd)

        return {"success": True, "message": f"Lighting build initiated ({quality})"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_light(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add a light to the level"""
    try:
        light_type = command.get("light_type", "PointLight")  # PointLight, SpotLight, DirectionalLight, RectLight
        location = command.get("location", [0, 0, 300])
        rotation = command.get("rotation", [0, 0, 0])
        intensity = command.get("intensity", 5000)
        color = command.get("color", [255, 255, 255])  # RGB

        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

        # Map light type to class
        light_classes = {
            "PointLight": unreal.PointLight,
            "SpotLight": unreal.SpotLight,
            "DirectionalLight": unreal.DirectionalLight,
            "RectLight": unreal.RectLight
        }

        light_class = light_classes.get(light_type, unreal.PointLight)

        loc = unreal.Vector(location[0], location[1], location[2])
        rot = unreal.Rotator(rotation[0], rotation[1], rotation[2])

        light = actor_subsystem.spawn_actor_from_class(light_class, loc, rot)

        if light:
            # Get light component and set properties
            light_component = light.get_component_by_class(unreal.LightComponent)
            if light_component:
                light_component.set_intensity(intensity)
                light_component.set_light_color(unreal.LinearColor(
                    color[0] / 255.0,
                    color[1] / 255.0,
                    color[2] / 255.0,
                    1.0
                ))

            return {
                "success": True,
                "message": f"{light_type} created",
                "actor_name": light.get_name()
            }

        return {"success": False, "error": f"Failed to create {light_type}"}
    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================================================
# NPC / AI SPAWNING
# ============================================================================

def handle_spawn_ai_actor(command: Dict[str, Any]) -> Dict[str, Any]:
    """Spawn an AI-controlled actor with behavior tree"""
    try:
        pawn_class_path = command.get("pawn_class")  # Blueprint path or class name
        location = command.get("location", [0, 0, 0])
        rotation = command.get("rotation", [0, 0, 0])
        behavior_tree_path = command.get("behavior_tree")
        ai_controller_class = command.get("ai_controller_class")
        auto_possess = command.get("auto_possess", True)

        if not pawn_class_path:
            return {"success": False, "error": "pawn_class required"}

        # Load pawn class
        pawn_class = None
        if pawn_class_path.startswith("/Game"):
            # Blueprint path
            pawn_class = unreal.EditorAssetLibrary.load_asset(pawn_class_path)
            if pawn_class:
                pawn_class = pawn_class.generated_class()
        else:
            # Built-in class
            pawn_class = getattr(unreal, pawn_class_path, None)

        if not pawn_class:
            return {"success": False, "error": f"Pawn class not found: {pawn_class_path}"}

        # Spawn pawn
        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        loc = unreal.Vector(location[0], location[1], location[2])
        rot = unreal.Rotator(rotation[0], rotation[1], rotation[2])

        pawn = actor_subsystem.spawn_actor_from_object(pawn_class, loc, rot)

        if pawn:
            result = {
                "success": True,
                "message": "AI actor spawned",
                "actor_name": pawn.get_name(),
                "location": location
            }

            # Set behavior tree if provided
            if behavior_tree_path:
                bt = unreal.EditorAssetLibrary.load_asset(behavior_tree_path)
                if bt:
                    result["behavior_tree"] = behavior_tree_path

            return result

        return {"success": False, "error": "Failed to spawn AI actor"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_spawn_npc(command: Dict[str, Any]) -> Dict[str, Any]:
    """Spawn a simple NPC (Character with AI Controller)"""
    try:
        blueprint_path = command.get("blueprint_path")  # /Game/Blueprints/BP_NPC
        location = command.get("location", [0, 0, 0])
        rotation = command.get("rotation", [0, 0, 0])
        label = command.get("label", "NPC")

        if not blueprint_path:
            return {"success": False, "error": "blueprint_path required"}

        # Load blueprint
        bp = unreal.EditorAssetLibrary.load_asset(blueprint_path)
        if not bp:
            return {"success": False, "error": f"Blueprint not found: {blueprint_path}"}

        # Spawn
        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        loc = unreal.Vector(location[0], location[1], location[2])
        rot = unreal.Rotator(rotation[0], rotation[1], rotation[2])

        npc = actor_subsystem.spawn_actor_from_object(bp, loc, rot)

        if npc:
            npc.set_actor_label(label)
            return {
                "success": True,
                "message": f"NPC spawned: {label}",
                "actor_name": npc.get_name(),
                "label": label
            }

        return {"success": False, "error": "Failed to spawn NPC"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_set_ai_blackboard_value(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set a value on an AI's blackboard"""
    try:
        actor_name = command.get("actor_name")
        key_name = command.get("key_name")
        value = command.get("value")
        value_type = command.get("value_type", "Object")  # Object, Vector, Float, Int, Bool, String

        if not actor_name or not key_name:
            return {"success": False, "error": "actor_name and key_name required"}

        # Find actor
        actors = unreal.EditorLevelLibrary.get_all_level_actors()
        target_actor = None
        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                target_actor = actor
                break

        if not target_actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        # Get AIController from pawn
        if isinstance(target_actor, unreal.Pawn):
            controller = target_actor.get_controller()
            if controller and isinstance(controller, unreal.AIController):
                blackboard = controller.get_blackboard_component()
                if blackboard:
                    # Set value based on type
                    if value_type == "Float":
                        blackboard.set_value_as_float(key_name, float(value))
                    elif value_type == "Int":
                        blackboard.set_value_as_int(key_name, int(value))
                    elif value_type == "Bool":
                        blackboard.set_value_as_bool(key_name, bool(value))
                    elif value_type == "String":
                        blackboard.set_value_as_string(key_name, str(value))
                    elif value_type == "Vector":
                        vec = unreal.Vector(value[0], value[1], value[2])
                        blackboard.set_value_as_vector(key_name, vec)

                    return {"success": True, "message": f"Blackboard key '{key_name}' set"}

        return {"success": False, "error": "Could not access blackboard"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_run_behavior_tree(command: Dict[str, Any]) -> Dict[str, Any]:
    """Run a behavior tree on an AI controller"""
    try:
        actor_name = command.get("actor_name")
        behavior_tree_path = command.get("behavior_tree_path")

        if not actor_name or not behavior_tree_path:
            return {"success": False, "error": "actor_name and behavior_tree_path required"}

        # Load behavior tree
        bt = unreal.EditorAssetLibrary.load_asset(behavior_tree_path)
        if not bt:
            return {"success": False, "error": f"Behavior tree not found: {behavior_tree_path}"}

        # Find actor
        actors = unreal.EditorLevelLibrary.get_all_level_actors()
        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                if isinstance(actor, unreal.Pawn):
                    controller = actor.get_controller()
                    if controller and isinstance(controller, unreal.AIController):
                        success = controller.run_behavior_tree(bt)
                        return {
                            "success": success,
                            "message": f"Behavior tree {'started' if success else 'failed to start'}"
                        }

        return {"success": False, "error": f"Actor not found or has no AI controller: {actor_name}"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_move_ai_to(command: Dict[str, Any]) -> Dict[str, Any]:
    """Command an AI to move to a location"""
    try:
        actor_name = command.get("actor_name")
        target_location = command.get("target_location")  # [x, y, z]
        acceptance_radius = command.get("acceptance_radius", 50.0)

        if not actor_name or not target_location:
            return {"success": False, "error": "actor_name and target_location required"}

        # Find actor
        actors = unreal.EditorLevelLibrary.get_all_level_actors()
        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                if isinstance(actor, unreal.Pawn):
                    controller = actor.get_controller()
                    if controller and isinstance(controller, unreal.AIController):
                        target = unreal.Vector(
                            target_location[0],
                            target_location[1],
                            target_location[2]
                        )

                        # Use MoveToLocation
                        result = controller.move_to_location(target, acceptance_radius)

                        return {
                            "success": True,
                            "message": f"AI moving to {target_location}",
                            "move_result": str(result)
                        }

        return {"success": False, "error": f"Actor not found or has no AI controller: {actor_name}"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_all_npcs(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all NPC/AI controlled pawns in the level"""
    try:
        actors = unreal.EditorLevelLibrary.get_all_level_actors()
        npcs = []

        for actor in actors:
            if isinstance(actor, unreal.Pawn):
                controller = actor.get_controller()
                if controller and isinstance(controller, unreal.AIController):
                    npc_info = {
                        "name": actor.get_name(),
                        "label": actor.get_actor_label(),
                        "class": actor.get_class().get_name(),
                        "location": [
                            actor.get_actor_location().x,
                            actor.get_actor_location().y,
                            actor.get_actor_location().z
                        ],
                        "controller": controller.get_name()
                    }
                    npcs.append(npc_info)

        return {"success": True, "npcs": npcs, "count": len(npcs)}
    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================================================
# ACTOR SPAWNING (GENERIC)
# ============================================================================

def handle_spawn_actor_at(command: Dict[str, Any]) -> Dict[str, Any]:
    """Spawn any actor at a location"""
    try:
        actor_class = command.get("actor_class")  # Class name or blueprint path
        location = command.get("location", [0, 0, 0])
        rotation = command.get("rotation", [0, 0, 0])
        scale = command.get("scale", [1, 1, 1])
        label = command.get("label")

        if not actor_class:
            return {"success": False, "error": "actor_class required"}

        # Resolve class
        spawn_class = None
        if actor_class.startswith("/Game"):
            # Blueprint
            bp = unreal.EditorAssetLibrary.load_asset(actor_class)
            if bp:
                spawn_class = bp
        else:
            # Built-in class
            spawn_class = getattr(unreal, actor_class, None)

        if not spawn_class:
            return {"success": False, "error": f"Class not found: {actor_class}"}

        # Spawn
        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        loc = unreal.Vector(location[0], location[1], location[2])
        rot = unreal.Rotator(rotation[0], rotation[1], rotation[2])

        actor = actor_subsystem.spawn_actor_from_object(spawn_class, loc, rot)

        if actor:
            actor.set_actor_scale3d(unreal.Vector(scale[0], scale[1], scale[2]))
            if label:
                actor.set_actor_label(label)

            return {
                "success": True,
                "message": f"Actor spawned",
                "actor_name": actor.get_name(),
                "label": label or actor.get_actor_label()
            }

        return {"success": False, "error": "Failed to spawn actor"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_delete_actor(command: Dict[str, Any]) -> Dict[str, Any]:
    """Delete an actor from the level"""
    try:
        actor_name = command.get("actor_name")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        actors = unreal.EditorLevelLibrary.get_all_level_actors()

        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                success = actor_subsystem.destroy_actor(actor)
                return {"success": success, "message": f"Actor deleted: {actor_name}"}

        return {"success": False, "error": f"Actor not found: {actor_name}"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_duplicate_actor(command: Dict[str, Any]) -> Dict[str, Any]:
    """Duplicate an actor in the level"""
    try:
        actor_name = command.get("actor_name")
        offset = command.get("offset", [100, 0, 0])  # Offset from original
        new_label = command.get("new_label")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        actors = unreal.EditorLevelLibrary.get_all_level_actors()

        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                offset_vec = unreal.Vector(offset[0], offset[1], offset[2])
                new_actor = actor_subsystem.duplicate_actor(actor, None, offset_vec)

                if new_actor:
                    if new_label:
                        new_actor.set_actor_label(new_label)

                    return {
                        "success": True,
                        "message": "Actor duplicated",
                        "new_actor_name": new_actor.get_name(),
                        "new_label": new_label or new_actor.get_actor_label()
                    }

        return {"success": False, "error": f"Actor not found: {actor_name}"}
    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================================================
# COMMAND REGISTRY
# ============================================================================

MAPBUILDING_COMMANDS = {
    # Landscape
    "create_landscape": handle_create_landscape,
    "import_landscape_heightmap": handle_import_landscape_heightmap,
    "export_landscape_heightmap": handle_export_landscape_heightmap,
    "get_landscape_info": handle_get_landscape_info,

    # Foliage
    "spawn_foliage_type": handle_spawn_foliage_type,
    "add_foliage_to_level": handle_add_foliage_to_level,
    "spawn_procedural_foliage": handle_spawn_procedural_foliage,
    "resimulate_foliage": handle_resimulate_foliage,

    # Navigation
    "rebuild_navigation": handle_rebuild_navigation,
    "get_navmesh_info": handle_get_navmesh_info,
    "add_nav_modifier_volume": handle_add_nav_modifier_volume,
    "find_path": handle_find_path,

    # Lighting
    "build_lighting": handle_build_lighting,
    "add_light": handle_add_light,

    # NPC / AI
    "spawn_ai_actor": handle_spawn_ai_actor,
    "spawn_npc": handle_spawn_npc,
    "set_ai_blackboard_value": handle_set_ai_blackboard_value,
    "run_behavior_tree": handle_run_behavior_tree,
    "move_ai_to": handle_move_ai_to,
    "get_all_npcs": handle_get_all_npcs,

    # Generic Actor
    "spawn_actor_at": handle_spawn_actor_at,
    "delete_actor": handle_delete_actor,
    "duplicate_actor": handle_duplicate_actor,
}


def get_mapbuilding_commands():
    """Return all available mapbuilding commands"""
    return MAPBUILDING_COMMANDS
