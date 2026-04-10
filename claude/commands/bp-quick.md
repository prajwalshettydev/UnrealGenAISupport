---
name: bp-quick
description: |
  轻量快速蓝图编辑，跳过 blueprint-plan 全流程。
  仅适用于：修改已有节点的 pin 值。
  不适用于：新建节点、添加连接、创建变量 → 请用 /bp
---

# bp-quick — 快速 Pin 值编辑

直接调用 blueprint-edit 执行，跳过 plan 流程中的以下步骤：
- C++ freshness gate 检查
- 知识库加载
- Blueprint index 模糊解析
- 完整规划和确认阶段

## 严格适用范围

✅ **可用**：
- 修改已有节点的 pin 默认值（`set_pin_values`）
- 断开某条连接（`remove_connections`，不新增连接）
- 触发重新编译

❌ **不可用**（重定向到 `/bp`）：
- 新建节点（`add_nodes`）
- 添加连接（`add_connections`）
- 创建变量或组件
- 任何涉及 class/object typed pin 的操作
- 跨 Blueprint 操作

## 用法

```
/bp-quick /Game/Blueprints/BP_NPC 把 PrintString 节点的 InString 改为 "Hello"
/bp-quick /Game/Blueprints/BP_Door 断开 EventBeginPlay 和 OpenDoor 之间的连接
```

$ARGUMENTS 直接传给 blueprint-edit 作为执行指令。
不需要 Step 1.2（freshness gate）和 Step 1.3（knowledge loading）。
从 Step 1.5（连接检查）开始执行。
