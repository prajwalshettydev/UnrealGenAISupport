<!-- Parent: ../AGENTS.md -->
<!-- Generated: 2026-04-08 | Updated: 2026-04-09 -->

# GenerativeAISupport Plugin

## Purpose
UE5 plugin that provides: (1) a FastMCP-based Python MCP server for Blueprint editing via Claude Code, (2) multi-vendor LLM API clients (Claude, OpenAI, DeepSeek, XAI), and (3) Claude Code skills for structured Blueprint authoring. This is the primary toolchain for AI-assisted Blueprint development in RPPilot.

## Key Files
| File | Description |
|------|-------------|
| `GenerativeAISupport.uplugin` | Plugin descriptor — lists modules, dependencies, and content paths |
| `README.md` | Plugin setup and usage guide |
| `LICENSE` | License file |

## Subdirectories
| Directory | Purpose |
|-----------|---------|
| `Content/Python/` | MCP server, socket transport, Blueprint tools, layout engine (see `Content/Python/AGENTS.md`) |
| `Source/GenerativeAISupport/` | C++ LLM API clients (Claude, OpenAI, DeepSeek, XAI) and utilities |
| `Source/GenerativeAISupportEditor/` | Editor-only C++ utilities for Blueprint node creation, actor ops, widget gen |
| `Docs/` | Plugin documentation |
| `Examples/` | Example Blueprints and schema examples |
| `claude/skills/` | Claude Code skills for Blueprint plan/edit/qa workflows (see `claude/skills/AGENTS.md`) |

## For AI Agents

### Working In This Directory
- The MCP server (`Content/Python/mcp_server.py`) is the primary entry point for Claude Code → UE Editor communication.
- C++ changes in `Source/` require a UBT rebuild via UE Editor before the Python server can call them.
- Blueprint MCP operations use a socket transport: Python → Unreal Python runtime → C++ backend.
- Do NOT modify `Content/Python/` files while the MCP server is running inside UE Editor.

### Testing Requirements
- MCP server smoke tests (no UE required): `python Content/Python/tests/smoke_tests.py`
- Transport protocol tests: `python Content/Python/tests/test_transport_protocol.py`
- Full Python suite: `pytest Content/Python/tests/`
- Blueprint operations: use `analyze_blueprint_quality` after edits.
- C++ editor utilities: test via UE Editor PIE or commandlets.

### Common Patterns
- Tool registration: `@mcp.tool()` in `mcp_server.py` delegates to handlers in `Content/Python/handlers/`.
- C++ bridge: Python calls `unreal.GenBlueprintUtils.*` / `unreal.GenActorUtils.*` which are exposed by the C++ editor module.
- Node lifecycle: `node_lifecycle_registry.py` defines A1-phase requirements, risk level, and `reconstruct_after` for each K2Node type — consult before constructing patch operations.
- Transport v2: set `SOCKET_PROTOCOL_VERSION=v2` for length-prefixed framing with 10s timeout; v1 (bare JSON) remains the default.
- Skills follow the plan→edit→qa three-phase pattern defined in `claude/skills/`.
- Runtime status reference: `Docs/agent_runtime_status.md` documents all MCP tools, error codes, strict_mode rules, and unresolved limitations.

## Dependencies

### Internal
- `SmellyCat/Source/AiNpcCore/` — referenced for protocol context (not a build dependency)
- `SmellyCat/Content/Blueprints/` — Blueprints edited via MCP tools

### External
- Unreal Engine 5 (Python plugin must be enabled in DefaultEngine.ini)
- FastMCP Python package
- Claude Code CLI (for skill invocation)

<!-- MANUAL: -->
