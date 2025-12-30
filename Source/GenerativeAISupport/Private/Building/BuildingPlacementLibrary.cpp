// BuildingPlacementLibrary.cpp
// Implementation of building placement functions

#include "Building/BuildingPlacementLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

bool UBuildingPlacementLibrary::TraceGround(UWorld* World, FVector Location, FHitResult& OutHit, float TraceDistance)
{
	if (!World)
	{
		return false;
	}

	FVector TraceStart = Location + FVector(0, 0, 1000.f);
	FVector TraceEnd = Location - FVector(0, 0, TraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	return World->LineTraceSingleByChannel(
		OutHit,
		TraceStart,
		TraceEnd,
		ECC_WorldStatic,
		QueryParams
	);
}

float UBuildingPlacementLibrary::CalculateSlopeAngle(const FVector& Normal)
{
	// Calculate angle from vertical (UpVector)
	float DotProduct = FVector::DotProduct(Normal, FVector::UpVector);
	float AngleRadians = FMath::Acos(FMath::Clamp(DotProduct, -1.f, 1.f));
	return FMath::RadiansToDegrees(AngleRadians);
}

FFlatAreaResult UBuildingPlacementLibrary::FindFlatArea(
	UObject* WorldContextObject,
	FVector SearchCenter,
	float SearchRadius,
	float MaxSlopeAngle,
	int32 NumSamplePoints)
{
	FFlatAreaResult Result;

	if (!WorldContextObject)
	{
		return Result;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return Result;
	}

	// Ensure minimum sample points
	NumSamplePoints = FMath::Max(NumSamplePoints, 5);

	TArray<FVector> SampleLocations;
	TArray<float> Heights;
	TArray<FVector> Normals;

	// Generate sample points in a grid pattern
	int32 GridSize = FMath::CeilToInt(FMath::Sqrt((float)NumSamplePoints));
	float StepSize = (SearchRadius * 2.f) / (GridSize - 1);

	for (int32 X = 0; X < GridSize; X++)
	{
		for (int32 Y = 0; Y < GridSize; Y++)
		{
			FVector SampleLocation = SearchCenter;
			SampleLocation.X += (X - GridSize / 2) * StepSize;
			SampleLocation.Y += (Y - GridSize / 2) * StepSize;
			SampleLocations.Add(SampleLocation);
		}
	}

	// Trace ground at each sample point
	float TotalHeight = 0.f;
	FVector TotalNormal = FVector::ZeroVector;
	float MinHeight = TNumericLimits<float>::Max();
	float MaxHeight = TNumericLimits<float>::Lowest();
	float MaxSlope = 0.f;

	for (const FVector& SampleLoc : SampleLocations)
	{
		FHitResult Hit;
		if (TraceGround(World, SampleLoc, Hit))
		{
			Heights.Add(Hit.ImpactPoint.Z);
			Normals.Add(Hit.ImpactNormal);
			TotalHeight += Hit.ImpactPoint.Z;
			TotalNormal += Hit.ImpactNormal;

			MinHeight = FMath::Min(MinHeight, Hit.ImpactPoint.Z);
			MaxHeight = FMath::Max(MaxHeight, Hit.ImpactPoint.Z);

			float SlopeAngle = CalculateSlopeAngle(Hit.ImpactNormal);
			MaxSlope = FMath::Max(MaxSlope, SlopeAngle);
		}
	}

	// Check if we got enough valid samples
	if (Heights.Num() < NumSamplePoints / 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindFlatArea: Not enough valid ground samples at %s"), *SearchCenter.ToString());
		return Result;
	}

	// Calculate results
	Result.AverageHeight = TotalHeight / Heights.Num();
	Result.GroundNormal = TotalNormal.GetSafeNormal();
	Result.HeightVariation = MaxHeight - MinHeight;
	Result.MaxSlopeAngle = MaxSlope;

	// Determine if area is flat enough
	if (MaxSlope <= MaxSlopeAngle)
	{
		Result.bFoundFlatArea = true;
		Result.Location = FVector(SearchCenter.X, SearchCenter.Y, Result.AverageHeight);

		UE_LOG(LogTemp, Log, TEXT("FindFlatArea: Found flat area at %s (slope: %.1f deg, height var: %.1f)"),
			*Result.Location.ToString(), MaxSlope, Result.HeightVariation);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("FindFlatArea: Area too steep at %s (slope: %.1f deg > max %.1f deg)"),
			*SearchCenter.ToString(), MaxSlope, MaxSlopeAngle);
	}

	return Result;
}

