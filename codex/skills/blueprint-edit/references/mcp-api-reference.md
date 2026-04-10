# MCP Edit Reference

Use these tools as the main edit path:

| Tool | Use |
|---|---|
| `describe_blueprint` | Before/after snapshots and scoped graph reads |
| `inspect_blueprint_node` | Confirm exact node schema and patch hints |
| `get_node_details` | Inspect one node's pins and typed values |
| `preflight_blueprint_patch` | Validate patch JSON before apply |
| `apply_blueprint_patch` | Surgical local edits |
| `apply_blueprint_spec` | Declarative create-or-build flows |
| `compile_blueprint_with_errors` | Post-apply verification |
| `diff_blueprint` | Change summary |
| `save_all_dirty_packages` | Persist successful edits |

Prefer read tools first, then a single verified write step.
