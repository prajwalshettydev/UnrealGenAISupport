import socket
import json
import sys
import os
import time
from collections import OrderedDict
from fastmcp import FastMCP
from fastmcp.utilities.types import Image
import re
import mss
import base64
import tempfile # For creating a secure temporary file
from io import BytesIO
from constants import DESTRUCTIVE_SCRIPT_PATTERNS, DESTRUCTIVE_COMMAND_KEYWORDS
from layout_engine import (
    build_occupancy as _build_occupancy,
    find_free_slot as _find_free_slot,
    classify_node_role as _classify_node_role,
    extract_layout_intent as _extract_layout_intent,
    AUTO_PAD_X, AUTO_PAD_Y,
)
from pathlib import Path


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


# Function to send a message to Unreal Engine via socket
def send_to_unreal(command):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            unreal_port = int(os.environ.get("UNREAL_PORT", "9877"))
            s.connect(('localhost', unreal_port))

            # Ensure proper JSON encoding
            json_str = json.dumps(command)
            s.sendall(json_str.encode('utf-8'))

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
                    json.loads(response_data.decode('utf-8'))
                    # If we get here, we have valid JSON
                    break
                except json.JSONDecodeError:
                    # Need more data, continue receiving
                    continue

            # Parse the complete response
            if response_data:
                return json.loads(response_data.decode('utf-8'))
            else:
                return {"success": False, "error": "No response received"}

        except Exception as e:
            print(f"Error sending to Unreal: {e}", file=sys.stderr)
            return {"success": False, "error": str(e)}


# ---------------------------------------------------------------------------
# Token optimization: compact JSON output helpers
# ---------------------------------------------------------------------------

def _strip_empty(obj):
    """Recursively remove keys with None, empty string, empty list, or empty dict values.

    Preserves False and 0 — only removes structurally absent values.
    """
    if isinstance(obj, dict):
        return {k: _strip_empty(v) for k, v in obj.items()
                if v is not None and not (isinstance(v, str) and v == "")
                and not (isinstance(v, (list, dict)) and len(v) == 0)}
    elif isinstance(obj, list):
        return [_strip_empty(item) for item in obj]
    return obj


_COMPACT_KEY_MAP = {
    "title": "t",
    "node_type": "nt",
    "role": "r",
    "tags": "tg",
    "semantic": "sem",
    "execution_semantics": "xs",
    "exec_edges": "ex",
    "data_edges": "da",
    "from_node": "fn",
    "to_node": "tn",
    "from_pin": "fp",
    "to_pin": "tp",
    "edge_type": "et",
    "data_type": "dt",
    "condition": "cond",
    "pseudocode": "pc",
    "execution_trace": "tr",
    "subgraph_id": "sg_id",
    "entry_nodes": "entry",
    "summary": "sum",
    "symbol_table": "sym",
    "data_expressions": "dex",
    "blueprint_id": "bp_id",
    "blueprint_name": "bp_name",
    "node_count": "nc",
    "default_value": "dv",
    "direction": "dir",
    "position": "pos",
}


def _compact_keys(obj):
    """Recursively rename keys using _COMPACT_KEY_MAP."""
    if isinstance(obj, dict):
        return {_COMPACT_KEY_MAP.get(k, k): _compact_keys(v) for k, v in obj.items()}
    elif isinstance(obj, list):
        return [_compact_keys(item) for item in obj]
    return obj


# --- bp_json_v2 semantic compact: readable short keys for LLM consumption ---
_SEMANTIC_KEYS = {
    # node object
    "node_guid": "guid", "node_type": "type", "node_title": "title",
    "canonical_id": "cid", "instance_id": "inst", "position": "pos",
    # pin object
    "direction": "dir", "default_value": "val", "default_object": "ref",
    "default_class": "cls", "is_connected": None,  # delete entirely
    "connected_to": "links", "required": "req",
    "pin_name": "pin",  # inside links array
    # search candidate
    "canonical_name": "cname", "display_name": "label", "node_kind": "kind",
    "spawn_strategy": "spawn", "category": "cat", "keywords": "kw",
    "relevance_score": "score", "is_latent": "latent", "is_pure": "pure",
    # inspect
    "patch_hint": "hint", "context_requirements": "ctx",
    "needs_world_context": "req_world", "needs_latent_support": "req_latent",
    # preflight
    "predicted_nodes": "predicted", "resolved_type": "resolved",
    "severity": "sev", "message": "msg", "suggestion": "fix", "index": "idx",
}
_DIR_VALUES = {"input": "in", "output": "out"}


def _to_semantic(obj):
    """Recursively apply semantic key compression (bp_json_v2)."""
    if isinstance(obj, dict):
        result = {}
        for k, v in obj.items():
            new_key = _SEMANTIC_KEYS.get(k, k)
            if new_key is None:  # marked for deletion (is_connected)
                continue
            new_val = _to_semantic(v)
            if new_key == "dir" and isinstance(new_val, str):
                new_val = _DIR_VALUES.get(new_val, new_val)
            # pin "type" → "ptype" (only inside pin objects that have "name" key)
            if k == "type" and "name" in obj:
                new_key = "ptype"
            result[new_key] = new_val
        return result
    if isinstance(obj, list):
        return [_to_semantic(i) for i in obj]
    return obj


def _compact_response(obj):
    """Strip empties + apply semantic keys.

    If the response contains the sentinel key `_semantic: true` (injected by C++ when
    schema_mode=semantic is active), skip _to_semantic transformation — C++ already
    output bp_json_v2 keys directly. The sentinel is stripped before returning.

    Falls back to heuristic detection (presence of 'guid'/'cname' at top level) for
    responses that predate the sentinel marker.

    NOTE: `_semantic` is an internal marker only. It is always consumed (popped) here
    and NEVER appears in the final MCP response returned to the caller.
    """
    if isinstance(obj, dict):
        has_sentinel = obj.pop("_semantic", False)
        has_heuristic = "guid" in obj or "cname" in obj
        if has_sentinel or has_heuristic:
            return _strip_empty(obj)  # already semantic — only strip empties
    return _to_semantic(_strip_empty(obj))


_VALID_SCHEMA_MODES = {"semantic", "verbose"}


def _normalize_schema_mode(mode: str) -> str:
    """Normalize and validate schema_mode. Raises ValueError on invalid input."""
    normalized = mode.lower().strip()
    if normalized not in _VALID_SCHEMA_MODES:
        raise ValueError(f"Invalid schema_mode: {mode!r}. Must be one of: {_VALID_SCHEMA_MODES}")
    return normalized


def _make_envelope(tool_name: str, ok: bool, result: dict,
                   summary: str = "", warnings: list | None = None,
                   diagnostics: list | None = None,
                   job_id: str | None = None) -> dict:
    """Build the unified response envelope for MCP tool responses.

    {ok, request_id, tool, summary, warnings, diagnostics, result, job_id}
    """
    import uuid
    return {
        "ok": ok,
        "request_id": str(uuid.uuid4())[:8],
        "tool": tool_name,
        "summary": summary or ("OK" if ok else result.get("error", "failed")),
        "warnings": warnings or [],
        "diagnostics": diagnostics or [],
        "result": result,
        "job_id": job_id,
    }


def _json_out(obj, compact=False):
    """Serialize to JSON, optionally compact (short keys, no indent, no empties)."""
    if compact:
        obj = _strip_empty(obj)
        obj = _compact_keys(obj)
        return json.dumps(obj, separators=(",", ":"), ensure_ascii=False)
    return json.dumps(obj, indent=2, ensure_ascii=False)


# ---------------------------------------------------------------------------
# Token optimization: describe_blueprint response caching (LRU, fingerprint-based)
# ---------------------------------------------------------------------------

_DESCRIBE_CACHE_MAX = 20
_DESCRIBE_CACHE_TTL = 120  # seconds


class _DescribeCache:
    """LRU cache for describe_blueprint results, keyed by (path, graph, depth)."""

    def __init__(self, max_size=_DESCRIBE_CACHE_MAX):
        self._store = OrderedDict()  # key -> {fingerprint, result_json, timestamp}
        self._max = max_size

    def get(self, key, fingerprint):
        """Return cached result_json if fingerprint matches and TTL valid, else None."""
        entry = self._store.get(key)
        if not entry:
            return None
        # Check fingerprint first (cheap string compare) before TTL syscall
        if entry["fingerprint"] != fingerprint:
            del self._store[key]
            return None
        if time.monotonic() - entry["timestamp"] > _DESCRIBE_CACHE_TTL:
            del self._store[key]
            return None
        self._store.move_to_end(key)
        return entry["result_json"]

    def put(self, key, fingerprint, result_json):
        if key in self._store:
            self._store.move_to_end(key)
        self._store[key] = {
            "fingerprint": fingerprint,
            "result_json": result_json,
            "timestamp": time.monotonic(),
        }
        while len(self._store) > self._max:
            self._store.popitem(last=False)

    def invalidate(self, blueprint_path):
        """Remove all entries for a given blueprint path."""
        keys_to_del = [k for k in self._store if k[0] == blueprint_path]
        for k in keys_to_del:
            del self._store[k]


_describe_cache = _DescribeCache()
_describe_legend_seen = set()  # non-empty = legend already sent this MCP session
_node_details_cache = {}  # (bp_path, func_id, guid) → details dict, max 100 entries
_NODE_DETAILS_CACHE_MAX = 100


def _postprocess_describe(result_json, compact, max_depth, blueprint_path):
    """Post-process describe output: strip legend on repeat, strip redundant graph fields.
    Operates on a parsed copy — never mutates cached data."""
    import copy as _copy
    try:
        result = json.loads(result_json)
    except (json.JSONDecodeError, TypeError):
        return result_json

    result = _copy.deepcopy(result)
    bp_obj = result.get("blueprint", result)

    # Strip graph-level bp_id/bp_name if same as parent (pseudocode+compact only)
    if compact and max_depth == "pseudocode":
        parent_bp_id = bp_obj.get("blueprint_id", bp_obj.get("bp_id", ""))
        parent_bp_name = bp_obj.get("blueprint_name", bp_obj.get("bp_name", ""))
        for graph in bp_obj.get("graphs", []):
            if graph.get("bp_id") == parent_bp_id:
                graph.pop("bp_id", None)
            if graph.get("bp_name") == parent_bp_name:
                graph.pop("bp_name", None)

    # Strip _legend on subsequent calls (session-level auto-omit)
    if _describe_legend_seen and "_legend" in result:
        del result["_legend"]
    elif "_legend" in result:
        _describe_legend_seen.add(True)

    out_json = json.dumps(result, separators=(",", ":"), ensure_ascii=False) if compact else json.dumps(result, indent=2, ensure_ascii=False)

    # Compression log
    before_len = len(result_json)
    after_len = len(out_json)
    if before_len > 0:
        pct = 100 * (before_len - after_len) // before_len
        print(f"[DESCRIBE_COMPRESS] {blueprint_path} {max_depth} {before_len}→{after_len} (-{pct}%)",
              file=sys.stderr)

    return out_json


