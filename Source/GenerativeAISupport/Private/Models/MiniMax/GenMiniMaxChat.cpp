// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#include "Models/MiniMax/GenMiniMaxChat.h"
#include "Secure/GenSecureKey.h"
#include "Http.h"
#include "LatentActions.h"
#include "Data/GenAIOrgs.h"
#include "Data/MiniMax/GenMiniMaxChatStructs.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Utilities/GenGlobalDefinitions.h"

namespace
{
const TCHAR* GetMiniMaxModelId(EMiniMaxModels Model)
{
	// API identifiers must not depend on localized editor-only enum metadata.
	switch (Model)
	{
	case EMiniMaxModels::MiniMax_M3_1_Flash:
		return TEXT("MiniMax-M3.1-Flash");
	case EMiniMaxModels::MiniMax_M3:
		return TEXT("MiniMax-M3");
	case EMiniMaxModels::MiniMax_M2_7:
		return TEXT("MiniMax-M2.7");
	default:
		return TEXT("unknown");
	}
}
}

void UGenMiniMaxChat::SendChatRequest(const FGenMiniMaxChatSettings& ChatSettings, const FOnMiniMaxChatCompletionResponse& OnComplete)
{
	MakeRequest(ChatSettings, [OnComplete](const FString& Response, const FString& Error, bool Success)
	{
		if (OnComplete.IsBound())
		{
			OnComplete.Execute(Response, Error, Success);
		}
	});
}

UGenMiniMaxChat* UGenMiniMaxChat::RequestMiniMaxChat(UObject* WorldContextObject, const FGenMiniMaxChatSettings& ChatSettings)
{
	UGenMiniMaxChat* AsyncAction = NewObject<UGenMiniMaxChat>();
	AsyncAction->ChatSettings = ChatSettings;
	return AsyncAction;
}

void UGenMiniMaxChat::Activate()
{
	MakeRequest(ChatSettings, [this](const FString& Response, const FString& Error, bool Success)
	{
		OnComplete.Broadcast(Response, Error, Success);
		Cancel();
	});
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> UGenMiniMaxChat::CreateHttpRequest(
	const FGenMiniMaxChatSettings& ChatSettings, const FString& ApiKey)
{
	const TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject());
	JsonPayload->SetStringField(TEXT("model"), GetMiniMaxModelId(ChatSettings.Model));
	JsonPayload->SetNumberField(TEXT("max_tokens"), ChatSettings.MaxTokens);

	TArray<TSharedPtr<FJsonValue>> MessagesArray;
	for (const FGenMiniMaxMessage& Message : ChatSettings.Messages)
	{
		const TSharedPtr<FJsonObject> JsonMessage = MakeShareable(new FJsonObject());
		JsonMessage->SetStringField(TEXT("role"), Message.Role);
		JsonMessage->SetStringField(TEXT("content"), Message.Content);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(JsonMessage)));
	}
	JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);

	FString PayloadString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(TEXT("https://api.minimax.io/v1/chat/completions"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	HttpRequest->SetContentAsString(PayloadString);
	return HttpRequest;
}

void UGenMiniMaxChat::MakeRequest(const FGenMiniMaxChatSettings& ChatSettings,
                                  const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
	const FString ApiKey = UGenSecureKey::GetGenerativeAIApiKey(EGenAIOrgs::MiniMax);
	if (ApiKey.IsEmpty())
	{
		ResponseCallback(TEXT(""), TEXT("MiniMax API key not set"), false);
		return;
	}

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHttpRequest(ChatSettings, ApiKey);
	HttpRequest->OnProcessRequestComplete().BindLambda(
		[ResponseCallback](FHttpRequestPtr Request, const FHttpResponsePtr& Response, const bool bSuccess)
		{
			if (!bSuccess || !Response.IsValid())
			{
				ResponseCallback(TEXT(""), TEXT("Request failed"), false);
				UE_LOG(LogGenAI, Error, TEXT("MiniMax Request failed, Response code: %d"),
				       Response.IsValid() ? Response->GetResponseCode() : -1);
				return;
			}
			ProcessResponse(Response->GetContentAsString(), ResponseCallback);
		});

	HttpRequest->ProcessRequest();
}

void UGenMiniMaxChat::ProcessResponse(const FString& ResponseStr,
                                      const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);

	// Attempt to deserialize the JSON response
	if (TSharedPtr<FJsonObject> JsonObject; FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		if (JsonObject->HasField(TEXT("choices")))
		{
			if (TArray<TSharedPtr<FJsonValue>> ChoicesArray = JsonObject->GetArrayField(TEXT("choices")); ChoicesArray.
				Num() > 0)
			{
				if (const TSharedPtr<FJsonObject> FirstChoice = ChoicesArray[0]->AsObject(); FirstChoice.IsValid() &&
					FirstChoice->HasField(TEXT("message")))
				{
					if (const TSharedPtr<FJsonObject> MessageObject = FirstChoice->GetObjectField(TEXT("message"));
						MessageObject.IsValid() && MessageObject->HasField(TEXT("content")))
					{
						const FString Content = MessageObject->GetStringField(TEXT("content"));
						ResponseCallback(Content, TEXT(""), true);
						return;
					}
				}
			}
		}

		// Log unexpected JSON structure
		UE_LOG(LogGenAI, Error, TEXT("MiniMax: Unexpected JSON structure: %s"), *ResponseStr);
		ResponseCallback(TEXT(""), TEXT("Unexpected JSON structure"), false);
	}
	else
	{
		// Log JSON parsing failure
		UE_LOG(LogGenAI, Error, TEXT("MiniMax: Failed to parse JSON: %s"), *ResponseStr);
		ResponseCallback(TEXT(""), TEXT("Failed to parse JSON"), false);
	}
}
