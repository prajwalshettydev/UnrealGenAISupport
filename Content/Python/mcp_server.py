import json
import sys
import os
import time

# Inject Scripts/ dir so lib.blueprint_qa_core is importable.
# __file__ = .../Plugins/GenerativeAISupport/Content/Python/mcp_server.py
# 5 parents up = SmellyCat/
_mcp_scripts_dir = str(
    __import__("pathlib").Path(os.path.abspath(__file__))
    .parent.parent.parent.parent.parent / "Scripts"
)
if _mcp_scripts_dir not in sys.path:
    sys.path.insert(0, _mcp_scripts_dir)
from fastmcp import FastMCP
from fastmcp.utilities.types import Image
import re
import mss
import tempfile # For creating a secure temporary file
from constants import DESTRUCTIVE_SCRIPT_PATTERNS, DESTRUCTIVE_COMMAND_KEYWORDS
from layout_engine import AUTO_PAD_X, AUTO_PAD_Y
from pathlib import Path

# --- Re-exports from tools/ modules (single implementation lives there) ---
from tools.transport import send_to_unreal
from tools.envelope import (
    _strip_empty, _compact_keys, _COMPACT_KEY_MAP,
    _SEMANTIC_KEYS, _DIR_VALUES, _to_semantic, _compact_response,
    _VALID_SCHEMA_MODES, _normalize_schema_mode,
    _make_envelope, _json_out,
    _DescribeCache, _describe_cache, _describe_legend_seen,
    _node_details_cache, _NODE_DETAILS_CACHE_MAX,
    _evict_bp_caches, _node_details_cache_evict,
    _postprocess_describe, _compute_fingerprint,
    _LOCALE_ALIASES,
    _DESCRIBE_CACHE_MAX, _DESCRIBE_CACHE_TTL,
)
from tools.describe import (
    _NODE_TYPE_MAP, _classify_node, _classify_function_call,
    _is_pure_node, _build_data_expression,
    _normalize_graph, _apply_node_filter, _apply_subgraph_filter,
    _graph_fetch_nodes, _graph_build_nodes_and_edges,
    _graph_build_subgraphs, _graph_assemble_result,
    _build_graph_description, _to_pseudocode_only,
    _resolve_neighborhood_node_ref, _bfs_neighborhood,
)
from tools.patch import (
    _patch_looks_like_guid, _patch_classify_error, _patch_extract_ref,
    _patch_validate, _patch_resolve_refs, _patch_mutate, _patch_finalize,
    _update_last_read_ts, _check_rbw_guard, _compute_patch_risk_score,
)
from tools.inspect_tools import (
    get_node_details as _impl_get_node_details,
    search_blueprint_nodes as _impl_search_blueprint_nodes,
    inspect_blueprint_node as _impl_inspect_blueprint_node,
    preflight_blueprint_patch as _impl_preflight_blueprint_patch,
)


# THIS FILE WILL RUN OUTSIDE THE UNREAL ENGINE SCOPE, 
# DO NOT IMPORT UNREAL MODULES HERE OR EXECUTE IT IN THE UNREAL ENGINE PYTHON INTERPRETER

# Create a PID file to let the Unreal plugin know this process is running
def write_pid_file():
    try:
        pid = os.getpid()
        pid_dir = os.path.join(os.path.expanduser("~"), ".unrealgenai")
        os.makedirs(pid_dir, exist_ok=True)
        pid_path = os.path.join(pid_dir, "mcp_server.pid")

        port = int(os.environ.get("UNREAL_PORT", "9877"))
        with open(pid_path, "w") as f:
            f.write(f"{pid}\n{port}")  # Store PID and port

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

# ---------------------------------------------------------------------------
# Tool Exposure Profile System
#
# Controls which tools are registered with MCP based on MCP_TOOL_PROFILE.
# Default: "core"  — daily Blueprint editing tools only (~22 tools, ~5K tokens)
# Supported profiles: core, analysis, scaffold, admin, full
# Supports combinations: MCP_TOOL_PROFILE=core,analysis
#
# In .mcp.json env block:
#   "MCP_TOOL_PROFILE": "core"           # daily /bp work
#   "MCP_TOOL_PROFILE": "core,analysis"  # diagnostics enabled
#   "MCP_TOOL_PROFILE": "full"           # all tools (dev/ops)
# ---------------------------------------------------------------------------

_VALID_GROUPS = {"core", "analysis", "scaffold", "admin", "full"}
_DEFAULT_PROFILE = "core"


def _resolve_active_groups() -> frozenset:
    """Parse MCP_TOOL_PROFILE into a set of active groups."""
    raw = os.getenv("MCP_TOOL_PROFILE", _DEFAULT_PROFILE).strip()
    if raw == "full":
        return frozenset(_VALID_GROUPS)
    groups = {g.strip() for g in raw.split(",") if g.strip()}
    unknown = groups - _VALID_GROUPS
    if unknown:
        print(
            f"[MCP] WARNING: Unknown profile group(s): {unknown}. Valid: {sorted(_VALID_GROUPS)}",
            file=sys.stderr,
        )
        if "MCP_TOOL_PROFILE" in os.environ:
            raise ValueError(f"Unknown MCP_TOOL_PROFILE group(s): {unknown}")
    return frozenset(groups)


_ACTIVE_GROUPS: frozenset = _resolve_active_groups()
_REGISTERED_TOOLS: list = []


def exposed_tool(group: str = "core"):
    """Conditionally register a function as an MCP tool based on active profile.

    Usage:
        @exposed_tool(group="core")
        def my_tool(...): ...
    """
    def decorator(fn):
        if group in _ACTIVE_GROUPS:
            _REGISTERED_TOOLS.append(fn.__name__)
            return mcp.tool()(fn)
        return fn  # function still callable internally, just not exposed via MCP
    return decorator


# send_to_unreal is imported from tools.transport (see top-level imports)


# ---------------------------------------------------------------------------
# Envelope, caching, compact JSON, and fingerprint helpers
# are now in tools/envelope.py — imported at top of this file.
# ---------------------------------------------------------------------------


@exposed_tool(group="core")
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

@exposed_tool(group="core")
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


@exposed_tool(group="admin")
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
            return ("This script appears to involve potentially destructive actions (e.g., deleting or saving files) "
                    "that were not explicitly requested. Please confirm if you want to proceed by saying 'Yes, execute it' "
                    "or modify your request to explicitly allow such actions.")

        command = {
            "type": "execute_python",
            "script": script
        }
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


@exposed_tool(group="admin")
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
                "For example, use `execute_python_script(script='your_code_here')` for Python execution.")

        # Check for potentially destructive commands
        if any(keyword in command.lower() for keyword in DESTRUCTIVE_COMMAND_KEYWORDS):
            return ("This command appears to involve potentially destructive actions (e.g., deleting or saving). "
                    "Please confirm by saying 'Yes, execute it' or explicitly request such actions.")

        command_dict = {
            "type": "execute_unreal_command",
            "command": command
        }
        response = send_to_unreal(command_dict)
        if response.get("success"):
            output = response.get("output", "Command executed with no detailed output returned")
            return f"Command '{command}' executed successfully. Output: {output}"
        else:
            return f"Failed to execute command '{command}': {response.get('error', 'Unknown error')}"
    except Exception as e:
        return f"Error sending command to Unreal: {str(e)}"


#
# Basic Object Commands
#

@exposed_tool(group="scaffold")
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


@exposed_tool(group="scaffold")
def edit_component_property(blueprint_path: str, component_name: str, property_name: str, value: str,
                            is_scene_actor: bool = False, actor_name: str = "") -> str:
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
        "actor_name": actor_name
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
            return result.get("message", f"Set {property_name} of {component_name} to {value}")
        else:
            error = result.get("error", "Unknown error")
            if "suggestions" in result:
                error += f"\nSuggestions: {result['suggestions']}"
            return f"Failed: {error}"
    except Exception as e:
        return f"Error: {str(e)}\nRaw response: {response}"


@exposed_tool(group="scaffold")
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


#
# Blueprint Commands
#

@exposed_tool(group="scaffold")
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

@exposed_tool(group="scaffold")
def take_editor_screenshot() -> Image:
    """
    Takes a screenshot of the primary monitor using a vendored OS-level library.
    This is a robust method that requires no installation and bypasses the Unreal API.
    """
    temp_path = "" # Ensure path is in scope for the finally block
    try:
        # 1. Create a secure, temporary file path with a .png extension.
        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as fp:
            temp_path = fp.name

        # 2. Use the simplest 'shot' method from mss to save the screenshot to the temp file.
        with mss.mss() as sct:
            sct.shot(mon=1, output=temp_path)

        # 3. Read the created temporary file in binary mode to get the raw bytes.
        with open(temp_path, "rb") as image_file:
            image_bytes = image_file.read()

        # 4. Return the Image object directly with the raw bytes.
        #    The FastMCP library handles the encoding internally.
        return Image(
            data=image_bytes,
            format="png"
        )

    except Exception as e:
        error_message = f"OS-level screenshot failed: {str(e)}"
        print(error_message)
        # Return the error as text if something goes wrong.
        return error_message

    finally:
        # 5. Clean up by deleting the temporary file.
        if temp_path and os.path.exists(temp_path):
            os.remove(temp_path)


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
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


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
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
    # Convert default_value to string if it's a number
    if default_value is not None and not isinstance(default_value, str):
        default_value = str(default_value)

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


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
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


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def add_node_to_blueprint(blueprint_path: str, function_id: str, node_type: str,
                          node_position: list = [0, 0], node_properties: dict = None) -> str:
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

        This is a low-level tool that bypasses the preflight validation performed by
        apply_blueprint_patch. For validated node creation with pin connection and compile checks,
        prefer apply_blueprint_patch with a single-node add_nodes entry instead.
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


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def get_node_suggestions(node_type: str) -> str:
    """
    Get suggestions for a node type in Unreal Blueprints
    
    Args:
        node_type: The partial or full node type to get suggestions for (e.g., "Add", "FloatToDouble")
        
    Returns:
        A string indicating success with suggestions or an error message
    """
    command = {
        "type": "get_node_suggestions",
        "node_type": node_type
    }

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


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def delete_node_from_blueprint(blueprint_path: str, function_id: str, node_id: str) -> str:
    """
    Delete a node from a Blueprint graph. All connections to/from this node are
    automatically broken.

    Use get_all_nodes_in_graph first to find the node's GUID.

    Args:
        blueprint_path: Path to the Blueprint asset (e.g., "/Game/Blueprints/BP_MyActor")
        function_id: "EventGraph" or function GUID
        node_id: GUID of the node to delete

    Returns:
        Success or failure message
    """
    command = {
        "type": "delete_node",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "node_id": node_id
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return f"Successfully deleted node {node_id} from function {function_id} in Blueprint at {blueprint_path}"
    else:
        return f"Failed to delete node: {response.get('error', 'Unknown error')}"


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def get_all_nodes_in_graph(blueprint_path: str, function_id: str) -> str:
    """
    Get a summary of all nodes in a Blueprint graph: GUIDs, types, and positions.

    This returns an OVERVIEW — for detailed pin/connection info on a specific node,
    use get_node_details with the node's GUID.

    Args:
        blueprint_path: Path to the Blueprint asset (e.g., "/Game/Blueprints/BP_MyActor")
        function_id: "EventGraph" or function GUID from list_blueprint_graphs

    Returns:
        JSON array of nodes, each with node_guid, node_type, and position [x, y].
    """
    command = {
        "type": "get_all_nodes",
        "blueprint_path": blueprint_path,
        "function_id": function_id
    }

    response = send_to_unreal(command)
    if response.get("success"):
        return response.get("nodes", "[]")
    else:
        return f"Failed to get nodes: {response.get('error', 'Unknown error')}"


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def connect_blueprint_nodes(blueprint_path: str, function_id: str,
                            source_node_id: str, source_pin: str,
                            target_node_id: str, target_pin: str) -> str:
    """
    Connect two nodes in a Blueprint graph by wiring an output pin to an input pin.

    Use get_node_details on both nodes first to discover exact pin names.
    Common pin naming conventions:
    - Execution pins: "execute" (input exec), "then" (output exec)
    - Return value: "ReturnValue"
    - Data pins: named by parameter (e.g., "InString", "Duration", "Target")

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")
        function_id: "EventGraph" or function GUID
        source_node_id: GUID of the node with the OUTPUT pin
        source_pin: Name of the output pin on the source node
        target_node_id: GUID of the node with the INPUT pin
        target_pin: Name of the input pin on the target node

    Returns:
        Success message, or failure with available pins listed for debugging.
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
        error = response.get("error", "Unknown error")
        if "source_available_pins" in response and "target_available_pins" in response:
            error += f"\nAvailable pins on source ({source_node_id}): {json.dumps(response['source_available_pins'], indent=2)}"
            error += f"\nAvailable pins on target ({target_node_id}): {json.dumps(response['target_available_pins'], indent=2)}"
        return f"Failed to connect nodes: {error}"


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
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


@exposed_tool(group="scaffold")
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
        return f"Successfully spawned Blueprint {blueprint_path}" + (
            f" with label '{actor_label}'" if actor_label else "")
    else:
        return f"Failed to spawn Blueprint: {response.get('error', 'Unknown error')}"


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def add_component_with_events(blueprint_path: str, component_name: str, component_class: str) -> str:
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
        "component_class": component_class
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


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
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
        Message indicating success or failure, with details on which connections succeeded or failed
    """
    command = {
        "type": "connect_nodes_bulk",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "connections": connections
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
                    failed_connections.append(f"Connection {idx}: {src} to {tgt} - {err}")

        # Format error message with details
        if failed_connections:
            detailed_errors = "\n- " + "\n- ".join(failed_connections)
            return f"Failed to connect nodes: {error_message}{detailed_errors}"
        else:
            return f"Failed to connect nodes: {error_message}"


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def get_blueprint_node_guid(blueprint_path: str, graph_type: str = "EventGraph", node_name: str = None,
                            function_id: str = None) -> str:
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


# ---------------------------------------------------------------------------
# Node CRUD Tools (Complete set for blueprint node manipulation)
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def get_node_details(blueprint_path: str, function_id: str, node_guid: str,
                     schema_mode: str = "semantic") -> str:
    """Get detailed information about a specific Blueprint node including all its pins,
    connections, default values, and display title."""
    result = _impl_get_node_details(blueprint_path, function_id, node_guid,
                                    schema_mode=schema_mode, send_fn=send_to_unreal)
    # P1-T4: update read timestamp so RBW guard knows context is fresh
    _update_last_read_ts(blueprint_path)
    return result


@exposed_tool(group="core")
def list_blueprint_graphs(blueprint_path: str) -> str:
    """
    List all graphs (EventGraph, Functions, Macros) in a Blueprint with their GUIDs and node counts.

    Use this FIRST when working with a Blueprint you haven't explored yet — it tells you
    what graphs exist and gives you the function_id needed by all other node tools.

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")

    Returns:
        JSON array of graphs, each with graph_guid, graph_name, graph_type, node_count.
    """
    command = {
        "type": "list_graphs",
        "blueprint_path": blueprint_path
    }
    response = send_to_unreal(command)
    if response.get("success"):
        return json.dumps(response.get("graphs", []), indent=2)
    else:
        return f"Failed: {response.get('error', 'Unknown error')}"


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def move_blueprint_node(blueprint_path: str, function_id: str, node_guid: str,
                        new_x: float, new_y: float) -> str:
    """
    Move a node to a new position in the Blueprint graph editor.

    Use this to reposition nodes for better visual layout after adding or rearranging nodes.
    Positions are in graph editor pixel coordinates (integers).

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")
        function_id: "EventGraph" or function GUID
        node_guid: The GUID of the node to move
        new_x: New X position in graph editor coordinates
        new_y: New Y position in graph editor coordinates

    Returns:
        Success or failure message.
    """
    command = {
        "type": "move_node",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "node_guid": node_guid,
        "new_x": new_x,
        "new_y": new_y
    }
    response = send_to_unreal(command)
    if response.get("success"):
        return f"Node {node_guid} moved to ({new_x}, {new_y})"
    else:
        return f"Failed: {response.get('error', 'Unknown error')}"


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def set_node_pin_value(blueprint_path: str, function_id: str, node_guid: str,
                       pin_name: str, value: str) -> str:
    """
    Set the default value of a pin on an existing Blueprint node.

    Use this to change literal values on nodes (e.g., setting a float input to "3.14",
    a string to "Hello", or a boolean to "true").
    Use get_node_details first to discover the exact pin names and their current values.

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")
        function_id: "EventGraph" or function GUID
        node_guid: The GUID of the node
        pin_name: Exact name of the pin to set (case-sensitive, from get_node_details)
        value: New default value as a string

    Returns:
        Success or failure message. On failure, check UE log for available pin names.
    """
    command = {
        "type": "set_node_pin_value",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "node_guid": node_guid,
        "pin_name": pin_name,
        "value": value
    }
    response = send_to_unreal(command)
    if response.get("success"):
        return f"Pin '{pin_name}' set to '{value}'"
    else:
        return f"Failed: {response.get('error', 'Unknown error')}"


# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def disconnect_blueprint_nodes(blueprint_path: str, function_id: str,
                               source_node_id: str, source_pin: str,
                               target_node_id: str, target_pin: str) -> str:
    """
    Break a connection between two pins in a Blueprint graph.

    Use this to remove a specific wire between nodes.
    Use get_node_details to find what's currently connected before calling this.

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")
        function_id: "EventGraph" or function GUID
        source_node_id: GUID of the source node (output side)
        source_pin: Pin name on the source node
        target_node_id: GUID of the target node (input side)
        target_pin: Pin name on the target node

    Returns:
        Success or failure message.
    """
    command = {
        "type": "disconnect_nodes",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "source_node_id": source_node_id,
        "source_pin": source_pin,
        "target_node_id": target_node_id,
        "target_pin": target_pin
    }
    response = send_to_unreal(command)
    if response.get("success"):
        return "Disconnected successfully"
    else:
        return f"Failed: {response.get('error', 'Unknown error')}"


# Safety check for potentially destructive actions
def is_potentially_destructive(script: str) -> bool:
    """
    Check if the script contains potentially destructive actions like deleting or saving files.
    Returns True if such actions are detected and not explicitly requested.
    """
    for keyword in DESTRUCTIVE_SCRIPT_PATTERNS:
        if re.search(keyword, script, re.IGNORECASE):
            return True
    return False


# Scene Control
# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def get_all_scene_objects() -> str:
    """
    Retrieve all actors in the current Unreal Engine level.
    
    Returns:
        JSON string of actors with their names, classes, and locations.
    """
    command = {"type": "get_all_scene_objects"}
    response = send_to_unreal(command)
    return json.dumps(response) if response.get("success") else f"Failed: {response.get('error', 'Unknown error')}"


# Project Control
@exposed_tool(group="scaffold")
def create_project_folder(folder_path: str) -> str:
    """
    Create a new folder in the Unreal project content directory.
    
    Args:
        folder_path: Path relative to /Game (e.g., "FlappyBird/Assets")
    """
    command = {"type": "create_project_folder", "folder_path": folder_path}
    response = send_to_unreal(command)
    return response.get("message", f"Failed: {response.get('error', 'Unknown error')}")


@exposed_tool(group="scaffold")
def get_files_in_folder(folder_path: str) -> str:
    """
    List all files in a specified project folder.
    
    Args:
        folder_path: Path relative to /Game (e.g., "FlappyBird/Assets")
    """
    command = {"type": "get_files_in_folder", "folder_path": folder_path}
    response = send_to_unreal(command)
    return json.dumps(response.get("files", [])) if response.get("success") else f"Failed: {response.get('error')}"


@exposed_tool(group="scaffold")
def create_game_mode(game_mode_path: str, pawn_blueprint_path: str, base_class: str = "GameModeBase") -> str:
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
            "base_class": base_class
        }
        response = send_to_unreal(command)
        parsed = json.loads(response)
        return parsed.get("message", f"Failed: {parsed.get('error')}")
    except Exception as e:
        return f"Error creating game mode: {str(e)}"


