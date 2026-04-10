# Blueprint Edit Safety Rules

Read this before every edit session.

## ⚠️ Stability Warning

The blueprint toolchain is in early stage:
- Automatic modifications may produce unexpected results
- Always save in UE Editor (Ctrl+S) before MCP edits
- Make small, incremental changes — not large batch rewrites
- If something goes wrong: Ctrl+Z in UE Editor to undo

## Guardrails

1. **Never edit without a plan.** If no spec/confirmation exists, redirect to blueprint-plan.
2. **One patch per change set.** Never chain multiple `apply_blueprint_patch` calls for related changes — combine into one patch.
3. **Always snapshot before editing.** `describe_blueprint` before and after, then `diff_blueprint`.
4. **Max 2 auto-fix retries.** If compile fails twice after auto-fix attempts, stop and ask the user.
5. **Never delete Event nodes.** BeginPlay, Tick, and other engine events should not be removed.
6. **Never modify nodes you didn't create** unless the spec explicitly says to.

## Rollback

If an edit goes wrong:
1. Tell user to Ctrl+Z in UE Editor (most reliable)
2. Or apply a corrective patch that reverses the changes
3. `undo_last_operation` has limited effect — only works for UE transaction-based changes

## Token Efficiency

- **Never** call `describe_blueprint(full)` for an entire graph. Use `standard` depth + targeted `get_node_details` for the specific nodes you need pin info for.
- If `add_node` creates a node with title "None" and empty pins, the function binding failed. Clean up the broken node immediately — do not try to set pins on it.

## Patch Stratification

All patch operations fall into one of three risk tiers. Apply the required safety level for each tier:

| Tier | Operations | Required Safety |
|------|-----------|-----------------|
| **L1 — Safe** | Set pin values, lightweight connection adjustments, known node patterns | Dry-run recommended |
| **L2 — Structural** | Add/remove nodes, add/remove connections, ReconstructNode, switch case changes | Dry-run + transaction strongly recommended |
| **L3 — Dangerous** | Macro-sensitive edits, Timeline-sensitive edits, lifecycle-sensitive call sites, broad batch edits, cross-graph changes | `strict_mode=True` + preflight + transaction + compile + QA **required** |

**How to determine tier**: consult `node_lifecycle_registry.get_node_lifecycle(node_type)` for node risk level. Any patch touching `MacroInstance`, `Timeline`, or `ComponentBoundEvent` is automatically L3.

**L3 policy**: `apply_blueprint_patch` with `strict_mode=True` will block restricted node types and return `STRICT_MODE_BLOCKED`. Do not bypass. Fix the plan to use supported node types or break into smaller L2 steps.

## Forbidden Actions

- Do NOT call `execute_python_script` to modify blueprints (bypass MCP safety)
- Do NOT modify C++ source files from this skill
- Do NOT create/delete blueprint assets outside the patch system
