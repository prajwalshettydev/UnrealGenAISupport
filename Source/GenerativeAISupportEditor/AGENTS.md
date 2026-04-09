<!-- Parent: ../../AGENTS.md -->
<!-- Generated: 2026-04-08 | Updated: 2026-04-09 -->

# GenerativeAISupportEditor Module (C++)

## Purpose
Editor-only C++ module providing MCP Blueprint node creation, actor/widget introspection, and commandlets for Blueprint export. This is the hot path (GenBlueprintNodeCreator.cpp performs 221+ operations per graph edit) for Claude Code → Blueprint editing. Not included in packaged game builds.

## Key Files
| Category | Files | Purpose |
|----------|-------|---------|
| **Blueprint Node Creation** | `GenBlueprintNodeCreator.h/cpp` | Core MCP node creator (221x hot path) |
| **Blueprint Utilities** | `GenBlueprintUtils.h/cpp` | Graph queries, node searches, layout helpers |
| **Actor Utilities** | `GenActorUtils.h/cpp` | Scene queries, property getters/setters, spawning |
| **Widget Utilities** | `GenWidgetUtils.h/cpp` | UMG widget creation and property binding |
| **Object Properties** | `GenObjectProperties.h/cpp` | Generic property reflection (used by GenActorUtils) |
| **Editor Commands** | `GenEditorCommands.h/cpp` | Unreal editor toolbar/menu integration |
| **Editor Window** | `GenEditorWindow.h/cpp` | Dockable editor panel for MCP status |
| **Plugin Settings** | `GenerativeAISupportSettings.h/cpp` | Project settings (API keys, endpoint URLs) |
| **Blueprint Export** | `Commandlets/BlueprintExportCommandlet.h/cpp` | Batch export blueprints for LLM context |

## Architecture

### GenBlueprintNodeCreator (Hot Path)
**Location**: `Private/MCP/GenBlueprintNodeCreator.cpp`
**Call Frequency**: 221+ operations per Blueprint graph edit (most frequent MCP consumer)

#### Three-Phase Node Creation
1. **A1 Phase** (after node construction, before AllocateDefaultPins): Set member properties (CustomFunctionName, InputKey, VariableName)
2. **A2 Phase** (after AllocateDefaultPins, before connections): Pin initialization and default values
3. **A3 Phase** (after connections): Node reconstruction and compilation

#### Key Functions
| Function | Purpose |
|----------|---------|
| `AddNode` | Single node creation (async, returns node GUID) |
| `AddNodesBulk` | Multi-node batch creation in one transaction |
| `DeleteNode` | Remove node from graph |
| `GetAllNodesInGraph` | List all nodes with pins and connections |
| `GetNodeDetails` | Full introspection: pins, connections, default values, title |
| `GetNodeSuggestions` | Auto-complete hints for node types |
| `MoveNode` | Reposition node in graph |
| `ConnectNodes` | Wire two pins together |
| `GetNodeGuidByFName` | Reverse lookup node GUID from UObject FName |

#### Three-Layer Subgraphs
- **Event flows**: Event → Sequence/Branch → Function calls
- **Shared functions**: Reusable functions called from multiple event flows
- **Macros**: Inline graph expansions

#### Performance Considerations
- Dirty flag (`bNodeCreationDirty`) tracks if Blueprint modified per-call
- Only marks Blueprint dirty if nodes actually added/modified (avoids unnecessary recompiles)
- Transactions group multi-node ops into single undo entry

### Blueprint Graph Utilities (`GenBlueprintUtils`)
| Function | Purpose |
|----------|---------|
| `FindGraphByName` | Get graph GUID by name (EventGraph, function name, macro name) |
| `GetGraphNodes` | List all nodes in graph |
| `GetNodeConnections` | Find pins wired to a given pin |
| `ValidateNodeType` | Check if node type is valid in schema |
| `LayoutNodes` | Auto-arrange nodes via Python layout engine |

### Actor Utilities (`GenActorUtils`)
| Function | Purpose |
|----------|---------|
| `SpawnActor` | Create actor in level (class path or mesh path) |
| `DeleteActor` | Remove actor from level |
| `GetActorProperty` | Read actor property (generic, supports struct breakdown) |
| `SetActorProperty` | Write actor property (with validation) |
| `GetActorList` | Find actors by class, name, or tag |

### Widget Utilities (`GenWidgetUtils`)
| Function | Purpose |
|----------|---------|
| `CreateWidget` | Create UMG widget in hierarchy |
| `UpdateWidgetProperty` | Modify widget property (visibility, text, image, etc.) |
| `GetWidgetHierarchy` | List widget tree |
| `BindEventDispatcher` | Connect widget event to handler |

