# XAI (Grok) Integration

Complete guide for integrating XAI's Grok models into your Unreal Engine projects using the UnrealGenAISupport plugin. XAI integration provides access to Grok's conversational AI capabilities with real-time knowledge and unique personality.

## Overview

The XAI Grok integration offers:
- **Grok 3 Models**: Latest generation with enhanced capabilities
- **Real-time Knowledge**: Access to current information and events
- **Unique Personality**: Grok's distinctive conversational style
- **High Performance**: Optimized for fast, engaging responses
- **Cost-Effective**: Competitive pricing for quality AI interactions
- **Flexible Configuration**: Customizable parameters for different use cases

## Supported Models

### Available Models

XAI integration uses string-based model selection for maximum flexibility:

```cpp
// Primary models
FString Model = TEXT("grok-3-latest");     // Latest Grok 3 model (recommended)
FString Model = TEXT("grok-3-mini-beta");  // Faster, lighter Grok 3 variant

// Custom model support
FString Model = TEXT("your-custom-model"); // Any XAI-compatible model
```

### Model Characteristics

| Model | Context Window | Performance | Best For |
|-------|----------------|-------------|----------|
| **grok-3-latest** | Large | High | Conversational AI, real-time information, creative tasks |
| **grok-3-mini-beta** | Large | Fast | Quick responses, simple queries, cost-effective usage |

### Choosing the Right Model

```cpp
// For comprehensive AI interactions
Settings.Model = TEXT("grok-3-latest");

// For fast, cost-effective responses
Settings.Model = TEXT("grok-3-mini-beta");

// Dynamic selection based on query
FString SelectedModel = SelectOptimalGrokModel(UserQuery);
```

## API Configuration

### Authentication Setup

Set your XAI API key using environment variables:

```bash
# Windows
setx PS_XAIAPIKEY "xai-your-xai-api-key-here"

# macOS/Linux
export PS_XAIAPIKEY="xai-your-xai-api-key-here"
echo 'export PS_XAIAPIKEY="xai-your-xai-api-key-here"' >> ~/.bashrc
```

Restart Unreal Editor after setting environment variables.

### Runtime Configuration (Alternative)

For testing or packaged builds:

```cpp
// Set API key at runtime
UGenSecureKey::SetGenAIApiKeyRuntime(EGenAIOrgs::XAI, TEXT("xai-your-api-key"));

// Switch to runtime keys
UGenSecureKey::SetUseApiKeyFromEnvironmentVars(false);
```

## Data Structures

### XAI-Specific Message Format

XAI uses its own message structure for optimal compatibility:

```cpp
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FGenXAIMessage
{
    GENERATED_BODY()

    /** Message role (system, user, assistant) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI",
              meta = (DisplayName = "Role"))
    FString Role;

    /** Message content */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI",
              meta = (DisplayName = "Content"))
    FString Content;

    /** Default constructor */
    FGenXAIMessage() = default;

    /** Convenience constructor */
    FGenXAIMessage(const FString& InRole, const FString& InContent)
        : Role(InRole), Content(InContent) {}
};
```

### Chat Configuration Structure

