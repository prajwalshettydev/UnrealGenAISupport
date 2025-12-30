// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#include "MCP/GenAIUtils.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "EditorAssetLibrary.h"
#include "EnvironmentQuery/EnvQuery.h"

AActor* UGenAIUtils::FindActorByName(const FString& ActorName)
{
	if (!GEditor) return nullptr;

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return nullptr;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if ((*It)->GetActorLabel() == ActorName)
		{
			return *It;
		}
	}

	return nullptr;
}

UWorld* UGenAIUtils::GetPIEWorld()
{
	if (!GEditor) return nullptr;

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE)
		{
			return Context.World();
		}
	}

	return nullptr;
}

AActor* UGenAIUtils::FindActorInPIE(const FString& ActorName)
{
	UWorld* PIEWorld = GetPIEWorld();
	if (!PIEWorld) return nullptr;

	for (TActorIterator<AActor> It(PIEWorld); It; ++It)
	{
		if ((*It)->GetActorLabel() == ActorName)
		{
			return *It;
		}
	}

	return nullptr;
}

UNavigationSystemV1* UGenAIUtils::GetNavSystem()
{
	UWorld* World = nullptr;

	// Try PIE world first
	World = GetPIEWorld();
	if (!World && GEditor)
	{
		World = GEditor->GetEditorWorldContext().World();
	}

	if (!World) return nullptr;

	return FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
}

FString UGenAIUtils::ListAIControllers()
{
	if (!GEditor)
	{
		return TEXT("{\"success\": false, \"error\": \"GEditor not available\"}");
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	TArray<TSharedPtr<FJsonValue>> ControllersArray;

	for (TActorIterator<AAIController> It(World); It; ++It)
	{
		AAIController* Controller = *It;
		if (!Controller) continue;

		TSharedPtr<FJsonObject> ControllerJson = MakeShareable(new FJsonObject);
		ControllerJson->SetStringField("name", Controller->GetActorLabel());
		ControllerJson->SetStringField("class", Controller->GetClass()->GetName());

		APawn* Pawn = Controller->GetPawn();
		if (Pawn)
		{
			ControllerJson->SetStringField("pawn", Pawn->GetActorLabel());
			ControllerJson->SetStringField("pawn_class", Pawn->GetClass()->GetName());
		}

		ControllersArray.Add(MakeShareable(new FJsonValueObject(ControllerJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", ControllersArray.Num());
	ResultJson->SetArrayField("ai_controllers", ControllersArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::GetAIControllerInfo(const FString& ActorName)
{
	AActor* Actor = FindActorByName(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found\"}"), *ActorName);
	}

	APawn* Pawn = Cast<APawn>(Actor);
	AAIController* Controller = nullptr;

	if (Pawn)
	{
		Controller = Cast<AAIController>(Pawn->GetController());
	}
	else
	{
		Controller = Cast<AAIController>(Actor);
	}

	if (!Controller)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No AI Controller found for '%s'\"}"), *ActorName);
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetStringField("controller_class", Controller->GetClass()->GetName());

	// Behavior Tree info
	UBehaviorTreeComponent* BTComp = Controller->FindComponentByClass<UBehaviorTreeComponent>();
	if (BTComp)
	{
		UBehaviorTree* BT = BTComp->GetCurrentTree();
		if (BT)
		{
			ResultJson->SetStringField("behavior_tree", BT->GetPathName());
			ResultJson->SetBoolField("is_running", BTComp->IsRunning());
		}
	}

	// Blackboard info
	UBlackboardComponent* BBComp = Controller->GetBlackboardComponent();
	if (BBComp)
	{
		ResultJson->SetBoolField("has_blackboard", true);
	}

	// Perception info
	UAIPerceptionComponent* PerceptionComp = Controller->GetAIPerceptionComponent();
	if (PerceptionComp)
	{
		ResultJson->SetBoolField("has_perception", true);
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::SetAIController(const FString& ActorName, const FString& ControllerClassPath)
{
	AActor* Actor = FindActorByName(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found\"}"), *ActorName);
	}

	APawn* Pawn = Cast<APawn>(Actor);
	if (!Pawn)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' is not a Pawn\"}"), *ActorName);
	}

	UClass* ControllerClass = LoadObject<UClass>(nullptr, *ControllerClassPath);
	if (!ControllerClass || !ControllerClass->IsChildOf(AAIController::StaticClass()))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Invalid AI Controller class: '%s'\"}"), *ControllerClassPath);
	}

	Pawn->AIControllerClass = ControllerClass;
	Pawn->MarkPackageDirty();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"AI Controller class set for '%s'\"}"), *ActorName);
}

