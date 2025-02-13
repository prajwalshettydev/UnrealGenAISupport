// Copyright Prajwal Shetty 2024. All rights Reserved. https://prajwalshetty.com/terms

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#if PLATFORM_WINDOWS
#include "Runtime/Core/Public/Windows/WindowsPlatformMisc.h"
#endif

#if PLATFORM_MAC
#include "Runtime/Core/Public/Apple/ApplePlatformMisc.h"
#endif

#if PLATFORM_LINUX
#include "Runtime/Core/Public/Linux/LinuxPlatformMisc.h"
#endif

#include "GenSecureKey.generated.h"

enum class EGenAIOrgs : uint8;

UCLASS()
class GENERATIVEAISUPPORT_API UGenSecureKey : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:
	// A map to store API keys by organization
	static TMap<EGenAIOrgs, FString> APIKeys;

	// Flag to determine whether to use environment variables for API keys
	static bool bUseApiKeyFromEnv;

public:
	/**
	 * Stores the API key in memory for runtime use. 
	 * This does NOT modify system environment variables.
	 * If environment variables are enabled (via SetUseApiKeyFromEnvironmentVars),
	 * it will prioritize reading from the system instead of the stored key.
	 */
	UFUNCTION(BlueprintCallable, Category = "GenAI|Secure Key")
	static void SetGenAIApiKeyRuntime(EGenAIOrgs Org, const FString& APIKey);
	
	// Gets the API key for a specific organization
	UFUNCTION(BlueprintCallable, Category = "GenAI|Secure Key")
	static FString GetGenerativeAIApiKey(EGenAIOrgs Org);

	// Set whether to use the API key from environment variables
	UFUNCTION(BlueprintCallable, Category = "GenAI|Secure Key")
	static void SetUseApiKeyFromEnvironmentVars(bool bUseEnvVariable);

	// Get whether to use the API key from environment variables
	static bool GetUseApiKeyFromEnvironmentVars();

	// Retrieve the environment variable based on key
	static FString GetEnvironmentVariable(FString key);
};