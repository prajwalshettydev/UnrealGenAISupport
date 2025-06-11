# Development Setup

This guide provides detailed instructions for setting up a development environment for contributing to the UnrealGenAISupport plugin. Follow these steps to ensure you have all necessary tools and configurations.

## Prerequisites

### System Requirements

**Minimum Requirements:**
- **OS**: Windows 10/11, macOS 10.15+, or Ubuntu 18.04+
- **RAM**: 16GB (32GB recommended for large projects)
- **Storage**: 20GB free space for development tools
- **CPU**: Quad-core processor (8-core recommended)

**Software Requirements:**
- **Unreal Engine**: 5.1 or higher
- **IDE**: Visual Studio 2019/2022 (Windows), Xcode 12+ (macOS), or CLion (Linux)
- **Python**: 3.10 or higher
- **Git**: Latest version
- **Node.js**: 16+ (for documentation tools)

### Platform-Specific Setup

#### Windows Development

1. **Install Visual Studio 2022**
   ```powershell
   # Download from https://visualstudio.microsoft.com/
   # Required workloads:
   # - Desktop development with C++
   # - Game development with C++
   # - .NET desktop development
   ```

2. **Install Windows SDK**
   - Windows 10 SDK (10.0.18362.0 or later)
   - Windows 11 SDK (recommended)

3. **Configure Visual Studio**
   ```json
   // Visual Studio settings for Unreal development
   {
       "editor.formatOnSave": true,
       "C_Cpp.intelliSenseEngine": "default",
       "C_Cpp.errorSquiggles": "enabled",
       "files.associations": {
           "*.uproject": "json",
           "*.uplugin": "json"
       }
   }
   ```

#### macOS Development

1. **Install Xcode**
   ```bash
   # Install from App Store or Apple Developer Portal
   xcode-select --install
   ```

2. **Install Homebrew**
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

3. **Install Development Tools**
   ```bash
   brew install git python@3.10 node cmake
   ```

#### Linux Development

1. **Install Build Tools**
   ```bash
   # Ubuntu/Debian
   sudo apt update
   sudo apt install build-essential clang cmake git python3.10 python3.10-dev nodejs npm
   
   # Arch Linux
   sudo pacman -S base-devel clang cmake git python nodejs npm
   
   # CentOS/RHEL
   sudo yum groupinstall "Development Tools"
   sudo yum install clang cmake git python310 python310-devel nodejs npm
   ```

2. **Configure Environment**
   ```bash
   export CC=clang
   export CXX=clang++
   echo 'export CC=clang' >> ~/.bashrc
   echo 'export CXX=clang++' >> ~/.bashrc
   ```

## Unreal Engine Setup

### Installation

