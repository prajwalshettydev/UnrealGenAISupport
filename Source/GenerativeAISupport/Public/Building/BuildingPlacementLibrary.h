// BuildingPlacementLibrary.h
// Blueprint-callable functions for placing buildings on terrain
// Part of GenerativeAISupport Plugin for Project Titan

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BuildingPlacementLibrary.generated.h"

/**
 * Result structure for flat area search
 */
USTRUCT(BlueprintType)
struct FFlatAreaResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Building Placement")
	bool bFoundFlatArea = false;

	UPROPERTY(BlueprintReadOnly, Category = "Building Placement")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Building Placement")
	FVector GroundNormal = FVector::UpVector;

	UPROPERTY(BlueprintReadOnly, Category = "Building Placement")
	float AverageHeight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Building Placement")
	float MaxSlopeAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Building Placement")
	float HeightVariation = 0.f;
};

/**
 * Result structure for footprint check
 */
USTRUCT(BlueprintType)
struct FFootprintCheckResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Building Placement")
	bool bCanPlace = false;

	UPROPERTY(BlueprintReadOnly, Category = "Building Placement")
	bool bHasCollision = false;

	UPROPERTY(BlueprintReadOnly, Category = "Building Placement")
	bool bTooSteep = false;

	UPROPERTY(BlueprintReadOnly, Category = "Building Placement")
	bool bInWater = false;

	UPROPERTY(BlueprintReadOnly, Category = "Building Placement")
	TArray<AActor*> BlockingActors;

	UPROPERTY(BlueprintReadOnly, Category = "Building Placement")
	FString FailReason;
};

/**
 * Blueprint Function Library for Building Placement
 * Provides functions to find suitable locations and place buildings on terrain
 */
UCLASS()
class GENERATIVEAISUPPORT_API UBuildingPlacementLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Find a flat area suitable for building placement
	 * @param WorldContextObject - World context
	 * @param SearchCenter - Center point to search around
	 * @param SearchRadius - Radius to check for flatness
	 * @param MaxSlopeAngle - Maximum allowed slope in degrees (default 15)
	 * @param NumSamplePoints - Number of points to sample (default 9)
	 * @return FFlatAreaResult with location info if found
	 */
	UFUNCTION(BlueprintCallable, Category = "Building Placement", meta = (DisplayName = "Find Flat Area", WorldContext = "WorldContextObject"))
	static FFlatAreaResult FindFlatArea(
		UObject* WorldContextObject,
		FVector SearchCenter,
		float SearchRadius = 500.f,
		float MaxSlopeAngle = 15.f,
		int32 NumSamplePoints = 9
	);

	/**
	 * Check if a building footprint can be placed at location
	 * @param WorldContextObject - World context
	 * @param Location - Desired placement location
	 * @param FootprintSize - Size of building footprint (X=Width, Y=Depth, Z=Height for collision check)
	 * @param Rotation - Desired rotation of building
	 * @param MaxSlopeAngle - Maximum allowed slope in degrees
	 * @param ActorsToIgnore - Actors to ignore in collision check
	 * @return FFootprintCheckResult with placement validity info
	 */
	UFUNCTION(BlueprintCallable, Category = "Building Placement", meta = (DisplayName = "Check Building Footprint", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore"))
	static FFootprintCheckResult CheckBuildingFootprint(
		UObject* WorldContextObject,
		FVector Location,
		FVector FootprintSize,
		FRotator Rotation,
		float MaxSlopeAngle,
		const TArray<AActor*>& ActorsToIgnore
	);

	/**
	 * Place a building on terrain with automatic ground alignment
	 * @param WorldContextObject - World context
	 * @param BuildingClass - Class of building to spawn
	 * @param Location - Desired location (will be adjusted to ground)
	 * @param Rotation - Desired rotation (Pitch/Roll will be zeroed for upright placement)
	 * @param bAlignToSlope - If true, building will tilt to match ground slope
	 * @param SpawnedBuilding - Output: The spawned building actor
	 * @return True if building was successfully placed
	 */
	UFUNCTION(BlueprintCallable, Category = "Building Placement", meta = (DisplayName = "Place Building On Terrain", WorldContext = "WorldContextObject"))
	static bool PlaceBuildingOnTerrain(
		UObject* WorldContextObject,
		TSubclassOf<AActor> BuildingClass,
		FVector Location,
		FRotator Rotation,
		bool bAlignToSlope,
		AActor*& SpawnedBuilding
	);

	/**
	 * Get ground height at a specific location
	 * @param WorldContextObject - World context
	 * @param Location - Location to check (X, Y used, Z ignored)
	 * @param OutHeight - Output: Ground height at location
	 * @param OutNormal - Output: Ground normal at location
	 * @return True if ground was found
	 */
	UFUNCTION(BlueprintCallable, Category = "Building Placement", meta = (DisplayName = "Get Ground Height", WorldContext = "WorldContextObject"))
	static bool GetGroundHeight(
		UObject* WorldContextObject,
		FVector Location,
		float& OutHeight,
		FVector& OutNormal
	);

	/**
	 * Get multiple ground heights in a grid pattern
	 * @param WorldContextObject - World context
	 * @param CenterLocation - Center of the grid
	 * @param GridSize - Size of the grid area
	 * @param GridResolution - Number of samples per axis (e.g., 5 = 5x5 grid = 25 samples)
	 * @param OutHeights - Output: Array of heights
	 * @param OutAverageHeight - Output: Average height
	 * @param OutHeightVariation - Output: Max height difference
	 * @return True if all samples were successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Building Placement", meta = (DisplayName = "Get Ground Heights Grid", WorldContext = "WorldContextObject"))
	static bool GetGroundHeightsGrid(
		UObject* WorldContextObject,
		FVector CenterLocation,
		FVector2D GridSize,
		int32 GridResolution,
		TArray<float>& OutHeights,
		float& OutAverageHeight,
		float& OutHeightVariation
	);

	/**
	 * Find nearby buildings/structures
	 * @param WorldContextObject - World context
	 * @param Location - Center location to search from
	 * @param SearchRadius - Radius to search
	 * @param BuildingTag - Optional tag to filter buildings (empty = all actors with collision)
	 * @return Array of nearby building actors
	 */
	UFUNCTION(BlueprintCallable, Category = "Building Placement", meta = (DisplayName = "Find Nearby Buildings", WorldContext = "WorldContextObject"))
	static TArray<AActor*> FindNearbyBuildings(
		UObject* WorldContextObject,
		FVector Location,
		float SearchRadius,
		FName BuildingTag = NAME_None
	);

	/**
	 * Check if location is in water
	 * @param WorldContextObject - World context
	 * @param Location - Location to check
	 * @return True if location is in water
	 */
	UFUNCTION(BlueprintCallable, Category = "Building Placement", meta = (DisplayName = "Is In Water", WorldContext = "WorldContextObject"))
	static bool IsInWater(
		UObject* WorldContextObject,
		FVector Location
	);

private:
	// Helper to perform a single ground trace
	static bool TraceGround(UWorld* World, FVector Location, FHitResult& OutHit, float TraceDistance = 10000.f);

	// Helper to calculate slope angle from normal
	static float CalculateSlopeAngle(const FVector& Normal);
};
