# Anthropic Claude Integration

Complete guide for integrating Anthropic's Claude models into your Unreal Engine projects using the UnrealGenAISupport plugin. Claude integration provides advanced reasoning capabilities, constitutional AI training, and fine-grained control over response generation.

## Overview

The Anthropic Claude integration offers:
- **Latest Claude 4 Models**: Claude Opus 4 and Sonnet 4 with enhanced capabilities
- **Claude 3.5 Family**: Proven models with excellent performance
- **Constitutional AI**: Built-in safety and helpfulness training
- **Temperature Control**: Fine-tuned creativity and determinism
- **Streaming Support**: Real-time response generation
- **Large Context Windows**: Up to 200K tokens for complex conversations

## Supported Models

### Available Model Enum

```cpp
enum class EClaudeModels : uint8
{
    Claude_4_Opus        UMETA(DisplayName = "Claude 4 Opus"),        // "claude-opus-4-20250514"
    Claude_4_Sonnet      UMETA(DisplayName = "Claude 4 Sonnet"),      // "claude-sonnet-4-20250514"
    Claude_3_5_Sonnet    UMETA(DisplayName = "Claude 3.5 Sonnet"),    // "claude-3-5-sonnet"
    Claude_3_7_Sonnet    UMETA(DisplayName = "Claude 3.7 Sonnet"),    // "claude-3-7-sonnet-latest"
    Claude_3_5_Haiku     UMETA(DisplayName = "Claude 3.5 Haiku"),     // "claude-3-5-haiku-latest"
    Claude_3_Opus        UMETA(DisplayName = "Claude 3 Opus")         // "claude-3-opus-latest"
};
```

### Model Characteristics

| Model | Context Window | Performance | Best For |
|-------|----------------|-------------|----------|
| **Claude 4 Opus** | 200k tokens | Highest | Complex reasoning, code analysis, creative writing |
| **Claude 4 Sonnet** | 200k tokens | High | Balanced performance and speed |
| **Claude 3.5 Sonnet** | 200k tokens | High | Production workloads, coding assistance |
| **Claude 3.7 Sonnet** | 200k tokens | High | Latest 3.x series improvements |
| **Claude 3.5 Haiku** | 200k tokens | Fast | Quick responses, simple tasks |
| **Claude 3 Opus** | 200k tokens | High | Legacy high-performance model |

### Choosing the Right Model

```cpp
// For maximum reasoning capability
Settings.Model = EClaudeModels::Claude_4_Opus;

// For balanced performance (recommended)
Settings.Model = EClaudeModels::Claude_4_Sonnet;

// For fast responses
Settings.Model = EClaudeModels::Claude_3_5_Haiku;

// For coding assistance
Settings.Model = EClaudeModels::Claude_3_5_Sonnet;
```

## API Configuration

### Authentication Setup

Set your Anthropic API key using environment variables:

```bash
# Windows
setx PS_ANTHROPICAPIKEY "sk-ant-your-anthropic-api-key-here"

# macOS/Linux
export PS_ANTHROPICAPIKEY="sk-ant-your-anthropic-api-key-here"
echo 'export PS_ANTHROPICAPIKEY="sk-ant-your-anthropic-api-key-here"' >> ~/.bashrc
```

Restart Unreal Editor after setting environment variables.

### Runtime Configuration (Alternative)

For testing or packaged builds:

```cpp
// Set API key at runtime
UGenSecureKey::SetGenAIApiKeyRuntime(EGenAIOrgs::Anthropic, TEXT("sk-ant-your-api-key"));

// Switch to runtime keys
UGenSecureKey::SetUseApiKeyFromEnvironmentVars(false);
```

## Chat Completion API

### Configuration Structure

