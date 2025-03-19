// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.
#include "MCP/GenBlueprintUtils.h"

#include "BlueprintEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Factories/BlueprintFactory.h"
#include "EditorLevelLibrary.h"
#include "EditorUtilityLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "Blueprint/BlueprintSupport.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "KismetCompiler.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealTypePrivate.h"

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

	// Open the Blueprint editor
	if (GEditor)
	{
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (AssetEditorSubsystem)
		{
			AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
		}
	}

	// Return the function graph GUID as a string
	UE_LOG(LogTemp, Log, TEXT("Created function %s in blueprint %s"), *FunctionName, *BlueprintPath);
	return FunctionGraph->GraphGuid.ToString();
}

FString UGenBlueprintUtils::AddNode(const FString& BlueprintPath, const FString& FunctionGuid,
                                    const FString& NodeType, float NodeX, float NodeY,
                                    const FString& PropertiesJson)
{
	// Load the blueprint asset
	UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
		return TEXT("");
	}

	// Find the function graph by GUID
	FGuid GraphGuid;
	if (!FGuid::Parse(FunctionGuid, GraphGuid))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid GUID format: %s"), *FunctionGuid);
		return TEXT("");
	}

	UEdGraph* FunctionGraph = nullptr;
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph->GraphGuid == GraphGuid)
		{
			FunctionGraph = Graph;
			break;
		}
	}

	if (!FunctionGraph)
	{
		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
		{
			if (Graph->GraphGuid == GraphGuid)
			{
				FunctionGraph = Graph;
				break;
			}
		}
	}

	if (!FunctionGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find function graph with GUID: %s"), *FunctionGuid);
		return TEXT("");
	}

	// Create the node based on its type
	UK2Node* NewNode = nullptr;

	// Check if it's a function call node
	UClass* TargetClass = FindClassByName("Actor"); // Default to Actor if not specified
	if (!TargetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find target class"));
		return TEXT("");
	}

	UFunction* TargetFunction = FindFunctionByName(TargetClass, NodeType);
	if (TargetFunction)
	{
		// It's a function call, create a function call node
		UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
		FunctionNode->FunctionReference.SetExternalMember(TargetFunction->GetFName(), TargetClass);
		NewNode = FunctionNode;
	}
	else if (NodeType.Equals(TEXT("Branch"), ESearchCase::IgnoreCase))
	{
		// Special case for Branch node
		NewNode = NewObject<UK2Node_IfThenElse>(FunctionGraph);
	}
	else if (NodeType.Equals(TEXT("Sequence"), ESearchCase::IgnoreCase))
	{
		// Special case for Sequence node
		NewNode = NewObject<UK2Node_ExecutionSequence>(FunctionGraph);
	}
	else if (NodeType.Equals(TEXT("Print"), ESearchCase::IgnoreCase) ||
		NodeType.Equals(TEXT("PrintString"), ESearchCase::IgnoreCase))
	{
		// Special case for Print String node
		UK2Node_CallFunction* PrintNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
		UClass* PrintClass = FindObject<UClass>(ANY_PACKAGE, TEXT("KismetSystemLibrary"));
		if (PrintClass)
		{
			UFunction* PrintFunction = FindFunctionByName(PrintClass, TEXT("PrintString"));
			if (PrintFunction)
			{
				PrintNode->FunctionReference.SetExternalMember(PrintFunction->GetFName(), PrintClass);
				NewNode = PrintNode;
			}
		}
	}
	else if (NodeType.Equals(TEXT("Delay"), ESearchCase::IgnoreCase))
	{
		UK2Node_CallFunction* DelayNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
		UClass* KismetClass = FindObject<UClass>(ANY_PACKAGE, TEXT("KismetSystemLibrary"));
		if (KismetClass)
		{
			UFunction* DelayFunction = FindFunctionByName(KismetClass, TEXT("Delay"));
			if (DelayFunction)
			{
				DelayNode->FunctionReference.SetExternalMember(DelayFunction->GetFName(), KismetClass);
				NewNode = DelayNode;
			}
		}
	}
	else if (NodeType.Equals(TEXT("ReturnNode"), ESearchCase::IgnoreCase))
	{
		NewNode = NewObject<UK2Node_FunctionResult>(FunctionGraph);
	}
	else
	{
		// Try to find a matching node type
		FString NodeClassName = TEXT("UK2Node_") + NodeType;
		UClass* NodeClass = FindObject<UClass>(ANY_PACKAGE, *NodeClassName);
		if (NodeClass && NodeClass->IsChildOf(UK2Node::StaticClass()))
		{
			NewNode = NewObject<UK2Node>(FunctionGraph, NodeClass);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Unsupported node type: %s"), *NodeType);
			return TEXT("");
		}
	}

	if (!NewNode)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create node of type: %s"), *NodeType);
		return TEXT("");
	}

	// Add the node to the graph
	FunctionGraph->AddNode(NewNode, false, false);

	// Set the node position
	NewNode->NodePosX = NodeX;
	NewNode->NodePosY = NodeY;

	// Set node properties from JSON
	if (!PropertiesJson.IsEmpty())
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(PropertiesJson);

		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			// Process node properties - this is simplified
			// A complete implementation would handle different property types
			for (auto& Prop : JsonObject->Values)
			{
				const FString& PropName = Prop.Key;
				TSharedPtr<FJsonValue> PropValue = Prop.Value;

				if (PropValue->Type == EJson::String)
				{
					const FString StringValue = PropValue->AsString();

					// Find the pin by name
					UEdGraphPin* Pin = NewNode->FindPin(FName(*PropName));
					if (Pin)
					{
						Pin->DefaultValue = StringValue;
					}
				}
				else if (PropValue->Type == EJson::Number)
				{
					const double NumValue = PropValue->AsNumber();

					// Find the pin by name
					UEdGraphPin* Pin = NewNode->FindPin(FName(*PropName));
					if (Pin)
					{
						Pin->DefaultValue = FString::SanitizeFloat(NumValue);
					}
				}
				else if (PropValue->Type == EJson::Boolean)
				{
					const bool BoolValue = PropValue->AsBool();

					// Find the pin by name
					UEdGraphPin* Pin = NewNode->FindPin(FName(*PropName));
					if (Pin)
					{
						Pin->DefaultValue = BoolValue ? TEXT("true") : TEXT("false");
					}
				}
			}
		}
	}

	// Reconstruct the node
	NewNode->ReconstructNode();

	// Mark the blueprint as modified
	Blueprint->Modify();

	// Notify blueprint that the graph changed
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	OpenBlueprintGraph(Blueprint, FunctionGraph);

	// Generate a new GUID if the node doesn't have one
	if (NewNode->NodeGuid.IsValid() == false)
	{
		NewNode->NodeGuid = FGuid::NewGuid();
	}

	UE_LOG(LogTemp, Log, TEXT("Added node of type %s to blueprint %s with GUID %s"),
	       *NodeType, *BlueprintPath, *NewNode->NodeGuid.ToString());

	// Return the actual node GUID
	return NewNode->NodeGuid.ToString();
}

