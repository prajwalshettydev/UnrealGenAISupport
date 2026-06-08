// 60db.ai text-to-speech client — wraps POST /tts-synthesize in a
// BlueprintAsyncActionBase async node. On success the OnComplete delegate
// emits both the raw audio bytes AND a USoundWaveProcedural ready to plug
// into an Audio Component for in-engine playback.
//
// Docs: https://docs.60db.ai/api-reference/tts/text-to-speech

#pragma once

#include "CoreMinimal.h"
#include "Data/SixtyDB/GenSixtyDBStructs.h"
#include "Engine/CancellableAsyncAction.h"
#include "Interfaces/IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GenSixtyDBTTS.generated.h"


class USoundWaveProcedural;


/** Native C++ callback. AudioBytes is raw decoded audio in the requested
 *  OutputFormat container (wav/mp3/ogg/flac). SoundWave is non-null when
 *  the container was WAV and PCM extraction succeeded; null otherwise. */
DECLARE_DELEGATE_FourParams(FOnSixtyDBTTSResponse,
    const TArray<uint8>& /*AudioBytes*/,
    USoundWaveProcedural* /*SoundWave*/,
    const FString& /*Error*/,
    bool /*bSuccess*/);

/** Blueprint async delegate. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FGenSixtyDBTTSDelegate,
    const TArray<uint8>&, AudioBytes,
    USoundWaveProcedural*, SoundWave,
    FGenSixtyDBTTSResult, Result,
    const FString&, Error,
    bool, Success);


UCLASS()
class GENERATIVEAISUPPORT_API UGenSixtyDBTTS : public UCancellableAsyncAction
{
    GENERATED_BODY()

public:
    /** Native C++ entry. */
    static TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> SendTTSRequest(
        const FGenSixtyDBTTSSettings& Settings,
        const FOnSixtyDBTTSResponse& OnComplete);

    UPROPERTY(BlueprintAssignable)
    FGenSixtyDBTTSDelegate OnComplete;

    /** Blueprint latent function. */
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"),
              Category = "GenAI|60db")
    static UGenSixtyDBTTS* RequestSixtyDBTTS(UObject* WorldContextObject,
                                             const FGenSixtyDBTTSSettings& Settings);

    virtual void Cancel() override;

protected:
    virtual void Activate() override;

private:
    FGenSixtyDBTTSSettings Settings;
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest;

    /** Build a USoundWaveProcedural from a WAV byte buffer. Returns nullptr if
     *  the buffer isn't a recognisable RIFF/WAVE (e.g. mp3, flac, ogg). */
    static USoundWaveProcedural* BuildSoundWaveFromWav(const TArray<uint8>& WavBytes);
};