```cpp
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FGenXAIChatSettings
{
    GENERATED_BODY()

    /** XAI model name (string-based for flexibility) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI",
              meta = (DisplayName = "Model"))
    FString Model = TEXT("grok-3-latest");

    /** Maximum number of tokens to generate */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI",
              meta = (ClampMin = "1", ClampMax = "32000",
                      DisplayName = "Max Tokens",
                      ToolTip = "Maximum number of tokens to generate in the response"))
    int32 MaxTokens = 2048;

    /** Array of XAI-specific messages */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI",
              meta = (DisplayName = "Messages",
                      ToolTip = "Conversation messages using XAI message format"))
    TArray<FGenXAIMessage> Messages;

    /** Temperature for response creativity */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI",
              meta = (ClampMin = "0.0", ClampMax = "2.0",
                      DisplayName = "Temperature"))
    float Temperature = 1.0f;

    /** Top-p nucleus sampling */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI",
              meta = (ClampMin = "0.0", ClampMax = "1.0",
                      DisplayName = "Top P"))
    float TopP = 1.0f;

    /** Top-k sampling */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI",
              meta = (ClampMin = "0", ClampMax = "100",
                      DisplayName = "Top K"))
    int32 TopK = 0;

    /** Frequency penalty */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI",
              meta = (ClampMin = "-2.0", ClampMax = "2.0",
                      DisplayName = "Frequency Penalty"))
    float FrequencyPenalty = 0.0f;

    /** Presence penalty */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI",
              meta = (ClampMin = "-2.0", ClampMax = "2.0",
                      DisplayName = "Presence Penalty"))
    float PresencePenalty = 0.0f;

    /** Stop sequences */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI",
              meta = (DisplayName = "Stop Sequences"))
    TArray<FString> Stop;

    /** User identifier for tracking */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI",
              meta = (DisplayName = "User ID"))
    FString User;
};
```

## C++ Integration

### Header Includes

```cpp
#include "Models/XAI/GenXAIChat.h"
#include "Data/XAI/GenXAIChatStructs.h"
#include "Secure/GenSecureKey.h"
```

### Basic Usage Pattern

```cpp
void YourClass::SendXAIRequest()
{
    // Configure XAI chat settings
    FGenXAIChatSettings ChatSettings;
    ChatSettings.Model = TEXT("grok-3-latest");
    ChatSettings.MaxTokens = 1500;
    ChatSettings.Temperature = 0.8f;  // Grok works well with higher creativity

    // Build conversation using XAI message format
    ChatSettings.Messages.Add(FGenXAIMessage{
        TEXT("system"), 
        TEXT("You are a helpful AI assistant for game development. "
             "Provide practical, engaging advice with Grok's characteristic wit and insight.")
    });
    
    ChatSettings.Messages.Add(FGenXAIMessage{
        TEXT("user"), 
        TEXT("What are the latest trends in game AI that I should consider for my indie game?")
    });

    // Send request with lambda callback
    UGenXAIChat::SendChatRequest(ChatSettings, 
        FOnXAIChatCompletionResponse::CreateLambda([this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
        {
            if (bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("Grok Response: %s"), *Response);
                ProcessGrokResponse(Response);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("XAI Error: %s"), *ErrorMessage);
                HandleXAIError(ErrorMessage);
            }
        })
    );
}

void YourClass::ProcessGrokResponse(const FString& Response)
{
    // Grok responses often include current information and unique insights
    ProcessAIResponse(Response);
    
    // Extract any time-sensitive information
    ExtractCurrentInformation(Response);
}

void YourClass::HandleXAIError(const FString& Error)
{
    // Handle XAI-specific errors
    if (Error.Contains(TEXT("rate_limit")))
    {
        HandleRateLimit();
    }
    else if (Error.Contains(TEXT("model_not_found")))
    {
        // Switch to a known working model
        RetryWithDifferentModel();
    }
}
```

### Advanced Configuration Example

