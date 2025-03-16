import socket
import json
import unreal
import threading

# Global queue for commands
command_queue = []
response_dict = {}

def handle_handshake(message):
    # Echo back the message as a simple response
    unreal.log(f"Handshake received: {message}")
    return {"success": True, "message": f"Received: {message}"}

def spawn_actor(actor_class_name, location=(0, 0, 0), rotation=(0, 0, 0), scale=(1, 1, 1), actor_label=None):
    try:
        # Import the C++ utility class
        gen_actor_utils = unreal.GenActorUtils

        # First handle specific cases for basic shapes
        actor_class_lower = actor_class_name.lower()

        # Map to properly capitalized names for finding assets
        shape_map = {
            "cube": "Cube",
            "sphere": "Sphere",
            "cylinder": "Cylinder",
            "cone": "Cone"
        }

        # Convert parameters to Unreal types
        loc = unreal.Vector(location[0], location[1], location[2])
        rot = unreal.Rotator(rotation[0], rotation[1], rotation[2])
        scale_vector = unreal.Vector(scale[0], scale[1], scale[2])

        actor = None

        # Use the right C++ function based on the actor type
        if actor_class_lower in shape_map:
            proper_name = shape_map[actor_class_lower]
            actor = gen_actor_utils.spawn_basic_shape(proper_name, loc, rot, scale_vector, actor_label or "")
        else:
            actor = gen_actor_utils.spawn_actor_from_class(actor_class_name, loc, rot, scale_vector, actor_label or "")

        if not actor:
            return {"success": False, "error": f"Failed to spawn actor of type {actor_class_name}"}

        unreal.log(f"Spawned actor: {actor.get_actor_label()} at {loc}")
        return {"success": True, "actor_name": actor.get_actor_label()}

    except Exception as e:
        unreal.log_error(f"Error spawning actor: {str(e)}")
        return {"success": False, "error": str(e)}
    
