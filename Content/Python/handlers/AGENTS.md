<!-- Parent: ../AGENTS.md -->
<!-- Generated: 2026-04-08 | Updated: 2026-04-09 -->

# Handlers Package

## Purpose
Command handler modules auto-registered via `@registry.command()` decorator at import time. Each handler processes a specific category of Blueprint editing, actor manipulation, UI, and Python execution commands from the MCP server.

## Key Files
| File | Description |
|------|-------------|
| `__init__.py` | Package marker (empty) |
| `base_handler.py` | Abstract `CommandHandler` base class with `handle(command)` interface |
| `basic_commands.py` | Low-level operations: `spawn`, `delete`, `get_property`, `set_property` |
| `actor_commands.py` | Actor scene manipulation: movement, rotation, scale, physics, visibility |
| `ui_commands.py` | Widget and UI operations: create, update, destroy, focus, input routing |
| `blueprint_commands.py` | Blueprint graph editing: add/remove nodes, connect pins, apply patches |
| `python_commands.py` | Sandbox Python execution: eval expressions, import modules, run scripts |

## Architectural Patterns

### Command Registration
All handlers use the `@registry.command(name, category="...")` decorator. The registry auto-collects handlers at module import time and makes them available to `mcp_server.py` for tool dispatch.

Example from `basic_commands.py`:
```python
@registry.command("spawn", category="basic")
def handle_spawn(command: Dict[str, Any]) -> Dict[str, Any]:
    """Handle a spawn command"""
    # Implementation
```

### Response Contract
All handlers return a response dictionary with mandatory fields:
- `"success"`: bool — operation result
- `"error"`: str (optional) — error message if success is False
- Additional fields per command type (e.g., `"actor_id"`, `"property_value"`)

### Error Handling
- Exceptions are caught and wrapped in `{"success": False, "error": "<message>"}`
- Errors are logged via `utils.logging` with context (command type, parameters)
- No exceptions escape to `mcp_server.py` — handler layer must be fault-tolerant

## Command Categories

### Basic Commands (`basic_commands.py`)
| Command | Input | Output | Purpose |
|---------|-------|--------|---------|
| `spawn` | actor_class, location, rotation, scale, actor_label | actor_id, location, rotation | Spawn actor in scene |
| `delete` | actor_id | success | Delete actor by ID |
| `get_property` | actor_id, property_name | property_value | Read actor property |
| `set_property` | actor_id, property_name, property_value | success | Write actor property |

### Actor Commands (`actor_commands.py`)
| Command | Input | Output | Purpose |
|---------|-------|--------|---------|
| `move_to` | actor_id, location, speed | success, time_estimate | Move actor to location |
| `set_rotation` | actor_id, rotation | success | Set actor rotation |
| `set_scale` | actor_id, scale | success | Set actor scale |
| `enable_physics` | actor_id | success | Enable physics simulation |
| `set_visible` | actor_id, visible | success | Toggle visibility |

### UI Commands (`ui_commands.py`)
| Command | Input | Output | Purpose |
|---------|-------|--------|---------|
| `create_widget` | widget_class, parent_id, name | widget_id | Create UI widget |
| `update_widget` | widget_id, properties | success | Update widget properties |
| `set_focus` | widget_id | success | Set input focus to widget |
| `input_text` | widget_id, text | success | Send text input to widget |

### Blueprint Commands (`blueprint_commands.py`)
| Command | Input | Output | Purpose |
|---------|-------|--------|---------|
| `apply_blueprint_patch` | blueprint_path, function_id, patch_json, dry_run, strict_mode | success, errors, guard_decisions | Batch node/connection mutations with optional dry-run and strict_mode guards |
| `apply_blueprint_spec` | spec_json, dry_run | success, ops, compile_errors | Declarative spec → lowered patch → execute atomically |
| `preflight_blueprint_patch` | blueprint_path, function_id, patch_json | predicted_changes, issues | Validate patch without applying (satisfies RBW guard for 300s) |
| `insert_exec_node_between` | blueprint_path, graph_name, from/to node GUIDs, new_node_type | success, created_nodes | Insert node between two connected exec pins |
| `append_node_after_exec` | blueprint_path, graph_name, after_node_guid, new_node_type | success, created_nodes | Append node at end/middle of exec chain |
| `wrap_exec_chain_with_branch` | blueprint_path, graph_name, entry_from_node_guid, condition_variable | success, created_nodes | Wrap exec chain with Branch node |
| `compile_blueprint_with_errors` | blueprint_path | compiled, errors, structured_errors, post_compile_hint | Compile and return structured error info |
| `add_switch_case` | blueprint_path, graph_id, node_guid, case_name | success, pin_count | Add case to SwitchString node (calls ReconstructNode) |

### Python Commands (`python_commands.py`)
| Command | Input | Output | Purpose |
|---------|-------|--------|---------|
| `eval` | code, globals | result | Evaluate Python expression |
| `exec` | code, globals | success, output | Execute Python code |
| `import` | module_name | success | Import module |

## For AI Agents

### Working In This Directory
- Do NOT modify handlers while the MCP server is running inside UE Editor.
- Add new command handlers by creating a new function, decorating with `@registry.command()`, and importing in `mcp_server.py`.
- All handlers must be synchronous — async operations should use Unreal's task graph via `utils.unreal_ops`.
- Always catch exceptions and return `{"success": False, "error": "..."}` rather than raising.
- For Blueprint node creation commands, consult `node_lifecycle_registry.get_node_lifecycle(node_type)` to determine A1-phase property requirements and whether `ReconstructNode` is needed after the operation.
- `strict_mode=True` in `apply_blueprint_patch` blocks restricted node types (MacroInstance, ComponentBoundEvent, Timeline) — handlers must pass this flag through to `tools/patch.py`.

### Testing Requirements
- Unit test each handler in isolation using mocked `unreal` module if needed.
- Integration test via MCP server using `test_mcp_commands.py`.
- Verify error cases: missing parameters, invalid actor IDs, malformed input.

### Common Patterns
- Use `utils.logging` for all debug output — Unreal logs appear in `Output.log`.
- Use `utils.unreal_conversions` to translate Python tuples ↔ Unreal vectors/rotators.
- Check actor validity before accessing properties: `unreal.get_actor(actor_id)` may return None.
- Commands that spawn/delete actors should update the scene cache: see `basic_commands.py` for pattern.

### Dependencies
- `base_handler.py` — abstract interface all handlers inherit
- `utils.logging` — standard logging interface
- `utils.unreal_conversions` — tuple ↔ Unreal type conversion
- `command_registry.py` — `@registry.command()` decorator (separate module)
- `unreal` — Unreal Engine Python API (auto-available in UE Editor)

## Data Flow

```
MCP Client (Claude Code)
    ↓
mcp_server.py (tool dispatcher)
    ↓
command_registry (handler lookup)
    ↓
handlers/{{ basic_commands | actor_commands | ... }} (@registry.command)
    ↓
unreal.* (Python API)
```

Each handler:
1. Receives command dict from dispatcher
2. Validates parameters
3. Calls Unreal Python API
4. Catches exceptions
5. Returns response dict

<!-- MANUAL: -->
