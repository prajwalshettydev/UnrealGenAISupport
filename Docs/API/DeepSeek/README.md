# DeepSeek Integration

Complete guide for integrating DeepSeek's advanced AI models into your Unreal Engine projects using the UnrealGenAISupport plugin. DeepSeek integration provides cutting-edge reasoning capabilities with their R1 reasoning model and cost-effective chat solutions.

## Overview

The DeepSeek integration offers:
- **DeepSeek R1**: Advanced reasoning model with chain-of-thought capabilities
- **DeepSeek Chat**: High-performance general-purpose conversational AI
- **Cost-Effective**: Competitive pricing for high-quality AI capabilities
- **Extended Context**: Large context windows for complex conversations
- **Reasoning Transparency**: Visible reasoning process with R1 model
- **Streaming Support**: Real-time response generation (planned)

## Supported Models

### Available Model Enum

```cpp
enum class EDeepSeekModels : uint8
{
    Chat         UMETA(DisplayName = "DeepSeek Chat"),      // "deepseek-chat"
    Reasoner     UMETA(DisplayName = "DeepSeek R1"),        // "deepseek-reasoner"
    Unknown      UMETA(DisplayName = "Unknown")             // "unknown"
};
```

### Model Characteristics

| Model | Context Window | Performance | Best For |
|-------|----------------|-------------|----------|
| **DeepSeek Chat** | 64k tokens | High | General conversations, coding, analysis |
| **DeepSeek R1** | 64k tokens | Highest | Complex reasoning, problem-solving, step-by-step analysis |

### Choosing the Right Model

```cpp
// For complex reasoning and problem-solving
Settings.Model = EDeepSeekModels::Reasoner;

// For general chat and quick responses
Settings.Model = EDeepSeekModels::Chat;

// Let the system decide based on content
EDeepSeekModels SelectedModel = SelectOptimalDeepSeekModel(UserQuery);
```

## API Configuration

### Authentication Setup

Set your DeepSeek API key using environment variables:

```bash
# Windows
setx PS_DEEPSEEKAPIKEY "sk-your-deepseek-api-key-here"

# macOS/Linux
export PS_DEEPSEEKAPIKEY="sk-your-deepseek-api-key-here"
echo 'export PS_DEEPSEEKAPIKEY="sk-your-deepseek-api-key-here"' >> ~/.bashrc
```

Restart Unreal Editor after setting environment variables.

### Runtime Configuration (Alternative)

For testing or packaged builds:

```cpp
// Set API key at runtime
UGenSecureKey::SetGenAIApiKeyRuntime(EGenAIOrgs::DeepSeek, TEXT("sk-your-api-key"));

// Switch to runtime keys
UGenSecureKey::SetUseApiKeyFromEnvironmentVars(false);
```

### Important HTTP Configuration

⚠️ **Critical for DeepSeek R1**: The reasoning model can take longer than 30 seconds to respond. Configure HTTP timeouts:

```ini
# Add to your project's DefaultEngine.ini
[HTTP]
HttpConnectionTimeout=180
HttpReceiveTimeout=180
```

## Chat Configuration Structure

