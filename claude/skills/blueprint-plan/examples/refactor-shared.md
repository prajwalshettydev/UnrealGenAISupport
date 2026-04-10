# Example: Extract Shared Logic into Function Graph

## User Request
"BP_SharedLogic 里 BeginPlay 和 Tick 都调用了同一段 Sequence→Print 链，把它提取成函数"

## Analysis
- `describe_blueprint` shows `shared_functions` subgraph with Sequence + 2 PrintString nodes
- Both BeginPlay and Tick flow into this shared Sequence

## Spec Output

### Target
- Blueprint: /Game/Blueprints/Tests/BP_SharedLogic
- Graph: EventGraph → extract to new Function graph

### Changes
1. Add function: "SharedPrintSequence"
2. Move shared nodes (Sequence, PrintString x2) into new function
3. Replace shared entry point with function call in both BeginPlay and Tick flows

### Connections
- BeginPlay.then → Call SharedPrintSequence.execute
- Tick.then → Call SharedPrintSequence.execute

### Expected Result
Shared logic lives in a reusable function graph. Both events call it without duplication.

### Risks
- Moving nodes between graphs may break existing connections
- Need to verify pin compatibility after extraction
