// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#pragma once

#include "CoreMinimal.h"
#include "Data/OpenAI/GenOAIModels.h"
#include "GenOAIChatStructs.generated.h"

UENUM(BlueprintType)
enum class EGenAIOpenAIReasoningEffort : uint8
{
	Default UMETA(DisplayName = "Default"),
	Minimal UMETA(DisplayName = "Minimal"),
	Low UMETA(DisplayName = "Low"),
	Medium UMETA(DisplayName = "Medium"),
	High UMETA(DisplayName = "High")
};

UENUM(BlueprintType)
enum class EGenAIOpenAIVerbosity : uint8
{
	Default UMETA(DisplayName = "Default"),
	Low UMETA(DisplayName = "Low"),
	Medium UMETA(DisplayName = "Medium"),
	High UMETA(DisplayName = "High")
};


USTRUCT(BlueprintType)
struct FMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString role;

    UPROPERTY(BlueprintReadWrite)
    FString content;
};

USTRUCT(BlueprintType)
struct FChoice
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FMessage message;
};

USTRUCT(BlueprintType)
struct FResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<FChoice> choices;

    UPROPERTY(BlueprintReadWrite)
    FString error;
};


USTRUCT(BlueprintType)
struct FGenChatMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI")
    FString Role = TEXT("user");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI")
    FString Content;
};

USTRUCT(BlueprintType)
struct FGenChatSettings
{
    GENERATED_BODY()

    // Model selection using enum
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI")
    EGenOAIChatModel ModelEnum = EGenOAIChatModel::GPT_35_Turbo;
    
    // Custom model name if ModelEnum is set to Custom
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI", meta = (EditCondition = "ModelEnum == EGenOAIChatModel::Custom", EditConditionHides))
    FString CustomModel;

    // Legacy field for C++ compatibility - will be populated automatically from ModelEnum or CustomModel
    UPROPERTY(BlueprintReadOnly, Category = "GenAI|OpenAI")
    FString Model = TEXT("gpt-3.5-turbo");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI")
    int32 MaxTokens = 10000;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI")
    float Temperature = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI")
    float TopP = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI")
    FString Stop;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI")
    TArray<FGenChatMessage> Messages;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI|GPT-5")
    EGenAIOpenAIReasoningEffort ReasoningEffort = EGenAIOpenAIReasoningEffort::Default;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI|GPT-5")
    EGenAIOpenAIVerbosity Verbosity = EGenAIOpenAIVerbosity::Default;

    // Helper function to ensure the Model field is correctly set from enum or custom value
    void UpdateModel()
    {
        if (ModelEnum == EGenOAIChatModel::Custom && !CustomModel.IsEmpty())
        {
            Model = CustomModel;
        }
        else
        {
            Model = UGenOAIModelUtils::ChatModelToString(ModelEnum);
        }
    }
};


/**
 * Structured Output Chat Settings
 */
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FGenOAIStructuredChatSettings
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GenAI")
    FGenChatSettings ChatSettings;

    // Use schema for structured outputs
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GenAI")
    bool bUseSchema = true;

    // name
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GenAI")
    FString Name;

    // JSON schema for structured outputs
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GenAI")
    FString SchemaJson;

};
