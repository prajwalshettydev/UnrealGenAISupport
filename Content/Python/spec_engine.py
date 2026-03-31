"""Blueprint Spec Engine — P1: Declarative Blueprint Spec Language

Converts a Blueprint Spec (JSON) into a sequence of concrete MCP operations,
then executes them atomically via the existing preflight → apply → compile pipeline.

Public API
----------
validate_spec(spec)         -> list[str]   (errors; empty = valid)
lower_spec(spec)            -> SpecPlan    (ops to execute, no side effects)
execute_spec(spec, send_fn, dry_run=False) -> SpecResult

Spec v1 Schema (all fields optional except 'blueprint'):
{
  "blueprint": "/Game/Blueprints/BP_Enemy",   # existing OR path for new BP
  "parent": "Character",                       # only used when create_if_missing=True
  "create_if_missing": true,
  "variables": [
    {"name": "Health", "type": "float", "default": 100, "category": "Stats"}
  ],
  "components": [
    {"name": "HealthBarWidget", "class": "WidgetComponent"}
  ],
  "custom_events": [
    {"name": "OnTakeDamage", "inputs": [{"name": "Damage", "type": "float"}]}
  ],
  "functions": [
    {
      "name": "CalculateDamage",
      "inputs": [{"name": "Raw", "type": "float"}],
      "outputs": [{"name": "Final", "type": "float"}]
    }
  ],
  "graphs": {
    "EventGraph": {
      "nodes": [
        {"ref_id": "begin", "node_type": "EventBeginPlay"},
        {
          "ref_id": "print",
          "node_type": "K2Node_CallFunction",
          "function_name": "PrintString",
          "pin_values": {"InString": "Ready"}
        }
      ],
      "edges": [
        {"from": "begin.then", "to": "print.execute"}
      ]
    }
  },
  "compile": true,
  "save": true,
  "auto_layout": true
}
"""

from __future__ import annotations

import json
import time
from dataclasses import dataclass, field
from typing import Any, Callable

__all__ = ["validate_spec", "lower_spec", "execute_spec", "SpecPlan", "SpecResult"]

# ---------------------------------------------------------------------------
# Data types
# ---------------------------------------------------------------------------

@dataclass
class SpecOp:
    """One atomic unit of work produced by the lowerer."""
    kind: str          # "create_bp" | "add_variable" | "add_component" | "add_function" |
                       # "add_custom_event" | "patch_graph" | "compile" | "save" | "auto_layout"
    payload: dict      # Arguments passed to the socket command or internal helper
    description: str   # Human-readable label for logging / dry-run output


@dataclass
class SpecPlan:
    """Output of lower_spec() — a linearised sequence of operations."""
    blueprint_path: str
    ops: list[SpecOp] = field(default_factory=list)
    errors: list[str] = field(default_factory=list)  # validation errors found during lowering


@dataclass
class OpResult:
    kind: str
    description: str
    success: bool
    output: Any = None
    error: str | None = None
    duration_ms: float = 0.0


@dataclass
class SpecResult:
    blueprint_path: str
    success: bool
    dry_run: bool
    ops: list[OpResult] = field(default_factory=list)
    compile_errors: list[str] = field(default_factory=list)

    def summary(self) -> str:
        ok = sum(1 for o in self.ops if o.success)
        fail = sum(1 for o in self.ops if not o.success)
        label = "[DRY RUN] " if self.dry_run else ""
        return (f"{label}apply_blueprint_spec: {ok} ok, {fail} failed — "
                f"{'PASS' if self.success else 'FAIL'}")


# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

_VALID_VAR_TYPES = {
    "bool", "boolean", "byte", "int", "int32", "int64", "float", "double",
    "string", "name", "text", "vector", "rotator", "transform", "color",
    "lineartrace", "object", "class", "softobject", "softclass",
}

