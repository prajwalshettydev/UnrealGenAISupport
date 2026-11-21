// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"

/**
 * Backoff for rate-limited and overloaded API calls (429, 408, 5xx, 529),
 * using llm-retry (Private/ThirdParty/llm-retry, v0.3.1).
 */
namespace GenHttpRetry
{
	/** Seconds to wait before resending after this response code, or a negative value to give up. */
	GENERATIVEAISUPPORT_API double GetRetryDelaySeconds(int32 ResponseCode, const FString& RetryAfterHeader, int32 Attempt);

	/** If the response is retryable and attempts remain, resends Request after the delay and returns true. */
	GENERATIVEAISUPPORT_API bool RetryIfTransient(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess, int32& Attempt);
}
