// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#pragma once

#include "CoreMinimal.h"
#include "GenOAIModels.generated.h"

/**
 * Enum representing available OpenAI chat models
 */
UENUM(BlueprintType)
enum class EGenOAIChatModel : uint8
{
	GPT_35_Turbo UMETA(DisplayName = "GPT-3.5 Turbo"),
	GPT_4 UMETA(DisplayName = "GPT-4"),
	GPT_4_Turbo UMETA(DisplayName = "GPT-4 Turbo"),
	GPT_4_1 UMETA(DisplayName = "GPT-4.1"),
	GPT_4_1_Mini UMETA(DisplayName = "GPT-4.1 Mini"),
	GPT_4_1_Nano UMETA(DisplayName = "GPT-4.1 Nano"),
	GPT_4O_Mini UMETA(DisplayName = "GPT-4o Mini"),
	GPT_4O UMETA(DisplayName = "GPT-4o"),
	O3 UMETA(DisplayName = "GPT-o3"),
	O3_Mini UMETA(DisplayName = "GPT-o3 Mini"),
	O4_Mini UMETA(DisplayName = "GPT-o4 Mini"),
	Custom UMETA(DisplayName = "Custom Model")
};

/**
 * Enum representing available OpenAI Whisper models
 */
UENUM(BlueprintType)
enum class EGenOAIWhisperModel : uint8
{
	Whisper_1 UMETA(DisplayName = "Whisper-1"),
	Custom UMETA(DisplayName = "Custom Model")
};

/**
 * Utility class for OpenAI models
 */
UCLASS()
class GENERATIVEAISUPPORT_API UGenOAIModelUtils : public UObject
{
	GENERATED_BODY()

  public:
	/**
	 * Convert model enum to string representation
	 */
	UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI|Models")
	static FString ChatModelToString(EGenOAIChatModel Model)
	{
		switch (Model)
		{
			case EGenOAIChatModel::GPT_35_Turbo:
				return TEXT("gpt-3.5-turbo");
			case EGenOAIChatModel::GPT_4:
				return TEXT("gpt-4");
			case EGenOAIChatModel::GPT_4_Turbo:
				return TEXT("gpt-4-turbo");
			case EGenOAIChatModel::GPT_4_1:
				return TEXT("gpt-4.1");
			case EGenOAIChatModel::GPT_4_1_Mini:
				return TEXT("gpt-4.1-mini");
			case EGenOAIChatModel::GPT_4_1_Nano:
				return TEXT("gpt-4.1-nano");
			case EGenOAIChatModel::GPT_4O_Mini:
				return TEXT("gpt-4o-mini");
			case EGenOAIChatModel::GPT_4O:
				return TEXT("gpt-4o");
			case EGenOAIChatModel::O3:
				return TEXT("o3");
			case EGenOAIChatModel::O3_Mini:
				return TEXT("o3-mini");
			case EGenOAIChatModel::O4_Mini:
				return TEXT("o4-mini");
			case EGenOAIChatModel::Custom:
			default:
				return TEXT("");
		}
	}

	/**
	 * Convert Whisper model enum to string representation
	 */
	UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI|Models")
	static FString WhisperModelToString(EGenOAIWhisperModel Model)
	{
		switch (Model)
		{
			case EGenOAIWhisperModel::Whisper_1:
				return TEXT("whisper-1");
			case EGenOAIWhisperModel::Custom:
			default:
				return TEXT("");
		}
	}
};