@exposed_tool(group="scaffold")
def add_widget_to_user_widget(user_widget_path: str, widget_type: str, widget_name: str,
                              parent_widget_name: str = "") -> str:
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
        "parent_widget_name": parent_widget_name
    }
    # Use json.loads to parse the JSON string returned by send_to_unreal
    response_str = send_to_unreal(command)
    try:
        response_dict = json.loads(response_str)
        # Return a user-friendly string summary
        if response_dict.get("success"):
            actual_name = response_dict.get("widget_name", widget_name)
            return response_dict.get("message",
                                     f"Successfully added widget '{actual_name}' of type '{widget_type}' to '{user_widget_path}'.")
        else:
            return f"Failed to add widget: {response_dict.get('error', 'Unknown C++ error')}"
    except json.JSONDecodeError:
        return f"Failed to parse response from Unreal: {response_str}"
    except Exception as e:
        # Catch potential errors if send_to_unreal itself failed before returning JSON
        if isinstance(response_str, dict) and not response_str.get("success"):
            return f"Failed to send command: {response_str.get('error', 'Unknown MCP error')}"
        return f"An unexpected error occurred: {str(e)} Response: {response_str}"


@exposed_tool(group="scaffold")
def edit_widget_property(user_widget_path: str, widget_name: str, property_name: str, value: str) -> str:
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
        "value": value  # Pass the string value directly
    }
    # Use json.loads to parse the JSON string returned by send_to_unreal
    response_str = send_to_unreal(command)
    try:
        response_dict = json.loads(response_str)
        # Return a user-friendly string summary
        if response_dict.get("success"):
            return response_dict.get("message",
                                     f"Successfully set property '{property_name}' on widget '{widget_name}'.")
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
@exposed_tool(group="scaffold")
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


# ---------------------------------------------------------------------------
# Source-file editing tools
# These run entirely inside this process (outside Unreal) and need no socket.
# ---------------------------------------------------------------------------

_ALLOWED_SOURCE_EXTENSIONS = {
    ".h", ".hpp", ".cpp", ".inl",
    ".cs",
    ".py",
    ".ini", ".cfg",
    ".json",
    ".md", ".txt",
    ".uplugin", ".uproject",
}
_MAX_READ_BYTES = 512 * 1024  # 512 KB
_MAX_WRITE_BYTES = 1024 * 1024  # 1 MB


def _get_project_root() -> Path:
    """Walk up from this file until a .uproject is found."""
    current = Path(__file__).resolve().parent
    for _ in range(10):
        if list(current.glob("*.uproject")):
            return current
        current = current.parent
    raise RuntimeError(
        "Could not locate project root: no .uproject found within 10 parent directories."
    )


def _validate_source_path(relative_path: str) -> Path:
    """
    Resolve relative_path against project root and enforce safety constraints.
    Returns the resolved absolute Path on success, raises ValueError on violation.
    """
    project_root = _get_project_root()
    # Resolve without following symlinks to detect traversal attempts
    resolved = (project_root / relative_path).resolve()

    if not str(resolved).startswith(str(project_root.resolve())):
        raise ValueError(
            f"Path traversal detected: '{relative_path}' resolves outside project root."
        )

    if resolved.suffix.lower() not in _ALLOWED_SOURCE_EXTENSIONS:
        raise ValueError(
            f"Extension '{resolved.suffix}' is not in the allowed list: "
            f"{sorted(_ALLOWED_SOURCE_EXTENSIONS)}"
        )

    return resolved


@exposed_tool(group="admin")
def read_source_file(relative_path: str, start_line: int = 1, end_line: int = 0) -> str:
    """
    Read a source-code or config file inside the Unreal project.

    Args:
        relative_path: Path relative to the project root (e.g. "Source/MyGame/MyActor.h").
        start_line: First line to return, 1-based (default 1 = beginning of file).
        end_line:   Last line to return inclusive, 1-based (default 0 = read to EOF).

    Returns:
        File content as a string, prefixed with the line range actually returned.
        Allowed extensions: .h .hpp .cpp .inl .cs .py .ini .cfg .json .md .txt .uplugin .uproject
        Maximum returned size: 512 KB.
    """
    try:
        path = _validate_source_path(relative_path)
    except ValueError as exc:
        return f"Error: {exc}"

    if not path.exists():
        return f"Error: File not found: '{relative_path}'"

    if path.stat().st_size > _MAX_READ_BYTES:
        return (
            f"Error: File exceeds the 512 KB read limit "
            f"({path.stat().st_size} bytes). Use start_line/end_line to read a slice."
        )

    try:
        content = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return f"Error: File is not valid UTF-8: '{relative_path}'"

    lines = content.splitlines(keepends=True)
    total = len(lines)

    lo = max(1, start_line) - 1          # convert to 0-based index
    hi = total if end_line <= 0 else min(end_line, total)

    if lo >= total:
        return f"Error: start_line {start_line} is beyond end of file ({total} lines)."

    selected = lines[lo:hi]
    header = f"// [{relative_path}] lines {lo + 1}-{lo + len(selected)} of {total}\n"
    return header + "".join(selected)


@exposed_tool(group="admin")
def write_source_file(relative_path: str, content: str) -> str:
    """
    Write (overwrite or create) a source-code or config file inside the Unreal project.

    The file is written atomically: content is first written to a temp file in the
    same directory, then renamed, so a crash mid-write cannot corrupt an existing file.

    Args:
        relative_path: Path relative to the project root (e.g. "Source/MyGame/NewActor.h").
        content:       Full UTF-8 text to write.

    Returns:
        A summary string on success, or an error message.
    """
    try:
        path = _validate_source_path(relative_path)
    except ValueError as exc:
        return f"Error: {exc}"

    encoded = content.encode("utf-8")
    if len(encoded) > _MAX_WRITE_BYTES:
        return (
            f"Error: Content exceeds the 1 MB write limit ({len(encoded)} bytes)."
        )

    existed = path.exists()
    path.parent.mkdir(parents=True, exist_ok=True)

    # Atomic write via temp file
    tmp = path.with_suffix(path.suffix + ".mcp_tmp")
    try:
        tmp.write_text(content, encoding="utf-8")
        tmp.replace(path)
    except Exception as exc:
        try:
            tmp.unlink(missing_ok=True)
        except Exception:
            pass
        return f"Error writing file: {exc}"

    action = "updated" if existed else "created"
    return (
        f"OK: '{relative_path}' {action} "
        f"({len(encoded)} bytes, {len(content.splitlines())} lines)."
    )


@exposed_tool(group="admin")
def patch_source_file(relative_path: str, old_text: str, new_text: str) -> str:
    """
    Replace an exact substring in a source file (surgical edit, no full overwrite).

    Finds the first occurrence of old_text in the file and replaces it with new_text.
    If old_text occurs zero or more than once the operation is refused with an explanation,
    so you can safely narrow your search string and retry.

    Args:
        relative_path: Path relative to the project root (e.g. "Source/MyGame/MyActor.cpp").
        old_text:      The exact text to find (use several surrounding lines for uniqueness).
        new_text:      The replacement text.

    Returns:
        A summary string on success, or an error message with a hint.
    """
    try:
        path = _validate_source_path(relative_path)
    except ValueError as exc:
        return f"Error: {exc}"

    if not path.exists():
        return f"Error: File not found: '{relative_path}'"

    if path.stat().st_size > _MAX_READ_BYTES:
        return f"Error: File exceeds the 512 KB read limit; use write_source_file instead."

    try:
        original = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return f"Error: File is not valid UTF-8: '{relative_path}'"

    count = original.count(old_text)
    if count == 0:
        return (
            "Error: old_text not found in file. "
            "Check whitespace, line endings, or indentation. "
            "Tip: use read_source_file to inspect the exact content first."
        )
    if count > 1:
        return (
            f"Error: old_text occurs {count} times in the file; "
            "provide more surrounding context so it is unique."
        )

    patched = original.replace(old_text, new_text, 1)
    encoded = patched.encode("utf-8")
    if len(encoded) > _MAX_WRITE_BYTES:
        return f"Error: Patched content exceeds the 1 MB write limit."

    tmp = path.with_suffix(path.suffix + ".mcp_tmp")
    try:
        tmp.write_text(patched, encoding="utf-8")
        tmp.replace(path)
    except Exception as exc:
        try:
            tmp.unlink(missing_ok=True)
        except Exception:
            pass
        return f"Error writing patched file: {exc}"

    delta = len(patched.splitlines()) - len(original.splitlines())
    sign = f"+{delta}" if delta >= 0 else str(delta)
    return (
        f"OK: patch applied to '{relative_path}' "
        f"({sign} lines, {len(encoded)} bytes total)."
    )



# ---------------------------------------------------------------------------
# describe_blueprint v2 helpers (classify, build graph, pseudocode, etc.)
# are now in tools/describe.py — imported at top of this file.
# ---------------------------------------------------------------------------


@exposed_tool(group="core")
def describe_blueprint(
    blueprint_path: str,
    graph_name: str = "",
    max_depth: str = "standard",
    include_pseudocode: bool = True,
    compact: bool = False,
    node_filter: str = "",
    node_ids: str = "",
    subgraph_filter: str = "",
    force_refresh: bool = False,
) -> str:
    """
    Get a structured, LLM-friendly description of an entire Blueprint graph (v2).

    Returns JSON following ue_blueprint_llm_graph_v0.2 schema with:
    - Exec flow and data flow edges SEPARATED
    - Linearized execution trace (handles loops/branches/latent, NOT a DAG assumption)
    - Two-layer pseudocode: high-level summary + low-level trace
    - Inline data expressions for pure nodes (math, getters, struct builders)
    - Three-layer subgraphs: event_flows, shared_functions, macros
    - Two-level semantic classification (intent + side_effect) per function call
    - Latent/async markers on Delay, Timeline, etc.
    - Graph normalization (reroute removal, chain merging)
    - Single batch socket call for all node details (fast)
    - Symbol table at standard/full depth
    - Response caching: repeated calls with same blueprint return cache hit (~100 tokens)

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")
        graph_name: Specific graph name. Empty = all graphs.
        max_depth: "minimal" | "standard" | "full" | "pseudocode"
            pseudocode: Returns only flow pseudocode + symbol table. Saves 60-70% tokens vs standard.
        include_pseudocode: Generate execution traces and pseudocode (default True)
        compact: Use compact output (short keys, no indent, strip empties). Saves 25-40% tokens.
            Key legend: t=title, nt=node_type, r=role, tg=tags, ex=exec_edges, da=data_edges,
            fn=from_node, tn=to_node, fp=from_pin, tp=to_pin, pc=pseudocode, tr=execution_trace,
            sum=summary, sym=symbol_table, dex=data_expressions, nc=node_count
        node_filter: Comma-separated node title patterns (regex). Only matching nodes are returned.
        node_ids: Comma-separated GUIDs. Only these specific nodes are returned.
        subgraph_filter: Comma-separated subgraph names. Only matching event flows are returned.
        force_refresh: Bypass cache and fetch fresh data.

    Returns:
        JSON with schema_version, execution_model, subgraphs, data_expressions, symbol_table.
    """
    if max_depth not in ("minimal", "standard", "full", "pseudocode"):
        return f"Invalid max_depth '{max_depth}'. Must be 'minimal', 'standard', 'full', or 'pseudocode'."

    # --- Cache check (skip for filtered queries — they're already cheap) ---
    has_filters = bool(node_filter or node_ids or subgraph_filter)
    cache_key = (blueprint_path, graph_name, max_depth)

    # Fetch graph list once — reused for both fingerprint and describe logic.
    list_resp = send_to_unreal({"type": "list_graphs", "blueprint_path": blueprint_path})
    if not list_resp.get("success"):
        return f"Failed: {list_resp.get('error', 'Unknown error')}"

    all_graphs = list_resp.get("graphs", [])
    if isinstance(all_graphs, str):
        all_graphs = json.loads(all_graphs)

    if not force_refresh and not has_filters:
        # Compute fingerprint from already-fetched graphs — no extra socket call.
        fingerprint = _compute_fingerprint(blueprint_path, graph_name, graphs=all_graphs)
        if fingerprint:
            cached = _describe_cache.get(cache_key, fingerprint)
            if cached is not None:
                cache_msg = {
                    "cache_hit": True,
                    "fingerprint": fingerprint,
                    "message": "No changes since last describe. Use force_refresh=True to bypass cache.",
                }
                return json.dumps(cache_msg, separators=(",", ":"), ensure_ascii=False)
    else:
        fingerprint = None

    if graph_name:
        all_graphs = [g for g in all_graphs if g["graph_name"] == graph_name]
        if not all_graphs:
            available = [g["graph_name"] for g in list_resp.get("graphs", [])]
            return f"Graph '{graph_name}' not found. Available: {available}"

    graph_descriptions = []
    for g in all_graphs:
        # pseudocode depth: build with standard to get traces, then strip to pseudocode only
        build_depth = "standard" if max_depth == "pseudocode" else max_depth
        build_pseudo = True if max_depth == "pseudocode" else include_pseudocode
        desc = _build_graph_description(
            blueprint_path, g, build_depth, build_pseudo, send_to_unreal,
            node_filter=node_filter, node_ids=node_ids, subgraph_filter=subgraph_filter)
        if max_depth == "pseudocode":
            desc = _to_pseudocode_only(desc)
        graph_descriptions.append(desc)

    bp_name = blueprint_path.rsplit("/", 1)[-1] if "/" in blueprint_path else blueprint_path
    if len(graph_descriptions) == 1:
        result = {"schema_version": "ue_blueprint_llm_graph_v0.2", "blueprint": graph_descriptions[0]}
    else:
        result = {
            "schema_version": "ue_blueprint_llm_graph_v0.2",
            "blueprint": {
                "blueprint_id": blueprint_path,
                "blueprint_name": bp_name,
                "summary": f"{bp_name} — {len(all_graphs)} graphs",
                "graphs": graph_descriptions,
            },
        }

    if compact:
        result["_legend"] = "t=title|nt=node_type|r=role|tg=tags|ex=exec_edges|da=data_edges|fn=from_node|tn=to_node|fp=from_pin|tp=to_pin|pc=pseudocode|tr=trace|sum=summary|sym=symbol_table|dex=data_expressions|nc=node_count"
    result_json = _json_out(result, compact)

    # --- Cache store (only for unfiltered full queries) ---
    if not has_filters and fingerprint:
        _describe_cache.put(cache_key, fingerprint, result_json)

    # P1-T4: update read timestamp so RBW guard knows context is fresh
    _update_last_read_ts(blueprint_path)

    # Post-process: strip legend on repeat, strip redundant graph fields
    return _postprocess_describe(result_json, compact, max_depth, blueprint_path)


# ---------------------------------------------------------------------------
# Layout helpers (P1) — anchor extraction and node role classification
# ---------------------------------------------------------------------------

# All layout helpers live in layout_engine.py — imported as _build_occupancy, _find_free_slot,
# _classify_node_role, _extract_layout_intent at the top of this file.


