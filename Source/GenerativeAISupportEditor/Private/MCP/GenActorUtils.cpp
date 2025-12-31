// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.


#include "MCP/GenActorUtils.h"

#include "ScopedTransaction.h"
#include "Engine/StaticMeshActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorLevelLibrary.h"
#include "EditorUtilityLibrary.h"
#include "UObject/ConstructorHelpers.h"


#include "AssetToolsModule.h"
#include "EngineUtils.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "MaterialEditingLibrary.h"
#include "Factories/BlueprintFactory.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/SavePackage.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "FileHelpers.h"
#include "EditorAssetLibrary.h"

AActor* UGenActorUtils::SpawnBasicShape(const FString& ShapeName, const FVector& Location, 
                                        const FRotator& Rotation, const FVector& Scale, 
                                        const FString& ActorLabel)
{
    // Get the world
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get editor world"));
        return nullptr;
    }

    // Spawn a static mesh actor
    // Undo/Redo support
    FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Spawn Basic Shape")));
    AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Location, Rotation);
    if (!Actor)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn StaticMeshActor"));
        return nullptr;
    }

    // Set the static mesh
    FString MeshPath = FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), *ShapeName, *ShapeName);
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
    
    if (!Mesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load static mesh at path: %s"), *MeshPath);
        Actor->Destroy();
        return nullptr;
    }

    Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
    
    // Set scale
    Actor->SetActorScale3D(Scale);
    
    // Set custom label if provided
    if (!ActorLabel.IsEmpty())
    {
        Actor->SetActorLabel(*ActorLabel);
    }
    
    return Actor;
}

AActor* UGenActorUtils::SpawnStaticMeshActor(const FString& MeshPath, const FVector& Location, 
                                            const FRotator& Rotation, const FVector& Scale, 
                                            const FString& ActorLabel)
{
    // Load the static mesh
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
    if (!Mesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not load static mesh from: %s"), *MeshPath);
        return nullptr;
    }

    // Get the world
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get editor world"));
        return nullptr;
    }

    // Undo/Redo support
    FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Spawn Static Mesh Actor")));
    // Spawn a StaticMeshActor
    AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation);
    if (!Actor)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn StaticMeshActor"));
        return nullptr;
    }

    // Set the static mesh
    UStaticMeshComponent* MeshComponent = Actor->GetStaticMeshComponent();
    if (MeshComponent)
    {
        MeshComponent->SetStaticMesh(Mesh);
        Actor->SetActorScale3D(Scale);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No StaticMeshComponent found on spawned actor"));
        Actor->Destroy();
        return nullptr;
    }

    // Set custom label if provided
    if (!ActorLabel.IsEmpty())
    {
        Actor->SetActorLabel(*ActorLabel);
    }

    UE_LOG(LogTemp, Log, TEXT("Spawned StaticMeshActor with mesh %s labeled %s"), *MeshPath, *ActorLabel);
    return Actor;
}

AActor* UGenActorUtils::SpawnActorFromClass(const FString& ActorClassName, const FVector& Location, 
                                           const FRotator& Rotation, const FVector& Scale, 
                                           const FString& ActorLabel)
{
    UClass* ActorClass = nullptr;
    
    // Check if it's a path to a Blueprint class
    if (ActorClassName.StartsWith("/"))
    {
        FSoftClassPath ClassPath(ActorClassName);
        ActorClass = ClassPath.TryLoadClass<AActor>();
    }
    else
    {
        // Try to find class by name
        FString FullClassName = FString::Printf(TEXT("/Script/Engine.%s"), *ActorClassName);
        ActorClass = FindObject<UClass>(nullptr, *ActorClassName);
        
        if (!ActorClass)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not find actor class: %s"), *ActorClassName);
            return nullptr;
        }
    }
    
    if (!ActorClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not load actor class from: %s"), *ActorClassName);
        return nullptr;
    }
    
    // Get the world
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get editor world"));
        return nullptr;
    }
    
    // Spawn actor
    AActor* Actor = World->SpawnActor(ActorClass, &Location, &Rotation);
    
    if (!Actor)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn actor of class: %s"), *ActorClassName);
        return nullptr;
    }
    
    // Set scale
    Actor->SetActorScale3D(Scale);
    
    // Set custom label if provided
    if (!ActorLabel.IsEmpty())
    {
        Actor->SetActorLabel(*ActorLabel);
    }
    
    return Actor;
}


