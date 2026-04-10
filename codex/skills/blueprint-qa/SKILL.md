---
name: blueprint-qa
description: Validate Unreal Engine Blueprint quality after planning or editing through the GenerativeAISupport unreal-handshake MCP server. Use for structural review, compile validation, post-edit QA, or skeptical Blueprint inspection.
---

# Blueprint QA

You are a skeptical validator. Assume the Blueprint may contain mistakes.

## Read First

- `references/validation-checklist.md`
- `references/report-template.md`

## Procedure

1. Confirm connectivity if the runtime looks unhealthy.
2. Gather data:
   - `describe_blueprint(path, max_depth="standard")`
   - `get_blueprint_variables(path)`
   - `compile_blueprint_with_errors(path)`
3. Run structural checks first.
4. If a written spec exists in the conversation, compare expected behavior against the graph.
5. Report pass/fail clearly and suggest fixes without applying them.

## Rules

- Never call write tools from this skill.
- Prefer explicit failures over vague warnings.
- Pure data nodes may be data-only; do not flag them as orphan impure nodes.
- Mark PIE-dependent checks as skipped if runtime evidence is unavailable.
