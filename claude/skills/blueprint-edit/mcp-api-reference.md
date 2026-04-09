# MCP Tool API Reference — Blueprint Edit

Read this when you need exact parameter formats for MCP tool calls.

## Tool Quick Reference

### 高层结构编辑工具（优先使用）

| Tool | Purpose |
|------|---------|
| `suggest_best_edit_tool` | 根据自然语言意图返回推荐工具，避免直接写 patch |
| `insert_exec_node_between` | 在两个已连接 exec pin 之间插入新节点（自动断边+建节点+重连） |
| `append_node_after_exec` | 追加节点到 exec 链末尾或中间（自动处理链尾/链中两种情况） |
| `wrap_exec_chain_with_branch` | 用 Branch 节点包裹执行链（自动创建 Branch + 可选 VariableGet） |

### 通用写入工具

| Tool | Purpose |
|------|---------|
| `apply_blueprint_patch` | 批量操作，支持 `dry_run` / `strict_mode`；结构编辑请优先用上方工具 |
| `apply_blueprint_spec` | 声明式创建整个 Blueprint（内部自动 preflight） |
| `preflight_blueprint_patch` | dry-run 预检，不执行 |
| `create_blueprint_from_template` | 创建 BP + 应用 patch atomically |
| `compile_blueprint_with_errors` | 编译，返回 `structured_errors` + `post_compile_hint` |
| `auto_layout_graph` | 节点自动排版 |
| `diff_blueprint` | 比较两次 describe_blueprint 快照 |

### 读取工具

| Tool | Purpose |
|------|---------|
| `describe_blueprint` | 读取图结构（pseudocode/compact/full），带指纹缓存 |
| `get_node_details` | 单节点所有 pin（名称、类型、连接），RBW guard 更新时间戳 |
| `list_blueprint_graphs` | 列出所有 graph + node_guids |
| `search_blueprint_nodes` | 全引擎节点类型模糊搜索 |
| `inspect_blueprint_node` | 节点类型静态 pin schema |
| `get_runtime_status` | UE socket 健康状态（protocol、queue、last_error） |

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
- `fname:K2Node_IfThenElse_0` — FName 格式，用于 ForEachLoop body 内节点
- Node title from `describe_blueprint` (e.g. `"Event BeginPlay.then"`) — fragile，会增加 risk score

Execution order: remove_nodes → add_nodes (with pin_values) → remove_connections → add_connections → set_pin_values → compile

### apply_blueprint_patch 新增参数和响应字段

**参数：**
- `dry_run=True` — 只做 preflight 预检，不执行，返回 `predicted_nodes` 和 `issues`
- `strict_mode=True` — 阻断 restricted 节点（MacroInstance/ComponentBoundEvent/Timeline），返回 `guard_decisions`
- `verify=True`（默认）— 执行后自动回读图状态，附在 response 中

**成功响应新增字段：**
- `trace_id` — 用于关联 UE Output.log 中 `[AI Plugin]` 日志
- `guard_decisions` — strict_mode 触发的规则列表（RESTRICTED_NODE_TYPE / DISPLAY_PIN_REF / STRUCTURAL_EDIT）
- `verification.affected_nodes` — 创建节点的回读确认

**失败响应新增字段（自动附加）：**
- `post_failure_state.snapshot` — 当前真实图状态，**直接读取**，无需再调 describe_blueprint
- `post_failure_state.residual_nodes` — ghost node 列表（rollback 未完全清除）
- `post_failure_hint.recommended_next_actions` — 下一步建议枚举值

**错误码（error_code 字段）：**
- `PIN_NOT_FOUND` / `TYPE_MISMATCH` / `NODE_NOT_FOUND` / `FUNCTION_BIND_FAILED`
- `COMPILE_ERROR` / `HIGH_RISK_PATCH_BLOCKED` / `STRICT_MODE_BLOCKED`
- `ROLLBACK_INCOMPLETE` / `TRANSPORT_TIMEOUT` / `QUEUE_OVERFLOW`

### compile_blueprint_with_errors 新增响应字段

- `structured_errors[].probable_patch_ops` — 推断的修复操作类型
- `structured_errors[].node_guid` — 单名唯一时的节点 GUID
- `structured_errors[].node_guid_candidates` — 多候选时的 GUID 列表（不伪造单一结果）
- `structured_errors[].mapping_confidence` — high / low / none
- `post_compile_hint.recommended_next_actions` — 下一步建议

## Internal-Only Tools (不直接调用)

以下工具由 `apply_blueprint_patch` 内部使用，不直接暴露给 MCP：
add_node, delete_node, connect_nodes, disconnect_nodes, set_node_pin_value, move_node, add_nodes_bulk, connect_nodes_bulk, get_all_nodes, get_node_guid, compile_blueprint, get_compilation_status, add_component, add_variable, add_function, get_node_suggestions, get_all_scene_objects, add_component_with_events.

Do NOT call these directly. Use `apply_blueprint_patch` or the structural tools instead.
