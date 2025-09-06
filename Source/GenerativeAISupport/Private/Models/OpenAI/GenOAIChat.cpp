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


TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> UGenOAIChat::SendChatRequest(const FGenChatSettings& ChatSettings, const FOnChatCompletionResponse& OnComplete)
{
	check(OnComplete.IsBound());
	return MakeRequest(ChatSettings, [OnComplete](const FString& Response, const FString& Error, bool Success)
	{
		OnComplete.Execute(Response, Error, Success);
	});
}

UGenOAIChat* UGenOAIChat::RequestOpenAIChat(UObject* WorldContextObject, const FGenChatSettings& ChatSettings)
{
	UGenOAIChat* AsyncAction = NewObject<UGenOAIChat>();
	AsyncAction->ChatSettings = ChatSettings;
	AsyncAction->RegisterWithGameInstance(WorldContextObject);
	return AsyncAction;
}

void UGenOAIChat::Activate()
{
	TWeakObjectPtr<UGenOAIChat> WeakThis(this);
	HttpRequest = MakeRequest(ChatSettings, [WeakThis](const FString& Response, const FString& Error, bool Success)
	{
		if (WeakThis.IsValid())
		{
			UGenOAIChat* StrongThis = WeakThis.Get();
			StrongThis->OnComplete.Broadcast(Response, Error, Success);
			StrongThis->Cancel();
		}
	});
}

void UGenOAIChat::Cancel()
{
	if (HttpRequest.IsValid() && HttpRequest->GetStatus() == EHttpRequestStatus::Processing)
	{
		HttpRequest->CancelRequest();
	}
	Super::Cancel();
}

TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> UGenOAIChat::MakeRequest(const FGenChatSettings& ChatSettings,
                              const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
	const FString ApiKey = UGenSecureKey::GetGenerativeAIApiKey(EGenAIOrgs::OpenAI);
	if (ApiKey.IsEmpty())
	{
		ResponseCallback(TEXT(""), TEXT("API key not set"), false);
		return nullptr;
	}

	// Make a mutable copy so we can update the model
	FGenChatSettings MutableSettings = ChatSettings;
	MutableSettings.UpdateModel();

	const TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject());
	JsonPayload->SetStringField(TEXT("model"), MutableSettings.Model);
	JsonPayload->SetNumberField(TEXT("max_completion_tokens"), MutableSettings.MaxTokens);
	JsonPayload->SetNumberField(TEXT("temperature"), MutableSettings.Temperature);
	JsonPayload->SetNumberField(TEXT("top_p"), MutableSettings.TopP);
	if (!MutableSettings.Stop.IsEmpty())
	{
		JsonPayload->SetStringField(TEXT("stop"), MutableSettings.Stop);
	}
	
	if (MutableSettings.ReasoningEffort != EGenAIOpenAIReasoningEffort::Default)
	{
		const FString ReasoningEffortString = StaticEnum<EGenAIOpenAIReasoningEffort>()->GetNameStringByValue(static_cast<int64>(MutableSettings.ReasoningEffort));
		JsonPayload->SetStringField(TEXT("reasoning_effort"), ReasoningEffortString.ToLower());
	}

	if (MutableSettings.Verbosity != EGenAIOpenAIVerbosity::Default)
	{
		const FString VerbosityString = StaticEnum<EGenAIOpenAIVerbosity>()->GetNameStringByValue(static_cast<int64>(MutableSettings.Verbosity));
		JsonPayload->SetStringField(TEXT("verbosity"), VerbosityString.ToLower());
	}

	TArray<TSharedPtr<FJsonValue>> MessagesArray;
	for (const auto& [Role, Content] : MutableSettings.Messages)
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
	return HttpRequest;
}

void UGenOAIChat::ProcessResponse(const FString& ResponseStr,
                                  const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
	TSharedPtr<FJsonObject> JsonObject;
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
		if (JsonObject->TryGetArrayField(TEXT("choices"), ChoicesArray) && ChoicesArray->Num() > 0)
		{
			const TSharedPtr<FJsonObject>* FirstChoiceObject;
			if ((*ChoicesArray)[0]->TryGetObject(FirstChoiceObject))
			{
				const TSharedPtr<FJsonObject>* MessageObject;
				if ((*FirstChoiceObject)->TryGetObjectField(TEXT("message"), MessageObject))
				{
					FString Content;
					if ((*MessageObject)->TryGetStringField(TEXT("content"), Content))
					{
						ResponseCallback(Content, TEXT(""), true);
						return;
					}
				}
			}
		}

		FString ErrorMessage;
		const TSharedPtr<FJsonObject>* ErrorObject;
		if (JsonObject->TryGetObjectField(TEXT("error"), ErrorObject))
		{
			if ((*ErrorObject)->TryGetStringField(TEXT("message"), ErrorMessage))
			{
				ResponseCallback(TEXT(""), ErrorMessage, false);
				return;
			}
		}
	}

	ResponseCallback(TEXT(""), FString::Printf(TEXT("Failed to parse response: %s"), *ResponseStr), false);
}
