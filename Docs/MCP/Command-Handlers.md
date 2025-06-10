# MCP Command Handlers

Comprehensive reference for all 25+ Model Control Protocol (MCP) tools available in the UnrealGenAISupport plugin. These tools enable AI assistants to perform sophisticated automation tasks within Unreal Engine.

## Overview

The MCP system provides specialized command handlers organized into functional categories. Each tool includes safety checks, intelligent error handling, and detailed feedback to ensure reliable automation.

### Tool Categories

- **[Blueprint Management](#blueprint-management)** - Complete Blueprint lifecycle automation
- **[Scene Control](#scene-control)** - Object spawning and scene manipulation
- **[Node Operations](#node-operations)** - Advanced Blueprint node management
- **[Component & Property Editing](#component--property-editing)** - Component manipulation and property setting
- **[Project Management](#project-management)** - File operations and project organization
- **[UI/UMG Integration](#uiumg-integration)** - User interface widget management
- **[System Integration](#system-integration)** - Python scripts and console commands
- **[Utility Functions](#utility-functions)** - Testing and documentation access

## Blueprint Management

### create_blueprint

Create new Blueprint classes from parent classes.

**Parameters:**
- `blueprint_name` (string): Name for the new Blueprint
- `parent_class` (string): Parent class name (e.g., "Actor", "Pawn", "Character")
- `save_path` (string, optional): Asset path for saving (default: "/Game/Blueprints")

**Example Usage:**
```python
# AI Prompt:
"Create a PlayerController Blueprint called 'MyPlayerController'"

# MCP Tool Call:
{
    "blueprint_name": "MyPlayerController",
    "parent_class": "PlayerController",
    "save_path": "/Game/Controllers"
}
```

**Returns:**
- Success: Blueprint path and compilation status
- Error: Detailed error message with suggestions

### compile_blueprint

Compile existing Blueprints to validate and apply changes.

**Parameters:**
- `blueprint_path` (string): Full asset path to the Blueprint

**Example Usage:**
```python
# AI Prompt:
"Compile the PlayerController Blueprint"

# MCP Tool Call:
{
    "blueprint_path": "/Game/Controllers/MyPlayerController"
}
```

**Returns:**
- Success: Compilation result and any warnings
- Error: Compilation errors with line numbers and suggestions

### spawn_blueprint_actor

Spawn instances of Blueprint actors in the current level.

**Parameters:**
- `blueprint_path` (string): Path to the Blueprint asset
- `location` (array, optional): World location [X, Y, Z] (default: [0, 0, 0])
- `rotation` (array, optional): World rotation [Pitch, Yaw, Roll] (default: [0, 0, 0])
- `scale` (array, optional): Scale [X, Y, Z] (default: [1, 1, 1])

**Example Usage:**
```python
# AI Prompt:
"Spawn my PlayerController Blueprint at location (500, 200, 100)"

# MCP Tool Call:
{
    "blueprint_path": "/Game/Controllers/MyPlayerController",
    "location": [500, 200, 100],
    "rotation": [0, 45, 0]
}
```

## Scene Control

### spawn_object

Spawn various types of objects in the current level.

**Parameters:**
- `object_type` (string): Type of object to spawn
  - Basic shapes: "Cube", "Sphere", "Cylinder", "Cone"
  - Actor types: "Actor", "Pawn", "Character"
  - Custom meshes: Asset path to static mesh
- `location` (array, optional): World location [X, Y, Z]
- `rotation` (array, optional): World rotation [Pitch, Yaw, Roll]
- `scale` (array, optional): Scale [X, Y, Z]
- `name` (string, optional): Custom name for the spawned object

**Example Usage:**
```python
# AI Prompt:
"Spawn 3 cubes in a line, spaced 200 units apart"

# MCP Tool Calls:
{
    "object_type": "Cube",
    "location": [0, 0, 0],
    "name": "Cube1"
}
{
    "object_type": "Cube", 
    "location": [200, 0, 0],
    "name": "Cube2"
}
{
    "object_type": "Cube",
    "location": [400, 0, 0], 
    "name": "Cube3"
}
```

**Supported Object Types:**
- **Basic Shapes**: Cube, Sphere, Cylinder, Cone
- **Actor Classes**: Actor, Pawn, Character, PlayerStart
- **Custom Meshes**: Any valid StaticMesh asset path

### create_material

Create materials with specified properties.

**Parameters:**
- `material_name` (string): Name for the new material
- `base_color` (array): RGB color values [R, G, B] (0.0-1.0 range)
- `metallic` (float, optional): Metallic value (0.0-1.0)
- `roughness` (float, optional): Roughness value (0.0-1.0)
- `save_path` (string, optional): Asset path for saving

**Example Usage:**
```python
# AI Prompt:
"Create a red metallic material called 'RedMetal'"

# MCP Tool Call:
{
    "material_name": "RedMetal",
    "base_color": [1.0, 0.0, 0.0],
    "metallic": 0.8,
    "roughness": 0.2,
    "save_path": "/Game/Materials"
}
```

### get_all_scene_objects

Retrieve information about all actors in the current level.

**Parameters:** None

**Example Usage:**
```python
# AI Prompt:
"Show me all objects currently in the scene"

# Returns:
{
    "actors": [
        {
            "name": "Cube1",
            "class": "StaticMeshActor", 
            "location": [0, 0, 0],
            "rotation": [0, 0, 0]
        },
        {
            "name": "PlayerStart",
            "class": "PlayerStart",
            "location": [100, 200, 50],
            "rotation": [0, 90, 0]
        }
    ],
    "total_count": 2
}
```

## Node Operations

### add_node_to_blueprint

Add nodes to Blueprint graphs with intelligent type mapping.

**Parameters:**
- `blueprint_path` (string): Path to the Blueprint asset
- `graph_name` (string): Graph name ("EventGraph", "ConstructionScript", or function name)
- `node_type` (string): Type of node to add
- `position` (array, optional): Node position [X, Y]
- `node_name` (string, optional): Custom name for the node

**Example Usage:**
```python
# AI Prompt:
"Add a Print String node to the EventGraph"

# MCP Tool Call:
{
    "blueprint_path": "/Game/Blueprints/MyActor",
    "graph_name": "EventGraph", 
    "node_type": "Print String",
    "position": [400, 300]
}
```

**Supported Node Types:**
- **Function Calls**: Any blueprint function or library function
- **Math Operations**: "Add", "Multiply", "Subtract", "Divide"
- **Control Flow**: "Branch", "ForLoop", "WhileLoop", "Sequence"
- **Events**: "BeginPlay", "Tick", "InputAction", "Collision"
- **Variables**: "Get [VariableName]", "Set [VariableName]"
- **Casts**: "Cast to [ClassName]"

### connect_blueprint_nodes

Connect pins between Blueprint nodes.

**Parameters:**
- `blueprint_path` (string): Path to the Blueprint asset
- `graph_name` (string): Graph name
- `connections` (array): Array of connection objects
  - `from_node` (string): Source node identifier
  - `from_pin` (string): Source pin name
  - `to_node` (string): Target node identifier  
  - `to_pin` (string): Target pin name

**Example Usage:**
```python
# AI Prompt:
"Connect the BeginPlay event to a Print String node"

# MCP Tool Call:
{
    "blueprint_path": "/Game/Blueprints/MyActor",
    "graph_name": "EventGraph",
    "connections": [
        {
            "from_node": "BeginPlay",
            "from_pin": "then",
            "to_node": "Print String", 
            "to_pin": "exec"
        }
    ]
}
```

### connect_blueprint_nodes_bulk

Efficiently connect multiple node pairs in a single operation.

**Parameters:**
- `blueprint_path` (string): Path to the Blueprint asset
- `graph_name` (string): Graph name
- `connections` (array): Large array of connection objects

**Example Usage:**
```python
# AI Prompt:
"Create a complex math calculation with multiple nodes and connections"

# MCP Tool Call:
{
    "blueprint_path": "/Game/Blueprints/Calculator",
    "graph_name": "EventGraph",
    "connections": [
        {"from_node": "InputA", "from_pin": "Value", "to_node": "Multiply", "to_pin": "A"},
        {"from_node": "InputB", "from_pin": "Value", "to_node": "Multiply", "to_pin": "B"},
        {"from_node": "Multiply", "from_pin": "ReturnValue", "to_node": "Add", "to_pin": "A"},
        {"from_node": "InputC", "from_pin": "Value", "to_node": "Add", "to_pin": "B"}
    ]
}
```

### get_node_suggestions

Get suggestions for unrecognized node types.

**Parameters:**
- `partial_name` (string): Partial or incorrect node name

**Example Usage:**
```python
# When node creation fails, automatic suggestion:
{
    "partial_name": "Print",
    "suggestions": [
        "Print String",
        "Print Text", 
        "Print Vector",
        "Print Float"
    ]
}
```

### delete_node_from_blueprint

Remove nodes from Blueprint graphs.

**Parameters:**
- `blueprint_path` (string): Path to the Blueprint asset
- `graph_name` (string): Graph name
- `node_identifier` (string): Node to delete

### get_all_nodes_in_graph

Retrieve all nodes in a Blueprint graph with their properties.

**Parameters:**
- `blueprint_path` (string): Path to the Blueprint asset
- `graph_name` (string): Graph name

**Returns:**
```python
{
    "nodes": [
        {
            "name": "BeginPlay",
            "type": "Event",
            "position": [0, 0],
            "guid": "12345-ABCDE-67890"
        },
        {
            "name": "Print String",
            "type": "Function Call",
            "position": [400, 300], 
            "guid": "67890-FGHIJ-12345"
        }
    ]
}
```

### get_blueprint_node_guid

Get the GUID of specific nodes (useful for connecting to existing nodes).

**Parameters:**
- `blueprint_path` (string): Path to the Blueprint asset
- `graph_name` (string): Graph name
- `node_name` (string): Name of the node

## Component & Property Editing

### add_component_to_blueprint

Add components to Blueprint classes.

**Parameters:**
- `blueprint_path` (string): Path to the Blueprint asset
- `component_class` (string): Component class name
- `component_name` (string): Name for the component
- `parent_component` (string, optional): Parent component name

**Example Usage:**
```python
# AI Prompt:
"Add a StaticMeshComponent called 'WeaponMesh' to my Weapon Blueprint"

# MCP Tool Call:
{
    "blueprint_path": "/Game/Weapons/BaseWeapon",
    "component_class": "StaticMeshComponent", 
    "component_name": "WeaponMesh",
    "parent_component": "RootComponent"
}
```

**Supported Component Classes:**
- **Mesh Components**: StaticMeshComponent, SkeletalMeshComponent
- **Collision**: BoxComponent, SphereComponent, CapsuleComponent
- **Physics**: RigidBodyComponent, PhysicsConstraintComponent
- **Audio**: AudioComponent
- **Rendering**: LightComponent, ParticleSystemComponent
- **Input**: InputComponent
- **Custom**: Any custom component class

### add_component_with_events

Add components and automatically create overlap event handling.

**Parameters:**
- `blueprint_path` (string): Path to the Blueprint asset
- `component_class` (string): Component class name
- `component_name` (string): Name for the component
- `create_overlap_events` (boolean): Whether to create overlap events

### edit_component_property

Edit properties of components in Blueprints or scene actors.

**Parameters:**
- `target_path` (string): Path to Blueprint or scene actor
- `component_name` (string): Name of the component
- `property_name` (string): Property to modify
- `property_value` (varies): New value for the property
- `property_type` (string, optional): Type hint for the property

**Example Usage:**
```python
# AI Prompt:
"Set the WeaponMesh component's static mesh to the sword asset"

# MCP Tool Call:
{
    "target_path": "/Game/Weapons/BaseWeapon",
    "component_name": "WeaponMesh",
    "property_name": "StaticMesh",
    "property_value": "/Game/Meshes/Weapons/Sword.Sword",
    "property_type": "object"
}
```

**Supported Property Types:**
- **Scalar**: float, int, bool values
- **Object**: Asset references and object pointers
- **Vector**: Location, rotation, scale [X, Y, Z]
- **Color**: Color values [R, G, B, A]
- **String**: Text properties
- **Enum**: Enumeration values

## Variable and Function Management

### add_variable_to_blueprint

Add variables to Blueprint classes with proper typing.

**Parameters:**
- `blueprint_path` (string): Path to the Blueprint asset
- `variable_name` (string): Name for the variable
- `variable_type` (string): Variable type
- `default_value` (varies, optional): Default value
- `is_editable` (boolean, optional): Whether editable in editor
- `is_blueprint_readonly` (boolean, optional): Whether read-only in blueprints

**Example Usage:**
```python
# AI Prompt:
"Add a float variable called 'Health' with default value 100.0"

# MCP Tool Call:
{
    "blueprint_path": "/Game/Characters/Player",
    "variable_name": "Health",
    "variable_type": "float",
    "default_value": 100.0,
    "is_editable": true
}
```

**Supported Variable Types:**
- **Primitives**: bool, int, float, string
- **Vectors**: Vector, Rotator, Transform
- **Objects**: Actor, Component, Material, Texture
- **Arrays**: Array of any type
- **Structs**: Custom structure types
- **Enums**: Enumeration types

### add_function_to_blueprint

Create custom functions in Blueprint classes.

**Parameters:**
- `blueprint_path` (string): Path to the Blueprint asset
- `function_name` (string): Name for the function
- `inputs` (array, optional): Input parameters
- `outputs` (array, optional): Output parameters
- `is_pure` (boolean, optional): Whether function is pure (no side effects)

**Example Usage:**
```python
# AI Prompt:
"Create a function called 'TakeDamage' that takes a float amount and returns a boolean"

# MCP Tool Call:
{
    "blueprint_path": "/Game/Characters/Player",
    "function_name": "TakeDamage",
    "inputs": [
        {"name": "DamageAmount", "type": "float"}
    ],
    "outputs": [
        {"name": "WasKilled", "type": "bool"}
    ],
    "is_pure": false
}
```

## Project Management

### create_project_folder

Create folders in the project content directory.

**Parameters:**
- `folder_path` (string): Path for the new folder

**Example Usage:**
```python
# AI Prompt:
"Create a folder structure for my RPG project"

# MCP Tool Calls:
{"folder_path": "/Game/Characters"}
{"folder_path": "/Game/Characters/Player"}
{"folder_path": "/Game/Characters/Enemies"}
{"folder_path": "/Game/Weapons"}
{"folder_path": "/Game/Environments"}
```

### get_files_in_folder

List files and folders in project directories.

**Parameters:**
- `folder_path` (string): Path to examine

**Returns:**
```python
{
    "files": [
        "BaseCharacter.uasset",
        "PlayerCharacter.uasset"
    ],
    "folders": [
        "Player",
        "Enemies"
    ],
    "total_items": 4
}
```

## UI/UMG Integration

### add_widget_to_user_widget

Add widgets to UMG user interface classes.

**Parameters:**
- `widget_blueprint_path` (string): Path to the UMG widget Blueprint
- `widget_class` (string): Class of widget to add
- `widget_name` (string): Name for the widget
- `parent_widget` (string, optional): Parent widget name

**Example Usage:**
```python
# AI Prompt:
"Add a health bar progress bar to my HUD widget"

# MCP Tool Call:
{
    "widget_blueprint_path": "/Game/UI/MainHUD",
    "widget_class": "ProgressBar",
    "widget_name": "HealthBar",
    "parent_widget": "Canvas"
}
```

**Supported Widget Classes:**
- **Basic**: Button, Text, Image, ProgressBar
- **Input**: EditableText, Slider, CheckBox
- **Layout**: Canvas, HorizontalBox, VerticalBox
- **Advanced**: ListView, TreeView, ScrollBox

### edit_widget_property

Edit properties of UMG widgets.

**Parameters:**
- `widget_blueprint_path` (string): Path to the UMG widget Blueprint
- `widget_name` (string): Name of the widget
- `property_name` (string): Property to modify
- `property_value` (varies): New value

**Example Usage:**
```python
# AI Prompt:
"Set the health bar's position to top-left corner"

# MCP Tool Call:
{
    "widget_blueprint_path": "/Game/UI/MainHUD",
    "widget_name": "HealthBar",
    "property_name": "Position",
    "property_value": [50, 50]
}
```

## System Integration

### execute_python_script

Execute Python code within Unreal Engine's interpreter.

**Parameters:**
- `script_content` (string): Python code to execute
- `description` (string, optional): Description of what the script does

**Example Usage:**
```python
# AI Prompt:
"Run a Python script to print all actors in the level"

# MCP Tool Call:
{
    "script_content": """
import unreal

# Get all actors in the current level
actors = unreal.EditorLevelLibrary.get_all_level_actors()
for actor in actors:
    print(f"Actor: {actor.get_name()}, Class: {actor.get_class().get_name()}")
""",
    "description": "List all actors in the current level"
}
```

**Safety Features:**
- Automatic detection of destructive operations
- User confirmation for potentially harmful code
- Error handling with detailed feedback
- Temporary file system for script execution

### execute_unreal_command

Execute Unreal Engine console commands.

**Parameters:**
- `command` (string): Console command to execute

**Example Usage:**
```python
# AI Prompt:
"Execute the stat fps command to show frame rate"

# MCP Tool Call:
{
    "command": "stat fps"
}
```

**Safety Features:**
- Built-in safety checks for destructive commands
- Log capture for command output
- Error handling for invalid commands

## Input and Game Systems

### add_input_binding

Create input action mappings for player input.

**Parameters:**
- `action_name` (string): Name for the input action
- `key` (string): Key to bind (e.g., "W", "Space", "LeftMouseButton")
- `modifiers` (array, optional): Modifier keys ("Shift", "Ctrl", "Alt")

**Example Usage:**
```python
# AI Prompt:
"Create WASD movement input bindings"

# MCP Tool Calls:
{"action_name": "MoveForward", "key": "W"}
{"action_name": "MoveBackward", "key": "S"}
{"action_name": "MoveRight", "key": "D"}
{"action_name": "MoveLeft", "key": "A"}
```

### create_game_mode

Create game mode classes with pawn assignments.

**Parameters:**
- `game_mode_name` (string): Name for the game mode
- `default_pawn_class` (string, optional): Default pawn class
- `player_controller_class` (string, optional): Player controller class

## Utility Functions

### handshake_test

Test MCP server connectivity and functionality.

**Parameters:** None

**Returns:**
```python
{
    "status": "success",
    "message": "MCP server connected successfully",
    "tools_available": 25,
    "unreal_version": "5.1.0",
    "server_version": "1.0.0"
}
```

### how_to_use

Access built-in knowledge base and documentation.

**Parameters:** None

**Returns:** 
Comprehensive usage examples and documentation for MCP tools.

## Error Handling and Suggestions

### Automatic Error Recovery

All tools include intelligent error handling:

```python
# Example error response:
{
    "success": false,
    "error": "Node type 'Print' not found",
    "suggestions": [
        "Print String",
        "Print Text", 
        "Print Vector"
    ],
    "error_code": "NODE_NOT_FOUND",
    "help": "Use get_node_suggestions tool for more options"
}
```

### Common Error Types

- **NODE_NOT_FOUND**: Node type doesn't exist
- **BLUEPRINT_NOT_FOUND**: Blueprint asset not found
- **PROPERTY_NOT_FOUND**: Component property doesn't exist
- **CONNECTION_FAILED**: Node pin connection failed
- **COMPILATION_ERROR**: Blueprint compilation failed
- **PERMISSION_DENIED**: Operation requires user confirmation

### Best Practices for AI Usage

1. **Use Specific Names**: "Multiply_FloatFloat" instead of "Multiply"
2. **Check Pin Names**: Use "then" for execution pins, not "OutputDelegate"
3. **Sequential Operations**: Complete one Blueprint operation before starting another
4. **Error Handling**: Always check suggestions when operations fail
5. **Position Management**: Space nodes 400x300 units apart to prevent overlap

---

This comprehensive command reference enables AI assistants to perform sophisticated automation tasks within Unreal Engine while maintaining safety and providing intelligent error recovery. Each tool is designed to work seamlessly with AI reasoning and natural language instructions.