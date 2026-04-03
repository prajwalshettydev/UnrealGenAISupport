---
name: blueprint-edit
description: |
  Execute Blueprint modifications via MCP patch calls.
  ONLY use when a blueprint-plan spec exists or the user has explicitly
  confirmed the exact changes to make. Never start editing without a plan.
  After editing, blueprint-qa should run automatically.
allowed-tools:
  - Read
  - mcp__unreal-handshake__apply_blueprint_spec
  - mcp__unreal-handshake__apply_blueprint_patch
  - mcp__unreal-handshake__create_blueprint_from_template
  - mcp__unreal-handshake__create_blueprint
  - mcp__unreal-handshake__auto_layout_graph
  - mcp__unreal-handshake__describe_blueprint
  - mcp__unreal-handshake__compile_blueprint_with_errors
  - mcp__unreal-handshake__diff_blueprint
  - mcp__unreal-handshake__search_node_type
  - mcp__unreal-handshake__search_blueprint_nodes
  - mcp__unreal-handshake__inspect_blueprint_node
  - mcp__unreal-handshake__preflight_blueprint_patch
  - mcp__unreal-handshake__save_all_dirty_packages
---

# Blueprint Edit — Guarded Executor

You execute blueprint modifications. You do NOT plan — that's blueprint-plan's job.

Read `${CLAUDE_SKILL_DIR}/safety-rules.md` before every edit session.

## Prerequisites

Before editing, one of these must be true:
- A blueprint-plan spec was just output and the user confirmed it
- The user explicitly stated what to change (e.g. "apply that patch" or "go ahead")
- The user gave a very small, unambiguous instruction (e.g. "set InString to Hello")

If none are true → redirect to `blueprint-plan`.

## Freshness Gate

Before editing, validate that the generated C++ Blueprint API snapshot is fresh.

1. Run:
   `python Scripts/export_cpp_bp_api.py --project-root . --check-freshness`
2. If the result is `missing` or `stale`, stop and tell the user to rerun the supported build + refresh path before editing Blueprint nodes that depend on latest C++ usage.
3. Do not build or patch against speculative "latest" C++ Blueprint APIs when
   the freshness gate is failing.
4. Treat the snapshot as project-C++ candidate knowledge only; live MCP node inspection remains authoritative for plugin-exposed capabilities.
5. If readiness kill-switch conditions are active (freshness red or rollback suite red), suspend write-path usage and stop at planning/inspection.
6. Supported refresh path after a successful C++ build:
   `powershell -ExecutionPolicy Bypass -File Scripts/post_build_refresh_bp_api.ps1 -ProjectRoot . -TargetName RPSceneEditor`

## Connection Check

If any MCP tool returns `[WinError 10061]` or socket connection error → UE Editor is not running or plugin is not enabled. Print troubleshooting steps (see blueprint-plan Step 1.5) and STOP. Do not retry.

## Procedure

### Step 1: Before-Snapshot

```
before = describe_blueprint(path, max_depth="pseudocode", compact=true)
# If modifying a specific flow, scope it:
before = describe_blueprint(path, max_depth="pseudocode", compact=true, subgraph_filter="<target flow>")
```

Only upgrade to `standard` + `subgraph_filter` when you need specific node GUIDs for patching.
**NEVER** call `standard` on the full graph. **NEVER** scan all blueprints for a local change.

### Step 2: Build Patch

- Refer to `${CLAUDE_SKILL_DIR}/mcp-api-reference.md` for patch format
- Refer to `${CLAUDE_SKILL_DIR}/patch-patterns.md` for common recipes
- **For ANY node type not in the pin lookup table (CLAUDE.md)**:
  1. `search_blueprint_nodes(query, blueprint_path)` → get shortlist
  2. `inspect_blueprint_node(canonical_name)` → get exact pin names + `patch_hint`
  3. Use `patch_hint` fields directly in `add_nodes`
- Use `ref_id` for new nodes, `canonical_id` or `instance_id` for existing (not display titles)
- Build ONE `patch_json` covering ALL changes

### Step 2.5: Preflight (MANDATORY — NO EXCEPTIONS)

```
result = preflight_blueprint_patch(path, function_id, patch_json)
```

- `valid == false` → read `issues`, fix patch, re-preflight (max 2 retries). Do NOT call apply.
- `valid == true` → proceed to Step 3.
- **NEVER skip. NEVER call `apply_blueprint_patch` without a preceding valid preflight in the same session.**

### Step 3: Apply

