// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GenAIOrgs.generated.h"


UENUM(BlueprintType)
enum class EGenAIOrgs : uint8
{
	OpenAI      UMETA(DisplayName = "OpenAI"),
	DeepSeek    UMETA(DisplayName = "DeepSeek"),
	Anthropic   UMETA(DisplayName = "Anthropic"),
	Meta        UMETA(DisplayName = "Meta"),
	Google      UMETA(DisplayName = "Google"),
	XAI         UMETA(DisplayName = "XAI"),
	Unknown     UMETA(DisplayName = "Unknown")
};

// Enum for selecting DeepSeek models
UENUM(BlueprintType)
enum class EDeepSeekModels : uint8
{
	Chat        UMETA(DisplayName = "deepseek-chat"),
	Reasoner    UMETA(DisplayName = "deepseek-reasoner"),
	Unknown     UMETA(DisplayName = "unknown")
};