```cpp
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FGenClaudeChatSettings
{
    GENERATED_BODY()

    /** Claude model selection */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API",
              meta = (DisplayName = "Claude Model"))
    EClaudeModels Model = EClaudeModels::Claude_4_Sonnet;

    /** Maximum number of tokens to generate */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API",
              meta = (ClampMin = "1", ClampMax = "200000",
                      DisplayName = "Max Tokens",
                      ToolTip = "Maximum number of tokens to generate in the response"))
    int32 MaxTokens = 1024;

    /** Temperature for response creativity (0.0 = deterministic, 1.0 = creative) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API",
              meta = (ClampMin = "0.0", ClampMax = "1.0",
                      DisplayName = "Temperature",
                      ToolTip = "Controls randomness in responses. Lower = more focused, Higher = more creative"))
    float Temperature = 0.7f;

    /** Array of messages for the conversation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API",
              meta = (DisplayName = "Messages",
                      ToolTip = "Conversation messages with role and content"))
    TArray<FGenChatMessage> Messages;

    /** Enable streaming responses (not yet implemented) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API",
              meta = (DisplayName = "Stream Response"))
    bool bStreamResponse = false;

    /** System message for Claude (set separately from Messages array) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API",
              meta = (DisplayName = "System Message",
                      ToolTip = "System instructions that guide Claude's behavior"))
    FString SystemMessage;

    /** Stop sequences to end generation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API",
              meta = (DisplayName = "Stop Sequences"))
    TArray<FString> StopSequences;

    /** Tool use configuration (for function calling) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API",
              meta = (DisplayName = "Enable Tools"))
    bool bEnableTools = false;

    /** Top-k nucleus sampling parameter */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API",
              meta = (ClampMin = "0", ClampMax = "1000",
                      DisplayName = "Top K"))
    int32 TopK = 0;

    /** Top-p nucleus sampling parameter */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API",
              meta = (ClampMin = "0.0", ClampMax = "1.0",
                      DisplayName = "Top P"))
    float TopP = 1.0f;
};
```

## C++ Integration

### Header Includes

```cpp
#include "Models/Anthropic/GenClaudeChat.h"
#include "Data/Anthropic/GenClaudeChatStructs.h"
#include "Secure/GenSecureKey.h"
```

### Basic Usage Pattern

```cpp
void YourClass::SendClaudeRequest()
{
    // Configure Claude chat settings
    FGenClaudeChatSettings ChatSettings;
    ChatSettings.Model = EClaudeModels::Claude_4_Sonnet;
    ChatSettings.MaxTokens = 2048;
    ChatSettings.Temperature = 0.7f;

    // Set system message (Claude-specific approach)
    ChatSettings.SystemMessage = TEXT("You are an expert Unreal Engine developer with deep knowledge of Blueprint and C++ integration. Provide clear, actionable advice with practical examples.");

    // Build conversation (no system message in Messages array for Claude)
    ChatSettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("How can I optimize my game's AI system for better performance in large open worlds?")
    });

    // Send request with lambda callback
    UGenClaudeChat::SendChatRequest(ChatSettings, 
        FOnClaudeChatCompletionResponse::CreateLambda([this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
        {
            if (bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("Claude Response: %s"), *Response);
                ProcessClaudeResponse(Response);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Claude Error: %s"), *ErrorMessage);
                HandleClaudeError(ErrorMessage);
            }
        })
    );
}

void YourClass::ProcessClaudeResponse(const FString& Response)
{
    // Process Claude's response
    // Claude responses tend to be well-structured and detailed
    
    // Example: Extract code blocks if present
    TArray<FString> CodeBlocks;
    ExtractCodeBlocks(Response, CodeBlocks);
    
    // Example: Parse structured advice
    ParseAdviceStructure(Response);
}

void YourClass::HandleClaudeError(const FString& Error)
{
    // Handle Claude-specific errors
    if (Error.Contains(TEXT("rate_limit")))
    {
        // Claude has different rate limits per model
        HandleRateLimit();
    }
    else if (Error.Contains(TEXT("invalid_request")))
    {
        // Check message format and token limits
        HandleInvalidRequest();
    }
}
```

### Advanced Configuration Example

```cpp
void YourClass::AdvancedClaudeRequest()
{
    FGenClaudeChatSettings AdvancedSettings;
    
    // Use latest Claude 4 model
    AdvancedSettings.Model = EClaudeModels::Claude_4_Opus;
    
    // High token limit for detailed analysis
    AdvancedSettings.MaxTokens = 4000;
    
    // Lower temperature for focused, analytical responses
    AdvancedSettings.Temperature = 0.3f;
    
    // Advanced system prompt
    AdvancedSettings.SystemMessage = TEXT(
        "You are a senior technical architect specializing in real-time game systems. "
        "When providing solutions:\n"
        "1. Always consider performance implications\n"
        "2. Provide both Blueprint and C++ approaches when relevant\n"
        "3. Include potential pitfalls and how to avoid them\n"
        "4. Structure responses with clear headings and examples\n"
        "5. Consider scalability and maintainability"
    );
    
    // Add stop sequences for structured responses
    AdvancedSettings.StopSequences.Add(TEXT("\n---END---\n"));
    
    // Configure nucleus sampling for focused responses
    AdvancedSettings.TopP = 0.9f;
    AdvancedSettings.TopK = 100;

    // Complex conversation with context
    AdvancedSettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("I'm building a multiplayer survival game with 100+ players. "
             "I need to design a scalable inventory system that can handle:\n"
             "- Real-time item updates\n"
             "- Complex crafting recipes\n"
             "- Trade between players\n"
             "- Persistence across sessions\n"
             "What architecture would you recommend?")
    });

    UGenClaudeChat::SendChatRequest(AdvancedSettings, OnAdvancedClaudeResponse);
}
```

