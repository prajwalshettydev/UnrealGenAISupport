// GenAnimationUtils.h - Animation & AI Utilities for NPCs
// Part of GenerativeAISupport Plugin - The Swiss Army Knife for Unreal Engine
// 2025-12-30

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenAnimationUtils.generated.h"

/**
 * Animation & AI Utility Class - NPC Animation and Movement Control
 * Skeletal Mesh Animation, AI Movement, Patrol Paths, and more
 */
UCLASS()
class GENERATIVEAISUPPORTEDITOR_API UGenAnimationUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ==================== ANIMATION PLAYBACK ====================

	/**
	 * Play animation on an actor with skeletal mesh
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Playback")
	static FString PlayAnimation(const FString& ActorName, const FString& AnimationPath, bool bLooping);

	/**
	 * Stop current animation on actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Playback")
	static FString StopAnimation(const FString& ActorName);

	/**
	 * Set animation blend time
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Playback")
	static FString SetAnimationBlendTime(const FString& ActorName, float BlendTime);

	/**
	 * Get currently playing animation
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Playback")
	static FString GetCurrentAnimation(const FString& ActorName);

	/**
	 * Set animation playback rate
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Playback")
	static FString SetAnimationPlayRate(const FString& ActorName, float PlayRate);

	// ==================== ANIMATION MONTAGE ====================

	/**
	 * Play animation montage on actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Montage")
	static FString PlayMontage(const FString& ActorName, const FString& MontagePath, float PlayRate);

	/**
	 * Stop montage on actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Montage")
	static FString StopMontage(const FString& ActorName, float BlendOutTime);

	/**
	 * Jump to montage section
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Montage")
	static FString JumpToMontageSection(const FString& ActorName, const FString& SectionName);

	// ==================== SKELETAL MESH ====================

	/**
	 * List all skeletal meshes in actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Skeletal")
	static FString ListSkeletalMeshes(const FString& ActorName);

	/**
	 * Set skeletal mesh on actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Skeletal")
	static FString SetSkeletalMesh(const FString& ActorName, const FString& SkeletalMeshPath);

	/**
	 * Set animation blueprint on skeletal mesh
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Skeletal")
	static FString SetAnimBlueprint(const FString& ActorName, const FString& AnimBlueprintPath);

	/**
	 * List animations compatible with skeletal mesh
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Skeletal")
	static FString ListCompatibleAnimations(const FString& SkeletonPath);

	// ==================== SIMPLE AI MOVEMENT ====================

	/**
	 * Make actor move to location (simple AI)
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|AI Movement")
	static FString MoveToLocation(const FString& ActorName, const FVector& TargetLocation, float AcceptanceRadius);

	/**
	 * Make actor patrol between points
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|AI Movement")
	static FString SetPatrolPoints(const FString& ActorName, const TArray<FVector>& PatrolPoints, bool bLoop);

	/**
	 * Start/Stop patrol
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|AI Movement")
	static FString TogglePatrol(const FString& ActorName, bool bStart);

	/**
	 * Make actor wander randomly in radius
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|AI Movement")
	static FString SetRandomWander(const FString& ActorName, float WanderRadius, float MinWaitTime, float MaxWaitTime);

	/**
	 * Stop all AI movement
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|AI Movement")
	static FString StopAIMovement(const FString& ActorName);

	/**
	 * Set movement speed
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|AI Movement")
	static FString SetMovementSpeed(const FString& ActorName, float WalkSpeed, float RunSpeed);

	// ==================== LOOK AT / FACING ====================

	/**
	 * Make actor look at target location
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Look At")
	static FString LookAtLocation(const FString& ActorName, const FVector& TargetLocation, float InterpSpeed);

	/**
	 * Make actor look at another actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Look At")
	static FString LookAtActor(const FString& ActorName, const FString& TargetActorName, float InterpSpeed);

	/**
	 * Stop look at
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Look At")
	static FString StopLookAt(const FString& ActorName);

	/**
	 * Set actor to face player
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Look At")
	static FString FacePlayer(const FString& ActorName, bool bContinuous);

	// ==================== RAGDOLL & PHYSICS ====================

	/**
	 * Enable ragdoll on skeletal mesh
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Ragdoll")
	static FString EnableRagdoll(const FString& ActorName);

	/**
	 * Disable ragdoll (return to animation)
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Ragdoll")
	static FString DisableRagdoll(const FString& ActorName);

	/**
	 * Apply impulse to ragdoll
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Ragdoll")
	static FString ApplyRagdollImpulse(const FString& ActorName, const FVector& Impulse, const FString& BoneName);

	// ==================== POSE & IK ====================

	/**
	 * Set bone rotation
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Pose")
	static FString SetBoneRotation(const FString& ActorName, const FString& BoneName, const FRotator& Rotation);

	/**
	 * Reset pose to animation
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Pose")
	static FString ResetPose(const FString& ActorName);

	/**
	 * List all bones in skeleton
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Pose")
	static FString ListBones(const FString& ActorName);

	// ==================== ANIMATION ASSETS ====================

	/**
	 * List all animation assets in directory
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Assets")
	static FString ListAnimationAssets(const FString& DirectoryPath);

	/**
	 * List all animation blueprints
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Assets")
	static FString ListAnimBlueprints(const FString& DirectoryPath);

	/**
	 * List all animation montages
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Assets")
	static FString ListMontages(const FString& DirectoryPath);

	/**
	 * Get animation info (length, frames, skeleton)
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Animation Utils|Assets")
	static FString GetAnimationInfo(const FString& AnimationPath);
};
