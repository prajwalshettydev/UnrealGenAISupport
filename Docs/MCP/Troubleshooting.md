# MCP Troubleshooting

Comprehensive troubleshooting guide for the UnrealGenAISupport Model Control Protocol (MCP) system. This guide covers common issues, debugging techniques, and performance optimization strategies.

## Connection Issues

### MCP Server Not Responding

#### Symptoms
- AI client reports "MCP server connection failed"
- No response to `handshake_test` command
- Timeout errors when executing MCP tools

#### Diagnostic Steps

**1. Check Server Process Status**

In Unreal Python Console:
```python
import subprocess
import platform

if platform.system() == "Windows":
    result = subprocess.run(["tasklist", "/FI", "IMAGENAME eq python.exe"], 
                          capture_output=True, text=True)
    print("Python processes:")
    for line in result.stdout.split('\n'):
        if 'python.exe' in line:
            print(line)
else:
    result = subprocess.run(["ps", "aux"], capture_output=True, text=True)
    mcp_processes = [line for line in result.stdout.split('\n') 
                     if 'mcp_server.py' in line]
    print("MCP processes:", mcp_processes)
```

**2. Verify Socket Connection**

Test direct socket connectivity:
```python
import socket
import json

def test_socket_connection():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)
        s.connect(('localhost', 9877))
        
        # Send handshake test
        test_command = {"command": "handshake_test", "args": {}}
        s.sendall(json.dumps(test_command).encode('utf-8'))
        
        response = s.recv(4096).decode('utf-8')
        s.close()
        
        print(f"Socket test successful: {response}")
        return True
    except Exception as e:
        print(f"Socket test failed: {e}")
        return False

test_socket_connection()
```

**3. Check Port Availability**

Verify port 9877 is available:
```python
import socket

def check_port(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    result = sock.connect_ex(('localhost', port))
    sock.close()
    
    if result == 0:
        print(f"Port {port} is in use")
        return True
    else:
        print(f"Port {port} is available")
        return False

check_port(9877)
```

#### Solutions

**Solution 1: Restart MCP Servers**

Complete restart sequence:
```python
# 1. Stop existing processes
import subprocess
import signal
import os

# Kill existing MCP processes
if os.name == 'nt':  # Windows
    subprocess.run(["taskkill", "/F", "/IM", "python.exe", "/FI", 
                   "WINDOWTITLE eq mcp_server*"], capture_output=True)
else:  # Unix-like
    subprocess.run(["pkill", "-f", "mcp_server.py"], capture_output=True)

print("Stopped existing MCP processes")

# 2. Restart socket server
exec(open(r"[ProjectPath]/Plugins/GenerativeAISupport/Content/Python/unreal_socket_server.py").read())

# 3. Restart external MCP server via auto-start or manual launch
```

**Solution 2: Alternative Port Configuration**

If port 9877 is occupied, use alternative port:

1. **Update Socket Server Config** (`Content/Python/socket_server_config.json`):
```json
{
    "host": "localhost",
    "port": 9878,
    "timeout": 10,
    "buffer_size": 4096
}
```

2. **Update AI Client Configuration**:
```json
{
    "mcpServers": {
        "unreal": {
            "command": "python",
            "args": ["/path/to/mcp_server.py"],
            "env": {
                "UNREAL_HOST": "localhost",
                "UNREAL_PORT": "9878"
            }
        }
    }
}
```

3. **Restart both servers with new port**

**Solution 3: Firewall and Security**

Check firewall settings:

**Windows:**
```cmd
# Check if Python is allowed through firewall
netsh advfirewall firewall show rule name=all | findstr Python

# Add firewall exception if needed
netsh advfirewall firewall add rule name="Unreal MCP Python" dir=in action=allow program="python.exe"
```

**macOS:**
```bash
# Check if firewall is blocking connections
sudo pfctl -s rules | grep 9877

# Add exception through System Preferences → Security & Privacy → Firewall
```

### AI Client Cannot Find MCP Server

