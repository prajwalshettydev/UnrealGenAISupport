// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HttpModule.h" // For FHttpResponsePtr
#include "GenChatSettings.h" // For FGenChatSettings
#include "GenOAIChat.h" // For FOnChatCompletion
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GenManualSerializerHelper.generated.h"

/**
 * 
 */
UCLASS()
class GENERATIVEAISUPPORT_API UGenManualSerializerHelper : public UObject
{
	GENERATED_BODY()

public:
	static void HandleJsonResponse(FHttpResponsePtr Response, FGenChatSettings& ChatSettings, FOnChatCompletion& Finished);
};