bool UGenBlueprintUtils::ConnectNodes(const FString& BlueprintPath, const FString& FunctionGuid,
                                      const FString& SourceNodeGuid, const FString& SourcePinName,
                                      const FString& TargetNodeGuid, const FString& TargetPinName)
{
	// Load the blueprint asset
	UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
		return false;
	}

	// Find the function graph by GUID
	FGuid GraphGuid;
	if (!FGuid::Parse(FunctionGuid, GraphGuid))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid function GUID format: %s"), *FunctionGuid);
		return false;
	}

	UEdGraph* FunctionGraph = nullptr;
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph->GraphGuid == GraphGuid)
		{
			FunctionGraph = Graph;
			break;
		}
	}

	if (!FunctionGraph)
	{
		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
		{
			if (Graph->GraphGuid == GraphGuid)
			{
				FunctionGraph = Graph;
				break;
			}
		}
	}

	if (!FunctionGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find function graph with GUID: %s"), *FunctionGuid);
		return false;
	}

	// Find the source and target nodes
	FGuid SourceGuid, TargetGuid;
	if (!FGuid::Parse(SourceNodeGuid, SourceGuid) || !FGuid::Parse(TargetNodeGuid, TargetGuid))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid node GUID format"));
		return false;
	}

	UK2Node* SourceNode = nullptr;
	UK2Node* TargetNode = nullptr;

	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		UK2Node* K2Node = Cast<UK2Node>(Node);
		if (K2Node)
		{
			if (K2Node->NodeGuid == SourceGuid)
			{
				SourceNode = K2Node;
			}
			else if (K2Node->NodeGuid == TargetGuid)
			{
				TargetNode = K2Node;
			}
		}
	}

	if (!SourceNode || !TargetNode)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find source or target node"));
		return false;
	}

	// Find the source and target pins
	UEdGraphPin* SourcePin = SourceNode->FindPin(FName(*SourcePinName));
	UEdGraphPin* TargetPin = TargetNode->FindPin(FName(*TargetPinName));

	if (!SourcePin || !TargetPin)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find source or target pin"));
		return false;
	}

	// Connect the pins
	SourcePin->MakeLinkTo(TargetPin);

	// Mark the blueprint as modified
	Blueprint->Modify();

	// Notify blueprint that the graph changed
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	UE_LOG(LogTemp, Log, TEXT("Connected %s.%s to %s.%s in blueprint %s"),
	       *SourceNodeGuid, *SourcePinName, *TargetNodeGuid, *TargetPinName, *BlueprintPath);
	return true;
}

