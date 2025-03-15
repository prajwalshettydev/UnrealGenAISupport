// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/OpenAI/GenOAIChatStructs.h"
#include "UObject/Object.h"
#include "GenClaudeChatStructs.generated.h"



UENUM(BlueprintType)
enum class EClaudeModels : uint8
{
	Claude_3_7_Sonnet UMETA(DisplayName = "claude-3-7-sonnet-latest"),
	Claude_3_5_Sonnet UMETA(DisplayName = "claude-3-5-sonnet"),
	Claude_3_5_Haiku UMETA(DisplayName = "claude-3-5-haiku-latest"),
	Claude_3_Opus UMETA(DisplayName = "claude-3-opus-latest")
};

USTRUCT(BlueprintType)
struct FGenClaudeChatSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API")
	EClaudeModels Model = EClaudeModels::Claude_3_7_Sonnet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API")
	int32 MaxTokens = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API")
	float Temperature = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API")
	bool bStreamResponse = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Claude API")
	TArray<FGenChatMessage> Messages;
};

/**
 * 
 */
UCLASS()
class GENERATIVEAISUPPORT_API UGenClaudeChatStructs : public UObject
{
	GENERATED_BODY()
};