```cpp
void YourClass::AdvancedXAIRequest()
{
    FGenXAIChatSettings AdvancedSettings;
    
    // Use latest Grok model
    AdvancedSettings.Model = TEXT("grok-3-latest");
    
    // Higher token limit for detailed responses
    AdvancedSettings.MaxTokens = 3000;
    
    // Balanced creativity for informative but engaging responses
    AdvancedSettings.Temperature = 0.7f;
    AdvancedSettings.TopP = 0.9f;
    
    // Reduce repetition
    AdvancedSettings.FrequencyPenalty = 0.3f;
    AdvancedSettings.PresencePenalty = 0.2f;
    
    // Add stop sequences for structured responses
    AdvancedSettings.Stop.Add(TEXT("\n---END---\n"));
    
    // User tracking
    AdvancedSettings.User = TEXT("game_developer_123");

    // System message that leverages Grok's strengths
    AdvancedSettings.Messages.Add(FGenXAIMessage{
        TEXT("system"), 
        TEXT("You are Grok, an AI assistant with access to current information and a knack for "
             "providing insightful, practical advice. Help with game development questions by:\n"
             "1. Providing current industry trends and information\n"
             "2. Offering practical, actionable advice\n"
             "3. Being engaging and slightly witty in your explanations\n"
             "4. Considering both technical and business aspects\n"
             "5. Drawing connections to current events when relevant")
    });
    
    // Complex query that benefits from current knowledge
    AdvancedSettings.Messages.Add(FGenXAIMessage{
        TEXT("user"), 
        TEXT("I'm developing a multiplayer survival game inspired by recent world events. "
             "What current technologies, trends, and player behaviors should I consider? "
             "How can I make the game relevant to today's gaming landscape while ensuring "
             "long-term appeal? Consider both technical implementation in Unreal Engine "
             "and market positioning.")
    });

    UGenXAIChat::SendChatRequest(AdvancedSettings, OnAdvancedXAIResponse);
}
```

### Model Selection Logic

```cpp
FString YourClass::SelectOptimalGrokModel(const FString& UserQuery, bool bPrioritizeSpeed = false)
{
    // Analyze query to determine optimal model
    
    if (bPrioritizeSpeed)
    {
        UE_LOG(LogTemp, Log, TEXT("Selected grok-3-mini-beta for fast response"));
        return TEXT("grok-3-mini-beta");
    }
    
    // Check for indicators that benefit from Grok's latest capabilities
    TArray<FString> AdvancedKeywords = {
        TEXT("current"), TEXT("latest"), TEXT("recent"), TEXT("trends"),
        TEXT("news"), TEXT("today"), TEXT("now"), TEXT("modern"),
        TEXT("analysis"), TEXT("complex"), TEXT("detailed"), TEXT("comprehensive")
    };
    
    for (const FString& Keyword : AdvancedKeywords)
    {
        if (UserQuery.Contains(Keyword, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogTemp, Log, TEXT("Selected grok-3-latest for advanced query"));
            return TEXT("grok-3-latest");
        }
    }
    
    // Default to latest for best quality
    UE_LOG(LogTemp, Log, TEXT("Selected grok-3-latest as default"));
    return TEXT("grok-3-latest");
}
```

### Conversation Management

```cpp
class YOURGAME_API UGrokConversationManager : public UObject
{
    GENERATED_BODY()

private:
    UPROPERTY()
    TArray<FGenXAIMessage> ConversationHistory;
    
    UPROPERTY()
    FString CurrentModel = TEXT("grok-3-latest");
    
    int32 MaxHistoryLength = 20;  // Keep conversation focused

public:
    void InitializeConversation(const FString& SystemPrompt, const FString& Model = TEXT("grok-3-latest"))
    {
        CurrentModel = Model;
        ConversationHistory.Empty();
        
        // Add system message
        ConversationHistory.Add(FGenXAIMessage{TEXT("system"), SystemPrompt});
    }
    
    void AddUserMessage(const FString& Message)
    {
        ConversationHistory.Add(FGenXAIMessage{TEXT("user"), Message});
        TrimHistoryIfNeeded();
    }
    
    void AddAssistantMessage(const FString& Message)
    {
        ConversationHistory.Add(FGenXAIMessage{TEXT("assistant"), Message});
        TrimHistoryIfNeeded();
    }
    
    void SendMessage(const FString& UserMessage, float Temperature = 0.8f)
    {
        AddUserMessage(UserMessage);
        
        FGenXAIChatSettings Settings;
        Settings.Model = CurrentModel;
        Settings.MaxTokens = 2000;
        Settings.Temperature = Temperature;
        Settings.Messages = ConversationHistory;
        
        UGenXAIChat::SendChatRequest(Settings, 
            FOnXAIChatCompletionResponse::CreateUObject(this, &UGrokConversationManager::OnMessageReceived)
        );
    }
    
    void SwitchModel(const FString& NewModel)
    {
        CurrentModel = NewModel;
        UE_LOG(LogTemp, Log, TEXT("Switched to Grok model: %s"), *NewModel);
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
        // Keep system message and recent conversation
        while (ConversationHistory.Num() > MaxHistoryLength)
        {
            // Remove oldest non-system message
            for (int32 i = 1; i < ConversationHistory.Num(); i++)
            {
                if (ConversationHistory[i].Role != TEXT("system"))
                {
                    ConversationHistory.RemoveAt(i);
                    break;
                }
            }
            
            // Safety check to prevent infinite loop
            if (ConversationHistory.Num() <= 1) break;
        }
    }

public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConversationUpdate, const FString&, Message, bool, bSuccess);
    
    UPROPERTY(BlueprintAssignable)
    FOnConversationUpdate OnConversationUpdate;
};
```

