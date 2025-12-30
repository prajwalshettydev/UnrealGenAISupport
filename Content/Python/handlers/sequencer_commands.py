"""
Sequencer Commands Handler
Wrapper for GenSequencerUtils C++ functions
Level Sequences, Cinematics, Cutscenes
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
# SEQUENCE CREATION & MANAGEMENT
# ============================================

def handle_create_sequence(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Create a new Level Sequence asset

    Args:
        command: Dictionary with:
            - sequence_path: Full path for the new sequence (e.g., /Game/Cinematics/MySequence)
            - frame_rate: Frames per second (default 30)
    """
    try:
        sequence_path = command.get("sequence_path")
        frame_rate = command.get("frame_rate", 30.0)

        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        log.log_command("create_sequence", f"Creating {sequence_path}")
        result = unreal.GenSequencerUtils.create_sequence(sequence_path, frame_rate)
        return _parse_json_response(result)

    except Exception as e:
        log.log_error(f"Error creating sequence: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_open_sequence(command: Dict[str, Any]) -> Dict[str, Any]:
    """Open a Level Sequence in the Sequencer editor"""
    try:
        sequence_path = command.get("sequence_path")
        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        result = unreal.GenSequencerUtils.open_sequence(sequence_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_list_sequences(command: Dict[str, Any]) -> Dict[str, Any]:
    """List all Level Sequences in a directory"""
    try:
        directory_path = command.get("directory_path", "/Game")
        result = unreal.GenSequencerUtils.list_sequences(directory_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_sequence_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get sequence info (duration, frame rate, tracks)"""
    try:
        sequence_path = command.get("sequence_path")
        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        result = unreal.GenSequencerUtils.get_sequence_info(sequence_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_delete_sequence(command: Dict[str, Any]) -> Dict[str, Any]:
    """Delete a sequence asset"""
    try:
        sequence_path = command.get("sequence_path")
        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        result = unreal.GenSequencerUtils.delete_sequence(sequence_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# TRACK MANAGEMENT
# ============================================

def handle_add_actor_to_sequence(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add an actor binding to a sequence"""
    try:
        sequence_path = command.get("sequence_path")
        actor_name = command.get("actor_name")

        if not sequence_path or not actor_name:
            return {"success": False, "error": "sequence_path and actor_name required"}

        result = unreal.GenSequencerUtils.add_actor_to_sequence(sequence_path, actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_camera_cut_track(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add a camera cut track to a sequence"""
    try:
        sequence_path = command.get("sequence_path")
        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        result = unreal.GenSequencerUtils.add_camera_cut_track(sequence_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_transform_track(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add a transform track to an actor binding"""
    try:
        sequence_path = command.get("sequence_path")
        actor_name = command.get("actor_name")

        if not sequence_path or not actor_name:
            return {"success": False, "error": "sequence_path and actor_name required"}

        result = unreal.GenSequencerUtils.add_transform_track(sequence_path, actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_animation_track(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add a skeletal animation track to an actor"""
    try:
        sequence_path = command.get("sequence_path")
        actor_name = command.get("actor_name")

        if not sequence_path or not actor_name:
            return {"success": False, "error": "sequence_path and actor_name required"}

        result = unreal.GenSequencerUtils.add_animation_track(sequence_path, actor_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_audio_track(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add an audio track to the sequence"""
    try:
        sequence_path = command.get("sequence_path")
        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        result = unreal.GenSequencerUtils.add_audio_track(sequence_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_event_track(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add an event track to the sequence"""
    try:
        sequence_path = command.get("sequence_path")
        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        result = unreal.GenSequencerUtils.add_event_track(sequence_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_list_tracks(command: Dict[str, Any]) -> Dict[str, Any]:
    """List all tracks in a sequence"""
    try:
        sequence_path = command.get("sequence_path")
        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        result = unreal.GenSequencerUtils.list_tracks(sequence_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# KEYFRAME OPERATIONS
# ============================================

def handle_add_transform_key(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add a transform keyframe for an actor"""
    try:
        sequence_path = command.get("sequence_path")
        actor_name = command.get("actor_name")
        frame_number = command.get("frame_number", 0)
        location = command.get("location", [0, 0, 0])
        rotation = command.get("rotation", [0, 0, 0])
        scale = command.get("scale", [1, 1, 1])

        if not sequence_path or not actor_name:
            return {"success": False, "error": "sequence_path and actor_name required"}

        loc = unreal.Vector(location[0], location[1], location[2])
        rot = unreal.Rotator(rotation[0], rotation[1], rotation[2])
        scl = unreal.Vector(scale[0], scale[1], scale[2])

        result = unreal.GenSequencerUtils.add_transform_key(
            sequence_path, actor_name, frame_number, loc, rot, scl
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_animation_section(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add an animation section to an actor"""
    try:
        sequence_path = command.get("sequence_path")
        actor_name = command.get("actor_name")
        animation_path = command.get("animation_path")
        start_frame = command.get("start_frame", 0)
        end_frame = command.get("end_frame", -1)

        if not sequence_path or not actor_name or not animation_path:
            return {"success": False, "error": "sequence_path, actor_name and animation_path required"}

        result = unreal.GenSequencerUtils.add_animation_section(
            sequence_path, actor_name, animation_path, start_frame, end_frame
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_audio_section(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add an audio section to the audio track"""
    try:
        sequence_path = command.get("sequence_path")
        sound_path = command.get("sound_path")
        start_frame = command.get("start_frame", 0)

        if not sequence_path or not sound_path:
            return {"success": False, "error": "sequence_path and sound_path required"}

        result = unreal.GenSequencerUtils.add_audio_section(
            sequence_path, sound_path, start_frame
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# CAMERA OPERATIONS
# ============================================

def handle_spawn_sequence_camera(command: Dict[str, Any]) -> Dict[str, Any]:
    """Spawn a cine camera and add to sequence"""
    try:
        sequence_path = command.get("sequence_path")
        camera_name = command.get("camera_name", "CineCamera")
        location = command.get("location", [0, 0, 0])
        rotation = command.get("rotation", [0, 0, 0])

        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        loc = unreal.Vector(location[0], location[1], location[2])
        rot = unreal.Rotator(rotation[0], rotation[1], rotation[2])

        result = unreal.GenSequencerUtils.spawn_sequence_camera(
            sequence_path, camera_name, loc, rot
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_camera_cut(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add a camera cut at a specific frame"""
    try:
        sequence_path = command.get("sequence_path")
        camera_name = command.get("camera_name")
        frame_number = command.get("frame_number", 0)

        if not sequence_path or not camera_name:
            return {"success": False, "error": "sequence_path and camera_name required"}

        result = unreal.GenSequencerUtils.add_camera_cut(
            sequence_path, camera_name, frame_number
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_add_camera_shake(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add a camera shake section"""
    try:
        sequence_path = command.get("sequence_path")
        camera_name = command.get("camera_name")
        shake_class = command.get("shake_class")
        start_frame = command.get("start_frame", 0)
        end_frame = command.get("end_frame", 30)

        if not sequence_path or not camera_name or not shake_class:
            return {"success": False, "error": "sequence_path, camera_name and shake_class required"}

        result = unreal.GenSequencerUtils.add_camera_shake(
            sequence_path, camera_name, shake_class, start_frame, end_frame
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# PLAYBACK CONTROL
# ============================================

def handle_play_sequence(command: Dict[str, Any]) -> Dict[str, Any]:
    """Play the sequence in editor"""
    try:
        sequence_path = command.get("sequence_path")
        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        result = unreal.GenSequencerUtils.play_sequence(sequence_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_stop_sequence(command: Dict[str, Any]) -> Dict[str, Any]:
    """Stop sequence playback"""
    try:
        result = unreal.GenSequencerUtils.stop_sequence()
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_set_playback_position(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set the playback position"""
    try:
        sequence_path = command.get("sequence_path")
        frame_number = command.get("frame_number", 0)

        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        result = unreal.GenSequencerUtils.set_playback_position(sequence_path, frame_number)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_set_playback_range(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set sequence play range"""
    try:
        sequence_path = command.get("sequence_path")
        start_frame = command.get("start_frame", 0)
        end_frame = command.get("end_frame", 100)

        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        result = unreal.GenSequencerUtils.set_playback_range(
            sequence_path, start_frame, end_frame
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# RENDERING & EXPORT
# ============================================

def handle_render_sequence(command: Dict[str, Any]) -> Dict[str, Any]:
    """Render sequence to video"""
    try:
        sequence_path = command.get("sequence_path")
        output_path = command.get("output_path")
        resolution = command.get("resolution", "1080p")

        if not sequence_path or not output_path:
            return {"success": False, "error": "sequence_path and output_path required"}

        result = unreal.GenSequencerUtils.render_sequence(
            sequence_path, output_path, resolution
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_export_sequence_to_fbx(command: Dict[str, Any]) -> Dict[str, Any]:
    """Export sequence to FBX"""
    try:
        sequence_path = command.get("sequence_path")
        output_path = command.get("output_path")

        if not sequence_path or not output_path:
            return {"success": False, "error": "sequence_path and output_path required"}

        result = unreal.GenSequencerUtils.export_to_fbx(sequence_path, output_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# SUBSEQUENCES
# ============================================

def handle_add_subsequence(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add a subsequence track"""
    try:
        parent_sequence_path = command.get("parent_sequence_path")
        sub_sequence_path = command.get("sub_sequence_path")
        start_frame = command.get("start_frame", 0)

        if not parent_sequence_path or not sub_sequence_path:
            return {"success": False, "error": "parent_sequence_path and sub_sequence_path required"}

        result = unreal.GenSequencerUtils.add_sub_sequence(
            parent_sequence_path, sub_sequence_path, start_frame
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_list_subsequences(command: Dict[str, Any]) -> Dict[str, Any]:
    """List subsequences in a sequence"""
    try:
        sequence_path = command.get("sequence_path")
        if not sequence_path:
            return {"success": False, "error": "sequence_path required"}

        result = unreal.GenSequencerUtils.list_sub_sequences(sequence_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# COMMAND REGISTRY
# ============================================

SEQUENCER_COMMANDS = {
    # Sequence Management
    "create_sequence": handle_create_sequence,
    "open_sequence": handle_open_sequence,
    "list_sequences": handle_list_sequences,
    "get_sequence_info": handle_get_sequence_info,
    "delete_sequence": handle_delete_sequence,

    # Track Management
    "add_actor_to_sequence": handle_add_actor_to_sequence,
    "add_camera_cut_track": handle_add_camera_cut_track,
    "add_transform_track": handle_add_transform_track,
    "add_animation_track": handle_add_animation_track,
    "add_audio_track": handle_add_audio_track,
    "add_event_track": handle_add_event_track,
    "list_tracks": handle_list_tracks,

    # Keyframes
    "add_transform_key": handle_add_transform_key,
    "add_animation_section": handle_add_animation_section,
    "add_audio_section": handle_add_audio_section,

    # Camera
    "spawn_sequence_camera": handle_spawn_sequence_camera,
    "add_camera_cut": handle_add_camera_cut,
    "add_camera_shake": handle_add_camera_shake,

    # Playback
    "play_sequence": handle_play_sequence,
    "stop_sequence": handle_stop_sequence,
    "set_playback_position": handle_set_playback_position,
    "set_playback_range": handle_set_playback_range,

    # Export
    "render_sequence": handle_render_sequence,
    "export_sequence_to_fbx": handle_export_sequence_to_fbx,

    # Subsequences
    "add_subsequence": handle_add_subsequence,
    "list_subsequences": handle_list_subsequences,
}


def get_sequencer_commands():
    """Return all available sequencer commands"""
    return SEQUENCER_COMMANDS
