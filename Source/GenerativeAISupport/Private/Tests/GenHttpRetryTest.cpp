// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#include "Misc/AutomationTest.h"
#include "Utilities/GenHttpRetry.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenHttpRetryTest, "GenerativeAISupport.HttpRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenHttpRetryTest::RunTest(const FString& Parameters)
{
	// Rate limits and overloads back off: ~0.5s, then ~1s.
	const double First = GenHttpRetry::GetRetryDelaySeconds(429, TEXT(""), 1);
	const double Second = GenHttpRetry::GetRetryDelaySeconds(503, TEXT(""), 2);
	TestTrue(TEXT("429 retries after ~0.5s"), First >= 0.45 && First <= 0.55);
	TestTrue(TEXT("503 retries after ~1s"), Second >= 0.9 && Second <= 1.1);
	TestTrue(TEXT("Anthropic 529 overloaded retries"), GenHttpRetry::GetRetryDelaySeconds(529, TEXT(""), 1) > 0.0);

	// Retry-After wins over the backoff formula.
	const double Hinted = GenHttpRetry::GetRetryDelaySeconds(429, TEXT("4"), 1);
	TestTrue(TEXT("Retry-After: 4 waits ~4s"), Hinted >= 3.6 && Hinted <= 4.4);

	// Success, caller errors and the last attempt do not retry.
	TestTrue(TEXT("200 is not retried"), GenHttpRetry::GetRetryDelaySeconds(200, TEXT(""), 1) < 0.0);
	TestTrue(TEXT("400 is not retried"), GenHttpRetry::GetRetryDelaySeconds(400, TEXT(""), 1) < 0.0);
	TestTrue(TEXT("401 is not retried"), GenHttpRetry::GetRetryDelaySeconds(401, TEXT(""), 1) < 0.0);
	TestTrue(TEXT("third attempt is the last"), GenHttpRetry::GetRetryDelaySeconds(429, TEXT(""), 3) < 0.0);
	return true;
}

#endif
