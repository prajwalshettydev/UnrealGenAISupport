import json
import os
import re
import socket
import sys
from pathlib import Path

from mcp.server.fastmcp import FastMCP

# THIS FILE WILL RUN OUTSIDE THE UNREAL ENGINE SCOPE,
# DO NOT IMPORT UNREAL MODULES HERE OR EXECUTE IT IN THE UNREAL ENGINE PYTHON INTERPRETER


# Create a PID file to let the Unreal plugin know this process is running
def write_pid_file():
    try:
        pid = os.getpid()
        pid_dir = os.path.join(os.path.expanduser("~"), ".unrealgenai")
        os.makedirs(pid_dir, exist_ok=True)
        pid_path = os.path.join(pid_dir, "mcp_server.pid")

        with open(pid_path, "w") as f:
            f.write(f"{pid}\n9877")  # Store PID and port

        # Register to delete the PID file on exit
        import atexit

        def cleanup_pid_file():
            try:
                if os.path.exists(pid_path):
                    os.remove(pid_path)
            except:
                pass

        atexit.register(cleanup_pid_file)

        return pid_path
    except Exception as e:
        print(f"Failed to write PID file: {e}", file=sys.stderr)
        return None


# Write PID file on startup
pid_file = write_pid_file()
if pid_file:
    print(f"MCP Server started with PID file at: {pid_file}", file=sys.stderr)

# Create an MCP server
mcp = FastMCP("UnrealHandshake")


# Function to send a message to Unreal Engine via socket
def send_to_unreal(command):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.connect(("localhost", 9877))  # Unreal listens on port 9877

            # Ensure proper JSON encoding
            json_str = json.dumps(command)
            s.sendall(json_str.encode("utf-8"))

            # Implement robust response handling
            buffer_size = 8192  # Increased buffer size
            response_data = b""

            # Keep receiving data until we have complete JSON
            while True:
                chunk = s.recv(buffer_size)
                if not chunk:
                    break

                response_data += chunk

                # Check if we have complete JSON
                try:
                    json.loads(response_data.decode("utf-8"))
                    # If we get here, we have valid JSON
                    break
                except json.JSONDecodeError:
                    # Need more data, continue receiving
                    continue

            # Parse the complete response
            if response_data:
                return json.loads(response_data.decode("utf-8"))
            else:
                return {"success": False, "error": "No response received"}

        except Exception as e:
            print(f"Error sending to Unreal: {e}", file=sys.stderr)
            return {"success": False, "error": str(e)}


@mcp.tool()
def how_to_use() -> str:
    """Hey LLM, this grabs the how_to_use.md from knowledge_base—it's your cheat sheet for running Unreal with this MCP. Fetch it at the start of a new chat session to get the lowdown on quirks and how shit works."""
    try:
        current_dir = Path(__file__).parent
        md_file_path = current_dir / "knowledge_base" / "how_to_use.md"

        if not md_file_path.exists():
            return "Error: how_to_use.md not found in knowledge_base subfolder."

        with open(md_file_path, "r", encoding="utf-8") as md_file:
            return md_file.read()

    except Exception as e:
        return f"Error loading how_to_use.md: {str(e)}—fix your shit."


# Define basic tools for Claude to call


@mcp.tool()
def handshake_test(message: str) -> str:
    """Send a handshake message to Unreal Engine"""
    try:
        command = {"type": "handshake", "message": message}
        response = send_to_unreal(command)
        if response.get("success"):
            return f"Handshake successful: {response['message']}"
        else:
            return f"Handshake failed: {response.get('error', 'Unknown error')}"
    except Exception as e:
        return f"Error communicating with Unreal: {str(e)}"


@mcp.tool()
def execute_python_script(script: str) -> str:
    """
    Execute a Python script within Unreal Engine's Python interpreter.

    Args:
        script: A string containing the Python code to execute in Unreal Engine.

    Returns:
        Message indicating success, failure, or a request for confirmation.

    Note:
        This tool sends the script to Unreal Engine, where it is executed via a temporary file using Unreal's internal
        Python execution system (similar to GEngine->Exec). This method is stable but may not handle Blueprint-specific
        APIs as seamlessly as direct Python API calls. For Blueprint manipulation, consider using dedicated tools like
        `add_node_to_blueprint` or ensuring the script uses stable `unreal` module functions. Use this tool for Python
        script execution instead of `execute_unreal_command` with 'py' commands.
    """
    try:
        if is_potentially_destructive(script):
            return (
                "This script appears to involve potentially destructive actions (e.g., deleting or saving files) "
                "that were not explicitly requested. Please confirm if you want to proceed by saying 'Yes, execute it' "
                "or modify your request to explicitly allow such actions."
            )

        command = {"type": "execute_python", "script": script}
        response = send_to_unreal(command)
        if response.get("success"):
            output = response.get("output", "No output returned")
            return f"Script executed successfully. Output: {output}"
        else:
            error = response.get("error", "Unknown error")
            output = response.get("output", "")
            if output:
                error += f"\n\nPartial output before error: {output}"
            return f"Failed to execute script: {response.get('error', 'Unknown error')}"
    except Exception as e:
        return f"Error sending script to Unreal: {str(e)}"