## Blueprint Integration

### Blueprint Latent Node

In Blueprints, use the "Request XAI Chat" latent node:

```cpp
UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), 
          Category = "GenAI|XAI")
static UGenXAIChat* RequestXAIChat(UObject* WorldContextObject, const FGenXAIChatSettings& ChatSettings);
```

### Blueprint Setup Example

1. **Add "Request XAI Chat" node** to your Blueprint
2. **Configure Chat Settings**:
   - Set Model to desired Grok model (e.g., "grok-3-latest")
   - Set Max Tokens (e.g., 2048)
   - Set Temperature (0.6-0.9 works well for Grok)
   - Build Messages array with XAI message format
3. **Connect execution pins**:
   - Input execution → Request XAI Chat
   - On Success → Process response logic
   - On Failure → Error handling logic

### Blueprint Message Construction

```cpp
// Example Blueprint message array construction for XAI
TArray<FGenXAIMessage> Messages;

// System message
FGenXAIMessage SystemMessage;
SystemMessage.Role = TEXT("system");
SystemMessage.Content = TEXT("You are Grok, helping with game development. Be insightful and practical.");
Messages.Add(SystemMessage);

// User message
FGenXAIMessage UserMessage;
UserMessage.Role = TEXT("user");
UserMessage.Content = TEXT("What's the current state of AI in gaming?");
Messages.Add(UserMessage);

// Set in Chat Settings
ChatSettings.Messages = Messages;
ChatSettings.Model = TEXT("grok-3-latest");
```

## XAI-Specific Features

### Leveraging Real-time Knowledge

Grok's access to current information makes it ideal for certain use cases:

```cpp
void YourClass::GetCurrentGamingTrends()
{
    FGenXAIChatSettings TrendsSettings;
    TrendsSettings.Model = TEXT("grok-3-latest");
    TrendsSettings.MaxTokens = 2500;
    TrendsSettings.Temperature = 0.7f;
    
    TrendsSettings.Messages.Add(FGenXAIMessage{
        TEXT("system"), 
        TEXT("You have access to current information. Provide up-to-date insights about "
             "gaming trends, technologies, and market conditions. Include specific examples "
             "and recent developments when relevant.")
    });
    
    TrendsSettings.Messages.Add(FGenXAIMessage{
        TEXT("user"), 
        TEXT("What are the most significant gaming trends happening right now? "
             "Include information about:\n"
             "- New technologies being adopted\n"
             "- Popular game mechanics and genres\n"
             "- Platform trends (PC, console, mobile, VR)\n"
             "- Monetization strategies\n"
             "- Player behavior changes\n"
             "Provide specific examples and recent developments.")
    });
    
    UGenXAIChat::SendChatRequest(TrendsSettings, OnCurrentTrendsReceived);
}

void YourClass::OnCurrentTrendsReceived(const FString& Response, const FString& Error, bool Success)
{
    if (Success)
    {
        // Process real-time information
        ExtractTrendData(Response);
        UpdateGameStrategy(Response);
    }
}
```

### Grok's Personality and Engagement