# ---------------------------------------------------------------------------
# Patch helpers (_patch_validate, _patch_resolve_refs, _patch_mutate, _patch_finalize)
# are now in tools/patch.py — imported at top of this file.
# ---------------------------------------------------------------------------


# ---------------------------------------------------------------------------
# apply_blueprint_patch — orchestrator (delegates to phase helpers above)
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def apply_blueprint_patch(
    blueprint_path: str,
    function_id: str,
    patch_json: str,
    verify: bool = True,
    dry_run: bool = False,
    strict_mode: bool = False,
) -> str:
    """
    Apply a batch of blueprint modifications in one call.

    This is the PRIMARY tool for modifying blueprints. Instead of calling
    add_node + connect_nodes + set_pin_value individually (many round-trips),
    describe all changes in a single patch JSON.

    PREFERRED ALTERNATIVES for common structural edits (use these instead of patch):
      - insert_exec_node_between: insert a node between two connected exec pins
      - append_node_after_exec: append a node at the end (or middle) of an exec chain
      - wrap_exec_chain_with_branch: wrap an exec chain with a Branch/condition node

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")
        function_id: "EventGraph" or function GUID
        patch_json: JSON string with the patch operations (see format below)
        verify: If True, return a compact verification of affected nodes (saves a separate describe call)
        dry_run: If True, validate the patch via preflight WITHOUT applying. Returns
                 predicted changes and issues. Safe to call before live apply.
        strict_mode: If True, apply extra guards:
                     - Restricted node types (MacroInstance/ComponentBoundEvent/Timeline) are BLOCKED
                     - Display-name pin refs trigger warnings
                     - remove+add_connections patterns suggest high-layer tools
                     Returns guard_decisions list explaining what was checked.

    PREFERRED ALTERNATIVES for structural exec-chain edits (use these INSTEAD of patch):
      - insert_exec_node_between: insert a node into an existing exec connection
      - append_node_after_exec: append a node at the end or middle of an exec chain
      - wrap_exec_chain_with_branch: wrap an exec chain with a Branch/condition node
      Call suggest_best_edit_tool(intent) to get a recommendation based on your goal.

    Patch format:
    {
      "add_nodes": [
        {
          "ref_id": "MyNode",           // local reference ID for connections
          "node_type": "K2Node_CallFunction",
          "function_name": "PrintString",  // optional, for CallFunction nodes
          "position": [400, 0],         // optional, defaults to auto
          "pin_values": {"InString": "Hello"}  // optional default pin values
        }
      ],
      "remove_nodes": ["<GUID>"],       // GUIDs of nodes to delete
      "add_connections": [
        {"from": "MyNode.then", "to": "<GUID>.execute"}
        // Can use ref_id.pin or GUID.pin or Title.pin
      ],
      "remove_connections": [
        {"from": "<GUID>.pin", "to": "<GUID>.pin"}
      ],
      "set_pin_values": [
        {"node": "<GUID or ref_id>", "pin": "PinName", "value": "new value"}
      ],
      "auto_compile": true              // optional, default true
    }

    Returns:
        JSON with results: created node GUIDs, connection results, errors.
    """
    # --- dry_run: validate via preflight without applying ---
    if dry_run:
        pf_resp = send_to_unreal({
            "type": "preflight_blueprint_patch",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
            "patch_json": patch_json,
        })
        valid = pf_resp.get("valid", pf_resp.get("success", False))
        issues = pf_resp.get("issues", [])
        predicted = pf_resp.get("predicted_nodes", [])
        envelope = _make_envelope(
            "apply_blueprint_patch", True,
            {"dry_run": True, "valid": valid, "issues": issues,
             "predicted_nodes": predicted},
            summary=f"dry_run: {'valid' if valid else 'invalid'}, {len(issues)} issue(s)",
            diagnostics=[{"message": i.get("message", str(i))} for i in issues],
        )
        return json.dumps(envelope, separators=(",", ":"), ensure_ascii=False)

    # --- Phase 1: Validate ---
    patch, err = _patch_validate(patch_json, blueprint_path, function_id, send_to_unreal)
    if err:
        return err

    # --- P1-T2: Generate trace_id for full chain observability ---
    import uuid as _uuid
    _trace_id = str(_uuid.uuid4())[:8]

    # --- Structured patch log for observability ---
    patch_log = {
        "blueprint": blueprint_path,
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "trace_id": _trace_id,
        "phases": {},
        "rollback": False,
        "total_created": 0,
        "total_connected": 0,
        "total_errors": 0,
        "error_categories": [],
    }

    # --- P1-T4: Read-Before-Write Guard ---
    _rbw_actions = _check_rbw_guard(blueprint_path, send_to_unreal)

    # --- P1-T5: Dry-Run Auto Guard ---
    try:
        _parsed_patch = json.loads(patch_json)
        _risk_score = _compute_patch_risk_score(_parsed_patch)
    except Exception:
        _parsed_patch = {}
        _risk_score = 0

    if _risk_score >= 6 and not dry_run:
        # Auto-preflight before high-risk mutate
        pf_resp = send_to_unreal({
            "type": "preflight_blueprint_patch",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
            "patch_json": patch_json,
        })
        pf_valid = pf_resp.get("valid", pf_resp.get("success", True))
        pf_issues = pf_resp.get("issues", [])
        if not pf_valid or pf_issues:
            guard_envelope = _make_envelope(
                "apply_blueprint_patch", False,
                {
                    "error_code": "HIGH_RISK_PATCH_BLOCKED",
                    "patch_risk_score": _risk_score,
                    "preflight_issues": pf_issues,
                    "hint": "Set dry_run=true first to inspect issues, then fix and re-apply.",
                    "guard_actions": _rbw_actions,
                },
                summary=f"Blocked: high-risk patch (score={_risk_score}), {len(pf_issues)} preflight issue(s)",
                diagnostics=[{"message": i.get("message", str(i))} for i in pf_issues],
            )
            return json.dumps(guard_envelope, separators=(",", ":"), ensure_ascii=False)

    # --- US-005: Strict Mode Guards ---
    # --- US-005: Strict Mode Guards ---
    # Guard evaluation always runs when strict_mode=True (including dry_run).
    # Blocking behavior only applies when NOT dry_run.
    # This ensures dry_run=True + strict_mode=True returns guard_decisions for pre-flight inspection.
    if strict_mode:
        guard_decisions = []
        strict_blocks = []

        try:
            _sm_patch = json.loads(patch_json)
        except Exception:
            _sm_patch = {}

        from node_lifecycle_registry import get_node_lifecycle, is_restricted

        # Rule 1: Restricted node types
        for node in _sm_patch.get("add_nodes", []):
            nt = node.get("node_type", "")
            if is_restricted(nt):
                info = get_node_lifecycle(nt)
                strict_blocks.append({
                    "rule": "RESTRICTED_NODE_TYPE",
                    "node_type": nt,
                    "status": info.get("status"),
                    "note": info.get("note", ""),
                })
                guard_decisions.append(f"BLOCKED:{nt}:{info.get('status')}")

        # Rule 2: display-name pin refs (not GUID, not fname:) trigger warning
        pin_display_name_refs = []
        for conn in _sm_patch.get("add_connections", []):
            for ep in (conn.get("from", ""), conn.get("to", "")):
                parts = ep.split("::", 1) if "::" in ep else ep.rsplit(".", 1)
                if len(parts) == 2:
                    ref, _pin = parts
                    if not _patch_looks_like_guid(ref) and not ref.startswith("fname:"):
                        pin_display_name_refs.append(ep)
        if pin_display_name_refs:
            guard_decisions.append(f"DISPLAY_PIN_REF:{len(pin_display_name_refs)}")

        # Rule 3: remove+add_connections pattern → suggest high-layer tools
        has_remove_conn = bool(_sm_patch.get("remove_connections"))
        has_add_conn = bool(_sm_patch.get("add_connections"))
        if has_remove_conn and has_add_conn:
            guard_decisions.append("STRUCTURAL_EDIT:prefer_high_layer_tools")

        # Always attach guard_decisions to patch_log (even in dry_run)
        if guard_decisions:
            patch_log["strict_mode_guard_decisions"] = guard_decisions

        # Block execution only when NOT dry_run
        if strict_blocks and not dry_run:
            strict_envelope = _make_envelope(
                "apply_blueprint_patch", False,
                {
                    "error_code": "STRICT_MODE_BLOCKED",
                    "strict_blocks": strict_blocks,
                    "guard_decisions": guard_decisions,
                    "recommended_next_actions": [a for a in [
                        "use_insert_exec_node_between" if (has_remove_conn and has_add_conn) else None,
                        "call_inspect_blueprint_node_first",
                        "rerun_with_dry_run",
                        "call_get_node_details_first",
                    ] if a],
                    "hint": "Restricted node types must be handled with specialist tools. See node_lifecycle_registry.",
                    "guard_actions": _rbw_actions,
                },
                summary=f"Strict mode blocked: {len(strict_blocks)} restricted node type(s)",
            )
            return json.dumps(strict_envelope, separators=(",", ":"), ensure_ascii=False)

        # In dry_run with strict_blocks: return guard info without blocking
        if strict_blocks and dry_run:
            dry_guard = _make_envelope(
                "apply_blueprint_patch", False,
                {
                    "dry_run": True,
                    "error_code": "STRICT_MODE_WOULD_BLOCK",
                    "strict_blocks": strict_blocks,
                    "guard_decisions": guard_decisions,
                    "hint": "dry_run=True: these issues would block execution in strict_mode. Fix before applying.",
                },
                summary=f"dry_run strict check: {len(strict_blocks)} issue(s) would be blocked",
            )
            return json.dumps(dry_guard, separators=(",", ":"), ensure_ascii=False)

    # --- Phase 2: Resolve refs ---
    ctx = _patch_resolve_refs(patch, blueprint_path, function_id, send_to_unreal)

    # Handle preflight rejection
    if ctx["preflight_reject"]:
        for k, v in ctx["patch_log_updates"].items():
            patch_log["phases"][k] = v
        patch_log["total_errors"] = ctx["patch_log_updates"].get("auto_preflight", {}).get("issues", 0)
        patch_log["error_categories"].append("PREFLIGHT_REJECTED")
        print(f"[PATCH_LOG] {json.dumps(patch_log, separators=(',', ':'), ensure_ascii=False)}",
              file=sys.stderr)
        return ctx["preflight_reject"]

    # --- Phase 3: Mutate ---
    results = _patch_mutate(patch, blueprint_path, function_id, ctx, send_to_unreal, patch_log)

    # --- Phase 4: Finalize ---
    results = _patch_finalize(blueprint_path, function_id, results, patch_log, send_to_unreal, verify)

    # --- Attach GUID drift log ---
    if ctx["guid_drift_log"]:
        results["guid_drift"] = ctx["guid_drift_log"]

    # --- Wrap in unified envelope ---
    ok = results.get("success", False)
    errs = results.get("errors", [])
    created = results.get("created_nodes", {})
    summary = (
        f"Patch applied: {len(created)} node(s) created"
        if ok and created else
        ("Patch applied" if ok else f"Patch failed: {errs[:1]}")
    )
    envelope = _make_envelope(
        "apply_blueprint_patch", ok, results,
        summary=summary,
        warnings=[e for e in errs if "warning" in e.lower()],
        diagnostics=[{"message": e} for e in errs if "warning" not in e.lower()],
    )
    # P1-T2: attach trace_id and guard_actions to response
    envelope["trace_id"] = _trace_id
    if _rbw_actions:
        envelope["guard_actions"] = _rbw_actions
    # US-005: attach strict_mode guard_decisions if present
    _sm_decisions = patch_log.get("strict_mode_guard_decisions", [])
    if _sm_decisions:
        envelope["guard_decisions"] = _sm_decisions
    # Always attach recommended_next_actions from post_failure_hint if present
    results_data = envelope.get("data", {})
    pf_hint = results_data.get("post_failure_hint", {})
    if pf_hint.get("recommended_next_actions"):
        envelope["recommended_next_actions"] = pf_hint["recommended_next_actions"]
    return json.dumps(envelope, separators=(",", ":"), ensure_ascii=False)


# ---------------------------------------------------------------------------
# preflight_blueprint_patch — validate patch without mutating
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def preflight_blueprint_patch(
    blueprint_path: str,
    patch_json: str,
    function_id: str = "EventGraph",
) -> str:
    """Validate a blueprint patch WITHOUT applying it."""
    return _impl_preflight_blueprint_patch(blueprint_path, patch_json,
                                           function_id=function_id, send_fn=send_to_unreal)


# ---------------------------------------------------------------------------
# debug_blueprint — inspect, simulate, and trace blueprint execution
# ---------------------------------------------------------------------------

@exposed_tool(group="core")  # was analysis; used in blueprint-qa normal workflow
def debug_blueprint(
    blueprint_path: str,
    mode: str = "compile_check",
    actor_label: str = "",
    event_name: str = "",
    variable_names: str = "",
) -> str:
    """
    Debug a Blueprint with multiple modes: compile check, variable inspection,
    event simulation, and execution tracing.

    Modes:
      "compile_check" — Compile and report detailed errors (no PIE needed)
      "inspect"       — Read current variable values from a live actor in PIE
      "simulate"      — Trigger an event/function on a live actor in PIE
      "trace"         — Get the most recent execution path from blueprint debug log

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")
        mode: One of "compile_check", "inspect", "simulate", "trace"
        actor_label: (inspect/simulate/trace) Label of the actor in the level
        event_name: (simulate) Name of the event or function to trigger
        variable_names: (inspect) Comma-separated variable names to read. Empty = all.

    Returns:
        JSON with debug results depending on mode.
    """
    if mode == "compile_check":
        # Delegate to compile_blueprint_with_errors which uses working APIs
        return compile_blueprint_with_errors(blueprint_path)

    elif mode == "inspect":
        # Read variable values from a live actor
        var_filter = ""
        if variable_names:
            names = [v.strip() for v in variable_names.split(",")]
            var_filter = f"filter_names = {names}"
        else:
            var_filter = "filter_names = None"

        script = f"""
import unreal
import json

actor_label = '{actor_label}'
bp_path = '{blueprint_path}'
{var_filter}

# Find actor in level
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
target = None
for a in actors:
    if a.get_actor_label() == actor_label:
        target = a
        break

if target is None:
    print(json.dumps({{"success": False, "error": f"Actor '{{actor_label}}' not found in level"}}))
else:
    result = {{"success": True, "actor": actor_label, "class": target.get_class().get_name(), "variables": {{}}}}

    # Get all properties
    bp_class = target.get_class()
    for prop in dir(target):
        if prop.startswith('_'):
            continue
        if filter_names and prop not in filter_names:
            continue
        try:
            val = target.get_editor_property(prop)
            # Convert to serializable
            if isinstance(val, (bool, int, float, str)):
                result["variables"][prop] = val
            elif isinstance(val, unreal.Vector):
                result["variables"][prop] = {{"x": val.x, "y": val.y, "z": val.z}}
            elif isinstance(val, unreal.Rotator):
                result["variables"][prop] = {{"pitch": val.pitch, "yaw": val.yaw, "roll": val.roll}}
            elif val is None:
                result["variables"][prop] = None
            else:
                result["variables"][prop] = str(val)
        except:
            pass

    print(json.dumps(result, default=str))
"""
        resp = send_to_unreal({"type": "execute_python", "script": script})
        if resp.get("success"):
            output = resp.get("output", "")
            for line in output.strip().split("\n"):
                line = line.strip()
                if line.startswith("{"):
                    try:
                        parsed = json.loads(line)
                        return json.dumps(parsed, indent=2, ensure_ascii=False)
                    except:
                        pass
            return json.dumps({"success": True, "raw_output": output})
        return json.dumps({"success": False, "error": resp.get("error", "Unknown")})

    elif mode == "simulate":
        # Trigger an event or function on a live actor
        if not event_name:
            return json.dumps({"success": False, "error": "event_name is required for simulate mode"})

        script = f"""
import unreal
import json

actor_label = '{actor_label}'
event_name = '{event_name}'

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
target = None
for a in actors:
    if a.get_actor_label() == actor_label:
        target = a
        break

if target is None:
    print(json.dumps({{"success": False, "error": f"Actor '{{actor_label}}' not found"}}))
else:
    result = {{"success": True, "actor": actor_label, "event": event_name}}

    # Try to call the function/event
    try:
        if hasattr(target, event_name):
            fn = getattr(target, event_name)
            if callable(fn):
                ret = fn()
                result["return_value"] = str(ret) if ret is not None else None
                result["executed"] = True
            else:
                result["executed"] = False
                result["error"] = f"{{event_name}} is not callable"
        else:
            # Try calling via editor utility
            try:
                unreal.SystemLibrary.execute_console_command(target, f"ce {{event_name}}")
                result["executed"] = True
                result["method"] = "console_event"
            except:
                result["executed"] = False
                result["error"] = f"Function/event '{{event_name}}' not found on actor"
    except Exception as e:
        result["executed"] = False
        result["error"] = str(e)

    print(json.dumps(result, default=str))
"""
        resp = send_to_unreal({"type": "execute_python", "script": script})
        if resp.get("success"):
            output = resp.get("output", "")
            for line in output.strip().split("\n"):
                line = line.strip()
                if line.startswith("{"):
                    try:
                        parsed = json.loads(line)
                        return json.dumps(parsed, indent=2, ensure_ascii=False)
                    except:
                        pass
            return json.dumps({"success": True, "raw_output": output})
        return json.dumps({"success": False, "error": resp.get("error", "Unknown")})

    elif mode == "trace":
        # Get recent blueprint execution log
        script = f"""
import unreal
import json

actor_label = '{actor_label}'

# Collect recent log lines related to blueprint execution
log_lines = []
try:
    # Get the most recent log entries
    all_logs = unreal.PythonBPLib.get_unreal_log_lines() if hasattr(unreal, 'PythonBPLib') else []
    for line in all_logs[-50:]:
        s = str(line)
        if 'Blueprint' in s or 'LogScript' in s or actor_label in s:
            log_lines.append(s)
except:
    pass

# Also check OutputLog
try:
    import subprocess
    # Read the most recent log file
    import glob
    log_dir = unreal.Paths.project_log_dir()
    log_files = sorted(glob.glob(str(log_dir) + '/*.log'), key=lambda f: -__import__('os').path.getmtime(f))
    if log_files:
        with open(log_files[0], 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()[-100:]
            for line in lines:
                if 'LogBlueprintUserMessages' in line or 'LogScript' in line:
                    log_lines.append(line.strip())
                if actor_label and actor_label in line:
                    log_lines.append(line.strip())
except:
    pass

result = {{
    "success": True,
    "actor": actor_label,
    "trace_lines": log_lines[-30:],
    "note": "Shows recent blueprint log output. For full execution tracing, enable Blueprint Debug in the editor."
}}
print(json.dumps(result, default=str))
"""
        resp = send_to_unreal({"type": "execute_python", "script": script})
        if resp.get("success"):
            output = resp.get("output", "")
            for line in output.strip().split("\n"):
                line = line.strip()
                if line.startswith("{"):
                    try:
                        parsed = json.loads(line)
                        return json.dumps(parsed, indent=2, ensure_ascii=False)
                    except:
                        pass
            return json.dumps({"success": True, "raw_output": output})
        return json.dumps({"success": False, "error": resp.get("error", "Unknown")})

    else:
        return json.dumps({
            "success": False,
            "error": f"Unknown mode '{mode}'. Use: compile_check, inspect, simulate, trace"
        })