```cpp
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FGenDSeekChatSettings
{
    GENERATED_BODY()

    /** DeepSeek model selection */
    UPROPERTY(BlueprintReadWrite, Category = "GenAI",
              meta = (DisplayName = "DeepSeek Model"))
    EDeepSeekModels Model = EDeepSeekModels::Chat;

    /** Maximum number of tokens to generate */
    UPROPERTY(BlueprintReadWrite, Category = "GenAI",
              meta = (ClampMin = "1", ClampMax = "64000",
                      DisplayName = "Max Tokens",
                      ToolTip = "Maximum number of tokens to generate in the response"))
    int32 MaxTokens = 4096;

    /** Array of messages for the conversation */
    UPROPERTY(BlueprintReadWrite, Category = "GenAI",
              meta = (DisplayName = "Messages",
                      ToolTip = "Conversation messages with role and content"))
    TArray<FGenChatMessage> Messages;

    /** Enable streaming responses (planned feature) */
    UPROPERTY(BlueprintReadWrite, Category = "GenAI",
              meta = (DisplayName = "Stream Response"))
    bool bStreamResponse = false;

    /** Temperature for response randomness (R1 specific) */
    UPROPERTY(BlueprintReadWrite, Category = "GenAI",
              meta = (ClampMin = "0.0", ClampMax = "2.0",
                      DisplayName = "Temperature"))
    float Temperature = 1.0f;

    /** Top-p nucleus sampling */
    UPROPERTY(BlueprintReadWrite, Category = "GenAI",
              meta = (ClampMin = "0.0", ClampMax = "1.0",
                      DisplayName = "Top P"))
    float TopP = 1.0f;

    /** Frequency penalty */
    UPROPERTY(BlueprintReadWrite, Category = "GenAI",
              meta = (ClampMin = "-2.0", ClampMax = "2.0",
                      DisplayName = "Frequency Penalty"))
    float FrequencyPenalty = 0.0f;

    /** Presence penalty */
    UPROPERTY(BlueprintReadWrite, Category = "GenAI",
              meta = (ClampMin = "-2.0", ClampMax = "2.0",
                      DisplayName = "Presence Penalty"))
    float PresencePenalty = 0.0f;

    /** Stop sequences to end generation */
    UPROPERTY(BlueprintReadWrite, Category = "GenAI",
              meta = (DisplayName = "Stop Sequences"))
    TArray<FString> Stop;
};
```

## C++ Integration

### Header Includes

```cpp
#include "Models/DeepSeek/GenDSeekChat.h"
#include "Data/DeepSeek/GenDSeekChatStructs.h"  // If available
#include "Secure/GenSecureKey.h"
```

### Basic Usage Pattern

```cpp
void YourClass::SendDeepSeekRequest()
{
    // Configure DeepSeek chat settings
    FGenDSeekChatSettings ChatSettings;
    ChatSettings.Model = EDeepSeekModels::Chat;
    ChatSettings.MaxTokens = 2048;
    ChatSettings.bStreamResponse = false;

    // Build conversation with system message
    ChatSettings.Messages.Add(FGenChatMessage{
        TEXT("system"), 
        TEXT("You are an expert Unreal Engine developer specializing in performance optimization and advanced C++ techniques.")
    });
    
    ChatSettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("How can I optimize my Blueprint-heavy game to achieve better performance on mobile devices?")
    });

    // Send request with lambda callback
    UGenDSeekChat::SendChatRequest(ChatSettings, 
        FOnDSeekChatCompletionResponse::CreateLambda([this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
        {
            if (bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("DeepSeek Response: %s"), *Response);
                ProcessDeepSeekResponse(Response);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("DeepSeek Error: %s"), *ErrorMessage);
                HandleDeepSeekError(ErrorMessage);
            }
        })
    );
}

void YourClass::ProcessDeepSeekResponse(const FString& Response)
{
    // DeepSeek responses are typically well-structured and detailed
    ProcessAIResponse(Response);
}

void YourClass::HandleDeepSeekError(const FString& Error)
{
    // Handle DeepSeek-specific errors
    if (Error.Contains(TEXT("timeout")))
    {
        // DeepSeek R1 may take longer - increase timeout and retry
        HandleTimeoutError();
    }
    else if (Error.Contains(TEXT("rate_limit")))
    {
        // Implement retry logic
        HandleRateLimit();
    }
}
```

### DeepSeek R1 Reasoning Example

