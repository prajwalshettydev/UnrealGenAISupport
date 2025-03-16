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
	// Existing functions
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static AActor* SpawnBasicShape(const FString& ShapeName, const FVector& Location, 
								  const FRotator& Rotation, const FVector& Scale, 
								  const FString& ActorLabel);
    
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static AActor* SpawnActorFromClass(const FString& ActorClassName, const FVector& Location, 
									  const FRotator& Rotation, const FVector& Scale, 
									  const FString& ActorLabel);
                                      
	// New functions for material and object modification
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static UMaterial* CreateMaterial(const FString& MaterialName, const FLinearColor& Color);
    
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static bool SetActorMaterial(const FString& ActorName, UMaterialInterface* Material);
    
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static bool SetActorMaterialByPath(const FString& ActorName, const FString& MaterialPath);
    
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static bool SetActorPosition(const FString& ActorName, const FVector& Position);
    
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static bool SetActorRotation(const FString& ActorName, const FRotator& Rotation);
    
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static bool SetActorScale(const FString& ActorName, const FVector& Scale);
    
	// Utility function to find actors by name
	static AActor* FindActorByName(const FString& ActorName);
};