@mcp.tool()
def execute_unreal_command(command: str) -> str:
    """
    Execute an Unreal Engine command-line (CMD) command.

    Args:
        command: A string containing the Unreal Engine command to execute (e.g., "obj list", "stat fps").

    Returns:
        Message indicating success or failure, including any output or errors.

    Note:
        This tool executes commands directly in Unreal Engine's command system, similar to the editor's console.
        It is intended for built-in editor commands (e.g., "stat fps", "obj list") and not for running Python scripts.
        Do not use this tool with 'py' commands (e.g., "py script.py"); instead, use `execute_python_script` for Python
        execution, which provides dedicated safety checks and output handling. Output capture is limited; for detailed
        output, consider wrapping the command in a Python script with `execute_python_script`.
    """
    try:
        # Check if the command is attempting to run a Python script
        if command.strip().lower().startswith("py "):
            return (
                "Error: Use `execute_python_script` to run Python scripts instead of `execute_unreal_command` with 'py' commands. "
                "For example, use `execute_python_script(script='your_code_here')` for Python execution."
            )

        # Check for potentially destructive commands
        destructive_keywords = ["delete", "save", "quit", "exit", "restart"]
        if any(keyword in command.lower() for keyword in destructive_keywords):
            return (
                "This command appears to involve potentially destructive actions (e.g., deleting or saving). "
                "Please confirm by saying 'Yes, execute it' or explicitly request such actions."
            )

        command_dict = {"type": "execute_unreal_command", "command": command}
        response = send_to_unreal(command_dict)
        if response.get("success"):
            output = response.get(
                "output", "Command executed with no detailed output returned"
            )
            return f"Command '{command}' executed successfully. Output: {output}"
        else:
            return f"Failed to execute command '{command}': {response.get('error', 'Unknown error')}"
    except Exception as e:
        return f"Error sending command to Unreal: {str(e)}"


#
# Basic Object Commands
#


@mcp.tool()
def spawn_object(
    actor_class: str,
    location: list = [0, 0, 0],
    rotation: list = [0, 0, 0],
    scale: list = [1, 1, 1],
    actor_label: str = None,
) -> str:
    """
    Spawn an object in the Unreal Engine level

    Args:
        actor_class: For basic shapes, use: "Cube", "Sphere", "Cylinder", or "Cone".
                     For other actors, use class name like "PointLight" or full path.
        location: [X, Y, Z] coordinates
        rotation: [Pitch, Yaw, Roll] in degrees
        scale: [X, Y, Z] scale factors
        actor_label: Optional custom name for the actor

    Returns:
        Message indicating success or failure
    """
    command = {
        "type": "spawn",
        "actor_class": actor_class,
        "location": location,
        "rotation": rotation,
        "scale": scale,
        "actor_label": actor_label,
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully spawned {actor_class}" + (
            f" with label '{actor_label}'" if actor_label else ""
        )
    else:
        error = response.get("error", "Unknown error")
        # Add hint for Claude to understand what went wrong
        if "not found" in error:
            hint = "\nHint: For basic shapes, use 'Cube', 'Sphere', 'Cylinder', or 'Cone'. For other actors, try using '/Script/Engine.PointLight' format."
            error += hint
        return f"Failed to spawn object: {error}"


