---
name: bp-observe
description: |
  蓝图系统纯观测模式。只读，不修改任何内容。
  查看运行时状态、历史失败统计、Blueprint 质量概览。
  等同于 /blueprint-qa --observe
---

# bp-observe — 蓝图系统观测

调用 blueprint-qa 的 --observe 模式，纯读取，不做 PASS/FAIL 评判。

## 适用场景

- "最近蓝图编辑失败率怎么样？"
- "BP_NPC 的质量现在怎么样？"
- "UE Editor 现在是否在线？协议状态是什么？"
- "有没有 ghost node 残留问题？"

## 输出内容

1. **运行时状态**：`get_runtime_status()` → 连接健康、协议版本、队列状态
2. **历史失败统计**：`get_trace_analytics(output_format="markdown")` → 最近失败率、top 错误码
3. **Blueprint 质量**（若提供路径）：`analyze_blueprint_quality(path, mode="full")` → C1-C6 概览

$ARGUMENTS 可选传入 blueprint_path 过滤特定 Blueprint。