# ---------------------------------------------------------------------------
# P0: add_nodes_bulk — expose existing handler as MCP tool
# ---------------------------------------------------------------------------

# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def add_nodes_bulk(blueprint_path: str, function_id: str, nodes_json: str) -> str:
    """
    Add multiple nodes to a Blueprint graph in a single call.

    Each node in the array has a ref_id (your local reference) that maps to the
    actual GUID assigned by Unreal. Use the returned mapping to wire connections.

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")
        function_id: "EventGraph" or function GUID
        nodes_json: JSON array of node specs, each with:
            - ref_id: your local ID for this node
            - node_type: UE node class (e.g., "K2Node_CallFunction")
            - position: [x, y] (optional)
            - properties: {} (optional, e.g., {"function_name": "PrintString"})

    Returns:
        JSON with ref_id → GUID mapping and any errors.
    """
    command = {
        "type": "add_nodes_bulk",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "nodes": nodes_json,
    }
    response = send_to_unreal(command)
    return json.dumps(response, indent=2)


# ---------------------------------------------------------------------------
# P0: compile_blueprint_with_errors — detailed compilation diagnostics
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def compile_blueprint_with_errors(blueprint_path: str) -> str:
    """
    Compile a Blueprint and return detailed error/warning information.

    Unlike compile_blueprint which only returns true/false, this tool captures
    specific error messages from the compilation log.

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")

    Returns:
        JSON with compiled status, errors array, and warnings array.
    """
    script = f"""
import unreal, json, os, glob

bp_path = '{blueprint_path}'
bp = unreal.load_asset(bp_path)
if bp is None:
    print(json.dumps({{"success": False, "error": "Blueprint not found"}}))
else:
    errors = []
    warnings = []

    # Mark the log position before compile so we only read new lines after
    log_dir = str(unreal.Paths.project_log_dir())
    log_files = sorted(glob.glob(log_dir + '/*.log'), key=lambda f: -os.path.getmtime(f))
    pre_compile_size = 0
    log_file = ""
    if log_files:
        log_file = log_files[0]
        pre_compile_size = os.path.getsize(log_file)

    # Compile using GenBlueprintUtils (the working C++ path)
    compiled = unreal.GenBlueprintUtils.compile_blueprint(bp_path)

    # Check if generated class exists (definitive compile success indicator)
    gen = bp.generated_class()
    has_gen_class = gen is not None

    # Read new log lines written during compile
    if log_file:
        try:
            with open(log_file, 'r', encoding='utf-8', errors='ignore') as f:
                f.seek(pre_compile_size)
                new_lines = f.readlines()
            bp_name = bp.get_name()
            # Only keep blueprint-related diagnostics, filter out script echo noise
            relevant_prefixes = ['LogBlueprint:', 'LogK2Compiler:', 'LogClass:', 'LogUObjectGlobals:']
            for line in new_lines:
                if bp_name not in line and bp_path not in line:
                    continue
                # Skip Python/AI Plugin log echo lines
                if 'LogPython:' in line or '[AI Plugin]' in line:
                    continue
                is_relevant = any(p in line for p in relevant_prefixes)
                if not is_relevant:
                    continue
                if 'Error' in line:
                    errors.append(line.strip())
                elif 'Warning' in line:
                    warnings.append(line.strip())
        except:
            pass

    print(json.dumps({{
        "success": True,
        "compiled": bool(compiled) and has_gen_class,
        "has_generated_class": has_gen_class,
        "errors": errors[-20:],
        "warnings": warnings[-10:]
    }}))
"""
    resp = send_to_unreal({"type": "execute_python", "script": script})
    if resp.get("success"):
        output = resp.get("output", "")
        for line in output.strip().split("\n"):
            if line.strip().startswith("{"):
                try:
                    inner = json.loads(line.strip())
                    compiled = inner.get("compiled", False)
                    errors = inner.get("errors", [])
                    warnings = inner.get("warnings", [])
                    summary = (
                        f"Compiled OK" if compiled else
                        f"Compile failed: {len(errors)} error(s)"
                    )
                    # --- P1-T1: Structure compile errors with node GUID mapping ---
                    structured_errors = []
                    for raw_err in errors:
                        entry = {"message": raw_err, "raw_log": raw_err, "severity": "error"}
                        # Extract node display name: matches  node 'XXX'  or  Node "XXX"
                        import re as _re
                        nm = _re.search(r"[Nn]ode ['\"]([^'\"]+)['\"]", raw_err)
                        gm = _re.search(r"[Gg]raph ['\"]([^'\"]+)['\"]", raw_err)
                        if nm:
                            entry["node_title"] = nm.group(1)
                        if gm:
                            entry["graph_name"] = gm.group(1)
                        structured_errors.append(entry)

                    # Attempt title→GUID mapping using a fresh list_graphs+get_all_nodes call
                    try:
                        lg = send_to_unreal({"type": "list_graphs", "blueprint_path": blueprint_path})
                        if lg.get("success"):
                            for _g in lg.get("graphs", []):
                                nodes_resp = send_to_unreal({
                                    "type": "get_all_nodes",
                                    "blueprint_path": blueprint_path,
                                    "function_id": _g.get("graph_guid", _g.get("graph_name", "")),
                                })
                                if nodes_resp.get("success"):
                                    _nodes = nodes_resp.get("nodes", [])
                                    if isinstance(_nodes, str):
                                        _nodes = json.loads(_nodes)
                                    t2g = {}
                                    for _n in _nodes:
                                        t = _n.get("node_title", _n.get("title", ""))
                                        g = _n.get("node_guid", _n.get("guid", ""))
                                        if t and g:
                                            t2g.setdefault(t, []).append(g)
                                    for se in structured_errors:
                                        nt = se.get("node_title")
                                        if nt and nt in t2g:
                                            candidates = t2g[nt]
                                            if len(candidates) == 1:
                                                se["node_guid"] = candidates[0]
                                                se["mapping_confidence"] = "high"
                                            else:
                                                se["node_guid_candidates"] = candidates
                                                se["mapping_confidence"] = "low"
                                        elif nt:
                                            se["mapping_confidence"] = "none"
                    except Exception:
                        pass  # mapping is best-effort; raw errors still returned

                    # --- US-004: Add probable_patch_ops and post_compile_hint ---
                    import re as _re2
                    _PIN_PAT = _re2.compile(r"[Pp]in ['\"]([^'\"]+)['\"]")
                    for se in structured_errors:
                        msg = se.get("message", "")
                        ops = []
                        # Infer probable patch operations from error patterns
                        if "pin" in msg.lower() and ("not found" in msg.lower() or "cannot" in msg.lower()):
                            ops.append("fix_pin_name_in_add_connections")
                            pm = _PIN_PAT.search(msg)
                            if pm:
                                se["pin_name_in_error"] = pm.group(1)
                        if "type mismatch" in msg.lower() or "incompatible" in msg.lower():
                            ops.append("remove_incompatible_connection")
                            ops.append("check_pin_types_with_get_node_details")
                        if "undefined" in msg.lower() or "not found" in msg.lower():
                            if se.get("node_title"):
                                ops.append("verify_node_exists_with_describe_blueprint")
                        if "variable" in msg.lower():
                            ops.append("verify_variable_exists_with_get_blueprint_variables")
                        if "function" in msg.lower():
                            ops.append("verify_function_name_with_search_blueprint_nodes")
                        se["probable_patch_ops"] = ops if ops else ["inspect_error_manually"]

                    # Build post_compile_hint
                    hint_actions = []
                    has_failures = not compiled
                    if has_failures:
                        hint_actions.append("call_describe_blueprint_force_refresh")
                        if any(se.get("node_guid") for se in structured_errors):
                            hint_actions.append("call_get_node_details_for_flagged_nodes")
                        if any(se.get("node_guid_candidates") for se in structured_errors):
                            hint_actions.append("inspect_candidate_node_guids_to_disambiguate")
                        if any("pin" in se.get("message", "").lower() for se in structured_errors):
                            hint_actions.append("use_get_node_details_to_verify_exact_pin_names")
                        hint_actions.append("rerun_compile_after_fix")

                    inner["structured_errors"] = structured_errors
                    inner["post_compile_hint"] = {
                        "compiled": compiled,
                        "recommended_next_actions": hint_actions,
                        "note": "structured_errors[].node_guid_candidates lists ambiguous nodes — use get_node_details to identify correct target",
                    } if has_failures else {
                        "compiled": True,
                        "recommended_next_actions": [],
                    }

                    envelope = _make_envelope(
                        "compile_blueprint_with_errors", compiled, inner,
                        summary=summary,
                        warnings=warnings,
                        diagnostics=structured_errors,
                    )
                    return json.dumps(envelope, separators=(",", ":"), ensure_ascii=False)
                except Exception:
                    return line.strip()
    return json.dumps(_make_envelope(
        "compile_blueprint_with_errors", False,
        {"error": resp.get("error", "Unknown")},
        summary="Compile failed: could not reach UE",
    ), separators=(",", ":"), ensure_ascii=False)


# ---------------------------------------------------------------------------
# P0: auto_layout_graph — automatic node arrangement
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def auto_layout_graph(blueprint_path: str, function_id: str = "EventGraph") -> str:
    """
    Automatically arrange nodes in a Blueprint graph for readability.

    Uses a simple layered layout: entry events on the left, downstream nodes
    spaced rightward by exec flow depth, with vertical spacing to avoid overlap.

    Args:
        blueprint_path: Path to the Blueprint
        function_id: "EventGraph" or function GUID

    Returns:
        JSON with number of nodes repositioned.
    """
    # Get current graph description to determine layout
    desc_resp = send_to_unreal({
        "type": "get_all_nodes_with_details",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
    })
    if not desc_resp.get("success"):
        # Fallback
        desc_resp = send_to_unreal({
            "type": "get_all_nodes",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
        })
        if not desc_resp.get("success"):
            return json.dumps({"success": False, "error": desc_resp.get("error")})

    nodes = desc_resp.get("nodes", [])
    if isinstance(nodes, str):
        nodes = json.loads(nodes)

    # Build exec adjacency
    exec_adj = {}
    node_by_guid = {}
    for n in nodes:
        guid = n.get("node_guid", n.get("id", ""))
        node_by_guid[guid] = n
        for p in n.get("pins", []):
            if p.get("type") == "exec" and p["direction"] == "output" and p.get("connected_to"):
                for conn in p.get("connected_to", []):
                    exec_adj.setdefault(guid, []).append(conn["node_guid"])

    # Find entries
    entries = []
    for guid, n in node_by_guid.items():
        nt = n.get("node_type", "")
        if "Event" in nt or "FunctionEntry" in nt:
            entries.append(guid)

    # BFS to assign layers
    layer = {}
    queue = [(e, 0) for e in entries]
    visited = set()
    for e, _ in queue:
        visited.add(e)
        layer[e] = 0

    while queue:
        cur, depth = queue.pop(0)
        for nxt in exec_adj.get(cur, []):
            if nxt not in visited:
                visited.add(nxt)
                layer[nxt] = depth + 1
                queue.append((nxt, depth + 1))

    # Assign unvisited nodes to layer -1 (pure/disconnected)
    for guid in node_by_guid:
        if guid not in layer:
            layer[guid] = -1

    # Group by layer, assign positions
    H_SPACING = 400
    V_SPACING = 200
    layers = {}
    for guid, l in layer.items():
        layers.setdefault(l, []).append(guid)

    moved = 0
    for l, guids in sorted(layers.items()):
        x = l * H_SPACING if l >= 0 else -H_SPACING
        for i, guid in enumerate(guids):
            y = i * V_SPACING
            resp = send_to_unreal({
                "type": "move_node",
                "blueprint_path": blueprint_path,
                "function_id": function_id,
                "node_guid": guid,
                "new_x": float(x),
                "new_y": float(y),
            })
            if resp.get("success"):
                moved += 1

    resp = {"success": True, "nodes_moved": moved}
    resp["warning"] = (
        "auto_layout_graph uses UE's built-in auto-arrange algorithm, which may overwrite "
        "positions computed by apply_blueprint_patch's layout_engine.py. "
        "Prefer omitting 'position' in add_nodes to use the patch engine's layout instead."
    )
    return json.dumps(resp)


# ---------------------------------------------------------------------------
# P1: find_scene_objects — filtered scene query
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def find_scene_objects(
    class_filter: str = "",
    name_filter: str = "",
    tag_filter: str = "",
) -> str:
    """
    Find actors in the level with optional filtering by class, name, or tag.

    Much more efficient than get_all_scene_objects for large scenes.

    Args:
        class_filter: Filter by class name (substring match, e.g., "Character", "StaticMesh")
        name_filter: Filter by actor label (substring match, e.g., "NPC", "Door")
        tag_filter: Filter by actor tag (exact match)

    Returns:
        JSON array of matching actors with name, class, location.
    """
    script = f"""
import unreal, json

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
results = []
class_f = '{class_filter}'.lower()
name_f = '{name_filter}'.lower()
tag_f = '{tag_filter}'

for a in actors:
    cls = a.get_class().get_name()
    label = a.get_actor_label()
    loc = a.get_actor_location()

    if class_f and class_f not in cls.lower():
        continue
    if name_f and name_f not in label.lower():
        continue
    if tag_f:
        tags = [str(t) for t in a.tags]
        if tag_f not in tags:
            continue

    results.append({{
        "name": label,
        "class": cls,
        "location": {{"x": loc.x, "y": loc.y, "z": loc.z}}
    }})

print(json.dumps(results, default=str))
"""
    resp = send_to_unreal({"type": "execute_python", "script": script})
    if resp.get("success"):
        output = resp.get("output", "").strip()
        for line in output.split("\n"):
            if line.strip().startswith("["):
                return line.strip()
    return json.dumps([])


# ---------------------------------------------------------------------------
# P1: get_blueprint_variables — all variables in one call
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def get_blueprint_variables(blueprint_path: str) -> str:
    """
    Get all Blueprint variables (name, type, default value, category) in one call.

    Args:
        blueprint_path: Path to the Blueprint

    Returns:
        JSON array of variables with name, type, default_value, category.
    """
    # Use C++ GenBlueprintUtils.GetBlueprintVariables to read Blueprint->NewVariables directly.
    # This returns ALL variables including unused ones.
    resp = send_to_unreal({"type": "get_blueprint_variables", "blueprint_path": blueprint_path})
    if resp.get("success"):
        return json.dumps({"success": True, "variables": resp.get("variables", [])}, ensure_ascii=False)
    return json.dumps({"success": False, "error": resp.get("error", "Failed to get variables")})


