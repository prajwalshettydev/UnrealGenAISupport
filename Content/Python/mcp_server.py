import socket
import json
import sys
from mcp.server.fastmcp import FastMCP

# THIS FILE WILL RUN OUTSIDE THE UNREAL ENGINE SCOPE, 
# DO NOT IMPORT UNREAL MODULES HERE OR EXECUTE IT IN THE UNREAL ENGINE PYTHON INTERPRETER

# Create an MCP server
mcp = FastMCP("UnrealHandshake")

# Function to send a message to Unreal Engine via socket
def send_to_unreal(command):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.connect(('localhost', 9877))  # Unreal listens on port 9877
            s.sendall(json.dumps(command).encode())
            response = s.recv(4096)  # Increased buffer size
            return json.loads(response.decode())
        except Exception as e:
            print(f"Error sending to Unreal: {e}", file=sys.stderr)
            return {"success": False, "error": str(e)}


# Define basic tools for Claude to call

@mcp.tool()
def handshake_test(message: str) -> str:
    """Send a handshake message to Unreal Engine"""
    try:
        command = {
            "type": "handshake",
            "message": message
        }
        response = send_to_unreal(command)
        if response.get("success"):
            return f"Handshake successful: {response['message']}"
        else:
            return f"Handshake failed: {response.get('error', 'Unknown error')}"
    except Exception as e:
        return f"Error communicating with Unreal: {str(e)}"


#
# Basic Object Commands
#

@mcp.tool()
def spawn_object(actor_class: str, location: list = [0, 0, 0], rotation: list = [0, 0, 0],
                 scale: list = [1, 1, 1], actor_label: str = None) -> str:
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
        "actor_label": actor_label
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully spawned {actor_class}" + (f" with label '{actor_label}'" if actor_label else "")
    else:
        error = response.get('error', 'Unknown error')
        # Add hint for Claude to understand what went wrong
        if "not found" in error:
            hint = "\nHint: For basic shapes, use 'Cube', 'Sphere', 'Cylinder', or 'Cone'. For other actors, try using '/Script/Engine.PointLight' format."
            error += hint
        return f"Failed to spawn object: {error}"

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
        "color": color
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully created material '{material_name}' with path: {response.get('material_path')}"
    else:
        return f"Failed to create material: {response.get('error', 'Unknown error')}"

