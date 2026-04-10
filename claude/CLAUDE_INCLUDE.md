## MCP Known Issues & Quirks

### `handshake_test` always returns "SystemLibrary: Attempted to access Unreal API from outside the main game thread"
This is **expected and harmless**. The handshake command is processed directly on the socket thread (not queued to the game thread) for speed. The error comes from trying to call `SystemLibrary.get_engine_version()` off the main thread. All other commands are properly queued to the game thread via `register_slate_post_tick_callback` and work correctly. **Do not try to fix this** — it confirms the socket is reachable.

### `undo_last_operation` does not work
The UE5 Python API for `EditorUtilityLibrary` undo is broken/unavailable. The tool exists but always fails. **Workaround**: tell the user to Ctrl+Z in the UE Editor manually, or re-apply a corrective patch.

### `get_blueprint_variables` only shows referenced variables
Variables that exist in the blueprint but are never used in any graph (no VariableGet/Set node) will not appear. This is because UE5 Python does not expose `Blueprint.NewVariables`. The tool works by scanning graph nodes.

### Node titles are in Chinese (or the Editor's locale)
All node titles (e.g. `打印字符串` = PrintString, `事件开始运行` = Event BeginPlay) come from UE's localization. The `describe_blueprint` pseudocode uses these titles directly. This affects LLM reasoning on English-language prompts but does not affect tool functionality. The semantic classification (`intent: "debug"`, `intent: "async"`) works for both Chinese and English titles.

### Creating Delay nodes produces K2Node_CallFunction, not K2Node_Delay
When using `add_node(node_type="Delay")`, UE creates a `K2Node_CallFunction` calling `KismetSystemLibrary::Delay`, not a native `K2Node_Delay`. This means:
- The node has a `then` exec output (not `Completed`)
- `describe_blueprint` still detects it as latent via the `LatentInfo` pin
- Connections should use `then` pin, not `Completed`

### MCP server process and fastmcp version
The `.mcp.json` uses `uv` to run `mcp_server.py`. The server requires `fastmcp`. Both v1.x and v3.x work (v3.x changed the `Image` import but that's handled). If tools stop loading after a `uv pip install`, check that fastmcp is still installed: `uv pip show fastmcp`.

## MCP Tool API Reference (implicit — do not print to user)

### Tool Quick Reference

| Tool | MCP Name | Purpose |
|------|----------|---------|
| `describe_blueprint` | `mcp__unreal-handshake__describe_blueprint` | Read blueprint as structured JSON |
| `apply_blueprint_patch` | `mcp__unreal-handshake__apply_blueprint_patch` | Batch modify blueprint (add/remove nodes, connections, pin values) |
| `debug_blueprint` | `mcp__unreal-handshake__debug_blueprint` | Compile check, inspect vars, simulate events, trace execution |
| `add_nodes_bulk` | `mcp__unreal-handshake__add_nodes_bulk` | Batch create nodes (returns ref_id -> GUID mapping) |
| `compile_blueprint_with_errors` | `mcp__unreal-handshake__compile_blueprint_with_errors` | Compile with detailed error messages |
| `auto_layout_graph` | `mcp__unreal-handshake__auto_layout_graph` | Auto-arrange nodes in graph editor |
| `find_scene_objects` | `mcp__unreal-handshake__find_scene_objects` | Filter actors by class/name/tag |
| `get_blueprint_variables` | `mcp__unreal-handshake__get_blueprint_variables` | Get all BP variables in one call |
| `create_blueprint_from_template` | `mcp__unreal-handshake__create_blueprint_from_template` | Create BP + apply patch atomically |
| `undo_last_operation` | `mcp__unreal-handshake__undo_last_operation` | Undo last editor action |
| `search_node_type` | `mcp__unreal-handshake__search_node_type` | Fuzzy search node types before adding |
| `diff_blueprint` | `mcp__unreal-handshake__diff_blueprint` | Compare two blueprint snapshots |

### describe_blueprint Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `blueprint_path` | string | yes | Asset path, e.g. `"/Game/Blueprints/BP_MyNPC"` |
| `graph_name` | string | no | Specific graph (e.g. `"EventGraph"`). Empty = all graphs. |
| `max_depth` | string | no | `"minimal"` / `"standard"` (default) / `"full"` |
| `include_pseudocode` | bool | no | Generate pseudocode per subgraph. Default `true`. |

Depth levels:
- `minimal` — graph summary, subgraph names, entry nodes, node count
- `standard` — + all nodes (id, title, type, tags), exec_edges, data_edges, pseudocode
- `full` — + every pin (name, type, default_value, is_connected), positions, symbol_table

### apply_blueprint_patch — Patch Format

```json
{
  "add_nodes": [
    {
      "ref_id": "MyNode",
      "node_type": "K2Node_CallFunction",
      "function_name": "PrintString",
      "position": [400, 0],
      "pin_values": {"InString": "Hello World"}
    }
  ],
  "remove_nodes": ["<GUID>"],
  "add_connections": [
    {"from": "BeginPlay.then", "to": "MyNode.execute"}
  ],
  "remove_connections": [
    {"from": "<GUID>.then", "to": "<GUID>.execute"}
  ],
  "set_pin_values": [
    {"node": "<GUID or ref_id>", "pin": "InString", "value": "Updated text"}
  ],
  "auto_compile": true
}
```

Node reference resolution (for `from`/`to`/`node` fields):
- `ref_id` from `add_nodes` in the same patch (e.g. `"MyNode.then"`)
- GUID from `describe_blueprint` output (e.g. `"A1B2C3D4-....then"`)
- Node title from `describe_blueprint` (e.g. `"Event BeginPlay.then"`) — resolved automatically

Execution order: remove_nodes -> add_nodes (with pin_values) -> remove_connections -> add_connections -> set_pin_values -> compile

### Common Patch Patterns

**Insert function call between two existing nodes:**
```json
{
  "add_nodes": [
    {"ref_id": "NewCall", "node_type": "K2Node_CallFunction",
     "function_name": "MyFunction", "position": [800, 0]}
  ],
  "remove_connections": [
    {"from": "<prev_guid>.then", "to": "<next_guid>.execute"}
  ],
  "add_connections": [
    {"from": "<prev_guid>.then", "to": "NewCall.execute"},
    {"from": "NewCall.then", "to": "<next_guid>.execute"}
  ]
}
```

**Add a branch with two paths:**
```json
{
  "add_nodes": [
    {"ref_id": "MyBranch", "node_type": "K2Node_IfThenElse", "position": [600, 0]},
    {"ref_id": "TrueAction", "node_type": "K2Node_CallFunction",
     "function_name": "PrintString", "pin_values": {"InString": "Yes"}},
    {"ref_id": "FalseAction", "node_type": "K2Node_CallFunction",
     "function_name": "PrintString", "pin_values": {"InString": "No"}}
  ],
  "add_connections": [
    {"from": "<prev>.then", "to": "MyBranch.execute"},
    {"from": "MyBranch.True", "to": "TrueAction.execute"},
    {"from": "MyBranch.False", "to": "FalseAction.execute"}
  ]
}
```

**Set a variable:**
```json
{
  "add_nodes": [
    {"ref_id": "SetVar", "node_type": "K2Node_VariableSet",
     "function_name": "MyVariable", "pin_values": {"MyVariable": "42"}}
  ],
  "add_connections": [
    {"from": "<prev>.then", "to": "SetVar.execute"}
  ]
}
```