def validate_spec(spec: dict) -> list[str]:
    """Return a list of validation errors. Empty list = valid."""
    errors: list[str] = []

    if not isinstance(spec, dict):
        return ["spec must be a JSON object"]

    bp = spec.get("blueprint", "")
    if not bp:
        errors.append("'blueprint' path is required")
    elif not bp.startswith("/"):
        errors.append(f"'blueprint' must be an absolute path starting with '/' (got: {bp!r})")

    # Variables
    for i, v in enumerate(spec.get("variables", [])):
        if not v.get("name"):
            errors.append(f"variables[{i}]: 'name' is required")
        vtype = v.get("type", "").lower().split(":")[0]
        if not vtype:
            errors.append(f"variables[{i}]: 'type' is required")

    # Components
    for i, c in enumerate(spec.get("components", [])):
        if not c.get("name"):
            errors.append(f"components[{i}]: 'name' is required")
        if not c.get("class"):
            errors.append(f"components[{i}]: 'class' is required")

    # Custom events
    for i, e in enumerate(spec.get("custom_events", [])):
        if not e.get("name"):
            errors.append(f"custom_events[{i}]: 'name' is required")

    # Functions
    for i, f_ in enumerate(spec.get("functions", [])):
        if not f_.get("name"):
            errors.append(f"functions[{i}]: 'name' is required")

    # Graphs
    for graph_name, graph in spec.get("graphs", {}).items():
        if not isinstance(graph, dict):
            errors.append(f"graphs.{graph_name}: must be an object")
            continue
        ref_ids: set[str] = set()
        for j, node in enumerate(graph.get("nodes", [])):
            if not node.get("ref_id"):
                errors.append(f"graphs.{graph_name}.nodes[{j}]: 'ref_id' is required")
            else:
                rid = node["ref_id"]
                if rid in ref_ids:
                    errors.append(f"graphs.{graph_name}: duplicate ref_id '{rid}'")
                ref_ids.add(rid)
            if not node.get("node_type"):
                errors.append(f"graphs.{graph_name}.nodes[{j}]: 'node_type' is required")

    return errors


# ---------------------------------------------------------------------------
# Lowerer  (spec → SpecPlan, pure — no side effects)
# ---------------------------------------------------------------------------

def lower_spec(spec: dict) -> SpecPlan:
    """Convert a spec dict into a linearised SpecPlan.

    Phase order:
      1. create_bp (if create_if_missing)
      2. add_variable (one per variable)
      3. add_component (one per component)
      4. add_custom_event (one per custom_event)
      5. add_function (one per function)
      6. patch_graph (one per graph — all nodes+edges in a single patch)
      7. compile (if spec.compile)
      8. auto_layout (if spec.auto_layout, per graph)
      9. save (if spec.save)
    """
    errors = validate_spec(spec)
    bp_path = spec.get("blueprint", "")
    plan = SpecPlan(blueprint_path=bp_path, errors=errors)

    if errors:
        return plan  # Don't lower an invalid spec

    # 1. Create BP
    if spec.get("create_if_missing", False):
        # Derive name and save_path from the blueprint path
        parts = bp_path.rsplit("/", 1)
        save_path = parts[0] if len(parts) == 2 else "/Game/Blueprints"
        bp_name = parts[-1].lstrip("BP_").replace("BP_", "") if parts[-1].startswith("BP_") else parts[-1]
        # Use the full last segment as the name
        bp_name = parts[-1]
        plan.ops.append(SpecOp(
            kind="create_bp",
            payload={
                "type": "create_blueprint",
                "blueprint_name": bp_name,
                "parent_class": spec.get("parent", "Actor"),
                "save_path": save_path,
                "expected_path": bp_path,
            },
            description=f"Create Blueprint '{bp_name}' (parent: {spec.get('parent', 'Actor')})",
        ))

    # 2. Variables
    for v in spec.get("variables", []):
        plan.ops.append(SpecOp(
            kind="add_variable",
            payload={
                "type": "add_variable",
                "blueprint_path": bp_path,
                "variable_name": v["name"],
                "variable_type": v.get("type", "float"),
                "default_value": str(v["default"]) if "default" in v else None,
                "category": v.get("category", "Default"),
            },
            description=f"Add variable '{v['name']}' ({v.get('type', 'float')})",
        ))

    # 3. Components
    for c in spec.get("components", []):
        plan.ops.append(SpecOp(
            kind="add_component",
            payload={
                "type": "add_component",
                "blueprint_path": bp_path,
                "component_class": c["class"],
                "component_name": c.get("name"),
            },
            description=f"Add component '{c.get('name')}' ({c['class']})",
        ))

    # 4. Custom events
    for e in spec.get("custom_events", []):
        # Custom events are added as functions with EventGraph wiring;
        # we use add_function with a special marker so the executor knows
        # to create a CustomEvent node instead of a regular function graph.
        plan.ops.append(SpecOp(
            kind="add_custom_event",
            payload={
                "type": "add_function",
                "blueprint_path": bp_path,
                "function_name": e["name"],
                "inputs": e.get("inputs", []),
                "outputs": [],
                "_is_custom_event": True,
            },
            description=f"Add custom event '{e['name']}'",
        ))

    # 5. Functions
    for f_ in spec.get("functions", []):
        plan.ops.append(SpecOp(
            kind="add_function",
            payload={
                "type": "add_function",
                "blueprint_path": bp_path,
                "function_name": f_["name"],
                "inputs": f_.get("inputs", []),
                "outputs": f_.get("outputs", []),
            },
            description=f"Add function '{f_['name']}'",
        ))

    # 6. Graph patches (one apply_blueprint_patch per graph)
    for graph_name, graph in spec.get("graphs", {}).items():
        nodes = graph.get("nodes", [])
        edges = graph.get("edges", [])
        if not nodes and not edges:
            continue

        # Build the patch JSON that apply_blueprint_patch expects
        add_nodes = []
        for node in nodes:
            n: dict[str, Any] = {
                "ref_id": node["ref_id"],
                "node_type": node["node_type"],
            }
            if "function_name" in node:
                n["function_name"] = node["function_name"]
            if "pin_values" in node:
                n["pin_values"] = node["pin_values"]
            if "position" in node:
                n["position"] = node["position"]
            add_nodes.append(n)

        patch = {
            "add_nodes": add_nodes,
            "add_connections": [
                {"from": e["from"], "to": e["to"]}
                for e in edges
            ],
            "auto_compile": False,  # compile step is explicit below
        }

        if "layout_intent" in graph:
            patch["layout_intent"] = graph["layout_intent"]

        plan.ops.append(SpecOp(
            kind="patch_graph",
            payload={
                "_graph_name": graph_name,
                "_blueprint_path": bp_path,
                "_patch": patch,
            },
            description=f"Patch graph '{graph_name}' ({len(nodes)} nodes, {len(edges)} edges)",
        ))

    # 7. Compile
    if spec.get("compile", True):
        plan.ops.append(SpecOp(
            kind="compile",
            payload={"type": "compile_blueprint", "blueprint_path": bp_path},
            description=f"Compile {bp_path}",
        ))

    # 8. Auto layout (per graph, after compile so node positions are stable)
    if spec.get("auto_layout", False):
        for graph_name in spec.get("graphs", {}):
            plan.ops.append(SpecOp(
                kind="auto_layout",
                payload={
                    "_blueprint_path": bp_path,
                    "_graph_name": graph_name,
                },
                description=f"Auto-layout graph '{graph_name}'",
            ))

    # 9. Save
    if spec.get("save", True):
        plan.ops.append(SpecOp(
            kind="save",
            payload={"type": "save_all_dirty_packages"},
            description="Save all dirty packages",
        ))

    return plan


