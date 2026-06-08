// 60db.ai integration — request/response USTRUCTs.
//
// 60db ships an OpenAI-compatible /v1/chat/completions endpoint (model
// "60db-tiny") plus a base64-JSON /tts-synthesize endpoint. We model both
// here so Blueprint callers see a single Settings struct per surface.
//
// Docs:
//   https://docs.60db.ai/api-reference/llm/chat-completion
//   https://docs.60db.ai/api-reference/tts/text-to-speech

#pragma once

#include "CoreMinimal.h"
#include "Data/OpenAI/GenOAIChatStructs.h"  // reuse FGenChatMessage for parity
#include "GenSixtyDBStructs.generated.h"


/** 60db chat (OpenAI-compatible). */
USTRUCT(BlueprintType)
struct FGenSixtyDBChatSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db")
    FString Model = TEXT("60db-tiny");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db")
    TArray<FGenChatMessage> Messages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db", meta = (ClampMin = "1", ClampMax = "32768"))
    int32 MaxTokens = 1024;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float Temperature = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db", meta = (ClampMin = "1", ClampMax = "1000"))
    int32 TopK = 20;

    /** Streaming responses are decoded server-side and the full text is delivered
     *  via the async OnComplete delegate. Pass false to ask the server for a
     *  single non-stream JSON body. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db")
    bool bStream = true;

    /** When true, 60db persists the conversation on its side and returns a
     *  chat_id. Default false — Unreal projects usually own chat state. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db")
    bool bSaveChat = false;

    /** Optional. Pass to continue an existing server-side chat. Ignored when
     *  bSaveChat is false. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db")
    FString ChatId;
};


/** 60db TTS sync REST settings — mirrors POST /tts-synthesize body shape.
 *  All numeric ranges follow the 60db docs verbatim (stability/similarity 0..100,
 *  speed 0.5..2.0). */
USTRUCT(BlueprintType)
struct FGenSixtyDBTTSSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db", meta = (MultiLine = "true"))
    FString Text;

    /** UUID from /default-voices or /myvoices. Empty falls back to "Zara". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db")
    FString VoiceId = TEXT("fbb75ed2-975a-40c7-9e06-38e30524a9a1");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db", meta = (ClampMin = "0.5", ClampMax = "2.0"))
    float Speed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db", meta = (ClampMin = "0", ClampMax = "100"))
    int32 Stability = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db", meta = (ClampMin = "0", ClampMax = "100"))
    int32 Similarity = 75;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db")
    bool bEnhance = true;

    /** wav is the most useful format for in-engine playback because it carries
     *  a parseable header. mp3 saves bandwidth for download-then-import flows. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|60db")
    FString OutputFormat = TEXT("wav");
};


/** Metadata returned alongside the audio bytes for a /tts-synthesize call. */
USTRUCT(BlueprintType)
struct FGenSixtyDBTTSResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "GenAI|60db")
    int32 SampleRate = 0;

    UPROPERTY(BlueprintReadOnly, Category = "GenAI|60db")
    float DurationSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "GenAI|60db")
    FString Encoding;

    UPROPERTY(BlueprintReadOnly, Category = "GenAI|60db")
    FString OutputFormat;
};
