// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#include "Utilities/GenHttpRetry.h"

#include "Containers/Ticker.h"
#include "Interfaces/IHttpResponse.h"

#define LLM_RETRY_IMPLEMENTATION
#include "ThirdParty/llm-retry/llm_retry.hpp"

double GenHttpRetry::GetRetryDelaySeconds(int32 ResponseCode, const FString& RetryAfterHeader, int32 Attempt)
{
	static const llm::RetryConfig Config;
	if (ResponseCode < 400 || Attempt >= Config.max_attempts
		|| !llm::detail::is_retryable_code(ResponseCode, Config.retry_on_http_codes))
	{
		return -1.0;
	}

	double DelayMs = llm::detail::compute_delay_ms(Attempt, Config);
	if (RetryAfterHeader.IsNumeric())
	{
		// The server said when to come back.
		DelayMs = FMath::Min(FCString::Atod(*RetryAfterHeader) * 1000.0, Config.max_delay_ms);
	}
	return DelayMs * FMath::FRandRange(1.0 - Config.jitter_factor, 1.0 + Config.jitter_factor) / 1000.0;
}

bool GenHttpRetry::RetryIfTransient(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess, int32& Attempt)
{
	if (!bSuccess || !Request.IsValid() || !Response.IsValid())
	{
		return false;
	}

	const int32 Code = Response->GetResponseCode();
	const double DelaySeconds = GetRetryDelaySeconds(Code, Response->GetHeader(TEXT("retry-after")), Attempt);
	if (DelaySeconds < 0.0)
	{
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("%s returned HTTP %d, retrying in %.1fs (attempt %d)"), *Request->GetURL(), Code, DelaySeconds, Attempt + 1);
	++Attempt;
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Request](float)
	{
		Request->ProcessRequest();
		return false;
	}), static_cast<float>(DelaySeconds));
	return true;
}
