// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Anthropic/GenClaudeChatStructs.h"
#include "Engine/CancellableAsyncAction.h"
#include "UObject/Object.h"
#include "GenClaudeChat.generated.h"



// Delegate for C++ callbacks
DECLARE_DELEGATE_ThreeParams(FOnClaudeChatCompletionResponse, const FString&, const FString&, bool);

// Blueprint async delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGenClaudeChatCompletionDelegate, const FString&, Response, const FString&, Error, bool, Success);

/**
 * 
 */
UCLASS()
class GENERATIVEAISUPPORT_API UGenClaudeChat : public UCancellableAsyncAction
{
	GENERATED_BODY()
    
public:
	// Static function for native C++
	static void SendChatRequest(const FGenClaudeChatSettings& ChatSettings, const FOnClaudeChatCompletionResponse& OnComplete);

	// Blueprint async function
	UPROPERTY(BlueprintAssignable)
	FGenClaudeChatCompletionDelegate OnComplete;

	// Blueprint latent function
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "GenAI|Claude")
	static UGenClaudeChat* RequestClaudeChat(UObject* WorldContextObject, const FGenClaudeChatSettings& ChatSettings);

private:
	// Stores settings for request
	FGenClaudeChatSettings ChatSettings;

	// Internal request processing
	static void MakeRequest(const FGenClaudeChatSettings& ChatSettings, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);
	static void ProcessResponse(const FString& ResponseStr, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);

protected:
	virtual void Activate() override;
};
