import unreal
import json
from typing import Dict, Any, List, Tuple, Union, Optional

from utils import unreal_conversions as uc
from utils import logging as log
from command_registry import registry


@registry.command("create_blueprint", category="blueprint", mutates_blueprint=True)
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


@registry.command("add_component", category="blueprint", mutates_blueprint=True)
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


@registry.command("add_variable", category="blueprint", mutates_blueprint=True)
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

        log.log_command("add_variable",
                        f"Blueprint: {blueprint_path}, Variable: {variable_name}, Type: {variable_type}")

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


@registry.command("add_function", category="blueprint", mutates_blueprint=True)
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
            log.log_result("add_function", True,
                           f"Added function {function_name} to {blueprint_path} with ID: {function_id}")
            return {"success": True, "function_id": function_id}
        else:
            log.log_error(f"Failed to add function {function_name} to {blueprint_path}")
            return {"success": False, "error": f"Failed to add function {function_name} to {blueprint_path}"}

    except Exception as e:
        log.log_error(f"Error adding function: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("add_node", category="blueprint", mutates_blueprint=True)
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

        log.log_command("add_node", f"Blueprint: {blueprint_path}, Node: {node_type}, Props: {node_properties}")

        # Convert node properties to JSON for C++ function
        node_properties_json = json.dumps(node_properties)

        # Call the C++ implementation from UGenBlueprintNodeCreator
        node_creator = unreal.GenBlueprintNodeCreator
        node_id = node_creator.add_node(blueprint_path, function_id, node_type,
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


@registry.command("connect_nodes", category="blueprint", mutates_blueprint=True)
def handle_connect_nodes(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")
        source_node_id = command.get("source_node_id")
        source_pin = command.get("source_pin")
        target_node_id = command.get("target_node_id")
        target_pin = command.get("target_pin")

        if not all([blueprint_path, function_id, source_node_id, source_pin, target_node_id, target_pin]):
            log.log_error("Missing required parameters for connect_nodes")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("connect_nodes",
                        f"Blueprint: {blueprint_path}, {source_node_id}.{source_pin} -> {target_node_id}.{target_pin}")

        gen_bp_utils = unreal.GenBlueprintUtils
        result_json = gen_bp_utils.connect_nodes(blueprint_path, function_id,
                                                 source_node_id, source_pin,
                                                 target_node_id, target_pin)
        result = json.loads(result_json)

        if result.get("success"):
            log.log_result("connect_nodes", True, f"Connected nodes in {blueprint_path}")
            return {"success": True}
        else:
            log.log_error(f"Failed to connect nodes: {result.get('error')}")
            return result  # Pass through the detailed response with available pins

    except Exception as e:
        log.log_error(f"Error connecting nodes: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("compile_blueprint", category="blueprint")
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


@registry.command("spawn_blueprint", category="blueprint")
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

@registry.command("add_nodes_bulk", category="blueprint", mutates_blueprint=True)
def handle_add_nodes_bulk(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to add multiple nodes to a Blueprint graph in a single operation
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - function_id: ID of the function to add the nodes to
            - nodes: Array of node definitions, each containing:
                * id: Optional ID for referencing the node (string)
                * node_type: Type of node to add (string)
                * node_position: Position of the node in the graph [X, Y]
                * node_properties: Properties to set on the node (optional)
            
    Returns:
        Response dictionary with success/failure status and node IDs mapped to reference IDs
    """

    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")
        nodes = command.get("nodes", [])

        if not blueprint_path or not function_id or not nodes:
            log.log_error("Missing required parameters for add_nodes_bulk")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("add_nodes_bulk", f"Blueprint: {blueprint_path}, Adding {len(nodes)} nodes")

        # Prepare nodes in the format expected by the C++ function
        nodes_json = json.dumps(nodes)

        # Call the C++ implementation from UGenBlueprintNodeCreator
        node_creator = unreal.GenBlueprintNodeCreator
        results_json = node_creator.add_nodes_bulk(blueprint_path, function_id, nodes_json)
        
        if results_json:
            results = json.loads(results_json)
            node_mapping = {}
            failed = []

            # Create a mapping from reference IDs to actual node GUIDs
            for node_result in results:
                if node_result.get("ok", True) and "node_guid" in node_result:
                    ref = node_result.get("ref_id", f"node_{len(node_mapping)}")
                    node_mapping[ref] = node_result["node_guid"]
                else:
                    # Node creation failed — collect structured error
                    failed.append({
                        "ref_id": node_result.get("ref_id", ""),
                        "error_code": node_result.get("error_code", "NODE_CREATION_FAILED"),
                        "message": node_result.get("message", "Unknown error"),
                    })

            created_count = len(node_mapping)
            failed_count = len(failed)
            log.log_result("add_nodes_bulk", True,
                           f"Added {created_count} nodes to {blueprint_path}"
                           + (f" ({failed_count} failed)" if failed_count else ""))

            resp = {"success": True, "nodes": node_mapping}
            if failed:
                resp["failed"] = failed
            return resp
        else:
            log.log_error(f"Failed to add nodes to {blueprint_path}")
            return {"success": False, "error": f"Failed to add nodes to {blueprint_path}"}

    except Exception as e:
        log.log_error(f"Error adding nodes: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("connect_nodes_bulk", category="blueprint", mutates_blueprint=True)
def handle_connect_nodes_bulk(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to connect multiple pairs of nodes in a Blueprint graph
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - function_id: ID of the function containing the nodes
            - connections: Array of connection definitions, each containing:
                * source_node_id: ID of the source node
                * source_pin: Name of the source pin
                * target_node_id: ID of the target node
                * target_pin: Name of the target pin
            
    Returns:
        Response dictionary with detailed connection results
    """
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")
        connections = command.get("connections", [])

        if not blueprint_path or not function_id or not connections:
            log.log_error("Missing required parameters for connect_nodes_bulk")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("connect_nodes_bulk", f"Blueprint: {blueprint_path}, Making {len(connections)} connections")

        # Convert connections list to JSON for C++ function
        connections_json = json.dumps(connections)

        # Call the C++ implementation - now returns a JSON string instead of boolean
        gen_bp_utils = unreal.GenBlueprintUtils
        result_json = gen_bp_utils.connect_nodes_bulk(blueprint_path, function_id, connections_json)

        # Parse the JSON result
        try:
            result_data = json.loads(result_json)
            log.log_result("connect_nodes_bulk", result_data.get("success", False),
                           f"Connected {result_data.get('successful_connections', 0)}/{result_data.get('total_connections', 0)} node pairs in {blueprint_path}")

            # Return the full result data for detailed error reporting
            return result_data
        except json.JSONDecodeError:
            log.log_error(f"Failed to parse JSON result from connect_nodes_bulk: {result_json}")
            return {"success": False, "error": "Failed to parse connection results"}

    except Exception as e:
        log.log_error(f"Error connecting nodes: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}

@registry.command("delete_node", category="blueprint", mutates_blueprint=True)
def handle_delete_node(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to delete a node from a Blueprint graph
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - function_id: ID of the function containing the node
            - node_id: ID of the node to delete
                
    Returns:
        Response dictionary with success/failure status
    """
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")
        node_id = command.get("node_id")

        if not blueprint_path or not function_id or not node_id:
            log.log_error("Missing required parameters for delete_node")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("delete_node", f"Blueprint: {blueprint_path}, Node ID: {node_id}")

        # Call the C++ implementation from UGenBlueprintNodeCreator
        node_creator = unreal.GenBlueprintNodeCreator
        success = node_creator.delete_node(blueprint_path, function_id, node_id)

        if success:
            log.log_result("delete_node", True, f"Deleted node {node_id} from {blueprint_path}")
            return {"success": True}
        else:
            log.log_error(f"Failed to delete node {node_id} from {blueprint_path}")
            return {"success": False, "error": f"Failed to delete node {node_id}"}

    except Exception as e:
        log.log_error(f"Error deleting node: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("get_all_nodes", category="blueprint")
def handle_get_all_nodes(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to get all nodes in a Blueprint graph
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - function_id: ID of the function to get nodes from
                
    Returns:
        Response dictionary with success/failure status and a list of nodes with their details
    """
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")

        if not blueprint_path or not function_id:
            log.log_error("Missing required parameters for get_all_nodes")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("get_all_nodes", f"Blueprint: {blueprint_path}, Function ID: {function_id}")

        # Call the C++ implementation from UGenBlueprintNodeCreator
        node_creator = unreal.GenBlueprintNodeCreator
        nodes_json = node_creator.get_all_nodes_in_graph(blueprint_path, function_id)

        if nodes_json:
            # Parse the JSON response
            try:
                nodes = json.loads(nodes_json)
                log.log_result("get_all_nodes", True, f"Retrieved {len(nodes)} nodes from {blueprint_path}")
                return {"success": True, "nodes": nodes}
            except json.JSONDecodeError as e:
                log.log_error(f"Error parsing nodes JSON: {str(e)}")
                return {"success": False, "error": f"Error parsing nodes JSON: {str(e)}"}
        else:
            log.log_error(f"Failed to get nodes from {blueprint_path}")
            return {"success": False, "error": "Failed to get nodes"}

    except Exception as e:
        log.log_error(f"Error getting nodes: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}

@registry.command("get_all_nodes_with_details", category="blueprint")
def handle_get_all_nodes_with_details(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to get ALL nodes with FULL details (pins, connections, titles)
    in a single call. Avoids N+1 round-trips for describe_blueprint.

    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - function_id: ID of the function/graph

    Returns:
        Response dictionary with success/failure and array of node detail objects.
    """
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")

        if not blueprint_path or not function_id:
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("get_all_nodes_with_details",
                        f"Blueprint: {blueprint_path}, Function ID: {function_id}")

        node_creator = unreal.GenBlueprintNodeCreator
        nodes_json = node_creator.get_all_nodes_with_details(blueprint_path, function_id)

        if nodes_json:
            try:
                nodes = json.loads(nodes_json)
                log.log_result("get_all_nodes_with_details", True,
                               f"Retrieved {len(nodes)} nodes with details from {blueprint_path}")
                return {"success": True, "nodes": nodes}
            except json.JSONDecodeError as e:
                return {"success": False, "error": f"Error parsing nodes JSON: {str(e)}"}
        else:
            return {"success": False, "error": "Failed to get nodes with details"}

    except Exception as e:
        log.log_error(f"Error getting nodes with details: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("get_node_suggestions", category="blueprint")
def handle_get_node_suggestions(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to get suggestions for a node type in Unreal Blueprints

    Args:
        command: The command dictionary containing:
            - node_type: The partial or full node type to get suggestions for (e.g., "Add", "FloatToDouble")

    Returns:
        Response dictionary with success/failure status and a list of suggested node types
    """
    try:
        node_type = command.get("node_type")

        if not node_type:
            log.log_error("Missing required parameter 'node_type' for get_node_suggestions")
            return {"success": False, "error": "Missing required parameter 'node_type'"}

        log.log_command("get_node_suggestions", f"Node Type: {node_type}")

        # Call the C++ implementation from UGenBlueprintNodeCreator
        node_creator = unreal.GenBlueprintNodeCreator
        suggestions_result = node_creator.get_node_suggestions(node_type)

        if suggestions_result:
            if suggestions_result.startswith("SUGGESTIONS:"):
                suggestions = suggestions_result[len("SUGGESTIONS:"):].split(", ")
                log.log_result("get_node_suggestions", True, f"Retrieved {len(suggestions)} suggestions for {node_type}")
                return {"success": True, "suggestions": suggestions}
            else:
                log.log_error(f"Unexpected response format from get_node_suggestions: {suggestions_result}")
                return {"success": False, "error": "Unexpected response format from Unreal"}
        else:
            log.log_result("get_node_suggestions", False, f"No suggestions found for {node_type}")
            return {"success": True, "suggestions": []}  # Empty list for no matches

    except Exception as e:
        log.log_error(f"Error getting node suggestions: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}

@registry.command("get_node_guid", category="blueprint")
def handle_get_node_guid(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle a command to retrieve the GUID of a pre-existing node in a Blueprint graph.
    
    Args:
        command: The command dictionary containing:
            - blueprint_path: Path to the Blueprint asset
            - graph_type: "EventGraph" or "FunctionGraph"
            - node_name: Name of the node (e.g., "BeginPlay") for EventGraph
            - function_id: ID of the function for FunctionGraph to get FunctionEntry
            
    Returns:
        Response dictionary with the node's GUID or an error
    """
    try:
        blueprint_path = command.get("blueprint_path")
        graph_type = command.get("graph_type", "EventGraph")
        node_name = command.get("node_name", "")
        function_id = command.get("function_id", "")

        if not blueprint_path:
            log.log_error("Missing blueprint_path for get_node_guid")
            return {"success": False, "error": "Missing blueprint_path"}

        if graph_type not in ["EventGraph", "FunctionGraph"]:
            log.log_error(f"Invalid graph_type: {graph_type}")
            return {"success": False, "error": f"Invalid graph_type: {graph_type}"}

        log.log_command("get_node_guid", f"Blueprint: {blueprint_path}, Graph: {graph_type}, Node: {node_name or function_id}")

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        node_guid = gen_bp_utils.get_node_guid(blueprint_path, graph_type, node_name, function_id)

        if node_guid:
            log.log_result("get_node_guid", True, f"Found node GUID: {node_guid}")
            return {"success": True, "node_guid": node_guid}
        else:
            log.log_error(f"Failed to find node: {node_name or 'FunctionEntry'}")
            return {"success": False, "error": f"Node not found: {node_name or 'FunctionEntry'}"}

    except Exception as e:
        log.log_error(f"Error getting node GUID: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("get_node_details", category="blueprint")
def handle_get_node_details(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")
        node_guid = command.get("node_guid")

        if not blueprint_path or not function_id or not node_guid:
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("get_node_details", f"Blueprint: {blueprint_path}, Node: {node_guid}")

        schema_mode = command.get("schema_mode", "verbose")
        node_creator = unreal.GenBlueprintNodeCreator
        result_json = node_creator.get_node_details(blueprint_path, function_id, node_guid, schema_mode)

        if result_json:
            result = json.loads(result_json)
            if "error" in result:
                return {"success": False, "error": result["error"]}
            log.log_result("get_node_details", True, f"Got details for node {node_guid}")
            return {"success": True, "details": result}
        else:
            return {"success": False, "error": "Failed to get node details"}

    except Exception as e:
        log.log_error(f"Error getting node details: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("list_graphs", category="blueprint")
def handle_list_graphs(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        blueprint_path = command.get("blueprint_path")
        if not blueprint_path:
            return {"success": False, "error": "Missing blueprint_path"}

        log.log_command("list_graphs", f"Blueprint: {blueprint_path}")

        node_creator = unreal.GenBlueprintNodeCreator
        result_json = node_creator.list_graphs(blueprint_path)

        if result_json:
            graphs = json.loads(result_json)
            log.log_result("list_graphs", True, f"Found {len(graphs)} graphs")
            return {"success": True, "graphs": graphs}
        else:
            return {"success": False, "error": "Failed to list graphs"}

    except Exception as e:
        log.log_error(f"Error listing graphs: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("set_node_pin_value", category="blueprint", mutates_blueprint=True)
def handle_set_node_pin_value(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")
        node_guid = command.get("node_guid")
        pin_name = command.get("pin_name")
        value = command.get("value", "")

        if not all([blueprint_path, function_id, node_guid, pin_name]):
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("set_node_pin_value", f"Node: {node_guid}, Pin: {pin_name}, Value: {value}")

        node_creator = unreal.GenBlueprintNodeCreator
        success = node_creator.set_node_pin_value(blueprint_path, function_id, node_guid, pin_name, str(value))

        if success:
            log.log_result("set_node_pin_value", True, f"Set {pin_name}={value}")
            return {"success": True}
        else:
            return {"success": False, "error": f"Failed to set pin '{pin_name}'. Use get_node_details to see available pins."}

    except Exception as e:
        log.log_error(f"Error setting pin value: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("disconnect_nodes", category="blueprint", mutates_blueprint=True)
def handle_disconnect_nodes(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")
        source_node_id = command.get("source_node_id")
        source_pin = command.get("source_pin")
        target_node_id = command.get("target_node_id")
        target_pin = command.get("target_pin")

        if not all([blueprint_path, function_id, source_node_id, source_pin, target_node_id, target_pin]):
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("disconnect_nodes", f"{source_node_id}.{source_pin} -X- {target_node_id}.{target_pin}")

        node_creator = unreal.GenBlueprintNodeCreator
        success = node_creator.disconnect_nodes(blueprint_path, function_id,
                                                 source_node_id, source_pin,
                                                 target_node_id, target_pin)

        if success:
            log.log_result("disconnect_nodes", True, "Disconnected")
            return {"success": True}
        else:
            return {"success": False, "error": "Failed to disconnect. Pins may not be connected or not found."}

    except Exception as e:
        log.log_error(f"Error disconnecting nodes: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("move_node", category="blueprint", mutates_blueprint=True)
def handle_move_node(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")
        node_guid = command.get("node_guid")
        new_x = command.get("new_x", 0)
        new_y = command.get("new_y", 0)

        if not all([blueprint_path, function_id, node_guid]):
            return {"success": False, "error": "Missing required parameters"}

        log.log_command("move_node", f"Node: {node_guid} -> ({new_x}, {new_y})")

        node_creator = unreal.GenBlueprintNodeCreator
        success = node_creator.move_node(blueprint_path, function_id, node_guid, float(new_x), float(new_y))

        if success:
            log.log_result("move_node", True, f"Moved to ({new_x}, {new_y})")
            return {"success": True}
        else:
            return {"success": False, "error": "Failed to move node"}

    except Exception as e:
        log.log_error(f"Error moving node: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("undo_transaction", category="blueprint")
def handle_undo_transaction(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        log.log_command("undo_transaction", "Undoing last transaction")
        utils = unreal.GenBlueprintUtils
        success = utils.undo_transaction()
        if success:
            log.log_result("undo_transaction", True, "Undo executed")
            return {"success": True}
        else:
            return {"success": False, "error": "No transaction to undo or undo failed"}
    except Exception as e:
        log.log_error(f"Error undoing transaction: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("get_blueprint_variables", category="blueprint")
def handle_get_blueprint_variables(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        blueprint_path = command.get("blueprint_path")
        if not blueprint_path:
            return {"success": False, "error": "Missing blueprint_path"}

        log.log_command("get_blueprint_variables", f"Blueprint: {blueprint_path}")
        utils = unreal.GenBlueprintUtils
        result_json = utils.get_blueprint_variables(blueprint_path)

        if result_json:
            result = json.loads(result_json)
            if isinstance(result, dict):
                return {"success": False, "error": result.get("error", "Unexpected dict response from C++")}
            if not isinstance(result, list):
                return {"success": False, "error": f"Invalid response format: {type(result).__name__}"}
            log.log_result("get_blueprint_variables", True, f"Found {len(result)} variables")
            return {"success": True, "variables": result}
        else:
            return {"success": False, "error": "No result from C++"}

    except Exception as e:
        log.log_error(f"Error getting variables: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("scan_all_blueprints", category="blueprint")
def handle_scan_all_blueprints(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        log.log_command("scan_all_blueprints", "Scanning /Game for all Blueprint assets")
        utils = unreal.GenBlueprintUtils
        result_json = utils.scan_all_blueprints()
        if result_json:
            result = json.loads(result_json)
            log.log_result("scan_all_blueprints", True, f"Found {len(result)} blueprints")
            return {"success": True, "blueprints": result}
        else:
            return {"success": False, "error": "No result from C++"}
    except Exception as e:
        log.log_error(f"Error scanning blueprints: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("save_all_dirty_packages", category="blueprint")
def handle_save_all_dirty_packages(command: Dict[str, Any]) -> Dict[str, Any]:
    """Save all unsaved (dirty) packages under /Game.

    Call this after any MCP operation that modifies assets to prevent
    git pull/merge conflicts on open files.
    """
    try:
        log.log_command("save_all_dirty_packages", "Saving all dirty /Game packages")
        utils = unreal.GenBlueprintUtils
        result_json = utils.save_all_dirty_packages()
        if result_json:
            result = json.loads(result_json)
            count = result.get("count", 0)
            failed = result.get("failed", [])
            log.log_result("save_all_dirty_packages", len(failed) == 0,
                           f"Saved {count}, failed {len(failed)}")
            return {"success": len(failed) == 0, **result}
        return {"success": False, "error": "No result from C++"}
    except Exception as e:
        log.log_error(f"Error saving dirty packages: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("add_switch_case", category="blueprint", mutates_blueprint=True)
def handle_add_switch_case(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add a new named case pin to a K2Node_SwitchString (Switch on String) node.

    STRUCTURAL MUTATION — set_editor_property("PinNames") alone does NOT work because
    ReconstructNode() is never called, leaving the exec pin unmaterialized. This handler
    calls the C++ AddSwitchCase which explicitly calls ReconstructNode() so the new
    output pin is immediately available for connect_nodes / apply_blueprint_patch.

    Args:
        blueprint_path: Asset path (e.g. "/Game/Blueprints/Core/BP_My")
        graph_id:       "EventGraph", other graph name, or graph GUID string
        node_guid:      GUID of the K2Node_SwitchString node
        case_name:      Case string to add (e.g. "StepOn")

    Returns:
        {"success": bool, "case_added": str, "method": str, "pin_count": int}
        method values: "PinNames+ReconstructNode" | "already_exists"
    """
    blueprint_path = command.get("blueprint_path")
    graph_id = command.get("graph_id", "EventGraph")
    node_guid = command.get("node_guid")
    case_name = command.get("case_name")

    if not all([blueprint_path, node_guid, case_name]):
        return {"success": False, "error": "blueprint_path, node_guid, and case_name are required"}

    try:
        log.log_command("add_switch_case",
                        f"BP={blueprint_path} graph={graph_id} guid={node_guid} case={case_name}")
        utils = unreal.GenBlueprintUtils
        result_json = utils.add_switch_case(blueprint_path, graph_id, node_guid, case_name)
        result = json.loads(result_json)
        log.log_result("add_switch_case", result.get("success", False),
                       f"method={result.get('method')} pins={result.get('pin_count')}")
        return result
    except Exception as e:
        log.log_error(f"Error in add_switch_case: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def _call_bool_cpp(cmd_name: str, cpp_method, log_details: str = "", *args) -> Dict[str, Any]:
    """Shared boilerplate for handlers that call a C++ bool-returning method."""
    try:
        log.log_command(cmd_name, log_details)
        result = cpp_method(*args)
        log.log_result(cmd_name, result)
        return {"success": result}
    except Exception as e:
        log.log_error(f"Error in {cmd_name}: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


# Transaction management for atomic blueprint operations

@registry.command("begin_transaction", category="blueprint")
def handle_begin_transaction(command: Dict[str, Any]) -> Dict[str, Any]:
    name = command.get("transaction_name", "BlueprintPatch")
    return _call_bool_cpp("begin_transaction",
                          unreal.GenBlueprintUtils.begin_blueprint_transaction,
                          f"Name: {name}", name)


@registry.command("end_transaction", category="blueprint")
def handle_end_transaction(command: Dict[str, Any]) -> Dict[str, Any]:
    return _call_bool_cpp("end_transaction",
                          unreal.GenBlueprintUtils.end_blueprint_transaction,
                          "Committing transaction")


@registry.command("cancel_transaction", category="blueprint")
def handle_cancel_transaction(command: Dict[str, Any]) -> Dict[str, Any]:
    return _call_bool_cpp("cancel_transaction",
                          unreal.GenBlueprintUtils.cancel_blueprint_transaction,
                          "Rolling back transaction")


# ---------------------------------------------------------------------------
# Preflight: validate patch without mutating
# ---------------------------------------------------------------------------

@registry.command("preflight_blueprint_patch", category="blueprint")
def handle_preflight_blueprint_patch(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id", "EventGraph")
        patch_json = command.get("patch_json", "{}")

        if not blueprint_path:
            return {"success": False, "error": "Missing blueprint_path"}

        log.log_command("preflight_blueprint_patch", f"Blueprint: {blueprint_path}")

        node_creator = unreal.GenBlueprintNodeCreator
        result_json = node_creator.preflight_patch(blueprint_path, function_id, patch_json)

        if result_json:
            result = json.loads(result_json)
            log.log_result("preflight_blueprint_patch", result.get("valid", False),
                           f"Issues: {len(result.get('issues', []))}")
            return {"success": True, **result}
        else:
            return {"success": False, "error": "No result from C++"}

    except Exception as e:
        log.log_error(f"Error in preflight: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


# ---------------------------------------------------------------------------
# Node Discovery: search, inspect, build index
# ---------------------------------------------------------------------------

@registry.command("build_node_index", category="blueprint")
def handle_build_node_index(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        log.log_command("build_node_index", "Building full engine node index")
        node_creator = unreal.GenBlueprintNodeCreator
        count = node_creator.build_node_index()
        log.log_result("build_node_index", True, f"Indexed {count} entries")
        return {"success": True, "indexed_count": count}
    except Exception as e:
        log.log_error(f"Error building node index: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("search_blueprint_nodes", category="blueprint")
def handle_search_blueprint_nodes(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        query = command.get("query", "")
        blueprint_path = command.get("blueprint_path", "")
        category_filter = command.get("category_filter", "")
        max_results = command.get("max_results", 5)

        if not query:
            return {"success": False, "error": "Missing query"}

        log.log_command("search_blueprint_nodes", f"Query: {query}")

        schema_mode = command.get("schema_mode", "verbose")
        node_creator = unreal.GenBlueprintNodeCreator
        result_json = node_creator.search_blueprint_nodes(
            query, blueprint_path, category_filter, max_results, schema_mode)

        if result_json:
            result = json.loads(result_json)
            candidates = result.get("candidates", [])
            log.log_result("search_blueprint_nodes", True,
                           f"Found {len(candidates)} candidates")
            return {"success": True, **result}
        else:
            return {"success": False, "error": "No result from C++"}

    except Exception as e:
        log.log_error(f"Error searching nodes: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


@registry.command("inspect_blueprint_node", category="blueprint")
def handle_inspect_blueprint_node(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        canonical_name = command.get("canonical_name", "")
        blueprint_path = command.get("blueprint_path", "")

        if not canonical_name:
            return {"success": False, "error": "Missing canonical_name"}

        log.log_command("inspect_blueprint_node", f"Node: {canonical_name}")

        node_creator = unreal.GenBlueprintNodeCreator
        schema_mode = command.get("schema_mode", "verbose")
        result_json = node_creator.inspect_blueprint_node(canonical_name, blueprint_path, schema_mode)

        if result_json:
            result = json.loads(result_json)
            if "error" in result:
                log.log_result("inspect_blueprint_node", False, result["error"])
                return {"success": False, "error": result["error"]}
            log.log_result("inspect_blueprint_node", True,
                           f"Pins: {len(result.get('pins', []))}")
            return {"success": True, **{k: v for k, v in result.items() if k != "error"}}
        else:
            return {"success": False, "error": "No result from C++"}

    except Exception as e:
        log.log_error(f"Error inspecting node: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}
