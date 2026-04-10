# UE5 Agent 执行层 — 运行时状态与能力文档

> 版本: 2.1 | 生成日期: 2026-04-09

---

## 1. 工具分组总览

### core（日常使用）
| 工具 | 用途 |
|------|------|
| `describe_blueprint` | 读取图结构（pseudocode/compact/full），带指纹缓存 |
| `list_blueprint_graphs` | 列出所有 graph（EventGraph/Function/Macro）+ node_guids |
| `get_node_details` | 单节点所有 pin、连接、默认值 |
| `get_blueprint_variables` | 变量列表 |
| `apply_blueprint_patch` | 批量操作（支持 strict_mode / dry_run） |
| `apply_blueprint_spec` | 声明式 spec，内部自动 lower 成 patch |
| `preflight_blueprint_patch` | dry-run 预检，不执行 |
| `compile_blueprint_with_errors` | 编译，返回 structured_errors + post_compile_hint |
| `insert_exec_node_between` | **优先使用** — 在两个已连接 exec pin 之间插入节点 |
| `append_node_after_exec` | **优先使用** — 追加节点到 exec 链末尾或中间 |
| `wrap_exec_chain_with_branch` | **优先使用** — 用 Branch 包裹执行链 |
| `suggest_best_edit_tool` | 根据意图返回推荐工具 |
| `get_runtime_status` | 检查 UE socket runtime 健康，含协议状态 |

### analysis（诊断）
| 工具 | 用途 |
|------|------|
| `analyze_blueprint_quality` | C1-C6 质量检查（孤立节点、空 stub 等） |
| `get_trace_analytics` | 聚合 ~/.unrealgenai/traces/ 失败统计 |
| `search_blueprint_nodes` | 全引擎节点类型模糊搜索 |
| `inspect_blueprint_node` | 节点类型静态 pin schema |

### scaffold / admin
见工具列表（build_blueprint_index、safe_pull 等）

---

## 2. 推荐调用序列

### 新 Blueprint 编辑任务
```
1. get_runtime_status          → 确认 UE 在线，协议正常
2. list_blueprint_graphs       → 拿到 graph_guid + node_guids
3. describe_blueprint(pseudocode) → 理解当前逻辑
4. suggest_best_edit_tool(intent) → 确定用哪个工具
5a. insert_exec_node_between / append_node_after_exec / wrap_exec_chain_with_branch
   OR
5b. apply_blueprint_patch(dry_run=True) → 预检
6. apply_blueprint_patch       → 执行
7. compile_blueprint_with_errors → 验证
8. analyze_blueprint_quality   → QA
```

### 失败后修复
```
1. 读取 response 中的 post_failure_state.snapshot
2. 读取 post_failure_hint.recommended_next_actions
3. 按 hint 执行（通常是 describe_blueprint(force_refresh=True) + get_node_details）
4. 重新构造 patch 并执行
```

---

## 3. strict_mode 行为说明

`apply_blueprint_patch(..., strict_mode=True)` 时执行以下规则：

| 规则 | 触发条件 | 行为 |
|------|---------|------|
| RESTRICTED_NODE_TYPE | patch 包含 MacroInstance / ComponentBoundEvent / Timeline | BLOCKED，返回 STRICT_MODE_BLOCKED |
| DISPLAY_PIN_REF | 连接 endpoint 用显示名而非 GUID/fname: | 警告，附 guard_decisions |
| STRUCTURAL_EDIT | 同时有 remove_connections + add_connections | 建议用高层工具 |

response 中 `guard_decisions` 字段列出所有触发规则。
`strict_mode=False`（默认）不影响现有行为。

---

## 4. Transport v1/v2 说明

### v1（默认，当前生产）
- 裸 JSON over TCP，靠 JSON parse 完整性判断消息结束
- 无 framing，无长度前缀
- 向后兼容所有现有工具

### v2（可选，通过环境变量启用）
- 报文格式: `[2-byte magic 0xABCD][4-byte BE length][json payload]`
- 明确消息边界，不依赖 JSON parse
- 10s read timeout，超时返回 `TRANSPORT_TIMEOUT` 错误码
- 启用方式: 设置环境变量 `SOCKET_PROTOCOL_VERSION=v2`

**UE 侧**（unreal_socket_server.py）能自动识别 magic header：
- 收到 `0xABCD` → v2 framing 路径
- 其他 → v1 legacy 路径（向后兼容）