bool UGenBlueprintUtils::CompileBlueprint(const FString& BlueprintPath)
{
	// Load the blueprint asset
	UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load blueprint: %s"), *BlueprintPath);
		return false;
	}

	// Compile the blueprint
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	UE_LOG(LogTemp, Log, TEXT("Compiled blueprint: %s"), *BlueprintPath);
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


FString UGenBlueprintUtils::AddNodesBulk(const FString& BlueprintPath, const FString& FunctionGuid,
                                     const FString& NodesJson)
{
    // Load the blueprint asset
    UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
        return TEXT("");
    }

    // Find the function graph by GUID
    FGuid GraphGuid;
    if (!FGuid::Parse(FunctionGuid, GraphGuid))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid GUID format: %s"), *FunctionGuid);
        return TEXT("");
    }

    UEdGraph* FunctionGraph = nullptr;
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph->GraphGuid == GraphGuid)
        {
            FunctionGraph = Graph;
            break;
        }
    }

    if (!FunctionGraph)
    {
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph->GraphGuid == GraphGuid)
            {
                FunctionGraph = Graph;
                break;
            }
        }
    }

    if (!FunctionGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find function graph with GUID: %s"), *FunctionGuid);
        return TEXT("");
    }

    // Parse the nodes array from JSON
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NodesJson);

    if (!FJsonSerializer::Deserialize(Reader, NodesArray))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse nodes JSON"));
        return TEXT("");
    }

    // Create a JSON object to store the results
    TSharedPtr<FJsonObject> ResultsObject = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> ResultsArray;

    // Process each node definition
    for (auto& NodeValue : NodesArray)
    {
        TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
        if (!NodeObject.IsValid()) continue;

        FString NodeType = NodeObject->GetStringField(TEXT("node_type"));
    	// Get position from array
    	TArray<TSharedPtr<FJsonValue>> PositionArray = NodeObject->GetArrayField(TEXT("node_position"));
    	double NodeX = PositionArray.Num() > 0 ? PositionArray[0]->AsNumber() : 0.0;
    	double NodeY = PositionArray.Num() > 1 ? PositionArray[1]->AsNumber() : 0.0;

        // Get properties JSON if available
        FString PropertiesJson;
        if (NodeObject->HasField(TEXT("node_properties")))
        {
            TSharedPtr<FJsonObject> PropsObject = NodeObject->GetObjectField(TEXT("node_properties"));
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PropertiesJson);
            FJsonSerializer::Serialize(PropsObject.ToSharedRef(), Writer);
        }

        // Get node ID from the node definition if available (for reference in results)
        FString NodeRefId;
        if (NodeObject->HasField(TEXT("id")))
        {
            NodeRefId = NodeObject->GetStringField(TEXT("id"));
        }

        // Create the node
        FString NodeGuid = AddNode(BlueprintPath, FunctionGuid, NodeType, NodeX, NodeY, PropertiesJson);
        
        if (!NodeGuid.IsEmpty())
        {
            // Add this node's result to the results array
            TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
            ResultObject->SetStringField(TEXT("node_guid"), NodeGuid);
            
            if (!NodeRefId.IsEmpty())
            {
                ResultObject->SetStringField(TEXT("ref_id"), NodeRefId);
            }
            
            ResultsArray.Add(MakeShareable(new FJsonValueObject(ResultObject)));
        }
    }

    // Convert results to JSON string
    FString ResultsJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultsJson);
    FJsonSerializer::Serialize(ResultsArray, Writer);

    // Mark the blueprint as modified
    Blueprint->Modify();

    // Notify blueprint that the graph changed
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

    UE_LOG(LogTemp, Log, TEXT("Added %d nodes to blueprint %s"), ResultsArray.Num(), *BlueprintPath);
    return ResultsJson;
}



bool UGenBlueprintUtils::ConnectNodesBulk(const FString& BlueprintPath, const FString& FunctionGuid,
									  const FString& ConnectionsJson)
{
	// Load the blueprint asset
	UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
		return false;
	}

	// Parse the connections array from JSON
	TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ConnectionsJson);

	if (!FJsonSerializer::Deserialize(Reader, ConnectionsArray))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse connections JSON"));
		return false;
	}

	// Connect each pair of nodes
	bool AllSuccessful = true;
	int SuccessfulConnections = 0;

	for (auto& ConnectionValue : ConnectionsArray)
	{
		TSharedPtr<FJsonObject> ConnectionObject = ConnectionValue->AsObject();
		if (!ConnectionObject.IsValid()) continue;

		// Extract connection details
		FString SourceNodeGuid = ConnectionObject->GetStringField(TEXT("source_node_id"));
		FString SourcePinName = ConnectionObject->GetStringField(TEXT("source_pin"));
		FString TargetNodeGuid = ConnectionObject->GetStringField(TEXT("target_node_id"));
		FString TargetPinName = ConnectionObject->GetStringField(TEXT("target_pin"));

		// Connect the nodes
		bool Success = ConnectNodes(BlueprintPath, FunctionGuid, SourceNodeGuid, SourcePinName, 
								   TargetNodeGuid, TargetPinName);

		if (Success)
		{
			SuccessfulConnections++;
		}
		else
		{
			AllSuccessful = false;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Connected %d/%d node pairs in blueprint %s"), 
		   SuccessfulConnections, ConnectionsArray.Num(), *BlueprintPath);
	return AllSuccessful;
}

