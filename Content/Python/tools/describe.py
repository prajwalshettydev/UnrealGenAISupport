"""Blueprint graph description builder (describe_blueprint internals).

Extracted from mcp_server.py — this is the single implementation.
mcp_server.py re-imports these symbols.
"""
import json
import re


# ---------------------------------------------------------------------------
# Node type classification map
# ---------------------------------------------------------------------------

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


def _resolve_neighborhood_node_ref(node_ref: str, details_by_guid: dict):
    """Resolve a node_ref string to a GUID using details_by_guid.

    Resolution order:
      1. GUID (32 hex chars) — direct lookup
      2. instance_id exact match (K2Node_X_3 or K2Node_X#3 form)
      3. canonical_id exact match
      4. Title regex (case-insensitive) — ambiguity returns candidates

    Returns (guid_or_None, warnings_list, candidates_list).
    candidates_list is non-empty only when the ref is ambiguous.
    """
    import re as _re

    # 1. GUID passthrough
    if _re.fullmatch(r"[0-9A-Fa-f]{32}", node_ref):
        if node_ref in details_by_guid:
            return node_ref, [], []
        return None, [f"GUID '{node_ref}' not found in graph"], []

    # 2. instance_id (normalise K2Node_X#3 → K2Node_X_3)
    normalised = node_ref.replace("#", "_")
    for guid, det in details_by_guid.items():
        iid = det.get("instance_id", "")
        if iid in (node_ref, normalised):
            return guid, [], []

    # 3. canonical_id exact match
    for guid, det in details_by_guid.items():
        if det.get("canonical_id", "") == node_ref:
            return guid, [], []

    # 4. Title regex (case-insensitive)
    try:
        pat = _re.compile(node_ref, _re.IGNORECASE)
    except _re.error:
        pat = _re.compile(_re.escape(node_ref), _re.IGNORECASE)

    matches = []
    for guid, det in details_by_guid.items():
        if pat.search(det.get("node_title", "")):
            matches.append({
                "guid": guid,
                "title": det.get("node_title", ""),
                "instance_id": det.get("instance_id", ""),
            })

    if len(matches) == 1:
        return matches[0]["guid"], [], []
    if len(matches) > 1:
        return None, [f"Ambiguous: {len(matches)} nodes match '{node_ref}'"], matches

    return None, [f"No node found matching '{node_ref}'"], []


def _bfs_neighborhood(center_guid: str, details_by_guid: dict,
                      depth: int, direction: str):
    """BFS from center_guid up to depth hops, filtered by direction.

    direction values:
      "exec_out"  — follow exec output edges forward
      "exec_in"   — follow exec input edges backward
      "exec"      — both exec directions
      "data"      — both data pin directions
      "all"       — exec + data, all directions

    Returns (neighbor_records, edge_records).
    Each neighbor_record: {guid, title, instance_id, node_type, distance, direction, via_pin}
    Each edge_record:     {from, to, type}
    """
    from collections import deque

    # Build adjacency dicts in a single O(N) pass
    exec_out: dict = {}   # guid -> [(neighbor_guid, from_pin_name, to_pin_name)]
    exec_in: dict = {}
    data_out: dict = {}
    data_in: dict = {}

    def _append(d, k, v):
        d.setdefault(k, []).append(v)

    for guid, det in details_by_guid.items():
        for pin in det.get("pins", []):
            if pin.get("direction") != "output":
                continue
            is_exec = pin.get("type", "") == "exec"
            adj_out = exec_out if is_exec else data_out
            adj_in = exec_in if is_exec else data_in
            from_pin = f"{guid}.{pin['name']}"
            for conn in pin.get("connected_to", []):
                tgt_guid = conn.get("node_guid", "")
                to_pin = f"{tgt_guid}.{conn.get('pin_name', '')}"
                _append(adj_out, guid, (tgt_guid, from_pin, to_pin))
                _append(adj_in, tgt_guid, (guid, from_pin, to_pin))

    # Select adjacency sets based on direction
    _dir_map = {
        "exec_out": [("exec_out", exec_out)],
        "exec_in":  [("exec_in",  exec_in)],
        "exec":     [("exec_out", exec_out), ("exec_in", exec_in)],
        "data":     [("data_out", data_out), ("data_in", data_in)],
        "all":      [("exec_out", exec_out), ("exec_in", exec_in),
                     ("data_out", data_out), ("data_in", data_in)],
    }
    active_adjs = _dir_map.get(direction, _dir_map["exec"])

    # BFS
    visited: dict = {center_guid: 0}
    queue = deque([(center_guid, 0)])
    neighbors = []
    edges_seen: set = set()
    edge_records = []

    while queue:
        current_guid, hop = queue.popleft()
        if hop >= depth:
            continue
        for dir_label, adj in active_adjs:
            for (nbr_guid, from_pin, to_pin) in adj.get(current_guid, []):
                edge_key = (from_pin, to_pin)
                if edge_key not in edges_seen:
                    edges_seen.add(edge_key)
                    etype = "exec" if "exec" in dir_label else "data"
                    edge_records.append({"from": from_pin, "to": to_pin, "type": etype})
                if nbr_guid in visited:
                    continue
                visited[nbr_guid] = hop + 1
                det = details_by_guid.get(nbr_guid, {})
                via = to_pin.split(".")[-1] if "." in to_pin else to_pin
                neighbors.append({
                    "guid": nbr_guid,
                    "title": det.get("node_title", ""),
                    "instance_id": det.get("instance_id", ""),
                    "node_type": det.get("node_type", ""),
                    "distance": hop + 1,
                    "direction": dir_label,
                    "via_pin": via,
                })
                queue.append((nbr_guid, hop + 1))

    return neighbors, edge_records


