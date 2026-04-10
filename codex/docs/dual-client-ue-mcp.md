# Dual-Client UE MCP Setup

This plugin package is designed so Claude and Codex can connect to the same Unreal Editor instance through `unreal-handshake`.

## Source Of Truth

- Claude can continue using its machine-level desktop config.
- The installed Codex plugin ships its own `.mcp.json`, rendered to the local plugin repository path during installation.
- The MCP server entry point remains `Content/Python/mcp_server.py` inside the repository checkout on the current machine.

## Codex Expectations

- Install the Codex plugin with `Scripts/install_codex_plugin.ps1`.
- Start a fresh Codex session after installation so the new plugin and MCP config are loaded.
- Verify:
  - `get_runtime_status`
  - `handshake_test`
  - `describe_blueprint("/Game/Blueprints/BP_NPCAIController")`

## Troubleshooting

- If Claude works but Codex does not, reinstall the Codex plugin, then start a brand-new Codex session.
- If neither client works, confirm UE is open, the Python side is enabled, and `127.0.0.1:9877` is reachable.
