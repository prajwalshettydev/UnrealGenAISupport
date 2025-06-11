// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#pragma once

#include "CoreMinimal.h"
#include "Data/XAI/GenXAIChatStructs.h"
#include "Engine/CancellableAsyncAction.h"
#include "GenXAIChat.generated.h"
#include "Interfaces/IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"

// Regular C++ delegate for native code
DECLARE_DELEGATE_ThreeParams(FOnXAIChatCompletionResponse, const FString&, const FString&, bool);

// Blueprint async delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGenXAIChatCompletionDelegate, const FString&, Response, const FString&,
											   Error, bool, Success);

UCLASS()
class GENERATIVEAISUPPORT_API UGenXAIChat : public UCancellableAsyncAction
{
	GENERATED_BODY()

  public:
	// Static function for native C++
	static void SendChatRequest(const FGenXAIChatSettings& ChatSettings,
								const FOnXAIChatCompletionResponse& OnComplete);

	// Blueprint-callable function
	UPROPERTY(BlueprintAssignable)
	FGenXAIChatCompletionDelegate OnComplete;

	// Blueprint latent function
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"),
			  Category = "GenAI")
	static UGenXAIChat* RequestXAIChat(UObject* WorldContextObject, const FGenXAIChatSettings& ChatSettings);

  private:
	FGenXAIChatSettings ChatSettings;

	// Shared implementation
	static void MakeRequest(const FGenXAIChatSettings& ChatSettings,
							const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);
	static void ProcessResponse(const FString& ResponseStr,
								const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);

  protected:
	virtual void Activate() override;
};
