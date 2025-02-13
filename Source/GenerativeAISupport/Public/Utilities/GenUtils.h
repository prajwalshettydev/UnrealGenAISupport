// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GenUtils.generated.h"

/**
 * 
 */
UCLASS()
class GENERATIVEAISUPPORT_API UGenUtils : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "GenAI|Utilities")
	static FString GetEnumDisplayName(const UEnum* Enum, int32 Value)
	{
		if (!Enum) return TEXT("Invalid");
		return Enum->GetDisplayNameTextByValue(Value).ToString();
	}
};