FString UGenAIUtils::ListBehaviorTrees(const FString& DirectoryPath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByPath(FName(*DirectoryPath), AssetList, true);

	TArray<TSharedPtr<FJsonValue>> TreesArray;

	for (const FAssetData& Asset : AssetList)
	{
		if (Asset.AssetClassPath == UBehaviorTree::StaticClass()->GetClassPathName())
		{
			TSharedPtr<FJsonObject> TreeJson = MakeShareable(new FJsonObject);
			TreeJson->SetStringField("name", Asset.AssetName.ToString());
			TreeJson->SetStringField("path", Asset.GetObjectPathString());

			TreesArray.Add(MakeShareable(new FJsonValueObject(TreeJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", TreesArray.Num());
	ResultJson->SetArrayField("behavior_trees", TreesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::GetBehaviorTreeInfo(const FString& BehaviorTreePath)
{
	UBehaviorTree* BT = LoadObject<UBehaviorTree>(nullptr, *BehaviorTreePath);
	if (!BT)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Behavior Tree not found at '%s'\"}"), *BehaviorTreePath);
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetStringField("path", BehaviorTreePath);
	ResultJson->SetStringField("name", BT->GetName());

	if (BT->BlackboardAsset)
	{
		ResultJson->SetStringField("blackboard", BT->BlackboardAsset->GetPathName());
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::SetBehaviorTree(const FString& ActorName, const FString& BehaviorTreePath)
{
	// This sets the default BT, actual running requires PIE
	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Use RunBehaviorTree in PIE to start BT for '%s'\"}"), *ActorName);
}

FString UGenAIUtils::RunBehaviorTree(const FString& ActorName, const FString& BehaviorTreePath)
{
	AActor* Actor = FindActorInPIE(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found in PIE. Start PIE first.\"}"), *ActorName);
	}

	APawn* Pawn = Cast<APawn>(Actor);
	AAIController* Controller = nullptr;

	if (Pawn)
	{
		Controller = Cast<AAIController>(Pawn->GetController());
	}

	if (!Controller)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No AI Controller for '%s'\"}"), *ActorName);
	}

	UBehaviorTree* BT = LoadObject<UBehaviorTree>(nullptr, *BehaviorTreePath);
	if (!BT)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Behavior Tree not found at '%s'\"}"), *BehaviorTreePath);
	}

	bool bStarted = Controller->RunBehaviorTree(BT);

	if (bStarted)
	{
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Behavior Tree started for '%s'\"}"), *ActorName);
	}

	return TEXT("{\"success\": false, \"error\": \"Failed to start Behavior Tree\"}");
}

FString UGenAIUtils::StopBehaviorTree(const FString& ActorName)
{
	AActor* Actor = FindActorInPIE(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found in PIE\"}"), *ActorName);
	}

	APawn* Pawn = Cast<APawn>(Actor);
	AAIController* Controller = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;

	if (!Controller)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No AI Controller for '%s'\"}"), *ActorName);
	}

	UBrainComponent* Brain = Controller->GetBrainComponent();
	if (Brain)
	{
		Brain->StopLogic(TEXT("Stopped via GenAIUtils"));
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Behavior Tree stopped for '%s'\"}"), *ActorName);
}