```cpp
void YourClass::GetEngagingGameContent()
{
    FGenXAIChatSettings ContentSettings;
    ContentSettings.Model = TEXT("grok-3-latest");
    ContentSettings.MaxTokens = 2000;
    ContentSettings.Temperature = 0.9f;  // Higher for creativity and personality
    
    ContentSettings.Messages.Add(FGenXAIMessage{
        TEXT("system"), 
        TEXT("You are Grok, known for wit, insight, and engaging explanations. "
             "Help create game content that's both informative and entertaining. "
             "Use your characteristic style while being helpful and practical.")
    });
    
    ContentSettings.Messages.Add(FGenXAIMessage{
        TEXT("user"), 
        TEXT("I need creative quest ideas for my fantasy RPG that avoid typical fetch quests. "
             "Make them engaging, memorable, and fun to develop. Think outside the box!")
    });
    
    UGenXAIChat::SendChatRequest(ContentSettings, OnCreativeContentReceived);
}
```

### Market Analysis Integration

```cpp
void YourClass::AnalyzeGameMarket(const FString& GameGenre, const FString& TargetPlatform)
{
    FGenXAIChatSettings MarketSettings;
    MarketSettings.Model = TEXT("grok-3-latest");
    MarketSettings.MaxTokens = 3000;
    MarketSettings.Temperature = 0.6f;  // Balanced for analysis
    
    MarketSettings.Messages.Add(FGenXAIMessage{
        TEXT("system"), 
        TEXT("Analyze the current gaming market using your access to recent data. "
             "Provide actionable insights for game developers including market conditions, "
             "competition, opportunities, and risks. Back up claims with specific examples.")
    });
    
    FString MarketQuery = FString::Printf(
        TEXT("Analyze the current market for %s games on %s platform. Include:\n"
             "1. Market size and growth trends\n"
             "2. Key competitors and their strategies\n"
             "3. Player preferences and behavior\n"
             "4. Monetization opportunities\n"
             "5. Technical considerations and requirements\n"
             "6. Marketing and distribution strategies\n"
             "7. Risks and challenges to consider\n"
             "Provide specific, actionable advice for an indie developer."),
        *GameGenre, *TargetPlatform
    );
    
    MarketSettings.Messages.Add(FGenXAIMessage{TEXT("user"), MarketQuery});
    
    UGenXAIChat::SendChatRequest(MarketSettings, OnMarketAnalysisReceived);
}
```

## Error Handling and Troubleshooting

### XAI-Specific Error Types

#### Model Availability
```cpp
void YourClass::HandleModelError(const FString& Error)
{
    if (Error.Contains(TEXT("model_not_found")) || Error.Contains(TEXT("invalid_model")))
    {
        UE_LOG(LogTemp, Warning, TEXT("XAI model not available, switching to fallback"));
        
        // Switch to known working model
        FString FallbackModel = TEXT("grok-3-mini-beta");
        RetryWithModel(FallbackModel);
    }
}

void YourClass::RetryWithModel(const FString& NewModel)
{
    FGenXAIChatSettings RetrySettings = LastFailedSettings;
    RetrySettings.Model = NewModel;
    
    UE_LOG(LogTemp, Log, TEXT("Retrying XAI request with model: %s"), *NewModel);
    UGenXAIChat::SendChatRequest(RetrySettings, OnRetryResponse);
}
```

#### Authentication Issues
```cpp
void YourClass::HandleXAIAuth(const FString& Error)
{
    if (Error.Contains(TEXT("authentication")) || Error.Contains(TEXT("unauthorized")))
    {
        UE_LOG(LogTemp, Error, TEXT("XAI authentication failed"));
        UE_LOG(LogTemp, Error, TEXT("Check PS_XAIAPIKEY environment variable"));
        UE_LOG(LogTemp, Error, TEXT("Ensure key format is correct for XAI"));
        
        // Check if key is set
        FString ApiKey = UGenSecureKey::GetGenerativeAIApiKey(EGenAIOrgs::XAI);
        if (ApiKey.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("No XAI API key found"));
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("API key is set (length: %d)"), ApiKey.Len());
        }
    }
}
```

