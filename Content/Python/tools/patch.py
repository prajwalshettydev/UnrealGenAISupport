"""Blueprint patch helpers (apply_blueprint_patch internals).

Extracted from mcp_server.py — this is the single implementation.
mcp_server.py re-imports these symbols.
"""
import json
import re
import sys
import time

from layout_engine import (
    build_occupancy as _build_occupancy,
    find_free_slot as _find_free_slot,
    classify_node_role as _classify_node_role,
    extract_layout_intent as _extract_layout_intent,
)

from tools.envelope import _evict_bp_caches, _LOCALE_ALIASES
from tools.describe import _build_graph_description


# --- Patch helper utilities (module-level, no closure needed) ---

def _patch_looks_like_guid(s: str) -> bool:
    """Return True if s looks like a UUID/GUID (32+ hex chars, optional hyphens)."""
    return bool(re.match(r'^[0-9A-Fa-f]{8}[-]?([0-9A-Fa-f]{4}[-]?){3}[0-9A-Fa-f]{12}$', s))


def _patch_classify_error(error_msg: str) -> str:
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


def _patch_extract_ref(endpoint: str) -> str:
    """Extract node ref from an endpoint string (strip pin portion)."""
    if "::" in endpoint:
        return endpoint.split("::", 1)[0]
    return endpoint.rsplit(".", 1)[0] if "." in endpoint else endpoint


# ---------------------------------------------------------------------------
# Phase 1: _patch_validate — parse JSON, early-exit for dry_run
# ---------------------------------------------------------------------------

def _patch_validate(patch_json, blueprint_path, function_id, send_fn):
    """Parse patch JSON. Handle dry_run early return.

    Returns (patch_dict, error_response_str | None).
    Exactly one of the two will be non-None:
      - patch_dict is set on success (normal path)
      - error_response_str is set on JSON decode failure
    """
    try:
        patch = json.loads(patch_json)
    except json.JSONDecodeError as e:
        return None, json.dumps({"success": False, "error": f"Invalid patch JSON: {e}"})
    return patch, None


# ---------------------------------------------------------------------------
# Phase 2: _patch_resolve_refs — build lookup maps from existing nodes
# ---------------------------------------------------------------------------

