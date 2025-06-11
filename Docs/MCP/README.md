# Model Control Protocol (MCP)

Comprehensive documentation for the UnrealGenAISupport Model Control Protocol implementation. MCP enables AI assistants to directly control and manipulate Unreal Engine projects through a sophisticated dual-server architecture.

## Overview

The UnrealGenAISupport MCP system provides revolutionary AI-controlled automation for Unreal Engine development. Through 25+ specialized tools, AI assistants can create Blueprints, manipulate scenes, manage project files, and execute complex development tasks automatically.

### Key Capabilities

- **Blueprint Automation**: Complete Blueprint lifecycle from creation to compilation
- **Scene Control**: Object spawning, material creation, and transformation management
- **Project Management**: File operations, folder creation, and asset organization
- **Advanced Node Management**: Sophisticated Blueprint node creation and connection
- **UI/UMG Integration**: Widget creation and property manipulation
- **Python & Console Integration**: Execute scripts and console commands safely
- **Real-time Feedback**: Comprehensive error handling with intelligent suggestions

## Quick Start

### 1. Enable Auto-Start (Recommended)

Configure automatic MCP server startup in Unreal Editor:

1. **Open Project Settings** → **Plugins** → **Generative AI Support**
2. **Enable "Auto Start Socket Server"**
3. **Restart Unreal Editor**

### 2. Configure AI Client

Add MCP server to your AI client configuration:

**Claude Desktop** (`claude_desktop_config.json`):
```json
{
    "mcpServers": {
        "unreal": {
            "command": "python",
            "args": ["/path/to/UnrealGenAISupport/Content/Python/mcp_server.py"],
            "env": {
                "UNREAL_HOST": "localhost",
                "UNREAL_PORT": "9877"
            }
        }
    }
}
```

**Cursor IDE** (`.cursor/mcp.json`):
```json
{
    "mcpServers": {
        "unreal": {
            "command": "python",
            "args": ["/path/to/UnrealGenAISupport/Content/Python/mcp_server.py"],
            "env": {
                "UNREAL_HOST": "localhost",
                "UNREAL_PORT": "9877"
            }
        }
    }
}
```

### 3. Test Connection

Ask your AI assistant:
> "Can you test the Unreal Engine MCP connection?"

The AI should respond with connection status and available tools.

## Architecture Overview

### Dual-Server Design

The MCP system uses a sophisticated dual-server architecture for maximum flexibility and safety:

```
AI Client (Claude/Cursor)
    ↓ MCP Protocol
External MCP Server (Python)
    ↓ JSON/TCP Socket (localhost:9877)
Internal Unreal Socket Server (Python)
    ↓ Unreal Python API
Unreal Engine Editor & C++ Utilities
```

#### External MCP Server
- **File**: `Content/Python/mcp_server.py`
- **Purpose**: Interface with AI clients using MCP protocol
- **Features**: 25+ MCP tools, safety checks, user confirmations
- **Framework**: Built with FastMCP for robust communication

#### Internal Unreal Socket Server
- **File**: `Content/Python/unreal_socket_server.py`
- **Purpose**: Execute commands within Unreal Engine
- **Features**: Command dispatching, error handling, timeout protection
- **Integration**: Direct access to Unreal Python API and C++ utilities

### Communication Protocol

- **Transport**: TCP socket on localhost:9877
- **Format**: JSON messages with command/response structure
- **Safety**: 10-second timeout, incomplete data recovery
- **Logging**: Comprehensive error tracking and debugging

## Documentation Structure

### 📋 [Server Setup](Server-Setup.md)
Complete installation and configuration guide for all supported AI clients.

- Installation requirements and dependencies
- Step-by-step setup for Claude Desktop and Cursor IDE
- Manual and automatic startup procedures
- Network configuration and troubleshooting

### 🛠️ [Command Handlers](Command-Handlers.md)
Detailed documentation of all 25+ MCP tools and their capabilities.

- Blueprint creation and manipulation tools
- Scene control and object management
- Project file operations
- Advanced node management and connections
- UI/UMG widget integration
- Python and console command execution

### 🔧 [Troubleshooting](Troubleshooting.md)
Common issues, debugging techniques, and performance optimization.

- Connection problems and solutions
- Blueprint compilation issues
- Node creation and connection troubleshooting
- Performance optimization strategies
- Error message interpretation

## Core Features

### Blueprint Automation

Complete Blueprint lifecycle automation:

```python
# Example AI prompt:
"Create a PlayerController Blueprint with:
- Float variable 'MovementSpeed' (default 600)
- Input action binding for WASD movement
- Function to handle movement input
- Connect all nodes properly"
```

**Available Tools:**
- `create_blueprint` - Create new Blueprint classes
- `add_variable_to_blueprint` - Add typed variables with defaults
- `add_function_to_blueprint` - Create functions with parameters
- `add_node_to_blueprint` - Add any type of Blueprint node
- `connect_blueprint_nodes` - Connect node pins intelligently
- `compile_blueprint` - Compile and validate Blueprints

### Scene Control

Direct manipulation of level content:

