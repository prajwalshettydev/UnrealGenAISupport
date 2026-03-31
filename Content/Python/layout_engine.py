"""Blueprint Auto-Layout Engine

Python-only rules engine for placing new Blueprint nodes without overlapping
existing ones.  No LLM involvement — geometry is deterministic and reproducible.

Public API
----------
compute_patch_positions(patch, existing_nodes, anchor_pos=None) -> dict[str, list]
extract_layout_intent(patch, title_to_guid, guid_to_pos) -> dict
find_free_slot(anchor_pos, occupied, role, placement_mode) -> list[float]
build_occupancy(existing_xs, existing_ys) -> set[tuple[int,int]]
classify_node_role(node_spec, patch) -> str
"""

from __future__ import annotations

__all__ = [
    "compute_patch_positions",
    "extract_layout_intent",
    "find_free_slot",
    "build_occupancy",
    "classify_node_role",
]

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

GRID_W: float = 400.0   # horizontal cell width
GRID_H: float = 250.0   # vertical cell height
AUTO_PAD_X: float = 500.0  # min safe distance to the right of existing nodes
AUTO_PAD_Y: float = 250.0  # vertical spacing between same-role nodes

# ---------------------------------------------------------------------------
# Occupancy grid
# ---------------------------------------------------------------------------

def build_occupancy(
    existing_xs: list[float],
    existing_ys: list[float],
) -> set[tuple[int, int]]:
    """Map existing node positions to a coarse grid.  O(N)."""
    occupied: set[tuple[int, int]] = set()
    for x, y in zip(existing_xs, existing_ys):
        occupied.add((round(x / GRID_W), round(y / GRID_H)))
    return occupied


def _ring_candidates(
    gx: int, gy: int, radius: int, bias: str
) -> list[tuple[int, int]]:
    """All grid cells at Manhattan distance == radius, sorted by placement bias."""
    cells = [
        (gx + dx, gy + dy)
        for dx in range(-radius, radius + 1)
        for dy in range(-radius, radius + 1)
        if abs(dx) + abs(dy) == radius
    ]
    if bias == "append_right":
        cells.sort(key=lambda c: (-c[0], abs(c[1] - gy)))
    elif bias == "insert_between":
        cells.sort(key=lambda c: (abs(c[0] - gx), abs(c[1] - gy)))
    elif bias == "side_branch":
        cells.sort(key=lambda c: (abs(c[1] - gy), -c[0]))
    return cells


def find_free_slot(
    anchor_pos: list[float],
    occupied: set[tuple[int, int]],
    role: str = "exec_main",
    placement_mode: str = "append_right",
) -> list[float]:
    """Spiral-search from anchor for the nearest unoccupied grid cell.

    Marks the chosen cell as occupied so subsequent calls in the same patch
    don't collide with each other.

    Falls back to a safe position far to the right if nothing found in 20 rings.
    """
    ax = round(anchor_pos[0] / GRID_W)
    ay = round(anchor_pos[1] / GRID_H)

    # Role-based starting offset
    if role in ("exec_main", "debug_print", "cast_or_adapter"):
        ax += 1
    elif role == "branch_side":
        ay += 1

    for r in range(0, 20):
        for gx, gy in _ring_candidates(ax, ay, r, placement_mode):
            if (gx, gy) not in occupied:
                occupied.add((gx, gy))
                return [float(gx * GRID_W), float(gy * GRID_H)]

    # Fallback
    safe_gx = (max(c[0] for c in occupied) + 5) if occupied else 5
    return [float(safe_gx * GRID_W), anchor_pos[1]]


# ---------------------------------------------------------------------------
# Node role classification
# ---------------------------------------------------------------------------

_ROLE_KEYWORDS: dict[str, set[str]] = {
    "debug_print":    {"PrintString", "PrintText", "Print"},
    "cast_or_adapter":{"CastTo", "MakeStruct", "BreakStruct", "MakeArray"},
    "branch_side":    {"K2Node_IfThenElse"},
}


def classify_node_role(node_spec: dict, patch: dict) -> str:
    """Rule-based node role classification.  No LLM required.

    Returns one of: exec_main | data_helper | debug_print | cast_or_adapter | branch_side
    """
    nt = node_spec.get("node_type", "")
    fn = node_spec.get("function_name", "")
    ref_id = node_spec.get("ref_id", "")

    for role, kws in _ROLE_KEYWORDS.items():
        # Use startswith to avoid false positives from substring matches
        # (e.g. "CastTo" in "UncalledTo" would wrongly match with plain `in`)
        if any(fn == k or fn.startswith(k) or nt == k or nt.startswith(k) for k in kws):
            return role

    # Has exec connection in this patch → mainline
    for conn in patch.get("add_connections", []):
        for ep in (conn.get("from", ""), conn.get("to", "")):
            if ref_id and ref_id in ep:
                return "exec_main"

    return "data_helper"


