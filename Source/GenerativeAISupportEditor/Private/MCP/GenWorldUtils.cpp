// GenWorldUtils.cpp - World Modding Utilities Implementation
// Part of GenerativeAISupport Plugin - The Swiss Army Knife for Unreal Engine
// 2025-12-30

#include "MCP/GenWorldUtils.h"
#include "Editor.h"
#include "EditorLevelLibrary.h"
#include "LevelEditor.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

// Lighting
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/DirectionalLight.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/DirectionalLightComponent.h"

// Volumes
#include "Engine/TriggerBox.h"
#include "Engine/BlockingVolume.h"
#include "Engine/PostProcessVolume.h"
#include "Sound/AudioVolume.h"

// Foliage
#include "InstancedFoliageActor.h"
#include "FoliageType.h"

// Splines
#include "Components/SplineComponent.h"

// Assets
#include "EditorAssetLibrary.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "Factories/Factory.h"

// Levels
#include "LevelUtils.h"
#include "EditorLevelUtils.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingAlwaysLoaded.h"

// JSON
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

// Viewport
#include "LevelEditorViewport.h"
#include "SLevelViewport.h"

// Selection
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "Editor/EditorEngine.h"
#include "Engine/Selection.h"

// Screenshot
#include "HighResScreenshot.h"
#include "Slate/SceneViewport.h"

// ==================== LIGHTING ====================

FString UGenWorldUtils::SpawnPointLight(const FVector& Location, float Intensity, const FLinearColor& Color, const FString& Label)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	APointLight* Light = World->SpawnActor<APointLight>(Location, FRotator::ZeroRotator);
	if (!Light)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to spawn point light\"}");
	}

	Light->PointLightComponent->SetIntensity(Intensity);
	Light->PointLightComponent->SetLightColor(Color);

	if (!Label.IsEmpty())
	{
		Light->SetActorLabel(*Label);
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Point light spawned\", \"actor\": \"%s\"}"), *Light->GetActorLabel());
}

FString UGenWorldUtils::SpawnSpotLight(const FVector& Location, const FRotator& Rotation, float Intensity, float InnerAngle, float OuterAngle, const FLinearColor& Color, const FString& Label)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	ASpotLight* Light = World->SpawnActor<ASpotLight>(Location, Rotation);
	if (!Light)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to spawn spot light\"}");
	}

	Light->SpotLightComponent->SetIntensity(Intensity);
	Light->SpotLightComponent->SetLightColor(Color);
	Light->SpotLightComponent->SetInnerConeAngle(InnerAngle);
	Light->SpotLightComponent->SetOuterConeAngle(OuterAngle);

	if (!Label.IsEmpty())
	{
		Light->SetActorLabel(*Label);
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Spot light spawned\", \"actor\": \"%s\"}"), *Light->GetActorLabel());
}

FString UGenWorldUtils::SpawnDirectionalLight(const FRotator& Rotation, float Intensity, const FLinearColor& Color, const FString& Label)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(FVector::ZeroVector, Rotation);
	if (!Light)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to spawn directional light\"}");
	}

	Light->GetComponent()->SetIntensity(Intensity);
	Light->GetComponent()->SetLightColor(Color);

	if (!Label.IsEmpty())
	{
		Light->SetActorLabel(*Label);
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Directional light spawned\", \"actor\": \"%s\"}"), *Light->GetActorLabel());
}

FString UGenWorldUtils::SetLightProperties(const FString& ActorName, float Intensity, const FLinearColor& Color, float AttenuationRadius)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	for (TActorIterator<ALight> It(World); It; ++It)
	{
		if (It->GetActorLabel() == ActorName)
		{
			ULightComponent* LightComp = It->GetLightComponent();
			if (LightComp)
			{
				LightComp->SetIntensity(Intensity);
				LightComp->SetLightColor(Color);
				if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(LightComp))
				{
					PointLight->SetAttenuationRadius(AttenuationRadius);
				}
				return TEXT("{\"success\": true, \"message\": \"Light properties updated\"}");
			}
		}
	}

	return TEXT("{\"success\": false, \"error\": \"Light not found\"}");
}

// ==================== VOLUMES ====================

