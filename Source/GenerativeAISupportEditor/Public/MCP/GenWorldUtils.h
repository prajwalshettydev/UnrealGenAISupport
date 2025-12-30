// GenWorldUtils.h - World Modding Utilities
// Part of GenerativeAISupport Plugin - The Swiss Army Knife for Unreal Engine
// 2025-12-30

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenWorldUtils.generated.h"

/**
 * World Modding Utility Class - Everything for level/world manipulation
 * Landscape, Foliage, Lighting, Volumes, Splines, Assets, and more
 */
UCLASS()
class GENERATIVEAISUPPORTEDITOR_API UGenWorldUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ==================== LIGHTING ====================

	/**
	 * Spawn a point light at location
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Lighting")
	static FString SpawnPointLight(const FVector& Location, float Intensity, const FLinearColor& Color, const FString& Label);

	/**
	 * Spawn a spot light at location
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Lighting")
	static FString SpawnSpotLight(const FVector& Location, const FRotator& Rotation, float Intensity, float InnerAngle, float OuterAngle, const FLinearColor& Color, const FString& Label);

	/**
	 * Spawn a directional light (sun)
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Lighting")
	static FString SpawnDirectionalLight(const FRotator& Rotation, float Intensity, const FLinearColor& Color, const FString& Label);

	/**
	 * Set light properties on existing light actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Lighting")
	static FString SetLightProperties(const FString& ActorName, float Intensity, const FLinearColor& Color, float AttenuationRadius);

	// ==================== VOLUMES ====================

	/**
	 * Spawn a trigger box volume
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Volumes")
	static FString SpawnTriggerBox(const FVector& Location, const FVector& BoxExtent, const FString& Label);

	/**
	 * Spawn a blocking volume
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Volumes")
	static FString SpawnBlockingVolume(const FVector& Location, const FVector& BoxExtent, const FString& Label);

	/**
	 * Spawn a post process volume
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Volumes")
	static FString SpawnPostProcessVolume(const FVector& Location, const FVector& BoxExtent, bool bUnbound, const FString& Label);

	/**
	 * Spawn an audio volume
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Volumes")
	static FString SpawnAudioVolume(const FVector& Location, const FVector& BoxExtent, const FString& Label);

	// ==================== FOLIAGE ====================

	/**
	 * Add foliage instances at locations
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Foliage")
	static FString AddFoliageAtLocations(const FString& FoliageTypePath, const TArray<FVector>& Locations);

	/**
	 * Remove foliage in radius
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Foliage")
	static FString RemoveFoliageInRadius(const FVector& Center, float Radius);

	/**
	 * List all foliage types in level
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Foliage")
	static FString ListFoliageTypes();

	// ==================== SPLINES ====================

	/**
	 * Create a spline actor with points
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Splines")
	static FString CreateSpline(const TArray<FVector>& Points, bool bClosedLoop, const FString& Label);

	/**
	 * Add point to existing spline
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Splines")
	static FString AddSplinePoint(const FString& SplineActorName, const FVector& Point, int32 Index);

	// ==================== ASSET MANAGEMENT ====================

	/**
	 * Import asset from file (texture, mesh, etc)
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Assets")
	static FString ImportAsset(const FString& FilePath, const FString& DestinationPath);

	/**
	 * Duplicate asset
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Assets")
	static FString DuplicateAsset(const FString& SourcePath, const FString& DestinationPath, const FString& NewName);

	/**
	 * Delete asset
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Assets")
	static FString DeleteAsset(const FString& AssetPath);

	/**
	 * Rename asset
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Assets")
	static FString RenameAsset(const FString& AssetPath, const FString& NewName);

	/**
	 * List assets in directory
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Assets")
	static FString ListAssets(const FString& DirectoryPath, const FString& ClassFilter);

	// ==================== LEVEL MANAGEMENT ====================

	/**
	 * Create a new sub-level
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Levels")
	static FString CreateSubLevel(const FString& LevelName, const FString& SavePath);

	/**
	 * Load sub-level
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Levels")
	static FString LoadSubLevel(const FString& LevelPath);

	/**
	 * Unload sub-level
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Levels")
	static FString UnloadSubLevel(const FString& LevelPath);

	/**
	 * List all levels in world
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Levels")
	static FString ListLevels();

	// ==================== CAMERA & VIEWPORT ====================

	/**
	 * Set editor camera location and rotation
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Camera")
	static FString SetEditorCamera(const FVector& Location, const FRotator& Rotation);

	/**
	 * Get current editor camera transform
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Camera")
	static FString GetEditorCamera();

	/**
	 * Focus editor camera on actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Camera")
	static FString FocusOnActor(const FString& ActorName);

	// ==================== SELECTION ====================

	/**
	 * Select actors by name pattern
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Selection")
	static FString SelectActors(const FString& NamePattern);

	/**
	 * Get currently selected actors
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Selection")
	static FString GetSelectedActors();

	/**
	 * Clear selection
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Selection")
	static FString ClearSelection();

	// ==================== TRANSFORM TOOLS ====================

	/**
	 * Move multiple actors by offset
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Transform")
	static FString MoveActors(const TArray<FString>& ActorNames, const FVector& Offset);

	/**
	 * Rotate multiple actors
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Transform")
	static FString RotateActors(const TArray<FString>& ActorNames, const FRotator& Rotation);

	/**
	 * Scale multiple actors
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Transform")
	static FString ScaleActors(const TArray<FString>& ActorNames, const FVector& Scale);

	/**
	 * Align actors to ground
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Transform")
	static FString AlignActorsToGround(const TArray<FString>& ActorNames);

	// ==================== UTILITIES ====================

	/**
	 * Take high-res screenshot
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Utilities")
	static FString TakeScreenshot(const FString& FileName, int32 ResolutionMultiplier);

	/**
	 * Build lighting for current level
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Utilities")
	static FString BuildLighting();

	/**
	 * Build all (geometry, lighting, paths, etc)
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Utilities")
	static FString BuildAll();

	/**
	 * Play in editor
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Utilities")
	static FString PlayInEditor();

	/**
	 * Stop play in editor
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|World Utils|Utilities")
	static FString StopPlayInEditor();
};
