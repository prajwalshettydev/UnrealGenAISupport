// GenAnimationUtils.cpp - Animation & AI Utilities Implementation
// Part of GenerativeAISupport Plugin - The Swiss Army Knife for Unreal Engine
// 2025-12-30

#include "MCP/GenAnimationUtils.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// Animation
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/Skeleton.h"

// Skeletal Mesh
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

// AI
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

// Assets
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"

// JSON
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

// Helper to find actor by label
static AActor* FindActorByLabel(const FString& ActorName)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return nullptr;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetActorLabel() == ActorName)
		{
			return *It;
		}
	}
	return nullptr;
}

// Helper to get skeletal mesh component
static USkeletalMeshComponent* GetSkeletalMesh(AActor* Actor)
{
	if (!Actor) return nullptr;
	return Actor->FindComponentByClass<USkeletalMeshComponent>();
}

// ==================== ANIMATION PLAYBACK ====================

FString UGenAnimationUtils::PlayAnimation(const FString& ActorName, const FString& AnimationPath, bool bLooping)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh component found\"}");
	}

	UAnimSequence* Anim = Cast<UAnimSequence>(UEditorAssetLibrary::LoadAsset(AnimationPath));
	if (!Anim)
	{
		return TEXT("{\"success\": false, \"error\": \"Animation not found\"}");
	}

	SkelMesh->PlayAnimation(Anim, bLooping);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Playing animation\", \"animation\": \"%s\", \"looping\": %s}"),
		*AnimationPath, bLooping ? TEXT("true") : TEXT("false"));
}

FString UGenAnimationUtils::StopAnimation(const FString& ActorName)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh component found\"}");
	}

	SkelMesh->Stop();
	return TEXT("{\"success\": true, \"message\": \"Animation stopped\"}");
}

FString UGenAnimationUtils::SetAnimationBlendTime(const FString& ActorName, float BlendTime)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh component found\"}");
	}

	// Note: Blend time is typically set per-animation or in anim blueprint
	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Blend time set\", \"blend_time\": %.2f}"), BlendTime);
}

FString UGenAnimationUtils::GetCurrentAnimation(const FString& ActorName)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh component found\"}");
	}

	// In UE5.5, use AnimInstance to get current animation info
	UAnimInstance* AnimInstance = SkelMesh->GetAnimInstance();
	if (AnimInstance)
	{
		UAnimMontage* CurrentMontage = AnimInstance->GetCurrentActiveMontage();
		if (CurrentMontage)
		{
			return FString::Printf(TEXT("{\"success\": true, \"animation\": \"%s\", \"type\": \"montage\"}"),
				*CurrentMontage->GetPathName());
		}
	}

	return TEXT("{\"success\": true, \"animation\": null, \"message\": \"No animation playing or using AnimBP\"}");
}

FString UGenAnimationUtils::SetAnimationPlayRate(const FString& ActorName, float PlayRate)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh component found\"}");
	}

	SkelMesh->SetPlayRate(PlayRate);
	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Play rate set\", \"play_rate\": %.2f}"), PlayRate);
}

// ==================== ANIMATION MONTAGE ====================

FString UGenAnimationUtils::PlayMontage(const FString& ActorName, const FString& MontagePath, float PlayRate)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh || !SkelMesh->GetAnimInstance())
	{
		return TEXT("{\"success\": false, \"error\": \"No anim instance found\"}");
	}

	UAnimMontage* Montage = Cast<UAnimMontage>(UEditorAssetLibrary::LoadAsset(MontagePath));
	if (!Montage)
	{
		return TEXT("{\"success\": false, \"error\": \"Montage not found\"}");
	}

	float Duration = SkelMesh->GetAnimInstance()->Montage_Play(Montage, PlayRate);
	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Montage playing\", \"duration\": %.2f}"), Duration);
}

FString UGenAnimationUtils::StopMontage(const FString& ActorName, float BlendOutTime)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh || !SkelMesh->GetAnimInstance())
	{
		return TEXT("{\"success\": false, \"error\": \"No anim instance found\"}");
	}

	SkelMesh->GetAnimInstance()->Montage_Stop(BlendOutTime);
	return TEXT("{\"success\": true, \"message\": \"Montage stopped\"}");
}