```cpp
void YourClass::UseDeepSeekReasoning()
{
    FGenDSeekChatSettings ReasoningSettings;
    
    // Use the reasoning model for complex problems
    ReasoningSettings.Model = EDeepSeekModels::Reasoner;
    ReasoningSettings.MaxTokens = 4096;  // Allow space for reasoning
    ReasoningSettings.bStreamResponse = false;
    
    // Lower temperature for more focused reasoning
    ReasoningSettings.Temperature = 0.3f;

    // System message is crucial for R1 model
    ReasoningSettings.Messages.Add(FGenChatMessage{
        TEXT("system"), 
        TEXT("You are a senior game developer and systems architect. "
             "Think through problems step by step, showing your reasoning process. "
             "Consider performance, scalability, and maintainability in your solutions.")
    });
    
    // Complex reasoning task
    ReasoningSettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("I'm building a large-scale multiplayer survival game with the following requirements:\n"
             "- 100+ concurrent players per server\n"
             "- Persistent world with dynamic weather and day/night cycles\n"
             "- Complex crafting system with 500+ recipes\n"
             "- Player-built structures that persist\n"
             "- Real-time combat with physics-based interactions\n"
             "- Anti-cheat protection\n\n"
             "Design a comprehensive server architecture that can handle these requirements "
             "while maintaining 60+ FPS performance and sub-100ms latency. "
             "Show your reasoning process and consider trade-offs.")
    });

    UGenDSeekChat::SendChatRequest(ReasoningSettings, 
        FOnDSeekChatCompletionResponse::CreateLambda([this](const FString& Response, const FString& Error, bool Success)
        {
            if (Success)
            {
                // R1 responses often include visible reasoning chains
                ProcessReasoningResponse(Response);
            }
            else
            {
                HandleReasoningError(Error);
            }
        })
    );
}

void YourClass::ProcessReasoningResponse(const FString& Response)
{
    // DeepSeek R1 often provides step-by-step reasoning
    // Look for reasoning patterns and extract structured insights
    
    TArray<FString> ReasoningSteps;
    ExtractReasoningSteps(Response, ReasoningSteps);
    
    for (int32 i = 0; i < ReasoningSteps.Num(); i++)
    {
        UE_LOG(LogTemp, Log, TEXT("Reasoning Step %d: %s"), i + 1, *ReasoningSteps[i]);
    }
    
    // Extract final conclusion
    FString Conclusion = ExtractConclusion(Response);
    UE_LOG(LogTemp, Log, TEXT("Final Conclusion: %s"), *Conclusion);
}

void YourClass::ExtractReasoningSteps(const FString& Response, TArray<FString>& OutSteps)
{
    // DeepSeek R1 often uses numbered steps or clear section breaks
    TArray<FString> Lines;
    Response.ParseIntoArrayLines(Lines);
    
    for (const FString& Line : Lines)
    {
        FString TrimmedLine = Line.TrimStartAndEnd();
        
        // Look for step indicators
        if (TrimmedLine.MatchesWildcard(TEXT("[0-9]. *")) ||
            TrimmedLine.MatchesWildcard(TEXT("Step [0-9]*:*")) ||
            TrimmedLine.StartsWith(TEXT("First,")) ||
            TrimmedLine.StartsWith(TEXT("Second,")) ||
            TrimmedLine.StartsWith(TEXT("Next,")) ||
            TrimmedLine.StartsWith(TEXT("Finally,")))
        {
            OutSteps.Add(TrimmedLine);
        }
    }
}
```

### Advanced Configuration Example

```cpp
void YourClass::AdvancedDeepSeekRequest()
{
    FGenDSeekChatSettings AdvancedSettings;
    
    // Use reasoning model for complex analysis
    AdvancedSettings.Model = EDeepSeekModels::Reasoner;
    
    // High token limit for detailed reasoning
    AdvancedSettings.MaxTokens = 6000;
    
    // Balanced temperature for reasoning and creativity
    AdvancedSettings.Temperature = 0.5f;
    AdvancedSettings.TopP = 0.9f;
    
    // Reduce repetition in reasoning
    AdvancedSettings.FrequencyPenalty = 0.3f;
    AdvancedSettings.PresencePenalty = 0.3f;
    
    // Add stop sequences for structured output
    AdvancedSettings.Stop.Add(TEXT("\n---CONCLUSION---\n"));
    AdvancedSettings.Stop.Add(TEXT("\n[END_REASONING]\n"));

    // Complex system message for reasoning
    AdvancedSettings.Messages.Add(FGenChatMessage{
        TEXT("system"), 
        TEXT("You are an expert technical architect and game developer with 15+ years of experience. "
             "When solving complex problems:\n"
             "1. Break down the problem into key components\n"
             "2. Analyze each component's requirements and constraints\n"
             "3. Consider multiple solution approaches\n"
             "4. Evaluate trade-offs (performance vs. complexity, cost vs. quality)\n"
             "5. Provide a detailed implementation strategy\n"
             "6. Identify potential risks and mitigation strategies\n"
             "Show your reasoning process clearly and conclude with actionable recommendations.")
    });
    
    // Multi-part technical challenge
    AdvancedSettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("I need to design a comprehensive AI system for an RTS game that includes:\n"
             "- Strategic AI for resource management and base building\n"
             "- Tactical AI for unit control and combat\n"
             "- Economic AI for trade and market simulation\n"
             "- Difficulty scaling that adapts to player skill\n"
             "- Performance optimization for 1000+ AI units\n"
             "- Mod support for custom AI behaviors\n\n"
             "The game targets 60 FPS on mid-range hardware with up to 8 AI players. "
             "Design this system considering modern AI techniques, Unreal Engine capabilities, "
             "and real-time performance constraints.")
    });

    UGenDSeekChat::SendChatRequest(AdvancedSettings, OnAdvancedDeepSeekResponse);
}
```

