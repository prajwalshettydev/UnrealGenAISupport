// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.


#include "MCP/GenActorUtils.h"

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
#include "UObject/SavePackage.h"

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
        ActorClass = FindObject<UClass>(ANY_PACKAGE, *ActorClassName);
        
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

// Create a new material with specific color
UMaterial* UGenActorUtils::CreateMaterial(const FString& MaterialName, const FLinearColor& Color)
{
    FString PackagePath = TEXT("/Game/Materials");
    FString FullPackagePath = PackagePath + TEXT("/") + MaterialName;

    // Create package for new material
    UPackage* Package = CreatePackage(*FullPackagePath);

    // Create new material
    UMaterial* Material = NewObject<UMaterial>(Package, *MaterialName, RF_Public | RF_Standalone);
    if (!Material)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create material '%s'"), *MaterialName);
        return nullptr;
    }

    // Use MaterialEditingLibrary to create and connect expressions
    UMaterialExpressionConstant3Vector* ConstantColor = 
        Cast<UMaterialExpressionConstant3Vector>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionConstant3Vector::StaticClass(), -350, 0));
    
    if (ConstantColor)
    {
        ConstantColor->Constant = Color;
        // Connect to base color using the MaterialEditingLibrary
        UMaterialEditingLibrary::ConnectMaterialProperty(ConstantColor, "RGB", EMaterialProperty::MP_BaseColor);
    }

    // Notify material that it has been modified and compile it
    UMaterialEditingLibrary::RecompileMaterial(Material);
    Material->MarkPackageDirty();

    // Save the material
    FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
    
    // Use the updated SavePackage signature that doesn't need FObjectSavingParameters

    // Create save package args
    FSavePackageArgs SaveArgs;
    
    bool bSaved = UPackage::SavePackage(
        Package,
        Material,
        *PackageFileName,
        SaveArgs
    );

    if (bSaved)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully created and saved material '%s'"), *MaterialName);
        return Material;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save material '%s'"), *MaterialName);
        return Material; // Still return the created material even if save failed
    }
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
    
    Actor->SetActorScale3D(Scale);
    UE_LOG(LogTemp, Log, TEXT("Set scale of actor '%s' to (%f, %f, %f)"), 
           *ActorName, Scale.X, Scale.Y, Scale.Z);
    return true;
}