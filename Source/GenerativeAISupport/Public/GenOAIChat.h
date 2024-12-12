// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GenOAIChat.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGenChatCompletionDelegate, const FString&, ResponseContent, const FString&, ErrorMessage, bool, bSuccess);

UCLASS()
class GENERATIVEAISUPPORT_API UGenOAIChat : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UGenOAIChat();

    UPROPERTY(BlueprintAssignable)
    FGenChatCompletionDelegate Finished;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "GenAI")
    static UGenOAIChat* CallOpenAIChat(UObject* WorldContextObject, FGenChatSettings ChatSettings);

    virtual void Activate() override;

private:
    void OnResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    void AppendMessage(const FString& Role, const FString& Content);

    FGenChatSettings ChatSettings;
};
