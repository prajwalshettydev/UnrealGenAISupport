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


# Define a tool for Claude to call
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
@mcp.tool()
def spawn_object(actor_class: str, location: list = [0, 0, 0], rotation: list = [0, 0, 0],
                 scale: list = [1, 1, 1], actor_label: str = None) -> str:
    """
    Spawn an object in the Unreal Engine level
    
    Args:
        actor_class: Class name of the actor to spawn (e.g., "Cube", "Sphere", "Cone")
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
        return f"Failed to spawn object: {response.get('error', 'Unknown error')}"

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


if __name__ == "__main__":
    import traceback
    try:
        print("Server starting...", file=sys.stderr)
        mcp.run()
    except Exception as e:
        print(f"Server crashed with error: {e}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        raise