def _graph_fetch_nodes(blueprint_path, graph_info, send_fn):
    """Phase 1: Fetch all node details from UE and index by GUID.

    Returns (details_by_guid, error_response_or_None).
    On failure the caller should return error_response immediately.
    """
    graph_type = graph_info.get("graph_type", "EventGraph")
    graph_name = graph_info["graph_name"]
    function_id = graph_name if graph_type == "EventGraph" else graph_info["graph_guid"]

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
            return {}, {"error": all_nodes_resp.get("error", "Failed to get nodes")}
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

    details_by_guid = {}
    for det in all_details:
        if isinstance(det, str):
            try:
                det = json.loads(det)
            except (json.JSONDecodeError, TypeError):
                continue
        if isinstance(det, dict):
            details_by_guid[det.get("node_guid", "")] = det

    return details_by_guid, None


def _graph_build_nodes_and_edges(details_by_guid, depth):
    """Phase 2: Build classified node list, edges, and symbol collections.

    Returns a dict with keys: nodes_out, exec_edges, data_edges,
    symbol_vars, symbol_events, symbol_structs, has_latent.
    """
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

    # Graph normalization — remove reroutes
    nodes_out, exec_edges, data_edges = _normalize_graph(nodes_out, exec_edges, data_edges)

    return {
        "nodes_out": nodes_out,
        "exec_edges": exec_edges,
        "data_edges": data_edges,
        "symbol_vars": symbol_vars,
        "symbol_events": symbol_events,
        "symbol_structs": symbol_structs,
        "has_latent": has_latent,
    }


def _graph_build_subgraphs(nodes_out, exec_edges, data_edges, details_by_guid,
                            depth, include_pseudo, filtering_active, subgraph_filter):
    """Phase 3: Detect subgraphs, build traces/pseudocode, data expressions, symbol table.

    Returns (subgraphs_event, subgraphs_shared, data_expressions, all_exec_assigned).
    """
    # Build exec adjacency (Issue 1: handle cycles via DFS with back-edge detection)
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
            elif nd.get("role") == "latent":
                trace.append(f"{prefix}[async] {title}({args_str})")
                pseudo.append(f"{prefix}[async] {title}({args_str})")
            else:
                trace.append(f"{prefix}{title}({args_str})" if args_str else f"{prefix}{title}")
                pseudo.append(f"{prefix}{title}({args_str})" if args_str else f"{prefix}{title}")

            # Follow exec children (except Branch which is handled above)
            # B5 fix: exec chains stay at same indent, only control flow increases indent
            children = exec_adj_with_cond.get(guid, [])
            if nt in ("Sequence",):
                for target, condition in children:
                    _dfs(target, indent + 1)
            elif len(children) == 1:
                _dfs(children[0][0], indent)
            else:
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
        if include_pseudo and not filtering_active:
            trace_lines, pseudo_lines = _build_trace_and_pseudo(entry, flow)
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

    return subgraphs_event, subgraphs_shared, data_expressions, all_exec_assigned


def _graph_assemble_result(blueprint_path, graph_info, depth, nodes_out, exec_edges,
                           data_edges, symbol_vars, symbol_events, symbol_structs,
                           has_latent, subgraphs_event, subgraphs_shared,
                           data_expressions, filtering_active, subgraph_filter):
    """Phase 4: Assemble the final response dict."""
    graph_name = graph_info["graph_name"]
    graph_type = graph_info.get("graph_type", "EventGraph")

    # Symbol table
    symbol_table = {}
    if depth in ("standard", "full"):
        symbol_table = {
            "variables": list(symbol_vars.values()),
            "events": list(symbol_events.values()),
            "structs": [{"name": s} for s in sorted(symbol_structs)],
        }

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
        "metadata": {"node_count": total_nodes, **({"filter_applied": True} if filtering_active or subgraph_filter else {})},
        "symbol_table": symbol_table,
        "data_expressions": data_expressions if depth != "minimal" else [],
        "subgraphs": {
            "event_flows": subgraphs_event,
            "shared_functions": subgraphs_shared,
        },
        "cross_subgraph_edges": [],
    }


def _build_graph_description(blueprint_path, graph_info, depth, include_pseudo, send_fn,
                              node_filter="", node_ids="", subgraph_filter=""):
    """Build the LLM-friendly JSON for one graph (v2).

    Orchestrates four phases: fetch -> build nodes/edges -> build subgraphs -> assemble.
    """
    # Phase 1: Fetch all node details from UE
    details_by_guid, error = _graph_fetch_nodes(blueprint_path, graph_info, send_fn)
    if error:
        return error

    # Phase 2: Build classified nodes, edges, and symbol collections
    built = _graph_build_nodes_and_edges(details_by_guid, depth)
    nodes_out = built["nodes_out"]
    exec_edges = built["exec_edges"]
    data_edges = built["data_edges"]

    # Apply node filtering if requested
    filtering_active = bool(node_filter or node_ids)
    if filtering_active:
        nodes_out, exec_edges, data_edges = _apply_node_filter(
            nodes_out, exec_edges, data_edges, node_filter, node_ids)

    # Phase 3: Detect subgraphs, build traces and data expressions
    subgraphs_event, subgraphs_shared, data_expressions, _ = _graph_build_subgraphs(
        nodes_out, exec_edges, data_edges, details_by_guid,
        depth, include_pseudo, filtering_active, subgraph_filter)

    # Phase 4: Assemble final result
    return _graph_assemble_result(
        blueprint_path, graph_info, depth, nodes_out, exec_edges, data_edges,
        built["symbol_vars"], built["symbol_events"], built["symbol_structs"],
        built["has_latent"], subgraphs_event, subgraphs_shared,
        data_expressions, filtering_active, subgraph_filter)


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
