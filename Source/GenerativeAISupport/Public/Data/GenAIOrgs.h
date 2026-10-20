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
	MiniMax     UMETA(DisplayName = "MiniMax"),
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

// Enum for selecting MiniMax models
UENUM(BlueprintType)
enum class EMiniMaxModels : uint8
{
	// Keep existing numeric values so saved Blueprint selections retain their model.
	MiniMax_M3_1_Flash = 3 UMETA(DisplayName = "MiniMax-M3.1-Flash"),
	MiniMax_M3 = 0         UMETA(DisplayName = "MiniMax-M3"),
	MiniMax_M2_7 = 1       UMETA(DisplayName = "MiniMax-M2.7", Hidden),
	Unknown = 2           UMETA(DisplayName = "unknown", Hidden)
};
