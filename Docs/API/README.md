# API Integrations

Complete documentation for all AI service integrations in the UnrealGenAISupport plugin. This guide covers implementation details, usage patterns, and best practices for integrating multiple AI providers into your Unreal Engine projects.

## Overview

The UnrealGenAISupport plugin provides seamless integration with multiple AI services through a unified, consistent API design. All services follow the same architectural patterns while maintaining their unique capabilities and features.

### Supported AI Services

| Service | Status | Features | Models |
|---------|--------|----------|---------|
| [OpenAI](OpenAI/README.md) | ✅ Complete | Chat, Structured Outputs, Multiple Models | GPT-4o, GPT-4.1, O3, O4-mini |
| [Anthropic](Anthropic/README.md) | ✅ Complete | Chat, Claude 4 Models, Temperature Control | Claude 4 Opus/Sonnet, Claude 3.5 family |
| [DeepSeek](DeepSeek/README.md) | ✅ Complete | Chat, Reasoning Models, R1 Support | DeepSeek Chat, DeepSeek Reasoner |
| [XAI](XAI/README.md) | ✅ Complete | Chat, Grok Models | Grok 3 Latest, Grok 3 Mini |
| Google Gemini | 🚧 Planned | Chat, Vision, Multimodal | Gemini 2.0 Flash, Gemini 1.5 |
| Meta Llama | 🚧 Planned | Chat, Local Models | Llama 4 Herd, Llama 3.3 |

## Quick Start

### 1. Add to Project

Add the plugin module to your project's `Build.cs` file:

```cpp
PrivateDependencyModuleNames.AddRange(new string[] { 
    "GenerativeAISupport" 
});
```

### 2. Set API Keys

Configure API keys using environment variables (recommended):

```bash
# Windows
setx PS_OPENAIAPIKEY "your_openai_key"
setx PS_ANTHROPICAPIKEY "your_anthropic_key"
setx PS_DEEPSEEKAPIKEY "your_deepseek_key"
setx PS_XAIAPIKEY "your_xai_key"

# macOS/Linux
export PS_OPENAIAPIKEY="your_openai_key"
export PS_ANTHROPICAPIKEY="your_anthropic_key"
export PS_DEEPSEEKAPIKEY="your_deepseek_key"
export PS_XAIAPIKEY="your_xai_key"
```

### 3. Basic Usage Example

```cpp
#include "Models/OpenAI/GenOAIChat.h"

// Create chat settings
FGenChatSettings Settings;
Settings.ModelEnum = EGenOAIChatModel::GPT_4O;
Settings.MaxTokens = 1000;
Settings.Messages.Add(FGenChatMessage{TEXT("user"), TEXT("Hello, AI!")});

// Send request
UGenOAIChat::SendChatRequest(Settings, 
    FOnChatCompletionResponse::CreateLambda([](const FString& Response, const FString& Error, bool Success)
    {
        if (Success)
        {
            UE_LOG(LogTemp, Log, TEXT("AI Response: %s"), *Response);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AI Error: %s"), *Error);
        }
    }));
```

## Architecture Overview

### Common Design Patterns

All AI service integrations follow these consistent patterns:

#### 1. Dual Interface Design
- **C++ Interface**: Static functions with delegates for performance
- **Blueprint Interface**: Latent async nodes for visual scripting

#### 2. Async Action Pattern
```cpp
// All AI services inherit from UCancellableAsyncAction
class GENERATIVEAISUPPORT_API UGenOAIChat : public UCancellableAsyncAction
{
    GENERATED_BODY()
    
public:
    // C++ static interface
    static void SendChatRequest(const FGenChatSettings& Settings, const FOnChatCompletionResponse& OnComplete);
    
    // Blueprint latent interface
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
    static UGenOAIChat* RequestOpenAIChat(UObject* WorldContextObject, const FGenChatSettings& Settings);
};
```

#### 3. Standardized Response Pattern
All services return the same response format:
- **Response** (FString): The AI-generated content
- **Error** (FString): Error message if request failed
- **Success** (bool): Whether the request completed successfully