FString UGenAIUtils::ListBlackboards(const FString& DirectoryPath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByPath(FName(*DirectoryPath), AssetList, true);

	TArray<TSharedPtr<FJsonValue>> BBArray;

	for (const FAssetData& Asset : AssetList)
	{
		if (Asset.AssetClassPath == UBlackboardData::StaticClass()->GetClassPathName())
		{
			TSharedPtr<FJsonObject> BBJson = MakeShareable(new FJsonObject);
			BBJson->SetStringField("name", Asset.AssetName.ToString());
			BBJson->SetStringField("path", Asset.GetObjectPathString());

			BBArray.Add(MakeShareable(new FJsonValueObject(BBJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", BBArray.Num());
	ResultJson->SetArrayField("blackboards", BBArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::GetBlackboardKeys(const FString& BlackboardPath)
{
	UBlackboardData* BB = LoadObject<UBlackboardData>(nullptr, *BlackboardPath);
	if (!BB)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Blackboard not found at '%s'\"}"), *BlackboardPath);
	}

	TArray<TSharedPtr<FJsonValue>> KeysArray;

	for (const FBlackboardEntry& Key : BB->Keys)
	{
		TSharedPtr<FJsonObject> KeyJson = MakeShareable(new FJsonObject);
		KeyJson->SetStringField("name", Key.EntryName.ToString());
		KeyJson->SetStringField("type", Key.KeyType ? Key.KeyType->GetClass()->GetName() : TEXT("Unknown"));

		KeysArray.Add(MakeShareable(new FJsonValueObject(KeyJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", KeysArray.Num());
	ResultJson->SetArrayField("keys", KeysArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::GetBlackboardValue(const FString& ActorName, const FString& KeyName)
{
	AActor* Actor = FindActorInPIE(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found in PIE\"}"), *ActorName);
	}

	APawn* Pawn = Cast<APawn>(Actor);
	AAIController* Controller = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;

	if (!Controller)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No AI Controller for '%s'\"}"), *ActorName);
	}

	UBlackboardComponent* BBComp = Controller->GetBlackboardComponent();
	if (!BBComp)
	{
		return TEXT("{\"success\": false, \"error\": \"No Blackboard component\"}");
	}

	FBlackboard::FKey KeyID = BBComp->GetKeyID(FName(*KeyName));
	if (KeyID == FBlackboard::InvalidKey)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Key '%s' not found\"}"), *KeyName);
	}

	FString ValueStr = BBComp->DescribeKeyValue(KeyID, EBlackboardDescription::Full);

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetStringField("key", KeyName);
	ResultJson->SetStringField("value", ValueStr);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::SetBlackboardValue(const FString& ActorName, const FString& KeyName, const FString& Value)
{
	AActor* Actor = FindActorInPIE(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found in PIE\"}"), *ActorName);
	}

	APawn* Pawn = Cast<APawn>(Actor);
	AAIController* Controller = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;

	if (!Controller)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No AI Controller for '%s'\"}"), *ActorName);
	}

	UBlackboardComponent* BBComp = Controller->GetBlackboardComponent();
	if (!BBComp)
	{
		return TEXT("{\"success\": false, \"error\": \"No Blackboard component\"}");
	}

	// Try to set as different types
	FName KeyFName(*KeyName);

	// Try bool
	if (Value.Equals(TEXT("true"), ESearchCase::IgnoreCase))
	{
		BBComp->SetValueAsBool(KeyFName, true);
	}
	else if (Value.Equals(TEXT("false"), ESearchCase::IgnoreCase))
	{
		BBComp->SetValueAsBool(KeyFName, false);
	}
	else if (Value.IsNumeric())
	{
		// Try float
		float FloatVal = FCString::Atof(*Value);
		BBComp->SetValueAsFloat(KeyFName, FloatVal);
	}
	else
	{
		// Try string/name
		BBComp->SetValueAsString(KeyFName, Value);
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Blackboard key '%s' set\"}"), *KeyName);
}

