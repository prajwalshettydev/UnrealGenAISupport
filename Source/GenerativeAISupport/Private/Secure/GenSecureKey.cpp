// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.


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
		case EGenAIOrgs::SixtyDB:
			// Try the project's PS_-prefixed convention first, then fall back to
			// the canonical SIXTYDB_API_KEY used by 60db.ai's own SDK examples.
			{
				FString FromPS = GetEnvironmentVariable(TEXT("PS_SIXTYDBAPIKEY"));
				if (!FromPS.IsEmpty()) return FromPS;
				FString FromCanonical = GetEnvironmentVariable(TEXT("SIXTYDB_API_KEY"));
				if (!FromCanonical.IsEmpty()) return FromCanonical;
			}
			// Fall through to the stored-map lookup below.
			EnvKey = TEXT("PS_SIXTYDBAPIKEY");
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