// Find actor by name implementation
AActor* UGenActorUtils::FindActorByName(const FString& ActorName)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get editor world"));
        return nullptr;
    }

    // Try to find the actor in the current level
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->GetActorLabel() == ActorName)
        {
            return Actor;
        }
    }

    // If not found by label, try using the full path format
    AActor* FoundActor = FindObject<AActor>(World, *ActorName);
    if (FoundActor)
    {
        return FoundActor;
    }

    UE_LOG(LogTemp, Warning, TEXT("Actor '%s' not found in the level"), *ActorName);
    return nullptr;
}

UMaterial* UGenActorUtils::CreateMaterial(const FString& MaterialName, const FLinearColor& Color)
{
    // Create unique asset name to avoid conflicts
    FString PackagePath = TEXT("/Game/Materials");
    FString FullPackagePath = PackagePath + TEXT("/") + MaterialName;
    
    // Check if package already exists to avoid partially loaded assets
    UPackage* ExistingPackage = FindPackage(nullptr, *FullPackagePath);
    if (ExistingPackage)
    {
        UE_LOG(LogTemp, Warning, TEXT("Material '%s' already exists, trying to load it"), *MaterialName);
        UMaterial* ExistingMaterial = LoadObject<UMaterial>(nullptr, *(FullPackagePath + "." + MaterialName));
        if (ExistingMaterial)
        {
            return ExistingMaterial;
        }
    }
    
    // Create package for new material
    UPackage* Package = CreatePackage(*FullPackagePath);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create package for material '%s'"), *MaterialName);
        return nullptr;
    }
    
    // Create new material
    UMaterial* Material = NewObject<UMaterial>(Package, *MaterialName, RF_Public | RF_Standalone | RF_Transactional);
    if (!Material)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create material '%s'"), *MaterialName);
        return nullptr;
    }
    
    // Set up material properties
    Material->bUseEmissiveForDynamicAreaLighting = false;
    Material->BlendMode = BLEND_Opaque;
    
    // Create color expression
    UMaterialExpressionConstant3Vector* ConstantColor =
        Cast<UMaterialExpressionConstant3Vector>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionConstant3Vector::StaticClass(), -350, 0));
    
    if (ConstantColor)
    {
        ConstantColor->Constant = Color;
        // Connect to base color
        UMaterialEditingLibrary::ConnectMaterialProperty(ConstantColor, "RGB", EMaterialProperty::MP_BaseColor);
    }
    
    // Set material to be fully created and initialized
    Material->PreEditChange(nullptr);
    Material->PostEditChange();
    UMaterialEditingLibrary::RecompileMaterial(Material);
    
    // Mark package as dirty
    Package->MarkPackageDirty();
    
    // Save the package
    FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    
    // Force immediate flush to disk to ensure complete save
    SaveArgs.SaveFlags = SAVE_NoError;
    
    bool bSaved = UPackage::SavePackage(
        Package,
        Material,
        *PackageFileName,
        SaveArgs
    );
    
    if (bSaved)
    {
        // Notify asset registry that we created a new asset
        FAssetRegistryModule::AssetCreated(Material);
        UE_LOG(LogTemp, Log, TEXT("Successfully created and saved material '%s'"), *MaterialName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save material '%s'"), *MaterialName);
    }
    
    return Material;
}

// Set material for an actor by direct material reference
bool UGenActorUtils::SetActorMaterial(const FString& ActorName, UMaterialInterface* Material)
{
    if (!Material)
    {
        UE_LOG(LogTemp, Error, TEXT("Null material provided"));
        return false;
    }
    
    AActor* Actor = FindActorByName(ActorName);
    if (!Actor)
    {
        return false; // Error logged in FindActorByName
    }
    
    // Find the static mesh component
    UStaticMeshComponent* MeshComponent = Actor->FindComponentByClass<UStaticMeshComponent>();
    if (!MeshComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("No static mesh component found on actor '%s'"), *ActorName);
        return false;
    }
    
    // Apply material to all mesh sections
    int32 MaterialCount = MeshComponent->GetNumMaterials();
    for (int32 i = 0; i < MaterialCount; i++)
    {
        MeshComponent->SetMaterial(i, Material);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Set material for actor '%s'"), *ActorName);
    return true;
}

// Set material for an actor by material path
bool UGenActorUtils::SetActorMaterialByPath(const FString& ActorName, const FString& MaterialPath)
{
    // Load the material
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    if (!Material)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load material at path '%s'"), *MaterialPath);
        return false;
    }
    
    return SetActorMaterial(ActorName, Material);
}

