// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#pragma once

#include "CoreMinimal.h"
#include "Data/OpenAI/GenOAIChatStructs.h"
#include "Engine/CancellableAsyncAction.h"
#include "Interfaces/IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GenOAIChat.generated.h"


// Note: this is a Regular C++ only delegate,
// why? because for blueprint we are using a unreal latent function, which inturn uses the MULTICAST_DELEGATE below
// and why Static delegate instead of the usual Dynamic Delegate? because the static delegate supports lambdas better and integrates to native c++ well,
// this does however remove the functionality of unreal reflection system, but we don't need that here, as the blueprint latent function will handle that
DECLARE_DELEGATE_ThreeParams(FOnChatCompletionResponse, const FString&, const FString&, bool);

// Blueprint async delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGenChatCompletionDelegate, const FString&, Response, const FString&, Error, bool, Success);

UCLASS()
class GENERATIVEAISUPPORT_API UGenOAIChat : public UCancellableAsyncAction
{
    GENERATED_BODY()

public:
    // Static function for native C++
    static void SendChatRequest(const FGenChatSettings& ChatSettings, const FOnChatCompletionResponse& OnComplete);

    // Blueprint-callable function
    UPROPERTY(BlueprintAssignable)
    FGenChatCompletionDelegate OnComplete;

    // Blueprint latent function
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "GenAI")
    static UGenOAIChat* SendRequestLatent(UObject* WorldContextObject, const FGenChatSettings& ChatSettings);

private:
    FGenChatSettings ChatSettings;

    // Shared implementation
    static void MakeRequest(const FGenChatSettings& ChatSettings, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);
    static void ProcessResponse(const FString& ResponseStr, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);

protected:
    virtual void Activate() override;
};
