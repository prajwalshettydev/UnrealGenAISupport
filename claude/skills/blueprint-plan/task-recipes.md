# Blueprint Task Recipes

Reference this when Step 4 (Plan Changes) begins. Find the pattern closest to the user's request, use the canonical tool sequence.

## Recipe 1: Add Interaction Chain

**Description**: User wants to add a new interaction trigger (key press, overlap, etc.) that calls into another Blueprint or fires a custom event.

**Task Classification**: `structure_edit` or `build_new_flow`

**Canonical Tool Sequence**:
1. `get_runtime_status` → confirm UE online
2. `describe_blueprint(path, pseudocode, compact)` → find existing trigger patterns
3. `suggest_best_edit_tool(intent="add interaction trigger chain")` → confirm preferred tool
4. `search_blueprint_nodes(query="InputKey")` → find correct node type
5. **Preferred**: `append_node_after_exec` to add to existing chain, or `apply_blueprint_spec` for new chain
6. `compile_blueprint_with_errors` → verify
7. `analyze_blueprint_quality(mode="quick")` → check for orphans

**Example MCP sketch**:
```
append_node_after_exec(blueprint_path, "EventGraph", after_node_guid=<trigger_guid>,
  new_node_type="K2Node_CallFunction", new_node_params={"function_name": "OnInteract"})
```

**Notes**: For cross-BP interactions, always trace the existing pattern first (Step 3.5 in blueprint-plan). Do NOT assume input system type — check via Step 2.5 Input System Discovery.

---

## Recipe 2: Wrap Logic with Guard Condition

**Description**: User wants to add a condition check before existing logic runs (e.g., "only if health > 0", "only if bIsAlive").

**Task Classification**: `conditional_wrap`

**Canonical Tool Sequence**:
1. `describe_blueprint(path, pseudocode, compact, subgraph_filter=<target flow>)` → find entry node of logic to wrap
2. `suggest_best_edit_tool(intent="wrap logic with branch condition")` → confirm wrap_exec_chain_with_branch
3. **Preferred**: `wrap_exec_chain_with_branch(blueprint_path, graph_name, entry_from_node_guid, condition_variable=<var_name>)`
4. `compile_blueprint_with_errors` → verify
5. Connect the False branch appropriately (leave unconnected or add early-exit)

**Example MCP sketch**:
```
wrap_exec_chain_with_branch(blueprint_path, "EventGraph",
  entry_from_node_guid=<event_guid>, entry_from_pin="then",
  condition_variable="bIsAlive", true_chain_node_guid=<existing_logic_guid>)
```

**Notes**: `wrap_exec_chain_with_branch` handles the Branch node creation, variable getter, and reconnection atomically. Do NOT build this manually with `apply_blueprint_patch` unless the variable needs to be created first.

---

## Recipe 3: Append Fail-Safe Branch

**Description**: User wants to add an error/fail path after an operation (e.g., "if null, skip", "if failed, print error").

**Task Classification**: `structure_edit`

**Canonical Tool Sequence**:
1. `describe_blueprint(path, pseudocode, compact, subgraph_filter=<target flow>)` → locate the node to append after
2. `insert_exec_node_between` or `append_node_after_exec` to add a Branch after the operation
3. Wire the False/failure branch to an appropriate handler (PrintString, early return, etc.)
4. `compile_blueprint_with_errors` → verify

**Example MCP sketch**:
```
insert_exec_node_between(blueprint_path, "EventGraph",
  from_node_guid=<op_guid>, from_pin_name="then",
  to_node_guid=<next_guid>, to_pin_name="execute",
  new_node_type="K2Node_IfThenElse")
```

---

## Recipe 4: Create Trigger-Response Flow

**Description**: User wants to create a complete new event-driven flow: trigger (event/overlap/timer) → some response logic.

**Task Classification**: `build_new_flow`

**Canonical Tool Sequence**:
1. `suggest_best_edit_tool(intent="create trigger-response flow")` → typically apply_blueprint_spec
2. Build a spec JSON with the full flow structure
3. `apply_blueprint_spec(spec_json, dry_run=True)` → validate first
4. `apply_blueprint_spec(spec_json)` → execute
5. `compile_blueprint_with_errors` → verify
6. `analyze_blueprint_quality(mode="quick")` → check structure

**Example spec sketch**:
```json
{
  "blueprint": "/Game/Blueprints/BP_Target",
  "custom_events": [{"name": "OnTriggered"}],
  "graphs": {
    "EventGraph": {
      "nodes": [
        {"ref_id": "evt", "node_type": "K2Node_CustomEvent", "pin_values": {"CustomFunctionName": "OnTriggered"}},
        {"ref_id": "print", "node_type": "K2Node_CallFunction", "function_name": "PrintString", "pin_values": {"InString": "Triggered!"}}
      ],
      "edges": [{"from": "evt.then", "to": "print.execute"}]
    }
  },
  "compile": true
}
```

---

## Recipe 5: Build Decision Pipeline

**Description**: User wants a chain of condition checks with different outcomes for each branch (e.g., state machine dispatch, priority selector).

**Task Classification**: `build_new_flow` or `batch_mutation`

**Canonical Tool Sequence**:
1. `apply_blueprint_spec` with a full declarative spec for the decision structure (multiple Branch nodes chained)
2. For extending existing decision chain: `append_node_after_exec` to add new Branch at end of chain
3. `compile_blueprint_with_errors` → verify all branches
4. `analyze_blueprint_quality(mode="full")` → check for dangling branches (C1)

**Notes**: Decision pipelines are batch operations — prefer `apply_blueprint_spec` over multiple `apply_blueprint_patch` calls. Always use `strict_mode=True` if the pipeline involves more than 10 nodes (L3 territory).

---

## Recipe 6: Repair After Compile Failure

**Description**: Blueprint has compile errors from a previous edit or C++ API change.

**Task Classification**: `repair_existing_flow`

**Canonical Tool Sequence**:
1. `get_blueprint_working_set(blueprint_path, task_type="repair_existing_flow")` → get full context in one call
2. Read `recent_compile_errors.structured_errors` from the bundle
3. For each error: look up `error_code` in `repair-recipes.md`
4. Build targeted fix patch using `structured_errors[].node_guid` to locate nodes
5. `apply_blueprint_patch(patch_json, dry_run=True)` → validate
6. `apply_blueprint_patch(patch_json)` → apply
7. `compile_blueprint_with_errors` → verify clean

**Notes**: Always use `get_blueprint_working_set` for repair tasks — it reads the current graph state, variables, and compile errors in one call, giving you everything needed to diagnose without multiple round-trips.