def _compute_fingerprint(blueprint_path, graph_name, send_fn=None, graphs=None):
    """Lightweight fingerprint from list_graphs node counts (no heavy node fetch).

    Pass `graphs` (already-fetched list) to avoid an extra socket round-trip.
    Pass `send_fn` to fetch graphs on demand (used for pre-describe cache check).
    """
    if graphs is None:
        if send_fn is None:
            return None
        resp = send_fn({"type": "list_graphs", "blueprint_path": blueprint_path})
        if not resp.get("success"):
            return None
        graphs = resp.get("graphs", [])
    if isinstance(graphs, str):
        graphs = json.loads(graphs)
    if graph_name:
        graphs = [g for g in graphs if g["graph_name"] == graph_name]
    parts = [f"{g.get('graph_name', '?')}:{g.get('node_count', '?')}" for g in graphs]
    return "|".join(sorted(parts))


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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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
        return f"Successfully spawned Blueprint {blueprint_path}" + (
            f" with label '{actor_label}'" if actor_label else "")
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

# @mcp.tool()  # INTERNAL — use higher-level tools instead
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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

@mcp.tool()
def get_node_details(blueprint_path: str, function_id: str, node_guid: str,
                     schema_mode: str = "semantic") -> str:
    """
    Get detailed information about a specific Blueprint node including all its pins,
    connections, default values, and display title.

    Use this BEFORE connecting nodes — you need to know the exact pin names.
    Use this AFTER adding a node — to verify it was created correctly.
    Use this to inspect an existing node you want to modify.

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")
        function_id: Graph identifier — use "EventGraph" for the main event graph,
                     or a function GUID from list_blueprint_graphs for function graphs
        node_guid: The GUID of the node (from get_all_nodes_in_graph or add_node_to_blueprint)
        schema_mode: Output format — "semantic" (default, bp_json_v2, ~40% fewer tokens)
                     or "verbose" (full field names, for debugging)

    Returns:
        JSON with node_guid, node_type, node_title, position, and pins array.
        Each pin has: name, direction (input/output), type, default_value,
        is_connected, and connected_to list [{node_guid, pin_name}].
    """
    schema_mode = _normalize_schema_mode(schema_mode)
    # LRU cache for node details (invalidated alongside _describe_cache)
    cache_key = (blueprint_path, function_id, node_guid, schema_mode)
    if cache_key in _node_details_cache:
        return json.dumps(_node_details_cache[cache_key], indent=2)

    command = {
        "type": "get_node_details",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "node_guid": node_guid,
        "schema_mode": schema_mode,
    }
    response = send_to_unreal(command)
    if response.get("success"):
        details = response.get("details", {})
        if len(_node_details_cache) >= _NODE_DETAILS_CACHE_MAX:
            # Evict oldest entry (FIFO)
            oldest_key = next(iter(_node_details_cache))
            del _node_details_cache[oldest_key]
        _node_details_cache[cache_key] = details
        return json.dumps(_compact_response(details), separators=(",", ":"), ensure_ascii=False)
    else:
        return f"Failed: {response.get('error', 'Unknown error')}"


@mcp.tool()
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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


# @mcp.tool()  # INTERNAL — use higher-level tools instead
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
# @mcp.tool()  # INTERNAL — use higher-level tools instead
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
    return json.dumps(response.get("files", [])) if response.get("success") else f"Failed: {response.get('error')}"


@mcp.tool()
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


@mcp.tool()
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


@mcp.tool()
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


@mcp.tool()
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


@mcp.tool()
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


@mcp.tool()
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
# describe_blueprint v2 — LLM-friendly graph representation
# ---------------------------------------------------------------------------

# Map UE node class names to (abstract_type, base_tags, role)
_NODE_TYPE_MAP = {
    "K2Node_Event":             ("Event",        ["entry"],       "entry"),
    "K2Node_CustomEvent":       ("CustomEvent",  ["entry"],       "entry"),
    "K2Node_InputAction":       ("InputEvent",   ["entry"],       "entry"),
    "K2Node_ComponentBoundEvent": ("ComponentEvent", ["entry"],   "entry"),
    "K2Node_InputKey":          ("InputKey",     ["entry"],       "entry"),
    "K2Node_EnhancedInputAction": ("EnhancedInput", ["entry"],   "entry"),
    "K2Node_AsyncAction":       ("AsyncAction",  ["latent"],      "call"),
    "K2Node_AIMoveTo":          ("AIMoveTo",     ["latent"],      "latent"),
    "K2Node_CallFunction":      ("FunctionCall", [],              "call"),
    "K2Node_CallArrayFunction": ("ArrayOp",      [],              "call"),
    "K2Node_IfThenElse":        ("Branch",       [],              "control"),
    "K2Node_ExecutionSequence": ("Sequence",     [],              "control"),
    "K2Node_ForEachElementInEnum": ("Loop",      [],              "control"),
    "K2Node_ForEachLoop":       ("Loop",         [],              "control"),
    "K2Node_WhileLoop":         ("Loop",         [],              "control"),
    "K2Node_MakeStruct":        ("MakeStruct",   ["struct"],      "pure"),
    "K2Node_BreakStruct":       ("BreakStruct",  ["struct"],      "pure"),
    "K2Node_MakeArray":         ("MakeArray",    [],              "pure"),
    "K2Node_VariableGet":       ("VariableGet",  ["state_read"],  "pure"),
    "K2Node_VariableSet":       ("VariableSet",  ["state_write"], "state"),
    "K2Node_MacroInstance":     ("MacroCall",    [],              "call"),
    "K2Node_Knot":              ("Reroute",      [],              "pure"),
    "K2Node_FunctionEntry":     ("FunctionEntry",["entry"],       "entry"),
    "K2Node_FunctionResult":    ("FunctionReturn",["exit"],       "call"),
    "K2Node_Self":              ("Self",         [],              "pure"),
    "K2Node_Cast":              ("Cast",         [],              "call"),
    "K2Node_DynamicCast":       ("Cast",         [],              "call"),
    "K2Node_Select":            ("Select",       [],              "pure"),
    "K2Node_SwitchEnum":        ("Switch",       [],              "control"),
    "K2Node_SwitchInteger":     ("Switch",       [],              "control"),
    "K2Node_SwitchString":      ("Switch",       [],              "control"),
    "K2Node_CommutativeAssociativeBinaryOperator": ("Math", [],   "pure"),
    "K2Node_PromotableOperator": ("Math",        [],              "pure"),
    "K2Node_SpawnActor":        ("SpawnActor",   ["external_call"],"call"),
    "K2Node_SpawnActorFromClass": ("SpawnActor",  ["external_call"],"call"),
    "K2Node_SetFieldsInStruct":  ("SetStructFields", ["struct"],  "call"),
    "K2Node_GetArrayItem":       ("ArrayOp",     [],              "pure"),
    "K2Node_Timeline":           ("Timeline",    ["latent"],      "latent"),
    "K2Node_Delay":              ("Delay",       ["latent"],      "latent"),
    "K2Node_RetriggerableDelay": ("Delay",       ["latent"],      "latent"),
}


def _classify_node(node_type_str: str):
    """Return (abstract_type, base_tags, role) for a UE node class name."""
    if node_type_str in _NODE_TYPE_MAP:
        return _NODE_TYPE_MAP[node_type_str]
    for prefix, val in _NODE_TYPE_MAP.items():
        if node_type_str.startswith(prefix):
            return val
    return ("Unknown", [], "call")


def _classify_function_call(title: str, node_det=None):
    """Two-level semantic classification for FunctionCall nodes.
    Supports both English and Chinese UE titles."""
    t = title.lower()
    semantic = {"type": "FunctionCall", "intent": "unknown", "side_effect": False}

    # Check for latent node by detecting LatentInfo pin (works regardless of language)
    if node_det:
        for p in node_det.get("pins", []):
            if p.get("name") == "LatentInfo" or "LatentActionInfo" in p.get("type", ""):
                semantic.update(intent="async", side_effect=True)
                return semantic

    # English + Chinese keyword matching
    if t.startswith("set ") or "设置" in t or "设定" in t:
        semantic.update(intent="state_write", side_effect=True)
    elif t.startswith("get ") or "获取" in t or "取得" in t:
        semantic.update(intent="state_read")
    elif "spawn" in t or "生成" in t:
        semantic.update(intent="spawn", side_effect=True)
    elif "destroy" in t or "销毁" in t or "摧毁" in t:
        semantic.update(intent="destroy", side_effect=True)
    elif "print" in t or "log" in t or "打印" in t or "输出日志" in t:
        semantic.update(intent="debug", side_effect=True)
    elif "delay" in t or "timer" in t or "延迟" in t or "定时" in t:
        semantic.update(intent="async", side_effect=True)
    elif ("play" in t or "播放" in t) and ("anim" in t or "montage" in t or "sound" in t or "动画" in t or "声音" in t):
        semantic.update(intent="play_media", side_effect=True)
    elif ("add" in t and "component" in t) or ("添加" in t and "组件" in t):
        semantic.update(intent="add_component", side_effect=True)
    elif "bind" in t or "delegate" in t or "绑定" in t:
        semantic.update(intent="bind_event", side_effect=True)
    elif "save" in t or "保存" in t:
        semantic.update(intent="save", side_effect=True)
    elif "load" in t or "加载" in t:
        semantic.update(intent="load")
    elif "cast" in t or "convert" in t or "转换" in t:
        semantic.update(intent="cast")
    elif "移动" in t or "move" in t:
        semantic.update(intent="movement", side_effect=True)
    return semantic


def _is_pure_node(node_det):
    """Check if a node has no exec pins (pure node)."""
    for p in node_det.get("pins", []):
        if p.get("type") == "exec":
            return False
    return True