def _patch_resolve_refs(patch, blueprint_path, function_id, send_fn):
    """Fetch all existing nodes and build resolution maps.

    Returns a dict with keys:
      ref_to_guid, instance_to_guid, canonical_to_guid, title_to_guid,
      guid_drift_log, resolution_warnings,
      existing_xs, existing_ys, guid_to_pos,
      preflight_reject (str|None — JSON response if preflight rejected),
      node_count_before (int),
      patch_log_updates (dict — phases/errors to merge into patch_log),
    Plus closures: resolve_node_id, resolve_endpoint, fname_lookup.
    """
    ref_to_guid = {}
    resolution_warnings: list[dict] = []
    title_to_guid = {}
    canonical_to_guid = {}
    instance_to_guid = {}
    guid_drift_log: list[dict] = []

    # Bulk fetch existing nodes
    bulk_resp = send_fn({
        "type": "get_all_nodes_with_details",
        "blueprint_path": blueprint_path,
        "function_id": function_id,
    })
    existing_xs: list[float] = []
    existing_ys: list[float] = []
    guid_to_pos: dict[str, list[float]] = {}

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
                existing_xs.append(xf)
                existing_ys.append(yf)
                guid_to_pos[guid] = [xf, yf]

    # --- Closure helpers that capture the maps ---
    def _fname_lookup(fname: str) -> str:
        fb_resp = send_fn({
            "type": "get_node_guid_by_fname",
            "blueprint_path": blueprint_path,
            "graph_id": function_id,
            "node_fname": fname,
            "node_class_filter": "",
        })
        if isinstance(fb_resp, dict) and fb_resp.get("success"):
            return fb_resp.get("node_guid", "")
        return ""

    def resolve_node_id(ref: str, *, late: bool = False, strict: bool = False) -> str:
        """Resolve a ref_id, fname:, instance_id, canonical_id, GUID, or title to a GUID."""
        if ref in ref_to_guid:
            return ref_to_guid[ref]
        if ref.startswith("fname:"):
            fname = ref[6:]
            guid = _fname_lookup(fname)
            if guid:
                guid_drift_log.append({
                    "fname": fname, "old_guid": None, "new_guid": guid,
                    "reason": "explicit fname", "phase": "resolve",
                })
                return guid
            if late:
                _evict_bp_caches(blueprint_path)
            raise ValueError(f"FName '{fname}' not found in graph")
        if _patch_looks_like_guid(ref):
            return ref
        if ref in instance_to_guid:
            return instance_to_guid[ref]
        if ref in canonical_to_guid:
            return canonical_to_guid[ref]
        if ref in title_to_guid:
            return title_to_guid[ref]
        if "#" in ref:
            node_type, _, idx = ref.partition("#")
            fname_guess = f"{node_type}_{idx}"
            guid = _fname_lookup(fname_guess)
            if guid:
                instance_to_guid[ref] = guid
                return guid
        if "_" in ref and " " not in ref and "#" not in ref:
            guid = _fname_lookup(ref)
            if guid:
                instance_to_guid[ref] = guid
                return guid
        if ref in _LOCALE_ALIASES:
            alias = _LOCALE_ALIASES[ref]
            if alias in canonical_to_guid:
                return canonical_to_guid[alias]
            if alias in title_to_guid:
                return title_to_guid[alias]
        if late:
            _evict_bp_caches(blueprint_path)
        if strict:
            raise ValueError(f"REF_UNRESOLVABLE: Cannot resolve '{ref}'. Known refs (sample): {list(instance_to_guid.keys())[:5]}")
        resolution_warnings.append({
            "severity": "warning", "code": "REF_UNRESOLVABLE",
            "message": f"Could not resolve ref '{ref}' — passing as-is (may fail at connect time)",
            "location": {"ref": ref, "phase": "resolution"},
        })
        return ref

    def resolve_endpoint(endpoint: str, *, late: bool = False, strict: bool = False):
        if "::" in endpoint:
            parts = endpoint.split("::", 1)
        else:
            parts = endpoint.rsplit(".", 1)
        if len(parts) != 2:
            return None, None
        node_ref, pin_name = parts
        return resolve_node_id(node_ref, late=late, strict=strict), pin_name

    # --- Auto-preflight ---
    patch_log_updates: dict = {}
    has_adds = patch.get("add_nodes") or patch.get("add_connections")
    preflight_reject = None
    if has_adds:
        preflight_resp = send_fn({
            "type": "preflight_blueprint_patch",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
            "patch_json": json.dumps(patch, separators=(",", ":"), ensure_ascii=False),
        })
        if preflight_resp.get("success") and not preflight_resp.get("valid", True):
            issues = preflight_resp.get("issues", [])
            patch_log_updates["auto_preflight"] = {"success": False, "issues": len(issues)}
            preflight_reject = json.dumps({
                "success": False,
                "preflight_failed": True,
                "issues": issues,
                "errors": [f"Preflight rejected: {i.get('message', '')}" for i in issues],
            }, separators=(",", ":"), ensure_ascii=False)

    # --- Pre-mutation node count ---
    node_count_before = -1
    try:
        _before_dr = send_fn({
            "type": "list_graphs",
            "blueprint_path": blueprint_path,
        })
        if isinstance(_before_dr, dict) and _before_dr.get("success"):
            for _g in _before_dr.get("graphs", []):
                if _g.get("graph_name", "") == function_id or _g.get("graph_type") == "EventGraph":
                    _v = _g.get("node_count", -1)
                    if isinstance(_v, int) and _v >= 0:
                        node_count_before = _v
                        break
    except Exception:
        pass

    return {
        "ref_to_guid": ref_to_guid,
        "instance_to_guid": instance_to_guid,
        "canonical_to_guid": canonical_to_guid,
        "title_to_guid": title_to_guid,
        "guid_drift_log": guid_drift_log,
        "resolution_warnings": resolution_warnings,
        "existing_xs": existing_xs,
        "existing_ys": existing_ys,
        "guid_to_pos": guid_to_pos,
        "preflight_reject": preflight_reject,
        "node_count_before": node_count_before,
        "patch_log_updates": patch_log_updates,
        # Closures
        "resolve_node_id": resolve_node_id,
        "resolve_endpoint": resolve_endpoint,
        "fname_lookup": _fname_lookup,
    }


