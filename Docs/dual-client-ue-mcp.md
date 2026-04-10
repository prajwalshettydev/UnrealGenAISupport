# Dual-Client UE MCP Setup

This plugin supports connecting both Claude and Codex to the same Unreal Editor instance through the `unreal-handshake` MCP server.

## Goal

Keep all project-level Codex MCP configuration inside the plugin repository:

- Plugin root: `C:/Users/PC/works/RPPilot/SmellyCat/Plugins/GenerativeAISupport`
- UE socket host: `localhost`
- UE socket port: `9877`

This workflow does **not** require changing `SmellyCat/.mcp.json`, `RPPilot/.mcp.json`, `DefaultEngine.ini`, or `DefaultGame.ini`.

## Source of Truth

- Claude Desktop uses the machine-level config at `C:/Users/PC/AppData/Local/Claude/claude_desktop_config.json`
- Codex and other project-scoped clients use the plugin-level `.mcp.json` at the plugin root
- The MCP server entry point is `Content/Python/mcp_server.py`

## Claude Desktop

Claude Desktop can keep using its existing global configuration as long as it contains a working `unreal-handshake` server entry that points at this plugin's `mcp_server.py`.

Recommended verification steps:

1. Open Unreal Editor and load the target project.
2. In Claude Desktop, confirm `unreal-handshake` is available.
3. Run:
   - `get_runtime_status`
   - `handshake_test`
   - `describe_blueprint("/Game/Blueprints/BP_NPCAIController")`

## Codex

Codex must open the **plugin directory itself** as the workspace root:

`C:/Users/PC/works/RPPilot/SmellyCat/Plugins/GenerativeAISupport`

Do not open `SmellyCat` root if the goal is to keep all MCP configuration and tracked changes inside the plugin repository only.

After opening the plugin directory as the workspace:

1. Start a fresh Codex session.
2. Let Codex load `.mcp.json` from the plugin root.
3. Verify that `mcp__unreal-handshake__*` tools are available.
4. Run:
   - `get_runtime_status`
   - `handshake_test`
   - `describe_blueprint("/Game/Blueprints/BP_NPCAIController")`

## Operational Rules

- Claude and Codex may both connect to the same Unreal Editor for read operations.
- This document only covers connection and read-path validation.
- Write-path coordination, transaction rules, and rollback conventions are intentionally out of scope for this setup doc.
- Do not move the plugin-level `.mcp.json` into `SmellyCat` root or `RPPilot` root.

## Troubleshooting

Use `Scripts/check_dual_client_ue_mcp.ps1` from the plugin root to verify:

- plugin `.mcp.json` exists
- `Content/Python/mcp_server.py` exists
- Claude Desktop config includes `unreal-handshake`
- `127.0.0.1:9877` is reachable
- the current working directory is the plugin root

If Claude works but Codex does not:

1. Confirm Codex opened the plugin directory, not `SmellyCat`
2. Start a brand-new Codex session after `.mcp.json` exists
3. Re-run the diagnostic script

If neither client works:

1. Confirm Unreal Editor is open
2. Confirm the UE-side socket server is available on `localhost:9877`
3. Confirm the configured `mcp_server.py` path still exists
