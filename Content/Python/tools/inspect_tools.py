"""Blueprint inspection tools (get_node_details, search, inspect, preflight).

Extracted from mcp_server.py — this is the single implementation.
mcp_server.py @exposed_tool wrappers call these functions.
"""
import json

from tools.envelope import (
    _normalize_schema_mode,
    _compact_response,
    _node_details_cache,
    _NODE_DETAILS_CACHE_MAX,
)


def get_node_details(blueprint_path: str, function_id: str, node_guid: str,
                     schema_mode: str = "semantic", send_fn=None) -> str:
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
        send_fn: transport function (injected by mcp_server)

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
    response = send_fn(command)
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


def search_blueprint_nodes(
    query: str,
    blueprint_path: str = "",
    category_filter: str = "",
    max_results: int = 5,
    schema_mode: str = "semantic",
    send_fn=None,
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
        send_fn: transport function (injected by mcp_server)

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
    response = send_fn(command)
    if response.get("success"):
        data = {k: v for k, v in response.items() if k != "success"}
        return json.dumps(_compact_response(data), separators=(",", ":"), ensure_ascii=False)
    return json.dumps(response, separators=(",", ":"), ensure_ascii=False)


def inspect_blueprint_node(
    canonical_name: str,
    blueprint_path: str = "",
    schema_mode: str = "semantic",
    send_fn=None,
) -> str:
    """
    Get full pin schema and patch_hint for a specific Blueprint node type.

    Call search_blueprint_nodes first to find the canonical_name, then call this
    to get exact pin names, types, and directions for building patches.

    Args:
        canonical_name: The canonical name from search results (e.g., "KismetSystemLibrary.PrintString")
        blueprint_path: Optional BP path for context-aware inspection
        schema_mode: Output format — "semantic" (default) or "verbose" (for debugging)
        send_fn: transport function (injected by mcp_server)

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
    response = send_fn(command)
    if response.get("success"):
        data = {k: v for k, v in response.items() if k != "success"}
        return json.dumps(_compact_response(data), separators=(",", ":"), ensure_ascii=False)
    return json.dumps(response, separators=(",", ":"), ensure_ascii=False)


def preflight_blueprint_patch(
    blueprint_path: str,
    patch_json: str,
    function_id: str = "EventGraph",
    send_fn=None,
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
        send_fn: transport function (injected by mcp_server)

    Returns:
        JSON with: valid (bool), issues (array), predicted_nodes (array with pin schemas).
    """
    command = {
        "type": "preflight_blueprint_patch",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
        "patch_json": patch_json,
    }
    response = send_fn(command)
    if response.get("success"):
        data = {k: v for k, v in response.items() if k != "success"}
        return json.dumps(_compact_response(data), separators=(",", ":"), ensure_ascii=False)
    return json.dumps(response, separators=(",", ":"), ensure_ascii=False)
