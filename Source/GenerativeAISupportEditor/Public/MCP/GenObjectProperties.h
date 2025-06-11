// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AutomationBlueprintFunctionLibrary.h"
#include "CoreMinimal.h"
#include "GenObjectProperties.generated.h"

/**
 *
 */
UCLASS()
class GENERATIVEAISUPPORTEDITOR_API UGenObjectProperties : public UAutomationBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	FString GetAllSceneObjects();

	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static FString EditComponentProperty(const FString& BlueprintPath, const FString& ComponentName,
										 const FString& PropertyName, const FString& Value, bool bIsSceneActor,
										 const FString& ActorName);
};