FString UGenWorldUtils::SpawnTriggerBox(const FVector& Location, const FVector& BoxExtent, const FString& Label)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	ATriggerBox* Trigger = World->SpawnActor<ATriggerBox>(Location, FRotator::ZeroRotator);
	if (!Trigger)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to spawn trigger box\"}");
	}

	// Set size via brush (trigger boxes use brush component)
	Trigger->SetActorScale3D(BoxExtent / 100.0f); // Approximate scaling

	if (!Label.IsEmpty())
	{
		Trigger->SetActorLabel(*Label);
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Trigger box spawned\", \"actor\": \"%s\"}"), *Trigger->GetActorLabel());
}

FString UGenWorldUtils::SpawnBlockingVolume(const FVector& Location, const FVector& BoxExtent, const FString& Label)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	ABlockingVolume* Volume = World->SpawnActor<ABlockingVolume>(Location, FRotator::ZeroRotator);
	if (!Volume)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to spawn blocking volume\"}");
	}

	Volume->SetActorScale3D(BoxExtent / 100.0f);

	if (!Label.IsEmpty())
	{
		Volume->SetActorLabel(*Label);
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Blocking volume spawned\", \"actor\": \"%s\"}"), *Volume->GetActorLabel());
}

FString UGenWorldUtils::SpawnPostProcessVolume(const FVector& Location, const FVector& BoxExtent, bool bUnbound, const FString& Label)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	APostProcessVolume* Volume = World->SpawnActor<APostProcessVolume>(Location, FRotator::ZeroRotator);
	if (!Volume)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to spawn post process volume\"}");
	}

	Volume->bUnbound = bUnbound;
	Volume->SetActorScale3D(BoxExtent / 100.0f);

	if (!Label.IsEmpty())
	{
		Volume->SetActorLabel(*Label);
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Post process volume spawned\", \"actor\": \"%s\"}"), *Volume->GetActorLabel());
}

FString UGenWorldUtils::SpawnAudioVolume(const FVector& Location, const FVector& BoxExtent, const FString& Label)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	AAudioVolume* Volume = World->SpawnActor<AAudioVolume>(Location, FRotator::ZeroRotator);
	if (!Volume)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to spawn audio volume\"}");
	}

	Volume->SetActorScale3D(BoxExtent / 100.0f);

	if (!Label.IsEmpty())
	{
		Volume->SetActorLabel(*Label);
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Audio volume spawned\", \"actor\": \"%s\"}"), *Volume->GetActorLabel());
}

// ==================== FOLIAGE ====================

FString UGenWorldUtils::AddFoliageAtLocations(const FString& FoliageTypePath, const TArray<FVector>& Locations)
{
	// Foliage requires more complex setup - simplified version
	return TEXT("{\"success\": false, \"error\": \"Foliage spawning requires manual setup - use Foliage Mode in editor\"}");
}

FString UGenWorldUtils::RemoveFoliageInRadius(const FVector& Center, float Radius)
{
	return TEXT("{\"success\": false, \"error\": \"Foliage removal requires manual setup\"}");
}