#### Symptoms
- "No MCP servers configured" message in AI client
- MCP tools not available in AI assistant
- AI client starts without MCP functionality

#### Diagnostic Steps

**1. Verify Configuration File Location**

Check if configuration files exist:

**Claude Desktop:**
```bash
# Windows
echo %APPDATA%\Claude\claude_desktop_config.json
dir "%APPDATA%\Claude\claude_desktop_config.json"

# macOS
ls -la ~/Library/Application\ Support/Claude/claude_desktop_config.json

# Linux
ls -la ~/.config/claude/claude_desktop_config.json
```

**Cursor IDE:**
```bash
# Project root
ls -la .cursor/mcp.json
```

**2. Validate Configuration Syntax**

Test JSON syntax:
```python
import json

config_path = "/path/to/claude_desktop_config.json"
try:
    with open(config_path, 'r') as f:
        config = json.load(f)
    print("Configuration file is valid JSON")
    print("MCP servers configured:", list(config.get('mcpServers', {}).keys()))
except json.JSONDecodeError as e:
    print(f"JSON syntax error: {e}")
except FileNotFoundError:
    print("Configuration file not found")
```

**3. Verify File Paths**

Check if MCP server script exists at configured path:
```python
import os

# Path from configuration
script_path = "/absolute/path/to/mcp_server.py"
print(f"Script exists: {os.path.exists(script_path)}")
print(f"Script readable: {os.access(script_path, os.R_OK)}")

if os.path.exists(script_path):
    # Check if it's the correct script
    with open(script_path, 'r') as f:
        content = f.read(200)  # First 200 characters
        if 'FastMCP' in content or 'mcp_server' in content:
            print("Script appears to be correct MCP server")
        else:
            print("Warning: File may not be MCP server script")
```

#### Solutions

**Solution 1: Fix Configuration File**

Create or fix configuration file:

**Claude Desktop** (`claude_desktop_config.json`):
```json
{
    "mcpServers": {
        "unreal-engine": {
            "command": "python",
            "args": ["/absolute/path/to/your/project/Plugins/GenerativeAISupport/Content/Python/mcp_server.py"],
            "env": {
                "UNREAL_HOST": "localhost",
                "UNREAL_PORT": "9877"
            }
        }
    }
}
```

**Critical Requirements:**
- Use **absolute paths** (not relative)
- Verify Python executable is in PATH
- Ensure environment variables are set correctly

**Solution 2: Path Resolution**

Find correct paths automatically:
```python
import os
import sys

# Find Python executable
python_exe = sys.executable
print(f"Python executable: {python_exe}")

# Find project root (assuming script runs from project)
project_root = os.getcwd()
while not os.path.exists(os.path.join(project_root, 'GenerativeAISupport.uplugin')):
    parent = os.path.dirname(project_root)
    if parent == project_root:  # Reached filesystem root
        print("Could not find project root")
        break
    project_root = parent

mcp_script = os.path.join(project_root, 'Plugins', 'GenerativeAISupport', 
                         'Content', 'Python', 'mcp_server.py')
print(f"MCP script path: {mcp_script}")
print(f"MCP script exists: {os.path.exists(mcp_script)}")
```

**Solution 3: Restart AI Client**

After configuration changes:
1. **Completely quit** AI client (check system tray/menu bar)
2. **Wait 5 seconds** for complete shutdown
3. **Restart AI client**
4. **Verify MCP connection** with test prompt

## Blueprint Operations Issues

### Blueprint Creation Failures

#### Symptoms
- "Failed to create Blueprint" errors
- Blueprints created but not compiled
- Missing Blueprint assets in Content Browser

#### Diagnostic Steps

**1. Check Asset Path Validity**

Verify Blueprint save paths:
```python
import unreal

def validate_asset_path(asset_path):
    """Validate if asset path is properly formatted and accessible."""
    if not asset_path.startswith('/Game/'):
        print(f"Invalid path format: {asset_path}")
        return False
    
    # Check if parent directory exists
    parent_path = '/'.join(asset_path.split('/')[:-1])
    if unreal.EditorAssetLibrary.does_directory_exist(parent_path):
        print(f"Parent directory exists: {parent_path}")
        return True
    else:
        print(f"Parent directory missing: {parent_path}")
        return False

# Test with actual path
validate_asset_path("/Game/Blueprints/MyBlueprint")
```

