// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.
#include "MCP/GenBlueprintUtils.h"

#include "BlueprintEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Factories/BlueprintFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "Blueprint/BlueprintSupport.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "KismetCompiler.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

UBlueprint* UGenBlueprintUtils::CreateBlueprint(const FString& BlueprintName, const FString& ParentClassName,
                                                const FString& SavePath)
{
	// Find parent class
	UClass* ParentClass = FindClassByName(ParentClassName);
	if (!ParentClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find parent class: %s"), *ParentClassName);
		return nullptr;
	}

	// Create package path
	FString FullPackagePath = SavePath + TEXT("/") + BlueprintName;

	// Check if the blueprint already exists
	UObject* ExistingObject = StaticLoadObject(UBlueprint::StaticClass(), nullptr, *FullPackagePath);
	if (ExistingObject)
	{
		UE_LOG(LogTemp, Warning, TEXT("Blueprint already exists at path: %s, returning existing blueprint"),
		       *FullPackagePath);
		return Cast<UBlueprint>(ExistingObject);
	}

	UPackage* Package = CreatePackage(*FullPackagePath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package for blueprint: %s"), *FullPackagePath);
		return nullptr;
	}

	// Create blueprint factory
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	// Create the blueprint
	UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(UBlueprint::StaticClass(), Package,
	                                                                   *BlueprintName, RF_Public | RF_Standalone,
	                                                                   nullptr, GWarn));
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create blueprint: %s"), *BlueprintName);
		return nullptr;
	}

	// Save the blueprint
	FAssetRegistryModule::AssetCreated(Blueprint);
	Package->MarkPackageDirty();

	// Save package
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

	FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackagePath,
	                                                                  FPackageName::GetAssetPackageExtension());
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	// Open the Blueprint editor
	if (GEditor)
	{
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (AssetEditorSubsystem)
		{
			AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
		}
	}
	
	OpenBlueprintGraph(Blueprint);

	UE_LOG(LogTemp, Log, TEXT("Successfully created blueprint: %s"), *BlueprintName);
	return Blueprint;
}


bool UGenBlueprintUtils::AddComponent(const FString& BlueprintPath, const FString& ComponentClass,
                                      const FString& ComponentName)
{
	// Load the blueprint asset
	UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
		return false;
	}

	// Find the component class
	UClass* CompClass = FindClassByName(ComponentClass);
	if (!CompClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find component class: %s"), *ComponentClass);
		return false;
	}

	// Make sure it's a valid component class
	if (!CompClass->IsChildOf(UActorComponent::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("%s is not a component class"), *ComponentClass);
		return false;
	}

	// Create a component instance
	UActorComponent* NewComponent = NewObject<UActorComponent>(GetTransientPackage(), CompClass, FName(*ComponentName));
	if (!NewComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create component instance of class %s"), *ComponentClass);
		return false;
	}

	// Add the component to the blueprint
	TArray<UActorComponent*> Components;
	Components.Add(NewComponent);

	FKismetEditorUtilities::FAddComponentsToBlueprintParams Params;
	FKismetEditorUtilities::AddComponentsToBlueprint(Blueprint, Components, Params);

	// Mark the blueprint as modified
	Blueprint->Modify();

	// Compile the blueprint
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Open the Blueprint editor
	if (GEditor)
	{
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (AssetEditorSubsystem)
		{
			AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
		}
	}
	OpenBlueprintGraph(Blueprint);

	UE_LOG(LogTemp, Log, TEXT("Added component %s to blueprint %s"), *ComponentClass, *BlueprintPath);
	return true;
}

