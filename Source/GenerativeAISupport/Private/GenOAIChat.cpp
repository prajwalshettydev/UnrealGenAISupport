// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms


#include "GenOAIChat.h"
#include "GenSecureKey.h"
#include "Http.h"
#include "Data/OpenAI/GenOAIChatStructs.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Engine.h"  // For GEngine and screen logging
#include "Utilities/GenGlobalDefinitions.h"

UGenOAIChat::UGenOAIChat()
{
}


UGenOAIChat* UGenOAIChat::CallOpenAIChat(UObject* WorldContextObject, FGenChatSettings ChatSettings)
{
    UGenOAIChat* ChatNode = NewObject<UGenOAIChat>();
    ChatNode->ChatSettings = ChatSettings;
    return ChatNode;
}

void UGenOAIChat::Activate()
{
    FString ApiKey = UGenSecureKey::GetGenerativeAIApiKey();

    if (ApiKey.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("API key is not set"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("API key is not set"));
        }
        Finished.Broadcast(TEXT(""), TEXT("API key is not set"), false);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Preparing to send request to OpenAI API"));
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Sending request to OpenAI API"));
    }

    TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject());
    JsonPayload->SetStringField(TEXT("model"), ChatSettings.Model);
    JsonPayload->SetNumberField(TEXT("max_completion_tokens"), ChatSettings.MaxTokens);

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

    UE_LOG(LogTemp, Log, TEXT("Payload: %s"), *PayloadString);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Payload: %s"), *PayloadString));
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    HttpRequest->SetContentAsString(PayloadString);

    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UGenOAIChat::OnResponse);
    HttpRequest->ProcessRequest();
}

void UGenOAIChat::OnResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogGenAI, Error, TEXT("Failed to get response from server"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to get response from server"));
        }
        Finished.Broadcast(TEXT(""), TEXT("Failed to get response from server"), false);
        return;
    }

    UE_LOG(LogGenAI, Log, TEXT("Received response: %s"), *Response->GetContentAsString());
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Received response: %s"), *Response->GetContentAsString()));
    }

    TSharedPtr<FJsonObject> JsonResponse;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
    if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
    {
        if (JsonResponse->HasField("error"))
        {
            FString ErrorMessage = JsonResponse->GetStringField("error");
            UE_LOG(LogGenAI, Error, TEXT("API Error: %s"), *ErrorMessage);
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("API Error: %s"), *ErrorMessage));
            }
            Finished.Broadcast(TEXT(""), ErrorMessage, false);
        }
        else if (JsonResponse->HasField("choices"))
        {
            const TArray<TSharedPtr<FJsonValue>>* Choices;
            if (JsonResponse->TryGetArrayField(TEXT("choices"), Choices) && Choices->Num() > 0)
            {
                TSharedPtr<FJsonObject> FirstChoice = (*Choices)[0]->AsObject();
                if (FirstChoice.IsValid() && FirstChoice->HasField("message"))
                {
                    TSharedPtr<FJsonObject> MessageObject = FirstChoice->GetObjectField("message");
                    if (MessageObject.IsValid() && MessageObject->HasField("content"))
                    {
                        FString Content = MessageObject->GetStringField("content");

                        UE_LOG(LogGenAI, Log, TEXT("Assistant Response: %s"), *Content);
                        if (GEngine)
                        {
                            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Assistant Response: %s"), *Content));
                        }

                        // Append assistant response to chat history
                        AppendMessage(TEXT("assistant"), Content);

                        Finished.Broadcast(Content, TEXT(""), true);
                        return;
                    }
                }
            }
        }
    }

    UE_LOG(LogGenAI, Error, TEXT("Unexpected response format"));
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Unexpected response format"));
    }
    Finished.Broadcast(TEXT(""), TEXT("Unexpected response format"), false);
}

void UGenOAIChat::AppendMessage(const FString& Role, const FString& Content)
{
    FGenChatMessage NewMessage;
    NewMessage.Role = Role;
    NewMessage.Content = Content;
    ChatSettings.Messages.Add(NewMessage);

    UE_LOG(LogGenAI, Log, TEXT("Appended message - Role: %s, Content: %s"), *Role, *Content);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("Appended message - Role: %s, Content: %s"), *Role, *Content));
    }
}