// Set position for an actor
bool UGenActorUtils::SetActorPosition(const FString& ActorName, const FVector& Position)
{
    AActor* Actor = FindActorByName(ActorName);
    if (!Actor)
    {
        return false; // Error logged in FindActorByName
    }
    
    FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Set Actor Position")));
    Actor->Modify();
    Actor->SetActorLocation(Position);
    UE_LOG(LogTemp, Log, TEXT("Set position of actor '%s' to (%f, %f, %f)"), 
           *ActorName, Position.X, Position.Y, Position.Z);
    return true;
}

// Set rotation for an actor
bool UGenActorUtils::SetActorRotation(const FString& ActorName, const FRotator& Rotation)
{
    AActor* Actor = FindActorByName(ActorName);
    if (!Actor)
    {
        return false; // Error logged in FindActorByName
    }
    
    FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Set Actor Rotation")));
    Actor->Modify();
    Actor->SetActorRotation(Rotation);
    UE_LOG(LogTemp, Log, TEXT("Set rotation of actor '%s' to (%f, %f, %f)"), 
           *ActorName, Rotation.Pitch, Rotation.Yaw, Rotation.Roll);
    return true;
}

// Set scale for an actor
bool UGenActorUtils::SetActorScale(const FString& ActorName, const FVector& Scale)
{
    AActor* Actor = FindActorByName(ActorName);
    if (!Actor)
    {
        return false; // Error logged in FindActorByName
    }
    
    FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Set Actor Scale")));
    Actor->Modify();
    Actor->SetActorScale3D(Scale);
    UE_LOG(LogTemp, Log, TEXT("Set scale of actor '%s' to (%f, %f, %f)"), 
           *ActorName, Scale.X, Scale.Y, Scale.Z);
    return true;
}

FString UGenActorUtils::CreateGameModeWithPawn(const FString& GameModePath, const FString& PawnBlueprintPath, const FString& BaseClassName)
{
    // Validate paths
    if (GameModePath.IsEmpty() || PawnBlueprintPath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("GameModePath or PawnBlueprintPath is empty"));
        return TEXT("{\"success\": false, \"error\": \"Empty path provided\"}");
    }

    // Check if game mode already exists
    if (LoadObject<UBlueprint>(nullptr, *GameModePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Game mode already exists at %s"), *GameModePath);
        return TEXT("{\"success\": false, \"error\": \"Game mode already exists\"}");
    }

    // Load the base class (default to AGameModeBase if not specified)
    FString BaseClassToUse = BaseClassName.IsEmpty() ? TEXT("GameModeBase") : BaseClassName;
    UClass* BaseClass = FindObject<UClass>(nullptr, *BaseClassToUse);
    if (!BaseClass || !BaseClass->IsChildOf(AGameModeBase::StaticClass()))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid base class %s for game mode"), *BaseClassToUse);
        return TEXT("{\"success\": false, \"error\": \"Invalid base class\"}");
    }

    // Load the pawn Blueprint
    UBlueprint* PawnBP = LoadObject<UBlueprint>(nullptr, *PawnBlueprintPath);
    if (!PawnBP || !PawnBP->GeneratedClass || !PawnBP->GeneratedClass->IsChildOf(APawn::StaticClass()))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid pawn blueprint: %s"), *PawnBlueprintPath);
        return TEXT("{\"success\": false, \"error\": \"Invalid pawn blueprint\"}");
    }

    // Create the new game mode Blueprint
    IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
    FString PackageName = FPackageName::GetLongPackagePath(GameModePath);
    FString AssetName = FPackageName::GetShortName(GameModePath);

    UBlueprintFactory* BlueprintFactory = NewObject<UBlueprintFactory>();
    BlueprintFactory->ParentClass = BaseClass;

    UObject* NewAsset = AssetTools.CreateAsset(AssetName, PackageName, UBlueprint::StaticClass(), BlueprintFactory);
    UBlueprint* GameModeBP = Cast<UBlueprint>(NewAsset);
    if (!GameModeBP)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create game mode blueprint at %s"), *GameModePath);
        return TEXT("{\"success\": false, \"error\": \"Failed to create game mode\"}");
    }

    // Set the DefaultPawnClass
    UClass* GameModeClass = GameModeBP->GeneratedClass;
    if (!GameModeClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Generated class not found for %s"), *GameModePath);
        return TEXT("{\"success\": false, \"error\": \"No generated class\"}");
    }
    if (AGameModeBase* GameModeCDO = Cast<AGameModeBase>(GameModeClass->GetDefaultObject()))
    {
        GameModeCDO->DefaultPawnClass = PawnBP->GeneratedClass;
    }

    // Set as current level's game mode
    if (GEditor)
    {
        UWorld* CurrentWorld = GEditor->GetEditorWorldContext().World();
        if (CurrentWorld)
        {
            ULevel* CurrentLevel = CurrentWorld->GetCurrentLevel();
            if (CurrentLevel)
            {
                CurrentLevel->GetWorldSettings()->DefaultGameMode = GameModeClass;
                CurrentLevel->GetWorldSettings()->MarkPackageDirty();
                UE_LOG(LogTemp, Log, TEXT("Set %s as default game mode for current scene"), *GameModePath);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("No current level found to set game mode"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No current world found to set game mode"));
        }
    }

    // Mark as modified and compile
    GameModeBP->Modify();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GameModeBP);

    UE_LOG(LogTemp, Log, TEXT("Created game mode %s with pawn %s and set as scene default"), *GameModePath, *PawnBlueprintPath);
    return FString::Printf(TEXT("{\"success\": true, \"message\": \"Created game mode %s with pawn %s and set as scene default\"}"),
                           *GameModePath, *PawnBlueprintPath);
}

