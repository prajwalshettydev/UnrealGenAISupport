# OpenAI Integration

Complete guide for integrating OpenAI's GPT models into your Unreal Engine projects using the UnrealGenAISupport plugin. OpenAI integration provides the most comprehensive feature set including structured outputs and multiple model options.

## Overview

The OpenAI integration offers:
- **Multiple GPT Models**: From GPT-3.5 Turbo to latest GPT-4o and O3 models
- **Structured Outputs**: JSON schema-based responses for game data
- **Custom Model Support**: Use any OpenAI-compatible model
- **Dual Interface**: C++ and Blueprint integration
- **Advanced Configuration**: Fine-grained control over requests

## Supported Models

### Available Model Enum

```cpp
enum class EGenOAIChatModel : uint8
{
    GPT_35_Turbo     UMETA(DisplayName = "GPT-3.5 Turbo"),      // "gpt-3.5-turbo"
    GPT_4            UMETA(DisplayName = "GPT-4"),               // "gpt-4"
    GPT_4_Turbo      UMETA(DisplayName = "GPT-4 Turbo"),        // "gpt-4-turbo"
    GPT_4_1          UMETA(DisplayName = "GPT-4.1"),            // "gpt-4.1"
    GPT_4_1_Mini     UMETA(DisplayName = "GPT-4.1 Mini"),       // "gpt-4.1-mini"
    GPT_4_1_Nano     UMETA(DisplayName = "GPT-4.1 Nano"),       // "gpt-4.1-nano"
    GPT_4O_Mini      UMETA(DisplayName = "GPT-4o Mini"),        // "gpt-4o-mini"
    GPT_4O           UMETA(DisplayName = "GPT-4o"),             // "gpt-4o"
    O3               UMETA(DisplayName = "O3"),                 // "o3"
    O3_Mini          UMETA(DisplayName = "O3 Mini"),            // "o3-mini"
    O4_Mini          UMETA(DisplayName = "O4 Mini"),            // "o4-mini"
    Custom           UMETA(DisplayName = "Custom Model")        // User-defined model
};
```

### Model Characteristics

| Model | Context Window | Cost | Best For |
|-------|----------------|------|----------|
| **GPT-4o** | 128k tokens | High | Complex reasoning, code generation |
| **GPT-4o Mini** | 128k tokens | Low | Cost-effective general tasks |
| **GPT-4.1** | 128k tokens | High | Latest capabilities, improved accuracy |
| **O3/O3 Mini** | Variable | Variable | Advanced reasoning tasks |
| **GPT-3.5 Turbo** | 16k tokens | Very Low | Simple chat, fast responses |

### Choosing the Right Model

```cpp
// For development and testing
Settings.ModelEnum = EGenOAIChatModel::GPT_4O_Mini;

// For production with complex AI needs
Settings.ModelEnum = EGenOAIChatModel::GPT_4O;

// For cost-sensitive applications
Settings.ModelEnum = EGenOAIChatModel::GPT_35_Turbo;

// For latest capabilities
Settings.ModelEnum = EGenOAIChatModel::GPT_4_1;
```

## API Configuration

### Authentication Setup

Set your OpenAI API key using environment variables:

```bash
# Windows
setx PS_OPENAIAPIKEY "sk-your-openai-api-key-here"

# macOS/Linux
export PS_OPENAIAPIKEY="sk-your-openai-api-key-here"
echo 'export PS_OPENAIAPIKEY="sk-your-openai-api-key-here"' >> ~/.bashrc
```

Restart Unreal Editor after setting environment variables.

### Runtime Configuration (Alternative)

For testing or packaged builds:

```cpp
// Set API key at runtime
UGenSecureKey::SetGenAIApiKeyRuntime(EGenAIOrgs::OpenAI, TEXT("sk-your-api-key"));

// Switch to runtime keys
UGenSecureKey::SetUseApiKeyFromEnvironmentVars(false);
```

## Chat Completion API

### Basic Configuration Structure