# ---------------------------------------------------------------------------
# Anchor extraction
# ---------------------------------------------------------------------------

def extract_layout_intent(
    patch: dict,
    title_to_guid: dict,
    guid_to_pos: dict,
) -> dict:
    """Derive layout intent (anchor, placement_mode) purely from patch structure.

    Returns:
        {
          "anchor_guid": str | None,
          "anchor_pos": [x, y] | None,
          "placement_mode": "append_right" | "insert_between" | "side_branch",
        }
    No LLM needed — caller may override with an explicit layout_intent in the patch.
    """
    result: dict = {
        "anchor_guid": None,
        "anchor_pos": None,
        "placement_mode": "append_right",
    }

    # Honour explicit field in patch
    explicit_intent = patch.get("layout_intent", {})
    result["placement_mode"] = explicit_intent.get("placement_mode", "append_right")

    explicit_ref = explicit_intent.get("anchor_guid") or explicit_intent.get("anchor_ref")
    if explicit_ref:
        pos = guid_to_pos.get(explicit_ref) or guid_to_pos.get(
            title_to_guid.get(explicit_ref, "")
        )
        if pos:
            result["anchor_guid"] = explicit_ref
            result["anchor_pos"] = list(pos)
            return result

    # Infer from add_connections — first endpoint that resolves to an existing node
    for conn in patch.get("add_connections", []):
        for endpoint in (conn.get("from", ""), conn.get("to", "")):
            node_ref = (
                endpoint.split("::", 1)[0]
                if "::" in endpoint
                else endpoint.rsplit(".", 1)[0]
            )
            guid = title_to_guid.get(node_ref) or (
                node_ref if node_ref in guid_to_pos else None
            )
            if guid and guid in guid_to_pos:
                result["anchor_guid"] = guid
                result["anchor_pos"] = list(guid_to_pos[guid])
                return result

    return result


# ---------------------------------------------------------------------------
# Main public entry point
# ---------------------------------------------------------------------------

def compute_patch_positions(
    patch: dict,
    existing_nodes: list[dict],
    anchor_pos: list[float] | None = None,
) -> dict[str, list[float]]:
    """Compute non-overlapping [x, y] positions for every new node in the patch.

    Args:
        patch:          apply_blueprint_patch JSON (has "add_nodes", "add_connections", …)
        existing_nodes: list of {guid, position: [x,y], ...} for existing graph nodes
        anchor_pos:     optional override anchor position (e.g. from extract_layout_intent)

    Returns:
        {ref_id: [x, y]} for every entry in patch["add_nodes"] that has no explicit position.
        Entries with an explicit "position" field are passed through unchanged.
    """
    xs = [float(n.get("position", [0])[0]) for n in existing_nodes if n.get("position")]
    ys = [float(n.get("position", [0, 0])[1]) for n in existing_nodes if n.get("position")]

    occupied = build_occupancy(xs, ys)

    max_x = max(xs) if xs else 0.0
    sorted_ys = sorted(ys)
    median_y = sorted_ys[len(sorted_ys) // 2] if sorted_ys else 0.0

    if anchor_pos is not None:
        base_x, base_y = anchor_pos[0], anchor_pos[1]
    else:
        base_x, base_y = max_x, median_y

    result: dict[str, list[float]] = {}
    intent = patch.get("layout_intent", {})
    placement_mode = intent.get("placement_mode", "append_right")

    # Pre-build upstream map once: {target_ref: source_ref} — O(M) then O(1) per node.
    # Uses explicit from→to direction (not bidirectional), matching mcp_server behaviour.
    _upstream_map: dict[str, str] = {}
    for conn in patch.get("add_connections", []):
        src_ep = conn.get("from", "")
        tgt_ep = conn.get("to", "")
        src_ref = src_ep.split("::", 1)[0] if "::" in src_ep else src_ep.rsplit(".", 1)[0]
        tgt_ref = tgt_ep.split("::", 1)[0] if "::" in tgt_ep else tgt_ep.rsplit(".", 1)[0]
        if src_ref and tgt_ref:
            _upstream_map.setdefault(tgt_ref, src_ref)  # first source wins

    for node_spec in patch.get("add_nodes", []):
        ref_id = node_spec.get("ref_id", "")
        if "position" in node_spec:
            result[ref_id] = list(node_spec["position"])
            gx = round(node_spec["position"][0] / GRID_W)
            gy = round(node_spec["position"][1] / GRID_H)
            occupied.add((gx, gy))
            continue

        role = classify_node_role(node_spec, patch)

        # Upstream anchor: O(1) lookup via pre-built map
        src_ref = _upstream_map.get(ref_id)
        local_anchor = result.get(src_ref) if src_ref else None
        anchor = local_anchor if local_anchor is not None else (
            anchor_pos if anchor_pos is not None else [base_x, base_y]
        )
        pos = find_free_slot(anchor, occupied, role, placement_mode)
        result[ref_id] = pos

    return result