**2. Check Blueprint Parent Class**

Verify parent class exists:
```python
import unreal

def check_parent_class(class_name):
    """Check if parent class is valid for Blueprint creation."""
    valid_classes = [
        'Actor', 'Pawn', 'Character', 'PlayerController', 'GameMode',
        'UserWidget', 'Component', 'ActorComponent', 'SceneComponent'
    ]
    
    if class_name in valid_classes:
        print(f"Valid parent class: {class_name}")
        return True
    
    # Check if it's a custom class
    try:
        class_obj = unreal.load_class(None, f"/Script/Engine.{class_name}")
        if class_obj:
            print(f"Found engine class: {class_name}")
            return True
    except:
        pass
    
    print(f"Invalid parent class: {class_name}")
    print(f"Available classes: {valid_classes}")
    return False

check_parent_class("Actor")
```

**3. Monitor Blueprint Compilation**

Check compilation status:
```python
import unreal

def check_blueprint_compilation(blueprint_path):
    """Check Blueprint compilation status and errors."""
    blueprint = unreal.EditorAssetLibrary.load_asset(blueprint_path)
    if not blueprint:
        print(f"Blueprint not found: {blueprint_path}")
        return False
    
    if hasattr(blueprint, 'blueprint_compile_errors'):
        errors = blueprint.blueprint_compile_errors
        if errors:
            print(f"Compilation errors:")
            for error in errors:
                print(f"  - {error}")
        else:
            print("No compilation errors")
    
    print(f"Blueprint status: {blueprint.get_class()}")
    return True

check_blueprint_compilation("/Game/Blueprints/MyBlueprint")
```

#### Solutions

**Solution 1: Create Missing Directories**

Create directory structure before Blueprint creation:
```python
import unreal

def ensure_directory_exists(directory_path):
    """Create directory if it doesn't exist."""
    if not unreal.EditorAssetLibrary.does_directory_exist(directory_path):
        success = unreal.EditorAssetLibrary.make_directory(directory_path)
        if success:
            print(f"Created directory: {directory_path}")
        else:
            print(f"Failed to create directory: {directory_path}")
        return success
    return True

# Example usage before Blueprint creation
ensure_directory_exists("/Game/Blueprints")
ensure_directory_exists("/Game/Blueprints/Characters")
```

**Solution 2: Fix Blueprint Compilation**

Force Blueprint recompilation:
```python
import unreal

def force_blueprint_compilation(blueprint_path):
    """Force Blueprint compilation and report results."""
    blueprint = unreal.EditorAssetLibrary.load_asset(blueprint_path)
    if not blueprint:
        print(f"Cannot load Blueprint: {blueprint_path}")
        return False
    
    try:
        # Force full compilation
        unreal.BlueprintCompilerLibrary.compile_blueprint(blueprint)
        
        # Save the asset
        unreal.EditorAssetLibrary.save_asset(blueprint_path)
        
        print(f"Successfully compiled: {blueprint_path}")
        return True
    except Exception as e:
        print(f"Compilation failed: {e}")
        return False

force_blueprint_compilation("/Game/Blueprints/MyBlueprint")
```

### Node Creation and Connection Issues

#### Symptoms
- "Node type not found" errors
- Nodes created but connections fail
- Nodes appear but don't function correctly

#### Diagnostic Steps

**1. Check Node Type Validity**