FString UGenActorUtils::GetActorProperties(const FString& ActorName)
{
    AActor* Actor = FindActorByName(ActorName);
    if (!Actor)
    {
        return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
    }

    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
    ResultJson->SetBoolField("success", true);
    ResultJson->SetStringField("name", Actor->GetActorLabel());
    ResultJson->SetStringField("class", Actor->GetClass()->GetName());

    // Location
    FVector Loc = Actor->GetActorLocation();
    TSharedPtr<FJsonObject> LocJson = MakeShareable(new FJsonObject);
    LocJson->SetNumberField("x", Loc.X);
    LocJson->SetNumberField("y", Loc.Y);
    LocJson->SetNumberField("z", Loc.Z);
    ResultJson->SetObjectField("location", LocJson);

    // Rotation
    FRotator Rot = Actor->GetActorRotation();
    TSharedPtr<FJsonObject> RotJson = MakeShareable(new FJsonObject);
    RotJson->SetNumberField("pitch", Rot.Pitch);
    RotJson->SetNumberField("yaw", Rot.Yaw);
    RotJson->SetNumberField("roll", Rot.Roll);
    ResultJson->SetObjectField("rotation", RotJson);

    // Scale
    FVector Scale = Actor->GetActorScale3D();
    TSharedPtr<FJsonObject> ScaleJson = MakeShareable(new FJsonObject);
    ScaleJson->SetNumberField("x", Scale.X);
    ScaleJson->SetNumberField("y", Scale.Y);
    ScaleJson->SetNumberField("z", Scale.Z);
    ResultJson->SetObjectField("scale", ScaleJson);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

    return OutputString;
}

FString UGenActorUtils::ListAllActors(const FString& ClassFilter)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return TEXT("{\"success\": false, \"error\": \"No world found\"}");
    }

    TArray<TSharedPtr<FJsonValue>> ActorsArray;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor) continue;

        // Apply class filter if specified
        if (!ClassFilter.IsEmpty())
        {
            FString ClassName = Actor->GetClass()->GetName();
            if (!ClassName.Contains(ClassFilter))
            {
                continue;
            }
        }

        TSharedPtr<FJsonObject> ActorJson = MakeShareable(new FJsonObject);
        ActorJson->SetStringField("name", Actor->GetActorLabel());
        ActorJson->SetStringField("class", Actor->GetClass()->GetName());

        FVector Loc = Actor->GetActorLocation();
        ActorJson->SetStringField("location", FString::Printf(TEXT("(%f, %f, %f)"), Loc.X, Loc.Y, Loc.Z));

        ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorJson)));
    }

    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
    ResultJson->SetBoolField("success", true);
    ResultJson->SetNumberField("count", ActorsArray.Num());
    ResultJson->SetArrayField("actors", ActorsArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

    return OutputString;
}

FString UGenActorUtils::DeleteActor(const FString& ActorName)
{
    AActor* Actor = FindActorByName(ActorName);
    if (!Actor)
    {
        return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
    }

    FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Delete Actor")));
    Actor->Modify();
    FString ActorLabel = Actor->GetActorLabel();
    bool bDestroyed = Actor->Destroy();

    if (bDestroyed)
    {
        return FString::Printf(TEXT("{\"success\": true, \"message\": \"Actor '%s' deleted\"}"), *ActorLabel);
    }

    return TEXT("{\"success\": false, \"error\": \"Failed to destroy actor\"}");
}