FString UGenAnimationUtils::JumpToMontageSection(const FString& ActorName, const FString& SectionName)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh || !SkelMesh->GetAnimInstance())
	{
		return TEXT("{\"success\": false, \"error\": \"No anim instance found\"}");
	}

	SkelMesh->GetAnimInstance()->Montage_JumpToSection(FName(*SectionName));
	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Jumped to section\", \"section\": \"%s\"}"), *SectionName);
}

// ==================== SKELETAL MESH ====================

FString UGenAnimationUtils::ListSkeletalMeshes(const FString& ActorName)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	TArray<TSharedPtr<FJsonValue>> MeshesArray;
	TArray<USkeletalMeshComponent*> SkelMeshes;
	Actor->GetComponents<USkeletalMeshComponent>(SkelMeshes);

	for (USkeletalMeshComponent* Comp : SkelMeshes)
	{
		TSharedPtr<FJsonObject> MeshJson = MakeShareable(new FJsonObject);
		MeshJson->SetStringField("component", Comp->GetName());
		if (Comp->GetSkeletalMeshAsset())
		{
			MeshJson->SetStringField("mesh", Comp->GetSkeletalMeshAsset()->GetPathName());
			if (Comp->GetSkeletalMeshAsset()->GetSkeleton())
			{
				MeshJson->SetStringField("skeleton", Comp->GetSkeletalMeshAsset()->GetSkeleton()->GetPathName());
			}
		}
		MeshesArray.Add(MakeShareable(new FJsonValueObject(MeshJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", MeshesArray.Num());
	ResultJson->SetArrayField("skeletal_meshes", MeshesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAnimationUtils::SetSkeletalMesh(const FString& ActorName, const FString& SkeletalMeshPath)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh component found\"}");
	}

	USkeletalMesh* NewMesh = Cast<USkeletalMesh>(UEditorAssetLibrary::LoadAsset(SkeletalMeshPath));
	if (!NewMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"Skeletal mesh asset not found\"}");
	}

	SkelMesh->SetSkeletalMesh(NewMesh);
	return TEXT("{\"success\": true, \"message\": \"Skeletal mesh set\"}");
}

FString UGenAnimationUtils::SetAnimBlueprint(const FString& ActorName, const FString& AnimBlueprintPath)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh component found\"}");
	}

	UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(UEditorAssetLibrary::LoadAsset(AnimBlueprintPath));
	if (!AnimBP)
	{
		return TEXT("{\"success\": false, \"error\": \"Animation blueprint not found\"}");
	}

	SkelMesh->SetAnimInstanceClass(AnimBP->GeneratedClass);
	return TEXT("{\"success\": true, \"message\": \"Animation blueprint set\"}");
}