def _build_data_expression(node_guid, pin_name, details_map, visited=None):
    """Recursively build a human-readable expression for a data input pin."""
    if visited is None:
        visited = set()
    if node_guid in visited:
        return "..."  # cycle guard
    visited.add(node_guid)

    det = details_map.get(node_guid)
    if not det:
        return "?"

    title = det.get("node_title", "?")
    ue_type = det.get("node_type", "")
    abstract, _, role = _classify_node(ue_type)

    # Leaf cases
    if abstract == "VariableGet":
        var_name = title.replace("Get ", "")
        return f"@{var_name}"
    if abstract == "Self":
        return "Self"

    # If this is a pure node (no exec pins), inline it as function(args).
    # B2 fix: also check _is_pure_node for FunctionCall math nodes created via
    # CreateMathFunctionNode — they have role "call" but no exec pins.
    is_actually_pure = (role == "pure") or _is_pure_node(det)
    if is_actually_pure and abstract not in ("Reroute",):
        args = []
        for p in det.get("pins", []):
            if p["direction"] != "input" or p.get("type") == "exec":
                continue
            if p.get("connected_to"):
                conn = p["connected_to"][0]
                arg_expr = _build_data_expression(
                    conn["node_guid"], conn["pin_name"], details_map, visited)
                args.append(arg_expr)
            elif p.get("default_value"):
                args.append(p["default_value"])
        # Detect math operations (both K2Node_Math and FunctionCall math nodes)
        _MATH_OPS = {
            "+": ["+", "add", "加", "整数+整数", "float+float"],
            "-": ["-", "subtract", "减", "整数-整数", "float-float"],
            "*": ["*", "multiply", "乘", "整数*整数", "float*float"],
            "/": ["/", "divide", "除", "整数/整数", "float/float"],
        }
        math_op = None
        title_lower = title.lower().replace(" ", "")
        for op_sym, keywords in _MATH_OPS.items():
            if any(k in title_lower for k in keywords):
                math_op = op_sym
                break
        if abstract == "Math" or math_op:
            op = math_op or title.replace(" ", "")
            if len(args) == 2:
                return f"({args[0]} {op} {args[1]})"
            return f"{op}({', '.join(args)})" if args else title
        if abstract in ("MakeStruct", "MakeArray", "SetStructFields"):
            return f"{title}({', '.join(args)})" if args else title
        return f"{title}({', '.join(args)})" if args else title

    # Reroute: pass through
    if abstract == "Reroute":
        for p in det.get("pins", []):
            if p["direction"] == "input" and p.get("connected_to"):
                conn = p["connected_to"][0]
                return _build_data_expression(
                    conn["node_guid"], conn["pin_name"], details_map, visited)
        return "?"

    # Impure node output: reference by name
    return f"@{title}.{pin_name}" if pin_name else f"@{title}"


def _normalize_graph(nodes, exec_edges, data_edges):
    """Remove reroute nodes and merge trivial exec chains."""
    reroute_guids = {n["id"] for n in nodes if n.get("node_type") == "Reroute"}
    if not reroute_guids:
        return nodes, exec_edges, data_edges

    # Rebuild data edges through reroutes
    new_data = []
    for e in data_edges:
        if e["from_node"] in reroute_guids:
            continue  # expression builder already handles reroutes
        if e["to_node"] in reroute_guids:
            continue
        new_data.append(e)

    # Rebuild exec edges through reroutes (shouldn't have exec, but just in case)
    new_exec = [e for e in exec_edges
                if e["from_node"] not in reroute_guids and e["to_node"] not in reroute_guids]

    new_nodes = [n for n in nodes if n["id"] not in reroute_guids]
    return new_nodes, new_exec, new_data


def _apply_node_filter(nodes, exec_edges, data_edges, node_filter="", node_ids=""):
    """Filter nodes by title regex or GUID list. Returns filtered (nodes, exec_edges, data_edges)."""
    if not node_filter and not node_ids:
        return nodes, exec_edges, data_edges

    keep_ids = set()

    if node_ids:
        # Exact GUID match
        id_list = [g.strip() for g in node_ids.split(",") if g.strip()]
        keep_ids.update(id_list)

    if node_filter:
        # Regex match on title or node_type
        patterns = [p.strip() for p in node_filter.split(",") if p.strip()]
        for n in nodes:
            for pat in patterns:
                try:
                    if (re.search(pat, n.get("title", ""), re.IGNORECASE) or
                            re.search(pat, n.get("node_type", ""), re.IGNORECASE)):
                        keep_ids.add(n["id"])
                        break
                except re.error:
                    # Treat as literal substring match on regex error
                    if (pat.lower() in n.get("title", "").lower() or
                            pat.lower() in n.get("node_type", "").lower()):
                        keep_ids.add(n["id"])
                        break

    filtered_nodes = [n for n in nodes if n["id"] in keep_ids]

    # Keep edges where both endpoints are in the filtered set
    # For boundary edges (one endpoint outside), include a stub
    filtered_exec = []
    for e in exec_edges:
        if e["from_node"] in keep_ids and e["to_node"] in keep_ids:
            filtered_exec.append(e)
        elif e["from_node"] in keep_ids or e["to_node"] in keep_ids:
            # Boundary edge — mark it
            stub = dict(e)
            stub["boundary"] = True
            filtered_exec.append(stub)

    filtered_data = []
    for e in data_edges:
        if e["from_node"] in keep_ids and e["to_node"] in keep_ids:
            filtered_data.append(e)
        elif e["from_node"] in keep_ids or e["to_node"] in keep_ids:
            stub = dict(e)
            stub["boundary"] = True
            filtered_data.append(stub)

    return filtered_nodes, filtered_exec, filtered_data


def _apply_subgraph_filter(subgraphs_event, subgraph_filter):
    """Filter event flow subgraphs by name."""
    if not subgraph_filter:
        return subgraphs_event
    names = [n.strip().lower() for n in subgraph_filter.split(",") if n.strip()]
    return [sg for sg in subgraphs_event if sg.get("name", "").lower() in names]


