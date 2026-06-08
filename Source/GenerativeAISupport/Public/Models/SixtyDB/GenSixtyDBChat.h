// 60db.ai chat client — wraps POST /v1/chat/completions in a
// BlueprintAsyncActionBase async node. Mirrors the GenOAIChat shape so
// callers can swap providers with a one-line code change.

#pragma once

#include "CoreMinimal.h"
#include "Data/SixtyDB/GenSixtyDBStructs.h"
#include "Engine/CancellableAsyncAction.h"
#include "Interfaces/IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GenSixtyDBChat.generated.h"


DECLARE_DELEGATE_ThreeParams(FOnSixtyDBChatResponse, const FString& /*Response*/, const FString& /*Error*/, bool /*bSuccess*/);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGenSixtyDBChatDelegate,
    const FString&, Response,
    const FString&, Error,
    bool, Success);


UCLASS()
class GENERATIVEAISUPPORT_API UGenSixtyDBChat : public UCancellableAsyncAction
{
    GENERATED_BODY()

public:
    /** Native C++ entry — fire-and-callback. Returns the underlying IHttpRequest
     *  so callers can keep a reference for cancellation if needed. */
    static TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> SendChatRequest(
        const FGenSixtyDBChatSettings& ChatSettings,
        const FOnSixtyDBChatResponse& OnComplete);

    /** Blueprint async delegate fired exactly once when the request resolves. */
    UPROPERTY(BlueprintAssignable)
    FGenSixtyDBChatDelegate OnComplete;

    /** Blueprint latent function — drop in any graph as a node with OnComplete pin. */
    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"),
              Category = "GenAI|60db")
    static UGenSixtyDBChat* RequestSixtyDBChat(UObject* WorldContextObject,
                                               const FGenSixtyDBChatSettings& ChatSettings);

    virtual void Cancel() override;

protected:
    virtual void Activate() override;

private:
    FGenSixtyDBChatSettings ChatSettings;
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest;

    static TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> MakeRequest(
        const FGenSixtyDBChatSettings& ChatSettings,
        const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);

    /** Parse a /v1/chat/completions JSON body, return the assistant text or
     *  surface an error string. Handles both stream=false (single JSON envelope)
     *  and stream=true (concatenated SSE buffer — we strip event prefixes and
     *  rebuild the full message client-side so the BP API stays one-shot). */
    static void ProcessResponse(const FString& ResponseStr,
                                bool bWasStreamRequested,
                                const TFunction<void(const FString&, const FString&, bool)>& ResponseCallback);
};
