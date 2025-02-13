// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms


#include "Models/OpenAI/GenOAIStructuredOpService.h"

#include "Http.h"
#include "Data/GenAIOrgs.h"
#include "Data/OpenAI/GenOAIChatStructs.h"
#include "Engine/Engine.h" // For GEngine logging
#include "Secure/GenSecureKey.h"
#include "Utilities/GenGlobalDefinitions.h"

void UGenOAIStructuredOpService::RequestStructuredOutput(const FGenOAIStructuredChatSettings& StructuredChatSettings, const FOnSchemaResponse& OnComplete)
{
    MakeRequest(
        StructuredChatSettings,
        [OnComplete](const FString& Response, const FString& Error, bool Success) {
            if (OnComplete.IsBound())
            {
                OnComplete.Execute(Response, Error, Success);
            }
        }
    );
}

UGenOAIStructuredOpService* UGenOAIStructuredOpService::RequestStructuredOutputLatent(UObject* WorldContextObject, const FGenOAIStructuredChatSettings& StructuredChatSettings)
{
    UGenOAIStructuredOpService* AsyncAction = NewObject<UGenOAIStructuredOpService>();
    AsyncAction->StructuredChatSettings = StructuredChatSettings;
    return AsyncAction;
}

void UGenOAIStructuredOpService::Activate()
{
    MakeRequest(
        StructuredChatSettings,
        [this](const FString& Response, const FString& Error, bool Success) {
            OnComplete.Broadcast(Response, Error, Success);
            Cancel();
        }
    );
}

void UGenOAIStructuredOpService::MakeRequest(const FGenOAIStructuredChatSettings& StructuredChatSettings, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
    FString ApiKey = UGenSecureKey::GetGenerativeAIApiKey(EGenAIOrgs::OpenAI);
    if (ApiKey.IsEmpty())
    {
        ResponseCallback(TEXT(""), TEXT("API key not set"), false);
        UE_LOG(LogGenAI, Error, TEXT("API key not set"));
        return;
    }

    // Create JSON payload
    TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject());
    JsonPayload->SetStringField(TEXT("model"), StructuredChatSettings.ChatSettings.Model);

    // Nested object for response_format
    TSharedPtr<FJsonObject> ResponseFormat = MakeShareable(new FJsonObject());
    ResponseFormat->SetStringField(TEXT("type"), TEXT("json_schema"));

    // Nested object for the actual schema
    TSharedPtr<FJsonObject> SchemaObject = MakeShareable(new FJsonObject());
    // Create a root object for the json_schema
    TSharedPtr<FJsonObject> RootSchemaObject = MakeShareable(new FJsonObject());
    RootSchemaObject->SetStringField(TEXT("name"), StructuredChatSettings.Name);
    
    TSharedRef<TJsonReader<>> SchemaReader = TJsonReaderFactory<>::Create(StructuredChatSettings.SchemaJson);
    if (!FJsonSerializer::Deserialize(SchemaReader, SchemaObject) || !SchemaObject.IsValid())
    {
        UE_LOG(LogGenAI, Error, TEXT("Failed to parse schema JSON: %s"), *StructuredChatSettings.SchemaJson);
        return;
    }
    RootSchemaObject->SetObjectField(TEXT("schema"), SchemaObject);

    ResponseFormat->SetObjectField(TEXT("json_schema"), RootSchemaObject);
    JsonPayload->SetObjectField(TEXT("response_format"), ResponseFormat);
    JsonPayload->SetNumberField(TEXT("max_completion_tokens"), StructuredChatSettings.ChatSettings.MaxTokens);
    //set messages field, and append "Generate Response in JSON only." to the prompt
    TArray<TSharedPtr<FJsonValue>> MessagesArray;
    for (const FGenChatMessage& Message : StructuredChatSettings.ChatSettings.Messages)
    {
        TSharedPtr<FJsonObject> JsonMessage = MakeShareable(new FJsonObject());
        JsonMessage->SetStringField(TEXT("role"), Message.Role);
        JsonMessage->SetStringField(TEXT("content"), Message.Content);

        if(Message.Role == TEXT("system"))
        {
            // in api documentation, it is mentioned that the system message should be appended with "Generate Response in JSON only."
            // todo, we can move this outside this scope, but that doesnt guarantee the callee will append the message
            JsonMessage->SetStringField(TEXT("content"), Message.Content + TEXT(" Generate Response in JSON only. Use proper JSON formatting and avoid introducing line breaks inside string values."));
        }
        
        MessagesArray.Add(MakeShareable(new FJsonValueObject(JsonMessage)));
    }
    JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);

    FString PayloadString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
    FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);

    // Create HTTP request
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    HttpRequest->SetContentAsString(PayloadString);
    
	//UE_LOG(LogGenAIVerbose, Log, TEXT("Sending chat request... Payload: %s"), *PayloadString);

    HttpRequest->OnProcessRequestComplete().BindLambda([ResponseCallback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess) {
        if (!bSuccess || !Response.IsValid())
        {
            ResponseCallback(TEXT(""), TEXT("Request failed"), false);
            UE_LOG(LogGenAI, Error, TEXT("Request failed, check your internet connection."));
            return;
        }
        ProcessResponse(Response->GetContentAsString(), ResponseCallback);
    });

    HttpRequest->ProcessRequest();
}


/**
 * \brief this function's rules are set according to the openAI refusal and content documentation for structured chat
 * link: https://platform.openai.com/docs/guides/structured-outputs?lang=python&context=ex4#json-mode
 * \param ResponseStr 
 * \param ResponseCallback 
 */
void UGenOAIStructuredOpService::ProcessResponse(const FString& ResponseStr, const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        if (JsonObject->HasField(TEXT("choices")))
        {
            if (TArray<TSharedPtr<FJsonValue>> ChoicesArray = JsonObject->GetArrayField(TEXT("choices")); ChoicesArray.Num() > 0)
            {
                if (const TSharedPtr<FJsonObject> FirstChoice = ChoicesArray[0]->AsObject(); FirstChoice.IsValid() && FirstChoice->HasField(TEXT("message")))
                {
                    if (const TSharedPtr<FJsonObject> MessageObject = FirstChoice->GetObjectField(TEXT("message")); MessageObject.IsValid())
                    {
                        if (MessageObject->HasField(TEXT("content")))
                        {
                            FString Content = MessageObject->GetStringField(TEXT("content"));
                            ResponseCallback(Content, TEXT(""), true);
						    //UE_LOG(LogGenAIVerbose, Log, TEXT("Chat response: %s"), *Content);
                            return;
                        }
                        else if (MessageObject->HasField(TEXT("refusal")))
                        {
                            const FString Refusal = MessageObject->GetStringField(TEXT("refusal"));
                            ResponseCallback(TEXT(""), Refusal, false);
						    //UE_LOG(LogGenAIVerbose, Log, TEXT("Chat response: %s"), *Refusal);
                            return;
                        }
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



