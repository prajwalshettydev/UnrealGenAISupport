# Coding Standards

This document outlines the comprehensive coding standards for the UnrealGenAISupport plugin. These standards ensure code consistency, maintainability, and quality across all contributions.

## General Principles

### Code Quality Principles

1. **Readability First**: Code should be self-documenting and easy to understand
2. **Consistency**: Follow established patterns throughout the codebase
3. **Performance**: Write efficient code without premature optimization
4. **Maintainability**: Design for easy modification and extension
5. **Testing**: All code should be testable and well-tested

### SOLID Principles

- **S**ingle Responsibility Principle: One class, one responsibility
- **O**pen/Closed Principle: Open for extension, closed for modification
- **L**iskov Substitution Principle: Subtypes must be substitutable
- **I**nterface Segregation Principle: Small, focused interfaces
- **D**ependency Inversion Principle: Depend on abstractions

### DRY and YAGNI

- **DRY** (Don't Repeat Yourself): Eliminate code duplication
- **YAGNI** (You Ain't Gonna Need It): Implement only what's needed now

## C++ Coding Standards

### General C++ Guidelines

Follow the [Unreal Engine Coding Standards](https://docs.unrealengine.com/5.1/en-US/epic-cplusplus-coding-standard-for-unreal-engine/) with plugin-specific additions.

### Naming Conventions

#### Classes and Structs
```cpp
// Plugin classes use 'Gen' prefix
class GENERATIVEAISUPPORT_API UGenOAIChat : public UCancellableAsyncAction
{
    GENERATED_BODY()
};

// Structs use 'F' prefix with 'Gen' prefix
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FGenChatMessage
{
    GENERATED_BODY()
};

// Enums use 'E' prefix with 'Gen' prefix  
UENUM(BlueprintType)
enum class EGenOAIChatModel : uint8
{
    GPT_4o,
    GPT_4o_Mini
};
```

#### Functions and Variables
```cpp
class UGenOAIChat
{
public:
    // Public functions: PascalCase
    static void SendChatRequest(const FGenChatSettings& Settings);
    
    // Blueprint functions: PascalCase with descriptive names
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI")
    static UGenOAIChat* RequestOpenAIChat(UObject* WorldContextObject, const FGenChatSettings& ChatSettings);

private:
    // Member variables: PascalCase with type prefix
    FGenChatSettings ChatSettings;
    bool bIsRequestActive = false;
    int32 RetryCount = 0;
    float RequestTimeout = 30.0f;
    
    // Private functions: PascalCase
    void ProcessResponse(const FString& ResponseStr);
    void HandleError(const FString& ErrorMessage);
};
```

#### Constants and Macros
```cpp
// Constants: PascalCase in namespace
namespace GenAIConstants
{
    static const int32 MaxMessageLength = 4096;
    static const float DefaultTimeout = 30.0f;
    static const FString DefaultModel = TEXT("gpt-4o");
}

// Macros: UPPER_SNAKE_CASE with GEN prefix
#define GEN_API_VALIDATE_SETTINGS(Settings) \
    if (!ValidateSettings(Settings)) { return; }
```

### File Structure

#### Header File Template
```cpp
// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"
#include "Data/OpenAI/GenOAIChatStructs.h"
#include "GenOAIChat.generated.h"

// Forward declarations
class FJsonObject;

// Delegates
DECLARE_DELEGATE_ThreeParams(FOnChatCompletionResponse, const FString&, const FString&, bool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGenChatCompletionDelegate, 
    const FString&, Response, 
    const FString&, Error, 
    bool, Success);

/**
 * Async action for OpenAI chat completions with Blueprint support.
 * 
 * Features:
 * - Cancellable async operations
 * - Blueprint and C++ integration
 * - Comprehensive error handling
 * - Model enum support with custom model options
 * 
 * Usage:
 * @code
 * FGenChatSettings Settings;
 * Settings.ModelEnum = EGenOAIChatModel::GPT_4o;
 * Settings.Messages.Add(FGenChatMessage{TEXT("user"), TEXT("Hello!")});
 * 
 * UGenOAIChat::SendChatRequest(Settings, 
 *     FOnChatCompletionResponse::CreateLambda([](const FString& Response, const FString& Error, bool Success) {
 *         if (Success) {
 *             UE_LOG(LogTemp, Log, TEXT("Response: %s"), *Response);
 *         }
 *     }));
 * @endcode
 */
UCLASS()
class GENERATIVEAISUPPORT_API UGenOAIChat : public UCancellableAsyncAction
{
    GENERATED_BODY()

public:
    UGenOAIChat();

    // Static function for native C++ integration
    static void SendChatRequest(const FGenChatSettings& ChatSettings, const FOnChatCompletionResponse& OnComplete);

    // Blueprint async delegate
    UPROPERTY(BlueprintAssignable)
    FGenChatCompletionDelegate OnComplete;

    // Blueprint latent function
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "GenAI|OpenAI")
    static UGenOAIChat* RequestOpenAIChat(UObject* WorldContextObject, const FGenChatSettings& ChatSettings);

protected:
    //~ Begin UCancellableAsyncAction Interface
    virtual void Activate() override;
    virtual void Cancel() override;
    //~ End UCancellableAsyncAction Interface

private:
    /** Settings for the current chat request */
    FGenChatSettings ChatSettings;

    /** Shared implementation for both C++ and Blueprint paths */
    static void MakeRequest(const FGenChatSettings& ChatSettings, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);
    
    /** Process HTTP response and extract chat completion */
    static void ProcessResponse(const FString& ResponseStr, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);
    
    /** Validate settings before making request */
    static bool ValidateSettings(const FGenChatSettings& Settings);
};
```

#### Implementation File Template
```cpp
// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License.

#include "Models/OpenAI/GenOAIChat.h"
#include "Engine/Engine.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Secure/GenSecureKey.h"
#include "Data/GenAIOrgs.h"

UGenOAIChat::UGenOAIChat()
{
    // Initialize default values
}

void UGenOAIChat::SendChatRequest(const FGenChatSettings& ChatSettings, const FOnChatCompletionResponse& OnComplete)
{
    if (!ValidateSettings(ChatSettings))
    {
        OnComplete.ExecuteIfBound(TEXT(""), TEXT("Invalid chat settings"), false);
        return;
    }

    MakeRequest(ChatSettings, [OnComplete](const FString& Response, const FString& Error, bool Success)
    {
        OnComplete.ExecuteIfBound(Response, Error, Success);
    });
}

// ... rest of implementation
```

### Blueprint Integration Standards

#### USTRUCT Definition
```cpp
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FGenChatSettings
{
    GENERATED_BODY()

    /** Model selection using enum for type safety */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI", 
              meta = (DisplayName = "AI Model"))
    EGenOAIChatModel ModelEnum = EGenOAIChatModel::GPT_4o;
    
    /** Custom model name when ModelEnum is set to Custom */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI", 
              meta = (EditCondition = "ModelEnum == EGenOAIChatModel::Custom", 
                      EditConditionHides = true,
                      DisplayName = "Custom Model Name"))
    FString CustomModel;

    /** Maximum number of tokens to generate */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI",
              meta = (ClampMin = "1", ClampMax = "128000", 
                      DisplayName = "Max Tokens",
                      ToolTip = "Maximum number of tokens to generate in the response"))
    int32 MaxTokens = 10000;

    /** Array of messages for the conversation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI",
              meta = (DisplayName = "Messages",
                      ToolTip = "Conversation messages with role and content"))
    TArray<FGenChatMessage> Messages;

    /** Helper function to ensure model string is correct */
    void UpdateModel()
    {
        if (ModelEnum == EGenOAIChatModel::Custom && !CustomModel.IsEmpty())
        {
            Model = CustomModel;
        }
        else
        {
            Model = UGenOAIModelUtils::ChatModelToString(ModelEnum);
        }
    }

    /** Default constructor */
    FGenChatSettings()
    {
        UpdateModel();
    }

private:
    /** Internal model string - automatically updated from enum or custom */
    UPROPERTY(BlueprintReadOnly, Category = "GenAI|OpenAI")
    FString Model = TEXT("gpt-4o");
};
```

### Memory Management

#### Smart Pointers and UPROPERTY
```cpp
class GENERATIVEAISUPPORT_API UGenAIServiceManager : public UObject
{
    GENERATED_BODY()

public:
    // Strong references for owned objects
    UPROPERTY()
    TObjectPtr<UGenOAIChat> OpenAIService;

    // Weak references for non-owned objects
    UPROPERTY()
    TWeakObjectPtr<UWorld> CachedWorld;

    // Arrays with proper UPROPERTY
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Services")
    TArray<TSubclassOf<UGenAIServiceBase>> AvailableServices;

    // Maps for lookup tables
    UPROPERTY()
    TMap<FString, TObjectPtr<UGenAIServiceBase>> ServiceRegistry;

    // SharedPtr for non-UObject data
    TSharedPtr<FJsonObject> CachedConfiguration;

    // UniquePtr for exclusive ownership
    TUniquePtr<FGenAIHttpManager> HttpManager;
};
```

### Error Handling Patterns

#### TOptional for Fallible Operations
```cpp
class UGenAIUtils
{
public:
    /**
     * Parse model string with error handling
     * @param ModelString Input model string
     * @return TOptional containing parsed model enum, or empty if parsing failed
     */
    static TOptional<EGenOAIChatModel> ParseModelString(const FString& ModelString)
    {
        if (ModelString.IsEmpty())
        {
            UE_LOG(LogGenAI, Warning, TEXT("Empty model string provided"));
            return {};
        }

        // Parse model string
        if (ModelString == TEXT("gpt-4o"))
        {
            return EGenOAIChatModel::GPT_4o;
        }
        else if (ModelString == TEXT("gpt-4o-mini"))
        {
            return EGenOAIChatModel::GPT_4o_Mini;
        }

        UE_LOG(LogGenAI, Warning, TEXT("Unknown model string: %s"), *ModelString);
        return {};
    }

    /**
     * Safe string conversion with validation
     */
    static bool ConvertToFloat(const FString& StringValue, float& OutValue)
    {
        if (StringValue.IsNumeric())
        {
            OutValue = FCString::Atof(*StringValue);
            return true;
        }
        
        UE_LOG(LogGenAI, Error, TEXT("Invalid float string: %s"), *StringValue);
        return false;
    }
};
```

#### Custom Exception Classes
```cpp
// Custom exception hierarchy for detailed error handling
UENUM(BlueprintType)
enum class EGenAIErrorType : uint8
{
    None            UMETA(DisplayName = "No Error"),
    InvalidInput    UMETA(DisplayName = "Invalid Input"),
    NetworkError    UMETA(DisplayName = "Network Error"), 
    APIError        UMETA(DisplayName = "API Error"),
    TimeoutError    UMETA(DisplayName = "Timeout Error"),
    AuthError       UMETA(DisplayName = "Authentication Error")
};

USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FGenAIError
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Error")
    EGenAIErrorType ErrorType = EGenAIErrorType::None;

    UPROPERTY(BlueprintReadOnly, Category = "Error")
    FString ErrorMessage;

    UPROPERTY(BlueprintReadOnly, Category = "Error")
    FString ErrorCode;

    UPROPERTY(BlueprintReadOnly, Category = "Error")
    int32 HTTPStatusCode = 0;

    FGenAIError() = default;

    FGenAIError(EGenAIErrorType InErrorType, const FString& InMessage, const FString& InCode = TEXT(""))
        : ErrorType(InErrorType), ErrorMessage(InMessage), ErrorCode(InCode)
    {
    }

    bool IsValid() const { return ErrorType != EGenAIErrorType::None; }
    FString ToString() const { return FString::Printf(TEXT("[%s] %s"), *ErrorCode, *ErrorMessage); }
};
```

## Python Coding Standards

### PEP 8 Compliance

All Python code must follow [PEP 8](https://peps.python.org/pep-0008/) with the following specific guidelines:

#### Code Formatting
```python
# Use black formatter with 88 character line limit
# pyproject.toml
[tool.black]
line-length = 88
target-version = ['py310']
include = '\.pyi?$'
extend-exclude = '''
/(
  # Exclude auto-generated files
  | .*_pb2\.py
)/
'''
```

#### Import Organization
```python
"""
MCP command handler for Blueprint operations.

This module provides handlers for Blueprint creation, manipulation,
and node management through the MCP protocol.

Example usage:
    from handlers.blueprint_commands import handle_create_blueprint
    
    command = {"type": "create_blueprint", "blueprint_name": "MyActor"}
    result = handle_create_blueprint(command)
"""

from __future__ import annotations

# Standard library imports
import json
import logging
from typing import Any, Dict, List, Optional, Union
from pathlib import Path

# Third-party imports
import unreal

# Local imports  
from utils import unreal_conversions as uc
from utils import logging as log
from .base_handler import BaseHandler

logger = logging.getLogger(__name__)
```

### Type Hints and Documentation

#### Function Documentation
```python
from typing import Dict, Any, Optional, List, Protocol
from dataclasses import dataclass

@dataclass
class CommandResult:
    """Result of command execution with detailed information.
    
    Attributes:
        success: Whether the command executed successfully
        message: Human-readable result message
        data: Optional additional data from command execution
        error_code: Optional error code for failed commands
    """
    success: bool
    message: str
    data: Optional[Dict[str, Any]] = None
    error_code: Optional[str] = None

def handle_create_blueprint(command: Dict[str, Any]) -> CommandResult:
    """
    Handle a command to create a new Blueprint from a specified parent class.
    
    This function creates a new Blueprint asset in Unreal Engine using the
    provided parameters. It validates input parameters, creates the Blueprint
    through the C++ utility functions, and returns a detailed result.
    
    Args:
        command: The command dictionary containing:
            - blueprint_name (str): Name for the new Blueprint
            - parent_class (str): Parent class name or path (e.g., "Actor", "/Script/Engine.Actor")
            - save_path (str): Path to save the Blueprint asset (e.g., "/Game/Blueprints")
            
    Returns:
        CommandResult containing:
            - success: True if Blueprint was created successfully
            - message: Descriptive message about the operation
            - data: Dictionary with blueprint_path if successful
            - error_code: Error code if operation failed
    
    Raises:
        ValueError: If required parameters are missing
        UnrealError: If Blueprint creation fails in Unreal Engine
        
    Example:
        >>> command = {
        ...     "blueprint_name": "MyActor",
        ...     "parent_class": "Actor", 
        ...     "save_path": "/Game/Blueprints"
        ... }
        >>> result = handle_create_blueprint(command)
        >>> print(f"Success: {result.success}")
        Success: True
    """
    try:
        # Extract and validate parameters
        blueprint_name = command.get("blueprint_name")
        if not blueprint_name:
            return CommandResult(
                success=False,
                message="Missing required parameter: blueprint_name",
                error_code="MISSING_PARAMETER"
            )
        
        parent_class = command.get("parent_class", "Actor")
        save_path = command.get("save_path", "/Game/Blueprints")

        # Log command execution
        log.log_command("create_blueprint", f"Name: {blueprint_name}, Parent: {parent_class}")

        # Call the C++ implementation
        gen_bp_utils = unreal.GenBlueprintUtils
        blueprint = gen_bp_utils.create_blueprint(blueprint_name, parent_class, save_path)

        if blueprint:
            blueprint_path = f"{save_path}/{blueprint_name}"
            log.log_result("create_blueprint", True, f"Path: {blueprint_path}")
            
            return CommandResult(
                success=True,
                message=f"Successfully created Blueprint '{blueprint_name}'",
                data={"blueprint_path": blueprint_path}
            )
        else:
            error_msg = f"Failed to create Blueprint {blueprint_name}"
            log.log_error(error_msg)
            
            return CommandResult(
                success=False,
                message=error_msg,
                error_code="CREATION_FAILED"
            )

    except Exception as e:
        error_msg = f"Error creating blueprint: {str(e)}"
        log.log_error(error_msg, include_traceback=True)
        
        return CommandResult(
            success=False,
            message=error_msg,
            error_code="EXCEPTION"
        )
```

#### Class Design Patterns
```python
from abc import ABC, abstractmethod
from typing import Protocol, runtime_checkable

@runtime_checkable
class CommandHandler(Protocol):
    """Protocol for MCP command handlers."""
    
    def handle_command(self, command: Dict[str, Any]) -> CommandResult:
        """Handle MCP command and return result."""
        ...

class BaseHandler(ABC):
    """Base class for all command handlers with common functionality."""
    
    def __init__(self, logger_name: Optional[str] = None) -> None:
        """Initialize handler with optional custom logger."""
        self.logger = logging.getLogger(logger_name or self.__class__.__name__)
    
    @abstractmethod
    def handle_command(self, command: Dict[str, Any]) -> CommandResult:
        """Handle command - must be implemented by subclasses."""
        pass
    
    def validate_required_params(self, command: Dict[str, Any], required: List[str]) -> Optional[str]:
        """
        Validate that all required parameters are present.
        
        Args:
            command: Command dictionary to validate
            required: List of required parameter names
            
        Returns:
            None if validation passes, error message if validation fails
        """
        missing = [param for param in required if param not in command]
        if missing:
            return f"Missing required parameters: {', '.join(missing)}"
        return None

class BlueprintHandler(BaseHandler):
    """Handler for Blueprint-related MCP commands."""
    
    def handle_command(self, command: Dict[str, Any]) -> CommandResult:
        """Route Blueprint commands to appropriate handler methods."""
        command_type = command.get("type", "")
        
        if command_type == "create_blueprint":
            return self._handle_create_blueprint(command)
        elif command_type == "add_component":
            return self._handle_add_component(command)
        else:
            return CommandResult(
                success=False,
                message=f"Unknown Blueprint command: {command_type}",
                error_code="UNKNOWN_COMMAND"
            )
    
    def _handle_create_blueprint(self, command: Dict[str, Any]) -> CommandResult:
        """Handle Blueprint creation command."""
        # Validation
        error = self.validate_required_params(command, ["blueprint_name"])
        if error:
            return CommandResult(success=False, message=error, error_code="VALIDATION_ERROR")
        
        # Implementation...
        pass
```

### Error Handling and Logging

#### Context Managers for Error Handling
```python
from contextlib import contextmanager
from typing import Generator, Optional

@contextmanager
def error_handler(operation: str, logger: Optional[logging.Logger] = None) -> Generator[None, None, None]:
    """
    Context manager for consistent error handling and logging.
    
    Args:
        operation: Name of the operation being performed
        logger: Optional logger instance
        
    Yields:
        None
        
    Raises:
        Re-raises any exceptions that occur within the context
    """
    log = logger or logging.getLogger(__name__)
    
    try:
        log.debug(f"Starting operation: {operation}")
        yield
        log.debug(f"Completed operation: {operation}")
        
    except Exception as e:
        log.error(f"Error in {operation}: {e}", exc_info=True)
        raise

# Usage example
def create_blueprint_with_error_handling(blueprint_name: str) -> CommandResult:
    """Create Blueprint with comprehensive error handling."""
    
    with error_handler("blueprint_creation"):
        # Validate input
        if not blueprint_name or not blueprint_name.strip():
            raise ValueError("Blueprint name cannot be empty")
        
        # Create Blueprint
        blueprint = unreal.GenBlueprintUtils.create_blueprint(
            blueprint_name, "Actor", "/Game/Blueprints"
        )
        
        if not blueprint:
            raise RuntimeError("Blueprint creation returned None")
        
        return CommandResult(
            success=True,
            message=f"Created Blueprint: {blueprint_name}"
        )
```

#### Custom Exception Hierarchy
```python
class UnrealGenAIError(Exception):
    """Base exception for UnrealGenAI operations."""
    
    def __init__(self, message: str, error_code: Optional[str] = None) -> None:
        super().__init__(message)
        self.error_code = error_code
        self.message = message

class CommandError(UnrealGenAIError):
    """Exception for MCP command execution errors."""
    pass

class ValidationError(UnrealGenAIError):
    """Exception for parameter validation errors."""
    pass

class UnrealEngineError(UnrealGenAIError):
    """Exception for Unreal Engine integration errors."""
    pass

class BlueprintError(UnrealEngineError):
    """Exception for Blueprint-specific operations."""
    pass

# Usage with specific error types
def handle_add_component(command: Dict[str, Any]) -> CommandResult:
    """Add component with specific error handling."""
    try:
        blueprint_path = command.get("blueprint_path")
        if not blueprint_path:
            raise ValidationError("blueprint_path is required", "MISSING_BLUEPRINT_PATH")
        
        # Attempt to add component
        success = unreal.GenBlueprintUtils.add_component(
            blueprint_path, command["component_class"], command.get("component_name", "")
        )
        
        if not success:
            raise BlueprintError(
                f"Failed to add component to {blueprint_path}", 
                "COMPONENT_ADD_FAILED"
            )
        
        return CommandResult(success=True, message="Component added successfully")
        
    except (ValidationError, BlueprintError) as e:
        return CommandResult(
            success=False, 
            message=e.message, 
            error_code=e.error_code
        )
    except Exception as e:
        return CommandResult(
            success=False, 
            message=f"Unexpected error: {str(e)}", 
            error_code="UNEXPECTED_ERROR"
        )
```

### Testing Standards

#### Test Structure and Patterns
```python
import pytest
from unittest.mock import Mock, patch, AsyncMock
from typing import Any, Dict

class TestBlueprintCommands:
    """Test suite for Blueprint command handlers."""
    
    def setup_method(self) -> None:
        """Setup method called before each test."""
        self.mock_unreal = Mock()
        self.handler = BlueprintHandler()
        
        # Mock Unreal Engine modules
        self.unreal_patcher = patch('unreal.GenBlueprintUtils')
        self.mock_bp_utils = self.unreal_patcher.start()
    
    def teardown_method(self) -> None:
        """Cleanup method called after each test."""
        self.unreal_patcher.stop()
    
    @pytest.mark.parametrize("blueprint_name,parent_class,expected_success", [
        ("ValidBlueprint", "Actor", True),
        ("", "Actor", False),  # Empty name should fail
        ("ValidBlueprint", "", False),  # Empty parent should fail
        (None, "Actor", False),  # None name should fail
    ])
    def test_create_blueprint_validation(
        self, 
        blueprint_name: str, 
        parent_class: str, 
        expected_success: bool
    ) -> None:
        """Test Blueprint creation with various parameter combinations."""
        # Given
        command = {
            "type": "create_blueprint",
            "blueprint_name": blueprint_name,
            "parent_class": parent_class
        }
        
        # Configure mock
        if expected_success:
            self.mock_bp_utils.create_blueprint.return_value = Mock()
        else:
            self.mock_bp_utils.create_blueprint.return_value = None
        
        # When
        result = self.handler.handle_command(command)
        
        # Then
        assert result.success == expected_success
        if expected_success:
            assert "Successfully created" in result.message
            assert result.data is not None
            assert "blueprint_path" in result.data
        else:
            assert result.error_code is not None
    
    @pytest.mark.asyncio
    async def test_async_command_handling(self) -> None:
        """Test asynchronous command handling."""
        # Given
        async_handler = AsyncBlueprintHandler()
        command = {"type": "create_blueprint", "blueprint_name": "AsyncTest"}
        
        # When
        result = await async_handler.handle_command_async(command)
        
        # Then
        assert isinstance(result, CommandResult)
        assert result.success is True
    
    def test_error_handling_and_logging(self, caplog) -> None:
        """Test error handling and logging behavior."""
        # Given
        command = {"type": "create_blueprint", "blueprint_name": "TestBlueprint"}
        self.mock_bp_utils.create_blueprint.side_effect = Exception("Simulated error")
        
        # When
        with caplog.at_level(logging.ERROR):
            result = self.handler.handle_command(command)
        
        # Then
        assert result.success is False
        assert "Error creating blueprint" in result.message
        assert "Simulated error" in caplog.text
    
    @patch('handlers.blueprint_commands.log')
    def test_logging_integration(self, mock_log) -> None:
        """Test integration with logging system."""
        # Given
        command = {"type": "create_blueprint", "blueprint_name": "LogTest"}
        self.mock_bp_utils.create_blueprint.return_value = Mock()
        
        # When
        result = self.handler.handle_command(command)
        
        # Then
        assert result.success is True
        mock_log.log_command.assert_called_once()
        mock_log.log_result.assert_called_once()
```

## Documentation Standards

### Code Comments

#### C++ Documentation
```cpp
/**
 * @brief OpenAI Chat API integration with Blueprint support
 * 
 * This class provides async chat completion functionality for OpenAI's GPT models.
 * It supports both C++ and Blueprint integration with proper cancellation and
 * error handling.
 * 
 * @note This class inherits from UCancellableAsyncAction to provide proper
 *       async behavior in both C++ and Blueprint contexts.
 * 
 * @warning API keys must be configured via environment variables before use.
 * 
 * @see FGenChatSettings for configuration options
 * @see UGenSecureKey for API key management
 * 
 * @since Plugin Version 1.0
 * @author Plugin Development Team
 */
class GENERATIVEAISUPPORT_API UGenOAIChat : public UCancellableAsyncAction
{
    /**
     * @brief Send chat request using static C++ interface
     * 
     * This method provides a clean C++ interface for sending chat requests
     * without requiring Blueprint integration. Ideal for C++ game logic.
     * 
     * @param ChatSettings Configuration for the chat request including model and messages
     * @param OnComplete Delegate called when request completes (success or failure)
     * 
     * @note The delegate will always be called, either with success=true and a response,
     *       or success=false with an error message.
     * 
     * @par Example Usage:
     * @code
     * FGenChatSettings Settings;
     * Settings.ModelEnum = EGenOAIChatModel::GPT_4o;
     * Settings.Messages.Add(FGenChatMessage{TEXT("user"), TEXT("Hello!")});
     * 
     * UGenOAIChat::SendChatRequest(Settings, 
     *     FOnChatCompletionResponse::CreateLambda([](const FString& Response, const FString& Error, bool Success) {
     *         if (Success) {
     *             UE_LOG(LogTemp, Log, TEXT("AI Response: %s"), *Response);
     *         } else {
     *             UE_LOG(LogTemp, Error, TEXT("AI Error: %s"), *Error);
     *         }
     *     }));
     * @endcode
     */
    static void SendChatRequest(const FGenChatSettings& ChatSettings, const FOnChatCompletionResponse& OnComplete);
};
```

#### Python Documentation  
```python
def handle_create_blueprint(command: Dict[str, Any]) -> CommandResult:
    """
    Create a new Blueprint asset in Unreal Engine.
    
    This function handles the MCP command for creating Blueprint assets. It validates
    the input parameters, creates the Blueprint using Unreal Engine's C++ utilities,
    and returns a structured result with success/failure information.
    
    The Blueprint will be created with the specified parent class and saved to the
    designated path. If no parent class is specified, it defaults to "Actor".
    
    Args:
        command: MCP command dictionary with the following keys:
            - blueprint_name (str): Name for the new Blueprint asset
            - parent_class (str, optional): Parent class name or path. 
                                           Defaults to "Actor" if not specified.
                                           Examples: "Actor", "/Script/Engine.Pawn"
            - save_path (str, optional): Asset path for saving the Blueprint.
                                        Defaults to "/Game/Blueprints" if not specified.
                                        
    Returns:
        CommandResult: Structured result containing:
            - success (bool): True if Blueprint creation succeeded
            - message (str): Human-readable description of the result
            - data (dict, optional): Additional data including 'blueprint_path' on success
            - error_code (str, optional): Machine-readable error code on failure
            
    Raises:
        This function catches all exceptions and returns them as CommandResult
        with success=False. No exceptions propagate to the caller.
        
    Note:
        The function uses Unreal Engine's GenBlueprintUtils C++ class internally.
        The Blueprint will be automatically saved and compiled after creation.
        
    Example:
        >>> command = {
        ...     "type": "create_blueprint",
        ...     "blueprint_name": "PlayerCharacter",
        ...     "parent_class": "Character",
        ...     "save_path": "/Game/Characters"
        ... }
        >>> result = handle_create_blueprint(command)
        >>> if result.success:
        ...     print(f"Created Blueprint at: {result.data['blueprint_path']}")
        ... else:
        ...     print(f"Failed: {result.message}")
    """
```

### README and Documentation Files

Each module should include comprehensive README files following this template:

```markdown
# Module Name

Brief description of the module's purpose and functionality.

## Overview

Detailed explanation of what this module does, its role in the larger system,
and how it fits into the plugin architecture.

## Features

- Feature 1: Description
- Feature 2: Description  
- Feature 3: Description

## Usage

### Basic Usage
```python
# Simple example
from module import function
result = function(parameters)
```

### Advanced Usage
```python
# Complex example with error handling
try:
    result = advanced_function(complex_parameters)
    if result.success:
        print(f"Success: {result.data}")
    else:
        print(f"Error: {result.error_code}")
except ModuleError as e:
    print(f"Module error: {e}")
```

## API Reference

### Functions

#### function_name(param1, param2)
Description of function, parameters, return values, and examples.

### Classes

#### ClassName
Description of class, its purpose, and usage examples.

## Configuration

Description of any configuration options, environment variables, or settings.

## Error Handling

Description of error conditions, exception types, and troubleshooting guidance.

## Examples

Working examples that demonstrate real-world usage scenarios.

## See Also

- [Related Module 1](../module1/README.md)
- [Related Module 2](../module2/README.md)
- [External Documentation](https://example.com)
```

---

These coding standards ensure consistency, maintainability, and quality across the UnrealGenAISupport codebase. All contributors should follow these guidelines to maintain the high standards of the project.