### Performance Monitoring

```cpp
class YOURGAME_API UXAIPerformanceTracker : public UObject
{
    GENERATED_BODY()

private:
    struct FXAIRequestMetrics
    {
        double StartTime;
        double EndTime;
        FString Model;
        int32 InputTokens;
        int32 OutputTokens;
        bool bSuccess;
    };
    
    TArray<FXAIRequestMetrics> RequestHistory;
    int32 MaxHistorySize = 100;

public:
    void StartTracking(const FString& Model, int32 EstimatedInputTokens)
    {
        FXAIRequestMetrics Metrics;
        Metrics.StartTime = FPlatformTime::Seconds();
        Metrics.Model = Model;
        Metrics.InputTokens = EstimatedInputTokens;
        Metrics.bSuccess = false;
        
        RequestHistory.Add(Metrics);
        
        // Trim history if needed
        while (RequestHistory.Num() > MaxHistorySize)
        {
            RequestHistory.RemoveAt(0);
        }
    }
    
    void EndTracking(bool bSuccess, int32 OutputTokens)
    {
        if (RequestHistory.Num() > 0)
        {
            FXAIRequestMetrics& LastRequest = RequestHistory.Last();
            LastRequest.EndTime = FPlatformTime::Seconds();
            LastRequest.OutputTokens = OutputTokens;
            LastRequest.bSuccess = bSuccess;
            
            AnalyzePerformance(LastRequest);
        }
    }
    
    void GetPerformanceStats(FString& OutStats)
    {
        if (RequestHistory.Num() == 0)
        {
            OutStats = TEXT("No XAI requests tracked yet");
            return;
        }
        
        // Calculate averages
        double TotalDuration = 0.0;
        int32 TotalTokens = 0;
        int32 SuccessCount = 0;
        
        for (const auto& Metrics : RequestHistory)
        {
            TotalDuration += (Metrics.EndTime - Metrics.StartTime);
            TotalTokens += Metrics.OutputTokens;
            if (Metrics.bSuccess) SuccessCount++;
        }
        
        double AvgDuration = TotalDuration / RequestHistory.Num();
        double AvgTokens = (double)TotalTokens / RequestHistory.Num();
        double SuccessRate = (double)SuccessCount / RequestHistory.Num() * 100.0;
        
        OutStats = FString::Printf(
            TEXT("XAI Performance Stats:\n"
                 "Requests: %d\n"
                 "Success Rate: %.1f%%\n"
                 "Avg Duration: %.2f seconds\n"
                 "Avg Output Tokens: %.1f\n"
                 "Avg Tokens/Second: %.1f"),
            RequestHistory.Num(),
            SuccessRate,
            AvgDuration,
            AvgTokens,
            AvgDuration > 0 ? AvgTokens / AvgDuration : 0.0
        );
    }

private:
    void AnalyzePerformance(const FXAIRequestMetrics& Metrics)
    {
        double Duration = Metrics.EndTime - Metrics.StartTime;
        
        UE_LOG(LogGenPerformance, Log, TEXT("XAI Request Performance:"));
        UE_LOG(LogGenPerformance, Log, TEXT("Model: %s"), *Metrics.Model);
        UE_LOG(LogGenPerformance, Log, TEXT("Duration: %.2f seconds"), Duration);
        UE_LOG(LogGenPerformance, Log, TEXT("Success: %s"), Metrics.bSuccess ? TEXT("Yes") : TEXT("No"));
        
        if (Metrics.bSuccess)
        {
            UE_LOG(LogGenPerformance, Log, TEXT("Output Tokens: %d"), Metrics.OutputTokens);
            if (Duration > 0)
            {
                UE_LOG(LogGenPerformance, Log, TEXT("Tokens/Second: %.2f"), Metrics.OutputTokens / Duration);
            }
        }
        
        // Performance warnings
        if (Duration > 30.0)
        {
            UE_LOG(LogGenPerformance, Warning, TEXT("XAI request took %.2f seconds - unusually long"), Duration);
        }
        
        if (!Metrics.bSuccess)
        {
            UE_LOG(LogGenPerformance, Error, TEXT("XAI request failed after %.2f seconds"), Duration);
        }
    }
};
```

