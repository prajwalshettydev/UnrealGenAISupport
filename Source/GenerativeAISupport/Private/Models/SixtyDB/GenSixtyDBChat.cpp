// 60db.ai chat client implementation. Lifts the shape of GenOAIChat.cpp
// verbatim — same IHttpRequest lifecycle, same WeakObjectPtr safety pattern.

#include "Models/SixtyDB/GenSixtyDBChat.h"

#include "Data/GenAIOrgs.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Http.h"
#include "Secure/GenSecureKey.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Utilities/GenGlobalDefinitions.h"


static const FString SIXTYDB_CHAT_URL = TEXT("https://api.60db.ai/v1/chat/completions");


TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> UGenSixtyDBChat::SendChatRequest(
    const FGenSixtyDBChatSettings& ChatSettings,
    const FOnSixtyDBChatResponse& OnComplete)
{
    check(OnComplete.IsBound());
    return MakeRequest(ChatSettings, [OnComplete](const FString& Response, const FString& Error, bool Success)
    {
        OnComplete.Execute(Response, Error, Success);
    });
}


UGenSixtyDBChat* UGenSixtyDBChat::RequestSixtyDBChat(UObject* WorldContextObject,
                                                     const FGenSixtyDBChatSettings& ChatSettings)
{
    UGenSixtyDBChat* AsyncAction = NewObject<UGenSixtyDBChat>();
    AsyncAction->ChatSettings = ChatSettings;
    AsyncAction->RegisterWithGameInstance(WorldContextObject);
    return AsyncAction;
}


void UGenSixtyDBChat::Activate()
{
    TWeakObjectPtr<UGenSixtyDBChat> WeakThis(this);
    HttpRequest = MakeRequest(ChatSettings, [WeakThis](const FString& Response, const FString& Error, bool Success)
    {
        if (WeakThis.IsValid())
        {
            UGenSixtyDBChat* StrongThis = WeakThis.Get();
            StrongThis->OnComplete.Broadcast(Response, Error, Success);
            StrongThis->Cancel();
        }
    });
}


void UGenSixtyDBChat::Cancel()
{
    if (HttpRequest.IsValid() && HttpRequest->GetStatus() == EHttpRequestStatus::Processing)
    {
        HttpRequest->CancelRequest();
    }
    Super::Cancel();
}


TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> UGenSixtyDBChat::MakeRequest(
    const FGenSixtyDBChatSettings& ChatSettings,
    const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
    const FString ApiKey = UGenSecureKey::GetGenerativeAIApiKey(EGenAIOrgs::SixtyDB);
    if (ApiKey.IsEmpty())
    {
        ResponseCallback(TEXT(""), TEXT("60db API key not set (SIXTYDB_API_KEY or PS_SIXTYDBAPIKEY)"), false);
        return nullptr;
    }

    // Build messages array.
    TArray<TSharedPtr<FJsonValue>> JsonMessages;
    JsonMessages.Reserve(ChatSettings.Messages.Num());
    for (const FGenChatMessage& Msg : ChatSettings.Messages)
    {
        TSharedPtr<FJsonObject> JsonMsg = MakeShareable(new FJsonObject());
        JsonMsg->SetStringField(TEXT("role"), Msg.Role);
        JsonMsg->SetStringField(TEXT("content"), Msg.Content);
        JsonMessages.Add(MakeShareable(new FJsonValueObject(JsonMsg)));
    }

    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    Payload->SetStringField(TEXT("model"), ChatSettings.Model.IsEmpty() ? TEXT("60db-tiny") : ChatSettings.Model);
    Payload->SetArrayField(TEXT("messages"), JsonMessages);
    Payload->SetNumberField(TEXT("max_tokens"), ChatSettings.MaxTokens);
    Payload->SetNumberField(TEXT("temperature"), ChatSettings.Temperature);
    Payload->SetNumberField(TEXT("top_k"), ChatSettings.TopK);
    Payload->SetBoolField(TEXT("stream"), ChatSettings.bStream);
    Payload->SetBoolField(TEXT("save_chat"), ChatSettings.bSaveChat);
    if (ChatSettings.bSaveChat && !ChatSettings.ChatId.IsEmpty())
    {
        Payload->SetStringField(TEXT("chat_id"), ChatSettings.ChatId);
    }

    FString PayloadStr;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadStr);
    FJsonSerializer::Serialize(Payload.ToSharedRef(), Writer);

    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(SIXTYDB_CHAT_URL);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Accept"),
                       ChatSettings.bStream ? TEXT("text/event-stream") : TEXT("application/json"));
    Request->SetContentAsString(PayloadStr);

    const bool bStream = ChatSettings.bStream;
    Request->OnProcessRequestComplete().BindLambda(
        [ResponseCallback, bStream](FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bConnectedOk)
    {
        if (!bConnectedOk || !Response.IsValid())
        {
            ResponseCallback(TEXT(""), TEXT("60db chat: HTTP connection failed"), false);
            return;
        }
        if (Response->GetResponseCode() != 200)
        {
            const FString Body = Response->GetContentAsString();
            ResponseCallback(TEXT(""),
                FString::Printf(TEXT("60db chat HTTP %d: %s"),
                                Response->GetResponseCode(),
                                *Body.Left(200)),
                false);
            return;
        }
        ProcessResponse(Response->GetContentAsString(), bStream, ResponseCallback);
    });

    Request->ProcessRequest();
    return Request;
}


