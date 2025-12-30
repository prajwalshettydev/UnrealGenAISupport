// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenAIUtils.generated.h"

/**
 * Editor utilities for AI/Behavior Tree systems
 * Allows configuring AI controllers, behavior trees, and navigation via Python/MCP
 */
UCLASS()
class GENERATIVEAISUPPORTEDITOR_API UGenAIUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// AI CONTROLLER MANAGEMENT
	// ============================================

	/**
	 * List all AI Controllers in the level.
	 * @return JSON array of AI controllers with their pawns
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString ListAIControllers();

	/**
	 * Get AI Controller info for an actor.
	 * @param ActorName - Actor label
	 * @return JSON with AI controller details
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString GetAIControllerInfo(const FString& ActorName);

	/**
	 * Set AI Controller class for an actor.
	 * @param ActorName - Actor label
	 * @param ControllerClassPath - Path to AI Controller class
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString SetAIController(const FString& ActorName, const FString& ControllerClassPath);

	// ============================================
	// BEHAVIOR TREES
	// ============================================

	/**
	 * List all Behavior Tree assets.
	 * @param DirectoryPath - Directory to search
	 * @return JSON array of behavior trees
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString ListBehaviorTrees(const FString& DirectoryPath = TEXT("/Game"));

	/**
	 * Get Behavior Tree info.
	 * @param BehaviorTreePath - Path to behavior tree asset
	 * @return JSON with tree structure
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString GetBehaviorTreeInfo(const FString& BehaviorTreePath);

	/**
	 * Assign Behavior Tree to AI Controller.
	 * @param ActorName - Actor label with AI Controller
	 * @param BehaviorTreePath - Path to behavior tree
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString SetBehaviorTree(const FString& ActorName, const FString& BehaviorTreePath);

	/**
	 * Run Behavior Tree on AI (PIE only).
	 * @param ActorName - Actor label
	 * @param BehaviorTreePath - Path to behavior tree
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString RunBehaviorTree(const FString& ActorName, const FString& BehaviorTreePath);

	/**
	 * Stop Behavior Tree on AI (PIE only).
	 * @param ActorName - Actor label
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString StopBehaviorTree(const FString& ActorName);

	// ============================================
	// BLACKBOARD
	// ============================================

	/**
	 * List all Blackboard assets.
	 * @param DirectoryPath - Directory to search
	 * @return JSON array of blackboards
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString ListBlackboards(const FString& DirectoryPath = TEXT("/Game"));

	/**
	 * Get Blackboard keys.
	 * @param BlackboardPath - Path to blackboard asset
	 * @return JSON with key definitions
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString GetBlackboardKeys(const FString& BlackboardPath);

	/**
	 * Get blackboard value (PIE only).
	 * @param ActorName - Actor label
	 * @param KeyName - Blackboard key name
	 * @return JSON with value
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString GetBlackboardValue(const FString& ActorName, const FString& KeyName);

	/**
	 * Set blackboard value (PIE only).
	 * @param ActorName - Actor label
	 * @param KeyName - Blackboard key name
	 * @param Value - Value as string
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString SetBlackboardValue(const FString& ActorName, const FString& KeyName, const FString& Value);

	// ============================================
	// NAVIGATION
	// ============================================

	/**
	 * Check if a location is navigable.
	 * @param Location - World location to check
	 * @return JSON with navigability result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString IsLocationNavigable(const FVector& Location);

	/**
	 * Find path between two points.
	 * @param StartLocation - Path start
	 * @param EndLocation - Path end
	 * @return JSON with path points
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString FindPath(const FVector& StartLocation, const FVector& EndLocation);

	/**
	 * Get random navigable point in radius.
	 * @param Origin - Center point
	 * @param Radius - Search radius
	 * @return JSON with random point
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString GetRandomNavigablePoint(const FVector& Origin, float Radius);

	/**
	 * Project point to navigation.
	 * @param Location - Location to project
	 * @param QueryExtent - Search extent
	 * @return JSON with projected point
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString ProjectPointToNavigation(const FVector& Location, const FVector& QueryExtent = FVector(100, 100, 250));

	// ============================================
	// AI MOVEMENT
	// ============================================

	/**
	 * Move AI to location (PIE only).
	 * @param ActorName - Actor label
	 * @param TargetLocation - Destination
	 * @param AcceptanceRadius - How close to get
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString MoveToLocation(const FString& ActorName, const FVector& TargetLocation, float AcceptanceRadius = 50.0f);

	/**
	 * Move AI to actor (PIE only).
	 * @param ActorName - AI Actor label
	 * @param TargetActorName - Target actor label
	 * @param AcceptanceRadius - How close to get
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString MoveToActor(const FString& ActorName, const FString& TargetActorName, float AcceptanceRadius = 50.0f);

	/**
	 * Stop AI movement (PIE only).
	 * @param ActorName - Actor label
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString StopMovement(const FString& ActorName);

	/**
	 * Get AI movement status (PIE only).
	 * @param ActorName - Actor label
	 * @return JSON with movement state
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString GetMovementStatus(const FString& ActorName);

	// ============================================
	// AI PERCEPTION
	// ============================================

	/**
	 * Get perception component info.
	 * @param ActorName - Actor label
	 * @return JSON with perception config
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString GetPerceptionInfo(const FString& ActorName);

	/**
	 * Get currently perceived actors (PIE only).
	 * @param ActorName - Actor label
	 * @return JSON array of perceived actors
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString GetPerceivedActors(const FString& ActorName);

	/**
	 * Configure sight sense.
	 * @param ActorName - Actor label
	 * @param SightRadius - Max sight distance
	 * @param LoseSightRadius - Distance to lose sight
	 * @param PeripheralVisionAngle - FOV angle in degrees
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString ConfigureSightSense(const FString& ActorName, float SightRadius,
									   float LoseSightRadius, float PeripheralVisionAngle);

	/**
	 * Configure hearing sense.
	 * @param ActorName - Actor label
	 * @param HearingRange - Max hearing distance
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString ConfigureHearingSense(const FString& ActorName, float HearingRange);

	// ============================================
	// EQS (Environment Query System)
	// ============================================

	/**
	 * List EQS queries.
	 * @param DirectoryPath - Directory to search
	 * @return JSON array of EQS queries
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString ListEQSQueries(const FString& DirectoryPath = TEXT("/Game"));

	/**
	 * Run EQS query (PIE only).
	 * @param ActorName - Querier actor
	 * @param QueryPath - Path to EQS query
	 * @return JSON with query results
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString RunEQSQuery(const FString& ActorName, const FString& QueryPath);

	// ============================================
	// SMART OBJECTS
	// ============================================

	/**
	 * List Smart Objects in level.
	 * @return JSON array of smart objects
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString ListSmartObjects();

	/**
	 * Find Smart Objects matching criteria.
	 * @param ActorName - Requesting actor
	 * @param TagQuery - Tag filter
	 * @return JSON array of matching smart objects
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString FindSmartObjects(const FString& ActorName, const FString& TagQuery);

	// ============================================
	// CROWD & AVOIDANCE
	// ============================================

	/**
	 * Get avoidance info for actor.
	 * @param ActorName - Actor label
	 * @return JSON with avoidance settings
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString GetAvoidanceInfo(const FString& ActorName);

	/**
	 * Set avoidance settings.
	 * @param ActorName - Actor label
	 * @param bEnableAvoidance - Enable/disable
	 * @param AvoidanceWeight - Weight (0-1)
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|AI Utils")
	static FString SetAvoidanceSettings(const FString& ActorName, bool bEnableAvoidance, float AvoidanceWeight);

private:
	/** Find actor by name */
	static AActor* FindActorByName(const FString& ActorName);

	/** Find actor in PIE world */
	static AActor* FindActorInPIE(const FString& ActorName);

	/** Get PIE world */
	static UWorld* GetPIEWorld();

	/** Get navigation system */
	static class UNavigationSystemV1* GetNavSystem();
};