@mcp.tool()
def set_object_material(actor_name: str, material_path: str) -> str:
    """
    Set the material of an object in the level
    
    Args:
        actor_name: Name of the actor to modify
        material_path: Path to the material asset (e.g., "/Game/Materials/MyMaterial")
        
    Returns:
        Message indicating success or failure
    """
    command = {
        "type": "modify_object",
        "actor_name": actor_name,
        "property_type": "material",
        "value": material_path
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully set material of '{actor_name}' to {material_path}"
    else:
        return f"Failed to set material: {response.get('error', 'Unknown error')}"

@mcp.tool()
def set_object_position(actor_name: str, position: list) -> str:
    """
    Set the position of an object in the level
    
    Args:
        actor_name: Name of the actor to modify
        position: [X, Y, Z] coordinates
        
    Returns:
        Message indicating success or failure
    """
    command = {
        "type": "modify_object",
        "actor_name": actor_name,
        "property_type": "position",
        "value": position
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully set position of '{actor_name}' to {position}"
    else:
        return f"Failed to set position: {response.get('error', 'Unknown error')}"

@mcp.tool()
def set_object_rotation(actor_name: str, rotation: list) -> str:
    """
    Set the rotation of an object in the level

    Args:
        actor_name: Name of the actor to modify
        rotation: [Pitch, Yaw, Roll] in degrees

    Returns:
        Message indicating success or failure
    """
    command = {
        "type": "modify_object",
        "actor_name": actor_name,
        "property_type": "rotation",
        "value": rotation
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully set rotation of '{actor_name}' to {rotation}"
    else:
        return f"Failed to set rotation: {response.get('error', 'Unknown error')}"

@mcp.tool()
def set_object_scale(actor_name: str, scale: list) -> str:
    """
    Set the scale of an object in the level

    Args:
        actor_name: Name of the actor to modify
        scale: [X, Y, Z] scale factors

    Returns:
        Message indicating success or failure
    """
    command = {
        "type": "modify_object",
        "actor_name": actor_name,
        "property_type": "scale",
        "value": scale
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully set scale of '{actor_name}' to {scale}"
    else:
        return f"Failed to set scale: {response.get('error', 'Unknown error')}"


#
# Blueprint Commands
#

@mcp.tool()
def create_blueprint(blueprint_name: str, parent_class: str = "Actor", save_path: str = "/Game/Blueprints") -> str:
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
        "save_path": save_path
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully created Blueprint '{blueprint_name}' with path: {response.get('blueprint_path', save_path + '/' + blueprint_name)}"
    else:
        return f"Failed to create Blueprint: {response.get('error', 'Unknown error')}"

@mcp.tool()
def add_component_to_blueprint(blueprint_path: str, component_class: str, component_name: str = None) -> str:
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
        "component_name": component_name
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully added {component_class} to Blueprint at {blueprint_path}"
    else:
        return f"Failed to add component: {response.get('error', 'Unknown error')}"

@mcp.tool()
def add_variable_to_blueprint(blueprint_path: str, variable_name: str, variable_type: str,
                              default_value: str = None, category: str = "Default") -> str:
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
    command = {
        "type": "add_variable",
        "blueprint_path": blueprint_path,
        "variable_name": variable_name,
        "variable_type": variable_type,
        "default_value": default_value,
        "category": category
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully added {variable_type} variable '{variable_name}' to Blueprint at {blueprint_path}"
    else:
        return f"Failed to add variable: {response.get('error', 'Unknown error')}"

@mcp.tool()
def add_function_to_blueprint(blueprint_path: str, function_name: str,
                              inputs: list = None, outputs: list = None) -> str:
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
        "outputs": outputs
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully added function '{function_name}' to Blueprint at {blueprint_path} with ID: {response.get('function_id', 'unknown')}"
    else:
        return f"Failed to add function: {response.get('error', 'Unknown error')}"

@mcp.tool()
def add_node_to_blueprint(blueprint_path: str, function_id: str, node_type: str,
                          node_position: list = [0, 0], node_properties: dict = None) -> str:
    """
    Add a node to a Blueprint graph
    
    Args:
        blueprint_path: Path to the Blueprint asset
        function_id: ID of the function to add the node to
        node_type: Type of node to add (e.g., "K2_SetActorLocation", "Branch")
        node_position: Position of the node in the graph [X, Y]
        node_properties: Properties to set on the node (optional)
        
    Returns:
        Message indicating success or failure
    """
    if node_properties is None:
        node_properties = {}

    command = {
        "type": "add_node",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "node_type": node_type,
        "node_position": node_position,
        "node_properties": node_properties
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully added {node_type} node to function {function_id} in Blueprint at {blueprint_path} with ID: {response.get('node_id', 'unknown')}"
    else:
        return f"Failed to add node: {response.get('error', 'Unknown error')}"

@mcp.tool()
def connect_blueprint_nodes(blueprint_path: str, function_id: str,
                            source_node_id: str, source_pin: str,
                            target_node_id: str, target_pin: str) -> str:
    """
    Connect nodes in a Blueprint graph
    
    Args:
        blueprint_path: Path to the Blueprint asset
        function_id: ID of the function containing the nodes
        source_node_id: ID of the source node
        source_pin: Name of the source pin
        target_node_id: ID of the target node
        target_pin: Name of the target pin
        
    Returns:
        Message indicating success or failure
    """
    command = {
        "type": "connect_nodes",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "source_node_id": source_node_id,
        "source_pin": source_pin,
        "target_node_id": target_node_id,
        "target_pin": target_pin
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully connected {source_node_id}.{source_pin} to {target_node_id}.{target_pin} in Blueprint at {blueprint_path}"
    else:
        return f"Failed to connect nodes: {response.get('error', 'Unknown error')}"

@mcp.tool()
def compile_blueprint(blueprint_path: str) -> str:
    """
    Compile a Blueprint
    
    Args:
        blueprint_path: Path to the Blueprint asset
        
    Returns:
        Message indicating success or failure
    """
    command = {
        "type": "compile_blueprint",
        "blueprint_path": blueprint_path
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully compiled Blueprint at {blueprint_path}"
    else:
        return f"Failed to compile Blueprint: {response.get('error', 'Unknown error')}"

@mcp.tool()
def spawn_blueprint_actor(blueprint_path: str, location: list = [0, 0, 0],
                          rotation: list = [0, 0, 0], scale: list = [1, 1, 1],
                          actor_label: str = None) -> str:
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
        "actor_label": actor_label
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully spawned Blueprint {blueprint_path}" + (f" with label '{actor_label}'" if actor_label else "")
    else:
        return f"Failed to spawn Blueprint: {response.get('error', 'Unknown error')}"


@mcp.tool()
def add_nodes_to_blueprint_bulk(blueprint_path: str, function_id: str, nodes: list) -> str:
    """
    Add multiple nodes to a Blueprint graph in a single operation
    
    Args:
        blueprint_path: Path to the Blueprint asset
        function_id: ID of the function to add the nodes to
        nodes: Array of node definitions, each containing:
            - id: Optional ID for referencing the node (string)
            - node_type: Type of node to add (string) 
            - node_position: Position of the node in the graph [X, Y]
            - node_properties: Properties to set on the node (optional)
        
    Returns:
        Message indicating success or failure with node mapping
    """
    command = {
        "type": "add_nodes_bulk",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "nodes": nodes
    }

    response = send_to_unreal(command)
    if response.get("success"):
        node_mapping = response.get("nodes", {})
        return f"Successfully added {len(node_mapping)} nodes to function {function_id} in Blueprint at {blueprint_path}\nNode mapping: {json.dumps(node_mapping, indent=2)}"
    else:
        return f"Failed to add nodes: {response.get('error', 'Unknown error')}"

@mcp.tool()
def connect_blueprint_nodes_bulk(blueprint_path: str, function_id: str, connections: list) -> str:
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
        Message indicating success or failure
    """
    command = {
        "type": "connect_nodes_bulk",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "connections": connections
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully connected {len(connections)} node pairs in Blueprint at {blueprint_path}"
    else:
        return f"Failed to connect nodes: {response.get('error', 'Unknown error')}"
@mcp.tool()
def get_blueprint_node_guid(blueprint_path: str, graph_type: str = "EventGraph", node_name: str = None, function_id: str = None) -> str:
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
        "function_id": function_id if function_id else ""
    }

    response = send_to_unreal(command)
    if response.get("success"):
        guid = response.get("node_guid")
        return f"Node GUID for {node_name or 'FunctionEntry'} in {graph_type} of {blueprint_path}: {guid}"
    else:
        return f"Failed to get node GUID: {response.get('error', 'Unknown error')}"


if __name__ == "__main__":
    import traceback
    try:
        print("Server starting...", file=sys.stderr)
        mcp.run()
    except Exception as e:
        print(f"Server crashed with error: {e}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        raise