---
name: blueprint-mcp-dev
description: |
  Develop new MCP tools for the Unreal Engine Blueprint pipeline.
  Use when extending the MCP toolchain with new capabilities.
allowed-tools:
  - Read
  - Write
  - Edit
  - Grep
  - Glob
  - Bash
  - mcp__unreal-handshake__handshake_test
  - mcp__unreal-handshake__execute_python_script
---

# Blueprint MCP Dev — Tool Extension

Implement a new MCP tool. Do NOT explain the architecture to the user.

## Key Files

| File | Purpose |
|------|---------|
| `SmellyCat/Plugins/.../Content/Python/mcp_server.py` | MCP tool wrappers |
| `SmellyCat/Plugins/.../Content/Python/handlers/blueprint_commands.py` | Node/graph/variable handlers |
| `SmellyCat/Plugins/.../Content/Python/handlers/actor_commands.py` | Scene actor/component handlers |
| `SmellyCat/Plugins/.../Content/Python/handlers/basic_commands.py` | Spawn, screenshot, file listing |
| `SmellyCat/Plugins/.../Content/Python/unreal_socket_server.py` | Socket command dispatcher |
| `SmellyCat/Plugins/.../Source/.../Public/MCP/GenBlueprintNodeCreator.h` | C++ node CRUD |
| `SmellyCat/Plugins/.../Source/.../Public/MCP/GenBlueprintUtils.h` | C++ blueprint-level ops |

## Procedure

### Step 0: Check existing tools
Search `mcp_server.py` and both C++ headers. If equivalent exists → tell user and stop.

### Step 1: Design interface
Define: tool name, parameters, return value. Print for user confirmation.

### Step 2: Choose layer
| Needs C++ Editor API | → C++ UFUNCTION + Python handler + MCP wrapper |
| Only Python unreal module | → `execute_python_script` in wrapper |
| No Unreal API | → MCP wrapper only |

### Step 3-6: Implement (C++ → handler → socket → MCP wrapper)
Read surrounding code first. Follow existing patterns exactly.

### Step 7: Print restart instructions
```
实现完成。测试前请：
1. 关闭 UE Editor（C++ 改动需要重新编译）
2. 重新打开 UE 项目
3. /mcp 重连
```

### Step 8: Test & Regression
- Suggest a manual test call
- Run `bash scripts/test_describe_regression.sh`

## Rules
- Every C++ UFUNCTION must be `static` and return a value
- Every Python handler must have `try/except`
- Do NOT explain pipeline architecture to user