FFootprintCheckResult UBuildingPlacementLibrary::CheckBuildingFootprint(
	UObject* WorldContextObject,
	FVector Location,
	FVector FootprintSize,
	FRotator Rotation,
	float MaxSlopeAngle,
	const TArray<AActor*>& ActorsToIgnore)
{
	FFootprintCheckResult Result;
	Result.bCanPlace = false;

	if (!WorldContextObject)
	{
		Result.FailReason = TEXT("Invalid world context");
		return Result;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		Result.FailReason = TEXT("Could not get world");
		return Result;
	}

	// First, check if the ground is flat enough
	FFlatAreaResult FlatCheck = FindFlatArea(WorldContextObject, Location, FMath::Max(FootprintSize.X, FootprintSize.Y) / 2.f, MaxSlopeAngle, 9);

	if (!FlatCheck.bFoundFlatArea)
	{
		Result.bTooSteep = true;
		Result.FailReason = FString::Printf(TEXT("Ground too steep (%.1f deg)"), FlatCheck.MaxSlopeAngle);
		return Result;
	}

	// Check for water
	if (IsInWater(WorldContextObject, Location))
	{
		Result.bInWater = true;
		Result.FailReason = TEXT("Location is in water");
		return Result;
	}

	// Check for collision with existing actors using BoxOverlapActors
	FVector BoxExtent = FootprintSize / 2.f;
	FVector BoxCenter = Location + FVector(0, 0, FootprintSize.Z / 2.f);

	TArray<AActor*> OverlappingActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

	bool bHasOverlap = UKismetSystemLibrary::BoxOverlapActors(
		WorldContextObject,
		BoxCenter,
		BoxExtent,
		ObjectTypes,
		nullptr,  // No class filter
		ActorsToIgnore,
		OverlappingActors
	);

	if (bHasOverlap && OverlappingActors.Num() > 0)
	{
		Result.bHasCollision = true;
		Result.BlockingActors = OverlappingActors;
		Result.FailReason = FString::Printf(TEXT("Collision with %d actors"), OverlappingActors.Num());
		return Result;
	}

	// All checks passed
	Result.bCanPlace = true;
	UE_LOG(LogTemp, Log, TEXT("CheckBuildingFootprint: Can place building at %s"), *Location.ToString());

	return Result;
}