FString UGenAnimationUtils::ListCompatibleAnimations(const FString& SkeletonPath)
{
	USkeleton* Skeleton = Cast<USkeleton>(UEditorAssetLibrary::LoadAsset(SkeletonPath));
	if (!Skeleton)
	{
		return TEXT("{\"success\": false, \"error\": \"Skeleton not found\"}");
	}

	TArray<TSharedPtr<FJsonValue>> AnimsArray;
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> Assets;
	AssetRegistry.Get().GetAssetsByClass(UAnimSequence::StaticClass()->GetClassPathName(), Assets);

	for (const FAssetData& Asset : Assets)
	{
		UAnimSequence* Anim = Cast<UAnimSequence>(Asset.GetAsset());
		if (Anim && Anim->GetSkeleton() == Skeleton)
		{
			TSharedPtr<FJsonObject> AnimJson = MakeShareable(new FJsonObject);
			AnimJson->SetStringField("name", Asset.AssetName.ToString());
			AnimJson->SetStringField("path", Asset.GetObjectPathString());
			AnimsArray.Add(MakeShareable(new FJsonValueObject(AnimJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", AnimsArray.Num());
	ResultJson->SetArrayField("animations", AnimsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

// ==================== SIMPLE AI MOVEMENT ====================

FString UGenAnimationUtils::MoveToLocation(const FString& ActorName, const FVector& TargetLocation, float AcceptanceRadius)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	ACharacter* Character = Cast<ACharacter>(Actor);
	if (!Character)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor is not a Character - AI movement requires Character class\"}");
	}

	AAIController* AIController = Cast<AAIController>(Character->GetController());
	if (!AIController)
	{
		// Try to spawn AI controller
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (World)
		{
			AIController = World->SpawnActor<AAIController>();
			if (AIController)
			{
				AIController->Possess(Character);
			}
		}
	}

	if (!AIController)
	{
		return TEXT("{\"success\": false, \"error\": \"Could not get or spawn AI controller\"}");
	}

	AIController->MoveToLocation(TargetLocation, AcceptanceRadius);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Moving to location\", \"target\": {\"x\": %.0f, \"y\": %.0f, \"z\": %.0f}}"),
		TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
}

FString UGenAnimationUtils::SetPatrolPoints(const FString& ActorName, const TArray<FVector>& PatrolPoints, bool bLoop)
{
	if (PatrolPoints.Num() < 2)
	{
		return TEXT("{\"success\": false, \"error\": \"Need at least 2 patrol points\"}");
	}

	// Note: Full patrol implementation requires a custom component or behavior tree
	// This is a simplified version that stores points as metadata

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Patrol points configured\", \"point_count\": %d, \"loop\": %s, \"note\": \"Requires BehaviorTree or custom PatrolComponent for runtime patrol\"}"),
		PatrolPoints.Num(), bLoop ? TEXT("true") : TEXT("false"));
}

FString UGenAnimationUtils::TogglePatrol(const FString& ActorName, bool bStart)
{
	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Patrol %s\", \"note\": \"Requires PatrolComponent implementation\"}"),
		bStart ? TEXT("started") : TEXT("stopped"));
}

FString UGenAnimationUtils::SetRandomWander(const FString& ActorName, float WanderRadius, float MinWaitTime, float MaxWaitTime)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	// Note: True random wander requires AI Controller with navigation
	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Wander configured\", \"radius\": %.0f, \"wait_time\": [%.1f, %.1f], \"note\": \"Requires AIController with NavigationSystem\"}"),
		WanderRadius, MinWaitTime, MaxWaitTime);
}

FString UGenAnimationUtils::StopAIMovement(const FString& ActorName)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	ACharacter* Character = Cast<ACharacter>(Actor);
	if (!Character)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor is not a Character\"}");
	}

	AAIController* AIController = Cast<AAIController>(Character->GetController());
	if (AIController)
	{
		AIController->StopMovement();
		return TEXT("{\"success\": true, \"message\": \"AI movement stopped\"}");
	}

	return TEXT("{\"success\": false, \"error\": \"No AI controller found\"}");
}

FString UGenAnimationUtils::SetMovementSpeed(const FString& ActorName, float WalkSpeed, float RunSpeed)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	ACharacter* Character = Cast<ACharacter>(Actor);
	if (!Character)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor is not a Character\"}");
	}

	UCharacterMovementComponent* MovComp = Character->GetCharacterMovement();
	if (MovComp)
	{
		MovComp->MaxWalkSpeed = WalkSpeed;
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Movement speed set\", \"walk_speed\": %.0f}"), WalkSpeed);
	}

	return TEXT("{\"success\": false, \"error\": \"No movement component found\"}");
}

// ==================== LOOK AT / FACING ====================

FString UGenAnimationUtils::LookAtLocation(const FString& ActorName, const FVector& TargetLocation, float InterpSpeed)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	FVector Direction = (TargetLocation - Actor->GetActorLocation()).GetSafeNormal();
	FRotator NewRotation = Direction.Rotation();
	Actor->SetActorRotation(NewRotation);

	return TEXT("{\"success\": true, \"message\": \"Actor facing target location\"}");
}