FString UGenAIUtils::IsLocationNavigable(const FVector& Location)
{
	UNavigationSystemV1* NavSys = GetNavSystem();
	if (!NavSys)
	{
		return TEXT("{\"success\": false, \"error\": \"No navigation system\"}");
	}

	FNavLocation NavLoc;
	bool bNavigable = NavSys->ProjectPointToNavigation(Location, NavLoc);

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetBoolField("navigable", bNavigable);

	if (bNavigable)
	{
		TSharedPtr<FJsonObject> LocJson = MakeShareable(new FJsonObject);
		LocJson->SetNumberField("x", NavLoc.Location.X);
		LocJson->SetNumberField("y", NavLoc.Location.Y);
		LocJson->SetNumberField("z", NavLoc.Location.Z);
		ResultJson->SetObjectField("projected_location", LocJson);
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::FindPath(const FVector& StartLocation, const FVector& EndLocation)
{
	UNavigationSystemV1* NavSys = GetNavSystem();
	if (!NavSys)
	{
		return TEXT("{\"success\": false, \"error\": \"No navigation system\"}");
	}

	FPathFindingQuery Query;
	Query.StartLocation = StartLocation;
	Query.EndLocation = EndLocation;

	FPathFindingResult Result = NavSys->FindPathSync(Query);

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetBoolField("path_found", Result.IsSuccessful());

	if (Result.IsSuccessful() && Result.Path.IsValid())
	{
		TArray<TSharedPtr<FJsonValue>> PointsArray;
		for (const FNavPathPoint& Point : Result.Path->GetPathPoints())
		{
			TSharedPtr<FJsonObject> PointJson = MakeShareable(new FJsonObject);
			PointJson->SetNumberField("x", Point.Location.X);
			PointJson->SetNumberField("y", Point.Location.Y);
			PointJson->SetNumberField("z", Point.Location.Z);
			PointsArray.Add(MakeShareable(new FJsonValueObject(PointJson)));
		}
		ResultJson->SetArrayField("path_points", PointsArray);
		ResultJson->SetNumberField("path_length", Result.Path->GetLength());
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::GetRandomNavigablePoint(const FVector& Origin, float Radius)
{
	UNavigationSystemV1* NavSys = GetNavSystem();
	if (!NavSys)
	{
		return TEXT("{\"success\": false, \"error\": \"No navigation system\"}");
	}

	FNavLocation RandomLoc;
	bool bFound = NavSys->GetRandomReachablePointInRadius(Origin, Radius, RandomLoc);

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetBoolField("found", bFound);

	if (bFound)
	{
		TSharedPtr<FJsonObject> LocJson = MakeShareable(new FJsonObject);
		LocJson->SetNumberField("x", RandomLoc.Location.X);
		LocJson->SetNumberField("y", RandomLoc.Location.Y);
		LocJson->SetNumberField("z", RandomLoc.Location.Z);
		ResultJson->SetObjectField("location", LocJson);
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::ProjectPointToNavigation(const FVector& Location, const FVector& QueryExtent)
{
	UNavigationSystemV1* NavSys = GetNavSystem();
	if (!NavSys)
	{
		return TEXT("{\"success\": false, \"error\": \"No navigation system\"}");
	}

	FNavLocation NavLoc;
	bool bProjected = NavSys->ProjectPointToNavigation(Location, NavLoc, QueryExtent);

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetBoolField("projected", bProjected);

	if (bProjected)
	{
		TSharedPtr<FJsonObject> LocJson = MakeShareable(new FJsonObject);
		LocJson->SetNumberField("x", NavLoc.Location.X);
		LocJson->SetNumberField("y", NavLoc.Location.Y);
		LocJson->SetNumberField("z", NavLoc.Location.Z);
		ResultJson->SetObjectField("location", LocJson);
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::MoveToLocation(const FString& ActorName, const FVector& TargetLocation, float AcceptanceRadius)
{
	AActor* Actor = FindActorInPIE(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found in PIE\"}"), *ActorName);
	}

	APawn* Pawn = Cast<APawn>(Actor);
	AAIController* Controller = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;

	if (!Controller)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No AI Controller for '%s'\"}"), *ActorName);
	}

	EPathFollowingRequestResult::Type Result = Controller->MoveToLocation(TargetLocation, AcceptanceRadius);

	FString ResultStr;
	switch (Result)
	{
	case EPathFollowingRequestResult::RequestSuccessful:
		ResultStr = TEXT("RequestSuccessful");
		break;
	case EPathFollowingRequestResult::AlreadyAtGoal:
		ResultStr = TEXT("AlreadyAtGoal");
		break;
	default:
		ResultStr = TEXT("Failed");
	}

	return FString::Printf(TEXT("{\"success\": true, \"result\": \"%s\"}"), *ResultStr);
}

FString UGenAIUtils::MoveToActor(const FString& ActorName, const FString& TargetActorName, float AcceptanceRadius)
{
	AActor* Actor = FindActorInPIE(ActorName);
	AActor* TargetActor = FindActorInPIE(TargetActorName);

	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found in PIE\"}"), *ActorName);
	}

	if (!TargetActor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Target actor '%s' not found in PIE\"}"), *TargetActorName);
	}

	APawn* Pawn = Cast<APawn>(Actor);
	AAIController* Controller = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;

	if (!Controller)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No AI Controller for '%s'\"}"), *ActorName);
	}

	EPathFollowingRequestResult::Type Result = Controller->MoveToActor(TargetActor, AcceptanceRadius);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Moving to actor '%s'\"}"), *TargetActorName);
}