```python
# Example AI prompt:
"Spawn 5 cubes in a line, create a red material, 
and apply it to all cubes. Space them 200 units apart."
```

**Available Tools:**
- `spawn_object` - Spawn actors, shapes, or custom meshes
- `create_material` - Create materials with custom colors
- `get_all_scene_objects` - Retrieve scene information
- `edit_component_property` - Modify object properties

### Project Management

Organize and manage project assets:

```python
# Example AI prompt:
"Create a folder structure for my RPG project with 
folders for Characters, Weapons, Environments, and UI"
```

**Available Tools:**
- `create_project_folder` - Create organized folder structures
- `get_files_in_folder` - Inspect project organization
- `execute_python_script` - Run custom automation scripts

## AI Integration Examples

### Creating a Complete Game Feature

```python
# AI Prompt Example:
"""
Create a complete pickup system for my game:

1. Create a "Pickup" Blueprint derived from Actor
2. Add a StaticMeshComponent called "MeshComponent"
3. Add a SphereComponent called "CollisionSphere" 
4. Add variables:
   - PickupValue (integer, default 10)
   - bIsCollected (boolean, default false)
5. Create an "OnPickup" function that:
   - Sets bIsCollected to true
   - Destroys the actor
6. Connect collision sphere's OnComponentBeginOverlap to OnPickup
7. Compile the Blueprint
8. Spawn an instance in the level
"""
```

The AI will automatically execute all necessary MCP commands to create this complete feature.

### Blueprint Node Management

```python
# AI Prompt Example:
"""
In my PlayerController Blueprint, add nodes to:
1. Get the possessed pawn
2. Cast it to my custom Character class
3. Call a "TakeDamage" function on it
4. Connect this to an Input Action for testing
"""
```

The AI will use advanced node creation and connection tools to build complex node graphs.

### UI/UMG Creation

```python
# AI Prompt Example:
"""
Create a health bar UI widget with:
1. A background image
2. A progress bar for health
3. A text label showing current/max health
4. Position everything properly with anchors
"""
```

The AI can create and configure UMG widgets with proper layout and styling.

## Safety Features

### Built-in Safety Mechanisms

- **Destructive Operation Detection**: Automatic identification of potentially harmful commands
- **User Confirmation**: Explicit confirmation required for destructive actions
- **Timeout Protection**: Commands automatically timeout after 10 seconds
- **Error Recovery**: Robust handling of incomplete or malformed commands

### Command Validation

```python
# Automatic safety checks for:
- File deletion operations
- Project-wide modifications
- Blueprint compilation conflicts
- Invalid property assignments
```

### Intelligent Error Handling

The system provides intelligent error recovery:

- **Node Suggestions**: Alternative node types when lookups fail
- **Pin Information**: Available pins when connections fail
- **Property Suggestions**: Valid property names when assignments fail
- **Unreal Log Integration**: Relevant error context from Unreal Engine

## Performance Considerations

### Optimization Strategies

1. **Bulk Operations**: Use bulk tools for multiple similar operations
2. **Sequential Processing**: Complete one Blueprint operation before starting another
3. **Node Spacing**: Maintain proper spacing (400x300 units) to prevent overlap
4. **Resource Management**: Automatic cleanup of temporary files and objects

### Known Limitations

1. **Blueprint Node Connections**: Can be unreliable with very complex graphs
2. **No Undo/Redo**: MCP operations don't integrate with Unreal's undo system
3. **Thread Safety**: All operations must execute on Unreal's main thread
4. **Async Operations**: Some complex operations may timeout

### Best Practices

```python
# Recommended patterns:
1. Test simple operations before complex ones
2. Use specific node types (e.g., "Multiply_FloatFloat" vs "Multiply")
3. Verify pin names (use "then" for execution pins)
4. Check Blueprint compilation status after major changes
5. Use get_node_suggestions for troubleshooting
```

## Getting Started Examples

### Basic Scene Setup

Ask your AI assistant:
> "Create a simple scene with a ground plane, 3 colored cubes, and a basic light setup"

### Blueprint Creation

Ask your AI assistant:
> "Create a simple door Blueprint that opens when the player gets close"

### Project Organization

Ask your AI assistant:
> "Set up a proper folder structure for a third-person action game"

### Advanced Automation

Ask your AI assistant:
> "Create a complete inventory system with item pickup, storage, and UI display"

## Monitoring and Debugging

### Real-time Feedback

The MCP system provides comprehensive feedback for all operations:

- **Success Confirmations**: Clear success messages with details
- **Error Explanations**: Detailed error context with solutions
- **Progress Updates**: Step-by-step operation feedback
- **Suggestion System**: Automatic alternatives for failed operations

### Debugging Tools

- **Connection Testing**: `handshake_test` tool for connectivity verification
- **Node Inspection**: `get_all_nodes_in_graph` for Blueprint analysis
- **Property Validation**: Automatic property name and type checking
- **Log Integration**: Unreal Engine log capture for comprehensive debugging

---

This MCP system represents a breakthrough in AI-assisted game development, enabling unprecedented automation and control over Unreal Engine projects while maintaining safety and providing intelligent error recovery. For detailed implementation guides, refer to the specific documentation sections above.