# ---------------------------------------------------------------------------
# P1: create_blueprint_from_template — atomic full creation
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def apply_blueprint_spec(spec_json: str, dry_run: bool = False) -> str:
    """Apply a declarative Blueprint Spec in one shot.

    This is the **high-level entry point** for Blueprint creation and modification.
    Prefer this over multiple apply_blueprint_patch calls when you want to:
    - Create a new Blueprint with variables, components, functions, and graph logic
    - Add several members + graph changes atomically
    - Specify intent declaratively rather than as low-level node patches

    Execution pipeline (two phases):
      Plan/Lower: spec → member ops + graph patches (no side effects)
      Execute:    preflight → transaction apply → compile → save

    Spec v1 format:
    {
      "blueprint": "/Game/Blueprints/BP_Enemy",
      "parent": "Character",          // only used if create_if_missing=true
      "create_if_missing": true,
      "variables": [
        {"name": "Health", "type": "float", "default": 100, "category": "Stats"}
      ],
      "components": [
        {"name": "Mesh", "class": "SkeletalMeshComponent"}
      ],
      "custom_events": [
        {"name": "OnTakeDamage", "inputs": [{"name": "Damage", "type": "float"}]}
      ],
      "functions": [
        {"name": "CalculateDamage",
         "inputs": [{"name": "Raw", "type": "float"}],
         "outputs": [{"name": "Final", "type": "float"}]}
      ],
      "graphs": {
        "EventGraph": {
          "nodes": [
            {"ref_id": "begin", "node_type": "EventBeginPlay"},
            {"ref_id": "print", "node_type": "K2Node_CallFunction",
             "function_name": "PrintString", "pin_values": {"InString": "Ready"}}
          ],
          "edges": [{"from": "begin.then", "to": "print.execute"}]
        }
      },
      "compile": true,
      "save": true,
      "auto_layout": false
    }

    Args:
        spec_json: Blueprint spec as a JSON string.
        dry_run:   If true, validate + lower but do NOT execute. Returns the
                   planned operations for inspection. Safe to call before live apply.

    Returns:
        JSON: {
          "success": bool,
          "dry_run": bool,
          "blueprint_path": str,
          "ops": [{"kind", "description", "success", "error", "duration_ms"}, ...],
          "compile_errors": [...],
          "summary": str
        }
    """
    from spec_engine import execute_spec

    result = execute_spec(spec_json, send_to_unreal, dry_run=dry_run)

    return json.dumps({
        "success": result.success,
        "dry_run": result.dry_run,
        "blueprint_path": result.blueprint_path,
        "ops": [
            {
                "kind": op.kind,
                "description": op.description,
                "success": op.success,
                "error": op.error,
                "duration_ms": round(op.duration_ms, 1),
            }
            for op in result.ops
        ],
        "compile_errors": result.compile_errors,
        "summary": result.summary(),
    }, ensure_ascii=False)


@exposed_tool(group="core")
def create_blueprint_from_template(
    blueprint_name: str,
    parent_class: str = "Actor",
    save_path: str = "/Game/Blueprints",
    patch_json: str = "{}",
) -> str:
    """
    Create a new Blueprint AND apply a full node/connection patch in one atomic call.

    Combines create_blueprint + apply_blueprint_patch + compile into one tool.

    Args:
        blueprint_name: Name for the new Blueprint
        parent_class: Parent class (e.g., "Actor", "Character", "Pawn")
        save_path: Save directory (e.g., "/Game/Blueprints")
        patch_json: Full patch JSON (same format as apply_blueprint_patch)

    Returns:
        JSON with blueprint_path and patch results.
    """
    # Step 1: Create
    create_resp = send_to_unreal({
        "type": "create_blueprint",
        "blueprint_name": blueprint_name,
        "parent_class": parent_class,
        "save_path": save_path,
    })
    if not create_resp.get("success"):
        return json.dumps({"success": False, "error": f"Create failed: {create_resp.get('error')}"})

    bp_path = create_resp.get("blueprint_path", f"{save_path}/{blueprint_name}")

    # Step 2: Apply patch if provided
    try:
        patch = json.loads(patch_json)
    except json.JSONDecodeError:
        patch = {}

    if patch and any(patch.get(k) for k in ("add_nodes", "add_connections", "set_pin_values")):
        # Use apply_blueprint_patch logic directly
        patch_result = apply_blueprint_patch(bp_path, "EventGraph", patch_json)
        try:
            result = json.loads(patch_result)
            result["blueprint_path"] = bp_path
            return json.dumps(result, indent=2)
        except:
            return json.dumps({"success": True, "blueprint_path": bp_path, "patch_raw": patch_result})

    return json.dumps({"success": True, "blueprint_path": bp_path, "note": "Created empty, no patch applied"})


# ---------------------------------------------------------------------------
# P1: undo_last_operation — safety net
# ---------------------------------------------------------------------------

@exposed_tool(group="admin")
def undo_last_operation() -> str:
    """
    Undo the last editor operation (safety net for blueprint/level modifications).

    Returns:
        Success or failure message.
    """
    resp = send_to_unreal({"type": "undo_transaction"})
    if resp.get("success"):
        return json.dumps({"success": True, "message": "Undo executed"})
    return json.dumps({"success": False, "error": resp.get("error", "Undo failed")})


# ---------------------------------------------------------------------------
# Blueprint Index — fuzzy discovery + change detection
# ---------------------------------------------------------------------------

@exposed_tool(group="admin")
def build_blueprint_index() -> str:
    """
    Build or rebuild the project blueprint index for fuzzy search.

    Scans all Blueprint assets under /Game, collects metadata (name, parent class,
    graphs, node counts, entry events), and saves to .claude/blueprint-index.json.

    Use this:
    - On first /bp call when no index exists
    - When the user says "rebuild index" or "重建索引"
    - When blueprints have been added/removed outside MCP

    Returns:
        JSON summary with blueprint count.
    """
    import re
    from datetime import datetime

    # 1. Scan all blueprint assets via C++
    scan_resp = send_to_unreal({"type": "scan_all_blueprints"})
    if not scan_resp.get("success"):
        return json.dumps({"success": False, "error": scan_resp.get("error", "Scan failed")})

    raw_bps = scan_resp.get("blueprints", [])

    # 2. For each BP, get graph metadata via list_graphs
    index_entries = []
    for bp_info in raw_bps:
        bp_path = bp_info.get("path", "")
        bp_name = bp_info.get("name", "")
        parent_class = bp_info.get("parent_class", "Unknown")

        # Clean path: remove ".BP_Name" suffix if present (asset path format)
        if "." in bp_path:
            bp_path = bp_path.rsplit(".", 1)[0]

        # Get graphs
        graph_resp = send_to_unreal({
            "type": "list_graphs",
            "blueprint_path": bp_path,
        })

        graphs = []
        total_nodes = 0
        entry_events = []

        if graph_resp.get("success"):
            raw_graphs = graph_resp.get("graphs", [])
            for g in raw_graphs:
                nc = g.get("node_count", 0)
                graphs.append({
                    "name": g.get("graph_name", ""),
                    "type": g.get("graph_type", ""),
                    "node_count": nc,
                })
                total_nodes += nc

            # Get entry events from EventGraph (lightweight: use describe minimal)
            for g in raw_graphs:
                if g.get("graph_type") == "EventGraph":
                    desc_resp = send_to_unreal({
                        "type": "get_all_nodes_with_details",
                        "blueprint_path": bp_path,
                        "function_id": g.get("graph_name", "EventGraph"),
                    })
                    if desc_resp.get("success"):
                        for node in desc_resp.get("nodes", []):
                            nt = node.get("node_type", "")
                            if nt in ("K2Node_Event", "K2Node_CustomEvent",
                                      "K2Node_ComponentBoundEvent", "K2Node_InputAction",
                                      "K2Node_EnhancedInputAction"):
                                entry_events.append(node.get("node_title", ""))
                    break  # Only check first EventGraph

        # Generate short names for fuzzy matching
        short_names = []
        # Strip BP_ prefix
        stripped = re.sub(r'^BP_', '', bp_name)
        short_names.append(stripped)
        # Split CamelCase
        split = re.sub(r'([a-z])([A-Z])', r'\1 \2', stripped)
        if split != stripped:
            short_names.append(split)
        # Chinese aliases for common types
        _CN_MAP = {
            "Character": "角色", "Controller": "控制器", "GameMode": "游戏模式",
            "Player": "玩家", "NPC": "NPC", "AI": "AI", "Door": "门",
            "Button": "按钮", "Vase": "花瓶", "Item": "物品", "Weapon": "武器",
            "Health": "血量", "Manager": "管理器", "Pickup": "拾取",
        }
        for eng, cn in _CN_MAP.items():
            if eng.lower() in bp_name.lower():
                short_names.append(cn)

        # Fingerprint for change detection
        graph_types = "-".join(sorted(set(g["type"] for g in graphs)))
        fingerprint = f"{total_nodes}-{len(graphs)}-{graph_types}"

        index_entries.append({
            "path": bp_path,
            "name": bp_name,
            "short_names": list(set(short_names)),
            "parent_class": parent_class,
            "graphs": graphs,
            "total_nodes": total_nodes,
            "entry_events": entry_events,
            "fingerprint": fingerprint,
        })

    # 3. Save to .claude/blueprint-index.json
    index_data = {
        "version": 1,
        "built_at": datetime.now().isoformat(),
        "blueprint_count": len(index_entries),
        "blueprints": index_entries,
    }

    index_path = _get_project_root() / ".claude" / "blueprint-index.json"
    index_path.parent.mkdir(parents=True, exist_ok=True)
    index_path.write_text(json.dumps(index_data, indent=2, ensure_ascii=False), encoding="utf-8")

    return json.dumps({
        "success": True,
        "blueprint_count": len(index_entries),
        "index_path": str(index_path),
        "summary": [f"{e['name']} ({e['total_nodes']} nodes, {e['parent_class']})" for e in index_entries],
    }, indent=2, ensure_ascii=False)


# ---------------------------------------------------------------------------
# P2: search_node_type — proactive fuzzy search
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def search_node_type(query: str) -> str:
    """
    Search for Blueprint node types matching a query string.

    Use this BEFORE adding nodes to discover the exact node type name.

    Args:
        query: Search term (e.g., "Print", "Delay", "SpawnActor", "Add")

    Returns:
        JSON array of matching node type names.
    """
    command = {
        "type": "get_node_suggestions",
        "node_type": query,
    }
    response = send_to_unreal(command)
    if response.get("success"):
        return json.dumps(response.get("suggestions", []), indent=2)
    return json.dumps({"suggestions": response.get("suggestions", str(response))})


# ---------------------------------------------------------------------------
# P2: search_blueprint_nodes — full-engine node search (lightweight shortlist)
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def search_blueprint_nodes(
    query: str,
    blueprint_path: str = "",
    category_filter: str = "",
    max_results: int = 5,
    schema_mode: str = "semantic",
) -> str:
    """Search ALL available Blueprint node types across the entire engine."""
    return _impl_search_blueprint_nodes(query, blueprint_path=blueprint_path,
                                        category_filter=category_filter,
                                        max_results=max_results,
                                        schema_mode=schema_mode, send_fn=send_to_unreal)


# ---------------------------------------------------------------------------
# P2: inspect_blueprint_node — full pin schema for a specific node type
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def inspect_blueprint_node(
    canonical_name: str,
    blueprint_path: str = "",
    schema_mode: str = "semantic",
) -> str:
    """Get full pin schema and patch_hint for a specific Blueprint node type."""
    return _impl_inspect_blueprint_node(canonical_name, blueprint_path=blueprint_path,
                                        schema_mode=schema_mode, send_fn=send_to_unreal)


# ---------------------------------------------------------------------------
# P2: get_compilation_status — check without recompiling
# ---------------------------------------------------------------------------

# @exposed_tool(group="core")  # INTERNAL — use higher-level tools instead
def get_compilation_status(blueprint_path: str) -> str:
    """
    Check a Blueprint's current compilation status WITHOUT triggering a recompile.

    Args:
        blueprint_path: Path to the Blueprint

    Returns:
        JSON with status: "UpToDate", "Dirty", "Error", or "Unknown".
    """
    script = f"""
import unreal, json

bp = unreal.load_asset('{blueprint_path}')
if bp is None:
    print(json.dumps({{"success": False, "error": "Blueprint not found"}}))
else:
    status = "Unknown"
    try:
        s = bp.status
        if s == unreal.BlueprintCompileStatus.UP_TO_DATE:
            status = "UpToDate"
        elif s == unreal.BlueprintCompileStatus.DIRTY:
            status = "Dirty"
        elif s == unreal.BlueprintCompileStatus.ERROR:
            status = "Error"
        else:
            status = str(s)
    except:
        pass
    print(json.dumps({{"success": True, "status": status, "blueprint": '{blueprint_path}'}}))
"""
    resp = send_to_unreal({"type": "execute_python", "script": script})
    if resp.get("success"):
        output = resp.get("output", "").strip()
        for line in output.split("\n"):
            if line.strip().startswith("{"):
                return line.strip()
    return json.dumps({"success": False, "error": resp.get("error", "Unknown")})


# ---------------------------------------------------------------------------
# P2: diff_blueprint — compare two describe_blueprint snapshots
# ---------------------------------------------------------------------------

@exposed_tool(group="core")  # was analysis; used in blueprint-edit Step 4 verify workflow
def diff_blueprint(before_json: str, after_json: str) -> str:
    """
    Compare two describe_blueprint snapshots and return the differences.

    Both inputs should be the JSON output from describe_blueprint.
    This is a pure comparison — no socket calls needed.

    Args:
        before_json: JSON string from describe_blueprint (before state)
        after_json: JSON string from describe_blueprint (after state)

    Returns:
        JSON with added_nodes, removed_nodes, added_connections, removed_connections,
        changed_pin_values arrays.
    """
    try:
        before = json.loads(before_json)
        after = json.loads(after_json)
    except json.JSONDecodeError as e:
        return json.dumps({"success": False, "error": f"Invalid JSON: {e}"})

    def _collect_nodes(bp_data):
        nodes = {}
        subgraphs = bp_data.get("blueprint", bp_data).get("subgraphs", {})
        for sg_list in (subgraphs.get("event_flows", []) if isinstance(subgraphs, dict)
                        else subgraphs):
            for n in sg_list.get("nodes", []) if isinstance(sg_list, dict) else []:
                nodes[n["id"]] = n
        return nodes

    def _collect_edges(bp_data, edge_type):
        edges = set()
        subgraphs = bp_data.get("blueprint", bp_data).get("subgraphs", {})
        for sg_list in (subgraphs.get("event_flows", []) if isinstance(subgraphs, dict)
                        else subgraphs):
            for e in sg_list.get(f"{edge_type}_edges", []) if isinstance(sg_list, dict) else []:
                edges.add((e.get("from_pin", ""), e.get("to_pin", "")))
        return edges

    before_nodes = _collect_nodes(before)
    after_nodes = _collect_nodes(after)

    added_nodes = [after_nodes[g] for g in set(after_nodes) - set(before_nodes)]
    removed_nodes = [before_nodes[g] for g in set(before_nodes) - set(after_nodes)]

    before_exec = _collect_edges(before, "exec")
    after_exec = _collect_edges(after, "exec")
    before_data = _collect_edges(before, "data")
    after_data = _collect_edges(after, "data")

    result = {
        "success": True,
        "added_nodes": [{"id": n["id"], "title": n.get("title", "")} for n in added_nodes],
        "removed_nodes": [{"id": n["id"], "title": n.get("title", "")} for n in removed_nodes],
        "added_exec_connections": [{"from": f, "to": t} for f, t in after_exec - before_exec],
        "removed_exec_connections": [{"from": f, "to": t} for f, t in before_exec - after_exec],
        "added_data_connections": [{"from": f, "to": t} for f, t in after_data - before_data],
        "removed_data_connections": [{"from": f, "to": t} for f, t in before_data - after_data],
    }
    return json.dumps(result, indent=2)


# ---------------------------------------------------------------------------
# get_node_neighborhood — BFS context query around a single Blueprint node
# ---------------------------------------------------------------------------

@exposed_tool(group="analysis")
def get_node_neighborhood(
    blueprint_path: str,
    node_ref: str,
    graph_name: str = "",
    depth: int = 2,
    direction: str = "exec",
    compact: bool = True,
) -> str:
    """Return the N-hop neighborhood around a specific Blueprint node.

    Useful for understanding local execution context without a full
    describe_blueprint dump.  Works like 'grep with context': find a node
    by name/GUID and see what feeds into it and follows from it.

    Args:
        blueprint_path: Path to the Blueprint (e.g. "/Game/Blueprints/BP_NPC")
        node_ref: Node identifier — one of:
                  • 32-char hex GUID
                  • instance_id ("K2Node_CallFunction_3" or "K2Node_CallFunction#3")
                  • title regex pattern ("PrintString", "BeginPlay", "On.*Ready")
        graph_name: Specific graph to search. Empty = EventGraph.
        depth: BFS hops from the center node (1–5, default 2).
               1 = direct neighbors only.  5 is the maximum.
        direction: Which edges to follow —
                   "exec_out"  downstream exec flow
                   "exec_in"   upstream exec flow
                   "exec"      both exec directions (default)
                   "data"      data pins both directions
                   "all"       exec + data all directions
        compact: Return compact JSON (default True, saves tokens).

    Returns:
        JSON: {
          "center":    {guid, title, instance_id, node_type},
          "neighbors": [{guid, title, instance_id, node_type,
                         distance, direction, via_pin}, ...],
          "edges":     [{from, to, type}, ...],
          "total_nodes": int,
          "warnings":  [...],
        }

        On error: {"error": "...", ...}
    """
    _VALID_DIRECTIONS = {"exec_out", "exec_in", "exec", "data", "all"}

    # Validate inputs
    if not (1 <= depth <= 5):
        return json.dumps({"error": f"depth must be between 1 and 5, got {depth}"})
    if direction not in _VALID_DIRECTIONS:
        return json.dumps({
            "error": f"Invalid direction '{direction}'. Must be one of: {sorted(_VALID_DIRECTIONS)}",
        })

    # Resolve graph
    graphs_resp = send_to_unreal({"type": "list_graphs", "blueprint_path": blueprint_path})
    if not graphs_resp.get("success"):
        return json.dumps({"error": f"Failed to list graphs: {graphs_resp.get('error', 'unknown')}"})

    all_graphs = graphs_resp.get("graphs", [])
    target_graph = None
    if graph_name:
        for g in all_graphs:
            if g.get("graph_name") == graph_name:
                target_graph = g
                break
        if target_graph is None:
            available = [g.get("graph_name") for g in all_graphs]
            return json.dumps({"error": f"Graph '{graph_name}' not found", "available": available})
    else:
        for g in all_graphs:
            if g.get("graph_type") == "EventGraph":
                target_graph = g
                break
        if target_graph is None and all_graphs:
            target_graph = all_graphs[0]

    if target_graph is None:
        return json.dumps({"error": "No graphs found in blueprint"})

    resolved_graph_name = target_graph.get("graph_name", "EventGraph")

    # Cache check
    cache_tag = f"nbhd:{node_ref}:{depth}:{direction}"
    cache_key = (blueprint_path, resolved_graph_name, cache_tag)
    fingerprint = json.dumps(
        {g.get("graph_name"): g.get("node_count", 0) for g in all_graphs},
        sort_keys=True,
    )
    cached = _describe_cache.get(cache_key, fingerprint)
    if cached:
        return cached

    # Batch fetch all nodes
    details_by_guid, fetch_err = _graph_fetch_nodes(blueprint_path, target_graph, send_to_unreal)
    if fetch_err:
        return json.dumps({"error": f"Failed to fetch nodes: {fetch_err}"})

    # Resolve node_ref
    center_guid, warnings, candidates = _resolve_neighborhood_node_ref(node_ref, details_by_guid)
    if center_guid is None:
        result = {"error": warnings[0] if warnings else "Node not found", "node_ref": node_ref}
        if candidates:
            result["candidates"] = candidates
        return json.dumps(result)

    # BFS traversal
    neighbors, edges = _bfs_neighborhood(center_guid, details_by_guid, depth, direction)

    # Build center record
    center_det = details_by_guid.get(center_guid, {})
    center = {
        "guid": center_guid,
        "title": center_det.get("node_title", ""),
        "instance_id": center_det.get("instance_id", ""),
        "node_type": center_det.get("node_type", ""),
    }

    result = {
        "center": center,
        "neighbors": neighbors,
        "edges": edges,
        "total_nodes": 1 + len(neighbors),
        "warnings": warnings,
    }

    output = (
        json.dumps(result, separators=(",", ":"), ensure_ascii=False)
        if compact
        else json.dumps(result, indent=2, ensure_ascii=False)
    )
    _describe_cache.set(cache_key, fingerprint, output)
    return output


