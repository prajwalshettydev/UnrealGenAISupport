"""Response envelope, caching, and compact JSON helpers.

Extracted from mcp_server.py — this is the single implementation.
mcp_server.py re-imports these symbols.
"""
import json
import sys
import time
from collections import OrderedDict


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
                   job_id: str | None = None,
                   error_code: str | None = None,
                   message: str | None = None,
                   per_operation_results: list | None = None,
                   rollback_status: str | None = None,
                   timing_ms: int | None = None) -> dict:
    """Build the unified response envelope for MCP tool responses.

    Schema v2 (additive migration — old keys preserved as deprecated aliases):
      success / ok      : bool   (ok is deprecated alias for success)
      data    / result  : dict   (result is deprecated alias for data)
      diagnostics[]     : absorbs warnings[] entries with severity:"warning"
      warnings[]        : deprecated alias, preserved for backward compat
      per_operation_results[]: per-item results for batch operations
      rollback_status   : "complete" | "partial" | null
      error_code        : top-level error code when success=false
      message           : top-level error message when success=false
      timing_ms         : wall-clock time for the operation
    """
    import uuid
    # Merge warnings into diagnostics as severity:"warning" entries
    merged_diags = list(diagnostics or [])
    for w in (warnings or []):
        if isinstance(w, str):
            merged_diags.append({"severity": "warning", "code": "WARNING", "message": w})
        elif isinstance(w, dict) and "severity" not in w:
            merged_diags.append({"severity": "warning", **w})
        else:
            merged_diags.append(w)
    return {
        # v2 canonical fields
        "success": ok,
        "summary": summary or ("OK" if ok else (result.get("error", "") if result else "failed")),
        "data": result,
        "diagnostics": merged_diags,
        "per_operation_results": per_operation_results or [],
        "rollback_status": rollback_status,
        "timing_ms": timing_ms,
        "error_code": error_code,
        "message": message,
        "request_id": str(uuid.uuid4())[:8],
        "tool": tool_name,
        "job_id": job_id,
        # deprecated aliases (backward compat — remove in Phase 2)
        "ok": ok,
        "result": result,
        "warnings": warnings or [],
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


def _evict_bp_caches(blueprint_path: str):
    """Invalidate all cached data for a blueprint path."""
    _describe_cache.invalidate(blueprint_path)
    keys_to_remove = [k for k in _node_details_cache if k[0] == blueprint_path]
    for k in keys_to_remove:
        del _node_details_cache[k]


def _node_details_cache_evict(blueprint_path: str):
    """Alias — just calls _evict_bp_caches."""
    _evict_bp_caches(blueprint_path)


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
    """Content-level fingerprint using node GUID set hash when available.

    Strategy (in priority order):
    1. sha256(graph_name + sorted(node_guids))[:16] — detects add/remove/rewire
    2. Fallback: graph_name:node_count — used when UE side doesn't return node_guids

    Pass `graphs` (already-fetched list) to avoid an extra socket round-trip.
    Pass `send_fn` to fetch graphs on demand (used for pre-describe cache check).
    """
    import hashlib
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

    parts = []
    fingerprint_strategy = "guid_hash"
    for g in sorted(graphs, key=lambda x: x.get("graph_name", "")):
        gname = g.get("graph_name", "?")
        node_guids = g.get("node_guids", [])
        if node_guids:
            # Content-level hash: sensitive to add/remove/any structural change
            sig = hashlib.sha256(
                (gname + "|" + ",".join(sorted(node_guids))).encode("utf-8")
            ).hexdigest()[:16]
        else:
            # Fallback: node_count only (weaker, but backwards-compatible)
            fingerprint_strategy = "node_count_fallback"
            sig = f"{gname}:{g.get('node_count', '?')}"
        parts.append(sig)

    fingerprint = "|".join(parts)
    # Embed strategy as suffix so callers can detect fallback mode
    return f"{fingerprint}|strategy:{fingerprint_strategy}"


# ---------------------------------------------------------------------------
# P2-T5: Locale Pin Alias Dictionary
# Maps Chinese (and other locale) pin display names to stable internal PinNames.
# Used in resolve_endpoint() to normalize pin names before graph lookup.
# ---------------------------------------------------------------------------
_PIN_LOCALE_ALIASES: dict = {
    # Exec flow pins
    "条件": "Condition",
    "执行": "execute",
    "然后": "then",
    "否则": "else",
    "完成": "Completed",
    "循环体": "LoopBody",
    "已完成": "Completed",
    # Common data pins
    "目标": "Target",
    "返回值": "ReturnValue",
    "持续时间": "Duration",
    "对象": "Object",
    "类": "Class",
    "输入": "Input",
    "输出": "Output",
    "结果": "Result",
    "值": "Value",
    "索引": "Index",
    "数组": "Array",
    "布尔": "Boolean",
    "整数": "Integer",
    "浮点": "Float",
    "字符串": "String",
    "向量": "Vector",
    "旋转体": "Rotator",
    "变换": "Transform",
}


# Locale aliases used by patch resolution
_LOCALE_ALIASES = {
    "分支": "K2Node_IfThenElse",
    "打印字符串": "CallFunction:KismetSystemLibrary.PrintString",
    "事件开始运行": "EventBeginPlay",
    "事件Tick": "EventTick",
    "序列": "K2Node_ExecutionSequence",
    "延迟": "CallFunction:KismetSystemLibrary.Delay",
    "设置Actor位置": "CallFunction:Actor.K2_SetActorLocation",
    "获取Actor位置": "CallFunction:Actor.K2_GetActorLocation",
    "获取玩家控制器": "CallFunction:GameplayStatics.GetPlayerController",
    "获取玩家角色": "CallFunction:GameplayStatics.GetPlayerCharacter",
    "是否有效": "CallFunction:KismetSystemLibrary.IsValid",
    "摧毁Actor": "CallFunction:Actor.K2_DestroyActor",
    "生成Actor": "CallFunction:GameplayStatics.BeginDeferredActorSpawnFromClass",
    "For Each循环": "CallFunction:KismetArrayLibrary.Array_ForEachLoop",
    "创建": "K2Node_ConstructObjectFromClass",
    "转换": "K2Node_DynamicCast",
    "自定义事件": "K2Node_CustomEvent",
}
