# MCP Tool API Reference — Blueprint Edit

Read this when you need exact parameter formats for MCP tool calls.

## Tool Quick Reference

| Tool | Purpose |
|------|---------|
| `describe_blueprint` | Read blueprint as structured JSON with pseudocode, expressions, semantics |
| `apply_blueprint_patch` | Batch modify blueprint (add/remove nodes, connections, pin values) |
| `create_blueprint_from_template` | Create BP + apply patch atomically |
| `compile_blueprint_with_errors` | Compile with detailed error messages |
| `auto_layout_graph` | Auto-arrange nodes in graph editor |
| `search_node_type` | Fuzzy search node types before adding |
| `diff_blueprint` | Compare two describe_blueprint snapshots |

## describe_blueprint Parameters

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

## apply_blueprint_patch — Patch Format

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
- Node title from `describe_blueprint` (e.g. `"Event BeginPlay.then"`)

Execution order: remove_nodes → add_nodes (with pin_values) → remove_connections → add_connections → set_pin_values → compile

## 18 Internal-Only Tools

These are NOT exposed to MCP but are used internally by `apply_blueprint_patch`:
add_node, delete_node, connect_nodes, disconnect_nodes, set_node_pin_value, move_node, add_nodes_bulk, connect_nodes_bulk, get_all_nodes, get_node_guid, compile_blueprint, get_compilation_status, add_component, add_variable, add_function, get_node_suggestions, get_all_scene_objects, add_component_with_events.

Do NOT call these directly. Use `apply_blueprint_patch` instead.