FString UGenWorldUtils::ListFoliageTypes()
{
	TArray<TSharedPtr<FJsonValue>> TypesArray;

	// List assets of type UFoliageType
	TArray<FAssetData> Assets;
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().GetAssetsByClass(UFoliageType::StaticClass()->GetClassPathName(), Assets);

	for (const FAssetData& Asset : Assets)
	{
		TSharedPtr<FJsonObject> TypeJson = MakeShareable(new FJsonObject);
		TypeJson->SetStringField("name", Asset.AssetName.ToString());
		TypeJson->SetStringField("path", Asset.GetObjectPathString());
		TypesArray.Add(MakeShareable(new FJsonValueObject(TypeJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", TypesArray.Num());
	ResultJson->SetArrayField("foliage_types", TypesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

// ==================== SPLINES ====================

FString UGenWorldUtils::CreateSpline(const TArray<FVector>& Points, bool bClosedLoop, const FString& Label)
{
	if (Points.Num() < 2)
	{
		return TEXT("{\"success\": false, \"error\": \"Need at least 2 points for spline\"}");
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	AActor* SplineActor = World->SpawnActor<AActor>(Points[0], FRotator::ZeroRotator);
	if (!SplineActor)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to spawn spline actor\"}");
	}

	USplineComponent* Spline = NewObject<USplineComponent>(SplineActor);
	Spline->RegisterComponent();
	SplineActor->SetRootComponent(Spline);

	Spline->ClearSplinePoints();
	for (int32 i = 0; i < Points.Num(); i++)
	{
		Spline->AddSplinePoint(Points[i], ESplineCoordinateSpace::World);
	}
	Spline->SetClosedLoop(bClosedLoop);

	if (!Label.IsEmpty())
	{
		SplineActor->SetActorLabel(*Label);
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Spline created with %d points\", \"actor\": \"%s\"}"),
		Points.Num(), *SplineActor->GetActorLabel());
}

FString UGenWorldUtils::AddSplinePoint(const FString& SplineActorName, const FVector& Point, int32 Index)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetActorLabel() == SplineActorName)
		{
			USplineComponent* Spline = It->FindComponentByClass<USplineComponent>();
			if (Spline)
			{
				Spline->AddSplinePointAtIndex(Point, Index, ESplineCoordinateSpace::World);
				return TEXT("{\"success\": true, \"message\": \"Spline point added\"}");
			}
		}
	}

	return TEXT("{\"success\": false, \"error\": \"Spline actor not found\"}");
}

// ==================== ASSET MANAGEMENT ====================

FString UGenWorldUtils::ImportAsset(const FString& FilePath, const FString& DestinationPath)
{
	// Asset import is complex and depends on file type
	// For now, return info about using the import dialog
	return TEXT("{\"success\": false, \"error\": \"Use UEditorAssetLibrary or Import Dialog for asset import\"}");
}

FString UGenWorldUtils::DuplicateAsset(const FString& SourcePath, const FString& DestinationPath, const FString& NewName)
{
	UObject* DuplicatedAsset = UEditorAssetLibrary::DuplicateAsset(SourcePath, DestinationPath + TEXT("/") + NewName);
	if (DuplicatedAsset)
	{
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Asset duplicated\", \"new_path\": \"%s/%s\"}"),
			*DestinationPath, *NewName);
	}
	return TEXT("{\"success\": false, \"error\": \"Failed to duplicate asset\"}");
}

FString UGenWorldUtils::DeleteAsset(const FString& AssetPath)
{
	bool bDeleted = UEditorAssetLibrary::DeleteAsset(AssetPath);
	if (bDeleted)
	{
		return TEXT("{\"success\": true, \"message\": \"Asset deleted\"}");
	}
	return TEXT("{\"success\": false, \"error\": \"Failed to delete asset\"}");
}

FString UGenWorldUtils::RenameAsset(const FString& AssetPath, const FString& NewName)
{
	FString Directory = FPaths::GetPath(AssetPath);
	FString NewPath = Directory + TEXT("/") + NewName;

	bool bRenamed = UEditorAssetLibrary::RenameAsset(AssetPath, NewPath);
	if (bRenamed)
	{
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Asset renamed\", \"new_path\": \"%s\"}"), *NewPath);
	}
	return TEXT("{\"success\": false, \"error\": \"Failed to rename asset\"}");
}

