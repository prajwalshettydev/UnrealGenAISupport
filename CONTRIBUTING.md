# Contributing to UnrealGenAISupport

Thank you for your interest in contributing to UnrealGenAISupport! This project brings cutting-edge AI integration to Unreal Engine through comprehensive API support and Model Control Protocol (MCP) functionality.

## 🚀 Quick Start

### Prerequisites

- **Unreal Engine 5.1+**
- **Visual Studio 2019/2022** (Windows) or **Xcode** (macOS)
- **Python 3.10+**
- **Git**

### Setup

1. **Fork and clone the repository**
   ```bash
   git clone https://github.com/your-username/UnrealGenAISupport.git
   cd UnrealGenAISupport
   ```

2. **Install Python dependencies**
   ```bash
   pip install mcp[cli] pytest black flake8 mypy
   ```

3. **Generate Unreal project files**
   - Right-click `.uproject` → "Generate Visual Studio project files"

4. **Set up API keys for testing** (optional)
   ```bash
   export PS_OPENAIAPIKEY="your_test_key"
   export PS_ANTHROPICAPIKEY="your_test_key"
   export PS_DEEPSEEKAPIKEY="your_test_key"
   ```

## 📚 Detailed Documentation

For comprehensive contribution guidelines, please refer to our detailed documentation:

### [📖 Complete Contributing Guide](Docs/Contributing/README.md)

This guide includes:
- **Detailed development setup instructions**
- **Coding standards for C++ and Python**
- **Testing guidelines and requirements**
- **Git workflow and pull request process**
- **Architecture principles and patterns**

### Quick Links

| Topic | Description | Link |
|-------|-------------|------|
| **Development Setup** | Environment configuration | [Setup Guide](Docs/Contributing/Development-Setup.md) |
| **Coding Standards** | C++ and Python guidelines | [Standards](Docs/Contributing/Coding-Standards.md) |
| **Architecture** | System design overview | [Architecture](Docs/Architecture/README.md) |
| **API Integration** | Adding new AI services | [API Docs](Docs/API/README.md) |
| **MCP Development** | Model Control Protocol | [MCP Docs](Docs/MCP/README.md) |

## 🎯 Contribution Areas

### High Priority 🔥

- **New AI Service Integration**: Google Gemini, Meta Llama, Baidu APIs
- **MCP Enhancements**: Blueprint automation, scene control, UI generation
- **Testing Framework**: Automated testing and CI/CD improvements
- **Documentation**: Code examples, tutorials, API references

### Medium Priority 🛠️

- **Performance Optimization**: HTTP pooling, memory management
- **Developer Experience**: Better error messages, enhanced logging
- **Platform Support**: Linux, additional Unreal Engine versions

### Always Welcome 📝

- **Bug fixes** with clear reproduction steps
- **Documentation improvements** and translations
- **Example projects** and learning materials
- **Code quality** and refactoring

## 🔄 Contribution Workflow

### 1. Issues First