# ---------------------------------------------------------------------------
# Phase 3: _patch_mutate — execute all graph mutations (phases 0.5–6)
# ---------------------------------------------------------------------------

def _patch_mutate(patch, blueprint_path, function_id, ctx, send_fn, patch_log):
    """Execute remove_nodes, add_nodes, connections, set_pin_values, compile.

    Args:
        ctx: dict from _patch_resolve_refs (maps + closures)
        patch_log: mutable patch log dict (phases written in-place)

    Returns results dict.
    """
    ref_to_guid = ctx["ref_to_guid"]
    title_to_guid = ctx["title_to_guid"]
    canonical_to_guid = ctx["canonical_to_guid"]
    instance_to_guid = ctx["instance_to_guid"]
    guid_drift_log = ctx["guid_drift_log"]
    resolve_node_id = ctx["resolve_node_id"]
    resolve_endpoint = ctx["resolve_endpoint"]
    _fname_lookup = ctx["fname_lookup"]
    _existing_xs = ctx["existing_xs"]
    _existing_ys = ctx["existing_ys"]
    _guid_to_pos = ctx["guid_to_pos"]

    results = {
        "success": True,
        "created_nodes": {},
        "removed_nodes": [],
        "connections_made": 0,
        "connections_removed": 0,
        "pin_values_set": 0,
        "errors": [],
        "_node_count_before": ctx["node_count_before"],
    }

    # --- Begin transaction for atomic rollback ---
    bp_short = blueprint_path.rsplit("/", 1)[-1] if "/" in blueprint_path else blueprint_path
    txn_resp = send_fn({
        "type": "begin_transaction",
        "transaction_name": f"ApplyPatch_{bp_short}",
    })
    results["_txn_started"] = txn_resp.get("success", False)

    # --- Phase 0.5: Auto-detach connections for nodes being removed ---
    _auto_detach_log = []
    for node_ref in patch.get("remove_nodes", []):
        guid = resolve_node_id(node_ref)
        if not guid or guid == node_ref:
            continue
        detail_resp = send_fn({
            "type": "get_node_details",
            "blueprint_path": blueprint_path,
            "node_guid": guid,
        })
        if not isinstance(detail_resp, dict) or not detail_resp.get("success"):
            continue
        for pin in detail_resp.get("pins", []):
            for link in pin.get("links", []):
                linked_node = link.get("node_guid") or link.get("node_id")
                linked_pin = link.get("pin_name") or link.get("pin")
                if linked_node and linked_pin:
                    dc_resp = send_fn({
                        "type": "disconnect_nodes",
                        "blueprint_path": blueprint_path,
                        "function_guid": function_id,
                        "source_node_guid": guid,
                        "source_pin_name": pin.get("name", ""),
                        "target_node_guid": linked_node,
                        "target_pin_name": linked_pin,
                    })
                    _auto_detach_log.append({
                        "node": guid, "pin": pin.get("name"), "linked_node": linked_node, "linked_pin": linked_pin,
                        "success": dc_resp.get("success", False) if isinstance(dc_resp, dict) else False,
                    })
    patch_log["phases"]["auto_detach"] = {"count": len(_auto_detach_log), "detached": _auto_detach_log}

    # --- Phase 1: Remove nodes ---
    for guid in patch.get("remove_nodes", []):
        resp = send_fn({
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
            patch_log["error_categories"].append(_patch_classify_error(err_msg))
    patch_log["phases"]["remove_nodes"] = {
        "success": len(results["removed_nodes"]) == len(patch.get("remove_nodes", [])),
        "count": len(results["removed_nodes"]),
    }

    # --- Phase 2: Add nodes ---
    _max_existing_x = max(_existing_xs) if _existing_xs else 0
    _sorted_ys = sorted(_existing_ys)
    _median_y = _sorted_ys[len(_sorted_ys) // 2] if _sorted_ys else 0

    _full_pos_map = dict(_guid_to_pos)
    for t, g in title_to_guid.items():
        if g in _guid_to_pos:
            _full_pos_map[t] = _guid_to_pos[g]

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
    _placed_new: dict[str, list[float]] = {}

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

        effective_type = node_type
        if "function_name" in node_spec and node_type == "K2Node_CallFunction":
            fn = node_spec["function_name"]
            if "." in fn:
                fn = fn.split(".")[-1]
            effective_type = fn

        props = {}
        if "function_name" in node_spec:
            props["function_name"] = node_spec["function_name"]
        for k, v in node_spec.items():
            if k not in ("ref_id", "node_type", "function_name", "position", "pin_values"):
                props[k] = v
        for pin_name, pin_val in node_spec.get("pin_values", {}).items():
            props[pin_name] = str(pin_val)

        resp = send_fn({
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
            patch_log["error_categories"].append(_patch_classify_error(err_msg))
            if suggestions:
                results["errors"].append(f"  suggestions: {suggestions}")
    patch_log["phases"]["add_nodes"] = {
        "success": len(results["created_nodes"]) == len(patch.get("add_nodes", [])),
        "count": len(results["created_nodes"]),
    }
    patch_log["total_created"] = len(results["created_nodes"])

    # --- Phase 2b: Refresh title/canonical/instance cache for newly created nodes ---
    for ref_id_key, new_guid in ref_to_guid.items():
        det_resp = send_fn({
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
        try:
            src_guid, src_pin = resolve_endpoint(conn.get("from", ""), strict=True)
            tgt_guid, tgt_pin = resolve_endpoint(conn.get("to", ""), strict=True)
        except ValueError as e:
            results.setdefault("diagnostics", []).append({
                "severity": "error", "code": "REF_UNRESOLVABLE",
                "message": str(e), "location": {"phase": "remove_connections", "connection": conn},
            })
            results["errors"].append(str(e))
            continue
        if not src_guid or not tgt_guid:
            results["errors"].append(f"disconnect: cannot resolve {conn}")
            continue
        resp = send_fn({
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
            patch_log["error_categories"].append(_patch_classify_error(err_msg))
    patch_log["phases"]["remove_connections"] = {
        "success": results["connections_removed"] == len(patch.get("remove_connections", [])),
        "count": results["connections_removed"],
    }

    # --- Phase 4: Add connections (bulk) ---
    bulk_conns = []
    for conn in patch.get("add_connections", []):
        src_guid, src_pin = resolve_endpoint(conn.get("from", ""), late=True)
        tgt_guid, tgt_pin = resolve_endpoint(conn.get("to", ""), late=True)
        if not src_guid or not tgt_guid:
            results["errors"].append(f"connect: cannot resolve {conn}")
            continue
        bulk_conns.append({
            "source_node_id": src_guid,
            "source_pin": src_pin,
            "target_node_id": tgt_guid,
            "target_pin": tgt_pin,
            "_src_ref": _patch_extract_ref(conn.get("from", "")),
            "_tgt_ref": _patch_extract_ref(conn.get("to", "")),
        })

    results.setdefault("connection_results", [])
    if bulk_conns:
        resp = send_fn({
            "type": "connect_nodes_bulk",
            "blueprint_path": blueprint_path,
            "function_id": function_id,
            "connections": [{k: v for k, v in c.items() if not k.startswith("_")}
                            for c in bulk_conns],
        })
        if resp.get("success"):
            successful_count = resp.get("successful_connections", len(bulk_conns))
            results["connections_made"] = successful_count
            failed_set = {str(fc) for fc in resp.get("failed_connections", [])}
            for ci in bulk_conns:
                edge_key = f"{ci.get('_src_ref','')}.{ci['source_pin']}→{ci.get('_tgt_ref','')}.{ci['target_pin']}"
                failed_match = next((fc for fc in resp.get("failed_connections", [])
                                     if ci["source_pin"] in str(fc) or ci["target_pin"] in str(fc)), None)
                if failed_match:
                    err_msg = f"connect failed: {failed_match}"
                    results["errors"].append(err_msg)
                    patch_log["error_categories"].append(_patch_classify_error(str(failed_match)))
                    results["connection_results"].append({
                        "from": f"{ci.get('_src_ref','')}.{ci['source_pin']}",
                        "to": f"{ci.get('_tgt_ref','')}.{ci['target_pin']}",
                        "success": False, "error": str(failed_match), "method": "bulk",
                    })
                else:
                    results["connection_results"].append({
                        "from": f"{ci.get('_src_ref','')}.{ci['source_pin']}",
                        "to": f"{ci.get('_tgt_ref','')}.{ci['target_pin']}",
                        "success": True, "method": "bulk",
                    })
        else:
            raw_err = resp.get("error")
            err_msg = f"connect_bulk: {raw_err}"
            results["errors"].append(err_msg)
            patch_log["error_categories"].append(_patch_classify_error(err_msg))
            for ci in bulk_conns:
                results["connection_results"].append({
                    "from": f"{ci.get('_src_ref','')}.{ci['source_pin']}",
                    "to": f"{ci.get('_tgt_ref','')}.{ci['target_pin']}",
                    "success": False, "error": err_msg, "method": "bulk",
                })

            # ── Diagnostic probe ────────────────────────────────────────────
            diag_result = {"src_pins": None, "tgt_pins": None}
            if bulk_conns:
                probe = bulk_conns[0]
                diag_resp = send_fn({
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
            _node_not_found = (
                diag_result["src_pins"] == [] or
                diag_result["tgt_pins"] == [] or
                raw_err is None
            )
            if _node_not_found:
                def _ref_to_fname(ref: str) -> str:
                    if _patch_looks_like_guid(ref):
                        return ""
                    fname_guess = None
                    if "#" in ref:
                        node_type, _, idx = ref.partition("#")
                        fname_guess = f"{node_type}_{idx}"
                    elif "_" in ref:
                        fname_guess = ref
                    if fname_guess:
                        verify_resp = send_fn({
                            "type": "get_node_guid_by_fname",
                            "blueprint_path": blueprint_path,
                            "graph_id": function_id,
                            "node_fname": fname_guess,
                            "node_class_filter": "",
                        })
                        if isinstance(verify_resp, dict) and verify_resp.get("success"):
                            return fname_guess
                    return ""

                fname_successes = 0
                fname_errors = []
                for ci in bulk_conns:
                    src_fname = _ref_to_fname(ci.get("_src_ref", ""))
                    tgt_fname = _ref_to_fname(ci.get("_tgt_ref", ""))
                    if src_fname and tgt_fname:
                        fb = send_fn({
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

                # Record drift for all connections that needed FName fallback
                for ci in bulk_conns:
                    for role, ref_key, guid_key in [
                        ("src", "_src_ref", "source_node_id"),
                        ("tgt", "_tgt_ref", "target_node_id"),
                    ]:
                        orig_ref = ci.get(ref_key, "")
                        orig_guid = ci.get(guid_key, "")
                        fname_val = _ref_to_fname(orig_ref) if orig_ref else ""
                        if fname_val and orig_guid and not _patch_looks_like_guid(orig_ref):
                            new_guid = _fname_lookup(fname_val)
                            if new_guid and new_guid != orig_guid:
                                guid_drift_log.append({
                                    "fname": fname_val,
                                    "old_guid": orig_guid,
                                    "new_guid": new_guid,
                                    "reason": "FName-fallback after GUID miss",
                                    "phase": "add_connections",
                                })

                # Per-edge results for FName fallback (P0-5)
                fname_error_idx = 0
                for ci in bulk_conns:
                    src_fname = _ref_to_fname(ci.get("_src_ref", ""))
                    tgt_fname = _ref_to_fname(ci.get("_tgt_ref", ""))
                    if src_fname and tgt_fname:
                        edge_key = f"{src_fname}.{ci['source_pin']}→{tgt_fname}.{ci['target_pin']}"
                        fname_ok = fname_error_idx < len(fname_errors) and edge_key not in fname_errors[fname_error_idx] if fname_errors else True
                        edge_err = next((e for e in fname_errors
                                         if ci['source_pin'] in e and ci['target_pin'] in e), None)
                        results["connection_results"].append({
                            "from": f"{ci.get('_src_ref','')}.{ci['source_pin']}",
                            "to": f"{ci.get('_tgt_ref','')}.{ci['target_pin']}",
                            "success": edge_err is None,
                            "error": edge_err,
                            "method": "fname_fallback",
                        })
                    else:
                        results["connection_results"].append({
                            "from": f"{ci.get('_src_ref','')}.{ci['source_pin']}",
                            "to": f"{ci.get('_tgt_ref','')}.{ci['target_pin']}",
                            "success": False,
                            "error": f"cannot resolve FName for '{ci.get('_src_ref','')}' or '{ci.get('_tgt_ref','')}'",
                            "method": "fname_fallback",
                        })

                if fname_successes == len(bulk_conns):
                    results["errors"] = [e for e in results["errors"]
                                         if "connect_bulk" not in e]
                    results["connections_made"] = fname_successes
                    results["connection_method"] = "FName-fallback"
                    patch_log["error_categories"] = [c for c in patch_log["error_categories"]
                                                     if c != _patch_classify_error(err_msg)]
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
        try:
            node_guid = resolve_node_id(pv.get("node", ""), strict=True)
        except ValueError as e:
            results.setdefault("diagnostics", []).append({
                "severity": "error", "code": "REF_UNRESOLVABLE",
                "message": str(e), "location": {"phase": "set_pin_values", "node": pv.get("node", "")},
            })
            results["errors"].append(str(e))
            continue
        resp = send_fn({
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
            patch_log["error_categories"].append(_patch_classify_error(err_msg))
    patch_log["phases"]["set_pin_values"] = {
        "success": results["pin_values_set"] == len(patch.get("set_pin_values", [])) + len(
            [n for n in patch.get("add_nodes", []) if n.get("pin_values")]),
        "count": results["pin_values_set"],
    }

    # --- Phase 6: Auto-compile ---
    if patch.get("auto_compile", True):
        comp_resp = send_fn({
            "type": "compile_blueprint_with_errors",
            "blueprint_path": blueprint_path,
        })
        comp_ok = comp_resp.get("compiled", comp_resp.get("success", False))
        results["compiled"] = comp_ok
        compile_diags = []
        for e in comp_resp.get("errors", []):
            msg = e if isinstance(e, str) else e.get("message", str(e))
            compile_diags.append({"severity": "error", "code": "COMPILE_ERROR",
                                   "message": msg, "location": {"phase": "compile"}})
            results["errors"].append(f"compile_error: {msg}")
        for w in comp_resp.get("warnings", []):
            msg = w if isinstance(w, str) else w.get("message", str(w))
            compile_diags.append({"severity": "warning", "code": "COMPILE_WARNING",
                                   "message": msg, "location": {"phase": "compile"}})
        results.setdefault("diagnostics", []).extend(compile_diags)
        results["compile_diagnostics"] = compile_diags
        if not comp_ok:
            patch_log["error_categories"].append("COMPILE_ERROR")
        patch_log["phases"]["compile"] = {
            "success": comp_ok,
            "errors": len([d for d in compile_diags if d["severity"] == "error"]),
            "warnings": len([d for d in compile_diags if d["severity"] == "warning"]),
        }

    # Merge resolution warnings into diagnostics
    results.setdefault("diagnostics", []).extend(ctx["resolution_warnings"])

    results["success"] = len(results["errors"]) == 0

    return results


# ---------------------------------------------------------------------------
# Phase 4: _patch_finalize — commit/rollback, graph_state, cache, verify
# ---------------------------------------------------------------------------

def _patch_finalize(blueprint_path, function_id, results, patch_log, send_fn, do_verify):
    """Handle transaction commit/rollback, graph_state, cache eviction, and optional verification.

    Mutates results and patch_log in place. Returns results.
    """
    txn_started = results.pop("_txn_started", False)

    # --- Commit or rollback transaction ---
    if txn_started:
        if results["errors"]:
            cancel_resp = send_fn({"type": "cancel_transaction"})
            if cancel_resp.get("success"):
                results["rolled_back"] = True
                patch_log["rollback"] = True
            cleanup_resp = send_fn({
                "type": "remove_unused_nodes",
                "blueprint_path": blueprint_path,
            })
            results["cleanup_ran"] = cleanup_resp.get("success", False)
            # --- Ghost node detection (P0-4) ---
            created_guids = list(results.get("created_nodes", {}).values())
            ghost_nodes = []
            for guid in created_guids:
                if not guid:
                    continue
                verify_resp = send_fn({
                    "type": "get_node_details",
                    "blueprint_path": blueprint_path,
                    "node_guid": guid,
                })
                if isinstance(verify_resp, dict) and verify_resp.get("success"):
                    ghost_nodes.append({"guid": guid, "title": verify_resp.get("title", "")})
            if ghost_nodes:
                results["rollback_status"] = "partial"
                results["rollback_diagnostics"] = ghost_nodes
                results.setdefault("diagnostics", []).append({
                    "severity": "warning",
                    "code": "ROLLBACK_INCOMPLETE",
                    "message": f"{len(ghost_nodes)} node(s) survived rollback (UE5 known issue). Call describe_blueprint(force_refresh=True) before next patch.",
                    "location": {"phase": "rollback"},
                })
                patch_log["rollback_partial"] = True
                patch_log["ghost_nodes"] = ghost_nodes
            else:
                results["rollback_status"] = "complete"
            for ref_id, guid in list(results.get("created_nodes", {}).items()):
                is_ghost = any(g["guid"] == guid for g in ghost_nodes)
                results["created_nodes"][ref_id] = {
                    "guid": guid,
                    "status": "ghost" if is_ghost else "rolled_back",
                }
        else:
            send_fn({"type": "end_transaction"})
            results["committed"] = True

    # --- Finalize structured patch log ---
    patch_log["total_errors"] = len(results["errors"])
    patch_log["success"] = results["success"]
    patch_log["error_categories"] = list(dict.fromkeys(patch_log["error_categories"]))
    print(f"[PATCH_LOG] {json.dumps(patch_log, separators=(',', ':'), ensure_ascii=False)}",
          file=sys.stderr)

    # --- graph_state ---
    def _get_node_count_lightweight() -> int:
        try:
            raw = send_fn({
                "type": "list_graphs",
                "blueprint_path": blueprint_path,
            })
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

    # Invalidate all caches for this blueprint
    _evict_bp_caches(blueprint_path)

    # --- Optional inline verification ---
    if do_verify and results["created_nodes"]:
        affected_guids = ",".join(results["created_nodes"].values())
        list_resp = send_fn({"type": "list_graphs", "blueprint_path": blueprint_path})
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
                    blueprint_path, target_graphs[0], "standard", False, send_fn,
                    node_ids=affected_guids)
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

    return results