### Editor Commands (`GenEditorCommands`)
Registers UE toolbar/menu items:
- "Generative AI" menu in main editor menu bar
- Blueprint editor context menu entries for AI operations
- Hotkeys for common MCP tasks

### Blueprint Export Commandlet
**Usage**: `Editor -run=BlueprintExportCommandlet -directory=/Game/Blueprints`

Exports all Blueprints in a directory as JSON for LLM context:
```json
{
  "blueprints": [
    {
      "path": "/Game/BP_NPC",
      "parent": "/Script/Engine.Character",
      "variables": [ { "name": "Health", "type": "float", "default": 100 } ],
      "functions": [ { "name": "TakeDamage", "inputs": [...], "outputs": [...] } ],
      "graph_summary": "..."
    }
  ]
}
```

## Data Flow

### Node Creation Request (MCP → Blueprint)
```
MCP Client (Claude Code)
    ↓ JSON: { "command": "add_node", "blueprint_path": "...", "node_type": "K2Node_CallFunction" }
    ↓
Python: handlers/blueprint_commands.py
    ↓
C++: GenBlueprintNodeCreator::AddNode() [PHASE A1]
    ↓ (member properties set)
    ↓
C++: GenBlueprintNodeCreator::AddNode() [PHASE A2]
    ↓ (pins allocated and initialized)
    ↓
C++: K2Node::ReconstructNode()
    ↓ (compilation, error checking)
    ↓
Response: { "success": true, "node_guid": "..." }
    ↓
MCP Client receives response
```

### Property Introspection (Blueprint → MCP)
```
MCP Client requests: GetNodeDetails(blueprint_path, node_guid)
    ↓
C++: GenBlueprintNodeCreator::GetNodeDetails()
    ↓
Node pin enumeration (exec + data pins)
    ↓
Connection queries (what's wired to each pin)
    ↓
JSON serialization of all details
    ↓
Response: { "pins": [...], "connections": [...], "properties": {...} }
    ↓
MCP Client receives introspection data
```

## For AI Agents

### Working In This Directory
- **Hot path**: GenBlueprintNodeCreator.cpp is called 221+ times per Blueprint edit. Changes here need profiling verification.
- **Three-phase pattern mandatory**: A1 (member init) → A2 (pin init) → A3 (reconstruct). Do not skip phases. Phase requirements per node type are defined in `Content/Python/node_lifecycle_registry.py`.
- **Transaction management**: Always use `GEditor->BeginTransaction()` / `EndTransaction()` for undo support.
- **Dirty flag**: Set `bNodeCreationDirty = true` when nodes are added/modified to trigger recompile only when necessary.
- **High-layer structural tools** (`insert_exec_node_between`, `append_node_after_exec`, `wrap_exec_chain_with_branch`) go through Python handlers — the C++ layer only needs `AddNode`, `ConnectNodes`, and `DeleteNode` primitives.
- Do NOT modify Blueprints while they are open in the Unreal Editor UI (unrelated to MCP operations) — can cause conflicts.

### Testing Requirements
- Unit test GenBlueprintNodeCreator with mock Graph/Node objects.
- Integration test: create Blueprint via MCP, verify node count, pin count, connections match request.
- Performance test: Add 100 nodes via AddNodesBulk, measure time and memory.
- Stress test: 10,000 operations, verify no memory leaks or dangling pointers.
- Test error cases:
  - Invalid node types (should return error, not crash)
  - Missing Blueprint (should handle gracefully)
  - Circular connections (should detect and reject)
  - Pin type mismatches (should validate before connecting)

### Common Patterns
- **Validate all paths**: Node types, Blueprint paths, pin names are user input — always validate.
- **Use transactions**: Multi-node edits must be wrapped in `GEditor->BeginTransaction()`.
- **Mark dirty correctly**: Only set `bNodeCreationDirty = true` when actual nodes added/modified.
- **Cache graph GUIDs**: Looking up graphs by name is O(n) — cache results if querying same graph repeatedly.
- **Handle None pointers**: Blueprint or graph may not exist — check before dereferencing.

### Dependencies
- `Kismet2/BlueprintEditorUtils.h` — Blueprint editing utilities
- `Kismet/KismetMathLibrary.h` — Math node helpers
- `Engine/Blueprint.h` — Blueprint asset type
- `BlueprintEditor.h` — Blueprint editor interface
- `EdGraphSchema_K2.h` — K2 graph schema validation
- `UObject/UnrealTypePrivate.h` — Property reflection

## Hot Path Profile

**GenBlueprintNodeCreator.cpp (221 calls/graph)**:
- `AddNode()` — ~15ms (node creation + reconstruction)
- `ConnectNodes()` — ~2ms (pin connection validation)
- `GetNodeDetails()` — ~5ms (introspection only, no modification)

Target: keep total per-graph-edit under 5 seconds for 100+ node edits.

<!-- MANUAL: -->
