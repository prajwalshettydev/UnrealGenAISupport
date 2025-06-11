import json
from typing import Any, Dict, List, Optional, Tuple, Union

import unreal
from utils import logging as log
from utils import unreal_conversions as uc


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

        log.log_command(
            "create_blueprint", f"Name: {blueprint_name}, Parent: {parent_class}"
        )

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        blueprint = gen_bp_utils.create_blueprint(
            blueprint_name, parent_class, save_path
        )

        if blueprint:
            blueprint_path = f"{save_path}/{blueprint_name}"
            log.log_result("create_blueprint", True, f"Path: {blueprint_path}")
            return {"success": True, "blueprint_path": blueprint_path}
        else:
            log.log_error(f"Failed to create Blueprint {blueprint_name}")
            return {
                "success": False,
                "error": f"Failed to create Blueprint {blueprint_name}",
            }

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

        log.log_command(
            "add_component",
            f"Blueprint: {blueprint_path}, Component: {component_class}",
        )

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        success = gen_bp_utils.add_component(
            blueprint_path, component_class, component_name or ""
        )

        if success:
            log.log_result(
                "add_component", True, f"Added {component_class} to {blueprint_path}"
            )
            return {"success": True}
        else:
            log.log_error(
                f"Failed to add component {component_class} to {blueprint_path}"
            )
            return {
                "success": False,
                "error": f"Failed to add component {component_class} to {blueprint_path}",
            }

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

        log.log_command(
            "add_variable",
            f"Blueprint: {blueprint_path}, Variable: {variable_name}, Type: {variable_type}",
        )

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        success = gen_bp_utils.add_variable(
            blueprint_path, variable_name, variable_type, str(default_value), category
        )

        if success:
            log.log_result(
                "add_variable",
                True,
                f"Added {variable_type} variable {variable_name} to {blueprint_path}",
            )
            return {"success": True}
        else:
            log.log_error(f"Failed to add variable {variable_name} to {blueprint_path}")
            return {
                "success": False,
                "error": f"Failed to add variable {variable_name} to {blueprint_path}",
            }

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

        log.log_command(
            "add_function", f"Blueprint: {blueprint_path}, Function: {function_name}"
        )

        # Convert inputs and outputs to JSON strings for C++ function
        inputs_json = json.dumps(inputs)
        outputs_json = json.dumps(outputs)

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        function_id = gen_bp_utils.add_function(
            blueprint_path, function_name, inputs_json, outputs_json
        )

        if function_id:
            log.log_result(
                "add_function",
                True,
                f"Added function {function_name} to {blueprint_path} with ID: {function_id}",
            )
            return {"success": True, "function_id": function_id}
        else:
            log.log_error(f"Failed to add function {function_name} to {blueprint_path}")
            return {
                "success": False,
                "error": f"Failed to add function {function_name} to {blueprint_path}",
            }

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

        # Call the C++ implementation from UGenBlueprintNodeCreator
        node_creator = unreal.GenBlueprintNodeCreator
        node_id = node_creator.add_node(
            blueprint_path,
            function_id,
            node_type,
            node_position[0],
            node_position[1],
            node_properties_json,
        )

        if node_id:
            log.log_result(
                "add_node",
                True,
                f"Added node {node_type} to {blueprint_path} with ID: {node_id}",
            )
            return {"success": True, "node_id": node_id}
        else:
            log.log_error(f"Failed to add node {node_type} to {blueprint_path}")
            return {
                "success": False,
                "error": f"Failed to add node {node_type} to {blueprint_path}",
            }

    except Exception as e:
        log.log_error(f"Error adding node: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_connect_nodes(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        blueprint_path = command.get("blueprint_path")
        function_id = command.get("function_id")
        source_node_id = command.get("source_node_id")
        source_pin = command.get("source_pin")
        target_node_id = command.get("target_node_id")
        target_pin = command.get("target_pin")

        if not all(
            [
                blueprint_path,
                function_id,
                source_node_id,
                source_pin,
                target_node_id,
                target_pin,
            ]
        ):
            log.log_error("Missing required parameters for connect_nodes")
            return {"success": False, "error": "Missing required parameters"}

        log.log_command(
            "connect_nodes",
            f"Blueprint: {blueprint_path}, {source_node_id}.{source_pin} -> {target_node_id}.{target_pin}",
        )

        gen_bp_utils = unreal.GenBlueprintUtils
        result_json = gen_bp_utils.connect_nodes(
            blueprint_path,
            function_id,
            source_node_id,
            source_pin,
            target_node_id,
            target_pin,
        )
        result = json.loads(result_json)

        if result.get("success"):
            log.log_result(
                "connect_nodes", True, f"Connected nodes in {blueprint_path}"
            )
            return {"success": True}
        else:
            log.log_error(f"Failed to connect nodes: {result.get('error')}")
            return result  # Pass through the detailed response with available pins

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
            log.log_result(
                "compile_blueprint", True, f"Compiled blueprint: {blueprint_path}"
            )
            return {"success": True}
        else:
            log.log_error(f"Failed to compile blueprint: {blueprint_path}")
            return {
                "success": False,
                "error": f"Failed to compile blueprint: {blueprint_path}",
            }

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

        log.log_command(
            "spawn_blueprint", f"Blueprint: {blueprint_path}, Label: {actor_label}"
        )

        # Convert to Unreal types
        loc = uc.to_unreal_vector(location)
        rot = uc.to_unreal_rotator(rotation)
        scale_vec = uc.to_unreal_vector(scale)

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        actor = gen_bp_utils.spawn_blueprint(
            blueprint_path, loc, rot, scale_vec, actor_label
        )

        if actor:
            actor_name = actor.get_actor_label()
            log.log_result(
                "spawn_blueprint",
                True,
                f"Spawned blueprint: {blueprint_path} as {actor_name}",
            )
            return {"success": True, "actor_name": actor_name}
        else:
            log.log_error(f"Failed to spawn blueprint: {blueprint_path}")
            return {
                "success": False,
                "error": f"Failed to spawn blueprint: {blueprint_path}",
            }

    except Exception as e:
        log.log_error(f"Error spawning blueprint: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


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

        log.log_command(
            "add_nodes_bulk", f"Blueprint: {blueprint_path}, Adding {len(nodes)} nodes"
        )

        # Prepare nodes in the format expected by the C++ function
        nodes_json = json.dumps(nodes)

        # Call the C++ implementation from UGenBlueprintNodeCreator
        node_creator = unreal.GenBlueprintNodeCreator
        results_json = node_creator.add_nodes_bulk(
            blueprint_path, function_id, nodes_json
        )

        if results_json:
            results = json.loads(results_json)
            node_mapping = {}

            # Create a mapping from reference IDs to actual node GUIDs
            for node_result in results:
                if "ref_id" in node_result:
                    node_mapping[node_result["ref_id"]] = node_result["node_guid"]
                else:
                    # For nodes without a reference ID, just include the GUID
                    node_mapping[f"node_{len(node_mapping)}"] = node_result["node_guid"]

            log.log_result(
                "add_nodes_bulk",
                True,
                f"Added {len(results)} nodes to {blueprint_path}",
            )
            return {"success": True, "nodes": node_mapping}
        else:
            log.log_error(f"Failed to add nodes to {blueprint_path}")
            return {
                "success": False,
                "error": f"Failed to add nodes to {blueprint_path}",
            }

    except Exception as e:
        log.log_error(f"Error adding nodes: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


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

        log.log_command(
            "connect_nodes_bulk",
            f"Blueprint: {blueprint_path}, Making {len(connections)} connections",
        )

        # Convert connections list to JSON for C++ function
        connections_json = json.dumps(connections)

        # Call the C++ implementation - now returns a JSON string instead of boolean
        gen_bp_utils = unreal.GenBlueprintUtils
        result_json = gen_bp_utils.connect_nodes_bulk(
            blueprint_path, function_id, connections_json
        )

        # Parse the JSON result
        try:
            result_data = json.loads(result_json)
            log.log_result(
                "connect_nodes_bulk",
                result_data.get("success", False),
                f"Connected {result_data.get('successful_connections', 0)}/{result_data.get('total_connections', 0)} node pairs in {blueprint_path}",
            )

            # Return the full result data for detailed error reporting
            return result_data
        except json.JSONDecodeError:
            log.log_error(
                f"Failed to parse JSON result from connect_nodes_bulk: {result_json}"
            )
            return {"success": False, "error": "Failed to parse connection results"}

    except Exception as e:
        log.log_error(f"Error connecting nodes: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


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

        log.log_command(
            "delete_node", f"Blueprint: {blueprint_path}, Node ID: {node_id}"
        )

        # Call the C++ implementation from UGenBlueprintNodeCreator
        node_creator = unreal.GenBlueprintNodeCreator
        success = node_creator.delete_node(blueprint_path, function_id, node_id)

        if success:
            log.log_result(
                "delete_node", True, f"Deleted node {node_id} from {blueprint_path}"
            )
            return {"success": True}
        else:
            log.log_error(f"Failed to delete node {node_id} from {blueprint_path}")
            return {"success": False, "error": f"Failed to delete node {node_id}"}

    except Exception as e:
        log.log_error(f"Error deleting node: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


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

        log.log_command(
            "get_all_nodes", f"Blueprint: {blueprint_path}, Function ID: {function_id}"
        )

        # Call the C++ implementation from UGenBlueprintNodeCreator
        node_creator = unreal.GenBlueprintNodeCreator
        nodes_json = node_creator.get_all_nodes_in_graph(blueprint_path, function_id)

        if nodes_json:
            # Parse the JSON response
            try:
                nodes = json.loads(nodes_json)
                log.log_result(
                    "get_all_nodes",
                    True,
                    f"Retrieved {len(nodes)} nodes from {blueprint_path}",
                )
                return {"success": True, "nodes": nodes}
            except json.JSONDecodeError as e:
                log.log_error(f"Error parsing nodes JSON: {str(e)}")
                return {
                    "success": False,
                    "error": f"Error parsing nodes JSON: {str(e)}",
                }
        else:
            log.log_error(f"Failed to get nodes from {blueprint_path}")
            return {"success": False, "error": "Failed to get nodes"}

    except Exception as e:
        log.log_error(f"Error getting nodes: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


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
            log.log_error(
                "Missing required parameter 'node_type' for get_node_suggestions"
            )
            return {"success": False, "error": "Missing required parameter 'node_type'"}

        log.log_command("get_node_suggestions", f"Node Type: {node_type}")

        # Call the C++ implementation from UGenBlueprintNodeCreator
        node_creator = unreal.GenBlueprintNodeCreator
        suggestions_result = node_creator.get_node_suggestions(node_type)

        if suggestions_result:
            if suggestions_result.startswith("SUGGESTIONS:"):
                suggestions = suggestions_result[len("SUGGESTIONS:") :].split(", ")
                log.log_result(
                    "get_node_suggestions",
                    True,
                    f"Retrieved {len(suggestions)} suggestions for {node_type}",
                )
                return {"success": True, "suggestions": suggestions}
            else:
                log.log_error(
                    f"Unexpected response format from get_node_suggestions: {suggestions_result}"
                )
                return {
                    "success": False,
                    "error": "Unexpected response format from Unreal",
                }
        else:
            log.log_result(
                "get_node_suggestions", False, f"No suggestions found for {node_type}"
            )
            return {"success": True, "suggestions": []}  # Empty list for no matches

    except Exception as e:
        log.log_error(
            f"Error getting node suggestions: {str(e)}", include_traceback=True
        )
        return {"success": False, "error": str(e)}


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

        log.log_command(
            "get_node_guid",
            f"Blueprint: {blueprint_path}, Graph: {graph_type}, Node: {node_name or function_id}",
        )

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        node_guid = gen_bp_utils.get_node_guid(
            blueprint_path, graph_type, node_name, function_id
        )

        if node_guid:
            log.log_result("get_node_guid", True, f"Found node GUID: {node_guid}")
            return {"success": True, "node_guid": node_guid}
        else:
            log.log_error(f"Failed to find node: {node_name or 'FunctionEntry'}")
            return {
                "success": False,
                "error": f"Node not found: {node_name or 'FunctionEntry'}",
            }

    except Exception as e:
        log.log_error(f"Error getting node GUID: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}
