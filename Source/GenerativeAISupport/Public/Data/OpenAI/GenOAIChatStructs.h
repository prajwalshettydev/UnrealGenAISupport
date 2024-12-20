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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SystemMessage;

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	TArray<FGenChatMessage> Messages;

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	int32 MaxTokens;

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	FString Model;
    
};