### Model Selection Logic

```cpp
EDeepSeekModels YourClass::SelectOptimalDeepSeekModel(const FString& UserQuery)
{
    // Analyze query complexity to choose the best model
    
    // Keywords that suggest need for reasoning
    TArray<FString> ReasoningKeywords = {
        TEXT("analyze"), TEXT("design"), TEXT("architecture"), TEXT("optimize"),
        TEXT("compare"), TEXT("evaluate"), TEXT("strategy"), TEXT("complex"),
        TEXT("step by step"), TEXT("how to"), TEXT("explain why"), TEXT("reasoning"),
        TEXT("pros and cons"), TEXT("trade-offs"), TEXT("best approach")
    };
    
    // Check for reasoning indicators
    int32 ReasoningScore = 0;
    for (const FString& Keyword : ReasoningKeywords)
    {
        if (UserQuery.Contains(Keyword, ESearchCase::IgnoreCase))
        {
            ReasoningScore++;
        }
    }
    
    // Check query complexity
    int32 QueryLength = UserQuery.Len();
    int32 SentenceCount = UserQuery.Split(TEXT(".")).Num();
    int32 QuestionCount = UserQuery.Split(TEXT("?")).Num();
    
    // Decision logic
    if (ReasoningScore >= 2 || QueryLength > 500 || SentenceCount > 3 || QuestionCount > 2)
    {
        UE_LOG(LogTemp, Log, TEXT("Selected DeepSeek R1 for complex reasoning task"));
        return EDeepSeekModels::Reasoner;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Selected DeepSeek Chat for general conversation"));
        return EDeepSeekModels::Chat;
    }
}
```

## Blueprint Integration

### Blueprint Latent Node

In Blueprints, use the "Request DeepSeek Chat" latent node:

```cpp
UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), 
          Category = "GenAI|DeepSeek")
static UGenDSeekChat* RequestDeepseekChat(UObject* WorldContextObject, const FGenDSeekChatSettings& ChatSettings);
```

### Blueprint Setup Example

1. **Add "Request DeepSeek Chat" node** to your Blueprint
2. **Configure Chat Settings**:
   - Set Model to desired DeepSeek model
   - Set Max Tokens (e.g., 4096 for reasoning tasks)
   - Build Messages array with system and user messages
3. **Connect execution pins**:
   - Input execution → Request DeepSeek Chat
   - On Success → Process response logic
   - On Failure → Error handling logic

### Blueprint Message Construction

```cpp
// Example Blueprint message array construction for DeepSeek
TArray<FGenChatMessage> Messages;

// System message (important for R1 reasoning)
FGenChatMessage SystemMessage;
SystemMessage.Role = TEXT("system");
SystemMessage.Content = TEXT("You are a helpful game development assistant with expertise in Unreal Engine. Provide detailed, step-by-step guidance.");
Messages.Add(SystemMessage);

// User message
FGenChatMessage UserMessage;
UserMessage.Role = TEXT("user");
UserMessage.Content = TEXT("How do I implement a robust save system for my RPG?");
Messages.Add(UserMessage);

// Set in Chat Settings
ChatSettings.Messages = Messages;
ChatSettings.Model = EDeepSeekModels::Reasoner;  // For complex guidance
```

## DeepSeek-Specific Features

### Reasoning Chain Analysis

DeepSeek R1 provides transparent reasoning processes:

