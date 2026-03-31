# Common Patch Patterns

## Insert function call between two existing nodes

```json
{
  "add_nodes": [
    {"ref_id": "NewCall", "function_name": "MyFunction", "position": [800, 0]}
  ],
  "remove_connections": [
    {"from": "<prev_guid>.then", "to": "<next_guid>.execute"}
  ],
  "add_connections": [
    {"from": "<prev_guid>.then", "to": "NewCall.execute"},
    {"from": "NewCall.then", "to": "<next_guid>.execute"}
  ]
}
```

## Add a branch with two paths

```json
{
  "add_nodes": [
    {"ref_id": "MyBranch", "node_type": "K2Node_IfThenElse", "position": [600, 0]},
    {"ref_id": "TrueAction", "function_name": "PrintString", "pin_values": {"InString": "Yes"}},
    {"ref_id": "FalseAction", "function_name": "PrintString", "pin_values": {"InString": "No"}}
  ],
  "add_connections": [
    {"from": "<prev>.then", "to": "MyBranch.execute"},
    {"from": "MyBranch.True", "to": "TrueAction.execute"},
    {"from": "MyBranch.False", "to": "FalseAction.execute"}
  ]
}
```

## Set a variable

```json
{
  "add_nodes": [
    {"ref_id": "SetVar", "node_type": "K2Node_VariableSet",
     "function_name": "MyVariable", "pin_values": {"MyVariable": "42"}}
  ],
  "add_connections": [
    {"from": "<prev>.then", "to": "SetVar.execute"}
  ]
}
```

## Add a Delay

```json
{
  "add_nodes": [
    {"ref_id": "Wait", "function_name": "Delay", "pin_values": {"Duration": "3.0"}}
  ],
  "add_connections": [
    {"from": "<prev>.then", "to": "Wait.execute"},
    {"from": "Wait.then", "to": "<next>.execute"}
  ]
}
```
Note: Delay uses `then` pin for completion output (not `Completed`).

## Create new blueprint with initial logic

Use `create_blueprint_from_template` instead of `apply_blueprint_patch`:
```
create_blueprint_from_template(
  blueprint_name="BP_MyActor",
  parent_class="Actor",
  save_path="/Game/Blueprints",
  patch_json='{"add_nodes": [...], "add_connections": [...]}'
)
```