@mcp.tool()
def edit_component_property(
    blueprint_path: str,
    component_name: str,
    property_name: str,
    value: str,
    is_scene_actor: bool = False,
    actor_name: str = "",
) -> str:
    """
    Edit a property of a component in a Blueprint or scene actor.

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/FlappyBird/BP_FlappyBird") or "" for scene actors
        component_name: Name of the component (e.g., "BirdMesh", "RootComponent")
        property_name: Name of the property to edit (e.g., "StaticMesh", "RelativeLocation")
        value: New value as a string (e.g., "'/Engine/BasicShapes/Sphere.Sphere'", "100,200,300")
        is_scene_actor: If True, edit a component on a scene actor (default: False)
        actor_name: Name of the actor in the scene (required if is_scene_actor is True, e.g., "Cube_1")

    Returns:
        Message indicating success or failure, with optional property suggestions if the property is not found.

    Capabilities:
        - Set component properties in Blueprints (e.g., StaticMesh, bSimulatePhysics).
        - Modify scene actor components (e.g., position, rotation, scale, material).
        - Supports scalar types (float, int, bool), objects (e.g., materials), and vectors/rotators (e.g., "100,200,300" for FVector).
        - Examples:
            - Set a mesh: edit_component_property("/Game/FlappyBird/BP_FlappyBird", "BirdMesh", "StaticMesh", "'/Engine/BasicShapes/Sphere.Sphere'")
            - Move an actor: edit_component_property("", "RootComponent", "RelativeLocation", "100,200,300", True, "Cube_1")
            - Rotate an actor: edit_component_property("", "RootComponent", "RelativeRotation", "0,90,0", True, "Cube_1")
            - Scale an actor: edit_component_property("", "RootComponent", "RelativeScale3D", "2,2,2", True, "Cube_1")
            - Enable physics: edit_component_property("/Game/FlappyBird/BP_FlappyBird", "BirdMesh", "bSimulatePhysics", "true")
    """
    command = {
        "type": "edit_component_property",
        "blueprint_path": blueprint_path,
        "component_name": component_name,
        "property_name": property_name,
        "value": value,
        "is_scene_actor": is_scene_actor,
        "actor_name": actor_name,
    }
    response = send_to_unreal(command)

    # CHANGED: Improved response handling to support both string and dict responses
    try:
        # Handle case where response is already a dict
        if isinstance(response, dict):
            result = response
        # Handle case where response is a string
        elif isinstance(response, str):
            import json

            result = json.loads(response)
        else:
            return f"Error: Unexpected response type: {type(response)}"

        if result.get("success"):
            return result.get(
                "message", f"Set {property_name} of {component_name} to {value}"
            )
        else:
            error = result.get("error", "Unknown error")
            if "suggestions" in result:
                error += f"\nSuggestions: {result['suggestions']}"
            return f"Failed: {error}"
    except Exception as e:
        return f"Error: {str(e)}\nRaw response: {response}"


