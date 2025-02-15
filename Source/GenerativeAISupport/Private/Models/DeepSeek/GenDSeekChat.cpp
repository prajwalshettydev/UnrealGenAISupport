// Fill out your copyright notice in the Description page of Project Settings.


#include "Models/DeepSeek/GenDSeekChat.h"

#include "HttpModule.h"
#include "Data/GenAIOrgs.h"
#include "Data/OpenAI/GenOAIChatStructs.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Secure/GenSecureKey.h"
#include "Utilities/GenUtils.h"


void UGenDSeekChat::SendChatRequest(const FGenDSeekChatSettings& ChatSettings,
                                    const FOnDSeekChatCompletionResponse& OnComplete)
{
	MakeRequest(ChatSettings, [OnComplete](const FString& Response, const FString& Error, bool Success)
	{
		if (OnComplete.IsBound())
		{
			OnComplete.Execute(Response, Error, Success);
		}
	});
}

UGenDSeekChat* UGenDSeekChat::SendRequestLatent(UObject* WorldContextObject, const FGenDSeekChatSettings& ChatSettings)
{
	UGenDSeekChat* AsyncAction = NewObject<UGenDSeekChat>();
	AsyncAction->ChatSettings = ChatSettings;
	return AsyncAction;
}

void UGenDSeekChat::Activate()
{
	MakeRequest(ChatSettings, [this](const FString& Response, const FString& Error, bool Success)
	{
		OnComplete.Broadcast(Response, Error, Success);
		Cancel();
	});
}

void UGenDSeekChat::MakeRequest(const FGenDSeekChatSettings& ChatSettings,
                                const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
	FString ApiKey = UGenSecureKey::GetGenerativeAIApiKey(EGenAIOrgs::DeepSeek);
	if (ApiKey.IsEmpty())
	{
		ResponseCallback(TEXT(""), TEXT("DeepSeek API key not set"), false);
		return;
	}


	// Construct JSON payload
	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject());
	JsonPayload->SetStringField(
		TEXT("model"),
		UGenUtils::GetEnumDisplayName(StaticEnum<EDeepSeekModels>(), static_cast<int32>(ChatSettings.Model)));
	JsonPayload->SetNumberField(TEXT("max_tokens"), ChatSettings.MaxTokens);
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
	HttpRequest->SetTimeout(180.0f); // Set 180 seconds timeout
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(TEXT("https://api.deepseek.com/chat/completions"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	HttpRequest->SetContentAsString(PayloadString);
	FHttpModule::Get().UpdateConfigs(); // Apply changes

	// UE_LOG(LogTemp, Warning, TEXT("Unreal HTTP Timeouts: Total: %f, Connection: %f"),
	// FHttpModule::Get().GetHttpTotalTimeout(),
	// FHttpModule::Get().GetHttpConnectionTimeout());
	
	UE_LOG(LogTemp, Log, TEXT("Payload: %s"), *PayloadString);

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
				UE_LOG(LogTemp, Error, TEXT("DeepSeek API request failed. HTTP Code: %d, Error: %s"), ResponseCode, *ErrorMessage);

				ResponseCallback(TEXT(""), ErrorMessage, false);
				return;
			}

			ProcessResponse(Response->GetContentAsString(), ResponseCallback);
		});
	HttpRequest->ProcessRequest();
}


void UGenDSeekChat::ProcessResponse(const FString& ResponseStr,
                                    const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		if (JsonObject->HasField(TEXT("choices")))
		{
			const TArray<TSharedPtr<FJsonValue>>& Choices = JsonObject->GetArrayField(TEXT("choices"));
			if (Choices.Num() > 0 && Choices[0]->AsObject()->HasField(TEXT("message")))
			{
				const TSharedPtr<FJsonObject> Message = Choices[0]->AsObject()->GetObjectField(TEXT("message"));
				FString Content = Message->GetStringField(TEXT("content"));

				// If using deepseek-reasoner, extract reasoning content as well
				if (Message->HasField(TEXT("reasoning_content")))
				{
					FString ReasoningContent = Message->GetStringField(TEXT("reasoning_content"));
					Content += TEXT("\n\nReasoning:\n") + ReasoningContent;
				}

				ResponseCallback(Content, TEXT(""), true);
				return;
			}
		}
	}

	ResponseCallback(TEXT(""), TEXT("Invalid response"), false);
}
