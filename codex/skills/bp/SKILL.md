---
name: bp
description: Universal Codex entry point for Unreal Blueprint work through the GenerativeAISupport unreal-handshake MCP server. Use when a user says bp, wants a /bp-like workflow in Codex, or wants one entry point for Blueprint planning, editing, QA, or MCP toolchain development.
---

# Blueprint Entry Point

Use this skill as the Codex-native replacement for `/bp`.

## Routing

- If the request is help, explain the available Codex entry points and stop.
- If the user wants quality review or post-edit verification, use `blueprint-qa`.
- If the user wants to extend the MCP toolchain itself, use `blueprint-mcp-dev`.
- If the user has already confirmed an exact Blueprint patch or asks to apply an approved plan, use `blueprint-edit`.
- Otherwise, default to `blueprint-plan`.

## Help Output

When the request is empty, `-h`, `--help`, or asks what `$bp` does, show:

- `$bp <goal>` for the universal entry point
- `$blueprint-plan <goal>` for planning and analysis
- `$blueprint-edit <approved change>` for guarded execution
- `$blueprint-qa <blueprint>` for validation
- `$blueprint-mcp-dev <tool goal>` for MCP extension work

## Session Rules

- Prefer planning first unless the requested edit is already decision complete.
- Keep Blueprint edits small and verifiable.
- Tell the user to start a fresh Codex session after installing or updating the plugin if skills or MCP tools are missing.
