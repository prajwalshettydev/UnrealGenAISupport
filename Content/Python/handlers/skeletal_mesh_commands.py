"""
Skeletal Mesh Commands Handler
Fuer animierte Meshes (NPCs, Charaktere)

Commands:
- get_skeleton_bones: Alle Bones eines Skeletts auflisten
- get_animation_sequences: Alle Animationen finden
- set_actor_animation: Animation auf Actor setzen
- play_animation: Animation abspielen
- stop_animation: Animation stoppen
- get_actor_skeleton_info: Skeleton Infos eines Actors
"""

import unreal
from typing import Dict, Any, List
from utils import logging as log


def handle_get_skeleton_bones(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Get all bones of a skeleton

    Args:
        command: Dictionary with:
            - skeleton_path: Path to Skeleton asset
    """
    try:
        skeleton_path = command.get("skeleton_path")

        if not skeleton_path:
            return {"success": False, "error": "skeleton_path required"}

        log.log_command("get_skeleton_bones", f"Getting bones from {skeleton_path}")

        skeleton = unreal.EditorAssetLibrary.load_asset(skeleton_path)
        if not skeleton:
            return {"success": False, "error": f"Skeleton not found: {skeleton_path}"}

        # Get bone names
        bone_names = []
        # The skeleton has a get_bone_tree method or we iterate
        num_bones = skeleton.get_num_bones() if hasattr(skeleton, 'get_num_bones') else 0

        for i in range(num_bones):
            bone_name = skeleton.get_bone_name(i)
            bone_names.append(str(bone_name))

        log.log_result("get_skeleton_bones", True, f"Found {len(bone_names)} bones")
        return {
            "success": True,
            "skeleton_path": skeleton_path,
            "bones": bone_names,
            "count": len(bone_names)
        }

    except Exception as e:
        log.log_error(f"Error getting skeleton bones: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_animation_sequences(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Find all animation sequences in a path

    Args:
        command: Dictionary with:
            - path: Search path (default: /Game)
            - skeleton_path: Optional filter by skeleton
    """
    try:
        search_path = command.get("path", "/Game")
        skeleton_filter = command.get("skeleton_path", None)

        log.log_command("get_animation_sequences", f"Finding animations in {search_path}")

        registry = unreal.AssetRegistryHelpers.get_asset_registry()

        filter = unreal.ARFilter()
        filter.class_names = ["AnimSequence"]
        filter.package_paths = [search_path]
        filter.recursive_paths = True

        assets = registry.get_assets(filter)

        anim_list = []
        for asset in assets:
            anim_info = {
                "name": str(asset.asset_name),
                "path": str(asset.package_name)
            }

            # If skeleton filter, check compatibility
            if skeleton_filter:
                try:
                    anim = unreal.EditorAssetLibrary.load_asset(str(asset.package_name))
                    if anim and hasattr(anim, 'get_skeleton'):
                        skel = anim.get_skeleton()
                        if skel and str(skel.get_path_name()) == skeleton_filter:
                            anim_list.append(anim_info)
                except:
                    pass
            else:
                anim_list.append(anim_info)

        log.log_result("get_animation_sequences", True, f"Found {len(anim_list)} animations")
        return {
            "success": True,
            "path": search_path,
            "animations": anim_list[:50],  # Limit
            "count": len(anim_list)
        }

    except Exception as e:
        log.log_error(f"Error finding animations: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_set_actor_animation(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Set animation on an actor's skeletal mesh

    Args:
        command: Dictionary with:
            - actor_name: Name/label of the actor
            - animation_path: Path to AnimSequence
            - loop: Loop animation (default: True)
    """
    try:
        actor_name = command.get("actor_name")
        animation_path = command.get("animation_path")
        loop = command.get("loop", True)

        if not actor_name or not animation_path:
            return {"success": False, "error": "actor_name and animation_path required"}

        log.log_command("set_actor_animation", f"Setting animation on {actor_name}")

        # Find actor
        actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        actors = actor_sub.get_all_level_actors()

        target_actor = None
        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                target_actor = actor
                break

        if not target_actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        # Load animation
        anim = unreal.EditorAssetLibrary.load_asset(animation_path)
        if not anim:
            return {"success": False, "error": f"Animation not found: {animation_path}"}

        # Find skeletal mesh component
        skel_comps = target_actor.get_components_by_class(unreal.SkeletalMeshComponent)
        if not skel_comps:
            return {"success": False, "error": "No SkeletalMeshComponent found on actor"}

        skel_comp = skel_comps[0]

        # Set animation mode and play
        skel_comp.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        skel_comp.set_animation(anim)
        skel_comp.set_looping(loop)
        skel_comp.play(True)

        log.log_result("set_actor_animation", True, "Animation set")
        return {
            "success": True,
            "message": f"Set animation on '{actor_name}'",
            "animation_path": animation_path,
            "looping": loop
        }

    except Exception as e:
        log.log_error(f"Error setting animation: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_play_animation(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Play/resume animation on actor

    Args:
        command: Dictionary with:
            - actor_name: Name/label of the actor
    """
    try:
        actor_name = command.get("actor_name")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        # Find actor
        actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        actors = actor_sub.get_all_level_actors()

        target_actor = None
        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                target_actor = actor
                break

        if not target_actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        skel_comps = target_actor.get_components_by_class(unreal.SkeletalMeshComponent)
        if not skel_comps:
            return {"success": False, "error": "No SkeletalMeshComponent found"}

        skel_comps[0].play(True)

        return {"success": True, "message": f"Playing animation on '{actor_name}'"}

    except Exception as e:
        log.log_error(f"Error playing animation: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_stop_animation(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Stop animation on actor

    Args:
        command: Dictionary with:
            - actor_name: Name/label of the actor
    """
    try:
        actor_name = command.get("actor_name")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        # Find actor
        actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        actors = actor_sub.get_all_level_actors()

        target_actor = None
        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                target_actor = actor
                break

        if not target_actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        skel_comps = target_actor.get_components_by_class(unreal.SkeletalMeshComponent)
        if not skel_comps:
            return {"success": False, "error": "No SkeletalMeshComponent found"}

        skel_comps[0].stop()

        return {"success": True, "message": f"Stopped animation on '{actor_name}'"}

    except Exception as e:
        log.log_error(f"Error stopping animation: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_actor_skeleton_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Get skeleton information from an actor

    Args:
        command: Dictionary with:
            - actor_name: Name/label of the actor
    """
    try:
        actor_name = command.get("actor_name")

        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        # Find actor
        actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        actors = actor_sub.get_all_level_actors()

        target_actor = None
        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                target_actor = actor
                break

        if not target_actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        skel_comps = target_actor.get_components_by_class(unreal.SkeletalMeshComponent)
        if not skel_comps:
            return {"success": False, "error": "No SkeletalMeshComponent found"}

        skel_comp = skel_comps[0]
        skel_mesh = skel_comp.get_skeletal_mesh_asset()

        info = {
            "actor_name": actor_name,
            "has_skeletal_mesh": skel_mesh is not None
        }

        if skel_mesh:
            info["mesh_path"] = str(skel_mesh.get_path_name())
            skeleton = skel_mesh.get_skeleton()
            if skeleton:
                info["skeleton_path"] = str(skeleton.get_path_name())
                info["bone_count"] = skeleton.get_num_bones() if hasattr(skeleton, 'get_num_bones') else 0

        # Current animation
        current_anim = skel_comp.get_animation()
        if current_anim:
            info["current_animation"] = str(current_anim.get_path_name())
            info["is_playing"] = skel_comp.is_playing()

        return {"success": True, **info}

    except Exception as e:
        log.log_error(f"Error getting skeleton info: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_set_animation_blueprint(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Set Animation Blueprint on actor

    Args:
        command: Dictionary with:
            - actor_name: Name/label of the actor
            - anim_bp_path: Path to Animation Blueprint
    """
    try:
        actor_name = command.get("actor_name")
        anim_bp_path = command.get("anim_bp_path")

        if not actor_name or not anim_bp_path:
            return {"success": False, "error": "actor_name and anim_bp_path required"}

        log.log_command("set_animation_blueprint", f"Setting AnimBP on {actor_name}")

        # Find actor
        actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        actors = actor_sub.get_all_level_actors()

        target_actor = None
        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                target_actor = actor
                break

        if not target_actor:
            return {"success": False, "error": f"Actor not found: {actor_name}"}

        # Load AnimBP
        anim_bp = unreal.EditorAssetLibrary.load_asset(anim_bp_path)
        if not anim_bp:
            return {"success": False, "error": f"AnimBP not found: {anim_bp_path}"}

        skel_comps = target_actor.get_components_by_class(unreal.SkeletalMeshComponent)
        if not skel_comps:
            return {"success": False, "error": "No SkeletalMeshComponent found"}

        skel_comp = skel_comps[0]
        skel_comp.set_animation_mode(unreal.AnimationMode.ANIMATION_BLUEPRINT)
        skel_comp.set_anim_instance_class(anim_bp.generated_class())

        log.log_result("set_animation_blueprint", True, "AnimBP set")
        return {
            "success": True,
            "message": f"Set AnimBP on '{actor_name}'",
            "anim_bp_path": anim_bp_path
        }

    except Exception as e:
        log.log_error(f"Error setting AnimBP: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


# ============================================================================
# COMMAND REGISTRY
# ============================================================================

SKELETAL_MESH_COMMANDS = {
    "get_skeleton_bones": handle_get_skeleton_bones,
    "get_animation_sequences": handle_get_animation_sequences,
    "set_actor_animation": handle_set_actor_animation,
    "play_animation": handle_play_animation,
    "stop_animation": handle_stop_animation,
    "get_actor_skeleton_info": handle_get_actor_skeleton_info,
    "set_animation_blueprint": handle_set_animation_blueprint,
}


def get_skeletal_mesh_commands():
    """Return all available skeletal mesh commands"""
    return SKELETAL_MESH_COMMANDS