void UGenSixtyDBChat::ProcessResponse(const FString& ResponseStr,
                                      bool bWasStreamRequested,
                                      const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback)
{
    if (bWasStreamRequested)
    {
        // SSE — concatenated `data: {...}` lines separated by \n\n, terminated
        // by `data: [DONE]`. We didn't push deltas to BP as they arrived (BP
        // async actions are one-shot), so reconstruct the full text here.
        FString Assembled;
        TArray<FString> Frames;
        ResponseStr.ParseIntoArray(Frames, TEXT("\n\n"), /*bCullEmpty=*/true);
        for (const FString& Frame : Frames)
        {
            TArray<FString> Lines;
            Frame.ParseIntoArray(Lines, TEXT("\n"), true);
            for (const FString& Line : Lines)
            {
                if (!Line.StartsWith(TEXT("data:"))) continue;
                FString Payload = Line.Mid(5).TrimStart();
                if (Payload.IsEmpty() || Payload == TEXT("[DONE]")) continue;

                TSharedPtr<FJsonObject> Json;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Payload);
                if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid()) continue;

                const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
                if (!Json->TryGetArrayField(TEXT("choices"), Choices) || Choices->Num() == 0) continue;

                const TSharedPtr<FJsonObject>* ChoiceObj = nullptr;
                if (!(*Choices)[0]->TryGetObject(ChoiceObj)) continue;

                const TSharedPtr<FJsonObject>* DeltaObj = nullptr;
                FString DeltaContent;
                if ((*ChoiceObj)->TryGetObjectField(TEXT("delta"), DeltaObj) &&
                    (*DeltaObj)->TryGetStringField(TEXT("content"), DeltaContent))
                {
                    Assembled += DeltaContent;
                }
                else
                {
                    // stream=false fallback shape (some 60db responses)
                    const TSharedPtr<FJsonObject>* MsgObj = nullptr;
                    FString MsgContent;
                    if ((*ChoiceObj)->TryGetObjectField(TEXT("message"), MsgObj) &&
                        (*MsgObj)->TryGetStringField(TEXT("content"), MsgContent))
                    {
                        Assembled += MsgContent;
                    }
                }
            }
        }
        ResponseCallback(Assembled, TEXT(""), true);
        return;
    }

    // Non-stream: a single JSON envelope.
    TSharedPtr<FJsonObject> Json;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
    if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
    {
        ResponseCallback(TEXT(""), TEXT("60db chat: JSON parse failed"), false);
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
    if (!Json->TryGetArrayField(TEXT("choices"), Choices) || Choices->Num() == 0)
    {
        FString Err;
        Json->TryGetStringField(TEXT("error"), Err);
        ResponseCallback(TEXT(""), Err.IsEmpty() ? TEXT("60db chat: no choices in response") : Err, false);
        return;
    }

    const TSharedPtr<FJsonObject>* ChoiceObj = nullptr;
    const TSharedPtr<FJsonObject>* MsgObj = nullptr;
    FString MsgContent;
    if ((*Choices)[0]->TryGetObject(ChoiceObj) &&
        (*ChoiceObj)->TryGetObjectField(TEXT("message"), MsgObj) &&
        (*MsgObj)->TryGetStringField(TEXT("content"), MsgContent))
    {
        ResponseCallback(MsgContent, TEXT(""), true);
        return;
    }
    ResponseCallback(TEXT(""), TEXT("60db chat: choices[0].message.content missing"), false);
}