Verify node types exist in Blueprint system:
```python
import unreal

def find_available_nodes(search_term):
    """Find available node types matching search term."""
    # This would require more complex Blueprint reflection
    # For now, provide common node types
    common_nodes = {
        'Print String': 'KismetSystemLibrary.PrintString',
        'Add': 'KismetMathLibrary.Add_IntInt',
        'Multiply': 'KismetMathLibrary.Multiply_FloatFloat',
        'Branch': 'KismetSystemLibrary.Branch',
        'BeginPlay': 'Event BeginPlay',
        'Tick': 'Event Tick'
    }
    
    matches = {k: v for k, v in common_nodes.items() 
               if search_term.lower() in k.lower()}
    
    if matches:
        print(f"Found matching nodes for '{search_term}':")
        for name, full_name in matches.items():
            print(f"  - {name} ({full_name})")
    else:
        print(f"No nodes found for '{search_term}'")
        print("Available node types:", list(common_nodes.keys()))
    
    return matches

find_available_nodes("Print")
```

**2. Validate Node Connections**

Check pin compatibility:
```python
def validate_node_connection(from_node, from_pin, to_node, to_pin):
    """Validate that node connection is theoretically possible."""
    # Common pin types and their compatibility
    pin_types = {
        'exec': ['exec'],  # Execution pins
        'then': ['exec'],  # Output execution
        'boolean': ['bool', 'boolean'],
        'integer': ['int', 'integer', 'float'],
        'float': ['float', 'integer', 'int'],
        'string': ['string', 'text'],
        'object': ['object', 'actor', 'component']
    }
    
    # Execution pin connections
    exec_pins = ['exec', 'then', 'finished', 'completed']
    
    if from_pin.lower() in exec_pins and to_pin.lower() in exec_pins:
        print(f"Valid execution connection: {from_pin} → {to_pin}")
        return True
    
    # Data pin connections require type compatibility
    from_type = None
    to_type = None
    
    for pin_type, compatible in pin_types.items():
        if from_pin.lower() in compatible:
            from_type = pin_type
        if to_pin.lower() in compatible:
            to_type = pin_type
    
    if from_type and to_type:
        if from_type == to_type or to_type in pin_types.get(from_type, []):
            print(f"Valid data connection: {from_pin} ({from_type}) → {to_pin} ({to_type})")
            return True
    
    print(f"Invalid connection: {from_pin} → {to_pin}")
    return False

# Test connections
validate_node_connection("then", "exec", "Print String", "BeginPlay")
validate_node_connection("ReturnValue", "In String", "Add", "Print String")
```

#### Solutions

**Solution 1: Use Specific Node Names**

Replace generic names with specific Blueprint node names:

```python
# Instead of generic names, use specific ones:
node_mappings = {
    "Print": "Print String",
    "Add": "Add_IntInt",  # or Add_FloatFloat
    "Multiply": "Multiply_FloatFloat",
    "Get": "Get [VariableName]",
    "Set": "Set [VariableName]",
    "Cast": "Cast to [ClassName]"
}

def suggest_specific_node(generic_name):
    """Suggest specific node names for generic requests."""
    suggestions = []
    
    if "print" in generic_name.lower():
        suggestions.extend(["Print String", "Print Text", "Print Vector"])
    elif "add" in generic_name.lower():
        suggestions.extend(["Add_IntInt", "Add_FloatFloat", "Add_VectorVector"])
    elif "multiply" in generic_name.lower():
        suggestions.extend(["Multiply_IntInt", "Multiply_FloatFloat"])
    
    return suggestions

print("Suggestions for 'Print':", suggest_specific_node("Print"))
```

**Solution 2: Fix Common Pin Name Issues**

Use correct pin names for connections:

```python
# Common pin name corrections:
pin_corrections = {
    "output": "then",           # Execution output
    "input": "exec",            # Execution input
    "result": "ReturnValue",    # Function return
    "value": "ReturnValue",     # Variable get
    "new_value": "Value",       # Variable set
    "condition": "Condition",   # Branch condition
    "target": "Target"          # Function target
}

def correct_pin_name(pin_name, node_type):
    """Correct common pin name mistakes."""
    corrected = pin_corrections.get(pin_name.lower(), pin_name)
    if corrected != pin_name:
        print(f"Corrected pin name: '{pin_name}' → '{corrected}' for {node_type}")
    return corrected
```

