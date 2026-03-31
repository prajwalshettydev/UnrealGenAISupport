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
