<!-- Parent: ../AGENTS.md -->
<!-- Generated: 2026-04-08 | Updated: 2026-04-09 -->

# Content/Python (MCP Server)

## Purpose
Python subsystem running inside Unreal Editor's Python runtime. Provides the FastMCP server that exposes Blueprint editing, actor manipulation, and scene inspection tools to Claude Code via the MCP protocol. Also contains the Blueprint Spec engine, auto-layout engine, and job manager.

## Key Files
| File | Description |
|------|-------------|
| `mcp_server.py` | **Hot path (339x).** FastMCP server entry point — registers all `@mcp.tool()` handlers and starts the socket server |
| `unreal_socket_server.py` | Socket server running in Unreal's Python runtime; dispatches incoming JSON commands to registered handlers |
| `init_unreal.py` | Bootstrap: launches MCP server subprocess from within UE Editor on startup |
| `spec_engine.py` | Declarative Blueprint Spec Language v1 — validates, lowers, and executes spec JSON into MCP operations |
| `layout_engine.py` | Deterministic Blueprint node auto-layout; Python rules engine (~80% of placement decisions) |
| `job_manager.py` | Async job tracker for long-running UE operations (submit/status/cancel/list) |
| `command_registry.py` | Single-source command registry with `@registry.command()` decorators and dispatch table |
| `constants.py` | Shared constants: host, port, destructive script patterns, safety keywords |
| `node_lifecycle_registry.py` | Per-node-type lifecycle matrix: A1-phase requirements, risk level, `reconstruct_after` ops, `static_schema_preflight` flag — consumed by `tools/patch.py` and `mcp_server.py` |
| `test_file_tools.py` | Standalone validation script for MCP file read/write/patch tools |

## Subdirectories
| Directory | Purpose |
|-----------|---------|
| `handlers/` | Command handler classes (basic, actor, blueprint, python, UI) |
| `tools/` | Low-level MCP tool implementations (transport v1/v2, envelope, patch, inspect, describe) |
| `utils/` | Logging bridge and type converters |
| `mss/` | Bundled python-mss screenshot library |
| `tests/` | Python tests: smoke, transport protocol, patch rollback, safety, BP API freshness |

## For AI Agents

### Working In This Directory
- `mcp_server.py` is the highest-traffic file in the project. Be conservative with changes.
- All new MCP tools go in `handlers/` as `@registry.command()` functions, then exposed in `mcp_server.py` via `@mcp.tool()`.
- Layout engine decisions must remain Python-rules-driven; do NOT hardcode LLM-suggested coordinates.
- Spec engine changes (`spec_engine.py`) require dry-run testing before live execution.
- The `constants.py` destructive-pattern list is a safety contract — changes require security review.
- `node_lifecycle_registry.py` is the single source of truth for per-node-type risk/A1-phase/reconstruct requirements — update it instead of duplicating logic in patch.py or handlers.
- Transport v2 (length-prefixed framing, magic `0xABCD`) is opt-in via `SOCKET_PROTOCOL_VERSION=v2` env var; `unreal_socket_server.py` auto-detects v1 vs v2 from the magic header.

### Testing Requirements
- Smoke tests (no UE required): `python tests/smoke_tests.py`
- Transport protocol tests (mock socket): `python tests/test_transport_protocol.py`
- Run `test_file_tools.py` directly: `python test_file_tools.py`
- Full test suite: `pytest tests/` from the Python dir.
- Full MCP integration: start UE Editor with Python plugin enabled, then connect Claude Code MCP client.

### Common Patterns
- Tool call flow: Claude Code → MCP socket → `mcp_server.py` → `command_registry` → `handlers/*.py` → `tools/transport.py` → Unreal Python runtime → C++ `GenBlueprintUtils`.
- Response format: all tools return JSON via `tools/envelope.py` helpers. Patch failures include `post_failure_state.snapshot` for auto-readback.
- Error handling: handlers raise `ValueError` with descriptive messages; the server catches and formats errors with structured error codes (see `Docs/agent_runtime_status.md` for taxonomy).
- For Blueprint patches: consult `node_lifecycle_registry.get_node_lifecycle(node_type)` before deciding A1-phase order and whether `ReconstructNode` is needed afterward.

## Dependencies

### Internal
- `tools/transport.py` — socket I/O to Unreal runtime
- `command_registry.py` — all handlers register here

### External
- FastMCP
- Python 3.x standard library (socket, json, threading)
- Unreal Python runtime (available only inside UE Editor)

<!-- MANUAL: -->