**Solution 3: Node Position Management**

Prevent node overlap issues:

```python
def calculate_node_position(node_index, nodes_per_row=3):
    """Calculate proper node positions to prevent overlap."""
    spacing_x = 400
    spacing_y = 300
    
    row = node_index // nodes_per_row
    col = node_index % nodes_per_row
    
    x = col * spacing_x
    y = row * spacing_y
    
    return [x, y]

# Example usage
for i in range(6):
    pos = calculate_node_position(i)
    print(f"Node {i} position: {pos}")
```

## Component and Property Issues

### Component Addition Failures

#### Symptoms
- Components not appearing in Blueprint
- "Component class not found" errors
- Components added but properties not accessible

#### Diagnostic Steps

**1. Verify Component Class Names**

Check valid component classes:
```python
import unreal

def list_available_components():
    """List commonly available component classes."""
    component_classes = [
        'StaticMeshComponent',
        'SkeletalMeshComponent', 
        'BoxComponent',
        'SphereComponent',
        'CapsuleComponent',
        'AudioComponent',
        'LightComponent',
        'ParticleSystemComponent',
        'SceneComponent',
        'ActorComponent'
    ]
    
    print("Available component classes:")
    for comp_class in component_classes:
        try:
            class_obj = unreal.load_class(None, f"/Script/Engine.{comp_class}")
            if class_obj:
                print(f"  ✓ {comp_class}")
            else:
                print(f"  ✗ {comp_class}")
        except:
            print(f"  ✗ {comp_class} (failed to load)")

list_available_components()
```

**2. Check Component Hierarchy**

Verify parent-child relationships:
```python
def validate_component_hierarchy(parent_name, child_name):
    """Check if component hierarchy is valid."""
    # Scene components can have children, Actor components cannot
    scene_components = [
        'SceneComponent', 'StaticMeshComponent', 'SkeletalMeshComponent',
        'BoxComponent', 'SphereComponent', 'CapsuleComponent'
    ]
    
    if parent_name in scene_components:
        print(f"Valid parent for hierarchy: {parent_name}")
        return True
    elif parent_name == 'RootComponent':
        print("RootComponent is valid parent")
        return True
    else:
        print(f"Invalid parent for hierarchy: {parent_name}")
        print(f"Valid scene components: {scene_components}")
        return False

validate_component_hierarchy("StaticMeshComponent", "BoxComponent")
```

#### Solutions

**Solution 1: Use Correct Component Names**

Ensure component class names are exact:

```python
# Correct component class names
correct_component_names = {
    'mesh': 'StaticMeshComponent',
    'skeletal': 'SkeletalMeshComponent',
    'box': 'BoxComponent',
    'sphere': 'SphereComponent',
    'capsule': 'CapsuleComponent',
    'collision': 'BoxComponent',  # Default collision
    'audio': 'AudioComponent',
    'light': 'LightComponent',
    'particle': 'ParticleSystemComponent',
    'scene': 'SceneComponent'
}

def get_correct_component_name(user_input):
    """Get correct component name from user input."""
    user_lower = user_input.lower()
    for key, correct_name in correct_component_names.items():
        if key in user_lower:
            return correct_name
    return user_input  # Return as-is if no match
```

**Solution 2: Fix Component Property Access**

Handle component property setting correctly:

```python
def set_component_property_safe(component_name, property_name, property_value):
    """Safely set component property with validation."""
    # Common property mappings
    property_mappings = {
        'StaticMeshComponent': {
            'mesh': 'StaticMesh',
            'material': 'Material',
            'scale': 'RelativeScale3D',
            'location': 'RelativeLocation',
            'rotation': 'RelativeRotation'
        },
        'BoxComponent': {
            'size': 'BoxExtent',
            'collision': 'CollisionResponseToChannels'
        }
    }
    
    component_props = property_mappings.get(component_name, {})
    correct_property = component_props.get(property_name.lower(), property_name)
    
    if correct_property != property_name:
        print(f"Corrected property: {property_name} → {correct_property}")
    
    return correct_property, property_value
```

