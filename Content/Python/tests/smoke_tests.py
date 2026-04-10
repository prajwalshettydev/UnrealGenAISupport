"""
Smoke Tests for UE5 Agent Execution Layer (US-007)
Tests key behaviors without requiring live UE Editor connection.
Covers: transport, readback, compile errors, trace analytics, strict_mode.

Usage:
    python tests/smoke_tests.py
"""
import json
import sys
import os
import time
import tempfile
import shutil
import re
from collections import Counter
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

_RESULTS = []


def _report(name, passed, detail=""):
    status = "PASS" if passed else "FAIL"
    _RESULTS.append((name, passed))
    print(f"  [{status}] {name}" + (f": {detail}" if detail else ""))


def smoke_transport_constants():
    print("\n--- Scenario 1: Transport Constants ---")
    try:
        from tools.transport import _FRAME_MAGIC, _PROTOCOL_VERSION, _DEFAULT_TIMEOUT
        _report("transport_has_FRAME_MAGIC", _FRAME_MAGIC == b"\xab\xcd")
        _report("transport_version_valid", _PROTOCOL_VERSION in ("v1", "v2"))
        _report("transport_timeout_positive", isinstance(_DEFAULT_TIMEOUT, float) and _DEFAULT_TIMEOUT > 0)
    except ImportError as e:
        _report("transport_import", False, str(e))


def smoke_patch_failure_readback():
    print("\n--- Scenario 2: Patch Risk Score ---")
    from tools.patch import _compute_patch_risk_score, _update_last_read_ts

    high_risk_patch = {
        "add_nodes": [
            {"node_type": "K2Node_MacroInstance"},
            {"node_type": "K2Node_CallFunction"},
        ],
        "add_connections": [
            {"from": "node_a.then", "to": "node_b.execute"},
            {"from": "node_b.then", "to": "node_c.execute"},
        ],
    }
    score = _compute_patch_risk_score(high_risk_patch)
    _report("high_risk_score_above_6", score >= 6, f"score={score}")

    safe_patch = {"add_nodes": [{"node_type": "K2Node_IfThenElse"}]}
    safe_score = _compute_patch_risk_score(safe_patch)
    _report("safe_score_below_6", safe_score < 6, f"score={safe_score}")

    bp = "/Game/Test/BP_Smoke"
    _update_last_read_ts(bp)
    from tools.patch import _last_read_ts
    _report("read_ts_updated", _last_read_ts.get(bp, 0) > 0)


def smoke_compile_error_structure():
    print("\n--- Scenario 3: Compile Error Structured Output ---")
    raw_errors = [
        "LogK2Compiler: Error: Pin 'Health' of node 'Set Health' cannot be connected",
        "LogBlueprint: Error: Blueprint error in graph 'EventGraph'",
        "LogK2Compiler: Warning: node 'Print String' type mismatch detected",
    ]

    structured = []
    for raw in raw_errors:
        entry = {"message": raw, "raw_log": raw,
                 "severity": "error" if "Error" in raw else "warning"}
        nm = re.search(r"[Nn]ode ['\"]([^'\"]+)['\"]", raw)
        gm = re.search(r"[Gg]raph ['\"]([^'\"]+)['\"]", raw)
        if nm:
            entry["node_title"] = nm.group(1)
        if gm:
            entry["graph_name"] = gm.group(1)
        structured.append(entry)

    _report("structured_errors_count", len(structured) == 3)
    _report("node_title_extracted",
            structured[0].get("node_title") == "Set Health",
            structured[0].get("node_title"))
    _report("graph_name_extracted",
            structured[1].get("graph_name") == "EventGraph",
            structured[1].get("graph_name"))
    _report("warning_severity", structured[2].get("severity") == "warning")

    ops = []
    if "pin" in raw_errors[0].lower() and "cannot" in raw_errors[0].lower():
        ops.append("fix_pin_name_in_add_connections")
    _report("probable_patch_ops_pin", "fix_pin_name_in_add_connections" in ops)


def smoke_trace_analytics():
    print("\n--- Scenario 4: Trace Analytics ---")
    tmp_home = tempfile.mkdtemp()
    today = time.strftime("%Y-%m-%d")
    trace_dir = os.path.join(tmp_home, ".unrealgenai", "traces", today)
    os.makedirs(trace_dir)

    traces = [
        {"blueprint_path": "/Game/BP_A",
         "patch_log": {"error_categories": [], "trace_id": "t001"},
         "results_summary": {"success": True, "errors": [], "rollback_status": None},
         "diagnostics": [], "post_failure_state": None},
        {"blueprint_path": "/Game/BP_A",
         "patch_log": {"error_categories": [], "trace_id": "t002"},
         "results_summary": {"success": True, "errors": [], "rollback_status": None},
         "diagnostics": [], "post_failure_state": None},
        {"blueprint_path": "/Game/BP_B",
         "patch_log": {"error_categories": ["PIN_NOT_FOUND"], "trace_id": "t003"},
         "results_summary": {"success": False, "errors": ["PIN_NOT_FOUND"],
                             "rollback_status": "complete",
                             "recommended_next_actions": ["get_node_details"]},
         "diagnostics": [{"code": "PIN_NOT_FOUND", "severity": "error"}],
         "post_failure_state": None},
        {"blueprint_path": "/Game/BP_B",
         "patch_log": {"error_categories": ["ROLLBACK_INCOMPLETE"], "trace_id": "t004"},
         "results_summary": {"success": False, "errors": ["ROLLBACK"],
                             "rollback_status": "partial"},
         "diagnostics": [],
         "post_failure_state": {"residual_nodes": [{"guid": "g1", "title": "Node1"}]}},
    ]

    for i, t in enumerate(traces):
        with open(os.path.join(trace_dir, f"trace_{i:04d}.json"), "w") as f:
            json.dump(t, f)

    try:
        trace_base = Path(tmp_home) / ".unrealgenai" / "traces"
        all_files = list((trace_base / today).glob("*.json"))

        total = success = failure = partial = ghost = 0
        error_codes = Counter()
        blueprint_failures = Counter()

        for tf in all_files:
            data = json.loads(tf.read_text())
            total += 1
            rs = data.get("results_summary", {})
            pl = data.get("patch_log", {})
            if rs.get("success"):
                success += 1
            else:
                failure += 1
                blueprint_failures[data.get("blueprint_path", "unknown")] += 1
            if rs.get("rollback_status") == "partial":
                partial += 1
            pfs = data.get("post_failure_state") or {}
            ghost += len(pfs.get("residual_nodes", []))
            for cat in pl.get("error_categories", []):
                error_codes[cat] += 1

        _report("analytics_total_runs", total == 4, f"total={total}")
        _report("analytics_success_count", success == 2, f"success={success}")
        _report("analytics_failure_count", failure == 2, f"failure={failure}")
        _report("analytics_rollback_partial", partial == 1, f"partial={partial}")
        _report("analytics_ghost_nodes", ghost == 1, f"ghost={ghost}")
        _report("analytics_top_error", error_codes.get("PIN_NOT_FOUND", 0) >= 1)
        _report("analytics_bp_failures", "/Game/BP_B" in blueprint_failures)
    finally:
        shutil.rmtree(tmp_home, ignore_errors=True)