```cpp
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FGenChatSettings
{
    GENERATED_BODY()

    /** Model selection using enum for type safety */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI", 
              meta = (DisplayName = "AI Model"))
    EGenOAIChatModel ModelEnum = EGenOAIChatModel::GPT_4O;
    
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

    /** Temperature for response randomness (0.0 = deterministic, 1.0 = creative) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI",
              meta = (ClampMin = "0.0", ClampMax = "2.0",
                      DisplayName = "Temperature"))
    float Temperature = 1.0f;

    /** Alternative to temperature, nucleus sampling */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI",
              meta = (ClampMin = "0.0", ClampMax = "1.0",
                      DisplayName = "Top P"))
    float TopP = 1.0f;

    /** Number of response choices to generate */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI",
              meta = (ClampMin = "1", ClampMax = "10",
                      DisplayName = "Choice Count"))
    int32 N = 1;

    /** Stop sequences to end generation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI",
              meta = (DisplayName = "Stop Sequences"))
    TArray<FString> Stop;

    /** Penalty for token frequency */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI",
              meta = (ClampMin = "-2.0", ClampMax = "2.0",
                      DisplayName = "Frequency Penalty"))
    float FrequencyPenalty = 0.0f;

    /** Penalty for token presence */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI",
              meta = (ClampMin = "-2.0", ClampMax = "2.0",
                      DisplayName = "Presence Penalty"))
    float PresencePenalty = 0.0f;

    /** User identifier for tracking */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI",
              meta = (DisplayName = "User ID"))
    FString User;
};
```

### C++ Integration

#### Header Includes

```cpp
#include "Models/OpenAI/GenOAIChat.h"
#include "Data/OpenAI/GenOAIChatStructs.h"
#include "Secure/GenSecureKey.h"
```

#### Basic Usage Pattern

```cpp
void YourClass::SendOpenAIRequest()
{
    // Configure chat settings
    FGenChatSettings ChatSettings;
    ChatSettings.ModelEnum = EGenOAIChatModel::GPT_4O;
    ChatSettings.MaxTokens = 1500;
    ChatSettings.Temperature = 0.7f;

    // Build conversation
    ChatSettings.Messages.Add(FGenChatMessage{
        TEXT("system"), 
        TEXT("You are a helpful assistant for game development in Unreal Engine.")
    });
    
    ChatSettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("How do I implement a basic AI behavior tree?")
    });

    // Send request with lambda callback
    UGenOAIChat::SendChatRequest(ChatSettings, 
        FOnChatCompletionResponse::CreateLambda([this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
        {
            if (bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("OpenAI Response: %s"), *Response);
                ProcessAIResponse(Response);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("OpenAI Error: %s"), *ErrorMessage);
                HandleAIError(ErrorMessage);
            }
        })
    );
}

void YourClass::ProcessAIResponse(const FString& Response)
{
    // Process the AI response
    // Parse and use the response data in your game logic
}

void YourClass::HandleAIError(const FString& Error)
{
    // Handle errors appropriately
    // Show user feedback, retry logic, etc.
}
```

#### Advanced Configuration Example

```cpp
void YourClass::AdvancedOpenAIRequest()
{
    FGenChatSettings AdvancedSettings;
    
    // Use latest model
    AdvancedSettings.ModelEnum = EGenOAIChatModel::GPT_4_1;
    
    // High token limit for detailed responses
    AdvancedSettings.MaxTokens = 4000;
    
    // Lower temperature for more focused responses
    AdvancedSettings.Temperature = 0.3f;
    
    // Add stop sequences
    AdvancedSettings.Stop.Add(TEXT("\n---\n"));
    AdvancedSettings.Stop.Add(TEXT("END_RESPONSE"));
    
    // Reduce repetition
    AdvancedSettings.FrequencyPenalty = 0.5f;
    AdvancedSettings.PresencePenalty = 0.3f;
    
    // User tracking
    AdvancedSettings.User = TEXT("game_player_123");

    // Complex conversation with context
    AdvancedSettings.Messages.Add(FGenChatMessage{
        TEXT("system"), 
        TEXT("You are an expert Unreal Engine developer specializing in Blueprint and C++ integration. "
             "Provide detailed, actionable advice with code examples when appropriate.")
    });
    
    AdvancedSettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("I need to create a dynamic quest system that can generate procedural objectives "
             "based on player progress and world state. What's the best architecture?")
    });

    UGenOAIChat::SendChatRequest(AdvancedSettings, OnAdvancedResponse);
}
```

