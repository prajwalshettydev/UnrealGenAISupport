// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#pragma once

#include "CoreMinimal.h"
#include "Data/MiniMax/GenMiniMaxChatStructs.h"
#include "Engine/CancellableAsyncAction.h"
#include "GenMiniMaxChat.generated.h"

class IHttpRequest;

// Regular C++ delegate for native code
DECLARE_DELEGATE_ThreeParams(FOnMiniMaxChatCompletionResponse, const FString&, const FString&, bool);

// Blueprint async delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGenMiniMaxChatCompletionDelegate, const FString&, Response, const FString&, Error, bool, Success);

UCLASS()
class GENERATIVEAISUPPORT_API UGenMiniMaxChat : public UCancellableAsyncAction
{
    GENERATED_BODY()

public:
    // Static function for native C++
    static void SendChatRequest(const FGenMiniMaxChatSettings& ChatSettings, const FOnMiniMaxChatCompletionResponse& OnComplete);

    // Blueprint-callable function
    UPROPERTY(BlueprintAssignable)
    FGenMiniMaxChatCompletionDelegate OnComplete;

    // Blueprint latent function
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "GenAI")
    static UGenMiniMaxChat* RequestMiniMaxChat(UObject* WorldContextObject, const FGenMiniMaxChatSettings& ChatSettings);

private:
    FGenMiniMaxChatSettings ChatSettings;

    friend class FGenMiniMaxRequestTest;

    // Build the actual HTTP request separately so automation can inspect it without sending.
    static TSharedRef<IHttpRequest, ESPMode::ThreadSafe> CreateHttpRequest(const FGenMiniMaxChatSettings& ChatSettings, const FString& ApiKey);

    // Shared implementation
    static void MakeRequest(const FGenMiniMaxChatSettings& ChatSettings, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);
    static void ProcessResponse(const FString& ResponseStr, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);

protected:
    virtual void Activate() override;
};
