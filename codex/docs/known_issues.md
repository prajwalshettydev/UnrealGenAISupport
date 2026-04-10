# Known Issues

This Codex plugin wraps the current GenerativeAISupport Unreal MCP toolchain. Use these constraints when planning or editing.

## Confirmed Constraints

- Blueprint write automation should still be treated as supervised-use only.
- Large graph rewrites are higher risk than small, scoped edits.
- If a patch fails, prefer reading the current graph state again before attempting a second fix.

## Practical Editing Limits

- Do not assume every engine macro node is safely patchable.
- Treat class/object pin wiring as high risk unless live MCP inspection confirms the exact expected value shape.
- Avoid full-graph `describe_blueprint(max_depth="standard")` by default on large Blueprints; use scoped reads first.

## Operator Guidance

- Save in UE before MCP edits.
- Use small change sets with compile and diff evidence between attempts.
- Stop after repeated compile or preflight failures and switch back to planning.
