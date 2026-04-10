# QA Validation Checklist

## Structural Checks (S1-S5) — Hard Pass/Fail

| # | Check | Pass Criteria | Tool |
|---|-------|---------------|------|
| S1 | Compiles | `compiled: true` | `compile_blueprint_with_errors(path)` |
| S2 | No orphan impure nodes | Every node with `role != "pure"` appears in at least one subgraph's `exec_edges` or `data_edges`. Pure nodes (VariableGet, Math, MakeStruct) are allowed to be data-only sources. | Inspect `describe_blueprint` output |
| S3 | Complete exec chains | Every entry event's exec_edges form a path to at least one terminal node. No dead-end output exec pins on non-terminal nodes. | Inspect `describe_blueprint` output |
| S4 | No unconnected required pins | With `max_depth="full"`: every non-optional input data pin is connected or has a non-empty `default_value`. Skip `self`, `WorldContextObject`, `LatentInfo` pins. | `describe_blueprint(path, max_depth="full")` |
| S5 | Variables typed | All variables have a `type` field. Key variables (non-temp) have defaults. | `get_blueprint_variables(path)` |

## Functional Checks (F1-F4) — Requires Spec

Only run if `docs/blueprint_specs/<BPName>.md` exists.

| # | Check | How |
|---|-------|-----|
| F1 | All spec'd events exist | Compare spec's event list vs entry node titles |
| F2 | All spec'd actions implemented | Each spec action maps to at least one call node |
| F3 | Data flow matches spec | Key data connections in spec exist in `data_edges` |
| F4 | No unspec'd side effects | Flag nodes with `semantic.side_effect: true` not in spec |

If no spec → skip and note "No spec found — functional checks skipped."

## Runtime Checks (R1-R3) — Requires PIE

Try `find_scene_objects(name_filter="<label>")` to detect if PIE is running.

| # | Check | How |
|---|-------|-----|
| R1 | Actor exists in level | `find_scene_objects` returns result |
| R2 | Key variables sane | `debug_blueprint(mode="inspect")` — no unexpected nulls |
| R3 | Entry event responds | `debug_blueprint(mode="simulate")` returns `executed: true` |

If PIE not running → mark all R checks as `[SKIP] PIE not running`.

## Semantic Structure Checks (M1-M5) — Medium Severity

Run after S1-S5. Use `describe_blueprint(path, max_depth="standard")` output for these checks.

| # | Check | Pass Criteria | How to Detect | Severity | Suggested Fix |
|---|-------|---------------|---------------|----------|---------------|
| M1 | Entry event exists | At least one node with `role: "entry"` (EventBeginPlay, CustomEvent, InputKey, InputAction, etc.) in the graph | Check `subgraphs[].nodes` for any node with `role == "entry"` or `node_type` matching `K2Node_Event`, `K2Node_CustomEvent`, `K2Node_InputKey`, `K2Node_InputAction` | Medium | Add appropriate entry event or verify this is a function graph (which doesn't need entry events) |
| M2 | No dangling Branch | Every `K2Node_IfThenElse` node has both `then` and `else` exec output pins connected | In `exec_edges`, check that for each Branch node GUID, both a `then` edge and an `else` edge exist as `from` endpoints | Medium | Connect the unconnected Branch output to an appropriate node (early return, log, or explicit no-op sequence) |
| M3 | No half-built condition path | No Branch node with exactly one of True/False connected where the unconnected side is not intentionally terminal | Same as M2 but flag as warning rather than fail if the unconnected side leads to a dead exec path | Low | Wire the missing branch or add a comment node explaining the intentional dead path |
| M4 | No dead-end exec flow | Every non-terminal node's exec output pin is connected | Check `exec_edges` — every node that is not a terminal (Return, EndPlay, last-in-chain) should appear as a `from` source for at least one exec edge | Medium | Connect the dangling exec output to the next logical node or add an explicit return/end |
| M5 | No empty event stub | No entry event node with zero outgoing exec connections (C1 from analyze_blueprint_quality covers this, but verify manually for CustomEvents) | Check nodes with `role == "entry"` — each should appear in at least one `exec_edge` as a `from` source | High | Implement the event body or delete the stub if unused |

**Note**: Checks M1-M5 are semantic — they require reading the `describe_blueprint` output. They complement the C1-C6 structural checks from `analyze_blueprint_quality`. M2/M3/M4 overlap with C1/C3 but provide more specific diagnosis language.