FString UGenAIUtils::StopMovement(const FString& ActorName)
{
	AActor* Actor = FindActorInPIE(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found in PIE\"}"), *ActorName);
	}

	APawn* Pawn = Cast<APawn>(Actor);
	AAIController* Controller = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;

	if (!Controller)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No AI Controller for '%s'\"}"), *ActorName);
	}

	Controller->StopMovement();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Movement stopped for '%s'\"}"), *ActorName);
}

FString UGenAIUtils::GetMovementStatus(const FString& ActorName)
{
	AActor* Actor = FindActorInPIE(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found in PIE\"}"), *ActorName);
	}

	APawn* Pawn = Cast<APawn>(Actor);
	AAIController* Controller = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;

	if (!Controller)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No AI Controller for '%s'\"}"), *ActorName);
	}

	EPathFollowingStatus::Type Status = Controller->GetMoveStatus();

	FString StatusStr;
	switch (Status)
	{
	case EPathFollowingStatus::Idle:
		StatusStr = TEXT("Idle");
		break;
	case EPathFollowingStatus::Waiting:
		StatusStr = TEXT("Waiting");
		break;
	case EPathFollowingStatus::Paused:
		StatusStr = TEXT("Paused");
		break;
	case EPathFollowingStatus::Moving:
		StatusStr = TEXT("Moving");
		break;
	default:
		StatusStr = TEXT("Unknown");
	}

	return FString::Printf(TEXT("{\"success\": true, \"status\": \"%s\"}"), *StatusStr);
}

