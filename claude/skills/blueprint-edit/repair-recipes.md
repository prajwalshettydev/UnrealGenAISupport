# Blueprint Edit — Repair Recipes

Reference this when a patch or compile step fails. Find the error code, follow the recovery action.

## Error Code Recovery Table

| Error Code | Likely Cause | Recovery Action |
|------------|-------------|-----------------|
| `PIN_NOT_FOUND` | Stale pin name; locale alias mismatch; pin renamed after ReconstructNode | 1. Read `post_failure_state.snapshot` (no describe needed). 2. `get_node_details(path, graph, node_guid)` to see real pin names. 3. Check `_PIN_LOCALE_ALIASES` in `tools/envelope.py` if locale may differ. 4. Rebuild patch with correct pin name. |
| `NODE_NOT_FOUND` | Stale GUID; wrong ref_id; node was deleted in a previous rollback | 1. `describe_blueprint(path, force_refresh=True, max_depth="standard")` to get current node list. 2. Re-resolve the target node by title or fname. 3. Rebuild patch with fresh GUID/ref. |
| `FUNCTION_BIND_FAILED` | `function_name` not found on target class; wrong `Class.Method` format; C++ API changed | 1. `search_blueprint_nodes(query="<function keyword>", blueprint_path=path)` to find correct canonical name. 2. `inspect_blueprint_node(canonical_name)` to verify pin schema. 3. Use exact `function_name` from `patch_hint` in the inspect result. |
| `TYPE_MISMATCH` | Connected pins have incompatible types; array output connected to single input; class pin set as string | 1. Read `structured_errors[].probable_patch_ops` for suggested fix type. 2. For array→single: insert `Get(0)` or `ForEach` node between the pins. 3. For class pins: use asset path in `DefaultObject` format, not plain string. |
| `ROLLBACK_INCOMPLETE` | Ghost node left after failed rollback; `rollback_status != complete` | 1. `describe_blueprint(path, force_refresh=True)` to identify ghost nodes. 2. Check `post_failure_state.residual_nodes` list. 3. Build a `remove_nodes` patch targeting each ghost GUID. 4. Compile and verify clean. |
| `COMPILE_ERROR` | Type mismatch, missing required input pin, broken connection after node removal | 1. Read `structured_errors[].node_guid` to locate the failing node. 2. Read `structured_errors[].probable_patch_ops` for repair type hint. 3. Read `post_compile_hint.recommended_next_actions`. 4. Build targeted fix patch. Max 2 auto-retries then escalate to user. |
| `HIGH_RISK_PATCH_BLOCKED` | Risk score ≥ 6 and `preflight_blueprint_patch` was not called within the last 300s | 1. Call `preflight_blueprint_patch(path, function_id, patch_json)` first. 2. Fix any `issues` returned. 3. Re-apply the patch (RBW guard satisfied for 300s). |
| `STRICT_MODE_BLOCKED` | Patch contains MacroInstance, ComponentBoundEvent, or Timeline node type with `strict_mode=True` | 1. Read `guard_decisions` to see which rule triggered. 2. Replace restricted node type with a supported alternative, OR split the operation and do the restricted part manually in UE Editor. 3. Do NOT disable strict_mode to bypass — fix the plan. |
| `STRICT_MODE_BLOCKED` (Timeline) | Timeline node is L3 restricted — track config requires UE Editor | Use the Timeline Node Workflow: create the Timeline node via patch, then follow the semi-automated workflow in SKILL.md to manually configure tracks in UE Editor. |
| `TRANSPORT_TIMEOUT` | UE socket read timed out (v2 mode, 10s limit); UE may be busy | 1. Wait 5-10s. 2. Call `get_runtime_status()` to check queue length and transport health. 3. If `queue_length > 0`, UE is processing — retry after queue drains. 4. If persistent, check UE Editor for blocking dialogs or crashes. |
| `QUEUE_OVERFLOW` | Command queue exceeded 50 items; too many rapid calls | 1. Stop sending commands immediately. 2. Call `get_runtime_status()` to monitor `queue_length`. 3. Wait until queue drains before resuming. 4. Break large batch operations into smaller sequential chunks. |
| `NODE_LIFECYCLE_UNSUPPORTED` | Node type marked `restricted` or `specialist_tool_required` in `node_lifecycle_registry` | 1. Check `node_lifecycle_registry.py` for the node type's `status` and `note`. 2. For `specialist_tool_required` (e.g. ComponentBoundEvent): use the dedicated MCP tool instead. 3. For `restricted` (e.g. Timeline): make the change manually in UE Editor, then verify via describe. 4. **For ForEachLoop specifically**: use `"node_type": "ForEachLoop"` in your patch — the alias resolver in `tools/patch.py` automatically expands it to `K2Node_MacroInstance` + correct `macro_graph`. This is fully supported. |
| `REF_UNRESOLVABLE` | `ref_id`, title, or `fname:` prefix could not be resolved to a node GUID | 1. `describe_blueprint(path, force_refresh=True)` to get current node list. 2. Use GUID directly instead of title reference (titles are fragile). 3. For ForEachLoop body nodes: use `get_node_guid_by_fname(path, graph, "K2Node_<Type>_N")` to resolve. |

## Common Repair Patterns

### Stale describe output causing multiple failures
**Symptom**: PIN_NOT_FOUND and NODE_NOT_FOUND in same patch.
**Fix**: Always read `post_failure_state.snapshot` first — it is the current graph state without an extra MCP call. Then rebuild the entire patch from scratch using fresh GUIDs.

### Compile fails after rollback
**Symptom**: Graph compiles cleanly before edit, fails after `rolled_back: true`.
**Fix**: Check `post_failure_state.residual_nodes`. Ghost nodes with broken connections cause compile failures. Remove them with a targeted `remove_nodes` patch.

### Pin name works in one locale, fails in another
**Symptom**: Works locally, fails on teammate's machine with Chinese-locale UE5.
**Fix**: Check `_PIN_LOCALE_ALIASES` in `Content/Python/tools/envelope.py`. Use English pin names — the envelope layer normalizes Chinese→English automatically.

### Wrong function binding creates "None" node
**Symptom**: `add_node` succeeds but the node has title "None" and empty pins.
**Fix**: The `function_name` did not resolve. Immediately remove the ghost node with `remove_nodes`, then use `search_blueprint_nodes` + `inspect_blueprint_node` to find the correct canonical name before retrying.