## Best Practices

### Optimizing for Grok's Strengths

```cpp
void YourClass::OptimizeForGrok()
{
    FGenXAIChatSettings OptimalSettings;
    
    // Use latest model for best capabilities
    OptimalSettings.Model = TEXT("grok-3-latest");
    
    // Allow sufficient tokens for detailed responses
    OptimalSettings.MaxTokens = 2500;
    
    // Higher temperature leverages Grok's personality
    OptimalSettings.Temperature = 0.8f;
    
    // System message that plays to Grok's strengths
    OptimalSettings.Messages.Add(FGenXAIMessage{
        TEXT("system"), 
        TEXT("You are Grok, with access to current information and a talent for insightful, "
             "engaging explanations. For game development questions:\n"
             "1. Provide current industry context and trends\n"
             "2. Offer practical, actionable advice\n"
             "3. Be engaging and slightly witty in explanations\n"
             "4. Draw connections to recent developments\n"
             "5. Consider both technical and business perspectives\n"
             "Use your unique perspective to provide valuable insights.")
    });
    
    // Query that benefits from current knowledge
    OptimalSettings.Messages.Add(FGenXAIMessage{
        TEXT("user"), 
        TEXT("How should indie game developers adapt their strategies based on current market trends? "
             "Consider recent changes in player behavior, platform policies, and technology adoption.")
    });
    
    UGenXAIChat::SendChatRequest(OptimalSettings, OnOptimalGrokResponse);
}
```

### Real-time Information Integration

```cpp
class YOURGAME_API UGrokInfoIntegrator : public UObject
{
    GENERATED_BODY()

public:
    // Get current information about specific topics
    UFUNCTION(BlueprintCallable, Category = "XAI")
    void GetCurrentInfo(const FString& Topic, const FString& Context)
    {
        FGenXAIChatSettings InfoSettings;
        InfoSettings.Model = TEXT("grok-3-latest");
        InfoSettings.MaxTokens = 2000;
        InfoSettings.Temperature = 0.6f;  // Balanced for factual information
        
        InfoSettings.Messages.Add(FGenXAIMessage{
            TEXT("system"), 
            TEXT("Provide current, accurate information on the requested topic. "
                 "Include recent developments, current status, and relevant context. "
                 "Be specific about timing and sources when possible.")
        });
        
        FString InfoQuery = FString::Printf(
            TEXT("Provide current information about %s in the context of %s. "
                 "Include recent developments, current trends, and practical implications."),
            *Topic, *Context
        );
        
        InfoSettings.Messages.Add(FGenXAIMessage{TEXT("user"), InfoQuery});
        
        UGenXAIChat::SendChatRequest(InfoSettings, 
            FOnXAIChatCompletionResponse::CreateUObject(this, &UGrokInfoIntegrator::OnInfoReceived)
        );
    }

private:
    UFUNCTION()
    void OnInfoReceived(const FString& Response, const FString& Error, bool bSuccess)
    {
        if (bSuccess)
        {
            ProcessCurrentInformation(Response);
            OnCurrentInfoUpdate.Broadcast(Response);
        }
        else
        {
            OnInfoError.Broadcast(Error);
        }
    }
    
    void ProcessCurrentInformation(const FString& Info)
    {
        // Extract time-sensitive information
        TArray<FString> CurrentFacts;
        ExtractCurrentFacts(Info, CurrentFacts);
        
        // Cache for later use
        CacheCurrentInformation(CurrentFacts);
    }
    
    void ExtractCurrentFacts(const FString& Info, TArray<FString>& OutFacts)
    {
        // Look for temporal indicators
        TArray<FString> TimeIndicators = {
            TEXT("currently"), TEXT("recently"), TEXT("now"), TEXT("today"),
            TEXT("this year"), TEXT("2024"), TEXT("2025"), TEXT("latest")
        };
        
        TArray<FString> Sentences;
        Info.ParseIntoArray(Sentences, TEXT("."), true);
        
        for (const FString& Sentence : Sentences)
        {
            for (const FString& Indicator : TimeIndicators)
            {
                if (Sentence.Contains(Indicator, ESearchCase::IgnoreCase))
                {
                    OutFacts.Add(Sentence.TrimStartAndEnd());
                    break;
                }
            }
        }
    }
    
    void CacheCurrentInformation(const TArray<FString>& Facts)
    {
        // Store current information with timestamp
        FDateTime Now = FDateTime::Now();
        for (const FString& Fact : Facts)
        {
            UE_LOG(LogTemp, Log, TEXT("Current Info [%s]: %s"), *Now.ToString(), *Fact);
        }
    }

public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentInfoUpdate, const FString&, Info);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInfoError, const FString&, Error);
    
    UPROPERTY(BlueprintAssignable)
    FOnCurrentInfoUpdate OnCurrentInfoUpdate;
    
    UPROPERTY(BlueprintAssignable)
    FOnInfoError OnInfoError;
};
```