def _build_graph_description(blueprint_path, graph_info, depth, include_pseudo, send_fn,
                              node_filter="", node_ids="", subgraph_filter=""):
    """Build the LLM-friendly JSON for one graph (v2)."""
    graph_name = graph_info["graph_name"]
    graph_type = graph_info.get("graph_type", "EventGraph")
    function_id = graph_name if graph_type == "EventGraph" else graph_info["graph_guid"]

    # 1. Get all nodes with details in ONE call (fixes Issue 7: N+1 performance)
    batch_resp = send_fn({
        "type": "get_all_nodes_with_details",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
    })

    if not batch_resp.get("success"):
        # Fallback to N+1 if batch endpoint not available
        all_nodes_resp = send_fn({
            "type": "get_all_nodes",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
        })
        if not all_nodes_resp.get("success"):
            return {"error": all_nodes_resp.get("error", "Failed to get nodes")}
        raw_nodes = all_nodes_resp.get("nodes", [])
        if isinstance(raw_nodes, str):
            raw_nodes = json.loads(raw_nodes)
        all_details = []
        for n in raw_nodes:
            resp = send_fn({
                "type": "get_node_details",
                "blueprint_path": blueprint_path,
                "function_id": function_id,
                "node_guid": n["node_guid"],
            })
            if resp.get("success"):
                all_details.append(resp.get("details", {}))
    else:
        all_details = batch_resp.get("nodes", [])
        if isinstance(all_details, str):
            all_details = json.loads(all_details)

    # 2. Index by GUID
    details_by_guid = {}
    for det in all_details:
        # C++ may return individual elements as JSON-encoded strings; deserialize if needed
        if isinstance(det, str):
            try:
                det = json.loads(det)
            except (json.JSONDecodeError, TypeError):
                continue
        if isinstance(det, dict):
            details_by_guid[det.get("node_guid", "")] = det

    # 3. Build node list, edges, symbol table (v2: with roles, semantics, expressions)
    nodes_out = []
    exec_edges = []
    data_edges = []
    symbol_vars = {}
    symbol_events = {}
    symbol_structs = set()
    has_latent = False

    for guid, det in details_by_guid.items():
        ue_type = det.get("node_type", "")
        title = det.get("node_title", ue_type)
        abstract_type, base_tags, role = _classify_node(ue_type)
        tags = list(base_tags)

        # Issue 4: Two-level semantic classification for FunctionCall
        semantic = None
        if abstract_type == "FunctionCall":
            semantic = _classify_function_call(title, det)
            if semantic.get("side_effect"):
                tags.append("side_effect")
            if semantic["intent"] != "unknown":
                tags.append(semantic["intent"])
            # B4 fix: Detect latent FunctionCall nodes (Delay created via CreateMathFunctionNode)
            if semantic["intent"] == "async":
                role = "latent"
                tags.append("latent")

        # Issue 6: Detect latent/async nodes
        exec_semantics = None
        if role == "latent":
            has_latent = True
            # Find the resume pin (usually "Completed" or "then")
            resume_pin = "Completed"
            for p in det.get("pins", []):
                if p["direction"] == "output" and p.get("type") == "exec" and p["name"] != "then":
                    resume_pin = p["name"]
                    break
            exec_semantics = {"type": "latent", "suspends_at": "execute", "resumes_at": resume_pin}
            tags.append("latent")

        # Collect symbols
        if abstract_type in ("Event", "CustomEvent", "InputEvent"):
            symbol_events[guid] = {"id": f"EV_{title.replace(' ', '_')}", "name": title}
        elif abstract_type in ("VariableGet", "VariableSet"):
            var_name = title.replace("Get ", "").replace("Set ", "")
            if var_name not in symbol_vars:
                pin_type = ""
                for p in det.get("pins", []):
                    if p["name"] != "execute" and p["name"] != "then" and p["direction"] == "output":
                        pin_type = p.get("type", "")
                        break
                symbol_vars[var_name] = {"id": f"VAR_{var_name}", "name": var_name, "type": pin_type}
        elif abstract_type in ("MakeStruct", "BreakStruct", "SetStructFields"):
            for keyword in ("Make ", "Break ", "Set members in "):
                if title.startswith(keyword):
                    symbol_structs.add(title[len(keyword):].strip())

        # Build node entry
        node_entry = {
            "id": guid,
            "title": title,
            "node_type": abstract_type,
            "role": role,
            "tags": tags,
        }
        # Propagate canonical_id and instance_id for stable references
        canonical_id = det.get("canonical_id", "")
        if canonical_id:
            node_entry["canonical_id"] = canonical_id
        instance_id = det.get("instance_id", "")
        if instance_id:
            node_entry["instance_id"] = instance_id
        if semantic:
            node_entry["semantic"] = semantic
        if exec_semantics:
            node_entry["execution_semantics"] = exec_semantics

        if depth == "full":
            node_entry["class"] = ue_type
            node_entry["position"] = det.get("position", [0, 0])
            node_entry["pins"] = []
            for p in det.get("pins", []):
                node_entry["pins"].append({
                    "pin_id": f"{guid}.{p['name']}",
                    "name": p["name"],
                    "direction": p["direction"],
                    "kind": "exec" if p.get("type") == "exec" else "data",
                    "data_type": p.get("type", "") if p.get("type") != "exec" else None,
                    "default_value": p.get("default_value", ""),
                    "connected_to": p.get("connected_to", []),
                })

        nodes_out.append(node_entry)

        # Extract edges from output pins
        for p in det.get("pins", []):
            if p["direction"] != "output" or not p.get("connected_to"):
                continue
            for conn in p.get("connected_to", []):
                edge = {
                    "from_node": guid,
                    "from_pin": f"{guid}.{p['name']}",
                    "to_node": conn["node_guid"],
                    "to_pin": f"{conn['node_guid']}.{conn['pin_name']}",
                }
                if p.get("type") == "exec":
                    edge["edge_type"] = "exec"
                    if p["name"] not in ("then", "execute", "exec_out", "Completed"):
                        edge["condition"] = p["name"]
                    exec_edges.append(edge)
                else:
                    edge["edge_type"] = "data"
                    edge["data_type"] = p.get("type", "")
                    data_edges.append(edge)

    # Bonus: Graph normalization — remove reroutes
    nodes_out, exec_edges, data_edges = _normalize_graph(nodes_out, exec_edges, data_edges)

    # Apply node filtering if requested
    _filtering_active = bool(node_filter or node_ids)
    if _filtering_active:
        nodes_out, exec_edges, data_edges = _apply_node_filter(
            nodes_out, exec_edges, data_edges, node_filter, node_ids)

    # 4. Build exec adjacency (Issue 1: handle cycles via DFS with back-edge detection)
    exec_adj = {}
    exec_adj_with_cond = {}  # guid -> [(target_guid, condition_or_None)]
    for e in exec_edges:
        exec_adj.setdefault(e["from_node"], []).append(e["to_node"])
        exec_adj_with_cond.setdefault(e["from_node"], []).append(
            (e["to_node"], e.get("condition")))

    entry_guids = [n["id"] for n in nodes_out if "entry" in n.get("tags", [])]
    node_map = {n["id"]: n for n in nodes_out}

    # Issue 3: Three-layer subgraph detection
    # Phase A: DFS from each entry, track which entries reach which nodes
    entry_reach = {}  # guid -> set of entry_guids that reach it
    for entry in entry_guids:
        visited_set = set()
        stack = [entry]
        while stack:
            cur = stack.pop()
            if cur in visited_set:
                continue
            visited_set.add(cur)
            entry_reach.setdefault(cur, set()).add(entry)
            for nxt in exec_adj.get(cur, []):
                stack.append(nxt)

    # Phase B: Classify nodes
    shared_node_ids = {g for g, entries in entry_reach.items() if len(entries) > 1}
    event_flow_nodes = {}  # entry_guid -> [guid]
    for entry in entry_guids:
        flow = []
        stack = [entry]
        visited_set = set()
        while stack:
            cur = stack.pop()
            if cur in visited_set:
                continue
            visited_set.add(cur)
            if cur not in shared_node_ids or cur == entry:
                flow.append(cur)
            for nxt in exec_adj.get(cur, []):
                stack.append(nxt)
        event_flow_nodes[entry] = flow

    # Phase C: Include pure data-source nodes in their consumer's subgraph
    all_exec_assigned = set()
    for guids in event_flow_nodes.values():
        all_exec_assigned.update(guids)
    all_exec_assigned.update(shared_node_ids)

    # Issue 5 + Issue 2: Build execution trace with expressions and two-layer pseudocode
    def _build_trace_and_pseudo(entry_guid, flow_guids):
        """DFS-based linearized trace with control flow markers and inline expressions."""
        trace = []
        pseudo = []
        visited_trace = set()

        def _dfs(guid, indent=0):
            if guid in visited_trace:
                nd = node_map.get(guid)
                label = nd["title"] if nd else guid
                trace.append(("  " * indent) + f"Loop → {label}")
                pseudo.append(("  " * indent) + f"Loop → {label}")
                return
            visited_trace.add(guid)

            nd = node_map.get(guid)
            if not nd:
                return

            nt = nd["node_type"]
            title = nd["title"]
            prefix = "  " * indent

            # Issue 2: Build inline data expressions for impure node inputs.
            # Only include connected pins and non-default literal values to keep
            # pseudocode concise. Skip infrastructure pins (self, WorldContextObject)
            # and common UE defaults (true/false bools, zero vectors, "None" names).
            _SKIP_PINS = {"self", "WorldContextObject", "LatentInfo"}
            _BORING_DEFAULTS = {
                "", "0", "0.0", "0.000000", "None", "true", "false",
                "(R=0.000000,G=0.000000,B=0.000000,A=0.000000)",
                "(R=0.000000,G=0.660000,B=1.000000,A=1.000000)",  # UE default PrintString blue
                "(Linkage=-1,UUID=-1,ExecutionFunction=\"\",CallbackTarget=None)",
            }
            det = details_by_guid.get(guid, {})
            input_exprs = {}
            for p in det.get("pins", []):
                if p["direction"] != "input" or p.get("type") == "exec":
                    continue
                if p["name"] in _SKIP_PINS:
                    continue
                if p.get("connected_to"):
                    conn = p["connected_to"][0]
                    expr = _build_data_expression(
                        conn["node_guid"], conn["pin_name"], details_by_guid)
                    if expr != "?":
                        input_exprs[p["name"]] = expr
                elif p.get("default_value") and p["default_value"] not in _BORING_DEFAULTS:
                    input_exprs[p["name"]] = p["default_value"]

            args_str = ", ".join(f"{k}={v}" for k, v in input_exprs.items()) if input_exprs else ""

            # Generate trace line based on node type
            if nt in ("Event", "CustomEvent", "InputEvent", "FunctionEntry"):
                trace.append(f"{prefix}On {title}")
                pseudo.append(f"{prefix}On {title}")
            elif nt == "Branch":
                cond = input_exprs.get("Condition", "?")
                trace.append(f"{prefix}Branch ({cond}):")
                pseudo.append(f"{prefix}If {cond}:")
                # Follow True and False branches
                for target, condition in exec_adj_with_cond.get(guid, []):
                    if condition == "True":
                        trace.append(f"{prefix}  True →")
                        pseudo.append(f"{prefix}  True →")
                        _dfs(target, indent + 2)
                    elif condition == "False":
                        trace.append(f"{prefix}  False →")
                        pseudo.append(f"{prefix}  False →")
                        _dfs(target, indent + 2)
                    else:
                        _dfs(target, indent + 1)
                return  # Don't follow children again below
            elif nt == "Loop":
                trace.append(f"{prefix}Loop {title}:")
                pseudo.append(f"{prefix}Loop {title}:")
            elif nt == "Sequence":
                trace.append(f"{prefix}Sequence:")
                pseudo.append(f"{prefix}Sequence:")
            elif nt == "VariableSet":
                var = title.replace("Set ", "")
                val = input_exprs.get(var, args_str)
                trace.append(f"{prefix}Set {var} = {val}")
                pseudo.append(f"{prefix}Set {var} = {val}")
            elif nt == "FunctionCall":
                sem = nd.get("semantic", {})
                intent = sem.get("intent", "")
                if nd.get("execution_semantics"):
                    trace.append(f"{prefix}[async] {title}({args_str})")
                    pseudo.append(f"{prefix}[async] {title}({args_str})")
                else:
                    trace.append(f"{prefix}Call {title}({args_str})")
                    pseudo.append(f"{prefix}Call {title}({args_str})")
            elif nt == "FunctionReturn":
                trace.append(f"{prefix}Return")
                pseudo.append(f"{prefix}Return")
            elif nt in ("VariableGet", "Self"):
                pass  # pure, shown in expressions
            elif role == "latent":
                trace.append(f"{prefix}[async] {title}({args_str})")
                pseudo.append(f"{prefix}[async] {title}({args_str})")
            else:
                trace.append(f"{prefix}{title}({args_str})" if args_str else f"{prefix}{title}")
                pseudo.append(f"{prefix}{title}({args_str})" if args_str else f"{prefix}{title}")

            # Follow exec children (except Branch which is handled above)
            # B5 fix: exec chains stay at same indent, only control flow increases indent
            children = exec_adj_with_cond.get(guid, [])
            if nt in ("Sequence",):
                # Sequence children get indent+1
                for target, condition in children:
                    _dfs(target, indent + 1)
            elif len(children) == 1:
                # Linear chain: same indent level
                _dfs(children[0][0], indent)
            else:
                # Multiple outputs (not Branch — handled above): indent each
                for target, condition in children:
                    _dfs(target, indent + 1)

        _dfs(entry_guid)
        return trace, pseudo

    # Build subgraphs
    subgraphs_event = []
    for entry in entry_guids:
        flow = event_flow_nodes.get(entry, [])
        entry_title = node_map[entry]["title"] if entry in node_map else "Unknown"
        sg_id = f"SG_{entry_title.replace(' ', '_').replace('/', '_')}"

        flow_set = set(flow)
        # Include pure data sources
        data_sources = set()
        for e in data_edges:
            if e["to_node"] in flow_set and e["from_node"] not in all_exec_assigned:
                data_sources.add(e["from_node"])

        sg_node_ids = flow_set | data_sources
        sg_nodes = [n for n in nodes_out if n["id"] in sg_node_ids] if depth != "minimal" else []
        sg_exec = [e for e in exec_edges if e["from_node"] in flow_set and e["to_node"] in flow_set] if depth != "minimal" else []
        sg_data = [e for e in data_edges if e["to_node"] in flow_set and e["from_node"] in sg_node_ids] if depth != "minimal" else []

        # Issue 5: Two-layer output
        trace_lines, pseudo_lines = [], []
        summary_text = f"Flow starting from {entry_title}"
        if include_pseudo and not _filtering_active:
            trace_lines, pseudo_lines = _build_trace_and_pseudo(entry, flow)
            # Build high-level summary from key actions
            key_actions = [n["title"] for n in nodes_out
                           if n["id"] in flow_set and n.get("role") == "call"
                           and n.get("semantic", {}).get("side_effect")]
            if key_actions:
                summary_text = f"On {entry_title}: {', '.join(key_actions[:5])}"

        sg = {
            "subgraph_id": sg_id,
            "name": entry_title,
            "summary": summary_text,
            "entry_nodes": [entry],
            "nodes": sg_nodes,
            "exec_edges": sg_exec,
            "data_edges": sg_data,
            "execution_trace": trace_lines,
            "pseudocode": pseudo_lines,
        }
        subgraphs_event.append(sg)

    # Apply subgraph filter if requested
    if subgraph_filter:
        subgraphs_event = _apply_subgraph_filter(subgraphs_event, subgraph_filter)

    # Shared nodes subgraph
    subgraphs_shared = []
    if shared_node_ids:
        shared_nodes = [n for n in nodes_out if n["id"] in shared_node_ids] if depth != "minimal" else []
        subgraphs_shared.append({
            "subgraph_id": "SG_Shared",
            "name": "Shared Logic",
            "summary": "Nodes reachable from multiple entry events",
            "entry_nodes": [],
            "nodes": shared_nodes,
            "exec_edges": [e for e in exec_edges if e["from_node"] in shared_node_ids] if depth != "minimal" else [],
            "data_edges": [e for e in data_edges if e["from_node"] in shared_node_ids or e["to_node"] in shared_node_ids] if depth != "minimal" else [],
            "execution_trace": [],
            "pseudocode": [],
        })

    # Unassigned pure nodes
    all_assigned = all_exec_assigned | {n["id"] for sg in subgraphs_event for n in sg.get("nodes", [])}
    unassigned = [n for n in nodes_out if n["id"] not in all_assigned]

    # Issue 2: Build data_expressions for the graph
    data_expressions = []
    if depth in ("standard", "full"):
        for guid, det in details_by_guid.items():
            nd = node_map.get(guid)
            if not nd or nd.get("role") == "pure":
                continue
            for p in det.get("pins", []):
                if p["direction"] != "input" or p.get("type") == "exec":
                    continue
                if p.get("connected_to"):
                    conn = p["connected_to"][0]
                    src_node = node_map.get(conn["node_guid"])
                    src_det = details_by_guid.get(conn["node_guid"], {})
                    if src_node and (src_node.get("role") == "pure" or _is_pure_node(src_det)):
                        expr = _build_data_expression(
                            conn["node_guid"], conn["pin_name"], details_by_guid)
                        if expr != "?" and expr != f"@{nd['title']}.{p['name']}":
                            data_expressions.append({
                                "target": f"{nd['title']}.{p['name']}",
                                "expr": expr
                            })

    # 5. Symbol table
    symbol_table = {}
    if depth in ("standard", "full"):
        symbol_table = {
            "variables": list(symbol_vars.values()),
            "events": list(symbol_events.values()),
            "structs": [{"name": s} for s in sorted(symbol_structs)],
        }

    # 6. Assemble
    bp_name = blueprint_path.rsplit("/", 1)[-1] if "/" in blueprint_path else blueprint_path
    total_nodes = len(nodes_out)
    sg_names = [sg["name"] for sg in subgraphs_event]

    return {
        "blueprint_id": blueprint_path,
        "blueprint_name": bp_name,
        "graph_name": graph_name,
        "graph_type": graph_type,
        "summary": f"{bp_name} — {graph_name} ({total_nodes} nodes, {len(sg_names)} flows: {', '.join(sg_names)})",
        "execution_model": {
            "type": "flow_graph",
            "may_have_cycles": any(n.get("node_type") in ("Loop",) for n in nodes_out),
            "has_latent_nodes": has_latent,
        },
        "metadata": {"node_count": total_nodes, **({"filter_applied": True} if _filtering_active or subgraph_filter else {})},
        "symbol_table": symbol_table,
        "data_expressions": data_expressions if depth != "minimal" else [],
        "subgraphs": {
            "event_flows": subgraphs_event,
            "shared_functions": subgraphs_shared,
        },
        "cross_subgraph_edges": [],
    }