检查当前协议状态: `get_runtime_status()` → `protocol_version_active`, `framing_mode`

---

## 5. 已解决的 Concern 列表

| Concern | 解决方式 |
|---------|---------|
| 失败后无自动回读 | `_patch_finalize` 失败路径自动附 `post_failure_state.snapshot` |
| 缓存指纹弱（node_count） | 升级为 sha256(node_guids) hash，删一加一能感知 |
| compile 错误不结构化 | `structured_errors` + `probable_patch_ops` + `post_compile_hint` |
| 无 trace_id | `trace_id` 贯穿 Python [PATCH_LOG] 和 UE [AI Plugin] 日志 |
| 无 trace 文件 | 写入 `~/.unrealgenai/traces/YYYY-MM-DD/<trace_id>.json` |
| 无失败统计 | `get_trace_analytics` 工具，支持日期过滤和 markdown 输出 |
| 高频编辑依赖手写 patch | `insert_exec_node_between` / `append_node_after_exec` / `wrap_exec_chain_with_branch` |
| 无工具推荐 | `suggest_best_edit_tool(intent)` |
| 无守卫 | RBW guard (300s) + dry-run guard (risk score) + strict_mode |
| TCP 无超时 | v2 模式下 `s.settimeout(10.0)` + `TRANSPORT_TIMEOUT` |
| 队列无上限 | `deque(maxlen=50)` + `QUEUE_OVERFLOW` |
| ReconstructNode 不系统 | `node_lifecycle_registry.py` 矩阵 |
| locale pin alias 窄 | `_PIN_LOCALE_ALIASES` 29 个中文→英文 pin 名 |

---

## 6. 未解决的问题（明确不做或技术受限）

| 问题 | 原因 |
|------|------|
| preflight pin type compatibility 检查 | 需要真实 UEdGraphPin* 对象，preflight 阶段无法创建；不做伪实现 |
| "当前打开资产"只读工具 | UE Editor Python API 未暴露当前活跃 asset；`unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)` 有 API 但返回的是编辑器实例，不稳定；明确不做 |
| 失败聚合统计实时面板 | `get_trace_analytics` 提供离线聚合，无实时流 |
| v2 framing 自动协商（握手） | 当前靠 env var 控制，无自动版本握手 |
| loop body 节点精确定位 | 需要 `fname:` 前缀，模型需自己构造；无自动工具 |

---

## 7. node_lifecycle_registry 快速参考

| 节点类型 | risk | status | reconstruct_after |
|---------|------|--------|-------------------|
| K2Node_CustomEvent | low | supported | — |
| K2Node_IfThenElse | low | supported | — |
| K2Node_CallFunction | medium | supported | — |
| K2Node_SwitchEnum | medium | supported | add_switch_case |
| K2Node_SwitchString | medium | supported | add_switch_case |
| K2Node_MacroInstance | high | **restricted** | — |
| K2Node_ComponentBoundEvent | high | **specialist_tool_required** | — |
| K2Node_Timeline | high | **restricted** | — |

---

## 8. Error Code Taxonomy

| Code | 含义 |
|------|------|
| `PIN_NOT_FOUND` | patch 中引用的 pin 名不存在 |
| `TYPE_MISMATCH` | 连接类型不兼容（UE 执行时返回） |
| `NODE_NOT_FOUND` | patch 中引用的节点 GUID/ref 无法解析 |
| `FUNCTION_BIND_FAILED` | CallFunction 的 function_name 找不到 |
| `COMPILE_ERROR` | Blueprint 编译失败 |
| `HIGH_RISK_PATCH_BLOCKED` | risk score ≥ 6 且未先 dry_run |
| `STRICT_MODE_BLOCKED` | strict_mode=True 且包含 restricted 节点类型 |
| `ROLLBACK_INCOMPLETE` | ghost node 在 rollback 后仍残留 |
| `TRANSPORT_TIMEOUT` | socket read 超时（v2 模式） |
| `TRANSPORT_PROTOCOL_ERROR` | v2 framing 解析失败（非法 magic/length） |
| `TRANSPORT_DISCONNECTED` | UE socket server 未响应 |
| `QUEUE_OVERFLOW` | command_queue 超过 50 条上限 |
| `NODE_LIFECYCLE_UNSUPPORTED` | 节点类型在 registry 中标记为 restricted |
| `REF_UNRESOLVABLE` | ref_id/title/fname 无法解析为 GUID |