bool UBuildingPlacementLibrary::PlaceBuildingOnTerrain(
	UObject* WorldContextObject,
	TSubclassOf<AActor> BuildingClass,
	FVector Location,
	FRotator Rotation,
	bool bAlignToSlope,
	AActor*& SpawnedBuilding)
{
	SpawnedBuilding = nullptr;

	if (!WorldContextObject || !BuildingClass)
	{
		UE_LOG(LogTemp, Error, TEXT("PlaceBuildingOnTerrain: Invalid parameters"));
		return false;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("PlaceBuildingOnTerrain: Could not get world"));
		return false;
	}

	// Find ground at location
	float GroundHeight;
	FVector GroundNormal;
	if (!GetGroundHeight(WorldContextObject, Location, GroundHeight, GroundNormal))
	{
		UE_LOG(LogTemp, Warning, TEXT("PlaceBuildingOnTerrain: Could not find ground at %s"), *Location.ToString());
		return false;
	}

	// Calculate spawn location
	FVector SpawnLocation = FVector(Location.X, Location.Y, GroundHeight);

	// Calculate rotation
	FRotator SpawnRotation;
	if (bAlignToSlope)
	{
		// Align to ground slope while keeping desired yaw
		FVector Right = FVector::CrossProduct(GroundNormal, FVector::ForwardVector).GetSafeNormal();
		FVector Forward = FVector::CrossProduct(Right, GroundNormal).GetSafeNormal();
		SpawnRotation = FRotationMatrix::MakeFromXZ(Forward, GroundNormal).Rotator();
		SpawnRotation.Yaw = Rotation.Yaw;
	}
	else
	{
		// Keep upright, only use yaw
		SpawnRotation = FRotator(0.f, Rotation.Yaw, 0.f);
	}

	// Spawn parameters
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Spawn the building
	SpawnedBuilding = World->SpawnActor<AActor>(BuildingClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (!SpawnedBuilding)
	{
		UE_LOG(LogTemp, Error, TEXT("PlaceBuildingOnTerrain: Failed to spawn building"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("PlaceBuildingOnTerrain: Spawned %s at %s"),
		*BuildingClass->GetName(), *SpawnLocation.ToString());

	return true;
}

bool UBuildingPlacementLibrary::GetGroundHeight(
	UObject* WorldContextObject,
	FVector Location,
	float& OutHeight,
	FVector& OutNormal)
{
	OutHeight = 0.f;
	OutNormal = FVector::UpVector;

	if (!WorldContextObject)
	{
		return false;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return false;
	}

	FHitResult Hit;
	if (TraceGround(World, Location, Hit))
	{
		OutHeight = Hit.ImpactPoint.Z;
		OutNormal = Hit.ImpactNormal;
		return true;
	}

	return false;
}

bool UBuildingPlacementLibrary::GetGroundHeightsGrid(
	UObject* WorldContextObject,
	FVector CenterLocation,
	FVector2D GridSize,
	int32 GridResolution,
	TArray<float>& OutHeights,
	float& OutAverageHeight,
	float& OutHeightVariation)
{
	OutHeights.Empty();
	OutAverageHeight = 0.f;
	OutHeightVariation = 0.f;

	if (!WorldContextObject)
	{
		return false;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return false;
	}

	GridResolution = FMath::Max(GridResolution, 2);

	float StepX = GridSize.X / (GridResolution - 1);
	float StepY = GridSize.Y / (GridResolution - 1);
	float StartX = CenterLocation.X - GridSize.X / 2.f;
	float StartY = CenterLocation.Y - GridSize.Y / 2.f;

	float TotalHeight = 0.f;
	float MinHeight = TNumericLimits<float>::Max();
	float MaxHeight = TNumericLimits<float>::Lowest();
	int32 ValidSamples = 0;

	for (int32 X = 0; X < GridResolution; X++)
	{
		for (int32 Y = 0; Y < GridResolution; Y++)
		{
			FVector SampleLocation(StartX + X * StepX, StartY + Y * StepY, CenterLocation.Z);
			FHitResult Hit;

			if (TraceGround(World, SampleLocation, Hit))
			{
				float Height = Hit.ImpactPoint.Z;
				OutHeights.Add(Height);
				TotalHeight += Height;
				MinHeight = FMath::Min(MinHeight, Height);
				MaxHeight = FMath::Max(MaxHeight, Height);
				ValidSamples++;
			}
			else
			{
				OutHeights.Add(0.f); // Placeholder for missing sample
			}
		}
	}

	if (ValidSamples > 0)
	{
		OutAverageHeight = TotalHeight / ValidSamples;
		OutHeightVariation = MaxHeight - MinHeight;
		return ValidSamples == GridResolution * GridResolution;
	}

	return false;
}

TArray<AActor*> UBuildingPlacementLibrary::FindNearbyBuildings(
	UObject* WorldContextObject,
	FVector Location,
	float SearchRadius,
	FName BuildingTag)
{
	TArray<AActor*> Result;

	if (!WorldContextObject)
	{
		return Result;
	}

	// Use SphereOverlapActors from KismetSystemLibrary
	TArray<AActor*> OverlappingActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

	UKismetSystemLibrary::SphereOverlapActors(
		WorldContextObject,
		Location,
		SearchRadius,
		ObjectTypes,
		nullptr,  // No class filter
		TArray<AActor*>(),  // No actors to ignore
		OverlappingActors
	);

	for (AActor* Actor : OverlappingActors)
	{
		if (Actor)
		{
			// Filter by tag if specified
			if (BuildingTag.IsNone() || Actor->ActorHasTag(BuildingTag))
			{
				Result.AddUnique(Actor);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("FindNearbyBuildings: Found %d buildings near %s"),
		Result.Num(), *Location.ToString());

	return Result;
}

bool UBuildingPlacementLibrary::IsInWater(
	UObject* WorldContextObject,
	FVector Location)
{
	if (!WorldContextObject)
	{
		return false;
	}

	// Use SphereOverlapActors to find water bodies
	TArray<AActor*> OverlappingActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

	UKismetSystemLibrary::SphereOverlapActors(
		WorldContextObject,
		Location,
		50.f,
		ObjectTypes,
		nullptr,
		TArray<AActor*>(),
		OverlappingActors
	);

	for (AActor* Actor : OverlappingActors)
	{
		if (Actor)
		{
			// Check if it's a water body by tag or class name
			if (Actor->ActorHasTag(TEXT("Water")) ||
				Actor->GetClass()->GetName().Contains(TEXT("Water")))
			{
				return true;
			}
		}
	}

	return false;
}