### Property Setting Errors

#### Symptoms
- "Property not found" errors
- Properties set but not taking effect
- Type mismatch errors

#### Diagnostic Steps

**1. Check Property Existence**

Verify properties exist on components:
```python
import unreal

def check_component_properties(component_class):
    """List available properties for a component class."""
    try:
        class_obj = unreal.load_class(None, f"/Script/Engine.{component_class}")
        if class_obj:
            print(f"Properties for {component_class}:")
            # This would require Blueprint reflection capabilities
            # For now, list common properties
            common_properties = {
                'StaticMeshComponent': ['StaticMesh', 'Material', 'RelativeLocation', 'RelativeRotation', 'RelativeScale3D'],
                'BoxComponent': ['BoxExtent', 'CollisionEnabled', 'CollisionResponseToChannels'],
                'SphereComponent': ['SphereRadius', 'CollisionEnabled']
            }
            
            props = common_properties.get(component_class, [])
            for prop in props:
                print(f"  - {prop}")
        else:
            print(f"Component class not found: {component_class}")
    except Exception as e:
        print(f"Error checking properties: {e}")

check_component_properties("StaticMeshComponent")
```

**2. Validate Property Types**

Check property type compatibility:
```python
def validate_property_type(property_name, property_value):
    """Validate property value type."""
    type_mappings = {
        'RelativeLocation': (list, tuple),  # Vector
        'RelativeRotation': (list, tuple),  # Rotator  
        'RelativeScale3D': (list, tuple),   # Vector
        'StaticMesh': str,                  # Asset path
        'Material': str,                    # Asset path
        'BoxExtent': (list, tuple),         # Vector
        'SphereRadius': (int, float),       # Number
        'CollisionEnabled': bool            # Boolean
    }
    
    expected_type = type_mappings.get(property_name)
    if expected_type:
        if isinstance(property_value, expected_type):
            print(f"Valid type for {property_name}: {type(property_value)}")
            return True
        else:
            print(f"Invalid type for {property_name}: expected {expected_type}, got {type(property_value)}")
            return False
    
    print(f"Unknown property type for: {property_name}")
    return None
```

#### Solutions

**Solution 1: Property Name Corrections**

Use correct property names:
```python
property_corrections = {
    'position': 'RelativeLocation',
    'location': 'RelativeLocation',
    'rotation': 'RelativeRotation',
    'scale': 'RelativeScale3D',
    'size': 'RelativeScale3D',
    'mesh': 'StaticMesh',
    'material': 'Material',
    'radius': 'SphereRadius',
    'extent': 'BoxExtent'
}

def correct_property_name(property_name):
    return property_corrections.get(property_name.lower(), property_name)
```

**Solution 2: Value Type Conversion**

Convert values to correct types:
```python
def convert_property_value(property_name, value):
    """Convert property value to correct type."""
    if property_name in ['RelativeLocation', 'RelativeRotation', 'RelativeScale3D', 'BoxExtent']:
        if isinstance(value, (list, tuple)) and len(value) == 3:
            return list(value)  # Ensure it's a list
        else:
            print(f"Vector property {property_name} requires [X, Y, Z] format")
            return [0, 0, 0]
    
    elif property_name in ['SphereRadius']:
        try:
            return float(value)
        except (ValueError, TypeError):
            print(f"Numeric property {property_name} requires number value")
            return 0.0
    
    elif property_name in ['StaticMesh', 'Material']:
        if isinstance(value, str):
            return value
        else:
            print(f"Asset property {property_name} requires string path")
            return ""
    
    return value
```

## Performance Optimization

### Slow Command Execution

#### Symptoms
- MCP commands taking longer than 10 seconds
- Timeout errors during complex operations
- Unreal Editor becoming unresponsive

#### Diagnostic Steps

**1. Monitor Command Performance**