bool UGenBlueprintUtils::OpenBlueprintGraph(UBlueprint* Blueprint, UEdGraph* Graph)
{
	if (!Blueprint || !Graph || !GEditor)
		return false;
        
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
		return false;
        
	// First make sure the blueprint editor is open
	IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Blueprint, false);
	if (!EditorInstance)
	{
		EditorInstance = AssetEditorSubsystem->OpenEditorForAsset(Blueprint) ? AssetEditorSubsystem->FindEditorForAsset(Blueprint, false) : nullptr;
		if (!EditorInstance)
			return false;
	}
    
	// Try to cast to blueprint editor
	FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(EditorInstance);
	if (BlueprintEditor)
	{
		
		// Focus on the specific graph
		BlueprintEditor->OpenGraphAndBringToFront(Graph);
		return true;
	}
    
	return false;
}

FString UGenBlueprintUtils::GetNodeGUID(const FString& BlueprintPath, const FString& GraphType, const FString& NodeName, const FString& FunctionGuid)
{
    // Load the blueprint asset
    UBlueprint* Blueprint = LoadBlueprintAsset(BlueprintPath);
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
        return TEXT("");
    }

    // Determine graph type and find the target graph
    UEdGraph* TargetGraph = nullptr;

    if (GraphType.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
    {
        if (NodeName.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("NodeName is required for EventGraph"));
            return TEXT("");
        }

        // Find EventGraph (typically the first UbergraphPage)
        if (Blueprint->UbergraphPages.Num() > 0)
        {
            TargetGraph = Blueprint->UbergraphPages[0];
        }
    }
    else if (GraphType.Equals(TEXT("FunctionGraph"), ESearchCase::IgnoreCase))
    {
        if (FunctionGuid.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("FunctionGuid is required for FunctionGraph"));
            return TEXT("");
        }

        FGuid GraphGuid;
        if (!FGuid::Parse(FunctionGuid, GraphGuid))
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid FunctionGuid format: %s"), *FunctionGuid);
            return TEXT("");
        }

        // Find the function graph by GUID
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph->GraphGuid == GraphGuid)
            {
                TargetGraph = Graph;
                break;
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid GraphType: %s. Use 'EventGraph' or 'FunctionGraph'"), *GraphType);
        return TEXT("");
    }

    if (!TargetGraph)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find graph of type %s in blueprint %s"), *GraphType, *BlueprintPath);
        return TEXT("");
    }

    // Find the target node
    UK2Node* TargetNode = nullptr;

    if (GraphType.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
    {
        for (UEdGraphNode* Node : TargetGraph->Nodes)
        {
            UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
            if (EventNode && EventNode->EventReference.GetMemberName().ToString().Equals(NodeName, ESearchCase::IgnoreCase))
            {
                TargetNode = EventNode;
                break;
            }
        }
    }
    else if (GraphType.Equals(TEXT("FunctionGraph"), ESearchCase::IgnoreCase))
    {
        for (UEdGraphNode* Node : TargetGraph->Nodes)
        {
            UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node);
            if (EntryNode)
            {
                TargetNode = EntryNode;
                break;
            }
        }
    }

    if (!TargetNode)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find node %s in %s of blueprint %s"),
               *(GraphType.Equals(TEXT("EventGraph")) ? NodeName : TEXT("FunctionEntry")),
               *GraphType, *BlueprintPath);
        return TEXT("");
    }

    // Ensure the node has a valid GUID
    if (!TargetNode->NodeGuid.IsValid())
    {
        TargetNode->NodeGuid = FGuid::NewGuid();
    }

    FString NodeGUID = TargetNode->NodeGuid.ToString();
    UE_LOG(LogTemp, Log, TEXT("Found node GUID %s for %s in %s of blueprint %s"),
           *NodeGUID, *(GraphType.Equals(TEXT("EventGraph")) ? NodeName : TEXT("FunctionEntry")),
           *GraphType, *BlueprintPath);

    return NodeGUID;
}