bool UGenBlueprintUtils::AddVariable(const FString& BlueprintPath, const FString& VariableName,
                                     const FString& VariableType, const FString& DefaultValue,
                                     const FString& Category)
{
	// Load the blueprint asset
	UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
		return false;
	}

	// Create a new variable property
	FName VarName = FName(*VariableName);

	// Determine the property type based on string input
	UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Get the property type from the VariableType string
	FEdGraphPinType PinType;

	if (VariableType.Equals(TEXT("boolean"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	}
	else if (VariableType.Equals(TEXT("byte"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
	}
	else if (VariableType.Equals(TEXT("int"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	}
	else if (VariableType.Equals(TEXT("float"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Float;
	}
	else if (VariableType.Equals(TEXT("string"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_String;
	}
	else if (VariableType.Equals(TEXT("text"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Text;
	}
	else if (VariableType.Equals(TEXT("name"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
	}
	else if (VariableType.Equals(TEXT("vector"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
	}
	else if (VariableType.Equals(TEXT("rotator"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
	}
	else if (VariableType.Equals(TEXT("transform"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
	}
	else if (VariableType.Equals(TEXT("color"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
	}
	else
	{
		// Try to find a class with this name
		UClass* Class = FindClassByName(VariableType);
		if (Class)
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
			PinType.PinSubCategoryObject = Class;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Unsupported variable type: %s"), *VariableType);
			return false;
		}
	}

	// Create a new variable 
	FBlueprintEditorUtils::AddMemberVariable(Blueprint, VarName, PinType);

	// Set the category if specified
	if (!Category.IsEmpty())
	{
		const FString CategoryName = Category;
		//UProperty* Property = FindField<UProperty>(Blueprint->GeneratedClass, *VariableName);
		FProperty* Property = Blueprint->GeneratedClass->FindPropertyByName(FName(*VariableName));
		if (Property)
		{
			const FName TargetVarName = FName(*Property->GetNameCPP());
			FBlueprintEditorUtils::SetBlueprintVariableCategory(Blueprint, TargetVarName, nullptr,
			                                                    FText::FromString(CategoryName));
		}
	}

	// Set the default value if specified
	if (!DefaultValue.IsEmpty())
	{
		// This is simplified - setting default values requires different approaches based on type
		//UProperty* Property = FindField<UProperty>(Blueprint->GeneratedClass, *VariableName);
		FProperty* Property = Blueprint->GeneratedClass->FindPropertyByName(FName(*VariableName));
		if (Property)
		{
			UK2Node_VariableGet* VarGetNode = NewObject<UK2Node_VariableGet>();
			VarGetNode->VariableReference.SetSelfMember(FName(*VariableName));
			VarGetNode->GetPropertyForVariable();

			// Set a default value - this is basic implementation, would need to be expanded for different types
			FString DefaultValueToUse = DefaultValue;
			FBlueprintEditorUtils::PropertyValueFromString(Property, DefaultValueToUse,
			                                               reinterpret_cast<uint8*>(Blueprint->GeneratedClass->
				                                               GetDefaultObject(true)));
		}
	}

	// Mark the blueprint as modified
	Blueprint->Modify();

	// Compile the blueprint
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Open the Blueprint editor
	if (GEditor)
	{
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (AssetEditorSubsystem)
		{
			AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
		}
	}

	OpenBlueprintGraph(Blueprint);
	UE_LOG(LogTemp, Log, TEXT("Added variable %s of type %s to blueprint %s"), *VariableName, *VariableType,
	       *BlueprintPath);
	return true;
}

FString UGenBlueprintUtils::AddFunction(const FString& BlueprintPath, const FString& FunctionName,
                                        const FString& InputsJson, const FString& OutputsJson)
{
	// Load the blueprint asset
	UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
		return TEXT("");
	}

	// Create a new function graph
	UEdGraph* FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(
		Blueprint,
		FName(*FunctionName),
		UEdGraph::StaticClass(),
		UEdGraphSchema_K2::StaticClass());

	if (!FunctionGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create function graph for function %s"), *FunctionName);
		return TEXT("");
	}

	// Setup the function entry node
	UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Add the function to the blueprint
	FBlueprintEditorUtils::AddFunctionGraph(Blueprint, FunctionGraph, /*bIsUserCreated=*/ true, /*UObjectClass=*/
	                                        static_cast<UClass*>(nullptr));

	// Get the function entry node
	UK2Node_FunctionEntry* EntryNode = nullptr;
	for (TObjectIterator<UK2Node_FunctionEntry> It; It; ++It)
	{
		if (It->GetGraph() == FunctionGraph)
		{
			EntryNode = *It;
			break;
		}
	}

	if (!EntryNode)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find function entry node for function %s"), *FunctionName);
		return TEXT("");
	}

	// Parse inputs and outputs from JSON
	// This is a simplified version - a proper implementation would need more robust JSON parsing
	TArray<TSharedPtr<FJsonValue>> Inputs;
	TArray<TSharedPtr<FJsonValue>> Outputs;

	TSharedRef<TJsonReader<>> InputReader = TJsonReaderFactory<>::Create(InputsJson);
	TSharedRef<TJsonReader<>> OutputReader = TJsonReaderFactory<>::Create(OutputsJson);

	FJsonSerializer::Deserialize(InputReader, Inputs);
	FJsonSerializer::Deserialize(OutputReader, Outputs);

	// Add inputs
	for (auto& Input : Inputs)
	{
		auto InputObj = Input->AsObject();
		if (InputObj.IsValid())
		{
			FString ParamName = InputObj->GetStringField(TEXT("name"));
			FString ParamType = InputObj->GetStringField(TEXT("type"));

			// Create and add pin based on type - simplified version
			if (ParamType.Equals(TEXT("boolean"), ESearchCase::IgnoreCase))
			{
				FEdGraphPinType PinType;
				PinType.PinCategory = K2Schema->PC_Boolean;

				EntryNode->CreateUserDefinedPin(
					*ParamName,
					PinType,
					EGPD_Output);
			}
			else if (ParamType.Equals(TEXT("int"), ESearchCase::IgnoreCase))
			{
				FEdGraphPinType PinType;
				PinType.PinCategory = K2Schema->PC_Int;
				EntryNode->CreateUserDefinedPin(
					*ParamName,
					PinType,
					EGPD_Output);
			}
			else if (ParamType.Equals(TEXT("float"), ESearchCase::IgnoreCase))
			{
				FEdGraphPinType PinType;
				PinType.PinCategory = K2Schema->PC_Float;
				EntryNode->CreateUserDefinedPin(
					*ParamName,
					PinType,
					EGPD_Output);
			}
			else if (ParamType.Equals(TEXT("string"), ESearchCase::IgnoreCase))
			{
				FEdGraphPinType PinType;
				PinType.PinCategory = K2Schema->PC_String;
				EntryNode->CreateUserDefinedPin(
					*ParamName,
					PinType,
					EGPD_Output);
			}
			// More types would need to be added for a complete implementation
		}
	}

	// Add outputs
	// For this simplified version, we're just adding a single return node
	UK2Node_FunctionResult* ResultNode = nullptr;
	for (TObjectIterator<UK2Node_FunctionResult> It; It; ++It)
	{
		if (It->GetGraph() == FunctionGraph)
		{
			ResultNode = *It;
			break;
		}
	}

	if (ResultNode)
	{
		for (auto& Output : Outputs)
		{
			auto OutputObj = Output->AsObject();
			if (OutputObj.IsValid())
			{
				FString ParamName = OutputObj->GetStringField(TEXT("name"));
				FString ParamType = OutputObj->GetStringField(TEXT("type"));

				// Create and add pin based on type - simplified version
				if (ParamType.Equals(TEXT("boolean"), ESearchCase::IgnoreCase))
				{
					FEdGraphPinType PinType;
					PinType.PinCategory = K2Schema->PC_Boolean;
					ResultNode->CreateUserDefinedPin(
						*ParamName,
						PinType,
						EGPD_Input);
				}
				else if (ParamType.Equals(TEXT("int"), ESearchCase::IgnoreCase))
				{
					FEdGraphPinType PinType;
					PinType.PinCategory = K2Schema->PC_Int;
					ResultNode->CreateUserDefinedPin(
						*ParamName,
						PinType,
						EGPD_Input);
				}
				else if (ParamType.Equals(TEXT("float"), ESearchCase::IgnoreCase))
				{
					FEdGraphPinType PinType;
					PinType.PinCategory = K2Schema->PC_Float;
					ResultNode->CreateUserDefinedPin(
						*ParamName,
						PinType,
						EGPD_Input);
				}
				else if (ParamType.Equals(TEXT("string"), ESearchCase::IgnoreCase))
				{
					FEdGraphPinType PinType;
					PinType.PinCategory = K2Schema->PC_String;
					ResultNode->CreateUserDefinedPin(
						*ParamName,
						PinType,
						EGPD_Input);
				}
				// More types would need to be added for a complete implementation
			}
		}
	}

	// Mark the blueprint as modified
	Blueprint->Modify();

	// Compile the blueprint
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	OpenBlueprintGraph(Blueprint, FunctionGraph);

	// Return the function graph GUID as a string
	UE_LOG(LogTemp, Log, TEXT("Created function %s in blueprint %s"), *FunctionName, *BlueprintPath);
	return FunctionGraph->GraphGuid.ToString();
}

FString UGenBlueprintUtils::ConnectNodes(const FString& BlueprintPath, const FString& FunctionGuid,
                                         const FString& SourceNodeGuid, const FString& SourcePinName,
                                         const FString& TargetNodeGuid, const FString& TargetPinName)
{
    UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
    if (!Blueprint) return TEXT("{\"success\": false, \"error\": \"Could not load blueprint\"}");

    FGuid GraphGuid;
    if (!FGuid::Parse(FunctionGuid, GraphGuid))
        return TEXT("{\"success\": false, \"error\": \"Invalid function GUID\"}");

    UEdGraph* FunctionGraph = nullptr;
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
        if (Graph->GraphGuid == GraphGuid) { FunctionGraph = Graph; break; }
    if (!FunctionGraph)
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
            if (Graph->GraphGuid == GraphGuid) { FunctionGraph = Graph; break; }
    if (!FunctionGraph)
        return TEXT("{\"success\": false, \"error\": \"Could not find function graph\"}");

    FGuid SourceGuid, TargetGuid;
    if (!FGuid::Parse(SourceNodeGuid, SourceGuid) || !FGuid::Parse(TargetNodeGuid, TargetGuid))
        return TEXT("{\"success\": false, \"error\": \"Invalid node GUID\"}");

    UK2Node* SourceNode = nullptr;
    UK2Node* TargetNode = nullptr;
    for (UEdGraphNode* Node : FunctionGraph->Nodes)
    {
        if (UK2Node* K2Node = Cast<UK2Node>(Node))
        {
            if (K2Node->NodeGuid == SourceGuid) SourceNode = K2Node;
            else if (K2Node->NodeGuid == TargetGuid) TargetNode = K2Node;
        }
    }

    if (!SourceNode || !TargetNode)
        return TEXT("{\"success\": false, \"error\": \"Could not find source or target node\"}");

    UEdGraphPin* SourcePin = SourceNode->FindPin(FName(*SourcePinName), EGPD_Output);
    UEdGraphPin* TargetPin = TargetNode->FindPin(FName(*TargetPinName), EGPD_Input);

    if (!SourcePin || !TargetPin)
    {
        TSharedPtr<FJsonObject> ResponseObject = MakeShareable(new FJsonObject);
        ResponseObject->SetBoolField(TEXT("success"), false);
        ResponseObject->SetStringField(TEXT("error"), TEXT("Invalid pin name"));

        auto AddPins = [](UK2Node* Node, const FString& FieldName, TSharedPtr<FJsonObject> JsonObj, EEdGraphPinDirection Direction)
        {
            TArray<TSharedPtr<FJsonValue>> PinsArray;
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin->Direction == Direction)
                {
                    TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
                    PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                    PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
                    PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                    if (Pin->PinType.PinSubCategory != NAME_None)
                        PinObj->SetStringField(TEXT("subtype"), Pin->PinType.PinSubCategory.ToString());
                    PinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
                }
            }
            JsonObj->SetArrayField(FieldName, PinsArray);
        };

        AddPins(SourceNode, TEXT("source_available_pins"), ResponseObject, EGPD_Output);
        AddPins(TargetNode, TEXT("target_available_pins"), ResponseObject, EGPD_Input);

        FString ResultJson;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
        FJsonSerializer::Serialize(ResponseObject.ToSharedRef(), Writer);
        return ResultJson;
    }

    // Attempt the connection directly, letting Unreal handle type conversion
    SourcePin->MakeLinkTo(TargetPin);

    // Verify if the connection was successful
    if (SourcePin->LinkedTo.Contains(TargetPin) && TargetPin->LinkedTo.Contains(SourcePin))
    {
        Blueprint->Modify();
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

        TSharedPtr<FJsonObject> ResponseObject = MakeShareable(new FJsonObject);
        ResponseObject->SetBoolField(TEXT("success"), true);
        FString ResultJson;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
        FJsonSerializer::Serialize(ResponseObject.ToSharedRef(), Writer);
        return ResultJson;
    }
    else
    {
        // Connection failed, provide detailed feedback
        TSharedPtr<FJsonObject> ResponseObject = MakeShareable(new FJsonObject);
        ResponseObject->SetBoolField(TEXT("success"), false);
        ResponseObject->SetStringField(TEXT("error"), TEXT("Failed to connect pins - type mismatch or invalid connection"));

        TSharedPtr<FJsonObject> SourcePinInfo = MakeShareable(new FJsonObject);
        SourcePinInfo->SetStringField(TEXT("name"), SourcePin->PinName.ToString());
        SourcePinInfo->SetStringField(TEXT("type"), SourcePin->PinType.PinCategory.ToString());
        if (SourcePin->PinType.PinSubCategory != NAME_None)
            SourcePinInfo->SetStringField(TEXT("subtype"), SourcePin->PinType.PinSubCategory.ToString());

        TSharedPtr<FJsonObject> TargetPinInfo = MakeShareable(new FJsonObject);
        TargetPinInfo->SetStringField(TEXT("name"), TargetPin->PinName.ToString());
        TargetPinInfo->SetStringField(TEXT("type"), TargetPin->PinType.PinCategory.ToString());
        if (TargetPin->PinType.PinSubCategory != NAME_None)
            TargetPinInfo->SetStringField(TEXT("subtype"), TargetPin->PinType.PinSubCategory.ToString());

        ResponseObject->SetObjectField(TEXT("source_pin"), SourcePinInfo);
        ResponseObject->SetObjectField(TEXT("target_pin"), TargetPinInfo);

        FString ResultJson;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
        FJsonSerializer::Serialize(ResponseObject.ToSharedRef(), Writer);
        return ResultJson;
    }
}

bool UGenBlueprintUtils::CompileBlueprint(const FString& BlueprintPath)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint) return false;
    
	// Validate pin connections before compiling
	TArray<FString> InvalidConnectionMessages;
	bool HasInvalidConnections = false;
    
	// Check all graphs
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			for (UEdGraphPin* Pin : Node->Pins)
			{
				// Check for multiple connections to exec output pins (a common issue)
				if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
				{
					if (Pin->LinkedTo.Num() > 1)
					{
						HasInvalidConnections = true;
						// Optionally fix by keeping only the first connection
						while (Pin->LinkedTo.Num() > 1)
						{
							Pin->LinkedTo.RemoveAt(1);
						}
						FString Message = FString::Printf(TEXT("Fixed invalid multiple connections from pin %s on node %s"), 
							*Pin->PinName.ToString(), *Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
						InvalidConnectionMessages.Add(Message);
					}
				}
			}
		}
	}
    
	// If issues were found and fixed, mark blueprint modified
	if (HasInvalidConnections)
	{
		Blueprint->Modify();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
    
	// Now compile
	UE_LOG(LogTemp, Log, TEXT("Compiled blueprint: %s"), *BlueprintPath);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
    
	return true;
}

AActor* UGenBlueprintUtils::SpawnBlueprint(const FString& BlueprintPath, const FVector& Location,
                                           const FRotator& Rotation, const FVector& Scale,
                                           const FString& ActorLabel)
{
	// Load the blueprint asset
	UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
		return nullptr;
	}

	// Make sure the blueprint has been compiled
	if (!Blueprint->GeneratedClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Blueprint has not been compiled"));
		return nullptr;
	}

	// Get the world
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get editor world"));
		return nullptr;
	}

	// Spawn the actor
	AActor* SpawnedActor = World->SpawnActor(Blueprint->GeneratedClass, &Location, &Rotation);
	if (!SpawnedActor)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn blueprint actor"));
		return nullptr;
	}

	// Set the scale
	SpawnedActor->SetActorScale3D(Scale);

	// Set the label if provided
	if (!ActorLabel.IsEmpty())
	{
		SpawnedActor->SetActorLabel(*ActorLabel);
	}

	UE_LOG(LogTemp, Log, TEXT("Spawned blueprint %s actor at location %s"), *BlueprintPath, *Location.ToString());
	return SpawnedActor;
}

// Helper functions
UBlueprint* UGenBlueprintUtils::LoadBlueprintAsset(const FString& BlueprintPath)
{
	return LoadObject<UBlueprint>(nullptr, *BlueprintPath);
}

UClass* UGenBlueprintUtils::FindClassByName(const FString& ClassName)
{
	// Try to find the class by name
	UClass* Class = FindObject<UClass>(ANY_PACKAGE, *ClassName);

	// If not found, try with various prefixes
	if (!Class)
	{
		FString PrefixedClassName = TEXT("/Script/Engine.") + ClassName;
		Class = FindObject<UClass>(ANY_PACKAGE, *PrefixedClassName);
	}

	if (!Class)
	{
		FString PrefixedClassName = TEXT("Blueprint'/Game/") + ClassName + TEXT(".") + ClassName + TEXT("_C'");
		Class = LoadObject<UClass>(nullptr, *PrefixedClassName);
	}

	return Class;
}

UFunction* UGenBlueprintUtils::FindFunctionByName(UClass* Class, const FString& FunctionName)
{
	if (!Class)
	{
		return nullptr;
	}

	return Class->FindFunctionByName(*FunctionName);
}


FString UGenBlueprintUtils::ConnectNodesBulk(const FString& BlueprintPath, const FString& FunctionGuid,
                                             const FString& ConnectionsJson)
{
    // Load the blueprint asset
    UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
    if (!Blueprint)
    {
       UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
       return TEXT("{\"success\": false, \"error\": \"Could not load blueprint\"}");
    }

    // Parse the connections array from JSON
    TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ConnectionsJson);

    if (!FJsonSerializer::Deserialize(Reader, ConnectionsArray))
    {
       UE_LOG(LogTemp, Error, TEXT("Failed to parse connections JSON"));
       return TEXT("{\"success\": false, \"error\": \"Failed to parse connections JSON\"}");
    }

    // Create a response array to track all connection results
    TArray<TSharedPtr<FJsonValue>> ResultsArray;
    int SuccessfulConnections = 0;
    bool HasErrors = false;

    for (int32 i = 0; i < ConnectionsArray.Num(); i++)
    {
       TSharedPtr<FJsonObject> ConnectionObject = ConnectionsArray[i]->AsObject();
       if (!ConnectionObject.IsValid()) continue;

       // Extract connection details
       FString SourceNodeGuid = ConnectionObject->GetStringField(TEXT("source_node_id"));
       FString SourcePinName = ConnectionObject->GetStringField(TEXT("source_pin"));
       FString TargetNodeGuid = ConnectionObject->GetStringField(TEXT("target_node_id"));
       FString TargetPinName = ConnectionObject->GetStringField(TEXT("target_pin"));

       // Connect the nodes - UNCOMMENTED THIS!
       FString ConnectionResult = ConnectNodes(BlueprintPath, FunctionGuid, SourceNodeGuid, SourcePinName, 
                                          TargetNodeGuid, TargetPinName);

       // Parse the result
       TSharedPtr<FJsonObject> ResultObject;
       TSharedRef<TJsonReader<>> ResultReader = TJsonReaderFactory<>::Create(ConnectionResult);
       
       if (FJsonSerializer::Deserialize(ResultReader, ResultObject) && ResultObject.IsValid())
       {
           // Add connection index and source/target identifiers
           ResultObject->SetNumberField(TEXT("connection_index"), i);
           ResultObject->SetStringField(TEXT("source_node"), SourceNodeGuid);
           ResultObject->SetStringField(TEXT("target_node"), TargetNodeGuid);
           
           // Check if successful
           bool bSuccess = false;
           if (ResultObject->TryGetBoolField(TEXT("success"), bSuccess) && bSuccess)
           {
               SuccessfulConnections++;
           }
           else
           {
               HasErrors = true;
           }
           
           ResultsArray.Add(MakeShareable(new FJsonValueObject(ResultObject)));
       }
       else
       {
           // Couldn't parse result, create error object
           TSharedPtr<FJsonObject> ErrorObject = MakeShareable(new FJsonObject);
           ErrorObject->SetBoolField(TEXT("success"), false);
           ErrorObject->SetStringField(TEXT("error"), TEXT("Failed to parse connection result"));
           ErrorObject->SetNumberField(TEXT("connection_index"), i);
           ErrorObject->SetStringField(TEXT("source_node"), SourceNodeGuid);
           ErrorObject->SetStringField(TEXT("target_node"), TargetNodeGuid);
           
           ResultsArray.Add(MakeShareable(new FJsonValueObject(ErrorObject)));
           HasErrors = true;
       }
    }

    // Create the final response
    TSharedPtr<FJsonObject> ResponseObject = MakeShareable(new FJsonObject);
    ResponseObject->SetBoolField(TEXT("success"), !HasErrors);
    ResponseObject->SetNumberField(TEXT("total_connections"), ConnectionsArray.Num());
    ResponseObject->SetNumberField(TEXT("successful_connections"), SuccessfulConnections);
    ResponseObject->SetArrayField(TEXT("results"), ResultsArray);

    FString ResultJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
    FJsonSerializer::Serialize(ResponseObject.ToSharedRef(), Writer);

    UE_LOG(LogTemp, Log, TEXT("Connected %d/%d node pairs in blueprint %s"), 
          SuccessfulConnections, ConnectionsArray.Num(), *BlueprintPath);
    
    return ResultJson;
}

bool UGenBlueprintUtils::OpenBlueprintGraph(UBlueprint* Blueprint, UEdGraph* Graph)
{
	if (!Blueprint || !GEditor)
		return false;
        
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
		return false;
		
	// First check if the blueprint editor is already open
	IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Blueprint, false);
    
        
	if (!EditorInstance)
	{
		// Open the blueprint editor if not already open
		EditorInstance = AssetEditorSubsystem->OpenEditorForAsset(Blueprint, EToolkitMode::WorldCentric) 
			? AssetEditorSubsystem->FindEditorForAsset(Blueprint, false) 
			: nullptr;
            
		if (!EditorInstance)
			return false;
	}
    
	// Try to cast to blueprint editor
	FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(EditorInstance);
	if (BlueprintEditor && Graph != nullptr)
	{
		// Only try to open the specific graph if it's provided and has nodes
		if (Graph != nullptr)
		{
			// Safety check for empty graphs to avoid crashes
			if (Graph->Nodes.Num() > 0)
			{
				// Focus on the specific graph
				BlueprintEditor->OpenGraphAndBringToFront(Graph);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Skipping opening empty graph in blueprint %s"), 
					*Blueprint->GetName());
			}
		}
        
		// Ensure the editor has focus regardless
		BlueprintEditor->FocusWindow();
		return true;
	}
	
    return false;
}


//
// bool UGenBlueprintUtils::OpenSceneTab()
// {
// 	if (!GEditor)
// 		return false;
//
// 	// Get the level editor subsystem
// 	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
//
// 	// Get the tab manager for the level editor
// 	TSharedPtr<FTabManager> TabManager = LevelEditorModule.GetLevelEditorTabManager();
// 	if (!TabManager.IsValid())
// 		return false;
//
// 	// Find and activate the viewport tab (this is the main scene tab)
// 	TSharedPtr<SDockTab> ViewportTab = TabManager->FindExistingLiveTab(FName("LevelEditorViewport"));
// 	if (ViewportTab.IsValid())
// 	{
// 		// Bring the tab to front
// 		ViewportTab->ActivateInParent(ETabActivationCause::UserClickedOnTab);
//
// 		// Focus the viewport
// 		// Example of casting if needed
// 		TSharedPtr<SLevelViewport> LevelViewport = StaticCastSharedPtr<SLevelViewport>(ViewportTab->GetContent());
// 		if (Viewport.IsValid())
// 		{
// 			Viewport->GetViewportWidget()->SetFocus();
// 			return true;
// 		}
// 	}
//     
// 	// If the tab wasn't found, try to create/show it
// 	LevelEditorModule.GetLevelEditorTabManager()->TryInvokeTab(FName("LevelEditorViewport"));
//     
// 	// Return success if we found or created the tab
// 	return TabManager->FindExistingLiveTab(FName("LevelEditorViewport")).IsValid();
// }


FString UGenBlueprintUtils::GetNodeGUID(const FString& BlueprintPath, const FString& GraphType, 
                                        const FString& NodeName, const FString& FunctionGuid)
{
    UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
    if (!Blueprint) return TEXT("{\"success\": false, \"error\": \"Could not load blueprint\"}");

    UEdGraph* TargetGraph = nullptr;
    if (GraphType.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
    {
        if (NodeName.IsEmpty()) return TEXT("{\"success\": false, \"error\": \"Node name required for EventGraph\"}");
        if (Blueprint->UbergraphPages.Num() > 0)
            TargetGraph = Blueprint->UbergraphPages[0];
    }
    else if (GraphType.Equals(TEXT("FunctionGraph"), ESearchCase::IgnoreCase))
    {
        if (FunctionGuid.IsEmpty()) return TEXT("{\"success\": false, \"error\": \"Function GUID required for FunctionGraph\"}");
        FGuid GraphGuid;
        if (!FGuid::Parse(FunctionGuid, GraphGuid)) return TEXT("{\"success\": false, \"error\": \"Invalid function GUID\"}");
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph->GraphGuid == GraphGuid)
            {
                TargetGraph = Graph;
                break;
            }
        }
    }

    if (!TargetGraph) return TEXT("{\"success\": false, \"error\": \"Could not find specified graph\"}");

    UK2Node* TargetNode = nullptr;
    if (GraphType.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
    {
        for (UEdGraphNode* Node : TargetGraph->Nodes)
        {
            if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
            {
                FString EventName = EventNode->EventReference.GetMemberName().ToString();
                if (EventName.Equals(NodeName, ESearchCase::IgnoreCase))
                {
                    TargetNode = EventNode;
                    break;
                }
                else if (NodeName.Equals(TEXT("BeginPlay")) || NodeName.Equals(TEXT("Tick")))
                {
                    if (EventNode->bIsEditable && EventNode->GetName().Contains(NodeName))
                    {
                        TargetNode = EventNode;
                        break;
                    }
                }
            }
        }

        // Optionally create the event if it doesn’t exist
        if (!TargetNode && (NodeName.Equals(TEXT("BeginPlay")) || NodeName.Equals(TEXT("Tick"))))
        {

        	UClass* BlueprintClass = Blueprint->GeneratedClass;
        	UFunction* Function = BlueprintClass->FindFunctionByName(*NodeName);
        	if (Function)
        	{
        		int NodePosY = 0;
        		UK2Node_Event* NewEventNode = FKismetEditorUtilities::AddDefaultEventNode(
					Blueprint, 
					TargetGraph, 
					FName(*NodeName), 
					BlueprintClass,
					NodePosY);
        		TargetNode = NewEventNode;
        		UE_LOG(LogTemp, Log, TEXT("Added event node: %s"), *NodeName);
        	}
        	else {
        		UE_LOG(LogTemp, Error, TEXT("Failed to add event node: %s"), *NodeName);
        	}
            if (TargetNode)
            {
                Blueprint->Modify();
                FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
            }
        }
    }
    else if (GraphType.Equals(TEXT("FunctionGraph"), ESearchCase::IgnoreCase))
    {
        for (UEdGraphNode* Node : TargetGraph->Nodes)
        {
            if (UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node))
            {
                TargetNode = EntryNode;
                break;
            }
        }
    }

    if (!TargetNode) return TEXT("{\"success\": false, \"error\": \"Node not found\"}");

    if (!TargetNode->NodeGuid.IsValid())
        TargetNode->NodeGuid = FGuid::NewGuid();

    TSharedPtr<FJsonObject> ResponseObject = MakeShareable(new FJsonObject);
    ResponseObject->SetBoolField(TEXT("success"), true);
    ResponseObject->SetStringField(TEXT("node_guid"), TargetNode->NodeGuid.ToString());
    TArray<TSharedPtr<FJsonValue>> PositionArray;
    PositionArray.Add(MakeShareable(new FJsonValueNumber(TargetNode->NodePosX)));
    PositionArray.Add(MakeShareable(new FJsonValueNumber(TargetNode->NodePosY)));
    ResponseObject->SetArrayField(TEXT("position"), PositionArray);

    FString ResultJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
    FJsonSerializer::Serialize(ResponseObject.ToSharedRef(), Writer);
    return ResultJson;
}