def create_material(material_name, color=(1, 0, 0)):
    try:
        # Create a new material in the content browser
        package_path = "/Game/Materials"
        factory = unreal.MaterialFactoryNew()
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

        # Create the material asset
        material = asset_tools.create_asset(material_name, package_path, unreal.Material, factory)

        # Get the color parameter
        color_param = unreal.LinearColor(color[0], color[1], color[2], 1.0)

        # Create a constant color expression and set its value
        constant_expr = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -350, 0)
        constant_expr.set_editor_property("constant", color_param)

        # Connect to the base color
        unreal.MaterialEditingLibrary.connect_material_property(constant_expr, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

        # Compile and save the material
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_asset(f"{package_path}/{material_name}")

        unreal.log(f"Created material: {material_name} with color {color}")
        return {"success": True, "material_path": f"{package_path}/{material_name}"}

    except Exception as e:
        unreal.log_error(f"Error creating material: {str(e)}")
        return {"success": False, "error": str(e)}

def modify_object(actor_name, property_type, value):
    try:
        # Find the actor in the level
        actor = unreal.EditorLevelLibrary.get_actor_reference(actor_name)
        if not actor:
            return {"success": False, "error": f"Actor '{actor_name}' not found in level"}

        if property_type == "material":
            # Set the material
            material_path = value
            material = unreal.EditorAssetLibrary.find_asset_data(material_path).get_asset()
            if not material:
                return {"success": False, "error": f"Material '{material_path}' not found"}

            # Get static mesh component
            static_mesh_component = actor.get_component_by_class(unreal.StaticMeshComponent)
            if static_mesh_component:
                static_mesh_component.set_material(0, material)
                unreal.log(f"Set material of {actor_name} to {material_path}")
                return {"success": True}
            return {"success": False, "error": "No static mesh component found on actor"}

        elif property_type == "position":
            # Set position (expects tuple of x,y,z)
            if not isinstance(value, (list, tuple)) or len(value) != 3:
                return {"success": False, "error": "Position value must be a tuple/list of 3 coordinates"}
            loc = unreal.Vector(value[0], value[1], value[2])
            actor.set_actor_location(loc, False, True)
            unreal.log(f"Set position of {actor_name} to {loc}")
            return {"success": True}

        elif property_type == "rotation":
            # Set rotation (expects tuple of pitch,yaw,roll)
            if not isinstance(value, (list, tuple)) or len(value) != 3:
                return {"success": False, "error": "Rotation value must be a tuple/list of 3 angles"}
            rot = unreal.Rotator(value[0], value[1], value[2])
            actor.set_actor_rotation(rot, False)
            unreal.log(f"Set rotation of {actor_name} to {rot}")
            return {"success": True}

        elif property_type == "scale":
            # Set scale (expects tuple of x,y,z)
            if not isinstance(value, (list, tuple)) or len(value) != 3:
                return {"success": False, "error": "Scale value must be a tuple/list of 3 values"}
            scale = unreal.Vector(value[0], value[1], value[2])
            actor.set_actor_scale3d(scale)
            unreal.log(f"Set scale of {actor_name} to {scale}")
            return {"success": True}

        else:
            return {"success": False, "error": f"Unknown property type: {property_type}"}

    except Exception as e:
        unreal.log_error(f"Error modifying object: {str(e)}")
        return {"success": False, "error": str(e)}

def handle_command(command):
    command_type = command.get("type")

    if command_type == "handshake":
        return handle_handshake(command.get("message", ""))
    elif command_type == "spawn":
        actor_class = command.get("actor_class", "Cube")
        location = command.get("location", (0, 0, 0))
        rotation = command.get("rotation", (0, 0, 0))
        scale = command.get("scale", (1, 1, 1))
        actor_label = command.get("actor_label")
        return spawn_actor(actor_class, location, rotation, scale, actor_label)
    elif command_type == "create_material":
        material_name = command.get("material_name", "NewMaterial")
        color = command.get("color", (1, 0, 0))
        return create_material(material_name, color)
    elif command_type == "modify_object":
        actor_name = command.get("actor_name")
        property_type = command.get("property_type")
        value = command.get("value")
        if not actor_name or not property_type or value is None:
            return {"success": False, "error": "Missing required parameters"}
        return modify_object(actor_name, property_type, value)
    else:
        return {"success": False, "error": "Unknown command type"}

def process_commands(delta_time=None):
    if not command_queue:
        return

    command_id, command = command_queue.pop(0)
    unreal.log(f"Processing command on main thread: {command}")

    try:
        response = handle_command(command)
        response_dict[command_id] = response
    except Exception as e:
        unreal.log_error(f"Error processing command: {str(e)}")
        response_dict[command_id] = {"success": False, "error": str(e)}

def socket_server_thread():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('localhost', 9877))
    server_socket.listen(1)
    unreal.log("Unreal Engine socket server started on port 9877")

    command_counter = 0

    while True:
        try:
            conn, addr = server_socket.accept()
            data = conn.recv(4096)
            if data:
                command = json.loads(data.decode())
                unreal.log(f"Received command: {command}")

                # For handshake, we can respond directly from the thread
                if command.get("type") == "handshake":
                    response = handle_handshake(command.get("message", ""))
                    conn.sendall(json.dumps(response).encode())
                else:
                    # For other commands, queue them for main thread execution
                    command_id = command_counter
                    command_counter += 1
                    command_queue.append((command_id, command))

                    # Wait for the response with a timeout
                    timeout = 10  # seconds
                    start_time = time.time()
                    while command_id not in response_dict and time.time() - start_time < timeout:
                        time.sleep(0.1)

                    if command_id in response_dict:
                        response = response_dict.pop(command_id)
                        conn.sendall(json.dumps(response).encode())
                    else:
                        error_response = {"success": False, "error": "Command timed out"}
                        conn.sendall(json.dumps(error_response).encode())
            conn.close()
        except Exception as e:
            unreal.log_error(f"Error in socket server: {str(e)}")

# Register tick function to process commands on main thread
def register_command_processor():
    unreal.register_slate_post_tick_callback(process_commands)
    unreal.log("Command processor registered")

# Start the server thread
import time
thread = threading.Thread(target=socket_server_thread)
thread.daemon = True
thread.start()
unreal.log("Socket server thread started")

# Register the command processor on the main thread
register_command_processor()