FString UGenActorUtils::DuplicateActor(const FString& ActorName, const FVector& NewLocation, const FString& NewLabel)
{
    AActor* SourceActor = FindActorByName(ActorName);
    if (!SourceActor)
    {
        return TEXT("{\"success\": false, \"error\": \"Source actor not found\"}");
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return TEXT("{\"success\": false, \"error\": \"No world found\"}");
    }

    // Duplicate using spawn with same class
    FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Duplicate Actor")));
    FActorSpawnParameters SpawnParams;
    SpawnParams.Template = SourceActor;

    AActor* NewActor = World->SpawnActor<AActor>(SourceActor->GetClass(), NewLocation, SourceActor->GetActorRotation(), SpawnParams);

    if (!NewActor)
    {
        return TEXT("{\"success\": false, \"error\": \"Failed to duplicate actor\"}");
    }

    if (!NewLabel.IsEmpty())
    {
        NewActor->SetActorLabel(*NewLabel);
    }

    return FString::Printf(TEXT("{\"success\": true, \"message\": \"Actor duplicated as '%s'\", \"new_actor\": \"%s\"}"),
        *NewActor->GetActorLabel(), *NewActor->GetActorLabel());
}

FString UGenActorUtils::SaveCurrentLevel()
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

    bool bSaved = FEditorFileUtils::SaveCurrentLevel();

    if (bSaved)
    {
        return TEXT("{\"success\": true, \"message\": \"Level saved successfully\"}");
    }

    return TEXT("{\"success\": false, \"error\": \"Failed to save level\"}");
}

FString UGenActorUtils::SaveAllDirtyAssets()
{
    bool bPromptUserToSave = false;
    bool bSaveMapPackages = true;
    bool bSaveContentPackages = true;
    bool bFastSave = false;
    bool bNotifyNoPackagesSaved = false;
    bool bCanBeDeclined = false;

    bool bSaved = FEditorFileUtils::SaveDirtyPackages(
        bPromptUserToSave,
        bSaveMapPackages,
        bSaveContentPackages,
        bFastSave,
        bNotifyNoPackagesSaved,
        bCanBeDeclined
    );

    if (bSaved)
    {
        return TEXT("{\"success\": true, \"message\": \"All dirty assets saved\"}");
    }

    return TEXT("{\"success\": false, \"error\": \"Failed to save some assets\"}");
}

FString UGenActorUtils::GetActorProperty(const FString& ActorName, const FString& PropertyName)
{
    AActor* Actor = FindActorByName(ActorName);
    if (!Actor)
    {
        return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
    }

    FProperty* Property = Actor->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        return FString::Printf(TEXT("{\"success\": false, \"error\": \"Property '%s' not found\"}"), *PropertyName);
    }

    FString ValueString;
    const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Actor);
    FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Set Actor Property")));
    Property->ExportTextItem_Direct(ValueString, ValuePtr, nullptr, nullptr, PPF_None);

    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
    ResultJson->SetBoolField("success", true);
    ResultJson->SetStringField("actor", ActorName);
    ResultJson->SetStringField("property", PropertyName);
    ResultJson->SetStringField("value", ValueString);
    ResultJson->SetStringField("type", Property->GetCPPType());

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

    return OutputString;
}

FString UGenActorUtils::SetActorProperty(const FString& ActorName, const FString& PropertyName, const FString& ValueString)
{
    AActor* Actor = FindActorByName(ActorName);
    if (!Actor)
    {
        return TEXT("{\"success\": false, \"error\": \"Actor not found\"}");
    }

    FProperty* Property = Actor->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        return FString::Printf(TEXT("{\"success\": false, \"error\": \"Property '%s' not found\"}"), *PropertyName);
    }

    void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Actor);
    FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Set Actor Property")));

    // Try to import the value from string
    const TCHAR* ImportResult = Property->ImportText_Direct(*ValueString, ValuePtr, Actor, PPF_None);

    if (ImportResult)
    {
        Actor->Modify();
        return FString::Printf(TEXT("{\"success\": true, \"message\": \"Property '%s' set to '%s'\"}"), *PropertyName, *ValueString);
    }

    return FString::Printf(TEXT("{\"success\": false, \"error\": \"Failed to set property '%s'\"}"), *PropertyName);
}