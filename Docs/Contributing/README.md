# Contributing to UnrealGenAISupport

Welcome to the UnrealGenAISupport contributor community! This guide provides comprehensive information for developers who want to contribute to this cutting-edge Unreal Engine plugin that integrates multiple AI services and Model Control Protocol functionality.

## Table of Contents

- [Getting Started](#getting-started) - Quick setup for new contributors
- [Development Setup](Development-Setup.md) - Detailed environment configuration
- [Coding Standards](Coding-Standards.md) - C++ and Python coding guidelines
- [Testing Guidelines](Testing-Guidelines.md) - Testing strategies and requirements
- [Contribution Workflow](Contribution-Workflow.md) - Git workflow and PR process
- [Architecture Guidelines](Architecture-Guidelines.md) - Design principles and patterns

## Getting Started

### Prerequisites

Before contributing to UnrealGenAISupport, ensure you have:

- **Unreal Engine 5.1+** installed
- **Visual Studio 2019/2022** (Windows) or **Xcode** (macOS)
- **Python 3.10+** with development headers
- **Git** for version control
- **Claude Desktop** or **Cursor IDE** for MCP testing

### Quick Setup

1. **Fork and Clone**
   ```bash
   git fork https://github.com/prajwalshettydev/UnrealGenAISupport
   git clone https://github.com/your-username/UnrealGenAISupport
   cd UnrealGenAISupport
   ```

2. **Install Dependencies**
   ```bash
   # Python dependencies for MCP development
   pip install mcp[cli] pytest black flake8 mypy
   
   # Install unreal package for IDE support
   pip install unreal
   ```

3. **Set API Keys (for testing)**
   ```bash
   export PS_OPENAIAPIKEY="your_test_key"
   export PS_ANTHROPICAPIKEY="your_test_key"
   export PS_DEEPSEEKAPIKEY="your_test_key"
   ```

4. **Generate Project Files**
   - Right-click `.uproject` file → "Generate Visual Studio project files"
   - Open solution in Visual Studio/Xcode

## Contribution Areas

### 🚀 High Priority Areas

1. **New AI Service Integration**
   - Google Gemini API implementation
   - Meta Llama API integration
   - Baidu API support

2. **MCP Enhancements**
   - Additional Blueprint node types
   - Enhanced scene manipulation tools
   - UI/UMG automation improvements

3. **Testing & Documentation**
   - Automated testing framework
   - Comprehensive API documentation
   - Video tutorials and examples

### 🔧 Medium Priority Areas

1. **Performance Optimization**
   - HTTP request pooling
   - Memory usage optimization
   - Blueprint compilation improvements

2. **Developer Experience**
   - Better error messages
   - Enhanced logging system
   - Development tools

3. **Platform Support**
   - Linux development support
   - Additional Unreal Engine versions
   - Package optimization

### 📝 Always Welcome

- **Bug fixes** with reproduction steps
- **Documentation improvements** 
- **Example projects** and tutorials
- **Code quality improvements**
- **Translation** of documentation

## Development Workflow

### Branch Strategy

- **main**: Stable, production-ready code
- **develop**: Integration branch for new features
- **feature/feature-name**: Individual feature development
- **bugfix/issue-description**: Bug fixes
- **hotfix/critical-fix**: Critical production fixes

### Commit Guidelines

Follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes
- `refactor`: Code refactoring
- `test`: Adding tests
- `chore`: Maintenance tasks

**Examples:**
```bash
feat(openai): add GPT-4.1 model support
fix(mcp): resolve Blueprint node connection issues
docs(api): update Anthropic integration examples
style(python): apply PEP 8 formatting to handlers
```

### Pull Request Process

1. **Create Feature Branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Develop and Test**
   - Follow coding standards
   - Add comprehensive tests
   - Update documentation

3. **Submit Pull Request**
   - Clear title and description
   - Reference related issues
   - Include testing instructions

4. **Code Review**
   - Address reviewer feedback
   - Ensure CI passes
   - Maintain code quality

## Code Quality Standards

### C++ Standards

- **Unreal Engine Coding Standards** compliance
- **Gen prefix** for all plugin classes
- **USTRUCT/UCLASS/UFUNCTION** for Blueprint integration
- **Smart pointers** and proper memory management
- **Comprehensive error handling**

### Python Standards

- **PEP 8** compliance (enforced by black)
- **Type hints** for all functions
- **PEP 257** compliant docstrings
- **pytest** for all test cases
- **mypy** type checking

### Documentation Standards

- **English-only** documentation
- **Detailed code examples** for all features
- **Mermaid diagrams** for complex flows
- **Relative links** between documents
- **Consistent formatting** and style

## Testing Requirements

### Required Tests

- **Unit tests** for all new functionality
- **Integration tests** for API endpoints
- **MCP command tests** for new tools
- **Blueprint tests** for visual scripting
- **Performance tests** for critical paths

### Test Structure

```python
# Python test example
import pytest
from unittest.mock import Mock, patch

class TestNewFeature:
    """Tests for new feature implementation."""
    
    def setup_method(self) -> None:
        """Setup before each test."""
        self.mock_service = Mock()
    
    @pytest.mark.asyncio
    async def test_feature_success(self) -> None:
        """Test successful feature execution."""
        # Given
        input_data = "test input"
        
        # When
        result = await new_feature(input_data)
        
        # Then
        assert result.success is True
        assert result.data is not None
```

### Running Tests

```bash
# Python tests
pytest Content/Python/tests/ -v

# C++ tests (if available)
# Run through Unreal Engine testing framework

# MCP integration tests
python test_mcp_integration.py
```

## Common Contribution Patterns

### Adding New AI Service

1. **Create Data Structures**
   ```cpp
   // Source/GenerativeAISupport/Public/Data/ServiceName/GenServiceStructs.h
   USTRUCT(BlueprintType)
   struct FGenServiceSettings
   {
       GENERATED_BODY()
       // Service-specific settings
   };
   ```

2. **Implement Async Action**
   ```cpp
   // Source/GenerativeAISupport/Public/Models/ServiceName/GenServiceChat.h
   UCLASS()
   class GENERATIVEAISUPPORT_API UGenServiceChat : public UCancellableAsyncAction
   {
       GENERATED_BODY()
       // Service implementation
   };
   ```

3. **Add Blueprint Integration**
   ```cpp
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "GenAI|ServiceName")
   static UGenServiceChat* RequestServiceChat(UObject* WorldContextObject, const FGenServiceSettings& Settings);
   ```

### Adding MCP Command

1. **Define Tool in MCP Server**
   ```python
   @mcp.tool()
   def new_command(parameter: str) -> str:
       """New MCP command description."""
       command = {
           "type": "new_command",
           "parameter": parameter
       }
       return send_to_unreal(command)
   ```

2. **Create Handler Function**
   ```python
   def handle_new_command(command: Dict[str, Any]) -> Dict[str, Any]:
       """Handle new command implementation."""
       try:
           parameter = command.get("parameter")
           # Implementation logic
           return {"success": True, "result": "success"}
       except Exception as e:
           return {"success": False, "error": str(e)}
   ```

3. **Add C++ Backend (if needed)**
   ```cpp
   UFUNCTION(BlueprintCallable, Category = "GenAI|MCP", CallInEditor = true)
   static bool ExecuteNewCommand(const FString& Parameter);
   ```

## Community Guidelines

### Communication

- **Be respectful** and inclusive
- **Use English** for all communication
- **Provide context** in discussions
- **Ask questions** when unsure
- **Share knowledge** with others

### Issue Reporting

- **Search existing issues** before creating new ones
- **Provide reproduction steps** for bugs
- **Include system information** (OS, Unreal version, etc.)
- **Use appropriate labels** and templates
- **Follow up** on your reports

### Code Reviews

- **Be constructive** in feedback
- **Explain reasoning** behind suggestions
- **Test changes** when possible
- **Respond promptly** to reviews
- **Learn from feedback**

## Recognition

Contributors are recognized through:

- **GitHub Contributors** section
- **Release notes** acknowledgments
- **Community highlights** in discussions
- **Maintainer status** for significant contributors

## Resources

### Documentation
- [Architecture Overview](../Architecture/README.md)
- [API Integration Guide](../API/README.md)
- [MCP System Documentation](../MCP/README.md)

### Development Tools
- [Unreal Engine Documentation](https://docs.unrealengine.com/)
- [Model Control Protocol](https://modelcontextprotocol.io/)
- [Python Type Checking](https://mypy.readthedocs.io/)

### Community
- [GitHub Discussions](https://github.com/prajwalshettydev/UnrealGenAISupport/discussions)
- [Issue Tracker](https://github.com/prajwalshettydev/UnrealGenAISupport/issues)
- [Project Creator](https://prajwalshetty.com/)

## Getting Help

If you need help with contributing:

1. **Check existing documentation** in this guide
2. **Search GitHub issues** for similar questions
3. **Start a discussion** on GitHub Discussions
4. **Contact maintainers** through appropriate channels

---

Thank you for contributing to UnrealGenAISupport! Your contributions help make AI integration in Unreal Engine more accessible and powerful for developers worldwide.