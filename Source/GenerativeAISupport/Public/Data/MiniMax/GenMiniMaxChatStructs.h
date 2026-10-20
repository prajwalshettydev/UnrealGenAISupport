// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#pragma once

#include "CoreMinimal.h"
#include "Data/GenAIOrgs.h"
#include "GenMiniMaxChatStructs.generated.h"

// Data structure for MiniMax chat messages
USTRUCT(BlueprintType)
struct FGenMiniMaxMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|MiniMax")
    FString Role;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|MiniMax")
    FString Content;
};

// Settings for the MiniMax chat API request
USTRUCT(BlueprintType)
struct FGenMiniMaxChatSettings
{
    GENERATED_BODY()

    // Model selection using enum
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|MiniMax")
    EMiniMaxModels Model = EMiniMaxModels::MiniMax_M3_1_Flash;

    // Maximum tokens to generate
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|MiniMax")
    int32 MaxTokens = 2048;

    // Array of messages for the conversation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|MiniMax")
    TArray<FGenMiniMaxMessage> Messages;
};