@exposed_tool(group="admin")
def reload_handlers(include_utils: bool = False) -> str:
    """Hot-reload Python handler modules in the UE socket server.
    Use after editing handler code to pick up changes without restarting the Editor.
    Set include_utils=True to also reload utility modules (unreal_conversions, logging)."""
    result = send_to_unreal({"type": "reload_handlers", "include_utils": include_utils})
    if result.get("success"):
        mods = ", ".join(result.get("reloaded", []))
        rev = result.get("revision", "?")
        return f"OK (rev {rev}): reloaded {mods}"
    else:
        errors = "\n".join(result.get("errors", []))
        return f"FAILED — handler map unchanged.\nErrors:\n{errors}"


# ---------------------------------------------------------------------------
# Subgraph templates — common Blueprint patterns as single tool calls
# ---------------------------------------------------------------------------

_SUBGRAPH_TEMPLATES = {
    "branch_print": {
        "description": "Branch on a condition, PrintString on each path (True/False)",
        "params": {
            "true_text": {"type": "string", "default": "True"},
            "false_text": {"type": "string", "default": "False"},
        },
        "build": lambda p: {
            "add_nodes": [
                {"ref_id": "_TPL_Branch", "node_type": "K2Node_IfThenElse", "position": [0, 0]},
                {"ref_id": "_TPL_PrintTrue", "node_type": "K2Node_CallFunction",
                 "function_name": "PrintString", "position": [300, -100],
                 "pin_values": {"InString": p.get("true_text", "True")}},
                {"ref_id": "_TPL_PrintFalse", "node_type": "K2Node_CallFunction",
                 "function_name": "PrintString", "position": [300, 100],
                 "pin_values": {"InString": p.get("false_text", "False")}},
            ],
            "add_connections": [
                {"from": "_TPL_Branch.True", "to": "_TPL_PrintTrue.execute"},
                {"from": "_TPL_Branch.False", "to": "_TPL_PrintFalse.execute"},
            ],
        },
        "entry_ref": "_TPL_Branch",
    },
    "delay_then": {
        "description": "Delay for N seconds, then call a function",
        "params": {
            "duration": {"type": "float", "default": "1.0"},
            "then_function": {"type": "string", "default": "PrintString"},
            "then_pin_values": {"type": "json", "default": "{}"},
        },
        "build": lambda p: {
            "add_nodes": [
                {"ref_id": "_TPL_Delay", "node_type": "K2Node_CallFunction",
                 "function_name": "Delay", "position": [0, 0],
                 "pin_values": {"Duration": p.get("duration", "1.0")}},
                {"ref_id": "_TPL_Then", "node_type": "K2Node_CallFunction",
                 "function_name": p.get("then_function", "PrintString"), "position": [300, 0],
                 "pin_values": json.loads(p.get("then_pin_values", "{}")) if isinstance(p.get("then_pin_values"), str) else p.get("then_pin_values", {})},
            ],
            "add_connections": [
                {"from": "_TPL_Delay.then", "to": "_TPL_Then.execute"},
            ],
        },
        "entry_ref": "_TPL_Delay",
    },
    "foreach_print": {
        "description": "ForEachLoop over an array, PrintString each element",
        "params": {
            "array_variable": {"type": "string", "default": "MyArray"},
        },
        "build": lambda p: {
            "add_nodes": [
                {"ref_id": "_TPL_Loop", "node_type": "K2Node_CallFunction",
                 "function_name": "ForEachLoop", "position": [0, 0]},
                {"ref_id": "_TPL_Print", "node_type": "K2Node_CallFunction",
                 "function_name": "PrintString", "position": [300, 0]},
            ],
            "add_connections": [
                {"from": "_TPL_Loop.LoopBody", "to": "_TPL_Print.execute"},
            ],
        },
        "entry_ref": "_TPL_Loop",
    },
    "print_chain": {
        "description": "Chain of N PrintString nodes (for debugging sequences)",
        "params": {
            "messages": {"type": "json_array", "default": '["Step 1","Step 2","Step 3"]'},
        },
        "build": lambda p: _build_print_chain(p),
        "entry_ref": "_TPL_Print_0",
    },
}


def _build_print_chain(params):
    """Build a chain of PrintString nodes from a messages list."""
    msgs_raw = params.get("messages", '["Step 1","Step 2","Step 3"]')
    msgs = json.loads(msgs_raw) if isinstance(msgs_raw, str) else msgs_raw
    nodes = []
    conns = []
    for i, msg in enumerate(msgs):
        nodes.append({
            "ref_id": f"_TPL_Print_{i}",
            "node_type": "K2Node_CallFunction",
            "function_name": "PrintString",
            "position": [i * 300, 0],
            "pin_values": {"InString": str(msg)},
        })
        if i > 0:
            conns.append({"from": f"_TPL_Print_{i-1}.then", "to": f"_TPL_Print_{i}.execute"})
    return {"add_nodes": nodes, "add_connections": conns}


@exposed_tool(group="admin")
def list_subgraph_templates() -> str:
    """List available subgraph templates with descriptions and parameters.
    Templates let you create common Blueprint patterns in a single call,
    saving token costs and MCP round-trips."""
    out = {}
    for name, tpl in _SUBGRAPH_TEMPLATES.items():
        out[name] = {
            "description": tpl["description"],
            "params": {k: {"type": v["type"], "default": v.get("default", "")}
                       for k, v in tpl["params"].items()},
        }
    return json.dumps(out, indent=2, ensure_ascii=False)


@exposed_tool(group="admin")
def apply_subgraph_template(
    blueprint_path: str,
    function_id: str,
    template_name: str,
    params_json: str = "{}",
    connect_after: str = "",
) -> str:
    """Apply a pre-built subgraph template to a Blueprint graph.

    This creates multiple nodes and connections in one call using a predefined pattern.
    Use list_subgraph_templates() to see available templates.

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")
        function_id: "EventGraph" or function GUID
        template_name: Name of the template (e.g., "branch_print", "delay_then")
        params_json: JSON string with template parameters (see list_subgraph_templates for schema)
        connect_after: Optional. Connect template entry to this endpoint (e.g., "Event BeginPlay.then" or "<GUID>.then")

    Returns:
        JSON with patch results (same format as apply_blueprint_patch).
    """
    tpl = _SUBGRAPH_TEMPLATES.get(template_name)
    if not tpl:
        available = list(_SUBGRAPH_TEMPLATES.keys())
        return json.dumps({"success": False, "error": f"Unknown template '{template_name}'. Available: {available}"})

    try:
        params = json.loads(params_json) if isinstance(params_json, str) else params_json
    except json.JSONDecodeError as e:
        return json.dumps({"success": False, "error": f"Invalid params_json: {e}"})

    patch = tpl["build"](params)

    # Connect to preceding node if specified
    if connect_after:
        entry_ref = tpl.get("entry_ref", "")
        if entry_ref:
            patch.setdefault("add_connections", []).append(
                {"from": connect_after, "to": f"{entry_ref}.execute"})

    patch["auto_compile"] = True
    return apply_blueprint_patch(blueprint_path, function_id, json.dumps(patch))


# ---------------------------------------------------------------------------
# P0.2 Git / UE Safety Tools
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def save_all_dirty_packages() -> str:
    """Save all unsaved (dirty) assets under /Game in the editor.

    Call this after any MCP operation that modifies Blueprint or other assets,
    BEFORE running git pull/merge/rebase. Prevents file conflicts from open,
    unsaved packages during source-control operations.

    Returns:
        JSON with: {"success": bool, "count": int, "saved": [...], "failed": [...]}
    """
    resp = send_to_unreal({"type": "save_all_dirty_packages"})
    if not isinstance(resp, dict):
        return json.dumps({"success": False, "error": "No response from UE"})
    return json.dumps(resp)


@exposed_tool(group="core")
def get_node_guid_by_fname(
    blueprint_path: str,
    graph_id: str,
    node_fname: str,
    node_class_filter: str = "",
) -> str:
    """Find any Blueprint graph node by its UObject FName and return its NodeGuid.

    Solves the ForEachLoop body traversal problem: the instance_id resolver only
    traverses exec-chain reachable nodes, so nodes inside loop bodies cannot be
    found by K2NodeType#N instance_id syntax.

    This tool iterates Graph->Nodes directly to find the node by UObject FName.

    Get the FName via execute_python_script:
        obj = unreal.load_object(None, f"{bp.get_path_name()}:EventGraph.K2Node_BreakStruct_0")
        print(obj.get_fname())  # "K2Node_BreakStruct_0"

    Args:
        blueprint_path:    Asset path
        graph_id:          "EventGraph", other graph name, or GUID string
        node_fname:        UObject FName (e.g. "K2Node_BreakStruct_2")
        node_class_filter: Optional class name filter (e.g. "BreakStruct")

    Returns:
        JSON: {"success": bool, "node_guid": str, "node_class": str, "node_name": str}
    """
    resp = send_to_unreal({
        "type": "get_node_guid_by_fname",
        "blueprint_path": blueprint_path,
        "graph_id": graph_id,
        "node_fname": node_fname,
        "node_class_filter": node_class_filter,
    })
    if not isinstance(resp, dict):
        return json.dumps({"success": False, "error": "No response from UE"})
    return json.dumps(resp)


@exposed_tool(group="core")
def add_switch_case(
    blueprint_path: str,
    graph_id: str,
    node_guid: str,
    case_name: str,
) -> str:
    """Add a new named case to a Switch on String (K2Node_SwitchString) node.

    STRUCTURAL MUTATION — this is the correct tool for extending a Switch on String
    node with a new case. Do NOT use apply_blueprint_patch set_pin_values or
    execute_python_script to modify PinNames directly; those approaches do NOT call
    ReconstructNode() at the C++ level, so the exec output pin is never materialized
    and subsequent connect_nodes calls will return src_pins=[].

    This tool explicitly calls ReconstructNode() after adding the case, so the new
    exec output pin is immediately available for apply_blueprint_patch / connect_nodes.

    Workflow after calling this tool:
      1. Use connect_nodes or apply_blueprint_patch to wire the new case output pin
         to its handler node.
      2. Compile with compile_blueprint_with_errors.

    Args:
        blueprint_path: Asset path (e.g. "/Game/Blueprints/Core/BP_NPCAIController")
        graph_id:       Graph name ("EventGraph", "Actions", etc.) or GUID string
        node_guid:      GUID of the K2Node_SwitchString node (get via execute_python_script
                        + ObjectIterator, or from describe_blueprint node IDs)
        case_name:      Case string to add (e.g. "StepOn", "WalkTo")

    Returns:
        JSON: {"success": bool, "case_added": str, "method": str, "pin_count": int}
        method "PinNames+ReconstructNode": case was added and exec pin materialized
        method "already_exists": case was already present, no change made (idempotent)
    """
    resp = send_to_unreal({
        "type": "add_switch_case",
        "blueprint_path": blueprint_path,
        "graph_id": graph_id,
        "node_guid": node_guid,
        "case_name": case_name,
    })
    if not isinstance(resp, dict):
        return json.dumps({"success": False, "error": "No response from UE"})

    # Structural mutation: invalidate all caches (GUID may have drifted)
    if resp.get("success") and resp.get("method") != "already_exists":
        _evict_bp_caches(blueprint_path)

    return json.dumps(resp)


@exposed_tool(group="core")
def check_ue_editor_running() -> str:
    """Check whether Unreal Editor is currently running.

    Use this before git pull/merge/rebase/checkout operations on branches that
    contain Blueprint assets. UE must be CLOSED before these operations to
    avoid file lock conflicts and binary .uasset corruption.

    Returns:
        JSON with: {"running": bool, "pid": int | null, "warning": str | null}
    """
    import psutil
    ue_process_names = {"UnrealEditor", "UnrealEditor-Win64-Debug", "UE4Editor", "UE5Editor"}
    for proc in psutil.process_iter(["pid", "name"]):
        try:
            if any(name in proc.info["name"] for name in ue_process_names):
                return json.dumps({
                    "running": True,
                    "pid": proc.info["pid"],
                    "warning": (
                        "Unreal Editor is running. Close it before git pull/merge/rebase "
                        "to avoid .uasset lock conflicts and binary corruption."
                    ),
                })
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            continue
    return json.dumps({"running": False, "pid": None, "warning": None})


@exposed_tool(group="core")
def safe_pull(remote: str = "origin", branch: str = "") -> str:
    """Run git pull after verifying Unreal Editor is closed and all assets are saved.

    Safety checks performed:
    1. Verify UE Editor is NOT running (raises error if it is)
    2. Save all dirty packages via save_all_dirty_packages
    3. Run git pull

    Args:
        remote: Git remote name (default: "origin")
        branch: Branch to pull (default: current branch)

    Returns:
        JSON with: {"success": bool, "step": str, "output": str, "error": str | null}
    """
    import subprocess

    # Step 1: Check UE Editor
    ue_check = json.loads(check_ue_editor_running())
    if ue_check.get("running"):
        return json.dumps({
            "success": False,
            "step": "ue_check",
            "output": "",
            "error": ue_check["warning"],
        })

    # Step 2: Save dirty packages (best-effort — UE must be running for this to work)
    # If UE isn't running this is a no-op, which is fine for pure git operations
    try:
        save_resp = send_to_unreal({"type": "save_all_dirty_packages"})
        failed = save_resp.get("failed", [])
        if failed:
            return json.dumps({
                "success": False,
                "step": "save_dirty",
                "output": "",
                "error": f"Failed to save {len(failed)} packages before pull: {failed[:5]}",
            })
    except Exception:
        pass  # UE not running is fine — no unsaved packages to worry about

    # Step 3: git pull
    cmd = ["git", "pull", remote]
    if branch:
        cmd.append(branch)

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        success = result.returncode == 0
        return json.dumps({
            "success": success,
            "step": "git_pull",
            "output": result.stdout.strip(),
            "error": result.stderr.strip() if not success else None,
        })
    except subprocess.TimeoutExpired:
        return json.dumps({"success": False, "step": "git_pull", "output": "", "error": "git pull timed out (>120s)"})
    except Exception as e:
        return json.dumps({"success": False, "step": "git_pull", "output": "", "error": str(e)})


# ---------------------------------------------------------------------------
# P0.4 Transaction Tools (explicit MCP exposure)
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def begin_transaction(transaction_name: str = "MCPEdit") -> str:
    """Begin an atomic UE editor transaction.

    All Blueprint modifications made after this call will be grouped under
    a single undo entry.  Always pair with end_transaction or cancel_transaction.

    Args:
        transaction_name: Label shown in UE Undo history (e.g. "AddHealthLogic").

    Returns:
        JSON: {"ok": bool, "transaction_name": str}
    """
    resp = send_to_unreal({"type": "begin_transaction",
                           "transaction_name": transaction_name})
    ok = resp.get("success", False)
    return json.dumps({"ok": ok, "transaction_name": transaction_name,
                       "error": resp.get("error") if not ok else None})


@exposed_tool(group="core")
def end_transaction() -> str:
    """Commit the current UE editor transaction.

    Call this after all Blueprint modifications are complete.
    Enables Ctrl+Z undo of the entire change set in UE Editor.

    Returns:
        JSON: {"ok": bool}
    """
    resp = send_to_unreal({"type": "end_transaction"})
    ok = resp.get("success", False)
    return json.dumps({"ok": ok, "error": resp.get("error") if not ok else None})