# ---------------------------------------------------------------------------
# Executor
# ---------------------------------------------------------------------------

def execute_spec(
    spec: dict | str,
    send_fn: Callable[[dict], dict],
    dry_run: bool = False,
) -> SpecResult:
    """Execute a Blueprint spec end-to-end.

    Args:
        spec:     spec dict (or JSON string)
        send_fn:  callable that sends a command to UE and returns the response dict
        dry_run:  if True, validate and lower but do not call send_fn

    Returns:
        SpecResult with per-op outcomes and a top-level success flag.
    """
    if isinstance(spec, str):
        try:
            spec = json.loads(spec)
        except json.JSONDecodeError as e:
            return SpecResult(
                blueprint_path="",
                success=False,
                dry_run=dry_run,
                ops=[OpResult("parse", "Parse spec JSON", False, error=str(e))],
            )

    plan = lower_spec(spec)
    result = SpecResult(
        blueprint_path=plan.blueprint_path,
        success=False,
        dry_run=dry_run,
    )

    # Validation errors
    if plan.errors:
        for err in plan.errors:
            result.ops.append(OpResult("validate", "Validate spec", False, error=err))
        return result

    if dry_run:
        # Return the plan without executing
        for op in plan.ops:
            result.ops.append(OpResult(
                kind=op.kind,
                description=op.description,
                success=True,
                output={"dry_run": True, "payload": op.payload},
            ))
        result.success = True
        return result

    # --------------- Live execution ---------------
    # Track whether the BP path was created (create_if_missing may set it)
    actual_bp_path = plan.blueprint_path

    # Build graph_name → function_id cache (populated when we learn graph GUIDs)
    _graph_id_cache: dict[str, str] = {}

    def _resolve_graph_id(graph_name: str) -> str | None:
        """Return the function_id for a graph, fetching from UE if needed."""
        if graph_name in _graph_id_cache:
            return _graph_id_cache[graph_name]
        resp = send_fn({"type": "list_graphs", "blueprint_path": actual_bp_path})
        if resp.get("success"):
            graphs = resp.get("graphs", [])
            if isinstance(graphs, str):
                graphs = json.loads(graphs)
            for g in graphs:
                name = g.get("graph_name", "")
                guid = g.get("graph_guid", name)
                _graph_id_cache[name] = guid
        return _graph_id_cache.get(graph_name)

    for op in plan.ops:
        t0 = time.perf_counter()
        op_result = OpResult(kind=op.kind, description=op.description, success=False)

        try:
            if op.kind == "create_bp":
                resp = send_fn(op.payload)
                ok = resp.get("success", False)
                if ok:
                    # Update actual path in case UE returned a different path
                    returned_path = resp.get("blueprint_path", actual_bp_path)
                    if returned_path:
                        actual_bp_path = returned_path
                        _graph_id_cache.clear()  # stale after create
                op_result.success = ok
                op_result.output = resp
                if not ok:
                    op_result.error = resp.get("error", "create_bp failed")

            elif op.kind in ("add_variable", "add_component", "add_function",
                             "add_custom_event"):
                payload = dict(op.payload)
                payload["blueprint_path"] = actual_bp_path
                payload.pop("_is_custom_event", None)  # internal marker, not sent to UE
                resp = send_fn(payload)
                ok = resp.get("success", False)
                op_result.success = ok
                op_result.output = resp
                if not ok:
                    op_result.error = resp.get("error", f"{op.kind} failed")

            elif op.kind == "patch_graph":
                graph_name = op.payload["_graph_name"]
                patch = op.payload["_patch"]

                # Step A: Get function_id for this graph
                function_id = _resolve_graph_id(graph_name)
                if function_id is None:
                    op_result.error = f"Graph '{graph_name}' not found in {actual_bp_path}"
                    op_result.success = False
                    result.ops.append(_finalize(op_result, t0))
                    continue

                # Step B: Preflight (dry-run validation)
                preflight_resp = send_fn({
                    "type": "preflight_blueprint_patch",
                    "blueprint_path": actual_bp_path,
                    "function_id": function_id,
                    "patch_json": json.dumps(patch),
                })
                if preflight_resp.get("success"):
                    pf_result = preflight_resp.get("result", {})
                    if isinstance(pf_result, str):
                        pf_result = json.loads(pf_result)
                    if not pf_result.get("valid", True):
                        issues = pf_result.get("issues", [])
                        op_result.error = f"Preflight failed: {issues[:3]}"
                        op_result.success = False
                        op_result.output = pf_result
                        result.ops.append(_finalize(op_result, t0))
                        continue

                # Step C: Apply patch (with transaction)
                apply_resp = send_fn({
                    "type": "apply_blueprint_patch",
                    "blueprint_path": actual_bp_path,
                    "function_id": function_id,
                    "patch_json": json.dumps(patch),
                })
                ok = apply_resp.get("success", False)
                op_result.success = ok
                op_result.output = apply_resp
                if not ok:
                    op_result.error = apply_resp.get("error", "patch_graph failed")
                else:
                    # Refresh graph id cache after structural changes
                    _graph_id_cache.clear()

            elif op.kind == "compile":
                resp = send_fn({"type": "compile_blueprint",
                                "blueprint_path": actual_bp_path})
                ok = resp.get("success", False)
                compile_errors = resp.get("errors", [])
                if compile_errors:
                    result.compile_errors.extend(compile_errors)
                op_result.success = ok
                op_result.output = resp
                if not ok:
                    op_result.error = resp.get("error", f"Compile failed: {compile_errors[:3]}")

            elif op.kind == "auto_layout":
                graph_name = op.payload["_graph_name"]
                function_id = _resolve_graph_id(graph_name)
                if function_id:
                    resp = send_fn({
                        "type": "auto_layout_graph",
                        "blueprint_path": actual_bp_path,
                        "function_id": function_id,
                    })
                    op_result.success = resp.get("success", False)
                    op_result.output = resp
                else:
                    op_result.success = True  # non-fatal: skip layout if graph not found
                    op_result.output = {"skipped": "graph not found"}

            elif op.kind == "save":
                resp = send_fn({"type": "save_all_dirty_packages"})
                ok = resp.get("success", False)
                op_result.success = ok
                op_result.output = resp
                if not ok:
                    op_result.error = resp.get("error", "save failed")

            else:
                op_result.success = False
                op_result.error = f"Unknown op kind: {op.kind}"

        except Exception as exc:
            op_result.success = False
            op_result.error = str(exc)

        result.ops.append(_finalize(op_result, t0))

    # Overall success = all ops succeeded (save is non-fatal)
    fatal_kinds = {k for k in ("create_bp", "add_variable", "add_component",
                               "add_function", "add_custom_event", "patch_graph", "compile")}
    result.success = all(
        op.success for op in result.ops
        if op.kind in fatal_kinds
    )
    return result


def _finalize(op: OpResult, t0: float) -> OpResult:
    op.duration_ms = (time.perf_counter() - t0) * 1000
    return op
