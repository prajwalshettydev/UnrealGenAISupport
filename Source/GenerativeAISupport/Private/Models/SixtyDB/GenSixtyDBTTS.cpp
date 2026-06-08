// 60db.ai text-to-speech client implementation.
//
// Flow:
//   1. POST /tts-synthesize with auth + JSON body
//   2. Parse JSON envelope, base64-decode audio_base64
//   3. If output_format == "wav", parse the RIFF header and feed PCM into a
//      USoundWaveProcedural so BP can plug it straight into an Audio Component
//   4. Fire OnComplete with bytes + sound wave + metadata

#include "Models/SixtyDB/GenSixtyDBTTS.h"

#include "Data/GenAIOrgs.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Http.h"
#include "Misc/Base64.h"
#include "Secure/GenSecureKey.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundWaveProcedural.h"
#include "Utilities/GenGlobalDefinitions.h"


static const FString SIXTYDB_TTS_URL = TEXT("https://api.60db.ai/tts-synthesize");
static const FString DEFAULT_VOICE_ID = TEXT("fbb75ed2-975a-40c7-9e06-38e30524a9a1"); // Zara


TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> UGenSixtyDBTTS::SendTTSRequest(
    const FGenSixtyDBTTSSettings& Settings,
    const FOnSixtyDBTTSResponse& OnComplete)
{
    check(OnComplete.IsBound());

    const FString ApiKey = UGenSecureKey::GetGenerativeAIApiKey(EGenAIOrgs::SixtyDB);
    if (ApiKey.IsEmpty())
    {
        OnComplete.Execute({}, nullptr, TEXT("60db API key not set (SIXTYDB_API_KEY or PS_SIXTYDBAPIKEY)"), false);
        return nullptr;
    }

    // Clamp ranges to 60db-doc-validated values, just in case BP supplied junk.
    const int32 Stability  = FMath::Clamp(Settings.Stability,  0, 100);
    const int32 Similarity = FMath::Clamp(Settings.Similarity, 0, 100);
    const float Speed      = FMath::Clamp(Settings.Speed, 0.5f, 2.0f);
    const FString Voice    = Settings.VoiceId.IsEmpty() ? DEFAULT_VOICE_ID : Settings.VoiceId;
    const FString Format   = Settings.OutputFormat.IsEmpty() ? TEXT("wav") : Settings.OutputFormat.ToLower();

    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    Payload->SetStringField(TEXT("text"), Settings.Text);
    Payload->SetStringField(TEXT("voice_id"), Voice);
    Payload->SetBoolField  (TEXT("enhance"), Settings.bEnhance);
    Payload->SetNumberField(TEXT("speed"), Speed);
    Payload->SetNumberField(TEXT("stability"), Stability);
    Payload->SetNumberField(TEXT("similarity"), Similarity);
    Payload->SetStringField(TEXT("output_format"), Format);

    FString PayloadStr;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadStr);
    FJsonSerializer::Serialize(Payload.ToSharedRef(), Writer);

    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(SIXTYDB_TTS_URL);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
    Request->SetContentAsString(PayloadStr);

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete](FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bConnectedOk)
    {
        if (!bConnectedOk || !Response.IsValid())
        {
            OnComplete.Execute({}, nullptr, TEXT("60db TTS: HTTP connection failed"), false);
            return;
        }
        if (Response->GetResponseCode() != 200)
        {
            const FString Body = Response->GetContentAsString();
            OnComplete.Execute({}, nullptr,
                FString::Printf(TEXT("60db TTS HTTP %d: %s"),
                                Response->GetResponseCode(),
                                *Body.Left(200)),
                false);
            return;
        }

        TSharedPtr<FJsonObject> Json;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
        if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
        {
            OnComplete.Execute({}, nullptr, TEXT("60db TTS: JSON parse failed"), false);
            return;
        }

        FString AudioB64;
        if (!Json->TryGetStringField(TEXT("audio_base64"), AudioB64) || AudioB64.IsEmpty())
        {
            FString Msg;
            Json->TryGetStringField(TEXT("message"), Msg);
            OnComplete.Execute({}, nullptr,
                Msg.IsEmpty() ? TEXT("60db TTS: no audio_base64 in response") : Msg,
                false);
            return;
        }

        TArray<uint8> AudioBytes;
        if (!FBase64::Decode(AudioB64, AudioBytes))
        {
            OnComplete.Execute({}, nullptr, TEXT("60db TTS: base64 decode failed"), false);
            return;
        }

        // Optional: build a USoundWaveProcedural for callers that want to play
        // the audio directly. Only works for WAV (we parse the RIFF header).
        FString OutputFormat;
        Json->TryGetStringField(TEXT("output_format"), OutputFormat);
        USoundWaveProcedural* SoundWave = nullptr;
        if (OutputFormat.IsEmpty() || OutputFormat.ToLower() == TEXT("wav"))
        {
            SoundWave = BuildSoundWaveFromWav(AudioBytes);
        }

        OnComplete.Execute(AudioBytes, SoundWave, TEXT(""), true);
    });

    Request->ProcessRequest();
    return Request;
}


