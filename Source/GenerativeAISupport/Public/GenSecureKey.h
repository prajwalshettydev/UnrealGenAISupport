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

UCLASS()
class GENERATIVEAISUPPORT_API UGenSecureKey : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Set the API key for Generative AI
	UFUNCTION(BlueprintCallable, Category = "Generative AI")
	static void SetGenerativeAIApiKey(FString apiKey);

	// Get the API key for Generative AI
	static FString GetGenerativeAIApiKey();

	// Set whether to use the API key from environment variables
	UFUNCTION(BlueprintCallable, Category = "Generative AI")
	static void SetUseApiKeyFromEnvironmentVars(bool bUseEnvVariable);

	// Get whether to use the API key from environment variables
	static bool GetUseApiKeyFromEnvironmentVars();

	// Retrieve the environment variable based on key
	static FString GetEnvironmentVariable(FString key);
};