FString UGenWorldUtils::ListAssets(const FString& DirectoryPath, const FString& ClassFilter)
{
	TArray<FString> AssetPaths = UEditorAssetLibrary::ListAssets(DirectoryPath);

	TArray<TSharedPtr<FJsonValue>> AssetsArray;
	for (const FString& Path : AssetPaths)
	{
		if (!ClassFilter.IsEmpty())
		{
			FAssetData AssetData = UEditorAssetLibrary::FindAssetData(Path);
			if (!AssetData.AssetClassPath.GetAssetName().ToString().Contains(ClassFilter))
			{
				continue;
			}
		}

		TSharedPtr<FJsonObject> AssetJson = MakeShareable(new FJsonObject);
		AssetJson->SetStringField("path", Path);

		FAssetData Data = UEditorAssetLibrary::FindAssetData(Path);
		AssetJson->SetStringField("name", Data.AssetName.ToString());
		AssetJson->SetStringField("class", Data.AssetClassPath.GetAssetName().ToString());

		AssetsArray.Add(MakeShareable(new FJsonValueObject(AssetJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", AssetsArray.Num());
	ResultJson->SetArrayField("assets", AssetsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

// ==================== LEVEL MANAGEMENT ====================

FString UGenWorldUtils::CreateSubLevel(const FString& LevelName, const FString& SavePath)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	FString FullPath = SavePath + TEXT("/") + LevelName;
	ULevelStreaming* NewLevel = UEditorLevelUtils::AddLevelToWorld(World, *FullPath, ULevelStreamingAlwaysLoaded::StaticClass());

	if (NewLevel)
	{
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Sub-level created\", \"path\": \"%s\"}"), *FullPath);
	}

	return TEXT("{\"success\": false, \"error\": \"Failed to create sub-level\"}");
}

FString UGenWorldUtils::LoadSubLevel(const FString& LevelPath)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	ULevelStreaming* StreamingLevel = UEditorLevelUtils::AddLevelToWorld(World, *LevelPath, ULevelStreamingAlwaysLoaded::StaticClass());
	if (StreamingLevel)
	{
		return TEXT("{\"success\": true, \"message\": \"Sub-level loaded\"}");
	}

	return TEXT("{\"success\": false, \"error\": \"Failed to load sub-level\"}");
}

FString UGenWorldUtils::UnloadSubLevel(const FString& LevelPath)
{
	// Level unloading implementation
	return TEXT("{\"success\": false, \"error\": \"Use World Partition or Level Browser for unloading\"}");
}

FString UGenWorldUtils::ListLevels()
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	TArray<TSharedPtr<FJsonValue>> LevelsArray;

	const TArray<ULevel*>& Levels = World->GetLevels();
	for (ULevel* Level : Levels)
	{
		if (Level)
		{
			TSharedPtr<FJsonObject> LevelJson = MakeShareable(new FJsonObject);
			LevelJson->SetStringField("name", Level->GetOuter()->GetName());
			LevelJson->SetBoolField("is_current", Level == World->GetCurrentLevel());
			LevelJson->SetBoolField("is_visible", Level->bIsVisible);
			LevelsArray.Add(MakeShareable(new FJsonValueObject(LevelJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", LevelsArray.Num());
	ResultJson->SetArrayField("levels", LevelsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

// ==================== CAMERA & VIEWPORT ====================

FString UGenWorldUtils::SetEditorCamera(const FVector& Location, const FRotator& Rotation)
{
	if (GEditor && GEditor->GetActiveViewport())
	{
		FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
		if (ViewportClient)
		{
			ViewportClient->SetViewLocation(Location);
			ViewportClient->SetViewRotation(Rotation);
			return TEXT("{\"success\": true, \"message\": \"Editor camera moved\"}");
		}
	}
	return TEXT("{\"success\": false, \"error\": \"No active viewport\"}");
}

FString UGenWorldUtils::GetEditorCamera()
{
	if (GEditor && GEditor->GetActiveViewport())
	{
		FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
		if (ViewportClient)
		{
			FVector Loc = ViewportClient->GetViewLocation();
			FRotator Rot = ViewportClient->GetViewRotation();

			TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
			ResultJson->SetBoolField("success", true);

			TSharedPtr<FJsonObject> LocJson = MakeShareable(new FJsonObject);
			LocJson->SetNumberField("x", Loc.X);
			LocJson->SetNumberField("y", Loc.Y);
			LocJson->SetNumberField("z", Loc.Z);
			ResultJson->SetObjectField("location", LocJson);

			TSharedPtr<FJsonObject> RotJson = MakeShareable(new FJsonObject);
			RotJson->SetNumberField("pitch", Rot.Pitch);
			RotJson->SetNumberField("yaw", Rot.Yaw);
			RotJson->SetNumberField("roll", Rot.Roll);
			ResultJson->SetObjectField("rotation", RotJson);

			FString OutputString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
			FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

			return OutputString;
		}
	}
	return TEXT("{\"success\": false, \"error\": \"No active viewport\"}");
}

FString UGenWorldUtils::FocusOnActor(const FString& ActorName)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetActorLabel() == ActorName)
		{
			GEditor->MoveViewportCamerasToActor(**It, false);
			return TEXT("{\"success\": true, \"message\": \"Camera focused on actor\"}");
		}
	}

	return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
}

// ==================== SELECTION ====================

FString UGenWorldUtils::SelectActors(const FString& NamePattern)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	GEditor->SelectNone(true, true);

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetActorLabel().Contains(NamePattern))
		{
			GEditor->SelectActor(*It, true, true);
			Count++;
		}
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Selected %d actors\", \"count\": %d}"), Count, Count);
}

FString UGenWorldUtils::GetSelectedActors()
{
	TArray<TSharedPtr<FJsonValue>> ActorsArray;

	USelection* Selection = GEditor->GetSelectedActors();
	if (Selection)
	{
		for (int32 i = 0; i < Selection->Num(); i++)
		{
			AActor* Actor = Cast<AActor>(Selection->GetSelectedObject(i));
			if (Actor)
			{
				TSharedPtr<FJsonObject> ActorJson = MakeShareable(new FJsonObject);
				ActorJson->SetStringField("name", Actor->GetActorLabel());
				ActorJson->SetStringField("class", Actor->GetClass()->GetName());
				ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorJson)));
			}
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", ActorsArray.Num());
	ResultJson->SetArrayField("selected", ActorsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenWorldUtils::ClearSelection()
{
	GEditor->SelectNone(true, true);
	return TEXT("{\"success\": true, \"message\": \"Selection cleared\"}");
}

// ==================== TRANSFORM TOOLS ====================

FString UGenWorldUtils::MoveActors(const TArray<FString>& ActorNames, const FVector& Offset)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	int32 MovedCount = 0;
	for (const FString& Name : ActorNames)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetActorLabel() == Name)
			{
				It->SetActorLocation(It->GetActorLocation() + Offset);
				MovedCount++;
				break;
			}
		}
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Moved %d actors\", \"count\": %d}"), MovedCount, MovedCount);
}

