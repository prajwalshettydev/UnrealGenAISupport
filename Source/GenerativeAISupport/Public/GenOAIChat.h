// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GenOAIChat.generated.h"


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

    UPROPERTY(BlueprintReadWrite, Category = "Chat")
    TArray<FGenChatMessage> Messages;

    UPROPERTY(BlueprintReadWrite, Category = "Chat")
    int32 MaxTokens;

    UPROPERTY(BlueprintReadWrite, Category = "Chat")
    FString Model;
};

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
