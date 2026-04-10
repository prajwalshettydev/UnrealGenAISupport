---
name: blueprint-plan
description: Plan Unreal Engine Blueprint changes for projects using the GenerativeAISupport unreal-handshake MCP server. Use when Codex needs to inspect, design, review, or scope Blueprint logic before any write, or when a user asks for Blueprint planning or architecture analysis.
---

# Blueprint Plan

You plan and analyze Blueprint work. Do not modify Blueprints from this skill.

## Read First

- `../../docs/agent_runtime_status.md`
- `../../docs/known_issues.md`
- `references/spec-template.md`

## Procedure

1. Confirm the runtime is reachable.
   - Prefer `get_runtime_status()` before deeper MCP reads.
   - If connection errors mention port refusal or socket failure, stop and give the user the UE/plugin troubleshooting steps.
2. Resolve the target Blueprint.
   - Use the explicit asset path if the user gave one.
   - Otherwise use `search_blueprint_nodes`, `list_blueprint_graphs`, and user wording to narrow to one Blueprint before planning.
3. Read current state conservatively.
   - Start with `describe_blueprint(path, max_depth="pseudocode", compact=true)`.
   - Add `subgraph_filter` when the request is scoped to one event flow.
   - Upgrade to `standard` only when you need specific node identities or pin details.
   - Use `get_blueprint_variables`, `compile_blueprint_with_errors`, `inspect_blueprint_node`, and `get_node_details` only for targeted questions.
4. Produce a decision-ready spec.
   - Include target Blueprint and graph.
   - List changes in execution order.
   - Include a capability matrix for any new node/function call you rely on.
   - Call out typed-pin, class/object, macro, and cross-BP risks explicitly.
5. Hand off cleanly.
   - If the user only asked for review, stop after the analysis.
   - If edits are needed, tell the user the next step is `blueprint-edit`.

## Rules

- Never call write tools from this skill.
- Never default to full-graph standard reads on large Blueprints.
- Treat live MCP inspection as authoritative over stale repo assumptions.
- If an intended pattern depends on unsupported or risky nodes, say so plainly and keep the spec read-only.
