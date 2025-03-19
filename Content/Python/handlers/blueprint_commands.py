import unreal
import json
from typing import Dict, Any, List, Tuple, Union, Optional

from utils import unreal_conversions as uc
from utils import logging as log


def handle_create_blueprint(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to create a new Blueprint from a specified parent class
    
    Args:
        command: The command dictionary containing:
            - blueprint_name: Name for the new Blueprint
            - parent_class: Parent class name or path (e.g., "Actor", "/Script/Engine.Actor")
            - save_path: Path to save the Blueprint asset (e.g., "/Game/Blueprints")
            
    Returns:
        Response dictionary with success/failure status and the Blueprint path if successful
    """
    try:
        blueprint_name = command.get("blueprint_name", "NewBlueprint")
        parent_class = command.get("parent_class", "Actor")
        save_path = command.get("save_path", "/Game/Blueprints")

        log.log_command("create_blueprint", f"Name: {blueprint_name}, Parent: {parent_class}")

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        blueprint = gen_bp_utils.create_blueprint(blueprint_name, parent_class, save_path)

        if blueprint:
            blueprint_path = f"{save_path}/{blueprint_name}"
            log.log_result("create_blueprint", True, f"Path: {blueprint_path}")
            return {"success": True, "blueprint_path": blueprint_path}
        else:
            log.log_error(f"Failed to create Blueprint {blueprint_name}")
            return {"success": False, "error": f"Failed to create Blueprint {blueprint_name}"}

    except Exception as e:
        log.log_error(f"Error creating blueprint: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_add_component(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to add a component to a Blueprint
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - component_class: Component class to add (e.g., "StaticMeshComponent")
            - component_name: Name for the new component
            
    Returns:
        Response dictionary with success/failure status
    """
    try:
        blueprint_path = command.get("blueprint_path")
        component_class = command.get("component_class")
        component_name = command.get("component_name")

        if not blueprint_path or not component_class:
            log.log_error("Missing required parameters for add_component")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("add_component", f"Blueprint: {blueprint_path}, Component: {component_class}")

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        success = gen_bp_utils.add_component(blueprint_path, component_class, component_name or "")

        if success:
            log.log_result("add_component", True, f"Added {component_class} to {blueprint_path}")
            return {"success": True}
        else:
            log.log_error(f"Failed to add component {component_class} to {blueprint_path}")
            return {"success": False, "error": f"Failed to add component {component_class} to {blueprint_path}"}

    except Exception as e:
        log.log_error(f"Error adding component: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_add_variable(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to add a variable to a Blueprint
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - variable_name: Name for the new variable
            - variable_type: Type of the variable (e.g., "float", "vector", "boolean")
            - default_value: Default value for the variable (optional)
            - category: Category for organizing variables in the Blueprint editor (optional)
            
    Returns:
        Response dictionary with success/failure status
    """
    try:
        blueprint_path = command.get("blueprint_path")
        variable_name = command.get("variable_name")
        variable_type = command.get("variable_type")
        default_value = command.get("default_value", "")
        category = command.get("category", "Default")

        if not blueprint_path or not variable_name or not variable_type:
            log.log_error("Missing required parameters for add_variable")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("add_variable", f"Blueprint: {blueprint_path}, Variable: {variable_name}, Type: {variable_type}")

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        success = gen_bp_utils.add_variable(blueprint_path, variable_name, variable_type, str(default_value), category)

        if success:
            log.log_result("add_variable", True, f"Added {variable_type} variable {variable_name} to {blueprint_path}")
            return {"success": True}
        else:
            log.log_error(f"Failed to add variable {variable_name} to {blueprint_path}")
            return {"success": False, "error": f"Failed to add variable {variable_name} to {blueprint_path}"}

    except Exception as e:
        log.log_error(f"Error adding variable: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_add_function(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to add a function to a Blueprint
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - function_name: Name for the new function
            - inputs: List of input parameters [{"name": "param1", "type": "float"}, ...] (optional)
            - outputs: List of output parameters (optional)
            
    Returns:
        Response dictionary with success/failure status and the function ID if successful
    """
    try:
        blueprint_path = command.get("blueprint_path")
        function_name = command.get("function_name")
        inputs = command.get("inputs", [])
        outputs = command.get("outputs", [])

        if not blueprint_path or not function_name:
            log.log_error("Missing required parameters for add_function")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("add_function", f"Blueprint: {blueprint_path}, Function: {function_name}")

        # Convert inputs and outputs to JSON strings for C++ function
        inputs_json = json.dumps(inputs)
        outputs_json = json.dumps(outputs)

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        function_id = gen_bp_utils.add_function(blueprint_path, function_name, inputs_json, outputs_json)

        if function_id:
            log.log_result("add_function", True, f"Added function {function_name} to {blueprint_path} with ID: {function_id}")
            return {"success": True, "function_id": function_id}
        else:
            log.log_error(f"Failed to add function {function_name} to {blueprint_path}")
            return {"success": False, "error": f"Failed to add function {function_name} to {blueprint_path}"}

    except Exception as e:
        log.log_error(f"Error adding function: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_add_node(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to add any type of node to a Blueprint graph
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - function_id: ID of the function to add the node to
            - node_type: Type of node to add - can be any of:
                * Function name (e.g. "K2_SetActorLocation")
                * Node class name (e.g. "Branch", "Sequence", "ForLoop")
                * Full node class path (e.g. "K2Node_IfThenElse")
            - node_position: Position of the node in the graph [X, Y]
            - node_properties: Dictionary of properties to set on the node (optional)
                * Can include pin values, node settings, etc.
            - target_class: Optional class to use for function calls (default: "Actor")
                
    Returns:
        Response dictionary with success/failure status and the node ID if successful
    """
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")
        node_type = command.get("node_type")
        node_position = command.get("node_position", [0, 0])
        node_properties = command.get("node_properties", {})

        if not blueprint_path or not function_id or not node_type:
            log.log_error("Missing required parameters for add_node")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("add_node", f"Blueprint: {blueprint_path}, Node: {node_type}")

        # Convert node properties to JSON for C++ function
        node_properties_json = json.dumps(node_properties)

        # Call the C++ implementation - this relies on the C++ code to handle any node type
        gen_bp_utils = unreal.GenBlueprintUtils
        node_id = gen_bp_utils.add_node(blueprint_path, function_id, node_type,
                                        node_position[0], node_position[1],
                                        node_properties_json)

        if node_id:
            log.log_result("add_node", True, f"Added node {node_type} to {blueprint_path} with ID: {node_id}")
            return {"success": True, "node_id": node_id}
        else:
            log.log_error(f"Failed to add node {node_type} to {blueprint_path}")
            return {"success": False, "error": f"Failed to add node {node_type} to {blueprint_path}"}

    except Exception as e:
        log.log_error(f"Error adding node: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_connect_nodes(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to connect nodes in a Blueprint graph
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - function_id: ID of the function containing the nodes
            - source_node_id: ID of the source node
            - source_pin: Name of the source pin
            - target_node_id: ID of the target node
            - target_pin: Name of the target pin
            
    Returns:
        Response dictionary with success/failure status
    """
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")
        source_node_id = command.get("source_node_id")
        source_pin = command.get("source_pin")
        target_node_id = command.get("target_node_id")
        target_pin = command.get("target_pin")

        if not blueprint_path or not function_id or not source_node_id or not source_pin or not target_node_id or not target_pin:
            log.log_error("Missing required parameters for connect_nodes")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("connect_nodes", f"Blueprint: {blueprint_path}, {source_node_id}.{source_pin} -> {target_node_id}.{target_pin}")

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        success = gen_bp_utils.connect_nodes(blueprint_path, function_id,
                                             source_node_id, source_pin,
                                             target_node_id, target_pin)

        if success:
            log.log_result("connect_nodes", True, f"Connected nodes in {blueprint_path}")
            return {"success": True}
        else:
            log.log_error(f"Failed to connect nodes in {blueprint_path}")
            return {"success": False, "error": f"Failed to connect nodes in {blueprint_path}"}

    except Exception as e:
        log.log_error(f"Error connecting nodes: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_compile_blueprint(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to compile a Blueprint
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            
    Returns:
        Response dictionary with success/failure status
    """
    try:
        blueprint_path = command.get("blueprint_path")

        if not blueprint_path:
            log.log_error("Missing required parameters for compile_blueprint")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("compile_blueprint", f"Blueprint: {blueprint_path}")

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        success = gen_bp_utils.compile_blueprint(blueprint_path)

        if success:
            log.log_result("compile_blueprint", True, f"Compiled blueprint: {blueprint_path}")
            return {"success": True}
        else:
            log.log_error(f"Failed to compile blueprint: {blueprint_path}")
            return {"success": False, "error": f"Failed to compile blueprint: {blueprint_path}"}

    except Exception as e:
        log.log_error(f"Error compiling blueprint: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_spawn_blueprint(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to spawn a Blueprint actor in the level
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - location: [X, Y, Z] coordinates (optional)
            - rotation: [Pitch, Yaw, Roll] in degrees (optional)
            - scale: [X, Y, Z] scale factors (optional)
            - actor_label: Optional custom name for the actor
            
    Returns:
        Response dictionary with success/failure status and the actor name if successful
    """
    try:
        blueprint_path = command.get("blueprint_path")
        location = command.get("location", (0, 0, 0))
        rotation = command.get("rotation", (0, 0, 0))
        scale = command.get("scale", (1, 1, 1))
        actor_label = command.get("actor_label", "")

        if not blueprint_path:
            log.log_error("Missing required parameters for spawn_blueprint")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("spawn_blueprint", f"Blueprint: {blueprint_path}, Label: {actor_label}")

        # Convert to Unreal types
        loc = uc.to_unreal_vector(location)
        rot = uc.to_unreal_rotator(rotation)
        scale_vec = uc.to_unreal_vector(scale)

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        actor = gen_bp_utils.spawn_blueprint(blueprint_path, loc, rot, scale_vec, actor_label)

        if actor:
            actor_name = actor.get_actor_label()
            log.log_result("spawn_blueprint", True, f"Spawned blueprint: {blueprint_path} as {actor_name}")
            return {"success": True, "actor_name": actor_name}
        else:
            log.log_error(f"Failed to spawn blueprint: {blueprint_path}")
            return {"success": False, "error": f"Failed to spawn blueprint: {blueprint_path}"}

    except Exception as e:
        log.log_error(f"Error spawning blueprint: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}