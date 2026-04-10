---
name: blueprint-edit
description: Execute approved Unreal Engine Blueprint changes through the GenerativeAISupport unreal-handshake MCP server. Use when a blueprint plan already exists, the user has confirmed an exact change, or a very small Blueprint edit is unambiguous and ready to apply.
---

# Blueprint Edit

You execute guarded Blueprint changes. Planning belongs to `blueprint-plan`.

## Read First

- `references/safety-rules.md`
- `references/mcp-api-reference.md`
- `references/patch-patterns.md`
- `references/repair-recipes.md`

## Preconditions

Before editing, one of these must be true:

- A `blueprint-plan` spec already exists in the conversation.
- The user explicitly approved the exact change.
- The requested edit is tiny and unambiguous.

If not, redirect to `blueprint-plan`.

## Procedure

1. Check connectivity.
   - If runtime errors indicate the socket is down, stop and give the user the Unreal troubleshooting steps.
2. Take a before snapshot.
   - Use `describe_blueprint(..., max_depth="pseudocode", compact=true)` first.
   - Scope with `subgraph_filter` when you only touch one flow.
3. Build the change.
   - Prefer `apply_blueprint_spec` for new Blueprint creation or larger declarative changes.
   - Prefer `apply_blueprint_patch` for local surgical edits.
   - Use `inspect_blueprint_node`, `search_blueprint_nodes`, `search_node_type`, and `get_node_details` to confirm exact node/pin shapes before wiring anything risky.
4. Preflight when patching.
   - Run `preflight_blueprint_patch` before `apply_blueprint_patch`.
   - Fix issues before applying; do not skip preflight for patch-based edits.
5. Apply and verify.
   - Apply the change.
   - Compile.
   - Diff before and after.
   - Save dirty packages.
6. Hand off to QA.
   - Run or recommend `blueprint-qa` after a successful edit.

## Rules

- Keep one coherent change set per apply step.
- Do not assume class/object pins accept plain display strings; confirm the required value shape from live MCP reads.
- Stop after repeated compile or preflight failures and switch back to planning.
- Do not bypass the MCP safety path with ad hoc Python edits.
