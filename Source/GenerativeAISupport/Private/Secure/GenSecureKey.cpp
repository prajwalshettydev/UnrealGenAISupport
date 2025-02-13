// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#include "Secure/GenSecureKey.h"
#include "Data/GenAIOrgs.h"
#include "Modules/ModuleManager.h"

// Static variables for storing API key and environment usage flag
TMap<EGenAIOrgs, FString> UGenSecureKey::APIKeys;
bool UGenSecureKey::bUseApiKeyFromEnv = true;

void UGenSecureKey::SetGenAIApiKeyRuntime(EGenAIOrgs Org, const FString& APIKey)
{
    APIKeys.Add(Org, APIKey);
}

FString UGenSecureKey::GetGenerativeAIApiKey(EGenAIOrgs Org)
{
	if (bUseApiKeyFromEnv)
	{
		// Determine the appropriate environment variable key for the organization
		FString EnvKey;
		switch (Org)
		{
		case EGenAIOrgs::OpenAI:
			EnvKey = TEXT("PS_OPENAIAPIKEY");
			break;
		case EGenAIOrgs::DeepSeek:
			EnvKey = TEXT("PS_DEEPSEEKAPIKEY");
			break;
		case EGenAIOrgs::Anthropic:
			EnvKey = TEXT("PS_ANTHROPICAPIKEY");
			break;
		case EGenAIOrgs::Meta:
			EnvKey = TEXT("PS_METAAPIKEY");
			break;
		case EGenAIOrgs::Google:
			EnvKey = TEXT("PS_GOOGLEAPIKEY");
			break;
		case EGenAIOrgs::XAI:
			EnvKey = TEXT("PS_XAIAPIKEY");
			break;
		default:
			return TEXT("");
		}

		// Check the environment variable
		FString KeyFromEnv = GetEnvironmentVariable(EnvKey);
		if (!KeyFromEnv.IsEmpty())
		{
			return KeyFromEnv;
		}
	}

	// Return the stored key from the map
	return APIKeys.Contains(Org) ? APIKeys[Org] : TEXT("");
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