@mcp.tool()
def create_material(material_name: str, color: list) -> str:
    """
    Create a new material with the specified color

    Args:
        material_name: Name for the new material
        color: [R, G, B] color values (0-1)

    Returns:
        Message indicating success or failure, and the material path if successful
    """
    command = {
        "type": "create_material",
        "material_name": material_name,
        "color": color,
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully created material '{material_name}' with path: {response.get('material_path')}"
    else:
        return f"Failed to create material: {response.get('error', 'Unknown error')}"


#
# Blueprint Commands
#


@mcp.tool()
def create_blueprint(
    blueprint_name: str,
    parent_class: str = "Actor",
    save_path: str = "/Game/Blueprints",
) -> str:
    """
    Create a new Blueprint class

    Args:
        blueprint_name: Name for the new Blueprint
        parent_class: Parent class name or path (e.g., "Actor", "/Script/Engine.Actor")
        save_path: Path to save the Blueprint asset

    Returns:
        Message indicating success or failure
    """
    command = {
        "type": "create_blueprint",
        "blueprint_name": blueprint_name,
        "parent_class": parent_class,
        "save_path": save_path,
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully created Blueprint '{blueprint_name}' with path: {response.get('blueprint_path', save_path + '/' + blueprint_name)}"
    else:
        return f"Failed to create Blueprint: {response.get('error', 'Unknown error')}"


@mcp.tool()
def add_component_to_blueprint(
    blueprint_path: str, component_class: str, component_name: str = None
) -> str:
    """
    Add a component to a Blueprint

    Args:
        blueprint_path: Path to the Blueprint asset
        component_class: Component class to add (e.g., "StaticMeshComponent", "PointLightComponent")
        component_name: Name for the new component (optional)

    Returns:
        Message indicating success or failure
    """
    command = {
        "type": "add_component",
        "blueprint_path": blueprint_path,
        "component_class": component_class,
        "component_name": component_name,
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully added {component_class} to Blueprint at {blueprint_path}"
    else:
        return f"Failed to add component: {response.get('error', 'Unknown error')}"


@mcp.tool()
def add_variable_to_blueprint(
    blueprint_path: str,
    variable_name: str,
    variable_type: str,
    default_value: str = None,
    category: str = "Default",
) -> str:
    """
    Add a variable to a Blueprint

    Args:
        blueprint_path: Path to the Blueprint asset
        variable_name: Name for the new variable
        variable_type: Type of the variable (e.g., "float", "vector", "boolean")
        default_value: Default value for the variable (optional)
        category: Category for organizing variables in the Blueprint editor (optional)

    Returns:
        Message indicating success or failure
    """
    # Convert default_value to string if it's a number
    if default_value is not None and not isinstance(default_value, str):
        default_value = str(default_value)

    command = {
        "type": "add_variable",
        "blueprint_path": blueprint_path,
        "variable_name": variable_name,
        "variable_type": variable_type,
        "default_value": default_value,
        "category": category,
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully added {variable_type} variable '{variable_name}' to Blueprint at {blueprint_path}"
    else:
        return f"Failed to add variable: {response.get('error', 'Unknown error')}"


@mcp.tool()
def add_function_to_blueprint(
    blueprint_path: str, function_name: str, inputs: list = None, outputs: list = None
) -> str:
    """
    Add a function to a Blueprint

    Args:
        blueprint_path: Path to the Blueprint asset
        function_name: Name for the new function
        inputs: List of input parameters [{"name": "param1", "type": "float"}, ...]
        outputs: List of output parameters [{"name": "return", "type": "boolean"}, ...]

    Returns:
        Message indicating success or failure
    """
    if inputs is None:
        inputs = []
    if outputs is None:
        outputs = []

    command = {
        "type": "add_function",
        "blueprint_path": blueprint_path,
        "function_name": function_name,
        "inputs": inputs,
        "outputs": outputs,
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully added function '{function_name}' to Blueprint at {blueprint_path} with ID: {response.get('function_id', 'unknown')}"
    else:
        return f"Failed to add function: {response.get('error', 'Unknown error')}"


@mcp.tool()
def add_node_to_blueprint(
    blueprint_path: str,
    function_id: str,
    node_type: str,
    node_position: list = [0, 0],
    node_properties: dict = None,
) -> str:
    """
    Add a node to a Blueprint graph

    Args:
        blueprint_path: Path to the Blueprint asset
        function_id: ID of the function to add the node to
        node_type: Type of node to add. Common supported types include:
            - Basic nodes: "ReturnNode", "FunctionEntry", "Branch", "Sequence"
            - Math operations: "Multiply", "Add", "Subtract", "Divide"
            - Utilities: "PrintString", "Delay", "GetActorLocation", "SetActorLocation"
            - For other functions, try using the exact function name from Blueprints
              (e.g., "GetWorldLocation", "SpawnActorFromClass")
            If the requested node type isn't found, the system will search for alternatives
            and return suggestions. You can then use these suggestions in a new request.
        node_position: Position of the node in the graph [X, Y]. **IMPORTANT**: Space nodes
            at least 400 units apart horizontally and 300 units vertically to avoid overlap
            and ensure a clean, organized graph (e.g., [0, 0], [400, 0], [800, 0] for a chain).
        node_properties: Properties to set on the node (optional)

    Returns:
        On success: The node ID (GUID)
        On failure: A response containing "SUGGESTIONS:" followed by alternative node types to try

    Note:
        Function libraries like KismetMathLibrary, KismetSystemLibrary, and KismetStringLibrary
        contain most common Blueprint functions. If a simple node name doesn't work, try the
        full function name, e.g., "Multiply_FloatFloat" instead of just "Multiply".
    """
    if node_properties is None:
        node_properties = {}

    command = {
        "type": "add_node",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "node_type": node_type,
        "node_position": node_position,
        "node_properties": node_properties,
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully added {node_type} node to function {function_id} in Blueprint at {blueprint_path} with ID: {response.get('node_id', 'unknown')}"
    else:
        return f"Failed to add node: {response.get('error', 'Unknown error')}"


@mcp.tool()
def get_node_suggestions(node_type: str) -> str:
    """
    Get suggestions for a node type in Unreal Blueprints

    Args:
        node_type: The partial or full node type to get suggestions for (e.g., "Add", "FloatToDouble")

    Returns:
        A string indicating success with suggestions or an error message
    """
    command = {"type": "get_node_suggestions", "node_type": node_type}

    response = send_to_unreal(command)
    if response.get("success"):
        suggestions = response.get("suggestions", [])
        if suggestions:
            return f"Suggestions for '{node_type}': {', '.join(suggestions)}"
        else:
            return f"No suggestions found for '{node_type}'"
    else:
        error = response.get("error", "Unknown error")
        return f"Failed to get suggestions for '{node_type}': {error}"


@mcp.tool()
def delete_node_from_blueprint(
    blueprint_path: str, function_id: str, node_id: str
) -> str:
    """
    Delete a node from a Blueprint graph

    Args:
        blueprint_path: Path to the Blueprint asset
        function_id: ID of the function containing the node
        node_id: ID of the node to delete

    Returns:
        Success or failure message
    """
    command = {
        "type": "delete_node",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "node_id": node_id,
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully deleted node {node_id} from function {function_id} in Blueprint at {blueprint_path}"
    else:
        return f"Failed to delete node: {response.get('error', 'Unknown error')}"


@mcp.tool()
def get_all_nodes_in_graph(blueprint_path: str, function_id: str) -> str:
    """
    Get all nodes in a Blueprint graph with their positions and types

    Args:
        blueprint_path: Path to the Blueprint asset
        function_id: ID of the function to get nodes from

    Returns:
        JSON string containing all nodes with their GUIDs, types, and positions
    """
    command = {
        "type": "get_all_nodes",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return response.get("nodes", "[]")
    else:
        return f"Failed to get nodes: {response.get('error', 'Unknown error')}"


@mcp.tool()
def connect_blueprint_nodes(
    blueprint_path: str,
    function_id: str,
    source_node_id: str,
    source_pin: str,
    target_node_id: str,
    target_pin: str,
) -> str:
    command = {
        "type": "connect_nodes",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "source_node_id": source_node_id,
        "source_pin": source_pin,
        "target_node_id": target_node_id,
        "target_pin": target_pin,
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully connected {source_node_id}.{source_pin} to {target_node_id}.{target_pin} in Blueprint at {blueprint_path}"
    else:
        error = response.get("error", "Unknown error")
        if "source_available_pins" in response and "target_available_pins" in response:
            error += f"\nAvailable pins on source ({source_node_id}): {json.dumps(response['source_available_pins'], indent=2)}"
            error += f"\nAvailable pins on target ({target_node_id}): {json.dumps(response['target_available_pins'], indent=2)}"
        return f"Failed to connect nodes: {error}"


@mcp.tool()
def compile_blueprint(blueprint_path: str) -> str:
    """
    Compile a Blueprint

    Args:
        blueprint_path: Path to the Blueprint asset

    Returns:
        Message indicating success or failure
    """
    command = {"type": "compile_blueprint", "blueprint_path": blueprint_path}

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully compiled Blueprint at {blueprint_path}"
    else:
        return f"Failed to compile Blueprint: {response.get('error', 'Unknown error')}"


@mcp.tool()
def spawn_blueprint_actor(
    blueprint_path: str,
    location: list = [0, 0, 0],
    rotation: list = [0, 0, 0],
    scale: list = [1, 1, 1],
    actor_label: str = None,
) -> str:
    """
    Spawn a Blueprint actor in the level

    Args:
        blueprint_path: Path to the Blueprint asset
        location: [X, Y, Z] coordinates
        rotation: [Pitch, Yaw, Roll] in degrees
        scale: [X, Y, Z] scale factors
        actor_label: Optional custom name for the actor

    Returns:
        Message indicating success or failure
    """
    command = {
        "type": "spawn_blueprint",
        "blueprint_path": blueprint_path,
        "location": location,
        "rotation": rotation,
        "scale": scale,
        "actor_label": actor_label,
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully spawned Blueprint {blueprint_path}" + (
            f" with label '{actor_label}'" if actor_label else ""
        )
    else:
        return f"Failed to spawn Blueprint: {response.get('error', 'Unknown error')}"


# @mcp.tool()
# def add_nodes_to_blueprint_bulk(blueprint_path: str, function_id: str, nodes: list) -> str:
#     """
#     Add multiple nodes to a Blueprint graph in a single operation
#
#     Args:
#         blueprint_path: Path to the Blueprint asset
#         function_id: ID of the function to add the nodes to
#         nodes: Array of node definitions, each containing:
#             - id: ID for referencing the node (string) - this is important for creating connections later
#             - node_type: Type of node to add (see add_node_to_blueprint for supported types)
#             - node_position: Position of the node in the graph [X, Y]
#             - node_properties: Properties to set on the node (optional)
#
#     Returns:
#         On success: Dictionary mapping your node IDs to the actual node GUIDs created in Unreal
#         On partial success: Dictionary with successful nodes and suggestions for failed nodes
#         On failure: Error message with suggestions
#
#     Example success response:
#         {
#           "success": true,
#           "nodes": {
#             "function_entry": "425E7A3949D7420A461175A4733BBA5C",
#             "multiply_node": "70354A7E444BB68EEF31718DC50CF89C",
#             "return_node": "6436796645ED674F3C64A8A94CBA416C"
#           }
#         }
#
#     Example partial success with suggestions:
#         {
#           "success": true,
#           "partial_success": true,
#           "nodes": {
#             "function_entry": "425E7A3949D7420A461175A4733BBA5C",
#             "return_node": "6436796645ED674F3C64A8A94CBA416C"
#           },
#           "suggestions": {
#             "multiply_node": {
#               "requested_type": "Multiply_Float",
#               "suggestions": ["KismetMathLibrary.Multiply_FloatFloat", "KismetMathLibrary.MultiplyByFloat"]
#             }
#           }
#         }
#
#     When you receive suggestions, you can retry adding those nodes using the suggested node types.
#     """
#     command = {
#         "type": "add_nodes_bulk",
#         "blueprint_path": blueprint_path,
#         "function_id": function_id,
#         "nodes": nodes
#     }
#
#     response = send_to_unreal(command)
#     if response.get("success"):
#         node_mapping = response.get("nodes", {})
#         return f"Successfully added {len(node_mapping)} nodes to function {function_id} in Blueprint at {blueprint_path}\nNode mapping: {json.dumps(node_mapping, indent=2)}"
#     else:
#         return f"Failed to add nodes: {response.get('error', 'Unknown error')}"


@mcp.tool()
def add_component_with_events(
    blueprint_path: str, component_name: str, component_class: str
) -> str:
    """
    Add a component to a Blueprint with overlap events if applicable.

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/FlappyBird/BP_FlappyBird")
        component_name: Name of the new component (e.g., "TriggerBox")
        component_class: Class of the component (e.g., "BoxComponent")

    Returns:
        Message with success, error, and event GUIDs if created
    """
    command = {
        "type": "add_component_with_events",
        "blueprint_path": blueprint_path,
        "component_name": component_name,
        "component_class": component_class,
    }
    response = send_to_unreal(command)
    try:
        import json

        result = json.loads(response)
        if result.get("success"):
            msg = result.get("message", f"Added component {component_name}")
            if "events" in result:
                events = json.loads(result["events"])
                if events["begin_guid"] or events["end_guid"]:
                    msg += f"\nOverlap Events - Begin GUID: {events['begin_guid']}, End GUID: {events['end_guid']}"
            return msg
        return f"Failed: {result.get('error', 'Unknown error')}"
    except Exception as e:
        return f"Error parsing response: {str(e)}\nRaw response: {response}"


@mcp.tool()
def connect_blueprint_nodes_bulk(
    blueprint_path: str, function_id: str, connections: list
) -> str:
    """
    Connect multiple pairs of nodes in a Blueprint graph

    Args:
        blueprint_path: Path to the Blueprint asset
        function_id: ID of the function containing the nodes
        connections: Array of connection definitions, each containing:
            - source_node_id: ID of the source node
            - source_pin: Name of the source pin
            - target_node_id: ID of the target node
            - target_pin: Name of the target pin

    Returns:
        Message indicating success or failure, with details on which connections succeeded or failed
    """
    command = {
        "type": "connect_nodes_bulk",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "connections": connections,
    }

    response = send_to_unreal(command)

    # Handle the new detailed response format
    if response.get("success"):
        successful = response.get("successful_connections", 0)
        total = response.get("total_connections", 0)
        return f"Successfully connected {successful}/{total} node pairs in Blueprint at {blueprint_path}"
    else:
        # Extract detailed error information
        error_message = response.get("error", "Unknown error")
        failed_connections = []

        # Look for detailed results in the response
        if "results" in response:
            for result in response.get("results", []):
                if not result.get("success", False):
                    idx = result.get("connection_index", -1)
                    src = result.get("source_node", "unknown")
                    tgt = result.get("target_node", "unknown")
                    err = result.get("error", "unknown error")
                    failed_connections.append(
                        f"Connection {idx}: {src} to {tgt} - {err}"
                    )

        # Format error message with details
        if failed_connections:
            detailed_errors = "\n- " + "\n- ".join(failed_connections)
            return f"Failed to connect nodes: {error_message}{detailed_errors}"
        else:
            return f"Failed to connect nodes: {error_message}"


@mcp.tool()
def get_blueprint_node_guid(
    blueprint_path: str,
    graph_type: str = "EventGraph",
    node_name: str = None,
    function_id: str = None,
) -> str:
    """
    Retrieve the GUID of a pre-existing node in a Blueprint graph.

    Args:
        blueprint_path: Path to the Blueprint asset (e.g., "/Game/Blueprints/TestBulkBlueprint")
        graph_type: Type of graph to query ("EventGraph" or "FunctionGraph", default: "EventGraph")
        node_name: Name of the node to find (e.g., "BeginPlay" for EventGraph, optional if using function_id)
        function_id: ID of the function to get the FunctionEntry node for (optional, used with graph_type="FunctionGraph")

    Returns:
        Message with the node's GUID or an error if not found
    """
    command = {
        "type": "get_node_guid",
        "blueprint_path": blueprint_path,
        "graph_type": graph_type,
        "node_name": node_name if node_name else "",
        "function_id": function_id if function_id else "",
    }

    response = send_to_unreal(command)
    if response.get("success"):
        guid = response.get("node_guid")
        return f"Node GUID for {node_name or 'FunctionEntry'} in {graph_type} of {blueprint_path}: {guid}"
    else:
        return f"Failed to get node GUID: {response.get('error', 'Unknown error')}"


# Safety check for potentially destructive actions
def is_potentially_destructive(script: str) -> bool:
    """
    Check if the script contains potentially destructive actions like deleting or saving files.
    Returns True if such actions are detected and not explicitly requested.
    """
    destructive_keywords = [
        r"unreal\.EditorAssetLibrary\.delete_asset",
        r"unreal\.EditorLevelLibrary\.destroy_actor",
        r"unreal\.save_package",
        r"os\.remove",
        r"shutil\.rmtree",
        r"file\.write",
        r"unreal\.EditorAssetLibrary\.save_asset",
    ]
    for keyword in destructive_keywords:
        if re.search(keyword, script, re.IGNORECASE):
            return True
    return False


# Scene Control
@mcp.tool()
def get_all_scene_objects() -> str:
    """
    Retrieve all actors in the current Unreal Engine level.

    Returns:
        JSON string of actors with their names, classes, and locations.
    """
    command = {"type": "get_all_scene_objects"}
    response = send_to_unreal(command)
    return (
        json.dumps(response)
        if response.get("success")
        else f"Failed: {response.get('error', 'Unknown error')}"
    )


# Project Control
@mcp.tool()
def create_project_folder(folder_path: str) -> str:
    """
    Create a new folder in the Unreal project content directory.

    Args:
        folder_path: Path relative to /Game (e.g., "FlappyBird/Assets")
    """
    command = {"type": "create_project_folder", "folder_path": folder_path}
    response = send_to_unreal(command)
    return response.get("message", f"Failed: {response.get('error', 'Unknown error')}")


@mcp.tool()
def get_files_in_folder(folder_path: str) -> str:
    """
    List all files in a specified project folder.

    Args:
        folder_path: Path relative to /Game (e.g., "FlappyBird/Assets")
    """
    command = {"type": "get_files_in_folder", "folder_path": folder_path}
    response = send_to_unreal(command)
    return (
        json.dumps(response.get("files", []))
        if response.get("success")
        else f"Failed: {response.get('error')}"
    )


@mcp.tool()
def create_game_mode(
    game_mode_path: str, pawn_blueprint_path: str, base_class: str = "GameModeBase"
) -> str:
    """Create a game mode Blueprint, set its default pawn, and assign it as the current scene’s default game mode.
    Args:
        game_mode_path: Path for new game mode (e.g., "/Game/MyGameMode")
        pawn_blueprint_path: Path to pawn Blueprint (e.g., "/Game/Blueprints/BP_Player")
        base_class: Base class for game mode (default: "GameModeBase")
    """
    try:
        command = {
            "type": "create_game_mode",
            "game_mode_path": game_mode_path,
            "pawn_blueprint_path": pawn_blueprint_path,
            "base_class": base_class,
        }
        response = send_to_unreal(command)
        parsed = json.loads(response)
        return parsed.get("message", f"Failed: {parsed.get('error')}")
    except Exception as e:
        return f"Error creating game mode: {str(e)}"


@mcp.tool()
def add_widget_to_user_widget(
    user_widget_path: str,
    widget_type: str,
    widget_name: str,
    parent_widget_name: str = "",
) -> str:
    """
    Adds a new widget (like TextBlock, Button, Image, CanvasPanel, VerticalBox) to a User Widget Blueprint.

    Args:
        user_widget_path: Path to the User Widget Blueprint (e.g., "/Game/UI/WBP_MainMenu").
        widget_type: Class name of the widget to add (e.g., "TextBlock", "Button", "Image", "CanvasPanel", "VerticalBox", "HorizontalBox", "SizeBox", "Border"). Case-sensitive.
        widget_name: A unique desired name for the new widget variable (e.g., "TitleText", "StartButton", "PlayerHealthBar"). The actual name might get adjusted for uniqueness.
        parent_widget_name: Optional. The name of an existing Panel widget (like CanvasPanel, VerticalBox) inside the User Widget to attach this new widget to. If empty, attempts to attach to the root or the first available CanvasPanel.

    Returns:
        JSON string indicating success (with actual widget name) or failure with an error message.
    """
    command = {
        "type": "add_widget_to_user_widget",
        "user_widget_path": user_widget_path,
        "widget_type": widget_type,
        "widget_name": widget_name,
        "parent_widget_name": parent_widget_name,
    }
    # Use json.loads to parse the JSON string returned by send_to_unreal
    response_str = send_to_unreal(command)
    try:
        response_dict = json.loads(response_str)
        # Return a user-friendly string summary
        if response_dict.get("success"):
            actual_name = response_dict.get("widget_name", widget_name)
            return response_dict.get(
                "message",
                f"Successfully added widget '{actual_name}' of type '{widget_type}' to '{user_widget_path}'.",
            )
        else:
            return f"Failed to add widget: {response_dict.get('error', 'Unknown C++ error')}"
    except json.JSONDecodeError:
        return f"Failed to parse response from Unreal: {response_str}"
    except Exception as e:
        # Catch potential errors if send_to_unreal itself failed before returning JSON
        if isinstance(response_str, dict) and not response_str.get("success"):
            return f"Failed to send command: {response_str.get('error', 'Unknown MCP error')}"
        return f"An unexpected error occurred: {str(e)} Response: {response_str}"


@mcp.tool()
def edit_widget_property(
    user_widget_path: str, widget_name: str, property_name: str, value: str
) -> str:
    """
    Edits a property of a specific widget within a User Widget Blueprint.

    Args:
        user_widget_path: Path to the User Widget Blueprint (e.g., "/Game/UI/WBP_MainMenu").
        widget_name: The name of the widget inside the User Widget whose property you want to change (e.g., "TitleText", "StartButton").
        property_name: The name of the property to edit. For layout properties controlled by the parent panel (like position, size, anchors in a CanvasPanel), prefix with "Slot." (e.g., "Text", "ColorAndOpacity", "Brush.ImageSize", "Slot.Position", "Slot.Size", "Slot.Anchors", "Slot.Alignment"). Case-sensitive.
        value: The new value for the property, formatted as a string EXACTLY as Unreal expects for ImportText. Examples:
            - Text: '"Hello World!"' (Note: String literal requires inner quotes)
            - Float: '150.0'
            - Integer: '10'
            - Boolean: 'true' or 'false'
            - LinearColor: '(R=1.0,G=0.0,B=0.0,A=1.0)'
            - Vector2D (for Size, Position): '(X=200.0,Y=50.0)'
            - Anchors: '(Minimum=(X=0.5,Y=0.0),Maximum=(X=0.5,Y=0.0))' (Top Center Anchor)
            - Alignment (Vector2D): '(X=0.5,Y=0.5)' (Center Alignment)
            - Font (FSlateFontInfo): "(FontObject=Font'/Engine/EngineFonts/Roboto.Roboto',Size=24)"
            - Texture (Object Path): "Texture2D'/Game/Textures/MyIcon.MyIcon'"
            - Enum (e.g., Stretch): 'ScaleToFit'

    Returns:
        JSON string indicating success or failure with an error message.
    """
    command = {
        "type": "edit_widget_property",
        "user_widget_path": user_widget_path,
        "widget_name": widget_name,
        "property_name": property_name,
        "value": value,  # Pass the string value directly
    }
    # Use json.loads to parse the JSON string returned by send_to_unreal
    response_str = send_to_unreal(command)
    try:
        response_dict = json.loads(response_str)
        # Return a user-friendly string summary
        if response_dict.get("success"):
            return response_dict.get(
                "message",
                f"Successfully set property '{property_name}' on widget '{widget_name}'.",
            )
        else:
            return f"Failed to edit widget property: {response_dict.get('error', 'Unknown C++ error')}"
    except json.JSONDecodeError:
        return f"Failed to parse response from Unreal: {response_str}"
    except Exception as e:
        # Catch potential errors if send_to_unreal itself failed before returning JSON
        if isinstance(response_str, dict) and not response_str.get("success"):
            return f"Failed to send command: {response_str.get('error', 'Unknown MCP error')}"
        return f"An unexpected error occurred: {str(e)} Response: {response_str}"


# Input
@mcp.tool()
def add_input_binding(action_name: str, key: str) -> str:
    """
    Add an input action binding to Project Settings.

    Args:
        action_name: Name of the action (e.g., "Flap")
        key: Key to bind (e.g., "Space Bar")
    """
    command = {"type": "add_input_binding", "action_name": action_name, "key": key}
    response = send_to_unreal(command)
    return response.get("message", f"Failed: {response.get('error')}")


if __name__ == "__main__":
    import traceback

    try:
        print("Server starting...", file=sys.stderr)
        mcp.run()
    except Exception as e:
        print(f"Server crashed with error: {e}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        raise
