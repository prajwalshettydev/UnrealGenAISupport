// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms


#include "Models/OpenAI/GenOAIChat.h"
#include "Secure/GenSecureKey.h"
#include "Http.h"
#include "LatentActions.h"
#include "Data/GenAIOrgs.h"
#include "Data/OpenAI/GenOAIChatStructs.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Engine.h"  // For GEngine and screen logging
#include "Utilities/GenGlobalDefinitions.h"


void UGenOAIChat::SendChatRequest(const FGenChatSettings& ChatSettings, const FOnChatCompletionResponse& OnComplete)
{
	MakeRequest(ChatSettings, [OnComplete](const FString& Response, const FString& Error, bool Success)
	{
		// Explicitly state this is intended as an action
		if (OnComplete.IsBound())
		{
			OnComplete.Execute(Response, Error, Success);
		}
	});
}

UGenOAIChat* UGenOAIChat::SendRequestLatent(UObject* WorldContextObject, const FGenChatSettings& ChatSettings)
{
	UGenOAIChat* AsyncAction = NewObject<UGenOAIChat>();
	AsyncAction->ChatSettings = ChatSettings;
	return AsyncAction;
}

void UGenOAIChat::Activate()
{
	MakeRequest(ChatSettings, [this](const FString& Response, const FString& Error, bool Success)
	{
		OnComplete.Broadcast(Response, Error, Success);
		Cancel();
	});
}

void UGenOAIChat::MakeRequest(const FGenChatSettings& ChatSettings,
                              const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
	const FString ApiKey = UGenSecureKey::GetGenerativeAIApiKey(EGenAIOrgs::OpenAI);
	if (ApiKey.IsEmpty())
	{
		ResponseCallback(TEXT(""), TEXT("API key not set"), false);
		return;
	}

	const TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject());
	JsonPayload->SetStringField(TEXT("model"), ChatSettings.Model);
	JsonPayload->SetNumberField(TEXT("max_completion_tokens"), ChatSettings.MaxTokens);

	TArray<TSharedPtr<FJsonValue>> MessagesArray;
	for (const auto& [Role, Content] : ChatSettings.Messages)
	{
		const TSharedPtr<FJsonObject> JsonMessage = MakeShareable(new FJsonObject());
		JsonMessage->SetStringField(TEXT("role"), Role);
		JsonMessage->SetStringField(TEXT("content"), Content);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(JsonMessage)));
	}
	JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);

	FString PayloadString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	HttpRequest->SetContentAsString(PayloadString);

	//UE_LOG(LogGenAIVerbose, Log, TEXT("Sending chat request... Payload: %s"), *PayloadString);

	HttpRequest->OnProcessRequestComplete().BindLambda(
		[ResponseCallback](FHttpRequestPtr Request, const FHttpResponsePtr& Response, const bool bSuccess)
		{
			if (!bSuccess || !Response.IsValid())
			{
				ResponseCallback(TEXT(""), TEXT("Request failed"), false);
				UE_LOG(LogGenAI, Error, TEXT("Request failed, Response code: %d"),
				       Response.IsValid() ? Response->GetResponseCode() : -1);
				return;
			}
			ProcessResponse(Response->GetContentAsString(), ResponseCallback);
		});

	HttpRequest->ProcessRequest();
}

void UGenOAIChat::ProcessResponse(const FString& ResponseStr,
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
						//UE_LOG(LogGenAIVerbose, Log, TEXT("Chat response: %s"), *Content);
						return;
					}
				}
			}
		}

		// Log unexpected JSON structure
		UE_LOG(LogGenAI, Error, TEXT("Unexpected JSON structure: %s"), *ResponseStr);
		ResponseCallback(TEXT(""), TEXT("Unexpected JSON structure"), false);
	}
	else
	{
		// Log JSON parsing failure
		UE_LOG(LogGenAI, Error, TEXT("Failed to parse JSON: %s"), *ResponseStr);
		ResponseCallback(TEXT(""), TEXT("Failed to parse JSON"), false);
	}
}