def _to_pseudocode_only(graph_desc):
    """Strip a standard-depth graph description down to pseudocode-only output."""
    flows = []
    for sg in graph_desc.get("subgraphs", {}).get("event_flows", []):
        pseudo_lines = sg.get("pseudocode", [])
        flows.append({
            "name": sg.get("name", ""),
            "summary": sg.get("summary", ""),
            "pseudocode": "\n".join(pseudo_lines) if pseudo_lines else "",
        })
    for sg in graph_desc.get("subgraphs", {}).get("shared_functions", []):
        pseudo_lines = sg.get("pseudocode", [])
        if pseudo_lines:
            flows.append({
                "name": sg.get("name", ""),
                "summary": sg.get("summary", ""),
                "pseudocode": "\n".join(pseudo_lines) if pseudo_lines else "",
            })
    return {
        "blueprint_id": graph_desc.get("blueprint_id", ""),
        "blueprint_name": graph_desc.get("blueprint_name", ""),
        "graph_name": graph_desc.get("graph_name", ""),
        "summary": graph_desc.get("summary", ""),
        "node_count": graph_desc.get("metadata", {}).get("node_count", 0),
        "symbol_table": graph_desc.get("symbol_table", {}),
        "flows": flows,
    }


@mcp.tool()
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

    # Post-process: strip legend on repeat, strip redundant graph fields
    return _postprocess_describe(result_json, compact, max_depth, blueprint_path)


# ---------------------------------------------------------------------------
# Layout helpers (P1) — anchor extraction and node role classification
# ---------------------------------------------------------------------------

# All layout helpers live in layout_engine.py — imported as _build_occupancy, _find_free_slot,
# _classify_node_role, _extract_layout_intent at the top of this file.


# ---------------------------------------------------------------------------
# apply_blueprint_patch — batch blueprint modifications from compact JSON
# ---------------------------------------------------------------------------

