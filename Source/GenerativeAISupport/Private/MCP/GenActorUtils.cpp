// Fill out your copyright notice in the Description page of Project Settings.


#include "MCP/GenActorUtils.h"

#include "Engine/StaticMeshActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorLevelLibrary.h"
#include "EditorUtilityLibrary.h"
#include "UObject/ConstructorHelpers.h"

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
