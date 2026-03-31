# GenerativeAISupport — AI Code Assistant Integration

This directory contains everything an AI code assistant (Claude Code, Cursor, etc.) needs to control the Unreal Editor via MCP (Model Context Protocol). It is designed to be installed into any UE5 project that includes the GenerativeAISupport plugin as a submodule.

## Architecture

```
AI Assistant (Claude Code, etc.)
    │  stdio (JSON-RPC)
    ▼
FastMCP Server  (Content/Python/mcp_server.py)
    │  TCP socket, localhost:9877
    ▼
UE Editor Socket Listener  (Content/Python/unreal_socket_server.py)
    │  dispatches to handler functions
    ▼
Python Handlers  (Content/Python/handlers/*.py)
    │  call C++ via unreal.GenBlueprintUtils.*
    ▼
C++ UFUNCTIONs  (Source/GenerativeAISupportEditor/Public/MCP/GenBlueprintUtils.h)
    │  Unreal Editor API
    ▼
Blueprint / Scene / Asset manipulation
```

### Key Source Files

| File | Purpose |
|------|---------|
| `Content/Python/mcp_server.py` | FastMCP tool definitions, `send_to_unreal()` socket client |
| `Content/Python/unreal_socket_server.py` | Socket listener, `CommandDispatcher`, game-thread queuing |
| `Content/Python/handlers/blueprint_commands.py` | Blueprint CRUD handler functions |
| `Content/Python/handlers/actor_commands.py` | Actor/scene manipulation handlers |
| `Content/Python/init_unreal.py` | Plugin startup, socket server initialization |
| `Source/GenerativeAISupportEditor/Public/MCP/GenBlueprintUtils.h` | C++ UFUNCTION declarations |
| `Source/GenerativeAISupportEditor/Private/MCP/GenBlueprintUtils.cpp` | C++ implementations |

## Quick Start (for parent projects)

### Prerequisites