@mcp.tool()
def apply_blueprint_patch(
    blueprint_path: str,
    function_id: str,
    patch_json: str,
    verify: bool = True,
    dry_run: bool = False,
) -> str:
    """
    Apply a batch of blueprint modifications in one call.

    This is the PRIMARY tool for modifying blueprints. Instead of calling
    add_node + connect_nodes + set_pin_value individually (many round-trips),
    describe all changes in a single patch JSON.

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")
        function_id: "EventGraph" or function GUID
        patch_json: JSON string with the patch operations (see format below)
        verify: If True, return a compact verification of affected nodes (saves a separate describe call)
        dry_run: If True, validate the patch via preflight WITHOUT applying. Returns
                 predicted changes and issues. Safe to call before live apply.

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
    # dry_run: validate via preflight and return predicted changes without applying
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

    try:
        patch = json.loads(patch_json)
    except json.JSONDecodeError as e:
        return json.dumps({"success": False, "error": f"Invalid patch JSON: {e}"})

    # --- Structured patch log for observability ---
    patch_log = {
        "blueprint": blueprint_path,
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "phases": {},
        "rollback": False,
        "total_created": 0,
        "total_connected": 0,
        "total_errors": 0,
        "error_categories": [],
    }

    def _looks_like_guid(s: str) -> bool:
        """Return True if s looks like a UUID/GUID (32+ hex chars, optional hyphens)."""
        import re as _re
        return bool(_re.match(r'^[0-9A-Fa-f]{8}[-]?([0-9A-Fa-f]{4}[-]?){3}[0-9A-Fa-f]{12}$', s))

    def _classify_error(error_msg: str) -> str:
        """Classify an error message into a failure taxonomy category."""
        msg = error_msg.lower()
        if "pin" in msg and ("not found" in msg or "available" in msg):
            return "PIN_NOT_FOUND"
        if "type" in msg and ("mismatch" in msg or "incompatible" in msg):
            return "TYPE_MISMATCH"
        if "node" in msg and ("not found" in msg or "cannot resolve" in msg or "cannot find" in msg):
            return "NODE_NOT_FOUND"
        if "function" in msg and ("not found" in msg or "could not" in msg or "bind" in msg):
            return "FUNCTION_BIND_FAILED"
        if "compile" in msg or "compilation" in msg:
            return "COMPILE_ERROR"
        return "UNKNOWN"

    results = {
        "success": True,
        "created_nodes": {},
        "removed_nodes": [],
        "connections_made": 0,
        "connections_removed": 0,
        "pin_values_set": 0,
        "errors": [],
    }

    # Map ref_id and title -> GUID for connection resolution
    ref_to_guid = {}

    # Build title, canonical_id, and instance_id -> GUID maps from existing nodes
    # Use bulk call instead of N+1 individual get_node_details calls
    title_to_guid = {}
    canonical_to_guid = {}   # template-level: first match wins
    instance_to_guid = {}    # instance-level: unique per node
    bulk_resp = send_to_unreal({
        "type": "get_all_nodes_with_details",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
    })
    # Single pass over existing nodes: build title/canonical/instance maps + collect positions.
    _existing_xs: list[float] = []
    _existing_ys: list[float] = []
    _guid_to_pos: dict[str, list[float]] = {}  # built here, reused in Phase 2

    if bulk_resp.get("success"):
        raw = bulk_resp.get("nodes", [])
        if isinstance(raw, str):
            raw = json.loads(raw)
        for det in raw:
            guid = det.get("node_guid", "")
            if not guid:
                continue
            title = det.get("node_title", "")
            if title:
                title_to_guid[title] = guid
            canonical = det.get("canonical_id", "")
            if canonical and canonical not in canonical_to_guid:
                canonical_to_guid[canonical] = guid
            instance = det.get("instance_id", "")
            if instance:
                instance_to_guid[instance] = guid
            pos = det.get("position", [])
            if isinstance(pos, list) and len(pos) >= 2:
                xf, yf = float(pos[0]), float(pos[1])
                _existing_xs.append(xf)
                _existing_ys.append(yf)
                _guid_to_pos[guid] = [xf, yf]

    def resolve_node_id(ref: str) -> str:
        """Resolve a ref_id, instance_id, canonical_id, GUID, or title to a GUID.

        Resolution order:
          1. ref_to_guid (freshly added nodes in this patch)
          2. instance_to_guid / canonical_to_guid / title_to_guid (exec-chain traversal)
          3. FName fallback: NodeType#N → NodeType_N lookup via get_node_guid_by_fname
             (reaches ForEachLoop body nodes, Switch body nodes, etc.)
          4. Assume it's already a GUID (passthrough)
        """
        if ref in ref_to_guid:
            return ref_to_guid[ref]
        if ref in instance_to_guid:
            return instance_to_guid[ref]
        if ref in canonical_to_guid:
            return canonical_to_guid[ref]
        if ref in title_to_guid:
            return title_to_guid[ref]

        # FName fallback: handles nodes inside ForEachLoop/Switch bodies
        # that exec-chain traversal cannot reach.
        # instance_id format "NodeType#N" → FName fast path "NodeType_N"
        if "#" in ref and not _looks_like_guid(ref):
            node_type, _, idx = ref.partition("#")
            fname_guess = f"{node_type}_{idx}"
            # Fast path: try direct FName match (works ~80% of the time)
            fb_resp = send_to_unreal({
                "type": "get_node_guid_by_fname",
                "blueprint_path": blueprint_path,
                "graph_id": function_id,
                "node_fname": fname_guess,
                "node_class_filter": "",
            })
            if isinstance(fb_resp, dict) and fb_resp.get("success"):
                guid = fb_resp.get("node_guid", "")
                if guid:
                    # Cache within this patch call's lifetime only
                    instance_to_guid[ref] = guid
                    return guid
            # Slow path: _scan_graph_for_instance (not yet implemented)
            # Falls through to passthrough below

        # FName passthrough: if ref looks like a node FName (e.g. K2Node_CallFunction_24)
        # but wasn't in exec-chain maps, try resolving it to a real GUID via FName lookup.
        # This handles refs that are already FNames rather than instance_id/GUID format.
        if not _looks_like_guid(ref) and "_" in ref and " " not in ref and "#" not in ref:
            fb_resp = send_to_unreal({
                "type": "get_node_guid_by_fname",
                "blueprint_path": blueprint_path,
                "graph_id": function_id,
                "node_fname": ref,
                "node_class_filter": "",
            })
            if isinstance(fb_resp, dict) and fb_resp.get("success"):
                guid = fb_resp.get("node_guid", "")
                if guid:
                    instance_to_guid[ref] = guid
                    return guid

        # Assume it's already a GUID
        return ref

    def resolve_endpoint(endpoint: str):
        """Parse 'NodeRef::PinName' or 'NodeRef.PinName' into (guid, pin_name).

        The '::' separator is preferred because pin names can contain dots
        (e.g. struct sub-pins like 'ReturnValue.Location'). The '.' separator
        is kept as a fallback for backwards compatibility.
        """
        if "::" in endpoint:
            parts = endpoint.split("::", 1)
        else:
            parts = endpoint.rsplit(".", 1)
        if len(parts) != 2:
            return None, None
        node_ref, pin_name = parts
        return resolve_node_id(node_ref), pin_name

    # --- Auto-preflight: catch obvious issues before mutating the graph ---
    # Only run if patch has add_nodes or add_connections (skip for remove-only patches)
    has_adds = patch.get("add_nodes") or patch.get("add_connections")
    if has_adds:
        preflight_resp = send_to_unreal({
            "type": "preflight_blueprint_patch",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
            "patch_json": patch_json,
        })
        if preflight_resp.get("success") and not preflight_resp.get("valid", True):
            issues = preflight_resp.get("issues", [])
            patch_log["phases"]["auto_preflight"] = {"success": False, "issues": len(issues)}
            patch_log["total_errors"] = len(issues)
            patch_log["error_categories"].append("PREFLIGHT_REJECTED")
            print(f"[PATCH_LOG] {json.dumps(patch_log, separators=(',', ':'), ensure_ascii=False)}",
                  file=sys.stderr)
            return json.dumps({
                "success": False,
                "preflight_failed": True,
                "issues": issues,
                "errors": [f"Preflight rejected: {i.get('message', '')}" for i in issues],
            }, separators=(",", ":"), ensure_ascii=False)

    # Record node count before mutation (best-effort, for graph_state diagnostic)
    try:
        _before_dr = send_to_unreal({
            "type": "list_graphs",
            "blueprint_path": blueprint_path,
        })
        _before_nc = -1
        if isinstance(_before_dr, dict) and _before_dr.get("success"):
            for _g in _before_dr.get("graphs", []):
                if _g.get("graph_name", "") == function_id or _g.get("graph_type") == "EventGraph":
                    _v = _g.get("node_count", -1)
                    if isinstance(_v, int) and _v >= 0:
                        _before_nc = _v
                        break
        results["_node_count_before"] = _before_nc
    except Exception:
        results["_node_count_before"] = -1

    # --- Begin transaction for atomic rollback ---
    bp_short = blueprint_path.rsplit("/", 1)[-1] if "/" in blueprint_path else blueprint_path
    txn_resp = send_to_unreal({
        "type": "begin_transaction",
        "transaction_name": f"ApplyPatch_{bp_short}",
    })
    txn_started = txn_resp.get("success", False)

    # --- Phase 1: Remove nodes ---
    for guid in patch.get("remove_nodes", []):
        resp = send_to_unreal({
            "type": "delete_node",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
            "node_id": guid,
        })
        if resp.get("success"):
            results["removed_nodes"].append(guid)
        else:
            err_msg = f"remove_node {guid}: {resp.get('error')}"
            results["errors"].append(err_msg)
            patch_log["error_categories"].append(_classify_error(err_msg))
    patch_log["phases"]["remove_nodes"] = {
        "success": len(results["removed_nodes"]) == len(patch.get("remove_nodes", [])),
        "count": len(results["removed_nodes"]),
    }

    # --- Phase 2: Add nodes ---
    # P0–P2: Safe auto-placement via layout_engine.
    _max_existing_x = max(_existing_xs) if _existing_xs else 0
    _sorted_ys = sorted(_existing_ys)
    _median_y = _sorted_ys[len(_sorted_ys) // 2] if _sorted_ys else 0

    # Build title→pos map from already-collected _guid_to_pos (no second pass over raw)
    _full_pos_map = dict(_guid_to_pos)
    for t, g in title_to_guid.items():
        if g in _guid_to_pos:
            _full_pos_map[t] = _guid_to_pos[g]

    # Extract anchor and placement_mode from patch connections
    _intent = _extract_layout_intent(patch, title_to_guid, _full_pos_map)
    _anchor_pos = _intent["anchor_pos"]
    _placement_mode = _intent["placement_mode"]

    if _anchor_pos is not None:
        auto_x = _anchor_pos[0]
        auto_y_base = _anchor_pos[1]
    else:
        auto_x = _max_existing_x
        auto_y_base = _median_y

    _occupied = _build_occupancy(_existing_xs, _existing_ys)

    _placed_new: dict[str, list[float]] = {}  # positions of newly added nodes in this batch

    # Pre-build upstream map once: O(M), then O(1) per node (same logic as layout_engine.py)
    _upstream_map: dict[str, str] = {}
    for _conn in patch.get("add_connections", []):
        _s = _conn.get("from", "")
        _t = _conn.get("to", "")
        _s = _s.split("::", 1)[0] if "::" in _s else _s.rsplit(".", 1)[0]
        _t = _t.split("::", 1)[0] if "::" in _t else _t.rsplit(".", 1)[0]
        if _s and _t:
            _upstream_map.setdefault(_t, _s)

    for node_spec in patch.get("add_nodes", []):
        ref_id = node_spec.get("ref_id", f"node_{auto_x}")
        node_type = node_spec.get("node_type", "K2Node_CallFunction")

        if "position" not in node_spec:
            role = _classify_node_role(node_spec, patch)
            _src = _upstream_map.get(ref_id)
            local_anchor = _placed_new.get(_src) if _src else None
            anchor = local_anchor if local_anchor is not None else (
                _anchor_pos if _anchor_pos is not None else [auto_x, auto_y_base]
            )
            pos = _find_free_slot(anchor, _occupied, role, _placement_mode)
        else:
            pos = node_spec["position"]

        _placed_new[ref_id] = pos
        auto_x = max(auto_x, pos[0])

        # B7 fix: For K2Node_CallFunction ONLY, use function_name as node_type
        # because C++ AddNode resolves function names via TryCreateNodeFromLibraries.
        # For other node types (CustomEvent, VariableGet, MakeStruct), keep original node_type.
        effective_type = node_type
        if "function_name" in node_spec and node_type == "K2Node_CallFunction":
            fn = node_spec["function_name"]
            # Strip "Class." prefix — C++ fuzzy match works on bare function names
            if "." in fn:
                fn = fn.split(".")[-1]
            effective_type = fn

        # Merge pin_values INTO node_properties so C++ sets them in the same
        # add_node call (avoids separate socket round-trips which have timing issues).
        props = {}
        # Always pass function_name in properties — C++ resolver fallback needs it
        # for component methods and BP class members that TryCreateNodeFromLibraries can't find.
        if "function_name" in node_spec:
            props["function_name"] = node_spec["function_name"]
        for k, v in node_spec.items():
            if k not in ("ref_id", "node_type", "function_name", "position", "pin_values"):
                props[k] = v
        for pin_name, pin_val in node_spec.get("pin_values", {}).items():
            props[pin_name] = str(pin_val)

        resp = send_to_unreal({
            "type": "add_node",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
            "node_type": effective_type,
            "node_position": pos,
            "node_properties": props,
        })
        if resp.get("success"):
            new_guid = resp.get("node_id", "")
            ref_to_guid[ref_id] = new_guid
            results["created_nodes"][ref_id] = new_guid
            results["pin_values_set"] += len(node_spec.get("pin_values", {}))
        else:
            err = resp.get("error", "Unknown")
            suggestions = resp.get("suggestions", "")
            err_msg = f"add_node {ref_id} ({node_type}): {err}"
            results["errors"].append(err_msg)
            patch_log["error_categories"].append(_classify_error(err_msg))
            if suggestions:
                results["errors"].append(f"  suggestions: {suggestions}")
    patch_log["phases"]["add_nodes"] = {
        "success": len(results["created_nodes"]) == len(patch.get("add_nodes", [])),
        "count": len(results["created_nodes"]),
    }
    patch_log["total_created"] = len(results["created_nodes"])

    # --- Phase 2b: Refresh title/canonical/instance cache for newly created nodes ---
    # Without this, connections by title/canonical to nodes created in Phase 2 would fail
    # because maps were built before Phase 2 ran.
    for ref_id, new_guid in ref_to_guid.items():
        det_resp = send_to_unreal({
            "type": "get_node_details",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
            "node_guid": new_guid,
        })
        if det_resp.get("success"):
            det = det_resp.get("details", {})
            title = det.get("node_title", "")
            if title:
                title_to_guid[title] = new_guid
            canonical = det.get("canonical_id", "")
            if canonical and canonical not in canonical_to_guid:
                canonical_to_guid[canonical] = new_guid
            instance = det.get("instance_id", "")
            if instance:
                instance_to_guid[instance] = new_guid

    # --- Phase 3: Remove connections ---
    for conn in patch.get("remove_connections", []):
        src_guid, src_pin = resolve_endpoint(conn.get("from", ""))
        tgt_guid, tgt_pin = resolve_endpoint(conn.get("to", ""))
        if not src_guid or not tgt_guid:
            results["errors"].append(f"disconnect: cannot resolve {conn}")
            continue
        resp = send_to_unreal({
            "type": "disconnect_nodes",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
            "source_node_id": src_guid,
            "source_pin": src_pin,
            "target_node_id": tgt_guid,
            "target_pin": tgt_pin,
        })
        if resp.get("success"):
            results["connections_removed"] += 1
        else:
            err_msg = f"disconnect {conn}: {resp.get('error')}"
            results["errors"].append(err_msg)
            patch_log["error_categories"].append(_classify_error(err_msg))
    patch_log["phases"]["remove_connections"] = {
        "success": results["connections_removed"] == len(patch.get("remove_connections", [])),
        "count": results["connections_removed"],
    }

    # --- Phase 4: Add connections (bulk) ---
    bulk_conns = []
    for conn in patch.get("add_connections", []):
        src_guid, src_pin = resolve_endpoint(conn.get("from", ""))
        tgt_guid, tgt_pin = resolve_endpoint(conn.get("to", ""))
        if not src_guid or not tgt_guid:
            results["errors"].append(f"connect: cannot resolve {conn}")
            continue
        # Store original ref strings for FName fallback later
        def _extract_ref(endpoint: str) -> str:
            if "::" in endpoint:
                return endpoint.split("::", 1)[0]
            return endpoint.rsplit(".", 1)[0] if "." in endpoint else endpoint
        bulk_conns.append({
            "source_node_id": src_guid,
            "source_pin": src_pin,
            "target_node_id": tgt_guid,
            "target_pin": tgt_pin,
            "_src_ref": _extract_ref(conn.get("from", "")),
            "_tgt_ref": _extract_ref(conn.get("to", "")),
        })

    if bulk_conns:
        resp = send_to_unreal({
            "type": "connect_nodes_bulk",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
            "connections": [{k: v for k, v in c.items() if not k.startswith("_")}
                            for c in bulk_conns],
        })
        if resp.get("success"):
            results["connections_made"] = resp.get("successful_connections", len(bulk_conns))
            for fc in resp.get("failed_connections", []):
                err_msg = f"connect failed: {fc}"
                results["errors"].append(err_msg)
                patch_log["error_categories"].append(_classify_error(str(fc)))
        else:
            raw_err = resp.get("error")
            err_msg = f"connect_bulk: {raw_err}"
            results["errors"].append(err_msg)
            patch_log["error_categories"].append(_classify_error(err_msg))

            # ── Diagnostic probe ────────────────────────────────────────────
            # Surface actual available pin names when C++ returned no detail.
            diag_result = {"src_pins": None, "tgt_pins": None}
            if bulk_conns:
                probe = bulk_conns[0]
                diag_resp = send_to_unreal({
                    "type": "connect_nodes",
                    "blueprint_path": blueprint_path,
                    "function_guid": function_id,
                    "source_node_guid": probe["source_node_id"],
                    "source_pin_name": probe["source_pin"],
                    "target_node_guid": probe["target_node_id"],
                    "target_pin_name": probe["target_pin"],
                })
                if not diag_resp.get("success"):
                    src_avail = [p.get("name") for p in diag_resp.get("source_available_pins", [])[:6]]
                    tgt_avail = [p.get("name") for p in diag_resp.get("target_available_pins", [])[:6]]
                    diag_result = {"src_pins": src_avail, "tgt_pins": tgt_avail}
                    results["errors"].append(
                        f"connect_diagnostic:   {probe['source_pin']}→{probe['target_pin']}: "
                        f"src_pins={src_avail} tgt_pins={tgt_avail}"
                    )

            # ── Automatic FName fallback ────────────────────────────────────
            # Triggered when: GUID path failed AND diagnostic shows node-not-found
            # (src_pins=[] or tgt_pins=[] means GUID drift, not pin-name issue)
            _node_not_found = (
                diag_result["src_pins"] == [] or
                diag_result["tgt_pins"] == [] or
                raw_err is None
            )
            if _node_not_found:
                def _ref_to_fname(ref: str) -> str:
                    """Convert node ref to FName, verify it exists in the graph.
                    Handles: instance_id (K2Node_SwitchString#0 → K2Node_SwitchString_0)
                             plain FName (K2Node_CallFunction_24 — already an FName)
                    """
                    if _looks_like_guid(ref):
                        return ""  # GUID refs cannot be converted to FName here
                    fname_guess = None
                    if "#" in ref:
                        node_type, _, idx = ref.partition("#")
                        fname_guess = f"{node_type}_{idx}"
                    elif "_" in ref:
                        # Already looks like an FName (e.g. K2Node_CallFunction_24)
                        fname_guess = ref
                    if fname_guess:
                        verify = send_to_unreal({
                            "type": "get_node_guid_by_fname",
                            "blueprint_path": blueprint_path,
                            "graph_id": function_id,
                            "node_fname": fname_guess,
                            "node_class_filter": "",
                        })
                        if isinstance(verify, dict) and verify.get("success"):
                            return fname_guess
                    return ""

                fname_successes = 0
                fname_errors = []
                for ci in bulk_conns:
                    src_fname = _ref_to_fname(ci.get("_src_ref", ""))
                    tgt_fname = _ref_to_fname(ci.get("_tgt_ref", ""))
                    if src_fname and tgt_fname:
                        fb = send_to_unreal({
                            "type": "connect_nodes_by_fname",
                            "blueprint_path": blueprint_path,
                            "graph_id": function_id,
                            "src_fname": src_fname, "src_pin": ci["source_pin"],
                            "tgt_fname": tgt_fname, "tgt_pin": ci["target_pin"],
                        })
                        if isinstance(fb, dict) and fb.get("success"):
                            fname_successes += 1
                        else:
                            fname_errors.append(
                                f"{src_fname}.{ci['source_pin']}→"
                                f"{tgt_fname}.{ci['target_pin']}: "
                                f"{fb.get('error') if isinstance(fb, dict) else 'no response'}"
                            )
                    else:
                        fname_errors.append(
                            f"cannot resolve FName for "
                            f"'{ci.get('_src_ref','')}' or '{ci.get('_tgt_ref','')}'"
                        )

                if fname_successes == len(bulk_conns):
                    # All connections succeeded via FName fallback
                    results["errors"] = [e for e in results["errors"]
                                         if "connect_bulk" not in e]
                    results["connections_made"] = fname_successes
                    results["connection_method"] = "FName-fallback"
                    patch_log["error_categories"] = [c for c in patch_log["error_categories"]
                                                     if c != _classify_error(err_msg)]
                elif fname_successes > 0:
                    results["errors"].append(
                        f"partial FName fallback: {fname_successes}/{len(bulk_conns)} succeeded"
                    )
                    results["errors"].extend(fname_errors)
                else:
                    results["errors"].extend(fname_errors)
            # ───────────────────────────────────────────────────────────────
    patch_log["phases"]["add_connections"] = {
        "success": results["connections_made"] == len(patch.get("add_connections", [])),
        "count": results["connections_made"],
        "attempted": len(bulk_conns),
    }
    patch_log["total_connected"] = results["connections_made"]

    # --- Phase 5: Set pin values on existing nodes ---
    for pv in patch.get("set_pin_values", []):
        node_guid = resolve_node_id(pv.get("node", ""))
        resp = send_to_unreal({
            "type": "set_node_pin_value",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
            "node_guid": node_guid,
            "pin_name": pv.get("pin", ""),
            "value": str(pv.get("value", "")),
        })
        if resp.get("success"):
            results["pin_values_set"] += 1
        else:
            err_msg = f"set_pin {pv.get('node')}.{pv.get('pin')}: {resp.get('error')}"
            results["errors"].append(err_msg)
            patch_log["error_categories"].append(_classify_error(err_msg))
    patch_log["phases"]["set_pin_values"] = {
        "success": results["pin_values_set"] == len(patch.get("set_pin_values", [])) + len(
            [n for n in patch.get("add_nodes", []) if n.get("pin_values")]),
        "count": results["pin_values_set"],
    }

    # --- Phase 6: Auto-compile ---
    if patch.get("auto_compile", True):
        comp_resp = send_to_unreal({
            "type": "compile_blueprint",
            "blueprint_path": blueprint_path,
        })
        results["compiled"] = comp_resp.get("success", False)
        if not comp_resp.get("success"):
            err_msg = f"compile: {comp_resp.get('error', 'failed')}"
            results["errors"].append(err_msg)
            patch_log["error_categories"].append("COMPILE_ERROR")
        patch_log["phases"]["compile"] = {"success": comp_resp.get("success", False)}

    results["success"] = len(results["errors"]) == 0

    # --- Commit or rollback transaction ---
    if txn_started:
        if results["errors"]:
            cancel_resp = send_to_unreal({"type": "cancel_transaction"})
            if cancel_resp.get("success"):
                results["rolled_back"] = True
                results["created_nodes"] = {}  # Nodes no longer exist after rollback
                ref_to_guid.clear()
                patch_log["rollback"] = True
            # UE5 Python CancelTransaction does not reliably remove nodes added via Python API.
            # Always run remove_unused_nodes as a hard cleanup guard after any rollback attempt.
            cleanup_resp = send_to_unreal({
                "type": "remove_unused_nodes",
                "blueprint_path": blueprint_path,
            })
            results["cleanup_ran"] = cleanup_resp.get("success", False)
        else:
            send_to_unreal({"type": "end_transaction"})
            results["committed"] = True

    # --- Finalize structured patch log ---
    patch_log["total_errors"] = len(results["errors"])
    patch_log["success"] = results["success"]
    # Deduplicate error categories
    patch_log["error_categories"] = list(dict.fromkeys(patch_log["error_categories"]))
    # Emit to stderr for structured observability (visible in MCP server logs)
    print(f"[PATCH_LOG] {json.dumps(patch_log, separators=(',', ':'), ensure_ascii=False)}",
          file=sys.stderr)

    # --- graph_state: best-effort node count (diagnostic only, never blocks patch) ---
    def _get_node_count_lightweight() -> int:
        try:
            raw = send_to_unreal({
                "type": "list_graphs",
                "blueprint_path": blueprint_path,
            })
            # list_graphs returns {"success": true, "graphs": [{"graph_name": ..., "node_count": N}, ...]}
            if isinstance(raw, dict) and raw.get("success"):
                for g in raw.get("graphs", []):
                    if g.get("graph_name", "") == function_id or g.get("graph_type") == "EventGraph":
                        nc = g.get("node_count", -1)
                        if isinstance(nc, int) and nc >= 0:
                            return nc
        except Exception:
            pass
        return -1

    results["graph_state"] = {
        "node_count_before": results.pop("_node_count_before", -1),
        "node_count_after": _get_node_count_lightweight(),
    }

    # Invalidate describe cache for this blueprint
    _describe_cache.invalidate(blueprint_path)
    # Also invalidate node details cache for this blueprint
    keys_to_remove = [k for k in _node_details_cache if k[0] == blueprint_path]
    for k in keys_to_remove:
        del _node_details_cache[k]

    # --- Phase 7: Optional inline verification ---
    if verify and results["created_nodes"]:
        affected_guids = ",".join(results["created_nodes"].values())
        list_resp = send_to_unreal({"type": "list_graphs", "blueprint_path": blueprint_path})
        if list_resp.get("success"):
            all_graphs = list_resp.get("graphs", [])
            if isinstance(all_graphs, str):
                all_graphs = json.loads(all_graphs)
            target_graphs = [g for g in all_graphs
                             if g["graph_name"] == function_id or g.get("graph_guid") == function_id]
            if not target_graphs:
                target_graphs = all_graphs[:1]
            if target_graphs:
                verify_desc = _build_graph_description(
                    blueprint_path, target_graphs[0], "standard", False, send_to_unreal,
                    node_ids=affected_guids)
                # Strip to compact verification: only affected nodes with their connections
                v_nodes = []
                for sg in verify_desc.get("subgraphs", {}).get("event_flows", []):
                    v_nodes.extend(sg.get("nodes", []))
                for sg in verify_desc.get("subgraphs", {}).get("shared_functions", []):
                    v_nodes.extend(sg.get("nodes", []))
                results["verification"] = {
                    "affected_nodes": [{
                        "id": n["id"],
                        "title": n.get("title", ""),
                        "connections": sum(1 for e in verify_desc.get("subgraphs", {}).get("event_flows", [{}])[0].get("exec_edges", [])
                                          if e.get("from_node") == n["id"] or e.get("to_node") == n["id"])
                    } for n in v_nodes],
                    "total_graph_nodes": verify_desc.get("metadata", {}).get("node_count", 0),
                }

    # Wrap in unified envelope
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
    return json.dumps(envelope, separators=(",", ":"), ensure_ascii=False)


# ---------------------------------------------------------------------------
# preflight_blueprint_patch — validate patch without mutating
# ---------------------------------------------------------------------------

@mcp.tool()
def preflight_blueprint_patch(
    blueprint_path: str,
    patch_json: str,
    function_id: str = "EventGraph",
) -> str:
    """
    Validate a blueprint patch WITHOUT applying it.

    Returns predicted pin schemas for each add_nodes entry and a list of issues
    (wrong pin names, unresolvable functions, missing nodes). Call this BEFORE
    apply_blueprint_patch to catch errors before they dirty the graph.

    Args:
        blueprint_path: Path to the Blueprint (e.g., "/Game/Blueprints/BP_MyActor")
        patch_json: Same JSON format as apply_blueprint_patch
        function_id: "EventGraph" or function GUID (default "EventGraph")

    Returns:
        JSON with: valid (bool), issues (array), predicted_nodes (array with pin schemas).
    """
    command = {
        "type": "preflight_blueprint_patch",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "patch_json": patch_json,
    }
    response = send_to_unreal(command)
    if response.get("success"):
        data = {k: v for k, v in response.items() if k != "success"}
        return json.dumps(_compact_response(data), separators=(",", ":"), ensure_ascii=False)
    return json.dumps(response, separators=(",", ":"), ensure_ascii=False)


# ---------------------------------------------------------------------------
# debug_blueprint — inspect, simulate, and trace blueprint execution
# ---------------------------------------------------------------------------

@mcp.tool()
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
actors = unreal.EditorLevelLibrary.get_all_level_actors()
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

actors = unreal.EditorLevelLibrary.get_all_level_actors()
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

# @mcp.tool()  # INTERNAL — use higher-level tools instead
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

@mcp.tool()
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

    # Also use BlueprintEditorLibrary to ensure full recompile
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)

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
                    envelope = _make_envelope(
                        "compile_blueprint_with_errors", compiled, inner,
                        summary=summary,
                        warnings=warnings,
                        diagnostics=[{"message": e} for e in errors],
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

@mcp.tool()
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

    return json.dumps({"success": True, "nodes_moved": moved})


# ---------------------------------------------------------------------------
# P1: find_scene_objects — filtered scene query
# ---------------------------------------------------------------------------

@mcp.tool()
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

actors = unreal.EditorLevelLibrary.get_all_level_actors()
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

@mcp.tool()
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

@mcp.tool()
@mcp.tool()
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

@mcp.tool()
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

@mcp.tool()
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

@mcp.tool()
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

@mcp.tool()
def search_blueprint_nodes(
    query: str,
    blueprint_path: str = "",
    category_filter: str = "",
    max_results: int = 5,
    schema_mode: str = "semantic",
) -> str:
    """
    Search ALL available Blueprint node types across the entire engine.

    Returns a lightweight shortlist of matching nodes. Use inspect_blueprint_node
    on any candidate to get full pin schema and patch_hint.

    Covers thousands of functions (not just the 10 common libraries).

    Args:
        query: Natural language or function name fragment (e.g., "move to location", "AI navigate", "print")
        blueprint_path: Optional BP path for context-aware scoring (e.g., "/Game/Blueprints/BP_MyNPC")
        category_filter: Optional category filter (e.g., "AI|Navigation", "Math")
        max_results: Number of results (default 5, max 10)
        schema_mode: Output format — "semantic" (default) or "verbose" (for debugging)

    Returns:
        JSON with candidates array (canonical_name, display_name, node_kind, spawn_strategy, score).
    """
    schema_mode = _normalize_schema_mode(schema_mode)
    command = {
        "type": "search_blueprint_nodes",
        "query": query,
        "blueprint_path": blueprint_path,
        "category_filter": category_filter,
        "max_results": min(max_results, 10),
        "schema_mode": schema_mode,
    }
    response = send_to_unreal(command)
    if response.get("success"):
        data = {k: v for k, v in response.items() if k != "success"}
        return json.dumps(_compact_response(data), separators=(",", ":"), ensure_ascii=False)
    return json.dumps(response, separators=(",", ":"), ensure_ascii=False)


# ---------------------------------------------------------------------------
# P2: inspect_blueprint_node — full pin schema for a specific node type
# ---------------------------------------------------------------------------

@mcp.tool()
def inspect_blueprint_node(
    canonical_name: str,
    blueprint_path: str = "",
    schema_mode: str = "semantic",
) -> str:
    """
    Get full pin schema and patch_hint for a specific Blueprint node type.

    Call search_blueprint_nodes first to find the canonical_name, then call this
    to get exact pin names, types, and directions for building patches.

    Args:
        canonical_name: The canonical name from search results (e.g., "KismetSystemLibrary.PrintString")
        blueprint_path: Optional BP path for context-aware inspection
        schema_mode: Output format — "semantic" (default) or "verbose" (for debugging)

    Returns:
        JSON with: canonical_name, patch_hint (ready to copy into apply_blueprint_patch),
        pins array (name, direction, type, required, default), context_requirements.
    """
    schema_mode = _normalize_schema_mode(schema_mode)
    command = {
        "type": "inspect_blueprint_node",
        "canonical_name": canonical_name,
        "blueprint_path": blueprint_path,
        "schema_mode": schema_mode,
    }
    response = send_to_unreal(command)
    if response.get("success"):
        data = {k: v for k, v in response.items() if k != "success"}
        return json.dumps(_compact_response(data), separators=(",", ":"), ensure_ascii=False)
    return json.dumps(response, separators=(",", ":"), ensure_ascii=False)


# ---------------------------------------------------------------------------
# P2: get_compilation_status — check without recompiling
# ---------------------------------------------------------------------------

# @mcp.tool()  # INTERNAL — use higher-level tools instead
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

@mcp.tool()
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


@mcp.tool()
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


@mcp.tool()
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


@mcp.tool()
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

@mcp.tool()
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


@mcp.tool()
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


@mcp.tool()
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
    return json.dumps(resp)


@mcp.tool()
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


@mcp.tool()
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

@mcp.tool()
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


@mcp.tool()
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


@mcp.tool()
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

@mcp.tool()
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


@mcp.tool()
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


@mcp.tool()
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

@mcp.tool()
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
    import sys as _sys
    from pathlib import Path as _Path

    _scripts_dir = str(_Path(__file__).parent.parent.parent.parent.parent.parent / "Scripts")
    if _scripts_dir not in _sys.path:
        _sys.path.insert(0, _scripts_dir)

    from lib.blueprint_qa_core import analyze as _qa_analyze

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

    # compile
    compile_resp = send_to_unreal({"type": "compile_blueprint", "blueprint_path": blueprint_path})

    report = _qa_analyze(bp, compile_resp, mode=mode, blueprint_path=blueprint_path)
    return json.dumps(report, indent=2, ensure_ascii=False)


if __name__ == "__main__":
    import traceback

    try:
        print("Server starting...", file=sys.stderr)
        mcp.run()
    except Exception as e:
        print(f"Server crashed with error: {e}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        raise
