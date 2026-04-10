# Example: Add PrintString after BeginPlay

## User Request
"给 BP_CRUDTest 加一个 BeginPlay 后打印 Hello"

## Spec Output

### Target
- Blueprint: /Game/Blueprints/Tests/BP_CRUDTest
- Graph: EventGraph
- Current state: 3 nodes, 3 event flows

### Changes
1. Add node: PrintString at [400, 0]
   - pin_values: {"InString": "Hello"}

### Connections
- 事件开始运行.then → PrintString.execute

### Expected Result
On BeginPlay, prints "Hello" to screen.

## Corresponding Patch JSON
```json
{
  "add_nodes": [
    {"ref_id": "Print", "function_name": "PrintString", "position": [400, 0], "pin_values": {"InString": "Hello"}}
  ],
  "add_connections": [
    {"from": "事件开始运行.then", "to": "Print.execute"}
  ],
  "auto_compile": true
}
```
