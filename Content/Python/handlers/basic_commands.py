import unreal
from typing import Dict, Any, List, Tuple

import base64
import os
import mss
import time
import tempfile # Used to find the OS's temporary folder

from utils import unreal_conversions as uc
from utils import logging as log

def handle_spawn(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a spawn command
    
    Args:
        command: The command dictionary containing:
            - actor_class: Actor class name/path or mesh path (e.g., "/Game/Blueprints/BP_Barrel" or "/Game/Meshes/SM_Barrel01.SM_Barrel01")
            - location: [X, Y, Z] coordinates (optional)
            - rotation: [Pitch, Yaw, Roll] in degrees (optional)
            - scale: [X, Y, Z] scale factors (optional)
            - actor_label: Optional custom name for the actor
            
    Returns:
        Response dictionary with success/failure status and additional info
    """
    try:
        # Extract parameters
        actor_class_name = command.get("actor_class", "Cube")
        location = command.get("location", (0, 0, 0))
        rotation = command.get("rotation", (0, 0, 0))
        scale = command.get("scale", (1, 1, 1))
        actor_label = command.get("actor_label")

        unreal.log(f"Spawn command: Class: {actor_class_name}, Label: {actor_label}")

        # Convert parameters to Unreal types
        loc = uc.to_unreal_vector(location)
        rot = uc.to_unreal_rotator(rotation)
        scale_vector = uc.to_unreal_vector(scale)

        actor = None
        gen_actor_utils = unreal.GenActorUtils

        # Check if it's a mesh path (e.g., "/Game/.../SM_Barrel01.SM_Barrel01")
        if actor_class_name.startswith("/Game") and "." in actor_class_name:
            # Try loading as a static mesh
            mesh = unreal.load_object(None, actor_class_name)
            if isinstance(mesh, unreal.StaticMesh):
                actor = gen_actor_utils.spawn_static_mesh_actor(actor_class_name, loc, rot, scale_vector, actor_label or "")
            else:
                # Fallback to actor class if not a mesh
                actor = gen_actor_utils.spawn_actor_from_class(actor_class_name, loc, rot, scale_vector, actor_label or "")
        else:
            # Handle basic shapes or actor classes
            shape_map = {"cube": "Cube", "sphere": "Sphere", "cylinder": "Cylinder", "cone": "Cone"}
            actor_class_lower = actor_class_name.lower()
            if actor_class_lower in shape_map:
                proper_name = shape_map[actor_class_lower]
                actor = gen_actor_utils.spawn_basic_shape(proper_name, loc, rot, scale_vector, actor_label or "")
            else:
                actor = gen_actor_utils.spawn_actor_from_class(actor_class_name, loc, rot, scale_vector, actor_label or "")

        if not actor:
            unreal.log_error(f"Failed to spawn actor of type {actor_class_name}")
            return {"success": False, "error": f"Failed to spawn actor of type {actor_class_name}"}

        actor_name = actor.get_actor_label()
        unreal.log(f"Spawned actor: {actor_name} at {loc}")
        return {"success": True, "actor_name": actor_name}

    except Exception as e:
        unreal.log_error(f"Error spawning actor: {str(e)}")
        return {"success": False, "error": str(e)}




def handle_take_screenshot(command):
    """
    Takes a screenshot using the HighResShot console command with a deterministic filename.
    This is the most reliable method for engine-based screenshots.
    """
    # 1. Create a unique, absolute file path in the OS's temp directory.
    # This ensures we have a clean place to work with guaranteed write permissions.
    temp_dir = tempfile.gettempdir()
    unique_filename = f"unreal_mcp_screenshot_{int(time.time())}.png"
    # Use forward slashes, as this is more reliable for Unreal console commands
    screenshot_path = os.path.join(temp_dir, unique_filename).replace('\\', '/')

    try:
        # 2. Construct the console command with the full, absolute filename.
        console_command = f'HighResShot 1 filename="{screenshot_path}"'
        unreal.log(f"Executing screenshot command: {console_command}")

        # Execute the command in the editor world context
        unreal.SystemLibrary.execute_console_command(unreal.EditorLevelLibrary.get_editor_world(), console_command)

        # 3. Poll for the file's existence instead of using a fixed sleep time.
        # This is much more reliable than a fixed wait.
        max_wait_seconds = 5
        wait_interval = 0.2
        time_waited = 0
        file_created = False
        while time_waited < max_wait_seconds:
            if os.path.exists(screenshot_path):
                file_created = True
                break
            time.sleep(wait_interval)
            time_waited += wait_interval

        if not file_created:
            return {"success": False, "error": f"Command was executed, but the output file was not found at the specified path: {screenshot_path}"}

        # 4. Read the file, encode it, and prepare the response
        with open(screenshot_path, 'rb') as image_file:
            image_data = image_file.read()
        base64_encoded_data = base64.b64encode(image_data).decode('utf-8')

        return {
            "success": True,
            "data": base64_encoded_data,
            "mime_type": "image/png"
        }

    except Exception as e:
        return {"success": False, "error": f"The screenshot process failed with an exception: {str(e)}"}

    finally:
        # 5. Clean up the temporary screenshot file from the temp directory
        if os.path.exists(screenshot_path):
            try:
                os.remove(screenshot_path)
            except Exception as e_cleanup:
                unreal.log_error(f"Failed to delete temporary screenshot file '{screenshot_path}': {e_cleanup}")

def handle_create_material(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a create_material command
    
    Args:
        command: The command dictionary containing:
            - material_name: Name for the new material
            - color: [R, G, B] color values (0-1)
            
    Returns:
        Response dictionary with success/failure status and material path if successful
    """
    try:
        # Extract parameters
        material_name = command.get("material_name", "NewMaterial")
        color = command.get("color", (1, 0, 0))

        log.log_command("create_material", f"Name: {material_name}, Color: {color}")

        # Use the C++ utility class
        gen_actor_utils = unreal.GenActorUtils
        color_linear = uc.to_unreal_color(color)

        material = gen_actor_utils.create_material(material_name, color_linear)

        if not material:
            log.log_error("Failed to create material")
            return {"success": False, "error": "Failed to create material"}

        material_path = f"/Game/Materials/{material_name}"
        log.log_result("create_material", True, f"Path: {material_path}")
        return {"success": True, "material_path": material_path}

    except Exception as e:
        log.log_error(f"Error creating material: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_all_scene_objects(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        level = unreal.EditorLevelLibrary.get_level(unreal.EditorLevelLibrary.get_editor_world())
        actors = unreal.GameplayStatics.get_all_actors_of_class(level, unreal.Actor)
        result = [
            {"name": actor.get_name(), "class": actor.get_class().get_name(), "location": [actor.get_actor_location().x, actor.get_actor_location().y, actor.get_actor_location().z]}
            for actor in actors
        ]
        return {"success": True, "actors": result}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_create_project_folder(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        folder_path = command.get("folder_path")
        full_path = f"/Game/{folder_path}"
        unreal.EditorAssetLibrary.make_directory(full_path)
        return {"success": True, "message": f"Created folder at {full_path}"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_files_in_folder(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        folder_path = f"/Game/{command.get('folder_path')}"
        files = unreal.EditorAssetLibrary.list_assets(folder_path, recursive=False)
        return {"success": True, "files": [str(f) for f in files]}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_add_input_binding(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        action_name = command.get("action_name")
        key = command.get("key")
        # Correctly access the InputSettings singleton
        input_settings = unreal.InputSettings.get_input_settings()
        # Create the input action mapping
        action_mapping = unreal.InputActionKeyMapping(action_name=action_name, key=unreal.InputCoreTypes.get_key(key))
        # Add the mapping to the input settings
        input_settings.add_action_mapping(action_mapping)
        # Save the changes to the config file
        input_settings.save_config()
        return {"success": True, "message": f"Added input binding {action_name} -> {key}"}
    except Exception as e:
        return {"success": False, "error": str(e)}