### Conversation Management

```cpp
class YOURGAME_API UClaudeConversationManager : public UObject
{
    GENERATED_BODY()

private:
    UPROPERTY()
    TArray<FGenChatMessage> ConversationHistory;
    
    UPROPERTY()
    FString SystemPrompt;
    
    int32 MaxHistoryTokens = 150000;  // Leave room for response

public:
    void InitializeConversation(const FString& InSystemPrompt)
    {
        SystemPrompt = InSystemPrompt;
        ConversationHistory.Empty();
    }
    
    void AddUserMessage(const FString& Message)
    {
        ConversationHistory.Add(FGenChatMessage{TEXT("user"), Message});
        TrimHistoryIfNeeded();
    }
    
    void AddAssistantMessage(const FString& Message)
    {
        ConversationHistory.Add(FGenChatMessage{TEXT("assistant"), Message});
        TrimHistoryIfNeeded();
    }
    
    void SendMessage(const FString& UserMessage, EClaudeModels Model = EClaudeModels::Claude_4_Sonnet)
    {
        AddUserMessage(UserMessage);
        
        FGenClaudeChatSettings Settings;
        Settings.Model = Model;
        Settings.MaxTokens = 2048;
        Settings.Temperature = 0.7f;
        Settings.SystemMessage = SystemPrompt;
        Settings.Messages = ConversationHistory;
        
        UGenClaudeChat::SendChatRequest(Settings, 
            FOnClaudeChatCompletionResponse::CreateUObject(this, &UClaudeConversationManager::OnMessageReceived)
        );
    }
    
private:
    UFUNCTION()
    void OnMessageReceived(const FString& Response, const FString& Error, bool bSuccess)
    {
        if (bSuccess)
        {
            AddAssistantMessage(Response);
            OnConversationUpdate.Broadcast(Response, true);
        }
        else
        {
            OnConversationUpdate.Broadcast(Error, false);
        }
    }
    
    void TrimHistoryIfNeeded()
    {
        // Estimate token count and trim if necessary
        int32 EstimatedTokens = EstimateTokenCount(ConversationHistory);
        
        while (EstimatedTokens > MaxHistoryTokens && ConversationHistory.Num() > 2)
        {
            // Remove oldest user-assistant pair
            ConversationHistory.RemoveAt(0);
            if (ConversationHistory.Num() > 0)
            {
                ConversationHistory.RemoveAt(0);
            }
            EstimatedTokens = EstimateTokenCount(ConversationHistory);
        }
    }
    
    int32 EstimateTokenCount(const TArray<FGenChatMessage>& Messages)
    {
        int32 TotalTokens = SystemPrompt.Len() / 4;  // Rough estimation
        for (const auto& Message : Messages)
        {
            TotalTokens += (Message.Role.Len() + Message.Content.Len()) / 4;
        }
        return TotalTokens;
    }

public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConversationUpdate, const FString&, Message, bool, bSuccess);
    
    UPROPERTY(BlueprintAssignable)
    FOnConversationUpdate OnConversationUpdate;
};
```

## Blueprint Integration

### Blueprint Latent Node

In Blueprints, use the "Request Claude Chat" latent node:

```cpp
UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), 
          Category = "GenAI|Claude")
static UGenClaudeChat* RequestClaudeChat(UObject* WorldContextObject, const FGenClaudeChatSettings& ChatSettings);
```

### Blueprint Setup Example

1. **Add "Request Claude Chat" node** to your Blueprint
2. **Configure Chat Settings**:
   - Set Model to desired Claude model
   - Set Max Tokens (e.g., 2048)
   - Set Temperature (0.0-1.0)
   - Set System Message
   - Build Messages array with user messages only