FString UGenWorldUtils::RotateActors(const TArray<FString>& ActorNames, const FRotator& Rotation)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	int32 RotatedCount = 0;
	for (const FString& Name : ActorNames)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetActorLabel() == Name)
			{
				It->SetActorRotation(It->GetActorRotation() + Rotation);
				RotatedCount++;
				break;
			}
		}
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Rotated %d actors\", \"count\": %d}"), RotatedCount, RotatedCount);
}

FString UGenWorldUtils::ScaleActors(const TArray<FString>& ActorNames, const FVector& Scale)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	int32 ScaledCount = 0;
	for (const FString& Name : ActorNames)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetActorLabel() == Name)
			{
				It->SetActorScale3D(Scale);
				ScaledCount++;
				break;
			}
		}
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Scaled %d actors\", \"count\": %d}"), ScaledCount, ScaledCount);
}

FString UGenWorldUtils::AlignActorsToGround(const TArray<FString>& ActorNames)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	int32 AlignedCount = 0;
	for (const FString& Name : ActorNames)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetActorLabel() == Name)
			{
				FVector Start = It->GetActorLocation() + FVector(0, 0, 1000);
				FVector End = It->GetActorLocation() - FVector(0, 0, 100000);

				FHitResult HitResult;
				if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic))
				{
					FVector NewLocation = HitResult.ImpactPoint;
					It->SetActorLocation(NewLocation);
					AlignedCount++;
				}
				break;
			}
		}
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Aligned %d actors to ground\", \"count\": %d}"), AlignedCount, AlignedCount);
}

// ==================== UTILITIES ====================

FString UGenWorldUtils::TakeScreenshot(const FString& FileName, int32 ResolutionMultiplier)
{
	FString ScreenshotPath = FPaths::ProjectSavedDir() / TEXT("Screenshots") / FileName + TEXT(".png");
	FScreenshotRequest::RequestScreenshot(ScreenshotPath, false, false);
	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Screenshot requested\", \"path\": \"%s\"}"), *ScreenshotPath);
}

FString UGenWorldUtils::BuildLighting()
{
	GEditor->Exec(GEditor->GetEditorWorldContext().World(), TEXT("BUILD LIGHTING"));
	return TEXT("{\"success\": true, \"message\": \"Lighting build started\"}");
}

FString UGenWorldUtils::BuildAll()
{
	GEditor->Exec(GEditor->GetEditorWorldContext().World(), TEXT("BUILD"));
	return TEXT("{\"success\": true, \"message\": \"Full build started\"}");
}

FString UGenWorldUtils::PlayInEditor()
{
	// Use console command to start PIE
	GEditor->Exec(GEditor->GetEditorWorldContext().World(), TEXT("PIE"));
	return TEXT("{\"success\": true, \"message\": \"Play in Editor requested\"}");
}

FString UGenWorldUtils::StopPlayInEditor()
{
	if (GEditor->PlayWorld)
	{
		GEditor->RequestEndPlayMap();
		return TEXT("{\"success\": true, \"message\": \"Play in Editor stopped\"}");
	}
	return TEXT("{\"success\": false, \"error\": \"Not playing in editor\"}");
}