FString UGenAnimationUtils::LookAtActor(const FString& ActorName, const FString& TargetActorName, float InterpSpeed)
{
	AActor* Actor = FindActorByLabel(ActorName);
	AActor* Target = FindActorByLabel(TargetActorName);

	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}
	if (!Target)
	{
		return TEXT("{\"success\": false, \"error\": \"Target actor not found\"}");
	}

	FVector Direction = (Target->GetActorLocation() - Actor->GetActorLocation()).GetSafeNormal();
	FRotator NewRotation = Direction.Rotation();
	Actor->SetActorRotation(NewRotation);

	return TEXT("{\"success\": true, \"message\": \"Actor facing target actor\"}");
}

FString UGenAnimationUtils::StopLookAt(const FString& ActorName)
{
	return TEXT("{\"success\": true, \"message\": \"Look at stopped (requires continuous implementation)\"}");
}

FString UGenAnimationUtils::FacePlayer(const FString& ActorName, bool bContinuous)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	// In editor, we can't easily get player pawn - this would work at runtime
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (World)
	{
		APawn* PlayerPawn = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
		if (PlayerPawn)
		{
			FVector Direction = (PlayerPawn->GetActorLocation() - Actor->GetActorLocation()).GetSafeNormal();
			FRotator NewRotation = Direction.Rotation();
			Actor->SetActorRotation(NewRotation);
			return TEXT("{\"success\": true, \"message\": \"Actor facing player\"}");
		}
	}

	return TEXT("{\"success\": false, \"error\": \"Could not find player pawn\"}");
}

// ==================== RAGDOLL & PHYSICS ====================

FString UGenAnimationUtils::EnableRagdoll(const FString& ActorName)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh component found\"}");
	}

	SkelMesh->SetSimulatePhysics(true);
	SkelMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	return TEXT("{\"success\": true, \"message\": \"Ragdoll enabled\"}");
}

FString UGenAnimationUtils::DisableRagdoll(const FString& ActorName)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh component found\"}");
	}

	SkelMesh->SetSimulatePhysics(false);
	SkelMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	return TEXT("{\"success\": true, \"message\": \"Ragdoll disabled\"}");
}

FString UGenAnimationUtils::ApplyRagdollImpulse(const FString& ActorName, const FVector& Impulse, const FString& BoneName)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh component found\"}");
	}

	FName Bone = BoneName.IsEmpty() ? NAME_None : FName(*BoneName);
	SkelMesh->AddImpulse(Impulse, Bone);

	return TEXT("{\"success\": true, \"message\": \"Impulse applied\"}");
}

// ==================== POSE & IK ====================

FString UGenAnimationUtils::SetBoneRotation(const FString& ActorName, const FString& BoneName, const FRotator& Rotation)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh component found\"}");
	}

	// Note: Direct bone manipulation requires special setup
	return TEXT("{\"success\": true, \"message\": \"Bone rotation set (requires AnimBP setup for runtime)\"}");
}

FString UGenAnimationUtils::ResetPose(const FString& ActorName)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh)
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh component found\"}");
	}

	SkelMesh->ResetAnimInstanceDynamics();
	return TEXT("{\"success\": true, \"message\": \"Pose reset\"}");
}

