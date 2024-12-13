// Fill out your copyright notice in the Description page of Project Settings.

#include "Serialize/Legacy/GenManualSerializerHelper.h"
#include "Data/OpenAI/GenOAIChatStructs.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Engine.h"
#include "Interfaces/IHttpResponse.h"
#include "Utilities/GenGlobalDefinitions.h"


void UGenManualSerializerHelper::HandleJsonResponse(FHttpResponsePtr Response, FGenChatSettings& ChatSettings, FGenChatCompletionDelegateLegacy& Finished)
{
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
                        ChatSettings.Messages.Add(FGenChatMessage{TEXT("assistant"), Content});

                        Finished.Broadcast(Content, TEXT(""), true);
                        return;
                    }
                }
            }
        }
    }
}