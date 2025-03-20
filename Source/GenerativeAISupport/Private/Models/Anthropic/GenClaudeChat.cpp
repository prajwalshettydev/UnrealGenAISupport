// Fill out your copyright notice in the Description page of Project Settings.


#include "Models/Anthropic/GenClaudeChat.h"

#include "HttpModule.h"
#include "Data/GenAIOrgs.h"
#include "Data/OpenAI/GenOAIChatStructs.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Secure/GenSecureKey.h"
#include "Utilities/GenUtils.h"


void UGenClaudeChat::SendChatRequest(const FGenClaudeChatSettings& ChatSettings, const FOnClaudeChatCompletionResponse& OnComplete)
{
    MakeRequest(ChatSettings, [OnComplete](const FString& Response, const FString& Error, bool Success)
    {
        if (OnComplete.IsBound())
        {
            OnComplete.Execute(Response, Error, Success);
        }
    });
}

UGenClaudeChat* UGenClaudeChat::RequestClaudeChat(UObject* WorldContextObject, const FGenClaudeChatSettings& ChatSettings)
{
    UGenClaudeChat* AsyncAction = NewObject<UGenClaudeChat>();
    AsyncAction->ChatSettings = ChatSettings;
    return AsyncAction;
}

void UGenClaudeChat::Activate()
{
    MakeRequest(ChatSettings, [this](const FString& Response, const FString& Error, bool Success)
    {
        if (OnComplete.IsBound())
        {
            OnComplete.Broadcast(Response, Error, Success);
        }
        Cancel();
    });
}

void UGenClaudeChat::MakeRequest(const FGenClaudeChatSettings& ChatSettings, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
    FString ApiKey = UGenSecureKey::GetGenerativeAIApiKey(EGenAIOrgs::Anthropic);
    if (ApiKey.IsEmpty())
    {
        ResponseCallback(TEXT(""), TEXT("Anthropic API key not set"), false);
        return;
    }

    // Construct JSON payload
    TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject());
    JsonPayload->SetStringField(
        TEXT("model"),
        UGenUtils::GetEnumDisplayName(StaticEnum<EClaudeModels>(), static_cast<int32>(ChatSettings.Model)));
    JsonPayload->SetNumberField(TEXT("max_tokens"), ChatSettings.MaxTokens);
    JsonPayload->SetNumberField(TEXT("temperature"), ChatSettings.Temperature);
    JsonPayload->SetBoolField(TEXT("stream"), ChatSettings.bStreamResponse);

    TArray<TSharedPtr<FJsonValue>> MessagesArray;
    for (const FGenChatMessage& Message : ChatSettings.Messages)
    {
        TSharedPtr<FJsonObject> JsonMessage = MakeShareable(new FJsonObject());
        JsonMessage->SetStringField(TEXT("role"), Message.Role);
        JsonMessage->SetStringField(TEXT("content"), Message.Content);
        MessagesArray.Add(MakeShareable(new FJsonValueObject(JsonMessage)));
    }
    JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);

    FString PayloadString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
    FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);

    // Create HTTP request
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetTimeout(180.0f);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetURL(TEXT("https://api.anthropic.com/v1/messages"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("x-api-key"), ApiKey);
    HttpRequest->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
    HttpRequest->SetContentAsString(PayloadString);
    
    UE_LOG(LogTemp, Log, TEXT("Claude API Request: %s"), *PayloadString);

    HttpRequest->OnProcessRequestComplete().BindLambda(
        [ResponseCallback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            if (!bSuccess || !Response.IsValid())
            {
                FString ErrorMessage = Response.IsValid() ? Response->GetContentAsString() : TEXT("Request failed. No response received.");
                int32 ResponseCode = Response.IsValid() ? Response->GetResponseCode() : -1;
                if(ResponseCode == 0)
                {
                    ErrorMessage = TEXT("Request most likely timed out. No response received.");
                }
                UE_LOG(LogTemp, Error, TEXT("Claude API request failed. HTTP Code: %d, Error: %s"), ResponseCode, *ErrorMessage);

                ResponseCallback(TEXT(""), ErrorMessage, false);
                return;
            }

            ProcessResponse(Response->GetContentAsString(), ResponseCallback);
        });
    
    HttpRequest->ProcessRequest();
}

void UGenClaudeChat::ProcessResponse(const FString& ResponseStr, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        if (JsonObject->HasField(TEXT("content")))
        {
            const TArray<TSharedPtr<FJsonValue>>* ContentArray;
            if (JsonObject->TryGetArrayField(TEXT("content"), ContentArray) && ContentArray->Num() > 0)
            {
                const TSharedPtr<FJsonObject>* ContentObj;
                if ((*ContentArray)[0]->TryGetObject(ContentObj) && ContentObj->IsValid() && (*ContentObj)->HasField(TEXT("text")))
                {
                    FString Content = (*ContentObj)->GetStringField(TEXT("text"));
                    ResponseCallback(Content, TEXT(""), true);
                    return;
                }
            }
        }
        else if (JsonObject->HasField(TEXT("error")))
        {
            TSharedPtr<FJsonObject> ErrorObj = JsonObject->GetObjectField(TEXT("error"));
            FString ErrorMessage = ErrorObj->HasField(TEXT("message"))
                ? ErrorObj->GetStringField(TEXT("message"))
                : TEXT("Unknown error from Claude API");
            ResponseCallback(TEXT(""), ErrorMessage, false);
            return;
        }
    }

    ResponseCallback(TEXT(""), TEXT("Invalid response format from Claude API"), false);
}