FString UGenAIUtils::GetPerceptionInfo(const FString& ActorName)
{
	AActor* Actor = FindActorByName(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found\"}"), *ActorName);
	}

	UAIPerceptionComponent* PerceptionComp = Actor->FindComponentByClass<UAIPerceptionComponent>();
	if (!PerceptionComp)
	{
		// Check on controller
		APawn* Pawn = Cast<APawn>(Actor);
		if (Pawn)
		{
			AAIController* Controller = Cast<AAIController>(Pawn->GetController());
			if (Controller)
			{
				PerceptionComp = Controller->GetAIPerceptionComponent();
			}
		}
	}

	if (!PerceptionComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No perception component for '%s'\"}"), *ActorName);
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetBoolField("has_perception", true);

	// List configured senses
	TArray<TSharedPtr<FJsonValue>> SensesArray;
	for (auto It = PerceptionComp->GetSensesConfigIterator(); It; ++It)
	{
		UAISenseConfig* Config = *It;
		if (Config)
		{
			TSharedPtr<FJsonObject> SenseJson = MakeShareable(new FJsonObject);
			SenseJson->SetStringField("type", Config->GetClass()->GetName());

			// Sight specific
			if (UAISenseConfig_Sight* SightConfig = Cast<UAISenseConfig_Sight>(Config))
			{
				SenseJson->SetNumberField("sight_radius", SightConfig->SightRadius);
				SenseJson->SetNumberField("lose_sight_radius", SightConfig->LoseSightRadius);
				SenseJson->SetNumberField("peripheral_vision_angle", SightConfig->PeripheralVisionAngleDegrees);
			}

			// Hearing specific
			if (UAISenseConfig_Hearing* HearingConfig = Cast<UAISenseConfig_Hearing>(Config))
			{
				SenseJson->SetNumberField("hearing_range", HearingConfig->HearingRange);
			}

			SensesArray.Add(MakeShareable(new FJsonValueObject(SenseJson)));
		}
	}
	ResultJson->SetArrayField("senses", SensesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::GetPerceivedActors(const FString& ActorName)
{
	AActor* Actor = FindActorInPIE(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found in PIE\"}"), *ActorName);
	}

	UAIPerceptionComponent* PerceptionComp = Actor->FindComponentByClass<UAIPerceptionComponent>();
	if (!PerceptionComp)
	{
		APawn* Pawn = Cast<APawn>(Actor);
		if (Pawn)
		{
			AAIController* Controller = Cast<AAIController>(Pawn->GetController());
			if (Controller)
			{
				PerceptionComp = Controller->GetAIPerceptionComponent();
			}
		}
	}

	if (!PerceptionComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No perception component for '%s'\"}"), *ActorName);
	}

	TArray<AActor*> PerceivedActors;
	PerceptionComp->GetCurrentlyPerceivedActors(nullptr, PerceivedActors);

	TArray<TSharedPtr<FJsonValue>> ActorsArray;
	for (AActor* Perceived : PerceivedActors)
	{
		if (Perceived)
		{
			TSharedPtr<FJsonObject> ActorJson = MakeShareable(new FJsonObject);
			ActorJson->SetStringField("name", Perceived->GetActorLabel());
			ActorJson->SetStringField("class", Perceived->GetClass()->GetName());

			FVector Loc = Perceived->GetActorLocation();
			ActorJson->SetStringField("location", FString::Printf(TEXT("(%f, %f, %f)"), Loc.X, Loc.Y, Loc.Z));

			ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", ActorsArray.Num());
	ResultJson->SetArrayField("perceived_actors", ActorsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::ConfigureSightSense(const FString& ActorName, float SightRadius,
										 float LoseSightRadius, float PeripheralVisionAngle)
{
	// This requires modifying component config which is complex
	return TEXT("{\"success\": false, \"error\": \"Sight configuration requires Blueprint/C++ modification\"}");
}

FString UGenAIUtils::ConfigureHearingSense(const FString& ActorName, float HearingRange)
{
	return TEXT("{\"success\": false, \"error\": \"Hearing configuration requires Blueprint/C++ modification\"}");
}

FString UGenAIUtils::ListEQSQueries(const FString& DirectoryPath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByPath(FName(*DirectoryPath), AssetList, true);

	TArray<TSharedPtr<FJsonValue>> QueriesArray;

	for (const FAssetData& Asset : AssetList)
	{
		if (Asset.AssetClassPath == UEnvQuery::StaticClass()->GetClassPathName())
		{
			TSharedPtr<FJsonObject> QueryJson = MakeShareable(new FJsonObject);
			QueryJson->SetStringField("name", Asset.AssetName.ToString());
			QueryJson->SetStringField("path", Asset.GetObjectPathString());

			QueriesArray.Add(MakeShareable(new FJsonValueObject(QueryJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", QueriesArray.Num());
	ResultJson->SetArrayField("eqs_queries", QueriesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::RunEQSQuery(const FString& ActorName, const FString& QueryPath)
{
	return TEXT("{\"success\": false, \"error\": \"EQS query execution not yet implemented\"}");
}

FString UGenAIUtils::ListSmartObjects()
{
	return TEXT("{\"success\": false, \"error\": \"Smart Objects not yet implemented\"}");
}

FString UGenAIUtils::FindSmartObjects(const FString& ActorName, const FString& TagQuery)
{
	return TEXT("{\"success\": false, \"error\": \"Smart Objects not yet implemented\"}");
}

FString UGenAIUtils::GetAvoidanceInfo(const FString& ActorName)
{
	AActor* Actor = FindActorByName(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found\"}"), *ActorName);
	}

	ACharacter* Character = Cast<ACharacter>(Actor);
	if (!Character)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' is not a Character\"}"), *ActorName);
	}

	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (!MovementComp)
	{
		return TEXT("{\"success\": false, \"error\": \"No movement component\"}");
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetBoolField("use_rvo_avoidance", MovementComp->bUseRVOAvoidance);
	ResultJson->SetNumberField("avoidance_weight", MovementComp->AvoidanceWeight);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenAIUtils::SetAvoidanceSettings(const FString& ActorName, bool bEnableAvoidance, float AvoidanceWeight)
{
	AActor* Actor = FindActorByName(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found\"}"), *ActorName);
	}

	ACharacter* Character = Cast<ACharacter>(Actor);
	if (!Character)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' is not a Character\"}"), *ActorName);
	}

	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (!MovementComp)
	{
		return TEXT("{\"success\": false, \"error\": \"No movement component\"}");
	}

	MovementComp->bUseRVOAvoidance = bEnableAvoidance;
	MovementComp->AvoidanceWeight = FMath::Clamp(AvoidanceWeight, 0.0f, 1.0f);
	Actor->MarkPackageDirty();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Avoidance settings updated for '%s'\"}"), *ActorName);
}