| Dependency | Install |
|------------|---------|
| [uv](https://docs.astral.sh/uv/) | `winget install astral-sh.uv` (Windows) / `brew install uv` (macOS) |
| Unreal Engine 5.4+ | With the project open in the Editor |
| AI assistant with MCP support | [Claude Code](https://docs.anthropic.com/en/docs/claude-code), Cursor, etc. |

### Installation

1. Add the plugin as a git submodule (if not already):
   ```bash
   git submodule add https://github.com/lixuan-shi/UnrealGenAISupport.git <YourProject>/Plugins/GenerativeAISupport
   ```

2. Run the setup script from the project root:
   ```bash
   bash setup.sh
   ```
   This installs: `.mcp.json`, `.claude/settings.json`, skill commands, and CLAUDE.md tool reference.

3. Open the UE project in the Editor. Go to **Edit > Plugins**, enable **GenerativeAISupport**, and restart.

4. Launch your AI assistant from the project root. Verify connectivity:
   ```
   handshake_test("ping")
   ```
   A response (even with a thread-safety warning) confirms the socket is reachable.

### What `setup.sh` Does

| Action | Target | Behavior |
|--------|--------|----------|
| Install skills | `.claude/commands/*.md` | Copies skill files; warns on conflict, never overwrites without `--force` |
| Inject tool reference | `CLAUDE.md` | Appends MCP tool API + known issues between sentinel markers; updates on re-run |
| Create MCP config | `.mcp.json` | Generates from template; merges if file exists with other servers |
| Create settings | `.claude/settings.json` | Enables MCP servers; merges if file exists |

Safe to run multiple times (idempotent). Use `--force` to overwrite diverged skill files.

## MCP Tool Reference

### Available Tools (12)

| Tool | Purpose |
|------|---------|
| `describe_blueprint` | Read blueprint as structured JSON (minimal/standard/full depth) |
| `apply_blueprint_patch` | Batch modify blueprint — add/remove nodes, connections, pin values |
| `debug_blueprint` | Compile check, inspect variables, simulate events, trace execution |
| `compile_blueprint_with_errors` | Compile with detailed error messages |
| `add_nodes_bulk` | Batch create nodes (returns ref_id -> GUID mapping) |
| `auto_layout_graph` | Auto-arrange nodes in the graph editor |
| `find_scene_objects` | Filter actors by class/name/tag |
| `get_blueprint_variables` | Get all blueprint variables in one call |
| `create_blueprint_from_template` | Create blueprint + apply patch atomically |
| `search_node_type` | Fuzzy search node types before adding |
| `diff_blueprint` | Compare two blueprint snapshots |
| `undo_last_operation` | Undo last editor action (currently broken — see Known Issues) |

### Core Workflow

```
1. describe_blueprint(path, "standard")          # understand current state
2. describe_blueprint(path, "full")               # get pin names for patching
3. apply_blueprint_patch(path, "EventGraph", {    # apply all changes in one call
     "add_nodes": [...],
     "add_connections": [...],
     "set_pin_values": [...]
   })
4. compile_blueprint_with_errors(path)            # verify compilation
5. auto_layout_graph(path)                        # clean up visual layout
```

### Patch Format

```json
{
  "add_nodes": [
    {"ref_id": "MyNode", "node_type": "K2Node_CallFunction",
     "function_name": "PrintString", "position": [400, 0],
     "pin_values": {"InString": "Hello World"}}
  ],
  "remove_nodes": ["<GUID>"],
  "add_connections": [
    {"from": "BeginPlay.then", "to": "MyNode.execute"}
  ],
  "remove_connections": [
    {"from": "<GUID>.then", "to": "<GUID>.execute"}
  ],
  "set_pin_values": [
    {"node": "<GUID or ref_id>", "pin": "InString", "value": "Updated"}
  ],
  "auto_compile": true
}
```

**Node reference resolution** — `from`/`to`/`node` fields accept:
- `ref_id` from `add_nodes` in the same patch (e.g. `"MyNode.then"`)
- GUID from `describe_blueprint` output
- Node title (e.g. `"Event BeginPlay.then"`) — resolved automatically

**Execution order:** remove_nodes → add_nodes → remove_connections → add_connections → set_pin_values → compile

### Depth Levels for `describe_blueprint`

| Level | What you get | When to use |
|-------|-------------|-------------|
| `minimal` | Graph summary, subgraph names, entry nodes, node count | Quick orientation |
| `standard` | + all nodes, exec/data edges, pseudocode | Understanding logic, planning changes |
| `full` | + every pin (name, type, default, connected), positions | When you need pin names for patching |

## Known Issues & Quirks

- **`handshake_test` thread warning** — Expected. The handshake runs on the socket thread, not the game thread. All other commands are properly queued.
- **`undo_last_operation` broken** — UE5 Python undo API is unavailable. Workaround: Ctrl+Z in Editor, or apply a corrective patch.
- **`get_blueprint_variables` misses unused variables** — Only shows variables referenced by graph nodes. UE5 Python does not expose `Blueprint.NewVariables`.
- **Node titles follow Editor locale** — `打印字符串` = PrintString, `事件开始运行` = Event BeginPlay. Semantic classification works for both languages.
- **Delay nodes** — `add_node(node_type="Delay")` creates `K2Node_CallFunction` calling `KismetSystemLibrary::Delay`. Use `then` pin, not `Completed`.
- **fastmcp versions** — Both v1.x and v3.x work. If tools stop loading, check: `uv pip show fastmcp`.

## Available Skills

Skills are structured SKILL.md workflows with frontmatter, tool restrictions, and supporting files. Install via `setup.sh` or copy `claude/skills/` to `.claude/skills/` manually.

### `/blueprint` (alias: `/bp`) — Universal Blueprint Entry Point

Routes to the appropriate skill based on intent. Uses a 3-phase pipeline:

| Phase | Skill | Purpose | Tools |
|-------|-------|---------|-------|
| Plan | `blueprint-plan` | Read, analyze, design changes. Outputs a spec. | Read-only MCP tools |
| Edit | `blueprint-edit` | Apply patches. Requires confirmed plan. | Write MCP tools |
| QA | `blueprint-qa` | Validate quality. Auto-runs after edit. | Read-only MCP tools |

Each skill restricts its `allowed-tools` to prevent accidental writes during planning or reads during editing.

See [skills/blueprint-plan/SKILL.md](skills/blueprint-plan/SKILL.md) for the entry point.

### `/blueprint-qa` — QA Evaluation

Independent quality validator: structural checks (compile, orphan nodes, exec chains, pin connections, variable types) → semantic checks against spec → runtime checks if PIE running → suggested fix patches. See [skills/blueprint-qa/SKILL.md](skills/blueprint-qa/SKILL.md).

### `/blueprint-mcp-dev` — Extend the MCP Toolchain

End-to-end workflow for adding a new MCP tool: check existing → design interface → implement C++ UFUNCTION → Python handler → socket registration → MCP wrapper → restart reminders. See [skills/blueprint-mcp-dev/SKILL.md](skills/blueprint-mcp-dev/SKILL.md).

### Skill Directory Structure

```
claude/skills/
├── blueprint-plan/          # Default entry — read-only analysis
│   ├── SKILL.md             # Frontmatter + procedure
│   ├── spec-template.md     # Spec output format
│   └── examples/            # Sample specs for reference
├── blueprint-edit/          # Guarded executor — write operations
│   ├── SKILL.md
│   ├── safety-rules.md      # Guardrails (read before every edit)
│   ├── mcp-api-reference.md # Patch format, tool params
│   └── patch-patterns.md    # Common patch recipes
├── blueprint-qa/            # Independent validator
│   ├── SKILL.md
│   ├── validation-checklist.md
│   └── report-template.md
└── blueprint-mcp-dev/       # Tool extension
    └── SKILL.md
```

## Extending: Adding New MCP Tools

Use the `/blueprint-mcp-dev` skill, or follow the pipeline manually:

1. **C++ UFUNCTION** in `GenBlueprintUtils.h/.cpp` — static, returns FString (JSON) or bool
2. **Python handler** in `handlers/*.py` — `handle_<name>(command)` with try/except
3. **Socket registration** in `unreal_socket_server.py` — add to dispatcher dict
4. **MCP tool wrapper** in `mcp_server.py` — `@mcp.tool()` with docstring

After changes: close UE Editor (C++ recompilation), restart MCP server, verify with `handshake_test`.