### Game Development Use Cases

```cpp
// Example: Community feedback analysis
void YourClass::AnalyzeCommunityFeedback(const FString& GameTitle)
{
    FGenXAIChatSettings FeedbackSettings;
    FeedbackSettings.Model = TEXT("grok-3-latest");
    FeedbackSettings.MaxTokens = 2500;
    FeedbackSettings.Temperature = 0.7f;
    
    FeedbackSettings.Messages.Add(FGenXAIMessage{
        TEXT("system"), 
        TEXT("Analyze current gaming community sentiment and feedback. "
             "Look for patterns, common complaints, and emerging trends in player feedback.")
    });
    
    FString FeedbackQuery = FString::Printf(
        TEXT("What is the current community sentiment around %s? "
             "Analyze recent player feedback, reviews, and discussions. "
             "Identify key themes, both positive and negative, and suggest actionable improvements."),
        *GameTitle
    );
    
    FeedbackSettings.Messages.Add(FGenXAIMessage{TEXT("user"), FeedbackQuery});
    
    UGenXAIChat::SendChatRequest(FeedbackSettings, OnFeedbackAnalysisReceived);
}

// Example: Technology adoption guidance
void YourClass::GetTechnologyGuidance(const FString& TechArea)
{
    FGenXAIChatSettings TechSettings;
    TechSettings.Model = TEXT("grok-3-latest");
    TechSettings.MaxTokens = 2000;
    TechSettings.Temperature = 0.6f;
    
    TechSettings.Messages.Add(FGenXAIMessage{
        TEXT("system"), 
        TEXT("Provide current guidance on game development technologies. "
             "Include adoption rates, industry support, pros/cons, and practical implementation advice.")
    });
    
    FString TechQuery = FString::Printf(
        TEXT("Should I adopt %s for my Unreal Engine project? "
             "Provide current information about:\n"
             "- Industry adoption and support\n"
             "- Integration with Unreal Engine\n"
             "- Performance implications\n"
             "- Development complexity\n"
             "- Future outlook\n"
             "Give practical recommendation for an indie developer."),
        *TechArea
    );
    
    TechSettings.Messages.Add(FGenXAIMessage{TEXT("user"), TechQuery});
    
    UGenXAIChat::SendChatRequest(TechSettings, OnTechGuidanceReceived);
}
```

---

This comprehensive XAI Grok integration guide provides everything needed to leverage Grok's unique capabilities, real-time knowledge access, and engaging personality in your Unreal Engine projects. Grok's strength lies in providing current, insightful information with an engaging conversational style that can enhance both development workflows and player-facing AI features.