// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Data/OpenAI/GenOAIChat.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GenManualSerializerHelper.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGenChatCompletionDelegateLegacy, const FString&, ResponseContent, const FString&, ErrorMessage, bool, bSuccess);

/**
 * 
 */
UCLASS()
class GENERATIVEAISUPPORT_API UGenManualSerializerHelper : public UObject
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintAssignable)
	FGenChatCompletionDelegateLegacy Finished;
	
	static void HandleJsonResponse(FHttpResponsePtr Response, FGenChatSettings& ChatSettings, FGenChatCompletionDelegateLegacy& Finished);
};