### Blueprint Integration

#### Blueprint Latent Node

In Blueprints, use the "Request OpenAI Chat" latent node:

```cpp
UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), 
          Category = "GenAI|OpenAI")
static UGenOAIChat* RequestOpenAIChat(UObject* WorldContextObject, const FGenChatSettings& ChatSettings);
```

#### Blueprint Setup Example

1. **Add "Request OpenAI Chat" node** to your Blueprint
2. **Configure Chat Settings**:
   - Set Model Enum to desired GPT model
   - Set Max Tokens (e.g., 1500)
   - Build Messages array with system and user messages
3. **Connect execution pins**:
   - Input execution → Request OpenAI Chat
   - On Success → Process response logic
   - On Failure → Error handling logic

#### Blueprint Message Construction

```cpp
// Example Blueprint message array construction
TArray<FGenChatMessage> Messages;

// System message
FGenChatMessage SystemMessage;
SystemMessage.Role = TEXT("system");
SystemMessage.Content = TEXT("You are a helpful game assistant.");
Messages.Add(SystemMessage);

// User message
FGenChatMessage UserMessage;
UserMessage.Role = TEXT("user");
UserMessage.Content = TEXT("Generate a random NPC name and backstory.");
Messages.Add(UserMessage);

// Set in Chat Settings
ChatSettings.Messages = Messages;
```

## Structured Outputs

OpenAI's structured outputs feature allows you to receive responses in specific JSON formats, perfect for game data generation.

### Structured Output Configuration

```cpp
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FGenOAIStructuredChatSettings
{
    GENERATED_BODY()

    /** Base chat settings */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GenAI")
    FGenChatSettings ChatSettings;
    
    /** Whether to use schema-based structured output */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GenAI")
    bool bUseSchema = true;
    
    /** Schema name for structured output */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GenAI")
    FString Name;
    
    /** JSON schema definition for structured output */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GenAI", 
              meta = (MultiLine = "true"))
    FString SchemaJson;
};
```

### JSON Schema Examples

#### Character Generation Schema

```cpp
void YourClass::GenerateCharacterData()
{
    FGenOAIStructuredChatSettings StructuredSettings;
    
    // Base chat configuration
    StructuredSettings.ChatSettings.ModelEnum = EGenOAIChatModel::GPT_4O;
    StructuredSettings.ChatSettings.MaxTokens = 1000;
    StructuredSettings.ChatSettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("Generate a fantasy RPG character with stats and backstory.")
    });
    
    // Schema configuration
    StructuredSettings.bUseSchema = true;
    StructuredSettings.Name = TEXT("Character");
    
    // Define JSON schema
    StructuredSettings.SchemaJson = TEXT(R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "Character's full name"
            },
            "class": {
                "type": "string",
                "enum": ["Warrior", "Mage", "Rogue", "Paladin", "Ranger"],
                "description": "Character class"
            },
            "level": {
                "type": "integer",
                "minimum": 1,
                "maximum": 100,
                "description": "Character level"
            },
            "stats": {
                "type": "object",
                "properties": {
                    "strength": {"type": "integer", "minimum": 1, "maximum": 20},
                    "dexterity": {"type": "integer", "minimum": 1, "maximum": 20},
                    "intelligence": {"type": "integer", "minimum": 1, "maximum": 20},
                    "constitution": {"type": "integer", "minimum": 1, "maximum": 20}
                },
                "required": ["strength", "dexterity", "intelligence", "constitution"],
                "additionalProperties": false
            },
            "backstory": {
                "type": "string",
                "description": "Character's background story"
            },
            "equipment": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {"type": "string"},
                        "type": {"type": "string", "enum": ["weapon", "armor", "accessory"]},
                        "rarity": {"type": "string", "enum": ["common", "rare", "epic", "legendary"]}
                    },
                    "required": ["name", "type", "rarity"]
                }
            }
        },
        "required": ["name", "class", "level", "stats", "backstory", "equipment"],
        "additionalProperties": false
    })");

    // Send structured request
    UGenOAIStructuredOpService::RequestStructuredOutput(StructuredSettings, 
        FOnSchemaResponse::CreateLambda([this](const FString& Response, const FString& Error, bool Success)
        {
            if (Success)
            {
                ParseCharacterData(Response);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Structured output error: %s"), *Error);
            }
        })
    );
}

void YourClass::ParseCharacterData(const FString& JsonResponse)
{
    // Parse the structured JSON response
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonResponse);
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        // Extract character data
        FString CharacterName = JsonObject->GetStringField(TEXT("name"));
        FString CharacterClass = JsonObject->GetStringField(TEXT("class"));
        int32 Level = JsonObject->GetIntegerField(TEXT("level"));
        
        // Extract stats
        TSharedPtr<FJsonObject> Stats = JsonObject->GetObjectField(TEXT("stats"));
        int32 Strength = Stats->GetIntegerField(TEXT("strength"));
        int32 Dexterity = Stats->GetIntegerField(TEXT("dexterity"));
        int32 Intelligence = Stats->GetIntegerField(TEXT("intelligence"));
        int32 Constitution = Stats->GetIntegerField(TEXT("constitution"));
        
        // Create character in game
        CreateGameCharacter(CharacterName, CharacterClass, Level, Strength, Dexterity, Intelligence, Constitution);
    }
}
```