#### 4. Configuration Structures
```cpp
// Each service has its own settings structure
USTRUCT(BlueprintType)
struct FGenChatSettings  // OpenAI
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI")
    EGenOAIChatModel ModelEnum = EGenOAIChatModel::GPT_4O;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI")
    int32 MaxTokens = 10000;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI")
    TArray<FGenChatMessage> Messages;
};
```

## Authentication System

### Environment Variables (Recommended)

The plugin uses environment variables with the format `PS_[ORG]APIKEY`:

```cpp
enum class EGenAIOrgs : uint8
{
    OpenAI,     // PS_OPENAIAPIKEY
    Anthropic,  // PS_ANTHROPICAPIKEY
    DeepSeek,   // PS_DEEPSEEKAPIKEY
    XAI,        // PS_XAIAPIKEY
    Meta,       // PS_METAAPIKEY (reserved)
    Google      // PS_GOOGLEAPIKEY (reserved)
};
```

### Runtime API Keys (Alternative)

For testing or packaged builds (with proper security considerations):

```cpp
// Set API key at runtime
UGenSecureKey::SetGenAIApiKeyRuntime(EGenAIOrgs::OpenAI, TEXT("your-api-key"));

// Toggle between environment and runtime keys
UGenSecureKey::SetUseApiKeyFromEnvironmentVars(false);
```

### Security Best Practices

⚠️ **Important Security Notes:**
- Never hardcode API keys in source code
- Use environment variables for development
- For production, implement secure key storage
- Consider using backend proxy for client applications

## Error Handling

### Multi-Layer Validation

The plugin implements comprehensive error handling:

1. **API Key Validation**: Checks for empty or invalid keys
2. **Request Validation**: Validates HTTP request construction
3. **Response Validation**: Ensures valid HTTP response
4. **JSON Parsing**: Validates response JSON structure
5. **Content Validation**: Verifies expected response format

### Logging Categories

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogGenAI, Log, All);           // Main errors and info
DECLARE_LOG_CATEGORY_EXTERN(LogGenPerformance, Log, All);  // Performance metrics
DECLARE_LOG_CATEGORY_EXTERN(LogGenAIVerbose, Log, All);    // Detailed debugging
```

### Error Response Format

All services provide detailed error information:

```cpp
// Typical error handling pattern
if (!Success)
{
    // Error string contains detailed information:
    // - HTTP status codes
    // - API error messages
    // - Request validation failures
    // - Network connectivity issues
    UE_LOG(LogGenAI, Error, TEXT("API Error: %s"), *Error);
}
```

## Message Format

### Universal Message Structure

All services use a common message format for compatibility:

```cpp
USTRUCT(BlueprintType)
struct FGenChatMessage
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI")
    FString Role;      // "system", "user", "assistant"
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI")
    FString Content;   // Message content
    
    // Default constructor
    FGenChatMessage() = default;
    
    // Convenience constructor
    FGenChatMessage(const FString& InRole, const FString& InContent)
        : Role(InRole), Content(InContent) {}
};
```

### Role Definitions

- **system**: Instructions or context for the AI
- **user**: Messages from the user/application
- **assistant**: Previous AI responses (for conversation history)

### Example Message Arrays

```cpp
TArray<FGenChatMessage> Messages;

// System prompt
Messages.Add(FGenChatMessage{TEXT("system"), TEXT("You are a helpful game development assistant.")});

// User query
Messages.Add(FGenChatMessage{TEXT("user"), TEXT("How do I create a Blueprint in Unreal Engine?")});

// Previous AI response (for context)
Messages.Add(FGenChatMessage{TEXT("assistant"), TEXT("To create a Blueprint...")});

