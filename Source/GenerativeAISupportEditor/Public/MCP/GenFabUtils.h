// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GenFabUtils.generated.h"

UCLASS()
class GENERATIVEAISUPPORTEDITOR_API UGenFabUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Fab")
	static FString StartSearchFreeFabAssets(const FString& Query, int32 MaxResults = 10, float TimeoutSeconds = 60.0f);

	UFUNCTION(BlueprintCallable, Category = "Generative AI|Fab")
	static FString StartAddFreeFabAssetToProject(const FString& ListingIdOrUrl, float TimeoutSeconds = 180.0f);

	UFUNCTION(BlueprintCallable, Category = "Generative AI|Fab")
	static FString GetFabOperationStatus(const FString& OperationId);
};