Track execution times:
```python
import time
import json

class PerformanceMonitor:
    def __init__(self):
        self.command_times = {}
    
    def start_command(self, command_name):
        self.command_times[command_name] = time.time()
    
    def end_command(self, command_name):
        if command_name in self.command_times:
            duration = time.time() - self.command_times[command_name]
            print(f"Command '{command_name}' took {duration:.2f} seconds")
            return duration
        return None
    
    def get_slow_commands(self, threshold=5.0):
        slow_commands = []
        for cmd, start_time in self.command_times.items():
            if time.time() - start_time > threshold:
                slow_commands.append(cmd)
        return slow_commands

# Usage example
monitor = PerformanceMonitor()
```

**2. Check System Resources**

Monitor memory and CPU usage:
```python
import psutil
import os

def check_system_resources():
    """Check system resources and identify bottlenecks."""
    # Memory usage
    memory = psutil.virtual_memory()
    print(f"Memory: {memory.percent}% used ({memory.used / 1024**3:.1f}GB / {memory.total / 1024**3:.1f}GB)")
    
    # CPU usage
    cpu_percent = psutil.cpu_percent(interval=1)
    print(f"CPU: {cpu_percent}% used")
    
    # Find Unreal Engine processes
    for proc in psutil.process_iter(['pid', 'name', 'memory_info', 'cpu_percent']):
        if 'Unreal' in proc.info['name'] or 'UE' in proc.info['name']:
            memory_mb = proc.info['memory_info'].rss / 1024 / 1024
            print(f"Unreal process {proc.info['pid']}: {memory_mb:.0f}MB, {proc.info['cpu_percent']:.1f}% CPU")

check_system_resources()
```

#### Solutions

**Solution 1: Increase Timeout Values**

Adjust timeout settings for complex operations:
```json
{
    "timeout": 30,
    "complex_operation_timeout": 60,
    "blueprint_compile_timeout": 45
}
```

**Solution 2: Batch Operations**

Use bulk operations for multiple similar tasks:
```python
# Instead of multiple single operations:
# create_object("Cube", [0, 0, 0])
# create_object("Cube", [200, 0, 0]) 
# create_object("Cube", [400, 0, 0])

# Use bulk operation:
bulk_spawn_objects([
    {"type": "Cube", "location": [0, 0, 0]},
    {"type": "Cube", "location": [200, 0, 0]},
    {"type": "Cube", "location": [400, 0, 0]}
])
```

**Solution 3: Optimize Node Operations**

Reduce node creation overhead:
```python
def optimize_node_creation():
    """Best practices for efficient node creation."""
    # 1. Create all nodes first, then connect
    nodes_to_create = [
        {"type": "BeginPlay", "position": [0, 0]},
        {"type": "Print String", "position": [400, 0]},
        {"type": "Add_IntInt", "position": [800, 0]}
    ]
    
    # 2. Batch connections
    connections_to_make = [
        {"from": "BeginPlay", "from_pin": "then", "to": "Print String", "to_pin": "exec"},
        {"from": "Add_IntInt", "from_pin": "ReturnValue", "to": "Print String", "to_pin": "In String"}
    ]
    
    return nodes_to_create, connections_to_make
```

### Memory Issues

#### Symptoms
- Increasing memory usage over time
- Unreal Editor crashes during MCP operations
- System becoming slow during extended use

#### Solutions

**Solution 1: Cleanup Temporary Objects**

Regular cleanup of temporary objects:
```python
import unreal
import gc

def cleanup_temporary_objects():
    """Clean up temporary objects and force garbage collection."""
    # Force Unreal garbage collection
    unreal.SystemLibrary.collect_garbage()
    
    # Python garbage collection
    gc.collect()
    
    print("Cleanup completed")

# Call periodically during long operations
```

**Solution 2: Monitor Memory Usage**

Track memory consumption:
```python
def monitor_memory_usage():
    """Monitor and report memory usage."""
    import psutil
    import os
    
    process = psutil.Process(os.getpid())
    memory_mb = process.memory_info().rss / 1024 / 1024
    
    if memory_mb > 1000:  # Alert if over 1GB
        print(f"High memory usage: {memory_mb:.0f}MB")
        return True
    
    return False
```

