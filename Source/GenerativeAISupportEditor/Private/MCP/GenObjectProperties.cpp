// Fill out your copyright notice in the Description page of Project Settings.


#include "MCP/GenObjectProperties.h"

#include "EngineUtils.h"
#include "K2Node_Event.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/OutputDeviceNull.h"

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
		
		// Store available actors for better error messages
		TArray<FString> AvailableActors;
		
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			// Add to available actors list for error reporting
			AvailableActors.Add(It->GetActorLabel());
			
			// CHANGE: Use GetActorLabel() instead of GetName()
			if (It->GetActorLabel() == ActorName)
			{
				SceneActor = *It;
				UE_LOG(LogTemp, Log, TEXT("Found scene actor: %s"), *ActorName);
				
				// Get available component names for better error messages
				TArray<FString> AvailableComponents;
				
				for (UActorComponent* Comp : SceneActor->GetComponents())
				{
					AvailableComponents.Add(Comp->GetName());
					if (Comp->GetName() == ComponentName)
					{
						TargetObject = Comp;
						UE_LOG(LogTemp, Log, TEXT("Found component: %s"), *ComponentName);
						break;
					}
				}
				
				// If we found the actor but not the component, return detailed error
				if (SceneActor && !TargetObject) {
					FString ComponentsList = FString::Join(AvailableComponents, TEXT(", "));
					return FString::Printf(
						TEXT("{\"success\": false, \"error\": \"Component '%s' not found on actor '%s'. Available components: ['%s']\"}"),
						*ComponentName, *ActorName, *ComponentsList);
				}
				
				break;
			}
		}
		
		// If we didn't find the actor, return detailed error
		if (!SceneActor) {
			FString ActorsList = FString::Join(AvailableActors, TEXT(", "));
			return FString::Printf(
				TEXT("{\"success\": false, \"error\": \"Scene actor not found: %s. Available actors: %s\"}"),
				*ActorName, *ActorsList);
		}
		
		// If we didn't find the component, the detailed error is already returned above
		if (!TargetObject) return TEXT("{\"success\": false, \"error\": \"Scene actor or component not found\"}");
	}
	else
	{
		// Blueprint code remains unchanged
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

	// Special handling for StaticMesh property
	if (PropertyNameLower == TEXT("staticmesh") || PropertyNameLower == TEXT("mesh"))
	{
		UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Component);
		if (!StaticMeshComp)
		{
			return TEXT("{\"success\": false, \"error\": \"StaticMesh property only applicable to StaticMeshComponent\"}");
		}

		// Strip quotes from the value
		FString CleanValue = Value;
		CleanValue.RemoveFromStart(TEXT("'"));
		CleanValue.RemoveFromEnd(TEXT("'"));
		CleanValue.RemoveFromStart(TEXT("\""));
		CleanValue.RemoveFromEnd(TEXT("\""));

		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *CleanValue);
		if (!Mesh)
		{
			// Try appending the asset name as a fallback
			FString FallbackPath = CleanValue + TEXT(".") + FName(*CleanValue).GetPlainNameString();
			Mesh = LoadObject<UStaticMesh>(nullptr, *FallbackPath);
			if (!Mesh)
			{
				return FString::Printf(TEXT("{\"success\": false, \"error\": \"Failed to load static mesh at path: %s\"}"), *CleanValue);
			}
		}

		StaticMeshComp->SetStaticMesh(Mesh);
		
		if (Blueprint)
		{
			Blueprint->Modify();
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		}
		else if (SceneActor)
		{
			SceneActor->Modify();
		}
		
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Set StaticMesh of %s to %s\"}"),
			*ComponentName, *CleanValue);
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
		// Handle object references (materials, meshes, etc.)
		FString CleanValue = Value;
		CleanValue.RemoveFromStart(TEXT("'"));
		CleanValue.RemoveFromEnd(TEXT("'"));
		CleanValue.RemoveFromStart(TEXT("\""));
		CleanValue.RemoveFromEnd(TEXT("\""));

		UObject* NewValue = LoadObject<UObject>(nullptr, *CleanValue);
		if (!NewValue)
		{
			// Try appending asset name as fallback
			FString FallbackPath = CleanValue + TEXT(".") + FName(*CleanValue).GetPlainNameString();
			NewValue = LoadObject<UObject>(nullptr, *FallbackPath);
		}

		if (NewValue && NewValue->IsA(ObjProp->PropertyClass))
		{
			ObjProp->SetPropertyValue_InContainer(Component, NewValue);
			bSuccess = true;
		}
		else
		{
			return FString::Printf(
				TEXT("{\"success\": false, \"error\": \"Could not load object or type mismatch. Expected %s, trying to load %s\"}"),
				*ObjProp->PropertyClass->GetName(), *CleanValue);
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
		FString CleanValue = Value;
		// Remove outer quotes if present (common when strings are passed with quotes)
		if (CleanValue.StartsWith(TEXT("\"")) && CleanValue.EndsWith(TEXT("\"")))
		{
			CleanValue = CleanValue.Mid(1, CleanValue.Len() - 2);
		}
		StrProp->SetPropertyValue_InContainer(Component, CleanValue);
		bSuccess = true;
	}
	else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		FString CleanValue = Value;
		if (CleanValue.StartsWith(TEXT("\"")) && CleanValue.EndsWith(TEXT("\"")))
		{
			CleanValue = CleanValue.Mid(1, CleanValue.Len() - 2);
		}
		FName NameValue(*CleanValue);
		NameProp->SetPropertyValue_InContainer(Component, NameValue);
		bSuccess = true;
	}
	else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		FString CleanValue = Value;
		if (CleanValue.StartsWith(TEXT("\"")) && CleanValue.EndsWith(TEXT("\"")))
		{
			CleanValue = CleanValue.Mid(1, CleanValue.Len() - 2);
		}
		FText TextValue = FText::FromString(CleanValue);
		TextProp->SetPropertyValue_InContainer(Component, TextValue);
		bSuccess = true;
	}
	else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		// Handle common struct types
		if (StructProp->Struct == TBaseStructure<FVector>::Get())
		{
			// Parse vector with flexible formats: "X,Y,Z", "(X,Y,Z)", or "X Y Z"
			FString CleanValue = Value;
			CleanValue.RemoveFromStart(TEXT("("));
			CleanValue.RemoveFromEnd(TEXT(")"));
			
			TArray<FString> Components;
			// Try comma delimiter first
			if (CleanValue.Contains(TEXT(",")))
			{
				CleanValue.ParseIntoArray(Components, TEXT(","), true);
			}
			// Try space delimiter if comma didn't work or didn't produce enough components
			else 
			{
				CleanValue.ParseIntoArray(Components, TEXT(" "), true);
			}
			
			// Support both 3-component and 2-component vectors (Z=0 for 2D)
			if (Components.Num() >= 2)
			{
				float X = FCString::Atof(*Components[0]);
				float Y = FCString::Atof(*Components[1]);
				float Z = Components.Num() >= 3 ? FCString::Atof(*Components[2]) : 0.0f;
				
				FVector Vector(X, Y, Z);
				StructProp->CopyCompleteValue_InContainer(Component, &Vector);
				bSuccess = true;
			}
		}
		else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
		{
			// Parse rotator with flexible formats
			FString CleanValue = Value;
			CleanValue.RemoveFromStart(TEXT("("));
			CleanValue.RemoveFromEnd(TEXT(")"));
			
			TArray<FString> Components;
			if (CleanValue.Contains(TEXT(",")))
			{
				CleanValue.ParseIntoArray(Components, TEXT(","), true);
			}
			else 
			{
				CleanValue.ParseIntoArray(Components, TEXT(" "), true);
			}
			
			if (Components.Num() >= 3)
			{
				float Pitch = FCString::Atof(*Components[0]);
				float Yaw = FCString::Atof(*Components[1]);
				float Roll = FCString::Atof(*Components[2]);
				
				FRotator Rotator(Pitch, Yaw, Roll);
				StructProp->CopyCompleteValue_InContainer(Component, &Rotator);
				bSuccess = true;
			}
		}
		else if (StructProp->Struct == TBaseStructure<FLinearColor>::Get())
		{
			// Parse color with flexible formats
			FString CleanValue = Value;
			CleanValue.RemoveFromStart(TEXT("("));
			CleanValue.RemoveFromEnd(TEXT(")"));
			
			TArray<FString> Components;
			if (CleanValue.Contains(TEXT(",")))
			{
				CleanValue.ParseIntoArray(Components, TEXT(","), true);
			}
			else 
			{
				CleanValue.ParseIntoArray(Components, TEXT(" "), true);
			}
			
			if (Components.Num() >= 3)
			{
				float R = FCString::Atof(*Components[0]);
				float G = FCString::Atof(*Components[1]);
				float B = FCString::Atof(*Components[2]);
				float A = Components.Num() >= 4 ? FCString::Atof(*Components[3]) : 1.0f;
				
				FLinearColor Color(R, G, B, A);
				StructProp->CopyCompleteValue_InContainer(Component, &Color);
				bSuccess = true;
			}
		}
		else if (StructProp->Struct->GetName() == TEXT("Transform"))
		{
			// Basic support for transforms - advanced use should use individual components
			FString Message = TEXT("Transform property detected. For best results, set individual components like RelativeLocation instead.");
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
			return FString::Printf(TEXT("{\"success\": false, \"error\": \"%s\"}"), *Message);
		}
		else
		{
			// For other structs, try using ImportText which handles string conversion for many UE types
			void* StructPtr = StructProp->ContainerPtrToValuePtr<void>(Component);
			if (StructPtr)
			{
				FString ImportErrorMsg;
				const TCHAR* ImportText = *Value;
				FOutputDeviceNull ImportErrorDevice;
				if (StructProp->Struct->ImportText(ImportText, StructPtr, nullptr, PPF_None, &ImportErrorDevice, StructProp->Struct->GetName()))
				{
					bSuccess = true;
				}
				else
				{
					return FString::Printf(TEXT("{\"success\": false, \"error\": \"Failed to import struct value: %s\"}"), *ImportErrorMsg);
				}
			}
		}
	}
	else if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		if (ByteProp->IsEnum())
		{
			// Handle enum properties by name or numeric value
			FString CleanValue = Value;
			if (CleanValue.StartsWith(TEXT("\"")) && CleanValue.EndsWith(TEXT("\"")))
			{
				CleanValue = CleanValue.Mid(1, CleanValue.Len() - 2);
			}

			UEnum* Enum = ByteProp->GetIntPropertyEnum();
			if (Enum)
			{
				// Try parsing as enum name first
				int32 EnumIndex = Enum->GetIndexByName(FName(*CleanValue));
				if (EnumIndex != INDEX_NONE)
				{
					uint8 EnumValue = static_cast<uint8>(EnumIndex);
					ByteProp->SetPropertyValue_InContainer(Component, EnumValue);
					bSuccess = true;
				}
				else
				{
					// Try parsing as integer value
					int32 IntValue = FCString::Atoi(*CleanValue);
					if (IntValue >= 0 && IntValue < Enum->NumEnums())
					{
						ByteProp->SetPropertyValue_InContainer(Component, static_cast<uint8>(IntValue));
						bSuccess = true;
					}
					else
					{
						// Generate list of valid enum values as suggestions
						TArray<FString> EnumValues;
						for (int32 i = 0; i < Enum->NumEnums(); i++)
						{
							EnumValues.Add(Enum->GetNameStringByIndex(i));
						}
						FString ValidValues = FString::Join(EnumValues, TEXT(", "));
						return FString::Printf(TEXT("{\"success\": false, \"error\": \"Invalid enum value: %s. Valid values: %s\"}"), 
							*CleanValue, *ValidValues);
					}
				}
			}
		}
		else
		{
			// Regular byte property
			uint8 ByteValue = (uint8)FCString::Atoi(*Value);
			ByteProp->SetPropertyValue_InContainer(Component, ByteValue);
			bSuccess = true;
		}
	}
	else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		FString CleanValue = Value;
		if (CleanValue.StartsWith(TEXT("\"")) && CleanValue.EndsWith(TEXT("\"")))
		{
			CleanValue = CleanValue.Mid(1, CleanValue.Len() - 2);
		}

		UEnum* Enum = EnumProp->GetEnum();
		if (Enum)
		{
			int32 EnumIndex = Enum->GetIndexByName(FName(*CleanValue));
			if (EnumIndex != INDEX_NONE)
			{
				// We have to get the underlying property (int/byte) to set the value
				void* ValuePtr = EnumProp->ContainerPtrToValuePtr<void>(Component);
				if (FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty())
				{
					int64 EnumValue = Enum->GetValueByIndex(EnumIndex);
					UnderlyingProp->SetIntPropertyValue(ValuePtr, EnumValue);
					bSuccess = true;
				}
			}
			else
			{
				// Try parsing as integer value
				int64 IntValue = FCString::Atoi64(*CleanValue);
				void* ValuePtr = EnumProp->ContainerPtrToValuePtr<void>(Component);
				if (FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty())
				{
					UnderlyingProp->SetIntPropertyValue(ValuePtr, IntValue);
					bSuccess = true;
				}
			}
		}
	}
	else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Array properties are not directly supported yet. Use individual element access in a script.\"}"));
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