@exposed_tool(group="core")
def cancel_transaction() -> str:
    """Roll back the current UE editor transaction.

    Call this when a modification sequence fails and you want to discard
    all changes made since begin_transaction.

    Returns:
        JSON: {"ok": bool}
    """
    resp = send_to_unreal({"type": "cancel_transaction"})
    ok = resp.get("success", False)
    return json.dumps({"ok": ok, "error": resp.get("error") if not ok else None})


# ---------------------------------------------------------------------------
# P0.1 Job Management Tools
# ---------------------------------------------------------------------------

@exposed_tool(group="analysis")
def job_status(job_id: str) -> str:
    """Query the status of a long-running job (e.g. build_navigation, build_target).

    Args:
        job_id: The job identifier returned when the job was submitted.

    Returns:
        JSON: {
          "job_id": str,
          "label": str,
          "state": "pending" | "running" | "completed" | "failed" | "cancelled" | "not_found",
          "result": any,
          "error": str | null,
          "elapsed_s": float | null
        }
    """
    from job_manager import get_job_status
    return json.dumps(get_job_status(job_id))


@exposed_tool(group="analysis")
def job_cancel(job_id: str) -> str:
    """Cancel a pending job before it runs.

    Only works if the job is still in 'pending' state.
    Running jobs cannot be cancelled mid-execution.

    Args:
        job_id: The job identifier to cancel.

    Returns:
        JSON: {"ok": bool, "job_id": str, "message": str}
    """
    from job_manager import cancel_job
    cancelled = cancel_job(job_id)
    return json.dumps({
        "ok": cancelled,
        "job_id": job_id,
        "message": "Job cancelled." if cancelled else "Job not found or already running/completed.",
    })


@exposed_tool(group="analysis")
def job_list(include_completed: bool = False) -> str:
    """List active (and optionally completed) jobs.

    Args:
        include_completed: If True, also return completed/failed/cancelled jobs.

    Returns:
        JSON array of job status objects.
    """
    from job_manager import list_jobs
    return json.dumps(list_jobs(include_completed))


# ---------------------------------------------------------------------------
# Blueprint Quality Analyzer
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def analyze_blueprint_quality(blueprint_path: str, mode: str = "full", graph_name: str = "") -> str:
    """Run static quality checks on a Blueprint.

    Checks performed:
      C1 — Empty Event Stub (entry nodes with no outgoing exec chain)
      C2 — Orphan Nodes (not connected to anything)
      C3 — Unreachable Nodes (per-graph BFS from entry nodes)
      C4 — Variable Usage (never_referenced / write_only / read_only)
      C5 — Complexity Metrics (node_count, branch_depth, entry_count, etc.)
      C6 — Compile Warnings (filtered to Blueprint-relevant log entries)

    Args:
        blueprint_path: Blueprint asset path (e.g. "/Game/Blueprints/BP_X")
        mode: "quick" (C1+C5+C6 only) or "full" (all checks, default)
        graph_name: Analyze a specific graph only (empty = all graphs)

    Returns:
        JSON QA report with issues, metrics, suppressed, and summary.
    """
    from collections import deque as _deque

    def _qa_analyze(bp_data, mode="full"):
        issues = []
        metrics = {"node_count": 0, "entry_count": 0, "branch_count": 0}
        bp = bp_data.get("blueprint", bp_data)
        for graph in bp.get("graphs", []):
            for flow in graph.get("subgraphs", {}).get("event_flows", []):
                nodes = flow.get("nodes", [])
                exec_edges = flow.get("exec_edges", flow.get("ex", []))
                data_edges = flow.get("data_edges", flow.get("da", []))
                if not nodes:
                    continue
                exec_src = {e.get("fn") for e in exec_edges if e.get("fn")}
                exec_tgt = {e.get("tn") for e in exec_edges if e.get("tn")}
                data_src = {e.get("fn") for e in data_edges if e.get("fn")}
                data_tgt = {e.get("tn") for e in data_edges if e.get("tn")}
                all_conn = exec_src | exec_tgt | data_src | data_tgt
                metrics["node_count"] += len(nodes)
                for n in nodes:
                    role = n.get("role", n.get("r", ""))
                    nt = n.get("nt", "")
                    if role == "entry":
                        metrics["entry_count"] += 1
                    if nt in ("K2Node_IfThenElse", "Branch"):
                        metrics["branch_count"] += 1
                # C1
                for n in nodes:
                    role = n.get("role", n.get("r", ""))
                    nid = n.get("id", "")
                    if role == "entry" and not any(e.get("fn") == nid for e in exec_edges):
                        issues.append({"check": "C1", "severity": "warn",
                            "message": f"Empty event stub: '{n.get('t', nid)}' has no outgoing exec",
                            "node_id": nid, "graph": flow.get("name", "")})
                if mode != "full":
                    continue
                # C2
                for n in nodes:
                    role = n.get("role", n.get("r", ""))
                    nid = n.get("id", "")
                    if role != "pure" and nid and nid not in all_conn:
                        issues.append({"check": "C2", "severity": "warn",
                            "message": f"Orphan node: '{n.get('t', nid)}' not in any edge",
                            "node_id": nid, "graph": flow.get("name", "")})
                # C3
                entry_ids = {n["id"] for n in nodes if n.get("role", n.get("r", "")) == "entry" and "id" in n}
                reachable = set(entry_ids)
                q = _deque(entry_ids)
                while q:
                    cur = q.popleft()
                    for e in exec_edges + data_edges:
                        if e.get("fn") == cur:
                            nxt = e.get("tn")
                            if nxt and nxt not in reachable:
                                reachable.add(nxt)
                                q.append(nxt)
                for n in nodes:
                    nid = n.get("id", "")
                    role = n.get("role", n.get("r", ""))
                    if role != "pure" and nid and nid not in reachable:
                        issues.append({"check": "C3", "severity": "warn",
                            "message": f"Unreachable: '{n.get('t', nid)}'",
                            "node_id": nid, "graph": flow.get("name", "")})
        checks_run = ["C1", "C5"] + (["C2", "C3"] if mode == "full" else [])
        return {"issues": issues, "metrics": metrics,
                "summary": {"total_issues": len(issues), "checks_run": checks_run}}

    # describe_blueprint
    raw_desc = describe_blueprint(
        blueprint_path=blueprint_path,
        graph_name=graph_name,
        max_depth="standard",
        include_pseudocode=False,
        compact=False,
        force_refresh=True,
    )
    parsed = json.loads(raw_desc) if isinstance(raw_desc, str) else raw_desc
    if "cache_hit" in parsed:
        # Force fresh on cache hit
        raw_desc = describe_blueprint(
            blueprint_path=blueprint_path, graph_name=graph_name,
            max_depth="standard", include_pseudocode=False,
            compact=False, force_refresh=True,
        )
        parsed = json.loads(raw_desc) if isinstance(raw_desc, str) else raw_desc

    bp = parsed.get("blueprint", parsed)

    # Normalize multi-graph symbol_table
    if "graphs" in bp and "subgraphs" not in bp:
        merged_vars: dict = {}
        total_nodes = 0
        for g in bp.get("graphs", []):
            sym = g.get("symbol_table", {})
            raw_vars = sym.get("variables", {})
            if isinstance(raw_vars, dict):
                merged_vars.update(raw_vars)
            elif isinstance(raw_vars, list):
                for v in raw_vars:
                    if isinstance(v, dict) and "name" in v:
                        merged_vars[v["name"]] = v
            total_nodes += g.get("metadata", {}).get("node_count", 0)
        bp["symbol_table"] = {"variables": merged_vars}
        bp["metadata"] = {"node_count": total_nodes}
    else:
        sym = bp.get("symbol_table", {})
        raw_vars = sym.get("variables", {})
        if isinstance(raw_vars, list):
            sym["variables"] = {v["name"]: v for v in raw_vars if isinstance(v, dict) and "name" in v}
            bp["symbol_table"] = sym

    # compile — use compile_blueprint_with_errors so C6 gets structured errors array
    compile_resp = json.loads(compile_blueprint_with_errors(blueprint_path))

    report = _qa_analyze(bp, compile_resp, mode=mode, blueprint_path=blueprint_path)
    return json.dumps(report, indent=2, ensure_ascii=False)


# ---------------------------------------------------------------------------
# P2: High-Level Structural Editing Tools
# These tools wrap apply_blueprint_patch for the most common structural edits,
# so the model doesn't need to construct patch JSON manually.
# ---------------------------------------------------------------------------

@exposed_tool(group="core")
def suggest_best_edit_tool(
    intent: str,
    context: str = "",
) -> str:
    """
    Recommend the best Blueprint editing tool for a given intent.

    Use this BEFORE reaching for apply_blueprint_patch directly.
    Describes which high-level tool to use for common structural edits.

    Args:
        intent:  Natural language description of what you want to do.
                 Examples: "insert a Delay between BeginPlay and PrintString",
                           "add a Branch to check a condition before running logic",
                           "append SetActorLocation after the movement call"
        context: Optional JSON string with extra context (blueprint_path, etc.)

    Returns:
        JSON with recommended_tool, usage_hint, and example_call.
    """
    intent_lower = intent.lower()

    # Pattern matching for the 3 high-layer structural tools
    if any(w in intent_lower for w in ("insert", "between", "middle", "before", "after existing")):
        return json.dumps({
            "recommended_tool": "insert_exec_node_between",
            "confidence": "high",
            "usage_hint": "Use when you want to insert a node INTO an existing exec connection. "
                          "Provide from_node_guid, from_pin_name, to_node_guid, to_pin_name, new_node_type.",
            "example_call": {
                "tool": "insert_exec_node_between",
                "blueprint_path": "/Game/Blueprints/BP_Example",
                "graph_name": "EventGraph",
                "from_node_guid": "<BeginPlay GUID>",
                "from_pin_name": "then",
                "to_node_guid": "<PrintString GUID>",
                "to_pin_name": "execute",
                "new_node_type": "KismetSystemLibrary.Delay",
                "new_node_params": {"Duration": 1.0},
            },
            "avoid": "Do NOT use apply_blueprint_patch with manual remove+add_connections for this — use this tool instead.",
        }, indent=2, ensure_ascii=False)

    if any(w in intent_lower for w in ("append", "after", "chain", "add after", "attach", "follow")):
        return json.dumps({
            "recommended_tool": "append_node_after_exec",
            "confidence": "high",
            "usage_hint": "Use when you want to add a node at the END of an exec chain (or insert preserving downstream). "
                          "Handles both chain-tail and chain-middle cases automatically.",
            "example_call": {
                "tool": "append_node_after_exec",
                "blueprint_path": "/Game/Blueprints/BP_Example",
                "graph_name": "EventGraph",
                "after_node_guid": "<source node GUID>",
                "after_pin_name": "then",
                "new_node_type": "Actor.K2_SetActorLocation",
            },
            "avoid": "Do NOT manually wire remove+add_connections in apply_blueprint_patch for this.",
        }, indent=2, ensure_ascii=False)

    if any(w in intent_lower for w in ("branch", "condition", "if", "wrap", "gate", "check", "bool")):
        return json.dumps({
            "recommended_tool": "wrap_exec_chain_with_branch",
            "confidence": "high",
            "usage_hint": "Use when you want to gate an exec chain behind a condition. "
                          "Auto-creates Branch node + optional VariableGet. "
                          "Pass condition_variable=None to auto-create a new bool variable.",
            "example_call": {
                "tool": "wrap_exec_chain_with_branch",
                "blueprint_path": "/Game/Blueprints/BP_Example",
                "graph_name": "EventGraph",
                "entry_from_node_guid": "<upstream node GUID>",
                "entry_from_pin": "then",
                "condition_variable": "bShouldRun",
            },
            "avoid": "Do NOT manually create K2Node_IfThenElse and wire it — use this tool instead.",
        }, indent=2, ensure_ascii=False)

    # Generic patch fallback
    if any(w in intent_lower for w in ("pin", "value", "default", "set pin", "property")):
        return json.dumps({
            "recommended_tool": "apply_blueprint_patch",
            "confidence": "medium",
            "usage_hint": "For setting pin default values, use apply_blueprint_patch with set_pin_values. "
                          "Always run with dry_run=True first.",
            "example_call": {
                "tool": "apply_blueprint_patch",
                "blueprint_path": "/Game/Blueprints/BP_Example",
                "function_id": "EventGraph",
                "patch_json": '{"set_pin_values": [{"node": "<GUID>", "pin": "Duration", "value": "2.0"}]}',
                "dry_run": True,
            },
        }, indent=2, ensure_ascii=False)

    # Fallback: recommend investigation first
    return json.dumps({
        "recommended_tool": "describe_blueprint",
        "confidence": "low",
        "usage_hint": "Intent is ambiguous. Start by describing the current graph state to understand GUIDs and connections, "
                      "then call suggest_best_edit_tool again with more specific intent.",
        "example_call": {
            "tool": "describe_blueprint",
            "blueprint_path": "/Game/Blueprints/BP_Example",
            "max_depth": "pseudocode",
        },
        "alternatives": [
            "insert_exec_node_between — insert a node in an exec chain",
            "append_node_after_exec — append a node after an exec node",
            "wrap_exec_chain_with_branch — wrap an exec chain with Branch",
            "apply_blueprint_patch — low-level batch operations",
        ],
    }, indent=2, ensure_ascii=False)


@exposed_tool(group="core")
def insert_exec_node_between(
    blueprint_path: str,
    graph_name: str,
    from_node_guid: str,
    from_pin_name: str,
    to_node_guid: str,
    to_pin_name: str,
    new_node_type: str,
    new_node_params: dict = None,
) -> str:
    """
    Insert a new node between two already-connected exec pins.

    Automatically: removes the existing connection, creates the new node,
    wires from_node.from_pin → new_node.execute, new_node.then → to_node.to_pin.
    Runs dry_run preflight before applying.

    Args:
        blueprint_path: e.g. "/Game/Blueprints/BP_NPC"
        graph_name:     "EventGraph" or function GUID
        from_node_guid: GUID of the upstream node
        from_pin_name:  exec output pin name on upstream node (usually "then")
        to_node_guid:   GUID of the downstream node
        to_pin_name:    exec input pin name on downstream node (usually "execute")
        new_node_type:  node type to insert (e.g. "KismetSystemLibrary.Delay")
        new_node_params: optional dict of pin_values for the new node

    Returns:
        JSON with success, created_nodes, and post_failure_state on error.
    """
    import uuid as _uuid

    # 1. Verify the original connection exists
    from_details = json.loads(get_node_details(blueprint_path, graph_name, from_node_guid))
    if not from_details.get("success", True) is not False:
        # get_node_details returns the node object directly on success
        pass

    ref_id = f"inserted_{_uuid.uuid4().hex[:6]}"
    patch = {
        "remove_connections": [
            {"from": f"{from_node_guid}.{from_pin_name}", "to": f"{to_node_guid}.{to_pin_name}"}
        ],
        "add_nodes": [
            {
                "ref_id": ref_id,
                "node_type": new_node_type,
                "pin_values": new_node_params or {},
            }
        ],
        "add_connections": [
            {"from": f"{from_node_guid}.{from_pin_name}", "to": f"{ref_id}.execute"},
            {"from": f"{ref_id}.then", "to": f"{to_node_guid}.{to_pin_name}"},
        ],
        "auto_compile": True,
    }

    # 2. Dry-run preflight
    dry = json.loads(apply_blueprint_patch(blueprint_path, graph_name, json.dumps(patch), dry_run=True))
    dry_issues = dry.get("issues", [])
    if dry_issues:
        return json.dumps({
            "success": False,
            "error_code": "INSERT_PREFLIGHT_FAILED",
            "preflight_issues": dry_issues,
            "hint": "Fix patch issues before applying",
        }, ensure_ascii=False)

    # 3. Apply
    return apply_blueprint_patch(blueprint_path, graph_name, json.dumps(patch))


@exposed_tool(group="core")
def append_node_after_exec(
    blueprint_path: str,
    graph_name: str,
    after_node_guid: str,
    after_pin_name: str = "then",
    new_node_type: str = "",
    new_node_params: dict = None,
) -> str:
    """
    Append a new node after an existing exec node's output pin.

    If the pin already has a connection, inserts between (preserving downstream).
    If the pin has no connection, simply connects the new node after.

    Args:
        blueprint_path:  e.g. "/Game/Blueprints/BP_NPC"
        graph_name:      "EventGraph" or function GUID
        after_node_guid: GUID of the node to append after
        after_pin_name:  exec output pin (default "then")
        new_node_type:   node type to append
        new_node_params: optional pin_values dict

    Returns:
        JSON with success and created_nodes.
    """
    import uuid as _uuid

    ref_id = f"appended_{_uuid.uuid4().hex[:6]}"

    # Check if after_pin already has a downstream connection
    details_raw = get_node_details(blueprint_path, graph_name, after_node_guid)
    try:
        details = json.loads(details_raw)
    except Exception:
        details = {}

    existing_downstream = None
    existing_downstream_pin = "execute"
    for pin in details.get("pins", []):
        if pin.get("name") == after_pin_name and pin.get("direction") == "output":
            conns = pin.get("connected_to", [])
            if conns:
                existing_downstream = conns[0].get("node_guid")
                existing_downstream_pin = conns[0].get("pin_name", "execute")
            break

    patch: dict = {
        "add_nodes": [
            {
                "ref_id": ref_id,
                "node_type": new_node_type,
                "pin_values": new_node_params or {},
            }
        ],
        "add_connections": [],
        "auto_compile": True,
    }

    if existing_downstream:
        # Insert between: remove old, wire through new node
        patch["remove_connections"] = [
            {"from": f"{after_node_guid}.{after_pin_name}",
             "to": f"{existing_downstream}.{existing_downstream_pin}"}
        ]
        patch["add_connections"] = [
            {"from": f"{after_node_guid}.{after_pin_name}", "to": f"{ref_id}.execute"},
            {"from": f"{ref_id}.then", "to": f"{existing_downstream}.{existing_downstream_pin}"},
        ]
    else:
        # Simple append at chain tail
        patch["add_connections"] = [
            {"from": f"{after_node_guid}.{after_pin_name}", "to": f"{ref_id}.execute"},
        ]

    return apply_blueprint_patch(blueprint_path, graph_name, json.dumps(patch))