UGenSixtyDBTTS* UGenSixtyDBTTS::RequestSixtyDBTTS(UObject* WorldContextObject,
                                                  const FGenSixtyDBTTSSettings& Settings)
{
    UGenSixtyDBTTS* Action = NewObject<UGenSixtyDBTTS>();
    Action->Settings = Settings;
    Action->RegisterWithGameInstance(WorldContextObject);
    return Action;
}


void UGenSixtyDBTTS::Activate()
{
    TWeakObjectPtr<UGenSixtyDBTTS> WeakThis(this);

    FOnSixtyDBTTSResponse NativeCb;
    NativeCb.BindLambda([WeakThis](const TArray<uint8>& Bytes,
                                   USoundWaveProcedural* SoundWave,
                                   const FString& Error,
                                   bool bSuccess)
    {
        if (!WeakThis.IsValid()) return;
        UGenSixtyDBTTS* Strong = WeakThis.Get();

        // Pull the metadata fields from the live HTTP response — we don't
        // re-parse here; instead we synthesize a minimal Result struct from
        // the SoundWave (if present). The native callback signature is leaner
        // than the BP one on purpose.
        FGenSixtyDBTTSResult Result;
        if (SoundWave)
        {
            Result.SampleRate      = SoundWave->GetSampleRateForCurrentPlatform();
            Result.DurationSeconds = SoundWave->Duration;
            Result.Encoding        = TEXT("linear16");
            Result.OutputFormat    = TEXT("wav");
        }
        Strong->OnComplete.Broadcast(Bytes, SoundWave, Result, Error, bSuccess);
        Strong->Cancel();
    });

    HttpRequest = SendTTSRequest(Settings, NativeCb);
}


void UGenSixtyDBTTS::Cancel()
{
    if (HttpRequest.IsValid() && HttpRequest->GetStatus() == EHttpRequestStatus::Processing)
    {
        HttpRequest->CancelRequest();
    }
    Super::Cancel();
}


/**
 * Minimal RIFF/WAVE parser sufficient for typical PCM16 mono output from
 * /tts-synthesize. Skips through chunks until "fmt " and "data" are found.
 *
 * Returns nullptr on any parse failure — callers can still use the raw
 * AudioBytes (e.g. write to disk for editor import).
 */
USoundWaveProcedural* UGenSixtyDBTTS::BuildSoundWaveFromWav(const TArray<uint8>& WavBytes)
{
    if (WavBytes.Num() < 44) return nullptr;
    if (FMemory::Memcmp(WavBytes.GetData(), "RIFF", 4) != 0) return nullptr;
    if (FMemory::Memcmp(WavBytes.GetData() + 8, "WAVE", 4) != 0) return nullptr;

    int32 Pos = 12;
    uint16 NumChannels    = 0;
    uint32 SampleRate     = 0;
    uint16 BitsPerSample  = 0;
    const uint8* PcmData  = nullptr;
    int32  PcmBytes       = 0;

    while (Pos + 8 <= WavBytes.Num())
    {
        const char* Id    = reinterpret_cast<const char*>(WavBytes.GetData() + Pos);
        const uint32 Size = *reinterpret_cast<const uint32*>(WavBytes.GetData() + Pos + 4);
        Pos += 8;
        if (Pos + static_cast<int32>(Size) > WavBytes.Num()) break;

        if (FMemory::Memcmp(Id, "fmt ", 4) == 0 && Size >= 16)
        {
            const uint8* Fmt = WavBytes.GetData() + Pos;
            NumChannels   = *reinterpret_cast<const uint16*>(Fmt + 2);
            SampleRate    = *reinterpret_cast<const uint32*>(Fmt + 4);
            BitsPerSample = *reinterpret_cast<const uint16*>(Fmt + 14);
        }
        else if (FMemory::Memcmp(Id, "data", 4) == 0)
        {
            PcmData  = WavBytes.GetData() + Pos;
            PcmBytes = static_cast<int32>(Size);
            break; // we don't need anything after data
        }

        // Chunks are word-aligned.
        Pos += Size + (Size & 1);
    }

    if (!PcmData || PcmBytes <= 0 || NumChannels == 0 || SampleRate == 0 || BitsPerSample != 16)
    {
        return nullptr;
    }

    USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>();
    SoundWave->SetSampleRate(SampleRate);
    SoundWave->NumChannels      = NumChannels;
    SoundWave->Duration         = static_cast<float>(PcmBytes) /
                                  (SampleRate * NumChannels * (BitsPerSample / 8));
    SoundWave->SoundGroup       = SOUNDGROUP_Default;
    SoundWave->bLooping         = false;
    SoundWave->QueueAudio(PcmData, PcmBytes);
    return SoundWave;
}