// Follow-up question
Messages.Add(FGenChatMessage{TEXT("user"), TEXT("Can you show me the C++ equivalent?")});
```

## Performance Considerations

### HTTP Configuration

The plugin automatically configures HTTP settings for optimal performance:

```cpp
// Recommended settings in DefaultEngine.ini for DeepSeek R1 reasoning model
[HTTP]
HttpConnectionTimeout=180
HttpReceiveTimeout=180
```

### Request Optimization

- **Connection Reuse**: HTTP connections are reused when possible
- **Timeout Handling**: Configurable timeouts for different model types
- **Memory Management**: Efficient string handling for large responses
- **Async Processing**: Non-blocking operations maintain game performance

### Blueprint Performance

- Use **Blueprint latent nodes** for UI applications
- Use **C++ static functions** for performance-critical code
- **Cache responses** when appropriate to avoid redundant API calls

## Blueprint Integration

### Latent Async Nodes

Each AI service provides Blueprint latent nodes:

```cpp
UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), 
          Category = "GenAI|OpenAI")
static UGenOAIChat* RequestOpenAIChat(UObject* WorldContextObject, const FGenChatSettings& ChatSettings);
```

### Blueprint Categories

All functions are organized under consistent categories:
- `GenAI|OpenAI` - OpenAI functions
- `GenAI|Claude` - Anthropic Claude functions
- `GenAI|DeepSeek` - DeepSeek functions
- `GenAI|XAI` - XAI functions

### Blueprint Examples

The plugin includes comprehensive Blueprint examples:
- **Location**: `Content/ExampleBlueprints/ChatAPIExamples.uasset`
- **Coverage**: All supported APIs with working examples
- **Usage**: Reference implementations for common patterns

## Testing and Validation

### API Testing

Use the provided Blueprint examples for testing:

1. Open `ChatAPIExamples.uasset` in the Blueprint editor
2. Configure API keys in environment variables
3. Run the example functions to test each service
4. Monitor output logs for responses and errors

### Integration Testing

```cpp
// Example integration test pattern
void TestAIIntegration()
{
    FGenChatSettings TestSettings;
    TestSettings.ModelEnum = EGenOAIChatModel::GPT_4O_Mini;  // Use cost-effective model for testing
    TestSettings.MaxTokens = 100;
    TestSettings.Messages.Add(FGenChatMessage{TEXT("user"), TEXT("Test message")});
    
    UGenOAIChat::SendChatRequest(TestSettings, 
        FOnChatCompletionResponse::CreateLambda([](const FString& Response, const FString& Error, bool Success)
        {
            if (Success)
            {
                UE_LOG(LogTemp, Log, TEXT("Integration test passed: %s"), *Response);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Integration test failed: %s"), *Error);
            }
        }));
}
```

## Service-Specific Documentation

### Detailed Integration Guides

- **[OpenAI Integration](OpenAI/README.md)** - Complete OpenAI API integration including structured outputs
- **[Anthropic Claude Integration](Anthropic/README.md)** - Claude models with temperature control and streaming
- **[DeepSeek Integration](DeepSeek/README.md)** - Chat and reasoning models with R1 support
- **[XAI Integration](XAI/README.md)** - Grok models for conversational AI

### Implementation Guides

Each service-specific guide includes:
- Model capabilities and limitations
- Configuration options and parameters
- C++ and Blueprint usage examples
- Best practices and optimization tips
- Troubleshooting common issues

## Migration and Compatibility

### Version Compatibility

The plugin maintains backward compatibility:
- **API interfaces** remain stable across versions
- **New models** are added without breaking existing code
- **Deprecation notices** provide migration guidance

### Multi-Service Strategy

Design your application to easily switch between AI services:

```cpp
// Abstract interface for AI services
class AIServiceInterface
{
public:
    virtual void SendMessage(const FString& Message, TFunction<void(FString, bool)> Callback) = 0;
};

// Service-specific implementations
class OpenAIService : public AIServiceInterface { /* ... */ };
class ClaudeService : public AIServiceInterface { /* ... */ };
```

---

This comprehensive API documentation provides everything needed to integrate AI services into your Unreal Engine projects. For detailed implementation guides, refer to the service-specific documentation pages.