- Check [existing issues](https://github.com/prajwalshettydev/UnrealGenAISupport/issues)
- Create new issue for bugs or feature requests using our **issue templates**
- Discuss major changes before implementation

**Issue Templates Available:**
- 🐛 **Bug Report** - Comprehensive bug reporting with environment details
- 🚀 **Feature Request** - Structured feature proposals with use cases
- 📚 **Documentation** - Links to guides and community discussions

Our templates ensure you provide all necessary information for efficient issue resolution.

### 2. Development Process

```bash
# Create feature branch
git checkout -b feature/your-feature-name

# Make changes following coding standards
# Add tests for new functionality
# Update documentation

# Commit with conventional commits format
git commit -m "feat(openai): add GPT-4.1 model support"

# Push and create pull request
git push origin feature/your-feature-name
```

### 3. Pull Request Guidelines

When you create a pull request, our **automated PR template** will guide you through providing all necessary information:

- **Clear title** describing the change
- **Detailed description** with context and motivation
- **Reference related issues** using `#issue-number`
- **Include testing instructions** for reviewers
- **Component area** selection (API/MCP/Blueprint/etc.)
- **Code quality checklist** for self-review
- **Testing coverage** documentation
- **Performance impact** assessment

**PR Template Features:**
- 📋 **Comprehensive checklist** covering all aspects
- 🔍 **Self-review guidance** for code quality
- 🧪 **Testing requirements** by component area
- 📸 **Screenshots/videos** section for visual changes
- ⚡ **Performance impact** tracking
- 🔄 **Breaking changes** identification

The template ensures consistent, high-quality pull requests and faster review cycles.

## 📋 Code Standards

### C++ (Unreal Engine)

- Follow **Unreal Engine Coding Standards**
- Use **Gen prefix** for all plugin classes
- Implement **USTRUCT/UCLASS/UFUNCTION** for Blueprint integration
- Proper **memory management** with smart pointers
- Comprehensive **error handling** and logging

```cpp
// Example: Adding new AI service
UCLASS()
class GENERATIVEAISUPPORT_API UGenNewServiceChat : public UCancellableAsyncAction
{
    GENERATED_BODY()
    
public:
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "GenAI|NewService")
    static UGenNewServiceChat* SendChatRequest(const FGenNewServiceSettings& Settings, const FOnChatCompletionResponse& OnComplete);
};
```

### Python (MCP System)

- **PEP 8** compliance (use `black` formatter)
- **Type hints** for all functions
- **Comprehensive docstrings** (PEP 257)
- **pytest** for testing
- **Error handling** with proper exceptions

```python
def handle_new_command(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Handle new MCP command with proper error handling.
    
    Args:
        command: Command dictionary with parameters
        
    Returns:
        Result dictionary with success status
        
    Raises:
        ValueError: When command parameters are invalid
    """
    try:
        # Implementation
        return {"success": True, "result": "success"}
    except Exception as e:
        logger.error(f"Command failed: {e}")
        return {"success": False, "error": str(e)}
```

## 🧪 Testing

### Required Tests

- **Unit tests** for all new functionality
- **Integration tests** for API endpoints  
- **MCP command tests** for new tools
- **Blueprint tests** for visual scripting features

### Running Tests

```bash
# Python tests
pytest Content/Python/tests/ -v

# Code formatting and linting
black Content/Python/
flake8 Content/Python/
mypy Content/Python/

# MCP integration tests
python test_mcp_integration.py
```

## 🏗️ Architecture Overview

The plugin follows a modular architecture:

- **Runtime Module**: Core C++ API integrations with Blueprint support
- **Editor Module**: MCP functionality and development tools
- **MCP System**: Dual-server Python architecture for AI control
- **Documentation**: Comprehensive guides and examples

For detailed architecture information, see [Architecture Documentation](Docs/Architecture/README.md).

## 🤝 Community Guidelines

- **Be respectful** and inclusive in all interactions
- **Use English** for all communication and documentation
- **Provide context** when asking questions or reporting issues
- **Search existing issues** before creating new ones
- **Test your changes** thoroughly before submitting

## 📞 Getting Help

1. **Check documentation** in the [Docs](Docs/) folder
2. **Search [GitHub Issues](https://github.com/prajwalshettydev/UnrealGenAISupport/issues)**
3. **Start a [GitHub Discussion](https://github.com/prajwalshettydev/UnrealGenAISupport/discussions)**
4. **Contact maintainers** through appropriate channels

## 🎉 Recognition

Contributors are recognized through:
- GitHub Contributors section
- Release notes acknowledgments  
- Community highlights
- Maintainer status for significant contributors

## 📖 Additional Resources

- [Unreal Engine Documentation](https://docs.unrealengine.com/)
- [Model Control Protocol](https://modelcontextprotocol.io/)
- [OpenAI API Documentation](https://platform.openai.com/docs/)
- [Anthropic API Documentation](https://docs.anthropic.com/)

---

**Ready to contribute?** Start by reading our [detailed contributing guide](Docs/Contributing/README.md) and explore the [architecture documentation](Docs/Architecture/README.md) to understand the system design.

Your contributions help make AI integration in Unreal Engine more accessible and powerful for developers worldwide! 🚀