## Error Message Reference

### Common Error Codes and Solutions

| Error Code | Description | Solution |
|------------|-------------|----------|
| `NODE_NOT_FOUND` | Blueprint node type doesn't exist | Use `get_node_suggestions` tool |
| `BLUEPRINT_NOT_FOUND` | Blueprint asset not found | Check asset path, create directories |
| `COMPONENT_NOT_FOUND` | Component class doesn't exist | Verify component class name |
| `PROPERTY_NOT_FOUND` | Component property doesn't exist | Check property name spelling |
| `CONNECTION_FAILED` | Node pin connection failed | Verify pin names and types |
| `COMPILATION_ERROR` | Blueprint compilation failed | Check for circular references, missing connections |
| `TIMEOUT_ERROR` | Operation exceeded time limit | Increase timeout, simplify operation |
| `PERMISSION_DENIED` | Operation requires user confirmation | User must approve destructive operations |
| `SOCKET_ERROR` | Socket communication failed | Restart MCP servers, check port |
| `INVALID_PATH` | Asset path is malformed | Use proper `/Game/` path format |

### Debugging Workflow

**Step 1: Identify Error Type**
```python
def categorize_error(error_message):
    """Categorize error for appropriate solution."""
    error_categories = {
        'socket': ['connection refused', 'socket error', 'timeout'],
        'blueprint': ['blueprint not found', 'compilation failed'],
        'node': ['node not found', 'pin not found', 'connection failed'],
        'component': ['component not found', 'property not found'],
        'path': ['invalid path', 'directory not found']
    }
    
    for category, keywords in error_categories.items():
        if any(keyword in error_message.lower() for keyword in keywords):
            return category
    
    return 'unknown'
```

**Step 2: Apply Category-Specific Solutions**
```python
def apply_solution(error_category, error_details):
    """Apply appropriate solution based on error category."""
    solutions = {
        'socket': "Restart MCP servers and check port 9877",
        'blueprint': "Verify Blueprint path and compilation status", 
        'node': "Use get_node_suggestions and check pin names",
        'component': "Verify component class and property names",
        'path': "Check asset path format and directory existence"
    }
    
    return solutions.get(error_category, "Check logs for more details")
```

## Advanced Debugging

### Enable Debug Logging

Enable comprehensive logging for troubleshooting:

**1. MCP Server Debug Mode**

Edit `mcp_server.py` configuration:
```python
DEBUG_MODE = True
LOG_LEVEL = "DEBUG"
VERBOSE_LOGGING = True
```

**2. Socket Server Logging**

Edit `socket_server_config.json`:
```json
{
    "enable_logging": true,
    "log_level": "DEBUG",
    "log_file": "mcp_debug.log",
    "verbose_mode": true
}
```

**3. Unreal Engine Logging**

Enable verbose logging in Unreal:
```
LogGenerativeAI Verbose
LogPython Verbose
LogBlueprint Verbose
```

### Log Analysis

**Parse MCP Logs**

Analyze log patterns:
```python
import re
from collections import Counter

def analyze_mcp_logs(log_file_path):
    """Analyze MCP logs for patterns and issues."""
    error_patterns = []
    command_counts = Counter()
    
    with open(log_file_path, 'r') as f:
        for line in f:
            # Count command usage
            if "Executing command:" in line:
                command = re.search(r"command: (\w+)", line)
                if command:
                    command_counts[command.group(1)] += 1
            
            # Collect errors
            if "ERROR" in line or "FAILED" in line:
                error_patterns.append(line.strip())
    
    print("Most used commands:", command_counts.most_common(5))
    print("Recent errors:", error_patterns[-5:])

# analyze_mcp_logs("mcp_debug.log")
```

---

This comprehensive troubleshooting guide provides systematic approaches to diagnosing and resolving issues with the UnrealGenAISupport MCP system. For additional support, refer to the [Server Setup](Server-Setup.md) and [Command Handlers](Command-Handlers.md) documentation.