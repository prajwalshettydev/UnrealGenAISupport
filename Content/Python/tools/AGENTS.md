<!-- Parent: ../AGENTS.md -->
<!-- Generated: 2026-04-08 | Updated: 2026-04-09 -->

# Tools Package

## Purpose
Low-level MCP transport, envelope serialization, and Blueprint patch utilities extracted from `mcp_server.py`. These modules are re-imported by `mcp_server.py` and used by handlers to implement high-level Blueprint operations.

## Key Files
| File | Description |
|------|-------------|
| `__init__.py` | Package marker (empty) |
| `transport.py` | Socket transport: `send_to_unreal(command)` — JSON RPC over TCP to UE Editor Python runtime |
| `envelope.py` | Envelope format for MCP commands, cache eviction, locale handling |
| `patch.py` | Blueprint patch helpers: error classification, GUID validation, graph layout intent extraction |
| `inspect_tools.py` | Node introspection: get pin schema, find references, walk graph |
| `describe.py` | Graph description builder: node type classification, execution trace, data flow analysis |

## Transport Layer (`transport.py`)

### Purpose
Sends JSON-encoded commands to Unreal Engine's Python socket listener on localhost:9877 (configurable via `UNREAL_PORT` env var). Supports two framing protocols.

### API
```python
def send_to_unreal(command: dict) -> dict:
    """Send command to UE Editor, return response."""
    # Connects to localhost:UNREAL_PORT
    # Sends JSON command (v1: raw JSON; v2: magic+length prefix)
    # Reads response until complete JSON received
    # Returns parsed JSON or error dict
```

### Connection Contract
- **Port**: `os.environ.get("UNREAL_PORT", "9877")`
- **Protocol v1** (default): bare JSON over TCP, message boundary detected by JSON parse completeness
- **Protocol v2** (opt-in): `SOCKET_PROTOCOL_VERSION=v2` — frame format: `[0xAB 0xCD][4-byte BE length][JSON payload]`; 10s read timeout; returns `TRANSPORT_TIMEOUT` on expiry
- **Response Handling**: v1 reads until valid JSON; v2 reads exact length from header
- **Error Handling**: Returns `{"success": False, "error": "<message>", "error_code": "TRANSPORT_*"}` on socket failure
- **Constants**: `_FRAME_MAGIC = b"\xab\xcd"`, `_PROTOCOL_VERSION`, `_DEFAULT_TIMEOUT`

### Usage
```python
from tools.transport import send_to_unreal

response = send_to_unreal({
    "command": "add_node",
    "blueprint_path": "/Game/BP_Test",
    "node_type": "K2Node_CallFunction"
})
```

## Envelope Format (`envelope.py`)

### Purpose
Wraps commands with metadata, manages Blueprint cache invalidation, handles locale-specific node pin names, and drives the graph fingerprint cache.

### Key Functions
- `_evict_bp_caches(blueprint_path)` — clear Blueprint compile cache before edits
- `_LOCALE_ALIASES` / `_PIN_LOCALE_ALIASES` — map 29 Chinese pin names to English equivalents for cross-locale compatibility

### Locale Handling
Chinese-locale UE5 branch nodes output pins named `then`/`else` (not `True`/`False`). The envelope layer normalizes these. The alias table covers 29 common Chinese→English pin name mappings.

### Cache Fingerprint
The describe_blueprint cache uses `sha256(sorted node_guids)` as its fingerprint (upgraded from node_count). This means adding one node and removing another — same count but different hash — correctly invalidates the cache.

## Patch Utilities (`patch.py`)

### Purpose
Helper functions for `apply_blueprint_patch` command validation, risk scoring, read-before-write guard, and graph layout.

### Key Functions

#### Error Classification
```python
def _patch_classify_error(error_msg: str) -> str:
    """Classify error into: PIN_NOT_FOUND, TYPE_MISMATCH, NODE_NOT_FOUND, FUNCTION_BIND_FAILED, etc."""
```

#### Risk Scoring
```python
def _compute_patch_risk_score(patch_json: str) -> int:
    """Score patch risk (0-10). Score >= 6 without prior dry_run → HIGH_RISK_PATCH_BLOCKED."""
    # Consults node_lifecycle_registry.risk_score_for_node() per node type in patch
```

