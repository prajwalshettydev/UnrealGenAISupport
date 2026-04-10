---
name: blueprint-qa
description: |
  Validate Unreal Engine Blueprint quality after modifications.
  Auto-triggered after blueprint-edit completes. Can also be invoked
  manually. Performs structural, functional, and optional runtime checks.
  Reports issues with severity and suggested fix patches.
allowed-tools:
  - Read
  - mcp__unreal-handshake__describe_blueprint
  - mcp__unreal-handshake__compile_blueprint_with_errors
  - mcp__unreal-handshake__get_blueprint_variables
  - mcp__unreal-handshake__analyze_blueprint_quality
  - mcp__unreal-handshake__get_trace_analytics
  - mcp__unreal-handshake__debug_blueprint
  - mcp__unreal-handshake__find_scene_objects
  - mcp__unreal-handshake__diff_blueprint
---

# Blueprint QA — Independent Validator

## 调用模式

**默认模式（QA 审查）**：以怀疑者视角审查，按 PASS/FAIL 打分，给出修复建议 patch。

**`--observe` 模式（纯观测）**：若用户输入含 `--observe` 或问题是"现状/失败率/质量怎么样"类，进入观测模式：
- **不做** PASS/FAIL 评判，不输出修复建议
- 调用：`get_runtime_status` + `get_trace_analytics(output_format="markdown")` + `analyze_blueprint_quality`
- 输出：状态报告（健康度概览，不是问题列表）

---

You are a skeptical reviewer. Assume the blueprint was written by someone else and may have issues. Use hard pass/fail criteria.

Read `${CLAUDE_SKILL_DIR}/validation-checklist.md` for the full check definitions.

## Connection Check

If any MCP tool returns `[WinError 10061]` or socket connection error → UE Editor is not running or plugin is not enabled. Print troubleshooting steps (see blueprint-plan Step 1.5) and STOP. Do not retry.

## Step 1: Resolve Blueprint

If no path given, use the most recently edited blueprint from conversation context.

## Step 2: Gather Data

```
graph    = describe_blueprint(path, max_depth="standard")
vars     = get_blueprint_variables(path)
compile  = compile_blueprint_with_errors(path)
qa       = analyze_blueprint_quality(path, mode="full")   # C1-C6 结构检查
# 可选：了解近期失败模式
# history = get_trace_analytics(date_from="today", blueprint_path=path, output_format="markdown")
```

## Step 3: Run Checks

Follow the checklist in `${CLAUDE_SKILL_DIR}/validation-checklist.md`.

## Step 4: Output Report

Use format from `${CLAUDE_SKILL_DIR}/report-template.md`.

## Step 5: Suggest Fixes

For each FAIL, output a ready-to-use `apply_blueprint_patch` JSON snippet. Do NOT apply them — that's blueprint-edit's job.

## Rules

- **Never** call `apply_blueprint_patch` or any write tool
- Be skeptical, not lenient
- Pure data nodes (VariableGet, Math) are allowed outside exec_edges — don't flag as orphans
- PIE-dependent checks (R1-R3) should be marked [SKIP] if PIE is not running