1. **Epic Games Launcher**
   - Download from [Epic Games](https://www.epicgames.com/store/en-US/download)
   - Install Unreal Engine 5.1+ through the launcher

2. **Engine Configuration**
   ```ini
   # Engine/Config/BaseEngine.ini additions for development
   [/Script/UnrealEd.EditorEngine]
   bEnableEditorAnalytics=False
   bSuppressMapErrors=False
   
   [/Script/Engine.Engine]
   bUseFixedFrameRate=False
   FixedFrameRate=60.000000
   ```

3. **Plugin Development Settings**
   ```ini
   # Engine/Config/BaseEditor.ini
   [/Script/UnrealEd.EditorLoadingSavingSettings]
   bMonitorContentDirectories=True
   bAutomaticallyCheckoutOnAssetModification=False
   
   [/Script/UnrealEd.EditorExperimentalSettings]
   bEnableModularGameplay=True
   ```

### Project Templates

Create a test project for plugin development:

```cpp
// TestProject.uproject
{
    "FileVersion": 3,
    "EngineAssociation": "5.1",
    "Category": "",
    "Description": "",
    "Modules": [
        {
            "Name": "TestProject",
            "Type": "Runtime",
            "LoadingPhase": "Default",
            "AdditionalDependencies": [
                "Engine",
                "GenerativeAISupport"
            ]
        }
    ],
    "Plugins": [
        {
            "Name": "GenerativeAISupport",
            "Enabled": true,
            "MarketplaceURL": "",
            "SupportedTargetPlatforms": [
                "Win64",
                "Mac",
                "Linux"
            ]
        }
    ]
}
```

## Python Environment Setup

### Virtual Environment

1. **Create Development Environment**
   ```bash
   # Create virtual environment
   python -m venv venv-unrealgenai
   
   # Activate environment
   # Windows
   venv-unrealgenai\Scripts\activate
   # macOS/Linux
   source venv-unrealgenai/bin/activate
   ```

2. **Install Dependencies**
   ```bash
   # Core development dependencies
   pip install --upgrade pip setuptools wheel
   
   # MCP and testing dependencies
   pip install mcp[cli] pytest pytest-asyncio pytest-cov
   
   # Code quality tools
   pip install black flake8 mypy isort pre-commit
   
   # Unreal Engine integration
   pip install unreal
   
   # Documentation tools
   pip install sphinx sphinx-rtd-theme myst-parser
   ```

3. **Development Requirements File**
   ```txt
   # requirements-dev.txt
   mcp[cli]>=1.0.0
   pytest>=7.0.0
   pytest-asyncio>=0.21.0
   pytest-cov>=4.0.0
   black>=23.0.0
   flake8>=6.0.0
   mypy>=1.0.0
   isort>=5.12.0
   pre-commit>=3.0.0
   unreal>=5.1.0
   sphinx>=6.0.0
   sphinx-rtd-theme>=1.2.0
   myst-parser>=1.0.0
   ```

### Code Quality Setup

1. **Pre-commit Hooks**
   ```yaml
   # .pre-commit-config.yaml
   repos:
     - repo: https://github.com/psf/black
       rev: 23.3.0
       hooks:
         - id: black
           files: ^Content/Python/
     
     - repo: https://github.com/pycqa/flake8
       rev: 6.0.0
       hooks:
         - id: flake8
           files: ^Content/Python/
           args: [--max-line-length=88]
     
     - repo: https://github.com/pycqa/isort
       rev: 5.12.0
       hooks:
         - id: isort
           files: ^Content/Python/
           args: [--profile=black]
     
     - repo: https://github.com/pre-commit/mirrors-mypy
       rev: v1.3.0
       hooks:
         - id: mypy
           files: ^Content/Python/
           additional_dependencies: [types-requests]
   ```

2. **Install Pre-commit**
   ```bash
   pre-commit install
   pre-commit run --all-files
   ```

3. **Editor Configuration**
   ```ini
   # .editorconfig
   root = true
   
   [*]
   charset = utf-8
   end_of_line = lf
   insert_final_newline = true
   trim_trailing_whitespace = true
   
   [*.{py,pyi}]
   indent_style = space
   indent_size = 4
   max_line_length = 88
   
   [*.{cpp,h,hpp}]
   indent_style = tab
   indent_size = 4
   
   [*.{json,yml,yaml}]
   indent_style = space
   indent_size = 2
   ```

## IDE Configuration

### Visual Studio Code (Recommended for Python)

1. **Install Extensions**
   ```json
   {
       "recommendations": [
           "ms-python.python",
           "ms-python.black-formatter",
           "ms-python.flake8",
           "ms-python.mypy-type-checker",
           "ms-vscode.cpptools",
           "epicgames.unreal-engine-tools",
           "ms-vscode.cmake-tools",
           "davidanson.vscode-markdownlint"
       ]
   }
   ```

2. **Workspace Settings**
   ```json
   {
       "python.defaultInterpreterPath": "./venv-unrealgenai/bin/python",
       "python.formatting.provider": "black",
       "python.linting.enabled": true,
       "python.linting.flake8Enabled": true,
       "python.linting.mypyEnabled": true,
       "python.testing.pytestEnabled": true,
       "python.testing.pytestArgs": ["Content/Python/tests"],
       "files.associations": {
           "*.uproject": "json",
           "*.uplugin": "json"
       },
       "C_Cpp.intelliSenseEngine": "default",
       "C_Cpp.configurationProvider": "ms-vscode.cmake-tools"
   }
   ```

### Visual Studio (Windows C++)

1. **Configure IntelliSense**
   ```xml
   <!-- .vs/ProjectSettings.json -->
   {
       "CurrentProjectSetting": "Debug|x64",
       "C++": {
           "IntelliSenseEngine": "default",
           "ErrorSquiggles": "enabled",
           "CodeAnalysis": {
               "RunOnBuild": true,
               "Rules": [
                   "C26400", // Don't assign a naked pointer result
                   "C26401", // Don't delete a naked pointer
                   "C26409"  // Avoid calling new explicitly
               ]
           }
       }
   }
   ```

2. **Project Configuration**
   ```xml
   <!-- UnrealGenAISupport.vcxproj.user -->
   <PropertyGroup>
       <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
       <LocalDebuggerCommand>$(UE_ENGINE_DIRECTORY)\Binaries\Win64\UnrealEditor.exe</LocalDebuggerCommand>
       <LocalDebuggerCommandArguments>"$(ProjectDir)TestProject.uproject"</LocalDebuggerCommandArguments>
       <LocalDebuggerWorkingDirectory>$(ProjectDir)</LocalDebuggerWorkingDirectory>
   </PropertyGroup>
   ```

## API Keys and Environment

### Development API Keys

1. **Create Test API Keys**
   - OpenAI: Create test key with limited quota
   - Anthropic: Use development tier key
   - DeepSeek: Register for API access
   - XAI: Set up development account

2. **Environment Configuration**
   ```bash
   # .env file (DO NOT commit to repository)
   PS_OPENAIAPIKEY=sk-test-your-openai-key
   PS_ANTHROPICAPIKEY=sk-ant-your-anthropic-key
   PS_DEEPSEEKAPIKEY=sk-your-deepseek-key
   PS_XAIAPIKEY=xai-your-xai-key
   
   # Development settings
   UNREAL_ENGINE_DIR=/path/to/UnrealEngine
   UE_PROJECT_DIR=/path/to/TestProject
   ```

3. **Environment Script**
   ```bash
   #!/bin/bash
   # setup-dev-env.sh
   
   # Set development environment variables
   export PS_OPENAIAPIKEY="your-dev-key"
   export PS_ANTHROPICAPIKEY="your-dev-key"
   export PS_DEEPSEEKAPIKEY="your-dev-key"
   export PS_XAIAPIKEY="your-dev-key"
   
   # Unreal Engine paths
   export UE_ENGINE_DIR="/path/to/UnrealEngine"
   export UE_PROJECT_DIR="/path/to/TestProject"
   
   # Python development
   source venv-unrealgenai/bin/activate
   
   echo "Development environment configured!"
   ```

## MCP Development Setup

### Client Configuration

1. **Claude Desktop Setup**
   ```json
   {
       "mcpServers": {
           "unreal-dev": {
               "command": "python",
               "args": ["./Content/Python/mcp_server.py"],
               "env": {
                   "UNREAL_HOST": "localhost",
                   "UNREAL_PORT": "9877",
                   "DEBUG_MODE": "true"
               }
           }
       }
   }
   ```

2. **Cursor IDE Setup**
   ```json
   {
       "mcpServers": {
           "unreal-dev": {
               "command": "python",
               "args": ["./Content/Python/mcp_server.py"],
               "env": {
                   "UNREAL_HOST": "localhost",
                   "UNREAL_PORT": "9877",
                   "DEBUG_MODE": "true"
               }
           }
       }
   }
   ```

### Development Server

1. **Debug Configuration**
   ```python
   # Content/Python/debug_config.py
   import logging
   import sys
   
   # Configure debug logging
   logging.basicConfig(
       level=logging.DEBUG,
       format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
       handlers=[
           logging.FileHandler('mcp_debug.log'),
           logging.StreamHandler(sys.stdout)
       ]
   )
   
   # Debug settings
   DEBUG_MODE = True
   DETAILED_LOGGING = True
   COMMAND_TRACING = True
   ```

2. **Test Server Script**
   ```python
   #!/usr/bin/env python3
   # test_mcp_server.py
   
   import asyncio
   import json
   import socket
   from typing import Dict, Any
   
   async def test_mcp_connection():
       """Test MCP server connection and basic commands."""
       try:
           # Test socket connection
           with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
               s.connect(('localhost', 9877))
               
               # Test handshake
               test_command = {
                   "type": "handshake",
                   "message": "test connection"
               }
               
               s.sendall(json.dumps(test_command).encode('utf-8'))
               response = s.recv(1024).decode('utf-8')
               
               print(f"MCP Server Response: {response}")
               return True
               
       except Exception as e:
           print(f"MCP Connection Failed: {e}")
           return False
   
   if __name__ == "__main__":
       asyncio.run(test_mcp_connection())
   ```

## Testing Setup

### Test Project Structure

```
TestProject/
├── Content/
│   ├── TestBlueprints/
│   │   ├── TestAIIntegration.uasset
│   │   └── TestMCPCommands.uasset
│   └── TestMaterials/
├── Source/
│   └── TestProject/
│       ├── TestAPIIntegration.cpp
│       └── TestMCPIntegration.cpp
└── Plugins/
    └── GenerativeAISupport/ (symlink to development plugin)
```

### Automated Testing

1. **Test Configuration**
   ```ini
   # TestProject/Config/DefaultEngine.ini
   [/Script/Engine.AutomationTestSettings]
   bSuppressLogErrors=False
   bSuppressLogWarnings=False
   bTreatLogWarningsAsTestErrors=True
   
   [/Script/UnrealEd.AutomationTestExcludelist]
   CommandlineDDCGraph=True
   ```

2. **Test Execution Scripts**
   ```bash
   #!/bin/bash
   # run-tests.sh
   
   echo "Running Python tests..."
   pytest Content/Python/tests/ -v --cov=Content/Python --cov-report=html
   
   echo "Running C++ tests..."
   "${UE_ENGINE_DIR}/Binaries/Win64/UnrealEditor-Cmd.exe" \
       "${UE_PROJECT_DIR}/TestProject.uproject" \
       -ExecCmds="Automation RunTests GenerativeAISupport; Quit" \
       -testexit="Automation Test Queue Empty" \
       -log
   
   echo "Running MCP integration tests..."
   python test_mcp_integration.py
   ```

## Debugging Setup

### Unreal Engine Debugging

1. **Debug Configuration**
   ```json
   {
       "name": "Debug UnrealEditor",
       "type": "cppvsdbg",
       "request": "launch",
       "program": "${env:UE_ENGINE_DIR}/Binaries/Win64/UnrealEditor-Win64-DebugGame.exe",
       "args": ["${workspaceFolder}/TestProject.uproject"],
       "stopAtEntry": false,
       "cwd": "${workspaceFolder}",
       "environment": [],
       "console": "externalTerminal"
   }
   ```

2. **Logging Configuration**
   ```cpp
   // Add to your test code for detailed logging
   DEFINE_LOG_CATEGORY_STATIC(LogGenAITest, Log, All);
   
   void TestFunction()
   {
       UE_LOG(LogGenAITest, Log, TEXT("Testing AI integration..."));
       UE_LOG(LogGenAITest, Warning, TEXT("Warning: %s"), *ErrorMessage);
       UE_LOG(LogGenAITest, Error, TEXT("Error occurred: %s"), *DetailedError);
   }
   ```

### Python Debugging

1. **Debug Server**
   ```python
   # debug_mcp_server.py
   import debugpy
   import sys
   
   # Enable debugging
   debugpy.listen(("localhost", 5678))
   print("Waiting for debugger to attach...")
   debugpy.wait_for_client()
   
   # Import and run normal MCP server
   from mcp_server import main
   main()
   ```

2. **VS Code Debug Configuration**
   ```json
   {
       "name": "Debug MCP Server",
       "type": "python",
       "request": "attach",
       "connect": {
           "host": "localhost",
           "port": 5678
       },
       "pathMappings": [
           {
               "localRoot": "${workspaceFolder}/Content/Python",
               "remoteRoot": "${workspaceFolder}/Content/Python"
           }
       ]
   }
   ```

## Performance Monitoring

### Profiling Setup

1. **Unreal Insights**
   ```cpp
   // Add to performance-critical code
   SCOPED_NAMED_EVENT(GenAI_APICall, FColor::Red);
   
   // For detailed timing
   SCOPE_CYCLE_COUNTER(STAT_GenAI_RequestProcessing);
   ```

2. **Python Profiling**
   ```python
   import cProfile
   import pstats
   from pstats import SortKey
   
   def profile_mcp_command():
       """Profile MCP command execution."""
       pr = cProfile.Profile()
       pr.enable()
       
       # Execute command
       result = handle_command(test_command)
       
       pr.disable()
       stats = pstats.Stats(pr)
       stats.sort_stats(SortKey.CUMULATIVE)
       stats.print_stats(10)
   ```

## Troubleshooting

### Common Issues

1. **Build Errors**
   ```bash
   # Clean and rebuild
   rm -rf Binaries/ Intermediate/
   ./GenerateProjectFiles.sh  # or .bat on Windows
   ```

2. **Python Import Issues**
   ```bash
   # Ensure correct Python path
   export PYTHONPATH="${PYTHONPATH}:${PWD}/Content/Python"
   ```

3. **MCP Connection Issues**
   ```bash
   # Check port availability
   netstat -tulpn | grep 9877
   
   # Test socket manually
   telnet localhost 9877
   ```

4. **API Key Issues**
   ```bash
   # Verify environment variables
   env | grep PS_
   
   # Test API connectivity
   curl -H "Authorization: Bearer $PS_OPENAIAPIKEY" \
        https://api.openai.com/v1/models
   ```

---

This development setup provides a comprehensive foundation for contributing to UnrealGenAISupport. Follow these guidelines to ensure a smooth development experience and maintain code quality standards.