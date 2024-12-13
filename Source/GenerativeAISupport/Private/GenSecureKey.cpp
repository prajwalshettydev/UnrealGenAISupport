// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#include "GenSecureKey.h"
#include "Modules/ModuleManager.h"

// Static variables for storing API key and environment usage flag
static FString GenerativeAIApiKey = TEXT("");
static bool bUseApiKeyFromEnv = true;

// todo: storing encrypted keys FOR builds. testing. 

void UGenSecureKey::SetGenerativeAIApiKey(FString apiKey)
{
	GenerativeAIApiKey = apiKey;
}

FString UGenSecureKey::GetGenerativeAIApiKey()
{
	if (bUseApiKeyFromEnv)
	{
		FString KeyFromEnv = GetEnvironmentVariable(TEXT("PS_OPENAIAPIKEY"));
		return KeyFromEnv.IsEmpty() ? GenerativeAIApiKey : KeyFromEnv;
	}
	return GenerativeAIApiKey;
}

void UGenSecureKey::SetUseApiKeyFromEnvironmentVars(bool bUseEnvVariable)
{
	bUseApiKeyFromEnv = bUseEnvVariable;
}

bool UGenSecureKey::GetUseApiKeyFromEnvironmentVars()
{
	return bUseApiKeyFromEnv;
}

FString UGenSecureKey::GetEnvironmentVariable(FString key)
{
	FString result;
#if PLATFORM_WINDOWS
	result = FWindowsPlatformMisc::GetEnvironmentVariable(*key);
#endif
#if PLATFORM_MAC
	result = FApplePlatformMisc::GetEnvironmentVariable(*key);
#endif
#if PLATFORM_LINUX
	result = FLinuxPlatformMisc::GetEnvironmentVariable(*key);
#endif
	return result;
}