// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/GenAIOrgs.h"
#include "Engine/CancellableAsyncAction.h"
#include "GenDSeekChat.generated.h"

struct FGenChatMessage;
// Delegate for C++ callbacks
DECLARE_DELEGATE_ThreeParams(FOnDSeekChatCompletionResponse, const FString&, const FString&, bool);

// Blueprint async delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGenDSeekChatCompletionDelegate, const FString&, Response, const FString&, Error, bool, Success);

// Chat settings structure
USTRUCT(BlueprintType)
struct FGenDSeekChatSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "GenAI")
	EDeepSeekModels Model = EDeepSeekModels::Chat;

	UPROPERTY(BlueprintReadWrite, Category = "GenAI")
	int32 MaxTokens = 4096;

	UPROPERTY(BlueprintReadWrite, Category = "GenAI")
	TArray<FGenChatMessage> Messages;

	UPROPERTY(BlueprintReadWrite, Category = "GenAI")
	bool bStreamResponse = false;
};


/**
 * 
 */
UCLASS()
class GENERATIVEAISUPPORT_API UGenDSeekChat : public UCancellableAsyncAction
{
	GENERATED_BODY()
	
public:
	// Static function for native C++
	static void SendChatRequest(const FGenDSeekChatSettings& ChatSettings, const FOnDSeekChatCompletionResponse& OnComplete);

	// Blueprint async function
	UPROPERTY(BlueprintAssignable)
	FGenDSeekChatCompletionDelegate OnComplete;

	// Blueprint latent function
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "GenAI|DeepSeek")
	static UGenDSeekChat* SendRequestLatent(UObject* WorldContextObject, const FGenDSeekChatSettings& ChatSettings);

private:
	// Stores settings for request
	FGenDSeekChatSettings ChatSettings;

	// Internal request processing
	static void MakeRequest(const FGenDSeekChatSettings& ChatSettings, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);
	static void ProcessResponse(const FString& ResponseStr, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);

protected:
	virtual void Activate() override;
};