#### Quest Generation Schema

```cpp
FString QuestSchema = TEXT(R"({
    "type": "object",
    "properties": {
        "title": {
            "type": "string",
            "description": "Quest title"
        },
        "description": {
            "type": "string",
            "description": "Detailed quest description"
        },
        "type": {
            "type": "string",
            "enum": ["main", "side", "daily", "random"],
            "description": "Quest type"
        },
        "difficulty": {
            "type": "string",
            "enum": ["easy", "normal", "hard", "expert"],
            "description": "Quest difficulty"
        },
        "objectives": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "description": {"type": "string"},
                    "target": {"type": "string"},
                    "quantity": {"type": "integer", "minimum": 1}
                },
                "required": ["description", "target", "quantity"]
            },
            "minItems": 1,
            "maxItems": 5
        },
        "rewards": {
            "type": "object",
            "properties": {
                "experience": {"type": "integer", "minimum": 0},
                "gold": {"type": "integer", "minimum": 0},
                "items": {
                    "type": "array",
                    "items": {"type": "string"}
                }
            },
            "required": ["experience", "gold"]
        },
        "estimated_duration": {
            "type": "integer",
            "description": "Estimated completion time in minutes",
            "minimum": 5,
            "maximum": 180
        }
    },
    "required": ["title", "description", "type", "difficulty", "objectives", "rewards", "estimated_duration"],
    "additionalProperties": false
})");
```

### Loading Schema from File

```cpp
void YourClass::LoadSchemaFromFile()
{
    FString SchemaFilePath = FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("Data/Schemas/CharacterSchema.json")
    );
    
    FString SchemaJson;
    if (FFileHelper::LoadFileToString(SchemaJson, *SchemaFilePath))
    {
        // Use loaded schema
        StructuredSettings.SchemaJson = SchemaJson;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load schema from: %s"), *SchemaFilePath);
    }
}
```

## Error Handling and Troubleshooting

### Common Error Types

#### Authentication Errors
```cpp
// Error: "Invalid API key"
// Solution: Check environment variable and API key format
if (Error.Contains(TEXT("Invalid API key")))
{
    UE_LOG(LogTemp, Error, TEXT("Check PS_OPENAIAPIKEY environment variable"));
}
```

#### Rate Limiting
```cpp
// Error: "Rate limit exceeded"
// Solution: Implement retry logic with exponential backoff
if (Error.Contains(TEXT("rate limit")))
{
    // Implement retry after delay
    FTimerHandle RetryTimer;
    GetWorld()->GetTimerManager().SetTimer(RetryTimer, [this]()
    {
        RetryLastRequest();
    }, 60.0f, false);  // Retry after 60 seconds
}
```

#### Token Limits
```cpp
// Error: "Token limit exceeded"
// Solution: Reduce MaxTokens or truncate messages
if (Error.Contains(TEXT("maximum context length")))
{
    // Reduce token count and retry
    ChatSettings.MaxTokens = FMath::Min(ChatSettings.MaxTokens, 2000);
    TruncateMessages(ChatSettings.Messages);
    RetryRequest(ChatSettings);
}
```

