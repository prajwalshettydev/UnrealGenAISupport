// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#pragma once

#include "CoreMinimal.h"
#include "Data/OpenAI/GenOAIChatStructs.h"
#include "Engine/CancellableAsyncAction.h"
#include "GenOAIStructuredOpService.generated.h"

// Static delegate for native C++ usage
DECLARE_DELEGATE_ThreeParams(FOnSchemaResponse, const FString&, const FString&, bool);

// Blueprint async delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGenSchemaResponseDelegate, const FString&, Response, const FString&, Error, bool, Success);


/**
 * 
 */
UCLASS()
class GENERATIVEAISUPPORT_API UGenOAIStructuredOpService : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	// Static function for native C++
	static void RequestStructuredOutput(const FGenOAIStructuredChatSettings& StructuredChatSettings, const FOnSchemaResponse& OnComplete);

	// Blueprint async function
	UPROPERTY(BlueprintAssignable)
	FGenSchemaResponseDelegate OnComplete;

	// Blueprint latent function
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "GenAI")
	static UGenOAIStructuredOpService* RequestStructuredOutputLatent(UObject* WorldContextObject, const FGenOAIStructuredChatSettings& StructuredChatSettings);

private:
	FString Prompt;
	FString SchemaJson;
	FGenOAIStructuredChatSettings StructuredChatSettings;

	static void MakeRequest(const FGenOAIStructuredChatSettings& StructuredChatSettings, const TFunction<void(const FString&, const FString&, bool)
	                        >& ResponseCallback);
	static void ProcessResponse(const FString& ResponseStr, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);

protected:
	virtual void Activate() override;
};