3. **Connect execution pins**:
   - Input execution → Request Claude Chat
   - On Success → Process response logic
   - On Failure → Error handling logic

### Blueprint Message Construction

```cpp
// Example Blueprint message array construction for Claude
TArray<FGenChatMessage> Messages;

// Note: System message is separate in Claude, not in Messages array
// Only add user and assistant messages to Messages array

// User message
FGenChatMessage UserMessage;
UserMessage.Role = TEXT("user");
UserMessage.Content = TEXT("Generate a unique weapon design for my fantasy game.");
Messages.Add(UserMessage);

// If continuing conversation, add assistant's previous response
FGenChatMessage AssistantMessage;
AssistantMessage.Role = TEXT("assistant");
AssistantMessage.Content = TEXT("Previous AI response...");
Messages.Add(AssistantMessage);

// New user message
FGenChatMessage NewUserMessage;
NewUserMessage.Role = TEXT("user");
NewUserMessage.Content = TEXT("Make it more magical and add special abilities.");
Messages.Add(NewUserMessage);

// Set in Chat Settings
ChatSettings.Messages = Messages;
ChatSettings.SystemMessage = TEXT("You are a creative game designer specializing in fantasy weapons.");
```

## Claude-Specific Features

### Constitutional AI and Safety

Claude is trained with Constitutional AI, making it inherently safer and more helpful:

```cpp
void YourClass::SafeContentGeneration()
{
    FGenClaudeChatSettings SafeSettings;
    SafeSettings.Model = EClaudeModels::Claude_4_Sonnet;
    SafeSettings.MaxTokens = 1500;
    SafeSettings.Temperature = 0.8f;
    
    // Claude's constitutional training helps with safe content generation
    SafeSettings.SystemMessage = TEXT(
        "You are helping design content for a T-rated game. "
        "All content should be appropriate for teenagers and avoid:\n"
        "- Excessive violence or gore\n"
        "- Inappropriate sexual content\n"
        "- Hate speech or discrimination\n"
        "- Real-world harmful activities\n"
        "Focus on creative, engaging, and positive content."
    );
    
    SafeSettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("Generate quest ideas for a medieval adventure game with moral choices.")
    });
    
    UGenClaudeChat::SendChatRequest(SafeSettings, OnSafeContentReceived);
}
```

### System Message Best Practices

Claude treats system messages specially and follows them consistently:

```cpp
FString CreateEffectiveSystemMessage(const FString& Domain, const FString& Personality, const TArray<FString>& Guidelines)
{
    FString SystemMessage = FString::Printf(TEXT("You are %s with expertise in %s.\n\n"), *Personality, *Domain);
    
    if (Guidelines.Num() > 0)
    {
        SystemMessage += TEXT("Guidelines for your responses:\n");
        for (int32 i = 0; i < Guidelines.Num(); i++)
        {
            SystemMessage += FString::Printf(TEXT("%d. %s\n"), i + 1, *Guidelines[i]);
        }
        SystemMessage += TEXT("\n");
    }
    
    SystemMessage += TEXT("Always provide helpful, accurate, and practical advice.");
    
    return SystemMessage;
}

// Usage example
TArray<FString> GameDevGuidelines = {
    TEXT("Provide both Blueprint and C++ solutions when applicable"),
    TEXT("Consider performance implications for real-time games"),
    TEXT("Include code examples with clear comments"),
    TEXT("Mention potential pitfalls and how to avoid them"),
    TEXT("Structure responses with clear headings")
};

FString SystemPrompt = CreateEffectiveSystemMessage(
    TEXT("Unreal Engine development"),
    TEXT("a senior game developer and technical architect"),
    GameDevGuidelines
);
```

### Response Quality Optimization

Claude excels at producing well-structured, detailed responses:

```cpp
void YourClass::OptimizeForQuality()
{
    FGenClaudeChatSettings QualitySettings;
    QualitySettings.Model = EClaudeModels::Claude_4_Opus;  // Highest quality model
    QualitySettings.MaxTokens = 3000;  // Allow detailed responses
    QualitySettings.Temperature = 0.4f;  // Focused but not rigid
    
    // System message that encourages structured responses
    QualitySettings.SystemMessage = TEXT(
        "You are an expert technical writer and game developer. "
        "Structure your responses with:\n"
        "1. Brief summary\n"
        "2. Detailed explanation\n"
        "3. Code examples\n"
        "4. Best practices\n"
        "5. Common pitfalls\n"
        "Use clear headings and formatting for readability."
    );
    
    QualitySettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("Explain how to implement a robust save/load system for an open-world RPG in Unreal Engine.")
    });
    
    UGenClaudeChat::SendChatRequest(QualitySettings, OnQualityResponseReceived);
}

void YourClass::OnQualityResponseReceived(const FString& Response, const FString& Error, bool bSuccess)
{
    if (bSuccess)
    {
        // Claude responses are typically well-structured
        // Parse sections based on headings
        TArray<FString> Sections;
        ParseResponseSections(Response, Sections);
        
        for (const FString& Section : Sections)
        {
            ProcessSection(Section);
        }
    }
}

void YourClass::ParseResponseSections(const FString& Response, TArray<FString>& OutSections)
{
    // Split by common heading patterns Claude uses
    TArray<FString> PotentialSections;
    
    // Split by numbered lists
    Response.ParseIntoArray(PotentialSections, TEXT("\n\n"), true);
    
    for (const FString& Section : PotentialSections)
    {
        if (!Section.IsEmpty() && Section.Len() > 10)
        {
            OutSections.Add(Section.TrimStartAndEnd());
        }
    }
}
```

## Error Handling and Troubleshooting

### Claude-Specific Error Types

#### Authentication Errors
```cpp
if (Error.Contains(TEXT("authentication")))
{
    UE_LOG(LogTemp, Error, TEXT("Check PS_ANTHROPICAPIKEY environment variable"));
    UE_LOG(LogTemp, Error, TEXT("Ensure key starts with 'sk-ant-'"));
}
```

#### Rate Limiting
```cpp
// Claude has model-specific rate limits
if (Error.Contains(TEXT("rate_limit")))
{
    // Implement exponential backoff
    float DelaySeconds = GetRateLimitDelay(ChatSettings.Model);
    
    FTimerHandle RetryTimer;
    GetWorld()->GetTimerManager().SetTimer(RetryTimer, [this]()
    {
        RetryClaudeRequest();
    }, DelaySeconds, false);
}

float YourClass::GetRateLimitDelay(EClaudeModels Model)
{
    switch (Model)
    {
        case EClaudeModels::Claude_4_Opus:
            return 60.0f;  // Longer delay for premium model
        case EClaudeModels::Claude_3_5_Haiku:
            return 30.0f;  // Shorter delay for faster model
        default:
            return 45.0f;  // Default delay
    }
}
```

#### Content Policy Violations
```cpp
if (Error.Contains(TEXT("content_policy")))
{
    UE_LOG(LogTemp, Warning, TEXT("Content policy violation detected"));
    
    // Claude's constitutional AI rarely triggers this, but handle gracefully
    OnContentPolicyViolation.Broadcast(TEXT("Please rephrase your request to comply with content guidelines."));
}
```

### Debugging Tools

#### Claude Response Analysis
```cpp
void YourClass::AnalyzeClaudeResponse(const FString& Response)
{
    // Claude responses often have clear structure
    UE_LOG(LogGenAIVerbose, Log, TEXT("Claude Response Analysis:"));
    UE_LOG(LogGenAIVerbose, Log, TEXT("Length: %d characters"), Response.Len());
    
    // Count code blocks
    int32 CodeBlockCount = 0;
    int32 SearchStart = 0;
    while (true)
    {
        int32 FoundIndex = Response.Find(TEXT("```"), ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchStart);
        if (FoundIndex == INDEX_NONE) break;
        CodeBlockCount++;
        SearchStart = FoundIndex + 3;
    }
    UE_LOG(LogGenAIVerbose, Log, TEXT("Code blocks found: %d"), CodeBlockCount / 2);
    
    // Analyze structure
    int32 HeadingCount = CountHeadings(Response);
    UE_LOG(LogGenAIVerbose, Log, TEXT("Headings found: %d"), HeadingCount);
    
    // Check for lists
    int32 ListItems = CountListItems(Response);
    UE_LOG(LogGenAIVerbose, Log, TEXT("List items found: %d"), ListItems);
}

