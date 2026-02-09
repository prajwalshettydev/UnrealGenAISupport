# Blueprint Editing via MCP - 功能说明

> 本文档记录 `feature/blueprint-ops-enhancement` 分支在上游 [prajwalshettydev/UnrealGenAISupport](https://github.com/prajwalshettydev/UnrealGenAISupport) 基础上所做的改动。

## 概述

本分支的目标是让 LLM（大语言模型）能够通过 MCP 协议**读取和修改** Unreal Engine 蓝图，实现 AI 辅助蓝图编辑的完整链路。

核心新增能力：
- **`get_blueprint_summary`** — 获取蓝图的结构化摘要（图表、变量、组件）
- **`apply_blueprint_patch`** — 批量执行蓝图操作（添加变量、函数、节点、连线、编译等）
- **`get_all_nodes_in_graph` 修复** — 修正返回类型，使 MCP 工具能正确传递节点列表

## 新增 MCP 工具

### get_blueprint_summary

获取蓝图的高层级摘要，为 LLM 提供蓝图结构上下文。

**参数：**
| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `blueprint_path` | string | (必填) | 蓝图资产路径，如 `/Game/Blueprints/BP_Player` |
| `include_graphs` | bool | `true` | 是否包含图表列表 |
| `include_variables` | bool | `true` | 是否包含变量列表 |
| `include_components` | bool | `false` | 是否包含构造脚本组件 |

**返回示例：**
```json
{
  "success": true,
  "summary": {
    "blueprint_path": "/Game/Unit/Tower/Tower_Basic",
    "asset_name": "Tower_Basic",
    "parent_class": "MobaUnitCharacter",
    "generated_class": "Tower_Basic_C",
    "graphs": [
      {
        "name": "EventGraph",
        "type": "UbergraphPage",
        "function_id": "EventGraph",
        "node_count": 38
      }
    ],
    "variables": [
      {"name": "current_health", "type": "float", "default_value": "0.0"},
      {"name": "moba_unit_data", "type": "MobaUnitData", "default_value": ""}
    ],
    "components": [],
    "warnings": []
  }
}
```

**实现说明：**
- 使用 `BlueprintEditorLibrary.find_event_graph()` / `find_graph()` 枚举图表（UE 5.7 兼容）
- 变量枚举采用 CDO（Class Default Object）反射方式，自动过滤继承属性，仅返回蓝图自身定义的变量
- 通过构造 `Default__ClassName` 路径并与父类 CDO 做差集来识别用户自定义属性

### apply_blueprint_patch

按顺序执行一批蓝图操作，支持原子化操作和失败中断。

**参数：**
| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `blueprint_path` | string | (必填) | 蓝图资产路径 |
| `operations` | list | (必填) | 操作列表，见下表 |
| `stop_on_error` | bool | `true` | 遇到第一个失败时是否中断 |
| `compile_after` | bool | `false` | 全部成功后是否自动编译 |

**支持的操作类型（`op` 字段）：**
| op | 说明 | 关键参数 |
|----|------|----------|
| `add_component` | 添加组件 | `component_class` |
| `add_variable` | 添加变量 | `variable_name`, `variable_type`, `default_value` |
| `add_function` | 添加函数 | `function_name` |
| `add_node` | 添加节点 | `function_id`, `node_type` |
| `connect_nodes` | 连接节点 | `source_node_id`, `source_pin`, `target_node_id`, `target_pin` |
| `delete_node` | 删除节点 | `function_id`, `node_id` |
| `compile_blueprint` | 编译蓝图 | (无额外参数) |
| `get_node_guid` | 获取节点 GUID | `graph_type`, `node_name` |

**调用示例：**
```json
{
  "blueprint_path": "/Game/Unit/Tower/Tower_Basic",
  "operations": [
    {"op": "add_variable", "variable_name": "MaxHealth", "variable_type": "float", "default_value": "100.0"},
    {"op": "add_variable", "variable_name": "IsAlive", "variable_type": "bool", "default_value": "true"},
    {"op": "compile_blueprint"}
  ],
  "stop_on_error": true,
  "compile_after": false
}
```

## Bug 修复

### get_all_nodes_in_graph 返回类型修复

**问题：** 原实现中 `response.get("nodes", "[]")` 期望获取字符串，但 handler 返回的是已解析的 Python list，导致 MCP 工具签名 `-> str` 验证失败。

**修复：** 在 `mcp_server.py` 中增加类型检查，当 `nodes` 为 list 时使用 `json.dumps()` 序列化为字符串。

### Socket Server 线程安全修复

**问题：** MCP 握手命令直接在 socket 线程中执行 `unreal.SystemLibrary.get_engine_version()`，触发 "Attempted to access Unreal API from outside the main game thread" 错误。

**修复：** 将所有命令（包括握手）统一排入主线程队列执行，并在 `_handle_handshake` 中增加 try-except 容错。

## UE 5.7.1 兼容性修复

以下修改解决了原插件在 UE 5.7.1 上的编译错误（原插件基于 UE 5.1 ~ 5.4 开发）：

| 修改内容 | 涉及文件 | 说明 |
|----------|----------|------|
| `ANY_PACKAGE` → `FindFirstObject` | GenActorUtils.cpp, GenWidgetUtils.cpp, GenBlueprintUtils.cpp, GenBlueprintNodeCreator.cpp | 11 处替换 |
| `FEditorStyle` → `FAppStyle` | GenEditorCommands.h, GenEditorWindow.cpp, GenerativeAISupportEditor.cpp | 类名和头文件替换 |
| `ClassDefaultObject` → `GetDefaultObject()` | GenActorUtils.cpp | API 签名变更 |
| `FStringOutputDevice` → `FOutputDeviceNull` | GenWidgetUtils.cpp | 类被移除 |
| `FMessageDialog::Open` 参数调整 | GenEditorWindow.cpp | Title 参数传值方式变更 |
| 移除 `EditorScriptingUtilities` / `Blutility` 依赖 | GenerativeAISupportEditor.Build.cs, GenerativeAISupport.uplugin | 模块在 5.7 中已移除 |
| 移除 `EditorStyle` 模块依赖 | GenerativeAISupportEditor.Build.cs | 已合并入 AppStyle |

## 改动文件清单

### Python（MCP / Handler）
| 文件 | 改动类型 |
|------|----------|
| `Content/Python/mcp_server.py` | 新增 2 个 MCP 工具，修复 1 个返回类型 |
| `Content/Python/handlers/blueprint_commands.py` | 新增 2 个 handler + 辅助函数 |
| `Content/Python/unreal_socket_server.py` | 注册新 handler，修复线程安全 |

### C++（UE 5.7.1 兼容性）
| 文件 | 改动类型 |
|------|----------|
| `GenerativeAISupport.uplugin` | 移除已废弃插件依赖 |
| `GenerativeAISupportEditor.Build.cs` | 移除已废弃模块依赖 |
| `GenActorUtils.cpp` | API 迁移 |
| `GenWidgetUtils.cpp` | API 迁移 |
| `GenBlueprintUtils.cpp` | API 迁移 |
| `GenBlueprintNodeCreator.cpp` | API 迁移 |
| `GenEditorCommands.h` | 头文件迁移 |
| `GenEditorWindow.cpp` | API 迁移 |
| `GenerativeAISupportEditor.cpp` | 头文件迁移 |

## 已知限制

- `get_blueprint_summary` 的变量枚举基于 CDO 反射差集方法，如果蓝图的父类不是 Actor / Pawn / Character 的直接子类，可能会漏掉或多出少量属性
- `EdGraph.nodes` 在 Python 中被标记为 protected，无法直接读取节点列表，需通过 C++ 端的 `GenBlueprintNodeCreator.get_all_nodes_in_graph()` 间接获取
- `BlueprintEditorLibrary` 在 UE 5.7 中暴露的 graph API 有限（`find_graph`、`find_event_graph`、`rename_graph`、`remove_graph` 等），无法直接枚举所有 graph
- 节点 Pin 连接合法性检查尚未实现

## 后续计划

- [ ] `get_blueprint_summary` 增加 `include_inherited_variables` 选项
- [ ] 实现 `preview_blueprint_patch`（Diff 预览）
- [ ] 实现 `undo_last_patch`（自动回滚）
- [ ] Pin 类型验证 / 连接合法性检查
- [ ] 支持更多蓝图父类的属性过滤（如 GameMode、PlayerController 等）
