---
name: blueprint-mcp-dev
description: Extend the GenerativeAISupport Unreal Blueprint MCP toolchain. Use when Codex needs to add, modify, or expose new unreal-handshake capabilities in the plugin's C++ or Python MCP layers.
---

# Blueprint MCP Dev

Implement MCP toolchain changes inside this repository.

## Key Files

- `Content/Python/mcp_server.py`
- `Content/Python/unreal_socket_server.py`
- `Content/Python/handlers/`
- `Source/GenerativeAISupportEditor/Public/MCP/`
- `Source/GenerativeAISupportEditor/Private/MCP/`

## Procedure

1. Search for an equivalent tool first.
2. Define the tool name, inputs, and return shape before editing.
3. Choose the narrowest layer that can implement the behavior.
   - MCP wrapper only
   - Python handler + socket dispatch
   - C++ UFUNCTION + Python bridge + MCP wrapper
4. Match existing patterns exactly.
5. If C++ changes are involved, tell the user UE must restart or rebuild before verification.

## Rules

- Add defensive error handling in Python handlers.
- Keep the wire shape stable unless the task explicitly requires a contract change.
- Validate the new tool with at least one manual smoke path and one regression-oriented read of nearby code.