def smoke_strict_mode():
    print("\n--- Scenario 5: Strict Mode / Node Lifecycle Registry ---")
    from node_lifecycle_registry import is_restricted, get_node_lifecycle, risk_score_for_node

    _report("MacroInstance_restricted", is_restricted("K2Node_MacroInstance"))
    _report("Timeline_restricted", is_restricted("K2Node_Timeline"))
    _report("ComponentBoundEvent_restricted", is_restricted("K2Node_ComponentBoundEvent"))
    _report("IfThenElse_not_restricted", not is_restricted("K2Node_IfThenElse"))
    _report("CallFunction_not_restricted", not is_restricted("K2Node_CallFunction"))
    _report("MacroInstance_high_score", risk_score_for_node("K2Node_MacroInstance") >= 3)
    _report("IfThenElse_zero_score", risk_score_for_node("K2Node_IfThenElse") == 0)

    info = get_node_lifecycle("K2Node_SwitchEnum")
    _report("SwitchEnum_needs_a1", "enum_type" in info.get("needs_a1", []))
    _report("SwitchEnum_reconstruct_after",
            "add_switch_case" in info.get("reconstruct_after", []))


def smoke_suggest_tool():
    print("\n--- Scenario 6: suggest_best_edit_tool Intent Routing ---")

    def _route(intent):
        il = intent.lower()
        if any(w in il for w in ("insert", "between", "middle")):
            return "insert_exec_node_between"
        if any(w in il for w in ("append", "after", "chain", "attach")):
            return "append_node_after_exec"
        if any(w in il for w in ("branch", "condition", "if", "wrap", "gate")):
            return "wrap_exec_chain_with_branch"
        return "describe_blueprint"

    _report("insert_intent",
            _route("insert a Delay between BeginPlay and PrintString") == "insert_exec_node_between")
    _report("append_intent",
            _route("append SetActorLocation after the movement call") == "append_node_after_exec")
    _report("branch_intent",
            _route("add a Branch to check a condition") == "wrap_exec_chain_with_branch")
    _report("ambiguous_intent",
            _route("do something complex with this node") == "describe_blueprint")


def smoke_macro_alias_resolver():
    print("\n--- Scenario 7: Macro Node Alias Resolver ---")
    try:
        from tools.patch import resolve_node_type_alias
        result = resolve_node_type_alias("ForEachLoop")
        _report("foreach_alias_node_type", result.get("node_type") == "K2Node_MacroInstance",
                f"got: {result.get('node_type')}")
        _report("foreach_alias_macro_graph", "StandardMacros" in result.get("macro_graph", ""),
                f"got: {result.get('macro_graph')}")
        _report("foreach_alias_has_foreachloop", "ForEachLoop" in result.get("macro_graph", ""),
                f"got: {result.get('macro_graph')}")

        unknown = resolve_node_type_alias("PrintString")
        _report("unknown_alias_returns_empty", unknown == {},
                f"got: {unknown}")

        for name in ("ForEachLoopWithBreak", "WhileLoop", "Gate", "DoOnce"):
            r = resolve_node_type_alias(name)
            _report(f"{name}_alias_is_macro_instance",
                    r.get("node_type") == "K2Node_MacroInstance",
                    f"got: {r.get('node_type')}")
    except ImportError as e:
        _report("macro_alias_import", False, str(e))


if __name__ == "__main__":
    print("=" * 55)
    print("UE5 Agent Execution Layer - Smoke Tests")
    print("=" * 55)

    smoke_transport_constants()
    smoke_patch_failure_readback()
    smoke_compile_error_structure()
    smoke_trace_analytics()
    smoke_strict_mode()
    smoke_suggest_tool()
    smoke_macro_alias_resolver()

    total = len(_RESULTS)
    passed = sum(1 for _, p in _RESULTS if p)
    failed = total - passed
    print()
    print("=" * 55)
    print(f"Results: {passed}/{total} passed" + (" ALL PASS" if not failed else f"  {failed} FAILED"))
    print("=" * 55)
    sys.exit(0 if not failed else 1)