int32 YourClass::CountHeadings(const FString& Text)
{
    int32 Count = 0;
    TArray<FString> Lines;
    Text.ParseIntoArrayLines(Lines);
    
    for (const FString& Line : Lines)
    {
        if (Line.StartsWith(TEXT("#")) || 
            Line.StartsWith(TEXT("## ")) ||
            Line.Contains(TEXT(":**")) ||
            (Line.Len() > 0 && Line.ToUpper() == Line && !Line.Contains(TEXT(" "))))
        {
            Count++;
        }
    }
    return Count;
}

int32 YourClass::CountListItems(const FString& Text)
{
    int32 Count = 0;
    TArray<FString> Lines;
    Text.ParseIntoArrayLines(Lines);
    
    for (const FString& Line : Lines)
    {
        FString TrimmedLine = Line.TrimStartAndEnd();
        if (TrimmedLine.StartsWith(TEXT("- ")) ||
            TrimmedLine.StartsWith(TEXT("* ")) ||
            TrimmedLine.MatchesWildcard(TEXT("[0-9]. *")) ||
            TrimmedLine.MatchesWildcard(TEXT("[0-9][0-9]. *")))
        {
            Count++;
        }
    }
    return Count;
}
```

## Best Practices

### Leveraging Claude's Strengths

#### Complex Reasoning Tasks
```cpp
void YourClass::ComplexReasoningTask()
{
    FGenClaudeChatSettings ReasoningSettings;
    ReasoningSettings.Model = EClaudeModels::Claude_4_Opus;  // Best reasoning model
    ReasoningSettings.MaxTokens = 4000;
    ReasoningSettings.Temperature = 0.2f;  // Low temperature for logical consistency
    
    ReasoningSettings.SystemMessage = TEXT(
        "You are analyzing complex game design problems. "
        "Break down your reasoning into clear steps:\n"
        "1. Problem analysis\n"
        "2. Constraints identification\n"
        "3. Solution exploration\n"
        "4. Implementation strategy\n"
        "5. Potential challenges\n"
        "Show your reasoning process clearly."
    );
    
    ReasoningSettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("I need to balance a complex economy system in my strategy game where players can:\n"
             "- Mine 5 different resources\n"
             "- Trade with AI and other players\n"
             "- Upgrade 20+ different buildings\n"
             "- Research 50+ technologies\n"
             "How can I ensure the economy remains balanced and engaging throughout a 4-hour game session?")
    });
    
    UGenClaudeChat::SendChatRequest(ReasoningSettings, OnReasoningResponseReceived);
}
```

#### Code Review and Analysis
```cpp
void YourClass::CodeReviewTask(const FString& CodeToReview)
{
    FGenClaudeChatSettings ReviewSettings;
    ReviewSettings.Model = EClaudeModels::Claude_3_5_Sonnet;  // Excellent for code analysis
    ReviewSettings.MaxTokens = 3000;
    ReviewSettings.Temperature = 0.3f;
    
    ReviewSettings.SystemMessage = TEXT(
        "You are a senior Unreal Engine developer performing code review. "
        "Analyze the code for:\n"
        "1. Performance implications\n"
        "2. Memory management\n"
        "3. Unreal Engine best practices\n"
        "4. Potential bugs or edge cases\n"
        "5. Code maintainability\n"
        "6. Blueprint integration considerations\n"
        "Provide specific, actionable feedback."
    );
    
    FString ReviewPrompt = FString::Printf(
        TEXT("Please review this Unreal Engine C++ code:\n\n```cpp\n%s\n```\n\n"
             "Focus on performance, correctness, and Unreal Engine best practices."),
        *CodeToReview
    );
    
    ReviewSettings.Messages.Add(FGenChatMessage{TEXT("user"), ReviewPrompt});
    
    UGenClaudeChat::SendChatRequest(ReviewSettings, OnCodeReviewReceived);
}
```

### Performance Optimization

#### Token Management for Large Contexts
```cpp
class YOURGAME_API UClaudeContextManager : public UObject
{
    GENERATED_BODY()

private:
    static const int32 CLAUDE_MAX_TOKENS = 200000;
    static const int32 RESPONSE_BUFFER = 4000;
    
public:
    bool OptimizeContextForClaude(FGenClaudeChatSettings& Settings)
    {
        int32 EstimatedTokens = EstimateTokens(Settings);
        int32 MaxAllowedTokens = CLAUDE_MAX_TOKENS - Settings.MaxTokens - RESPONSE_BUFFER;
        
        if (EstimatedTokens <= MaxAllowedTokens)
        {
            return true;  // No optimization needed
        }
        
        // Strategy 1: Summarize older messages
        if (Settings.Messages.Num() > 4)
        {
            SummarizeOlderMessages(Settings);
            EstimatedTokens = EstimateTokens(Settings);
        }
        
        // Strategy 2: Truncate system message if still too large
        if (EstimatedTokens > MaxAllowedTokens)
        {
            TruncateSystemMessage(Settings, MaxAllowedTokens - EstimateMessageTokens(Settings.Messages));
        }
        
        return EstimateTokens(Settings) <= MaxAllowedTokens;
    }
    
private:
    int32 EstimateTokens(const FGenClaudeChatSettings& Settings)
    {
        return EstimateSystemTokens(Settings.SystemMessage) + EstimateMessageTokens(Settings.Messages);
    }
    