### Debugging Tools

#### Verbose Logging
```cpp
// Enable detailed logging in DefaultEngine.ini
[Core.Log]
LogGenAI=Verbose
LogGenAIVerbose=All
LogGenPerformance=All
```

#### Request Inspection
```cpp
void YourClass::DebugRequest(const FGenChatSettings& Settings)
{
    UE_LOG(LogGenAIVerbose, Log, TEXT("OpenAI Request Details:"));
    UE_LOG(LogGenAIVerbose, Log, TEXT("Model: %s"), *UGenOAIModelUtils::ChatModelToString(Settings.ModelEnum));
    UE_LOG(LogGenAIVerbose, Log, TEXT("MaxTokens: %d"), Settings.MaxTokens);
    UE_LOG(LogGenAIVerbose, Log, TEXT("Temperature: %f"), Settings.Temperature);
    UE_LOG(LogGenAIVerbose, Log, TEXT("Message Count: %d"), Settings.Messages.Num());
    
    for (int32 i = 0; i < Settings.Messages.Num(); i++)
    {
        UE_LOG(LogGenAIVerbose, Log, TEXT("Message %d: [%s] %s"), 
               i, *Settings.Messages[i].Role, *Settings.Messages[i].Content);
    }
}
```

## Best Practices

### Performance Optimization

#### Token Management
```cpp
// Calculate approximate token count before sending
int32 EstimateTokenCount(const TArray<FGenChatMessage>& Messages)
{
    int32 TotalTokens = 0;
    for (const auto& Message : Messages)
    {
        // Rough estimation: 1 token ≈ 4 characters
        TotalTokens += (Message.Role.Len() + Message.Content.Len()) / 4;
    }
    return TotalTokens;
}

// Optimize token usage
void OptimizeTokenUsage(FGenChatSettings& Settings)
{
    int32 EstimatedTokens = EstimateTokenCount(Settings.Messages);
    
    // Leave room for response
    int32 MaxInputTokens = 120000 - Settings.MaxTokens;  // GPT-4o context limit
    
    if (EstimatedTokens > MaxInputTokens)
    {
        // Truncate older messages while keeping system message
        TArray<FGenChatMessage> OptimizedMessages;
        
        // Always keep system message
        if (Settings.Messages.Num() > 0 && Settings.Messages[0].Role == TEXT("system"))
        {
            OptimizedMessages.Add(Settings.Messages[0]);
        }
        
        // Add recent messages until token limit
        for (int32 i = Settings.Messages.Num() - 1; i >= 1; i--)
        {
            OptimizedMessages.Insert(Settings.Messages[i], 1);
            
            if (EstimateTokenCount(OptimizedMessages) > MaxInputTokens)
            {
                OptimizedMessages.RemoveAt(1);  // Remove the message that exceeded limit
                break;
            }
        }
        
        Settings.Messages = OptimizedMessages;
    }
}
```

#### Async Request Management
```cpp
class YOURGAME_API UAIRequestManager : public UObject
{
    GENERATED_BODY()
    
private:
    TQueue<TFunction<void()>> RequestQueue;
    bool bIsProcessing = false;
    float RequestCooldown = 1.0f;  // Minimum time between requests
    
public:
    void QueueRequest(TFunction<void()> RequestFunction)
    {
        RequestQueue.Enqueue(RequestFunction);
        ProcessNextRequest();
    }
    
private:
    void ProcessNextRequest()
    {
        if (bIsProcessing || RequestQueue.IsEmpty())
            return;
            
        bIsProcessing = true;
        
        TFunction<void()> NextRequest;
        if (RequestQueue.Dequeue(NextRequest))
        {
            NextRequest();
            
            // Set cooldown before processing next request
            FTimerHandle CooldownTimer;
            GetWorld()->GetTimerManager().SetTimer(CooldownTimer, [this]()
            {
                bIsProcessing = false;
                ProcessNextRequest();
            }, RequestCooldown, false);
        }
    }
};
```

