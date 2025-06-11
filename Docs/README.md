# UnrealGenAISupport Documentation

Comprehensive documentation for the UnrealGenAISupport plugin - a cutting-edge Unreal Engine plugin that provides seamless integration with multiple generative AI services and Model Control Protocol (MCP) for direct AI control of Unreal Engine projects.

## Quick Start

The UnrealGenAISupport plugin enables game developers to integrate advanced AI capabilities directly into their Unreal Engine projects through both traditional API calls and revolutionary Model Control Protocol (MCP) functionality that allows AI assistants to directly control Unreal Engine.

### Key Features

- **Multi-API Support**: OpenAI GPT-4o/4.1, Anthropic Claude 4, DeepSeek R1, XAI Grok 3
- **Model Control Protocol**: Direct AI control of Unreal Engine through Claude Desktop/Cursor IDE
- **Blueprint Integration**: Full visual scripting support for all AI functionality
- **Dual Architecture**: Runtime module for production + Editor module for development
- **Type Safety**: Comprehensive USTRUCT/UCLASS integration with Blueprint support

## Documentation Structure

### 📋 [Architecture](Architecture/README.md)
Detailed technical architecture, component design, and system integration patterns.

- [Core Components](Architecture/Core-Components.md) - Runtime and Editor modules, dependencies
- [MCP System](Architecture/MCP-System.md) - Dual-server architecture and communication protocols  
- [API Integration](Architecture/API-Integration.md) - Async patterns, data structures, delegates

### 🛠️ [Contributing](Contributing/README.md)
Developer guides, setup instructions, and best practices for contributing to the plugin.

- [Development Setup](Contributing/Development-Setup.md) - Environment configuration, dependencies, installation
- [Coding Standards](Contributing/Coding-Standards.md) - C++ and Python coding guidelines, best practices

### 🔌 [API Integrations](API/README.md)
Complete documentation for all supported AI service APIs with implementation examples.

- [OpenAI](API/OpenAI/README.md) - GPT-4o, GPT-4.1, Structured Outputs, DALL-E integration
- [Anthropic](API/Anthropic/README.md) - Claude 4 Opus/Sonnet, Claude 3.5 family integration
- [DeepSeek](API/DeepSeek/README.md) - DeepSeek Chat, R1 Reasoning model integration
- [XAI](API/XAI/README.md) - Grok 3 latest, Grok 3 mini integration

### 🤖 [Model Control Protocol](MCP/README.md)
Comprehensive MCP implementation documentation for AI-controlled Unreal Engine automation.

- [Server Setup](MCP/Server-Setup.md) - Installation, configuration, client setup
- [Command Handlers](MCP/Command-Handlers.md) - 40+ tools for Blueprint, scene, and project control
- [Troubleshooting](MCP/Troubleshooting.md) - Common issues, debugging, performance optimization

### 🤝 [Contributing](Contributing/README.md)
Complete guide for contributors including setup, standards, and workflow.

- [Development Setup](Contributing/Development-Setup.md) - Environment configuration, dependencies, tools
- [Coding Standards](Contributing/Coding-Standards.md) - C++ and Python coding guidelines, best practices
- [Testing Guidelines](Contributing/Testing-Guidelines.md) - Testing strategies, requirements, examples
- [Contribution Workflow](Contributing/Contribution-Workflow.md) - Git workflow, PR process, code review

## Getting Started

1. **Installation**: Follow the [Development Setup](Contributing/Development-Setup.md) to install and configure the plugin
2. **API Keys**: Configure your AI service API keys using environment variables
3. **First API Call**: Try the [OpenAI Integration](API/OpenAI/README.md) examples
4. **MCP Setup**: Enable AI control with [MCP Server Setup](MCP/Server-Setup.md)
5. **Blueprint Usage**: Explore [Blueprint examples](API/README.md) for visual scripting

## Architecture Overview

UnrealGenAISupport follows a modular architecture designed for both runtime performance and editor-time development:

```
UnrealGenAISupport/
├── Runtime Module (GenerativeAISupport)
│   ├── API Integrations (OpenAI, Anthropic, DeepSeek, XAI)
│   ├── Data Structures (USTRUCT definitions)
│   ├── Async Actions (UCancellableAsyncAction pattern)
│   └── Security (API key management)
└── Editor Module (GenerativeAISupportEditor)
    ├── MCP System (External + Internal servers)
    ├── Blueprint Automation (Node creation, connections)
    ├── Scene Control (Object spawning, materials)
    └── Development Tools (Settings, debugging)
```

## Plugin Status

**Current Version**: 1.0 (Beta)  
**Unreal Engine Support**: 5.1+  
**Platform Support**: Windows, macOS, Linux  
**License**: MIT

> **⚠️ Beta Warning**: This plugin is in active development. Use version control and avoid production environments.

## Key Technical Highlights

- **Async Pattern**: All API calls use `UCancellableAsyncAction` for non-blocking operations
- **Type Safety**: Full USTRUCT integration with Blueprint reflection system
- **Memory Management**: Smart pointers and UPROPERTY for proper Unreal Engine integration
- **Error Handling**: Comprehensive error reporting with detailed error messages
- **Performance**: Optimized HTTP requests with configurable timeouts
- **Security**: Environment variable-based API key management

## Community & Support

- **Repository**: [GitHub - UnrealGenAISupport](https://github.com/prajwalshettydev/UnrealGenAISupport)
- **Issues**: Report bugs and feature requests on GitHub
- **Examples**: [Test Project](https://github.com/prajwalshettydev/unreal-llm-api-test-project)
- **Creator**: [Prajwal Shetty](https://prajwalshetty.com/)

## Quick Reference

### Environment Variables
```bash
PS_OPENAIAPIKEY="your_openai_key"
PS_ANTHROPICAPIKEY="your_anthropic_key" 
PS_DEEPSEEKAPIKEY="your_deepseek_key"
PS_XAIAPIKEY="your_xai_key"
```

### Basic C++ Usage
```cpp
#include "Models/OpenAI/GenOAIChat.h"

FGenChatSettings Settings;
Settings.Model = EGenOAIChatModel::GPT_4o;
Settings.Messages.Add(FGenChatMessage{TEXT("user"), TEXT("Hello AI!")});

UGenOAIChat::SendChatRequest(Settings, 
    FOnChatCompletionResponse::CreateLambda([](const FString& Response, const FString& Error, bool Success) {
        UE_LOG(LogTemp, Log, TEXT("AI Response: %s"), *Response);
    }));
```

### Basic Blueprint Usage
Add the "Request OpenAI Chat" node in Blueprint, configure settings, and connect the completion delegate to handle responses.

---

*This documentation covers the complete UnrealGenAISupport plugin ecosystem. Each section contains detailed implementation examples, best practices, and troubleshooting guides.*