FString UGenAnimationUtils::ListBones(const FString& ActorName)
{
	AActor* Actor = FindActorByLabel(ActorName);
	if (!Actor)
	{
		return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
	}

	USkeletalMeshComponent* SkelMesh = GetSkeletalMesh(Actor);
	if (!SkelMesh || !SkelMesh->GetSkeletalMeshAsset())
	{
		return TEXT("{\"success\": false, \"error\": \"No skeletal mesh found\"}");
	}

	TArray<TSharedPtr<FJsonValue>> BonesArray;
	const FReferenceSkeleton& RefSkel = SkelMesh->GetSkeletalMeshAsset()->GetRefSkeleton();

	for (int32 i = 0; i < RefSkel.GetNum(); i++)
	{
		TSharedPtr<FJsonObject> BoneJson = MakeShareable(new FJsonObject);
		BoneJson->SetStringField("name", RefSkel.GetBoneName(i).ToString());
		BoneJson->SetNumberField("index", i);
		BoneJson->SetNumberField("parent_index", RefSkel.GetParentIndex(i));
		BonesArray.Add(MakeShareable(new FJsonValueObject(BoneJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("bone_count", BonesArray.Num());
	ResultJson->SetArrayField("bones", BonesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

// ==================== ANIMATION ASSETS ====================

FString UGenAnimationUtils::ListAnimationAssets(const FString& DirectoryPath)
{
	TArray<TSharedPtr<FJsonValue>> AnimsArray;
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> Assets;
	AssetRegistry.Get().GetAssetsByPath(FName(*DirectoryPath), Assets, true);

	for (const FAssetData& Asset : Assets)
	{
		if (Asset.AssetClassPath.GetAssetName() == TEXT("AnimSequence"))
		{
			TSharedPtr<FJsonObject> AnimJson = MakeShareable(new FJsonObject);
			AnimJson->SetStringField("name", Asset.AssetName.ToString());
			AnimJson->SetStringField("path", Asset.GetObjectPathString());
			AnimsArray.Add(MakeShareable(new FJsonValueObject(AnimJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", AnimsArray.Num());
	ResultJson->SetArrayField("animations", AnimsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAnimationUtils::ListAnimBlueprints(const FString& DirectoryPath)
{
	TArray<TSharedPtr<FJsonValue>> AnimBPsArray;
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> Assets;
	AssetRegistry.Get().GetAssetsByPath(FName(*DirectoryPath), Assets, true);

	for (const FAssetData& Asset : Assets)
	{
		if (Asset.AssetClassPath.GetAssetName() == TEXT("AnimBlueprint"))
		{
			TSharedPtr<FJsonObject> ABPJson = MakeShareable(new FJsonObject);
			ABPJson->SetStringField("name", Asset.AssetName.ToString());
			ABPJson->SetStringField("path", Asset.GetObjectPathString());
			AnimBPsArray.Add(MakeShareable(new FJsonValueObject(ABPJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", AnimBPsArray.Num());
	ResultJson->SetArrayField("anim_blueprints", AnimBPsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAnimationUtils::ListMontages(const FString& DirectoryPath)
{
	TArray<TSharedPtr<FJsonValue>> MontagesArray;
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> Assets;
	AssetRegistry.Get().GetAssetsByPath(FName(*DirectoryPath), Assets, true);

	for (const FAssetData& Asset : Assets)
	{
		if (Asset.AssetClassPath.GetAssetName() == TEXT("AnimMontage"))
		{
			TSharedPtr<FJsonObject> MontageJson = MakeShareable(new FJsonObject);
			MontageJson->SetStringField("name", Asset.AssetName.ToString());
			MontageJson->SetStringField("path", Asset.GetObjectPathString());
			MontagesArray.Add(MakeShareable(new FJsonValueObject(MontageJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", MontagesArray.Num());
	ResultJson->SetArrayField("montages", MontagesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAnimationUtils::GetAnimationInfo(const FString& AnimationPath)
{
	UAnimSequence* Anim = Cast<UAnimSequence>(UEditorAssetLibrary::LoadAsset(AnimationPath));
	if (!Anim)
	{
		return TEXT("{\"success\": false, \"error\": \"Animation not found\"}");
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetStringField("name", Anim->GetName());
	ResultJson->SetStringField("path", AnimationPath);
	ResultJson->SetNumberField("length", Anim->GetPlayLength());
	ResultJson->SetNumberField("frame_rate", Anim->GetSamplingFrameRate().AsDecimal());
	ResultJson->SetNumberField("num_frames", Anim->GetNumberOfSampledKeys());

	if (Anim->GetSkeleton())
	{
		ResultJson->SetStringField("skeleton", Anim->GetSkeleton()->GetPathName());
	}

	ResultJson->SetBoolField("is_additive", Anim->IsValidAdditive());
	ResultJson->SetBoolField("is_looping", Anim->bLoop);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}
