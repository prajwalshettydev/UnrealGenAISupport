// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenOAIChatStructs.generated.h"

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

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	FString Role;

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	FString Content;
};

USTRUCT(BlueprintType)
struct FGenChatSettings
{
	GENERATED_BODY()
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AgentID;

	// SystemMessage no longer needed, as it is embedded in the messages array
	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// FString SystemMessage;

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	TArray<FGenChatMessage> Messages;

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	int32 MaxTokens = 10000;

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	FString Model; //can be gpt-4o-mini, 
    
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