### Cost Management

#### Model Selection Strategy
```cpp
EGenOAIChatModel SelectOptimalModel(const FString& TaskType, int32 ComplexityLevel)
{
    if (TaskType == TEXT("simple_chat") || ComplexityLevel <= 3)
    {
        return EGenOAIChatModel::GPT_4O_Mini;  // Cost-effective for simple tasks
    }
    else if (TaskType == TEXT("code_generation") || ComplexityLevel >= 8)
    {
        return EGenOAIChatModel::GPT_4O;  // High capability for complex tasks
    }
    else if (TaskType == TEXT("reasoning") || ComplexityLevel >= 9)
    {
        return EGenOAIChatModel::O3_Mini;  // Advanced reasoning
    }
    else
    {
        return EGenOAIChatModel::GPT_4O_Mini;  // Default to cost-effective option
    }
}
```

#### Usage Tracking
```cpp
class YOURGAME_API UOpenAIUsageTracker : public UObject
{
    GENERATED_BODY()
    
private:
    int32 TotalTokensUsed = 0;
    int32 TotalRequests = 0;
    float EstimatedCost = 0.0f;
    
public:
    void TrackUsage(EGenOAIChatModel Model, int32 InputTokens, int32 OutputTokens)
    {
        TotalTokensUsed += InputTokens + OutputTokens;
        TotalRequests++;
        
        // Estimate cost based on model pricing
        float InputCost = GetInputTokenCost(Model) * InputTokens / 1000.0f;
        float OutputCost = GetOutputTokenCost(Model) * OutputTokens / 1000.0f;
        EstimatedCost += InputCost + OutputCost;
        
        UE_LOG(LogGenPerformance, Log, TEXT("OpenAI Usage - Tokens: %d, Requests: %d, Est. Cost: $%.4f"), 
               TotalTokensUsed, TotalRequests, EstimatedCost);
    }
    
private:
    float GetInputTokenCost(EGenOAIChatModel Model)
    {
        // Cost per 1K tokens (as of 2024)
        switch (Model)
        {
            case EGenOAIChatModel::GPT_4O: return 0.005f;
            case EGenOAIChatModel::GPT_4O_Mini: return 0.00015f;
            case EGenOAIChatModel::GPT_4_1: return 0.005f;
            default: return 0.001f;
        }
    }
    
    float GetOutputTokenCost(EGenOAIChatModel Model)
    {
        switch (Model)
        {
            case EGenOAIChatModel::GPT_4O: return 0.015f;
            case EGenOAIChatModel::GPT_4O_Mini: return 0.0006f;
            case EGenOAIChatModel::GPT_4_1: return 0.015f;
            default: return 0.002f;
        }
    }
};
```

### Security Considerations

#### API Key Protection
```cpp
// Never hardcode API keys
// ❌ BAD
FString ApiKey = TEXT("sk-hardcoded-key-here");

// ✅ GOOD - Use environment variables
FString ApiKey = UGenSecureKey::GetGenerativeAIApiKey(EGenAIOrgs::OpenAI);

// ✅ GOOD - Validate key before use
if (ApiKey.IsEmpty() || !ApiKey.StartsWith(TEXT("sk-")))
{
    UE_LOG(LogTemp, Error, TEXT("Invalid OpenAI API key configuration"));
    return;
}
```

#### Content Validation
```cpp
bool ValidateUserInput(const FString& UserContent)
{
    // Check for sensitive information
    if (UserContent.Contains(TEXT("password")) || 
        UserContent.Contains(TEXT("api_key")) ||
        UserContent.Contains(TEXT("secret")))
    {
        UE_LOG(LogTemp, Warning, TEXT("Potential sensitive information detected in user input"));
        return false;
    }
    
    // Check content length
    if (UserContent.Len() > 10000)
    {
        UE_LOG(LogTemp, Warning, TEXT("User input too long, potential abuse"));
        return false;
    }
    
    return true;
}
```

---

This comprehensive OpenAI integration guide provides everything needed to leverage OpenAI's powerful models in your Unreal Engine projects. For additional examples and advanced patterns, refer to the Blueprint examples in `Content/ExampleBlueprints/ChatAPIExamples.uasset`.