`apply_blueprint_patch` now uses **transactions** - failures auto-rollback (no dirty graphs).
If response has `rolled_back: true`: analyze error, rebuild patch, retry (max 2 retries).
If response has `rollback_status` and it is not `complete`: stop the write workflow, treat the graph as requiring manual inspection, and do not proceed to the next automated patch.

**For modifications:**
```
apply_blueprint_patch(path, function_id, patch_json)
auto_layout_graph(path)
```

**For new blueprints:**
```
create_blueprint_from_template(name, parent_class, save_path, patch_json)
```

### Step 4: Verify

```
compile_result = compile_blueprint_with_errors(path)
after = describe_blueprint(path, max_depth="standard")
diff = diff_blueprint(before, after)
```

- Show user a concise diff summary
- If compile fails: read errors → build fix patch → re-apply (max 2 retries)
- After 2 failed retries → show errors and ask user

### Step 5: Hand Off to QA

After successful edit, `blueprint-qa` should run. If it finds issues, it will report them — do NOT auto-fix from QA results without user confirmation.

## Rules

- ONE patch call per change set
- Use `ref_id` for new nodes, `canonical_id`/`instance_id` or GUID for existing (NOT display titles)
- Treat `.claude/bp_cpp_api_index.json` and `15_cpp_bp_api.generated.md` as
  candidate guides only; live MCP node inspection remains authoritative.
- **Cross-BP edits**: When the spec involves replicating an existing behavior across multiple BPs, verify the pattern trace output exists in the spec before generating patches. If missing, redirect to blueprint-plan.
- Always preflight before applying
- Always diff after applying
- Max 2 auto-fix retries on compile failure or preflight failure
- Save the spec to `docs/blueprint_specs/<BPName>.md` after successful edit
- **Typed pins**: Before setting class/object pin values, read the reference node's `ref` field (semantic key for DefaultObject) via `get_node_details`. If it contains an asset path, use the same format. String `val` alone won't work for class pins.
- **Pattern replication**: When replicating an existing cross-BP flow, read the source pattern's full node details (including `ref`/`cls` typed pin fields) BEFORE generating the patch. Do not assume pin values from titles or `val` strings.
- **Array-to-single**: If connecting an array output to a single-object input, do NOT connect directly. Insert Get(index) or use GetActorOfClass (singular) instead of GetAllActorsOfClass (plural).

## apply_blueprint_spec vs apply_blueprint_patch

| Use `apply_blueprint_spec` when | Use `apply_blueprint_patch` when |
|----------------------------------|----------------------------------|
| Creating a new Blueprint from scratch | Modifying an existing graph locally |
| Adding multiple members (vars/components/events/functions) at once | Making targeted node/edge changes to one graph |
| Specifying full declarative intent (create + graph in one call) | The spec already exists and you only need to update a subgraph |
| Output from blueprint-plan includes a complete `spec` block | Patch is a small surgical edit (< 5 nodes) |

**`apply_blueprint_spec` always runs preflight internally — no separate preflight call needed.**

After any `apply_blueprint_spec` or `apply_blueprint_patch`, call `save_all_dirty_packages()`.

## Git + Blueprint Conflict Recovery

When a git rebase/merge has resolved .uasset conflicts by taking the target branch version:

1. Check `.bp_merge_work/reapply_report.md` for list of affected blueprints
2. For each affected blueprint, read the pre-rebase IR at `.bp_merge_work/ours/ir.json`
3. Compare with current state via `describe_blueprint(pseudocode, compact)`
4. Identify missing logic (nodes, connections, events) from the IR diff
5. Use standard `/bp` flow to re-apply: search → inspect → preflight → apply
6. Compile, verify, save

Key rules:
- **Do NOT copy .uasset files from old branches** — they may be incompatible with current C++ classes
- Always work from the target branch's .uasset as base, add logic on top via MCP
- Use `blueprint_ir_tools.py diff` to see exactly what was lost

## Auto Layout Rules (mandatory)

- **Do NOT specify explicit `"position"` coordinates unless you have a specific layout reason.** Let `apply_blueprint_patch` auto-place via the layout engine — it avoids overlap, anchors to the target flow, and is deterministic.
- If a patch has `add_connections` referencing existing nodes, those nodes serve as the layout anchor. The engine positions new nodes relative to them automatically.
- If you must override a position (e.g. inserting a node between two existing ones), use `placement_mode: "insert_between"` in `layout_intent` rather than guessing pixel coordinates.
- **Never attempt to compute coordinates yourself.** The occupancy grid engine handles collision detection.
- After applying a patch with many nodes, call `auto_layout_graph` to tidy the subgraph if needed.
