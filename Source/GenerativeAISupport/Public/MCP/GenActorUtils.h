// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenActorUtils.generated.h"

/**
 * 
 */
UCLASS()
class GENERATIVEAISUPPORT_API UGenActorUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	public:
        UFUNCTION(BlueprintCallable, Category="Generative AI|Utils")
        static AActor* SpawnBasicShape(const FString& ShapeName, const FVector& Location, 
                                      const FRotator& Rotation, const FVector& Scale, 
                                      const FString& ActorLabel = TEXT(""));
    
        UFUNCTION(BlueprintCallable, Category="Generative AI|Utils")
        static AActor* SpawnActorFromClass(const FString& ActorClassName, const FVector& Location, 
                                          const FRotator& Rotation, const FVector& Scale, 
                                          const FString& ActorLabel = TEXT(""));
};
