// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#pragma once

#include "CoreMinimal.h"
#include "GenXAIChatStructs.generated.h"

// Data structure for XAI Grok chat messages
USTRUCT(BlueprintType)
struct FGenXAIMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI")
    FString Role;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI")
    FString Content;
};

// Settings for the XAI Grok chat API request
USTRUCT(BlueprintType)
struct FGenXAIChatSettings
{
    GENERATED_BODY()

    // Model name (e.g., "grok-3-latest")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI")
    FString Model = TEXT("grok-3-latest");

    // Maximum tokens to generate
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI")
    int32 MaxTokens = 2048;

    // Array of messages for the conversation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|XAI")
    TArray<FGenXAIMessage> Messages;
};
