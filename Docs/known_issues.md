# Known Issues — MCP Blueprint Toolchain

## ISSUE-001: ForEachLoop (K2Node_MacroInstance) cannot be created via MCP patch

**Status:** Resolved (2026-04-09)  
**Priority:** Medium  
**Affects:** `apply_blueprint_patch`, `apply_blueprint_spec`

### Description

`ForEachLoop` is a UE built-in macro node (`K2Node_MacroInstance` referencing `StandardMacros.ForEachLoop`). It is not registered in the function/node catalog, so:

- `search_blueprint_nodes(query="ForEachLoop")` returns no match
- `inspect_blueprint_node(canonical_name="ForEachLoop")` returns `Cannot resolve`
- Attempting to add it via `apply_blueprint_patch` with `node_type: K2Node_MacroInstance` is unreliable — pin names (Loop Body, Completed, Array Element, Array Index) cannot be pre-validated

### Impact

Any Blueprint logic that requires iterating an array via ForEach cannot be fully automated through MCP. The agent must ask the user to manually place the ForEach node in the UE Editor, then wire remaining nodes via MCP.

### Workaround (current)

1. User manually places a `ForEach Loop` node in the Blueprint graph in UE Editor
2. Agent uses `describe_blueprint` to get the node's GUID after placement
3. Agent completes all other node additions and connections via `apply_blueprint_patch`

### Suggested Fix

Add a dedicated MCP tool or special `node_type` handler for UE standard macro nodes:

```python
# Option A: add a macro node type alias in the MCP server
# In handlers/blueprint_commands.py, add a resolver for:
#   node_type = "ForEachLoop"  →  K2Node_MacroInstance + MacroGraphReference=/Script/Engine.StandardMacros:ForEachLoop

# Option B: expose a high-level "add_foreach_node" tool
# that internally creates K2Node_MacroInstance with correct macro reference
# and returns the pin names: execute, Array, Loop Body, Array Element, Array Index, Completed
```

Pin schema for reference (once created manually):
| Pin Name | Direction | Type |
|----------|-----------|------|
| `execute` | Input exec | — |
| `Array` | Input data | Wildcard array |
| `Loop Body` | Output exec | — |
| `Array Element` | Output data | Wildcard |
| `Array Index` | Output data | int |
| `Completed` | Output exec | — |

### Resolution

Implemented via a Python-level alias resolver in `Content/Python/tools/patch.py`.

**Supported aliases** (use directly as `node_type` in `apply_blueprint_patch`):
- `ForEachLoop` → `K2Node_MacroInstance` + `StandardMacros:ForEachLoop`
- `ForEachLoopWithBreak` → `K2Node_MacroInstance` + `StandardMacros:ForEachLoopWithBreak`
- `WhileLoop` → `K2Node_MacroInstance` + `StandardMacros:WhileLoop`
- `Gate` → `K2Node_MacroInstance` + `StandardMacros:Gate`
- `DoOnce` → `K2Node_MacroInstance` + `StandardMacros:DoOnce`

**Usage example:**
```json
{
  "add_nodes": [
    {
      "ref_id": "loop",
      "node_type": "ForEachLoop",
      "pin_values": {}
    }
  ]
}
```

The alias resolver (`resolve_node_type_alias` in `tools/patch.py`) is called automatically inside `_patch_mutate` before each node is sent to UE. No changes to the patch format are required — users just write `"node_type": "ForEachLoop"`.

**Note**: `static_schema_preflight: false` for these nodes — pin types are wildcard and cannot be pre-validated. Connect to `Loop Body`, `Array Element`, `Array Index`, and `Completed` pins by name after creation.

### Related Files

- `Content/Python/tools/patch.py` — `_MACRO_NODE_ALIASES`, `resolve_node_type_alias()`
- `Content/Python/node_lifecycle_registry.py` — `ForEachLoop` alias entry
- `Content/Python/tests/smoke_tests.py` — `smoke_macro_alias_resolver()` test