@exposed_tool(group="core")
def wrap_exec_chain_with_branch(
    blueprint_path: str,
    graph_name: str,
    entry_from_node_guid: str,
    entry_from_pin: str,
    condition_variable: str = None,
    true_chain_node_guid: str = None,
    false_chain_node_guid: str = None,
) -> str:
    """
    Wrap an existing execution chain with a Branch node.

    Creates a Branch node at the entry point. Connects condition_variable
    (or creates a new bool variable if None) to the Condition pin.
    The true pin connects to true_chain_node_guid (or the original downstream if None).
    The false pin connects to false_chain_node_guid if provided.

    Args:
        blueprint_path:         e.g. "/Game/Blueprints/BP_NPC"
        graph_name:             "EventGraph" or function GUID
        entry_from_node_guid:   GUID of the node whose exec output gets the Branch inserted
        entry_from_pin:         exec output pin name (usually "then")
        condition_variable:     Name of existing bool variable; None = auto-create
        true_chain_node_guid:   Node to connect to Branch's True pin; None = original downstream
        false_chain_node_guid:  Node to connect to Branch's False pin; None = leave unconnected

    Returns:
        JSON with success, created_nodes (branch + optional variable).
    """
    import uuid as _uuid

    branch_ref = f"branch_{_uuid.uuid4().hex[:6]}"

    # Find existing downstream for true chain default
    details_raw = get_node_details(blueprint_path, graph_name, entry_from_node_guid)
    try:
        details = json.loads(details_raw)
    except Exception:
        details = {}

    original_downstream = None
    original_downstream_pin = "execute"
    for pin in details.get("pins", []):
        if pin.get("name") == entry_from_pin and pin.get("direction") == "output":
            conns = pin.get("connected_to", [])
            if conns:
                original_downstream = conns[0].get("node_guid")
                original_downstream_pin = conns[0].get("pin_name", "execute")
            break

    true_target = true_chain_node_guid or original_downstream
    true_target_pin = original_downstream_pin if not true_chain_node_guid else "execute"

    patch: dict = {
        "add_nodes": [
            {
                "ref_id": branch_ref,
                "node_type": "K2Node_IfThenElse",
                "pin_values": {},
            }
        ],
        "remove_connections": [],
        "add_connections": [
            {"from": f"{entry_from_node_guid}.{entry_from_pin}", "to": f"{branch_ref}.execute"},
        ],
        "auto_compile": True,
    }

    # Remove original connection if we have one
    if original_downstream:
        patch["remove_connections"].append({
            "from": f"{entry_from_node_guid}.{entry_from_pin}",
            "to": f"{original_downstream}.{original_downstream_pin}",
        })

    # Connect true branch
    if true_target:
        patch["add_connections"].append(
            {"from": f"{branch_ref}.then", "to": f"{true_target}.{true_target_pin}"}
        )

    # Connect false branch
    if false_chain_node_guid:
        patch["add_connections"].append(
            {"from": f"{branch_ref}.else", "to": f"{false_chain_node_guid}.execute"}
        )

    # Handle condition variable
    if condition_variable:
        var_ref = f"cond_get_{_uuid.uuid4().hex[:6]}"
        patch["add_nodes"].append({
            "ref_id": var_ref,
            "node_type": "K2Node_VariableGet",
            "pin_values": {"variable_name": condition_variable},
        })
        patch["add_connections"].append(
            {"from": f"{var_ref}.ReturnValue", "to": f"{branch_ref}.Condition"}
        )
    # else: leave Condition pin unconnected (defaults to false) — caller can wire it separately

    return apply_blueprint_patch(blueprint_path, graph_name, json.dumps(patch))


@exposed_tool(group="analysis")
def get_trace_analytics(
    date_from: str = "",
    date_to: str = "",
    blueprint_path: str = "",
    limit: int = 500,
    output_format: str = "json",
) -> str:
    """
    Aggregate statistics from ~/.unrealgenai/traces/ patch execution logs.

    Analyzes trace files written by apply_blueprint_patch to surface failure patterns,
    top error codes, high-failure blueprints, and recommended actions.

    Args:
        date_from:      Start date YYYY-MM-DD (default: today)
        date_to:        End date YYYY-MM-DD inclusive (default: today)
        blueprint_path: Optional filter — only traces for this blueprint
        limit:          Max trace files to scan (default 500)
        output_format:  "json" (default) or "markdown" for human/model reading

    Returns:
        JSON or markdown summary with:
          total_runs, success_count, failure_count, rollback_partial_count,
          ghost_node_count, timeout_count, top_error_codes,
          top_node_types_in_failures, top_blueprints_by_failure,
          top_recommended_next_actions, date_range, scanned_files
    """
    from pathlib import Path
    from collections import Counter
    import glob

    trace_base = Path.home() / ".unrealgenai" / "traces"

    # Determine date range
    today = time.strftime("%Y-%m-%d")
    d_from = date_from.strip() or today
    d_to = date_to.strip() or today

    # Collect matching trace files
    all_files = []
    try:
        for day_dir in sorted(trace_base.iterdir()):
            if not day_dir.is_dir():
                continue
            day_str = day_dir.name
            if day_str < d_from or day_str > d_to:
                continue
            for tf in day_dir.glob("*.json"):
                all_files.append(tf)
    except FileNotFoundError:
        return json.dumps({
            "success": True,
            "total_runs": 0,
            "message": f"No trace directory at {trace_base}. Run apply_blueprint_patch first.",
            "date_range": f"{d_from} → {d_to}",
        }, ensure_ascii=False)

    all_files = all_files[:limit]

    # Parse trace files
    total_runs = 0
    success_count = 0
    failure_count = 0
    rollback_partial = 0
    ghost_node_count = 0
    timeout_count = 0
    error_codes: Counter = Counter()
    node_types_in_failures: Counter = Counter()
    blueprint_failures: Counter = Counter()
    next_actions: Counter = Counter()

    for tf in all_files:
        try:
            data = json.loads(tf.read_text(encoding="utf-8"))
        except Exception:
            continue

        bp = data.get("blueprint_path", "")
        if blueprint_path and blueprint_path not in bp:
            continue

        total_runs += 1
        rs = data.get("results_summary", {})
        pl = data.get("patch_log", {})

        if rs.get("success"):
            success_count += 1
        else:
            failure_count += 1
            blueprint_failures[bp or "unknown"] += 1

        if rs.get("rollback_status") == "partial":
            rollback_partial += 1

        # Ghost nodes
        pfs = data.get("post_failure_state") or {}
        ghost_node_count += len(pfs.get("residual_nodes", []))

        # Error codes
        for cat in pl.get("error_categories", []):
            error_codes[cat] += 1

        # Detect timeouts
        if "TRANSPORT_TIMEOUT" in pl.get("error_categories", []):
            timeout_count += 1

        # Node types from diagnostics
        for diag in data.get("diagnostics", []):
            code = diag.get("code", "")
            if code:
                error_codes[code] += 1

        # Recommended next actions
        for action in (rs.get("recommended_next_actions") or []):
            next_actions[action] += 1

        # Extract node types from patch input (best-effort)
        try:
            patch_input = data.get("patch_log", {}).get("patch_input") or ""
            if isinstance(patch_input, str) and patch_input:
                pi = json.loads(patch_input)
            elif isinstance(patch_input, dict):
                pi = patch_input
            else:
                pi = {}
            if not rs.get("success"):
                for node in pi.get("add_nodes", []):
                    nt = node.get("node_type", "")
                    if nt:
                        node_types_in_failures[nt.split(".")[-1]] += 1
        except Exception:
            pass

    result = {
        "success": True,
        "date_range": f"{d_from} → {d_to}",
        "scanned_files": len(all_files),
        "total_runs": total_runs,
        "success_count": success_count,
        "failure_count": failure_count,
        "success_rate": f"{100 * success_count // total_runs}%" if total_runs else "N/A",
        "rollback_partial_count": rollback_partial,
        "ghost_node_count": ghost_node_count,
        "timeout_count": timeout_count,
        "top_error_codes": dict(error_codes.most_common(10)),
        "top_node_types_in_failures": dict(node_types_in_failures.most_common(10)),
        "top_blueprints_by_failure": dict(blueprint_failures.most_common(10)),
        "top_recommended_next_actions": dict(next_actions.most_common(10)),
    }

    if output_format == "markdown":
        lines = [
            f"# Trace Analytics: {d_from} → {d_to}",
            "",
            f"**Scanned**: {len(all_files)} files | **Total runs**: {total_runs}",
            f"**Success**: {success_count} ({result['success_rate']}) | **Failures**: {failure_count}",
            f"**Rollback partial**: {rollback_partial} | **Ghost nodes**: {ghost_node_count} | **Timeouts**: {timeout_count}",
            "",
            "## Top Error Codes",
            *[f"- `{k}`: {v}" for k, v in error_codes.most_common(10)],
            "",
            "## Top Node Types in Failures",
            *[f"- `{k}`: {v}" for k, v in node_types_in_failures.most_common(8)],
            "",
            "## Top Blueprints by Failure Count",
            *[f"- `{k}`: {v}" for k, v in blueprint_failures.most_common(8)],
            "",
            "## Top Recommended Next Actions",
            *[f"- `{k}`: {v}" for k, v in next_actions.most_common(8)],
        ]
        return "\n".join(lines)

    return json.dumps(result, indent=2, ensure_ascii=False)


@exposed_tool(group="core")
def get_runtime_status() -> str:
    """
    Check the health and status of the UE Editor socket runtime.

    Returns listener health, queue length, transport protocol details, and last error.
    Use this FIRST to diagnose connectivity issues before sending Blueprint commands.

    Returns:
        JSON with:
          listener_alive           bool — whether UE socket server responded
          queue_length             int  — current pending command count (-1 if unknown)
          protocol_version_requested str — what Python side is configured to use (v1/v2)
          protocol_version_active  str  — what UE side last negotiated (v1/v2)
          framing_mode             str  — "length_prefixed" (v2) or "json_parse" (v1)
          last_transport_error     str  — last error code from transport layer
          error_code               str  — present only if listener is down
    """
    import os as _os

    protocol_requested = _os.environ.get("SOCKET_PROTOCOL_VERSION", "v1")

    resp = send_to_unreal({"type": "handshake", "message": "runtime_status_ping"})
    alive = isinstance(resp, dict) and resp.get("success", False)

    # Import active protocol state from socket server module (if running in UE)
    protocol_active = protocol_requested  # default: assume matches requested
    last_transport_error = ""
    try:
        import unreal_socket_server as _uss
        protocol_active = getattr(_uss, "_active_protocol", protocol_requested)
        last_transport_error = getattr(_uss, "_last_transport_error", "")
    except ImportError:
        pass  # running outside UE — fallback to requested

    # Try to get queue length from UE side
    queue_resp = send_to_unreal({"type": "get_queue_length"})
    queue_len = queue_resp.get("queue_length", -1) if isinstance(queue_resp, dict) else -1

    framing_mode = "length_prefixed" if protocol_active == "v2" else "json_parse"

    result = {
        "listener_alive": alive,
        "queue_length": queue_len,
        "protocol_version_requested": protocol_requested,
        "protocol_version_active": protocol_active,
        "framing_mode": framing_mode,
        "last_transport_error": last_transport_error,
    }
    if not alive:
        result["error_code"] = "TRANSPORT_DISCONNECTED"
        result["hint"] = "UE Editor may not be running or socket server not started. Check auto_start_socket_server in plugin settings."

    return json.dumps(result, ensure_ascii=False)


@exposed_tool(group="core")
def get_blueprint_working_set(
    blueprint_path: str,
    graph_name: str = "EventGraph",
    target_node_guid: str = "",
    task_type: str = "",
) -> str:
    """
    Return a single bundled context package for Blueprint editing tasks.

    Replaces 5-7 separate MCP calls (runtime_status, list_graphs, describe,
    variables, compile errors, trace, suggest_tool) with one call. Use this
    at the start of a planning or editing session instead of calling each
    tool individually.

    Args:
        blueprint_path: Asset path, e.g. "/Game/Blueprints/BP_NPC"
        graph_name: Graph to describe (default "EventGraph")
        target_node_guid: Optional node GUID to include neighborhood details for
        task_type: Optional task classification hint (e.g. "structure_edit",
                   "repair_existing_flow") — calls suggest_best_edit_tool internally

    Returns:
        JSON bundle with: runtime_status, graph_guids, description (pseudocode
        compact), variables, recommended_tools (if task_type given),
        recent_compile_errors, risk_flags
    """
    bundle: dict = {}

    # 1. Runtime status
    try:
        bundle["runtime_status"] = json.loads(get_runtime_status())
    except Exception as e:
        bundle["runtime_status"] = {"error": str(e)}

    # 2. Graph GUIDs
    try:
        bundle["graph_guids"] = json.loads(list_blueprint_graphs(blueprint_path))
    except Exception as e:
        bundle["graph_guids"] = {"error": str(e)}

    # 3. Compact pseudocode description of target graph
    try:
        bundle["description"] = json.loads(
            describe_blueprint(
                blueprint_path=blueprint_path,
                graph_name=graph_name,
                max_depth="pseudocode",
                compact=True,
                include_pseudocode=True,
            )
        )
    except Exception as e:
        bundle["description"] = {"error": str(e)}

    # 4. Variables
    try:
        bundle["variables"] = json.loads(get_blueprint_variables(blueprint_path))
    except Exception as e:
        bundle["variables"] = {"error": str(e)}

    # 5. Node neighborhood (optional)
    if target_node_guid:
        try:
            bundle["node_neighborhood"] = json.loads(
                get_node_details(blueprint_path, graph_name, target_node_guid)
            )
        except Exception as e:
            bundle["node_neighborhood"] = {"error": str(e)}

    # 6. Recommended tools (optional)
    if task_type:
        try:
            bundle["recommended_tools"] = json.loads(
                suggest_best_edit_tool(intent=task_type)
            )
        except Exception as e:
            bundle["recommended_tools"] = {"error": str(e)}

    # 7. Recent compile errors
    try:
        compile_result = json.loads(compile_blueprint_with_errors(blueprint_path))
        bundle["recent_compile_errors"] = {
            "compiled": compile_result.get("compiled", False),
            "errors": compile_result.get("errors", []),
            "structured_errors": compile_result.get("structured_errors", []),
        }
    except Exception as e:
        bundle["recent_compile_errors"] = {"error": str(e)}

    # 8. Risk flags — restricted node types in the graph
    try:
        from node_lifecycle_registry import is_restricted
        risk_flags = []
        desc = bundle.get("description", {})
        for sg in desc.get("subgraphs", []):
            for node in sg.get("nodes", []):
                nt = node.get("node_type", "")
                if nt and is_restricted(nt):
                    risk_flags.append({
                        "node_guid": node.get("id", ""),
                        "node_type": nt,
                        "title": node.get("title", ""),
                    })
        bundle["risk_flags"] = risk_flags
    except Exception as e:
        bundle["risk_flags"] = {"error": str(e)}

    return json.dumps(bundle, ensure_ascii=False)


@exposed_tool(group="core")
def validate_patch_intent(
    blueprint_path: str,
    function_id: str = "EventGraph",
    patch_json: str = "{}",
) -> str:
    """Validate that graph state still matches a patch's assumptions before applying.

    Runs two checks without executing any mutations:
    1. Fingerprint freshness: compares current node-GUID sha256 hash against
       the hash stored at last describe/read time. Detects graph changes since
       the patch was planned.
    2. Ref resolvability: verifies that every node reference in add_connections
       endpoints is either a GUID, an fname: ref, or a ref_id declared in
       add_nodes. Title-based refs that cannot be resolved at preflight are
       flagged.

    Use this before apply_blueprint_patch for L2/L3 (structural/dangerous)
    patches to catch stale-context failures early, before the RBW guard fires.

    Args:
        blueprint_path: Asset path, e.g. "/Game/Blueprints/BP_NPC"
        function_id: Graph name or GUID (default "EventGraph")
        patch_json: The patch JSON string to validate

    Returns:
        JSON with: valid (bool), stale_fingerprint (bool),
                   unresolvable_refs (list), warnings (list)
    """
    from tools.patch import validate_patch_intent as _validate_patch_intent
    result = _validate_patch_intent(blueprint_path, function_id, patch_json, send_to_unreal)
    return json.dumps(result, ensure_ascii=False)


if __name__ == "__main__":
    import traceback

    # Print active tool profile summary at startup
    _raw_profile = os.getenv("MCP_TOOL_PROFILE", _DEFAULT_PROFILE)
    print(f"[MCP] Profile: {_raw_profile}", file=sys.stderr)
    print(f"[MCP] Active groups: {sorted(_ACTIVE_GROUPS)}", file=sys.stderr)
    print(f"[MCP] Registered tools ({len(_REGISTERED_TOOLS)}): {sorted(_REGISTERED_TOOLS)}", file=sys.stderr)

    try:
        print("Server starting...", file=sys.stderr)
        mcp.run()
    except Exception as e:
        print(f"Server crashed with error: {e}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        raise