```cpp
class YOURGAME_API UDeepSeekReasoningAnalyzer : public UObject
{
    GENERATED_BODY()

public:
    struct FReasoningStep
    {
        FString StepDescription;
        FString Reasoning;
        TArray<FString> Considerations;
        FString Conclusion;
    };

    UFUNCTION(BlueprintCallable, Category = "DeepSeek")
    static TArray<FReasoningStep> AnalyzeReasoningChain(const FString& Response)
    {
        TArray<FReasoningStep> Steps;
        
        // Parse DeepSeek R1 reasoning patterns
        TArray<FString> Sections;
        ParseIntoSections(Response, Sections);
        
        for (const FString& Section : Sections)
        {
            FReasoningStep Step;
            ExtractStepComponents(Section, Step);
            
            if (!Step.StepDescription.IsEmpty())
            {
                Steps.Add(Step);
            }
        }
        
        return Steps;
    }

private:
    static void ParseIntoSections(const FString& Response, TArray<FString>& OutSections)
    {
        // Split by common reasoning indicators
        TArray<FString> SplitPatterns = {
            TEXT("\n\n"),
            TEXT("Step "),
            TEXT("First,"),
            TEXT("Second,"),
            TEXT("Next,"),
            TEXT("Then,"),
            TEXT("Finally,"),
            TEXT("In conclusion,")
        };
        
        OutSections.Add(Response);  // Start with full response
        
        for (const FString& Pattern : SplitPatterns)
        {
            TArray<FString> NewSections;
            for (const FString& Section : OutSections)
            {
                TArray<FString> SplitResults;
                Section.ParseIntoArray(SplitResults, *Pattern, true);
                NewSections.Append(SplitResults);
            }
            OutSections = NewSections;
        }
    }
    
    static void ExtractStepComponents(const FString& Section, FReasoningStep& OutStep)
    {
        TArray<FString> Lines;
        Section.ParseIntoArrayLines(Lines);
        
        if (Lines.Num() == 0) return;
        
        // First line is often the step description
        OutStep.StepDescription = Lines[0].TrimStartAndEnd();
        
        // Look for reasoning and considerations
        for (int32 i = 1; i < Lines.Num(); i++)
        {
            FString Line = Lines[i].TrimStartAndEnd();
            
            if (Line.StartsWith(TEXT("Because")) || Line.StartsWith(TEXT("Since")) || Line.Contains(TEXT("reasoning")))
            {
                OutStep.Reasoning = Line;
            }
            else if (Line.StartsWith(TEXT("Consider")) || Line.StartsWith(TEXT("Note")) || Line.StartsWith(TEXT("Important")))
            {
                OutStep.Considerations.Add(Line);
            }
            else if (Line.StartsWith(TEXT("Therefore")) || Line.StartsWith(TEXT("Thus")) || Line.StartsWith(TEXT("Conclusion")))
            {
                OutStep.Conclusion = Line;
            }
        }
    }
};
```

### Performance Monitoring for Long Responses

```cpp
class YOURGAME_API UDeepSeekPerformanceTracker : public UObject
{
    GENERATED_BODY()

private:
    struct FRequestMetrics
    {
        double StartTime;
        double EndTime;
        EDeepSeekModels Model;
        int32 InputTokens;
        int32 OutputTokens;
        bool bTimedOut;
    };
    
    TArray<FRequestMetrics> RequestHistory;

public:
    void StartTracking(EDeepSeekModels Model, int32 EstimatedInputTokens)
    {
        FRequestMetrics Metrics;
        Metrics.StartTime = FPlatformTime::Seconds();
        Metrics.Model = Model;
        Metrics.InputTokens = EstimatedInputTokens;
        Metrics.bTimedOut = false;
        
        RequestHistory.Add(Metrics);
    }
    
    void EndTracking(bool bSuccess, int32 OutputTokens)
    {
        if (RequestHistory.Num() > 0)
        {
            FRequestMetrics& LastRequest = RequestHistory.Last();
            LastRequest.EndTime = FPlatformTime::Seconds();
            LastRequest.OutputTokens = OutputTokens;
            LastRequest.bTimedOut = !bSuccess;
            
            AnalyzePerformance(LastRequest);
        }
    }

private:
    void AnalyzePerformance(const FRequestMetrics& Metrics)
    {
        double Duration = Metrics.EndTime - Metrics.StartTime;
        
        UE_LOG(LogGenPerformance, Log, TEXT("DeepSeek Request Performance:"));
        UE_LOG(LogGenPerformance, Log, TEXT("Model: %s"), 
               Metrics.Model == EDeepSeekModels::Reasoner ? TEXT("R1") : TEXT("Chat"));
        UE_LOG(LogGenPerformance, Log, TEXT("Duration: %.2f seconds"), Duration);
        UE_LOG(LogGenPerformance, Log, TEXT("Input Tokens: %d"), Metrics.InputTokens);
        UE_LOG(LogGenPerformance, Log, TEXT("Output Tokens: %d"), Metrics.OutputTokens);
        UE_LOG(LogGenPerformance, Log, TEXT("Tokens/Second: %.2f"), 
               Duration > 0 ? Metrics.OutputTokens / Duration : 0.0);
        
        // Warn about unusually long response times
        if (Duration > 120.0 && Metrics.Model == EDeepSeekModels::Reasoner)
        {
            UE_LOG(LogGenPerformance, Warning, 
                   TEXT("DeepSeek R1 took %.2f seconds - consider increasing HTTP timeout"), Duration);
        }
        
        if (Metrics.bTimedOut)
        {
            UE_LOG(LogGenPerformance, Error, TEXT("DeepSeek request timed out after %.2f seconds"), Duration);
        }
    }
};
```

