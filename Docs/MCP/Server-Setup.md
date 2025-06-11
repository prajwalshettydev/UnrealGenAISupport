# MCP Server Setup

Complete guide for setting up the Model Control Protocol (MCP) server system with the UnrealGenAISupport plugin. This guide covers installation, configuration, and setup for all supported AI clients.

## Prerequisites

### System Requirements

- **Unreal Engine**: 5.1 or higher
- **Python**: 3.10+ (comes with Unreal Engine)
- **AI Client**: Claude Desktop App, Cursor IDE, or compatible MCP client
- **Network**: localhost access (port 9877)
- **Permissions**: Ability to execute Python scripts

### Plugin Requirements

1. **UnrealGenAISupport Plugin**: Installed and enabled in your project
2. **Python Editor Script Plugin**: Enabled in Unreal Engine
   - Go to **Edit** → **Plugins** → Search "Python Editor Script Plugin" → **Enable**
3. **Project Configuration**: Add plugin to your project's Build.cs if using C++

```cpp
PrivateDependencyModuleNames.AddRange(new string[] { 
    "GenerativeAISupport",
    "GenerativeAISupportEditor"  // For MCP functionality
});
```

## Installation Methods

### Method 1: Auto-Start Setup (Recommended)

The easiest way to set up MCP is using the built-in auto-start feature:

#### Step 1: Enable Auto-Start

1. **Open Unreal Editor** with your project
2. **Navigate to**: Edit → Project Settings → Plugins → Generative AI Support
3. **Check**: "Auto Start Socket Server" option
4. **Restart Unreal Editor**

#### Step 2: Verify Auto-Start

After restart, check the Output Log for:
```
LogGenerativeAI: MCP Socket Server started automatically on port 9877
LogGenerativeAI: External MCP Server started with PID: [process_id]
```

#### Step 3: Configure AI Client