    int32 EstimateSystemTokens(const FString& SystemMessage)
    {
        return SystemMessage.Len() / 4;  // Rough estimation
    }
    
    int32 EstimateMessageTokens(const TArray<FGenChatMessage>& Messages)
    {
        int32 Total = 0;
        for (const auto& Message : Messages)
        {
            Total += (Message.Role.Len() + Message.Content.Len()) / 4;
        }
        return Total;
    }
    
    void SummarizeOlderMessages(FGenClaudeChatSettings& Settings)
    {
        if (Settings.Messages.Num() < 4) return;
        
        // Keep first and last 2 messages, summarize the middle
        TArray<FGenChatMessage> NewMessages;
        
        // Keep first 2 messages
        NewMessages.Add(Settings.Messages[0]);
        NewMessages.Add(Settings.Messages[1]);
        
        // Create summary of middle messages
        FString MiddleSummary = TEXT("Previous conversation summary: ");
        for (int32 i = 2; i < Settings.Messages.Num() - 2; i++)
        {
            MiddleSummary += FString::Printf(TEXT("[%s: %s] "), 
                *Settings.Messages[i].Role, 
                *Settings.Messages[i].Content.Left(100));
        }
        
        NewMessages.Add(FGenChatMessage{TEXT("assistant"), MiddleSummary});
        
        // Keep last 2 messages
        NewMessages.Add(Settings.Messages[Settings.Messages.Num() - 2]);
        NewMessages.Add(Settings.Messages[Settings.Messages.Num() - 1]);
        
        Settings.Messages = NewMessages;
    }
    
    void TruncateSystemMessage(FGenClaudeChatSettings& Settings, int32 MaxTokens)
    {
        int32 MaxChars = MaxTokens * 4;  // Rough conversion
        if (Settings.SystemMessage.Len() > MaxChars)
        {
            Settings.SystemMessage = Settings.SystemMessage.Left(MaxChars - 100) + TEXT("...[truncated]");
        }
    }
};
```

### Creative Content Generation

Claude excels at creative tasks with proper guidance:

```cpp
void YourClass::GenerateCreativeContent()
{
    FGenClaudeChatSettings CreativeSettings;
    CreativeSettings.Model = EClaudeModels::Claude_4_Sonnet;
    CreativeSettings.MaxTokens = 2500;
    CreativeSettings.Temperature = 0.8f;  // Higher for creativity
    
    CreativeSettings.SystemMessage = TEXT(
        "You are a creative writer and game designer with expertise in fantasy worldbuilding. "
        "Create immersive, detailed content that would fit in a high-fantasy RPG. "
        "Focus on:\n"
        "- Rich, evocative descriptions\n"
        "- Consistent internal logic\n"
        "- Engaging narrative hooks\n"
        "- Game-appropriate content\n"
        "Format your response with clear sections."
    );
    
    CreativeSettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("Create a mysterious ancient dungeon for my RPG. Include:\n"
             "- The dungeon's history and purpose\n"
             "- 3 unique rooms with environmental storytelling\n"
             "- A central puzzle or challenge\n"
             "- Hidden lore that reveals the broader world story\n"
             "- Suggested loot and rewards")
    });
    
    UGenClaudeChat::SendChatRequest(CreativeSettings, OnCreativeContentReceived);
}
```

---

This comprehensive Anthropic Claude integration guide provides everything needed to leverage Claude's advanced reasoning and safety features in your Unreal Engine projects. Claude's constitutional AI training makes it particularly suitable for content generation that needs to be safe, helpful, and well-structured.