## Error Handling and Troubleshooting

### DeepSeek-Specific Error Types

#### Timeout Errors (Common with R1)
```cpp
void YourClass::HandleDeepSeekTimeout(const FString& Error)
{
    if (Error.Contains(TEXT("timeout")) || Error.Contains(TEXT("timed out")))
    {
        UE_LOG(LogTemp, Warning, TEXT("DeepSeek request timed out - R1 reasoning may take longer"));
        
        // Suggest configuration fixes
        UE_LOG(LogTemp, Log, TEXT("Consider adding to DefaultEngine.ini:"));
        UE_LOG(LogTemp, Log, TEXT("[HTTP]"));
        UE_LOG(LogTemp, Log, TEXT("HttpConnectionTimeout=180"));
        UE_LOG(LogTemp, Log, TEXT("HttpReceiveTimeout=180"));
        
        // Retry with shorter prompt or different model
        RetryWithOptimizedSettings();
    }
}

void YourClass::RetryWithOptimizedSettings()
{
    // Switch to Chat model for faster response
    FGenDSeekChatSettings RetrySettings = LastFailedSettings;
    RetrySettings.Model = EDeepSeekModels::Chat;
    RetrySettings.MaxTokens = FMath::Min(RetrySettings.MaxTokens, 2048);
    
    // Simplify the prompt
    if (RetrySettings.Messages.Num() > 0)
    {
        FString& LastMessage = RetrySettings.Messages.Last().Content;
        LastMessage = SimplifyPrompt(LastMessage);
    }
    
    UGenDSeekChat::SendChatRequest(RetrySettings, OnRetryResponse);
}

FString YourClass::SimplifyPrompt(const FString& OriginalPrompt)
{
    // Extract key question from complex prompt
    TArray<FString> Sentences;
    OriginalPrompt.ParseIntoArray(Sentences, TEXT("."), true);
    
    // Find the main question
    for (const FString& Sentence : Sentences)
    {
        if (Sentence.Contains(TEXT("?")) || Sentence.Contains(TEXT("how")) || Sentence.Contains(TEXT("what")))
        {
            return Sentence.TrimStartAndEnd() + TEXT("?");
        }
    }
    
    // Fallback: Take first sentence
    return Sentences.Num() > 0 ? Sentences[0] + TEXT(".") : OriginalPrompt.Left(200);
}
```

#### System Message Requirements
```cpp
void YourClass::ValidateDeepSeekSettings(FGenDSeekChatSettings& Settings)
{
    // DeepSeek R1 performs better with system messages
    if (Settings.Model == EDeepSeekModels::Reasoner)
    {
        bool bHasSystemMessage = false;
        for (const auto& Message : Settings.Messages)
        {
            if (Message.Role == TEXT("system"))
            {
                bHasSystemMessage = true;
                break;
            }
        }
        
        if (!bHasSystemMessage)
        {
            UE_LOG(LogTemp, Warning, TEXT("DeepSeek R1 works best with system messages"));
            
            // Add default system message
            FGenChatMessage SystemMessage;
            SystemMessage.Role = TEXT("system");
            SystemMessage.Content = TEXT("You are a helpful assistant. Think through problems step by step.");
            Settings.Messages.Insert(SystemMessage, 0);
        }
    }
}
```

### Debugging Tools