#### Read-Before-Write Guard
High-risk patches (risk score ≥ 6) are blocked unless `preflight_blueprint_patch` (dry-run) was called within the last 300 seconds for the same blueprint path. This is the RBW guard.

#### GUID Validation
```python
def _patch_looks_like_guid(s: str) -> bool:
    """Return True if s looks like a UUID/GUID (32+ hex chars, optional hyphens)."""
```

#### Layout Intent Extraction
```python
def _extract_layout_intent(patch_json: str) -> dict:
    """Extract semantic node placement hints from patch (used by layout engine)."""
```

#### Post-Failure State
On patch failure, `_patch_finalize` automatically attaches `post_failure_state.snapshot` (current graph state) and `post_failure_hint.recommended_next_actions` to the error response, enabling agents to self-recover without a manual `describe_blueprint` call.

## Node Introspection (`inspect_tools.py`)

### Purpose
Query Blueprint node schemas and graph structure without modification.

### Key Functions
| Function | Input | Output | Purpose |
|----------|-------|--------|---------|
| `get_node_pins(bp, node_guid)` | blueprint_path, node_guid | pin_list | List pins on node with types |
| `get_pin_connections(bp, pin_ref)` | blueprint_path, pin_ref | [connected_pins] | Find pins wired to given pin |
| `find_references(bp, var_name)` | blueprint_path, var_name | [node_guids] | Find nodes reading/writing variable |

## Graph Description (`describe.py`)

### Purpose
Build high-level summaries of Blueprint graphs for LLM context (node classifications, execution traces, pseudocode).

### Node Type Classification
Maps UE5 K2Node classes to semantic categories:
- **Entry nodes**: `K2Node_Event`, `K2Node_CustomEvent`, `K2Node_InputKey`
- **Control flow**: `K2Node_IfThenElse` (Branch), `K2Node_ForEachLoop` (Loop), `K2Node_Sequence`
- **State access**: `K2Node_VariableGet` (read), `K2Node_VariableSet` (write)
- **Struct ops**: `K2Node_MakeStruct`, `K2Node_BreakStruct`
- **Function calls**: `K2Node_CallFunction`, `K2Node_MacroInstance`
- **Pure nodes**: math, conversions, literals

See `_NODE_TYPE_MAP` for full classification.

### Execution Trace
Linearizes graph execution including loops, branches, and latent operations (Delay, Timeline) as a sequential pseudocode trace for LLM comprehension.

## For AI Agents

### Working In This Directory
- **Do NOT modify** while MCP server is running inside UE Editor.
- These are low-level utilities — use via `mcp_server.py` handlers, not directly.
- All functions must be synchronous (no async/await).
- Socket operations may timeout — always handle connection errors gracefully.

### Testing Requirements
- Unit test `transport.py` with mocked socket if UE Editor unavailable.
- Test locale handling (`envelope.py`) with Chinese-locale UE5 dumps.
- Verify patch validation (`patch.py`) rejects malformed GUIDs, invalid node types.
- Test graph description builder with blueprints of varying complexity (5, 50, 500 nodes).

### Common Patterns
- **Transport**: Always check response for `"success"` key before accessing data.
- **Envelopes**: Call `_evict_bp_caches()` BEFORE editing a Blueprint to clear stale compile state.
- **Patch validation**: Classify errors early to provide meaningful feedback to callers.
- **Graph description**: Cache descriptions briefly (5-10 min) to avoid re-parsing the same Blueprint.

### Dependencies
- `layout_engine` (separate module) — node placement rules and graph analysis
- `transport.py` — socket I/O (imported by handlers)
- `node_lifecycle_registry` — risk scoring and A1-phase requirements (imported by `patch.py`)
- `unreal` module (not imported here, but available at runtime in UE Editor)

## Data Flow

```
MCP Command (JSON)
    ↓
envelope.py (wrap metadata, evict cache)
    ↓
transport.py (send_to_unreal → localhost:9877)
    ↓
UE Editor Python Runtime (listener socket)
    ↓
C++ backend (GenBlueprintNodeCreator, etc.)
    ↓
Blueprint graph modification
    ↓
Response JSON (result, errors, new node guids)
    ↓
transport.py (receive, parse)
    ↓
handler (return to MCP client)
```

<!-- MANUAL: -->