Skip to the [AI Client Configuration](#ai-client-configuration) section.

### Method 2: Manual Setup

For more control or troubleshooting, you can set up the servers manually:

#### Step 1: Start Internal Socket Server

1. **Open Unreal Editor** with your project
2. **Open**: Window → Developer Tools → Python Console
3. **Execute**:
```python
exec(open(r"[YourProjectPath]/Plugins/GenerativeAISupport/Content/Python/unreal_socket_server.py").read())
```

Replace `[YourProjectPath]` with your actual project path.

**Expected Output**:
```
Socket server listening on localhost:9877
Server ready for MCP commands
```

#### Step 2: Start External MCP Server

**Option A: Command Line**
```bash
cd /path/to/your/project/Plugins/GenerativeAISupport/Content/Python
python mcp_server.py
```

**Option B: Using Python**
```python
import subprocess
import os

server_path = "/path/to/your/project/Plugins/GenerativeAISupport/Content/Python/mcp_server.py"
subprocess.Popen(["python", server_path])
```

**Expected Output**:
```
FastMCP server starting...
MCP tools initialized: 25 tools available
Server ready for client connections
```

## AI Client Configuration

### Claude Desktop App Setup

#### Step 1: Locate Configuration File

**Windows**:
```
%APPDATA%\Claude\claude_desktop_config.json
```

**macOS**:
```
~/Library/Application Support/Claude/claude_desktop_config.json
```

**Linux**:
```
~/.config/claude/claude_desktop_config.json
```

#### Step 2: Edit Configuration

Add or modify the configuration file:

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

**Important**: Use absolute paths for the `args` parameter.

#### Step 3: Restart Claude Desktop

1. **Quit Claude Desktop** completely
2. **Restart Claude Desktop**
3. **Verify Connection** by asking: "Can you test the Unreal Engine MCP connection?"

### Cursor IDE Setup

#### Step 1: Create MCP Configuration

In your project root, create `.cursor/mcp.json`:

```json
{
    "mcpServers": {
        "unreal-engine": {
            "command": "python",
            "args": ["./Plugins/GenerativeAISupport/Content/Python/mcp_server.py"],
            "env": {
                "UNREAL_HOST": "localhost",
                "UNREAL_PORT": "9877"
            }
        }
    }
}
```

#### Step 2: Restart Cursor

1. **Close all Cursor windows**
2. **Restart Cursor IDE**
3. **Open your project**
4. **Test connection** through Cursor's AI chat

### Generic MCP Client Setup

For other MCP-compatible clients:

```json
{
    "server_name": "unreal-engine",
    "command": ["python", "/path/to/mcp_server.py"],
    "environment": {
        "UNREAL_HOST": "localhost",
        "UNREAL_PORT": "9877"
    },
    "timeout": 30
}
```

## Configuration Options

### Socket Server Configuration

Edit `Content/Python/socket_server_config.json`:

```json
{
    "auto_start_socket_server": true,
    "host": "localhost",
    "port": 9877,
    "timeout": 10,
    "buffer_size": 4096,
    "enable_logging": true,
    "log_level": "INFO"
}
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `auto_start_socket_server` | `false` | Enable automatic server startup |
| `host` | `"localhost"` | Server host address |
| `port` | `9877` | Server port number |
| `timeout` | `10` | Command timeout in seconds |
| `buffer_size` | `4096` | Socket buffer size |
| `enable_logging` | `true` | Enable debug logging |
| `log_level` | `"INFO"` | Logging verbosity level |

### Plugin Settings

Access through **Project Settings** → **Plugins** → **Generative AI Support**:

- **Auto Start Socket Server**: Automatically start MCP servers on editor launch
- **Default Port**: Configure default socket port
- **Enable Debug Logging**: Extra verbose logging for troubleshooting

## Verification and Testing

### Step 1: Test Socket Connection

In Unreal's Python Console:
```python
import socket
import json

# Test socket connection
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    s.connect(('localhost', 9877))
    test_command = {"command": "handshake_test", "args": {}}
    s.sendall(json.dumps(test_command).encode('utf-8'))
    response = s.recv(4096).decode('utf-8')
    print(f"Response: {response}")
    s.close()
    print("Socket server is working correctly!")
except Exception as e:
    print(f"Socket connection failed: {e}")
```

### Step 2: Test MCP Tools

Ask your AI client to execute a simple test:
> "Can you use the handshake_test tool to verify the Unreal Engine MCP connection?"

**Expected Response**:
```
Handshake successful! MCP server is connected to Unreal Engine.
Available tools: 25 tools ready for use.
```

### Step 3: Test Basic Operations

Ask your AI client:
> "Can you spawn a cube in the Unreal Engine level?"

The AI should successfully spawn a cube and report the operation result.

### Step 4: Test Blueprint Operations

Ask your AI client:
> "Can you create a simple Blueprint called 'TestActor' derived from Actor?"

The AI should create the Blueprint and confirm successful compilation.

## Troubleshooting Setup Issues

### Common Problems and Solutions

#### "Port 9877 is already in use"

**Solution**:
```bash
# Check what's using the port
netstat -tulpn | grep 9877  # Linux/macOS
netstat -ano | findstr 9877  # Windows

# Kill the process if needed
kill [PID]  # Linux/macOS
taskkill /PID [PID] /F  # Windows
```

#### "Python script not found"

**Verification Steps**:
1. Check file paths are correct and absolute
2. Verify plugin is properly installed
3. Ensure Python Editor Script Plugin is enabled

**Test Path**:
```python
import os
script_path = "/path/to/your/mcp_server.py"
print(f"File exists: {os.path.exists(script_path)}")
```

#### "MCP server not responding"

**Debugging Steps**:

1. **Check Server Status**:
```python
# In Unreal Python Console
import subprocess
result = subprocess.run(["ps", "aux"], capture_output=True, text=True)
print("MCP processes:", [line for line in result.stdout.split('\n') if 'mcp_server.py' in line])
```

2. **Check Logs**:
   - Unreal Engine Output Log
   - Python Console output
   - AI client error messages

3. **Restart Everything**:
   - Stop all MCP processes
   - Restart Unreal Editor
   - Restart AI client
   - Test connection again

#### "AI client can't find MCP server"

**Configuration Check**:
1. Verify absolute paths in configuration files
2. Check environment variables are set correctly
3. Ensure AI client has been restarted after configuration
4. Test with minimal configuration first

**Minimal Test Configuration**:
```json
{
    "mcpServers": {
        "test": {
            "command": "python",
            "args": ["/absolute/path/to/mcp_server.py"]
        }
    }
}
```

### Network Troubleshooting

#### Firewall Issues

**Windows Firewall**:
```cmd
# Allow Python through firewall
netsh advfirewall firewall add rule name="Unreal MCP Server" dir=in action=allow program="python.exe" enable=yes
```

**macOS Firewall**:
- System Preferences → Security & Privacy → Firewall
- Allow Python applications

#### Port Conflicts

**Find Alternative Port**:
```python
import socket

def find_free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(('', 0))
        s.listen(1)
        port = s.getsockname()[1]
    return port

print(f"Free port: {find_free_port()}")
```

Update configuration files with the new port.

## Advanced Configuration

### Custom Script Locations

For non-standard installations:

```json
{
    "mcpServers": {
        "unreal-custom": {
            "command": "/custom/python/path/python",
            "args": ["/custom/project/location/mcp_server.py"],
            "env": {
                "UNREAL_HOST": "localhost",
                "UNREAL_PORT": "9877",
                "PYTHONPATH": "/custom/python/libs"
            }
        }
    }
}
```

### Multiple Project Support

Configure different MCP servers for different projects:

```json
{
    "mcpServers": {
        "unreal-project-a": {
            "command": "python",
            "args": ["/path/to/projectA/Plugins/GenerativeAISupport/Content/Python/mcp_server.py"],
            "env": {"UNREAL_PORT": "9877"}
        },
        "unreal-project-b": {
            "command": "python",
            "args": ["/path/to/projectB/Plugins/GenerativeAISupport/Content/Python/mcp_server.py"],
            "env": {"UNREAL_PORT": "9878"}
        }
    }
}
```

### Security Considerations

#### Local Network Only

The MCP server only accepts localhost connections by default. For network access:

```json
{
    "host": "0.0.0.0",
    "port": 9877,
    "allowed_hosts": ["192.168.1.100", "10.0.0.50"]
}
```

⚠️ **Warning**: Only enable network access in trusted environments.

#### Restricted Commands

Configure command restrictions in the MCP server:

```python
# In mcp_server.py configuration
RESTRICTED_COMMANDS = [
    "execute_python_script",  # Disable Python execution
    "execute_unreal_command"  # Disable console commands
]
```

## Process Management

### PID File Management

The system creates PID files for process tracking:

**Location**: `~/.unrealgenai/mcp_server.pid`

**Manual Cleanup**:
```bash
# Check PID file
cat ~/.unrealgenai/mcp_server.pid

# Kill process if needed
kill $(cat ~/.unrealgenai/mcp_server.pid)

# Clean up PID file
rm ~/.unrealgenai/mcp_server.pid
```

### Automatic Cleanup

The plugin automatically handles cleanup on:
- Unreal Editor shutdown
- Project close
- Plugin disable

### Service Mode (Advanced)

For always-on MCP servers, create system services:

**Linux systemd service** (`/etc/systemd/system/unreal-mcp.service`):
```ini
[Unit]
Description=Unreal Engine MCP Server
After=network.target

[Service]
Type=simple
User=youruser
ExecStart=/usr/bin/python /path/to/mcp_server.py
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

**macOS launchd** (`~/Library/LaunchAgents/com.unrealgenai.mcp.plist`):
```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.unrealgenai.mcp</string>
    <key>ProgramArguments</key>
    <array>
        <string>/usr/bin/python3</string>
        <string>/path/to/mcp_server.py</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
</dict>
</plist>
```

## Performance Optimization

### Server Performance

```python
# Optimize socket server settings
{
    "buffer_size": 8192,      # Larger buffer for complex commands
    "timeout": 30,            # Longer timeout for complex operations
    "max_connections": 5,     # Limit concurrent connections
    "thread_pool_size": 4     # Parallel command processing
}
```

### Memory Management

Monitor memory usage:
```python
import psutil
import os

# Check MCP server memory usage
for proc in psutil.process_iter(['pid', 'name', 'memory_info']):
    if 'python' in proc.info['name'].lower():
        cmdline = proc.cmdline()
        if any('mcp_server.py' in arg for arg in cmdline):
            memory_mb = proc.info['memory_info'].rss / 1024 / 1024
            print(f"MCP Server PID {proc.info['pid']}: {memory_mb:.1f} MB")
```

---

This comprehensive setup guide ensures reliable MCP server operation across all supported platforms and AI clients. For ongoing operation and troubleshooting, refer to the [Troubleshooting](Troubleshooting.md) documentation.