#### Response Quality Analysis
```cpp
void YourClass::AnalyzeDeepSeekResponseQuality(const FString& Response, EDeepSeekModels Model)
{
    UE_LOG(LogGenAIVerbose, Log, TEXT("DeepSeek Response Analysis:"));
    UE_LOG(LogGenAIVerbose, Log, TEXT("Model: %s"), 
           Model == EDeepSeekModels::Reasoner ? TEXT("R1") : TEXT("Chat"));
    UE_LOG(LogGenAIVerbose, Log, TEXT("Length: %d characters"), Response.Len());
    
    // Count reasoning indicators (especially for R1)
    int32 ReasoningIndicators = CountReasoningIndicators(Response);
    UE_LOG(LogGenAIVerbose, Log, TEXT("Reasoning indicators: %d"), ReasoningIndicators);
    
    // Check response structure
    bool bWellStructured = IsResponseWellStructured(Response);
    UE_LOG(LogGenAIVerbose, Log, TEXT("Well structured: %s"), bWellStructured ? TEXT("Yes") : TEXT("No"));
    
    // Analyze code blocks
    int32 CodeBlocks = CountCodeBlocks(Response);
    UE_LOG(LogGenAIVerbose, Log, TEXT("Code blocks: %d"), CodeBlocks);
    
    // Check for step-by-step reasoning (R1 specialty)
    if (Model == EDeepSeekModels::Reasoner)
    {
        bool bHasStepByStep = Response.Contains(TEXT("step")) || Response.Contains(TEXT("first")) || Response.Contains(TEXT("then"));
        UE_LOG(LogGenAIVerbose, Log, TEXT("Contains step-by-step reasoning: %s"), 
               bHasStepByStep ? TEXT("Yes") : TEXT("No"));
    }
}

int32 YourClass::CountReasoningIndicators(const FString& Response)
{
    TArray<FString> Indicators = {
        TEXT("because"), TEXT("therefore"), TEXT("since"), TEXT("thus"),
        TEXT("reasoning"), TEXT("analysis"), TEXT("conclusion"), TEXT("step"),
        TEXT("first"), TEXT("second"), TEXT("next"), TEXT("finally")
    };
    
    int32 Count = 0;
    for (const FString& Indicator : Indicators)
    {
        int32 SearchStart = 0;
        while (true)
        {
            int32 FoundIndex = Response.Find(Indicator, ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchStart);
            if (FoundIndex == INDEX_NONE) break;
            Count++;
            SearchStart = FoundIndex + Indicator.Len();
        }
    }
    return Count;
}
```

## Best Practices

### Optimizing for DeepSeek R1

```cpp
void YourClass::OptimizeForReasoningModel()
{
    FGenDSeekChatSettings OptimalSettings;
    
    // Use R1 for complex reasoning tasks
    OptimalSettings.Model = EDeepSeekModels::Reasoner;
    
    // Allow enough tokens for reasoning chain
    OptimalSettings.MaxTokens = 4096;
    
    // Lower temperature for focused reasoning
    OptimalSettings.Temperature = 0.3f;
    
    // Craft system message to encourage structured thinking
    OptimalSettings.Messages.Add(FGenChatMessage{
        TEXT("system"), 
        TEXT("You are an expert problem solver. For complex questions:\n"
             "1. Break down the problem into components\n"
             "2. Analyze each component systematically\n"
             "3. Consider multiple approaches\n"
             "4. Evaluate trade-offs\n"
             "5. Provide step-by-step reasoning\n"
             "6. Conclude with clear recommendations\n"
             "Show your thinking process clearly.")
    });
    
    // Frame questions to encourage reasoning
    OptimalSettings.Messages.Add(FGenChatMessage{
        TEXT("user"), 
        TEXT("I need to solve this complex architectural problem step by step: [problem description].\n"
             "Please think through this systematically and show your reasoning process.")
    });
    
    UGenDSeekChat::SendChatRequest(OptimalSettings, OnReasoningResponse);
}
```

### Cost-Effective Usage Strategies

