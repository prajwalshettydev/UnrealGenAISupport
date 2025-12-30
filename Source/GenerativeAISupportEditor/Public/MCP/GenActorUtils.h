// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenActorUtils.generated.h"

/**
 * 
 */
UCLASS()
class GENERATIVEAISUPPORTEDITOR_API  UGenActorUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	// Existing functions
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static AActor* SpawnBasicShape(const FString& ShapeName, const FVector& Location, 
								  const FRotator& Rotation, const FVector& Scale, 
								  const FString& ActorLabel);
	
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")							  
	static AActor* SpawnStaticMeshActor(const FString& MeshPath, const FVector& Location, const FRotator& Rotation,
	                             const FVector& Scale, const FString& ActorLabel);

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

	
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static FString CreateGameModeWithPawn(const FString& GameModePath, const FString& PawnBlueprintPath,
	                               const FString& BaseClassName);

	// Utility function to find actors by name
	static AActor* FindActorByName(const FString& ActorName);

	/**
	 * Get all properties of an actor as JSON.
	 * @param ActorName - The name/label of the actor
	 * @return JSON with actor properties
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static FString GetActorProperties(const FString& ActorName);

	/**
	 * List all actors in the current level with optional class filter.
	 * @param ClassFilter - Optional class name to filter (empty = all actors)
	 * @return JSON array of actors with name, class, location
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static FString ListAllActors(const FString& ClassFilter);

	/**
	 * Delete an actor from the level.
	 * @param ActorName - The name/label of the actor to delete
	 * @return JSON result with success status
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static FString DeleteActor(const FString& ActorName);

	/**
	 * Duplicate an actor in the level.
	 * @param ActorName - The name/label of the actor to duplicate
	 * @param NewLocation - Location for the new actor
	 * @param NewLabel - Label for the new actor
	 * @return JSON result with new actor info
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static FString DuplicateActor(const FString& ActorName, const FVector& NewLocation, const FString& NewLabel);

	/**
	 * Save the current level.
	 * @return JSON result with success status
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static FString SaveCurrentLevel();

	/**
	 * Save all modified assets.
	 * @return JSON result with number of saved assets
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static FString SaveAllDirtyAssets();

	/**
	 * Get a specific property value from an actor.
	 * @param ActorName - The name/label of the actor
	 * @param PropertyName - Name of the property
	 * @return JSON with property value
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static FString GetActorProperty(const FString& ActorName, const FString& PropertyName);

	/**
	 * Set a property value on an actor.
	 * @param ActorName - The name/label of the actor
	 * @param PropertyName - Name of the property
	 * @param ValueString - Value as string
	 * @return JSON result with success status
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Actor Utils")
	static FString SetActorProperty(const FString& ActorName, const FString& PropertyName, const FString& ValueString);
};
