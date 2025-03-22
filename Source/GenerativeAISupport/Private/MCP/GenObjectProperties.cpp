// Fill out your copyright notice in the Description page of Project Settings.


#include "MCP/GenObjectProperties.h"

#include "EngineUtils.h"
#include "K2Node_Event.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Kismet2/BlueprintEditorUtils.h"

FString UGenObjectProperties::EditComponentProperty(const FString& BlueprintPath, const FString& ComponentName,
                                                    const FString& PropertyName, const FString& Value,
                                                    bool bIsSceneActor, const FString& ActorName)
{
	UObject* TargetObject = nullptr;
	UBlueprint* Blueprint = nullptr;
	AActor* SceneActor = nullptr;

	// Load target (Blueprint component or scene actor)
	if (bIsSceneActor)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World) return TEXT("{\"success\": false, \"error\": \"No editor world found\"}");
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetName() == ActorName)
			{
				SceneActor = *It;
				for (UActorComponent* Comp : SceneActor->GetComponents())
				{
					if (Comp->GetName() == ComponentName)
					{
						TargetObject = Comp;
						break;
					}
				}
				break;
			}
		}
		if (!TargetObject) return TEXT("{\"success\": false, \"error\": \"Scene actor or component not found\"}");
	}
	else
	{
		Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
		if (!Blueprint) return TEXT("{\"success\": false, \"error\": \"Could not load blueprint at path: ") +
			BlueprintPath + TEXT("\"}");
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (Node->GetVariableName().ToString() == ComponentName)
			{
				TargetObject = Node->ComponentTemplate;
				break;
			}
		}
		if (!TargetObject) return TEXT("{\"success\": false, \"error\": \"Component ") + ComponentName +
			TEXT(" not found in ") + BlueprintPath + TEXT("\"}");
	}

	UActorComponent* Component = Cast<UActorComponent>(TargetObject);
	if (!Component) return TEXT("{\"success\": false, \"error\": \"Invalid component template for ") + ComponentName +
		TEXT("\"}");

	// Handle material setting for mesh components
	FString PropertyNameLower = PropertyName.ToLower();
	bool IsMaterialProperty = (PropertyNameLower == TEXT("material") ||
		PropertyNameLower == TEXT("setmaterial") ||
		PropertyNameLower == TEXT("basematerial"));

	if (IsMaterialProperty)
	{
		// Strip quotes from the value
		FString CleanValue = Value;
		CleanValue.RemoveFromStart(TEXT("'"));
		CleanValue.RemoveFromEnd(TEXT("'"));
		CleanValue.RemoveFromStart(TEXT("\""));
		CleanValue.RemoveFromEnd(TEXT("\""));

		UE_LOG(LogTemp, Log, TEXT("Attempting to load material at path: %s"), *CleanValue);

		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *CleanValue);
		if (!Material) {
			// Try appending the asset name as a fallback (e.g., "/Game/Materials/M_BirdYellow.M_BirdYellow")
			FString FallbackPath = CleanValue + TEXT(".") + FName(*CleanValue).GetPlainNameString();
			UE_LOG(LogTemp, Log, TEXT("Fallback attempt with path: %s"), *FallbackPath);
			Material = LoadObject<UMaterialInterface>(nullptr, *FallbackPath);
			if (!Material) {
				return FString::Printf(TEXT("{\"success\": false, \"error\": \"Failed to load material at path: %s\"}"), *CleanValue);
			}
		}

		int32 MaterialIndex = 0; // Default to slot 0

		if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Component))
		{
			StaticMeshComp->SetMaterial(MaterialIndex, Material);
		}
		else if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Component))
		{
			SkeletalMeshComp->SetMaterial(MaterialIndex, Material);
		}
		else
		{
			return TEXT(
				"{\"success\": false, \"error\": \"Material setting only supported on StaticMeshComponent or SkeletalMeshComponent\"}");
		}

		if (Blueprint)
		{
			Blueprint->Modify();
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		}
		else if (SceneActor)
		{
			SceneActor->Modify();
		}
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Set material on %s to %s at index %d\"}"),
		                       *ComponentName, *Value, MaterialIndex);
	}

	// Generic property handling
	FProperty* Property = Component->GetClass()->FindPropertyByName(FName(*PropertyName));
	TArray<FString> Suggestions;
	if (!Property)
	{
		for (TFieldIterator<FProperty> PropIt(Component->GetClass()); PropIt; ++PropIt)
		{
			FString PropName = PropIt->GetName();
			if (PropName.Contains(PropertyName, ESearchCase::IgnoreCase))
			{
				Suggestions.Add(PropName + TEXT(" (") + PropIt->GetCPPType() + TEXT(")"));
			}
		}
		FString SuggestionStr = Suggestions.Num() > 0 ? FString::Join(Suggestions, TEXT(", ")) : TEXT("none");
		return FString::Printf(
			TEXT("{\"success\": false, \"error\": \"Property %s not found\", \"suggestions\": \"%s\"}"),
			*PropertyName, *SuggestionStr);
	}

	bool bSuccess = false;

	if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
	{
		UObject* NewValue = LoadObject<UObject>(nullptr, *Value);
		if (NewValue && NewValue->IsA(ObjProp->PropertyClass))
		{
			ObjProp->SetPropertyValue_InContainer(Component, NewValue);
			bSuccess = true;
		}
	}
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
	{
		float FloatValue = FCString::Atof(*Value);
		FloatProp->SetPropertyValue_InContainer(Component, FloatValue);
		bSuccess = true;
	}
	else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
	{
		int32 IntValue = FCString::Atoi(*Value);
		IntProp->SetPropertyValue_InContainer(Component, IntValue);
		bSuccess = true;
	}
	else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		bool BoolValue = Value.ToBool();
		BoolProp->SetPropertyValue_InContainer(Component, BoolValue);
		bSuccess = true;
	}
	else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		StrProp->SetPropertyValue_InContainer(Component, Value);
		bSuccess = true;
	}
	else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		if (StructProp->Struct == TBaseStructure<FVector>::Get())
		{
			TArray<FString> Components;
			Value.ParseIntoArray(Components, TEXT(","), true);
			if (Components.Num() == 3)
			{
				FVector Vector(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]),
				               FCString::Atof(*Components[2]));
				StructProp->CopyCompleteValue_InContainer(Component, &Vector);
				bSuccess = true;
			}
		}
		else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
		{
			TArray<FString> Components;
			Value.ParseIntoArray(Components, TEXT(","), true);
			if (Components.Num() == 3)
			{
				FRotator Rotator(FCString::Atof(*Components[0]), FCString::Atof(*Components[1]),
				                 FCString::Atof(*Components[2]));
				StructProp->CopyCompleteValue_InContainer(Component, &Rotator);
				bSuccess = true;
			}
		}
	}

	if (!bSuccess)
	{
		return FString::Printf(
			TEXT("{\"success\": false, \"error\": \"Failed to set %s to %s - unsupported type or invalid value\"}"),
			*PropertyName, *Value);
	}

	if (Blueprint)
	{
		Blueprint->Modify();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
	else if (SceneActor)
	{
		SceneActor->Modify();
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Set %s.%s to %s\"}"),
	                       *ComponentName, *PropertyName, *Value);
}

FString UGenObjectProperties::GetAllSceneObjects()
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return TEXT("[]");

	TArray<TSharedPtr<FJsonValue>> ActorsArray;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		TSharedPtr<FJsonObject> ActorObject = MakeShareable(new FJsonObject);
		ActorObject->SetStringField(TEXT("name"), Actor->GetName());
		ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());

		TArray<TSharedPtr<FJsonValue>> ComponentsArray;
		for (UActorComponent* Comp : Actor->GetComponents())
		{
			TSharedPtr<FJsonObject> CompObject = MakeShareable(new FJsonObject);
			CompObject->SetStringField(TEXT("name"), Comp->GetName());
			CompObject->SetStringField(TEXT("class"), Comp->GetClass()->GetName());
			ComponentsArray.Add(MakeShareable(new FJsonValueObject(CompObject)));
		}
		ActorObject->SetArrayField(TEXT("components"), ComponentsArray);
		ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorObject)));
	}

	FString ResultJson;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
	FJsonSerializer::Serialize(ActorsArray, Writer);
	return ResultJson;
}
