# Blueprint Skeleton Templates

Use these ready-made `apply_blueprint_spec` JSON templates when creating new Blueprint logic from scratch. Always run with `dry_run: true` first to validate, then remove `dry_run` to apply.

---

## Template 1: Event-Driven Logic Chain

**Use when**: Adding logic that runs when an event fires (BeginPlay, custom trigger, etc.)

**Task classification**: `build_new_flow`

```json
{
  "blueprint": "/Game/Blueprints/BP_TARGET",
  "graphs": {
    "EventGraph": {
      "nodes": [
        {
          "ref_id": "begin",
          "node_type": "K2Node_Event",
          "pin_values": {"EventReference": "EventBeginPlay"}
        },
        {
          "ref_id": "action",
          "node_type": "K2Node_CallFunction",
          "function_name": "PrintString",
          "pin_values": {"InString": "Ready"}
        }
      ],
      "edges": [
        {"from": "begin.then", "to": "action.execute"}
      ]
    }
  },
  "compile": true,
  "dry_run": true
}
```

**Customize**: Replace `PrintString` with your actual action node. Add more nodes in `nodes` array and wire them via `edges`.

---

## Template 2: Trigger-Response Flow

**Use when**: An actor should respond to overlap, hit, or interaction events.

**Task classification**: `build_new_flow`

```json
{
  "blueprint": "/Game/Blueprints/BP_TARGET",
  "custom_events": [
    {"name": "OnTriggered", "inputs": []}
  ],
  "graphs": {
    "EventGraph": {
      "nodes": [
        {
          "ref_id": "overlap",
          "node_type": "K2Node_ComponentBoundEvent",
          "pin_values": {"DelegatePropertyName": "OnComponentBeginOverlap"}
        },
        {
          "ref_id": "response",
          "node_type": "K2Node_CustomEvent",
          "pin_values": {"CustomFunctionName": "OnTriggered"}
        },
        {
          "ref_id": "action",
          "node_type": "K2Node_CallFunction",
          "function_name": "PrintString",
          "pin_values": {"InString": "Triggered!"}
        }
      ],
      "edges": [
        {"from": "response.then", "to": "action.execute"}
      ]
    }
  },
  "compile": true,
  "dry_run": true
}
```

**Note**: `ComponentBoundEvent` is L3 (restricted) — requires `strict_mode=False` and manual component binding in UE Editor after creation. For simpler triggers, use `K2Node_InputKey` or `K2Node_Event` (overlap) instead.

**Simpler alternative** (no component binding needed):
```json
{
  "blueprint": "/Game/Blueprints/BP_TARGET",
  "custom_events": [
    {"name": "OnTriggered", "inputs": []}
  ],
  "graphs": {
    "EventGraph": {
      "nodes": [
        {
          "ref_id": "evt",
          "node_type": "K2Node_CustomEvent",
          "pin_values": {"CustomFunctionName": "OnTriggered"}
        },
        {
          "ref_id": "action",
          "node_type": "K2Node_CallFunction",
          "function_name": "PrintString",
          "pin_values": {"InString": "Triggered!"}
        }
      ],
      "edges": [
        {"from": "evt.then", "to": "action.execute"}
      ]
    }
  },
  "compile": true,
  "dry_run": true
}
```

---

## Template 3: State-Driven Branch Chain

**Use when**: Logic needs to check a variable condition before proceeding.

**Task classification**: `conditional_wrap` or `build_new_flow`

```json
{
  "blueprint": "/Game/Blueprints/BP_TARGET",
  "variables": [
    {"name": "bIsActive", "type": "bool", "default": false}
  ],
  "graphs": {
    "EventGraph": {
      "nodes": [
        {
          "ref_id": "begin",
          "node_type": "K2Node_Event",
          "pin_values": {"EventReference": "EventBeginPlay"}
        },
        {
          "ref_id": "get_state",
          "node_type": "K2Node_VariableGet",
          "pin_values": {"VariableName": "bIsActive"}
        },
        {
          "ref_id": "branch",
          "node_type": "K2Node_IfThenElse"
        },
        {
          "ref_id": "on_true",
          "node_type": "K2Node_CallFunction",
          "function_name": "PrintString",
          "pin_values": {"InString": "Active!"}
        },
        {
          "ref_id": "on_false",
          "node_type": "K2Node_CallFunction",
          "function_name": "PrintString",
          "pin_values": {"InString": "Inactive."}
        }
      ],
      "edges": [
        {"from": "begin.then", "to": "branch.execute"},
        {"from": "get_state.bIsActive", "to": "branch.Condition"},
        {"from": "branch.then", "to": "on_true.execute"},
        {"from": "branch.else", "to": "on_false.execute"}
      ]
    }
  },
  "compile": true,
  "dry_run": true
}
```

**Customize**: Replace `bIsActive` with your variable name. Replace `PrintString` nodes with actual True/False branch logic.

**Note**: For wrapping existing logic with a Branch, prefer `wrap_exec_chain_with_branch` over this template — it handles the rewiring automatically.

---

## Usage Instructions

1. Copy the template JSON
2. Replace `/Game/Blueprints/BP_TARGET` with your Blueprint path
3. Run with `dry_run: true` first:
   ```
   apply_blueprint_spec(spec_json=<template_with_dry_run_true>)
   ```
4. Review the `ops` array in the response — check for errors
5. Remove `"dry_run": true` and run again to apply
6. Call `compile_blueprint_with_errors(path)` to verify
7. Call `analyze_blueprint_quality(path, mode="quick")` to check structure