```cpp
class YOURGAME_API UDeepSeekCostOptimizer : public UObject
{
    GENERATED_BODY()

public:
    // Smart model selection based on query type
    EDeepSeekModels SelectCostEffectiveModel(const FString& Query, int32 ComplexityThreshold = 3)
    {
        int32 ComplexityScore = CalculateQueryComplexity(Query);
        
        if (ComplexityScore >= ComplexityThreshold)
        {
            UE_LOG(LogTemp, Log, TEXT("High complexity (%d) - using DeepSeek R1"), ComplexityScore);
            return EDeepSeekModels::Reasoner;
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Low complexity (%d) - using DeepSeek Chat"), ComplexityScore);
            return EDeepSeekModels::Chat;
        }
    }

private:
    int32 CalculateQueryComplexity(const FString& Query)
    {
        int32 Score = 0;
        
        // Length factor
        if (Query.Len() > 200) Score++;
        if (Query.Len() > 500) Score++;
        
        // Question complexity
        int32 QuestionMarks = Query.Split(TEXT("?")).Num() - 1;
        Score += FMath::Min(QuestionMarks, 2);
        
        // Complex keywords
        TArray<FString> ComplexKeywords = {
            TEXT("analyze"), TEXT("design"), TEXT("architecture"), TEXT("optimize"),
            TEXT("compare"), TEXT("evaluate"), TEXT("strategy"), TEXT("implement"),
            TEXT("step by step"), TEXT("reasoning"), TEXT("trade-offs")
        };
        
        for (const FString& Keyword : ComplexKeywords)
        {
            if (Query.Contains(Keyword, ESearchCase::IgnoreCase))
            {
                Score++;
            }
        }
        
        return Score;
    }
};
```

### Integration with Game Systems

```cpp
// Example: AI-powered NPC dialogue generation
void YourClass::GenerateNPCDialogue(const FString& NPCBackground, const FString& PlayerContext)
{
    FGenDSeekChatSettings DialogueSettings;
    DialogueSettings.Model = EDeepSeekModels::Chat;  // Chat is sufficient for dialogue
    DialogueSettings.MaxTokens = 1000;
    DialogueSettings.Temperature = 0.8f;  // Higher for creative dialogue
    
    DialogueSettings.Messages.Add(FGenChatMessage{
        TEXT("system"), 
        TEXT("You are creating dialogue for an RPG. Generate natural, engaging conversation "
             "that fits the character and situation. Keep responses under 150 words.")
    });
    
    FString DialoguePrompt = FString::Printf(
        TEXT("Generate dialogue for an NPC with this background: %s\n"
             "Player context: %s\n"
             "The NPC should provide helpful information while staying in character."),
        *NPCBackground, *PlayerContext
    );
    
    DialogueSettings.Messages.Add(FGenChatMessage{TEXT("user"), DialoguePrompt});
    
    UGenDSeekChat::SendChatRequest(DialogueSettings, OnDialogueGenerated);
}

// Example: Code review assistance
void YourClass::ReviewGameCode(const FString& CodeSnippet, const FString& Context)
{
    FGenDSeekChatSettings ReviewSettings;
    ReviewSettings.Model = EDeepSeekModels::Reasoner;  // Use reasoning for thorough analysis
    ReviewSettings.MaxTokens = 3000;
    ReviewSettings.Temperature = 0.2f;  // Low temperature for objective analysis
    
    ReviewSettings.Messages.Add(FGenChatMessage{
        TEXT("system"), 
        TEXT("You are a senior Unreal Engine developer reviewing code. Analyze for:\n"
             "1. Performance implications\n"
             "2. Memory management\n"
             "3. Unreal Engine best practices\n"
             "4. Potential bugs\n"
             "5. Code maintainability\n"
             "Provide specific, actionable feedback.")
    });
    
    FString ReviewPrompt = FString::Printf(
        TEXT("Please review this Unreal Engine code:\n\n```cpp\n%s\n```\n\n"
             "Context: %s\n\n"
             "Focus on performance, correctness, and best practices."),
        *CodeSnippet, *Context
    );
    
    ReviewSettings.Messages.Add(FGenChatMessage{TEXT("user"), ReviewPrompt});
    
    UGenDSeekChat::SendChatRequest(ReviewSettings, OnCodeReviewReceived);
}
```

---

This comprehensive DeepSeek integration guide provides everything needed to leverage DeepSeek's advanced reasoning capabilities and cost-effective chat solutions in your Unreal Engine projects. Remember to configure HTTP timeouts appropriately for DeepSeek R1's longer reasoning processes.