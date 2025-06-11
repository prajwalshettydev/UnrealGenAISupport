// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#include "MCP/GenBlueprintNodeCreator.h"
#include "BlueprintEditor.h"
#include "Components/ShapeComponent.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_Event.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_InputAction.h"
#include "K2Node_Literal.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_SwitchInteger.h"
#include "K2Node_SwitchString.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealTypePrivate.h"

TMap<FString, FString> UGenBlueprintNodeCreator::NodeTypeMap;
bool IsBlueprintDirty = false;

FString UGenBlueprintNodeCreator::AddNode(const FString& BlueprintPath, const FString& FunctionGuid,
										  const FString& NodeType, float NodeX, float NodeY,
										  const FString& PropertiesJson, bool bFinalizeChanges)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
		return TEXT("");
	}

	UEdGraph* FunctionGraph = GetGraphFromFunctionId(Blueprint, FunctionGuid);
	if (!FunctionGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find function graph with GUID: %s"), *FunctionGuid);
		return TEXT("");
	}

	if (bFinalizeChanges)
		IsBlueprintDirty = false;

	UK2Node* NewNode = nullptr;
	TArray<FString> Suggestions;

	if (TryCreateKnownNodeType(FunctionGraph, NodeType, NewNode, PropertiesJson))
	{
		// Node created successfully, make sure:
		if (!NewNode)
		{
			UE_LOG(LogTemp, Error, TEXT("Node creation failed for type: %s"), *NodeType);
			return TEXT("");
		}
	}
	else
	{
		FString Result = TryCreateNodeFromLibraries(FunctionGraph, NodeType, NewNode, Suggestions);
		if (!Result.IsEmpty() && Result.StartsWith(TEXT("SUGGESTIONS:")))
		{
			return Result; // Return suggestions directly to caller
		}
		else if (!NewNode)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create node type: %s"), *NodeType);
			return TEXT("");
		}
	}

	FunctionGraph->AddNode(NewNode, false, false);
	NewNode->NodePosX = NodeX;
	NewNode->NodePosY = NodeY;
	NewNode->AllocateDefaultPins();

	if (!PropertiesJson.IsEmpty())
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(PropertiesJson);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			for (auto& Prop : JsonObject->Values)
			{
				const FString& PropName = Prop.Key;
				TSharedPtr<FJsonValue> PropValue = Prop.Value;
				UEdGraphPin* Pin = NewNode->FindPin(FName(*PropName));
				if (Pin)
				{
					if (PropValue->Type == EJson::String)
						Pin->DefaultValue = PropValue->AsString();
					else if (PropValue->Type == EJson::Number)
						Pin->DefaultValue = FString::SanitizeFloat(PropValue->AsNumber());
					else if (PropValue->Type == EJson::Boolean)
						Pin->DefaultValue = PropValue->AsBool() ? TEXT("true") : TEXT("false");
				}

				if ((NodeType.Equals(TEXT("VariableGet"), ESearchCase::IgnoreCase) ||
					 NodeType.Equals(TEXT("VariableSet"), ESearchCase::IgnoreCase)) &&
					PropName.Equals(TEXT("variable_name"), ESearchCase::IgnoreCase) && PropValue->Type == EJson::String)
				{
					FString VariableName = PropValue->AsString();
					if (!VariableName.IsEmpty())
					{
						FMemberReference VarRef;
						VarRef.SetSelfMember(FName(*VariableName));
						if (UK2Node_VariableGet* VarGet = Cast<UK2Node_VariableGet>(NewNode))
							VarGet->VariableReference = VarRef;
						else if (UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(NewNode))
							VarSet->VariableReference = VarRef;
					}
				}
			}
		}
	}

	NewNode->ReconstructNode();

	if (bFinalizeChanges)
	{
		if (GEditor)
		{
			UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
			if (AssetEditorSubsystem)
			{
				AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
				if (FBlueprintEditor* BlueprintEditor =
						static_cast<FBlueprintEditor*>(AssetEditorSubsystem->FindEditorForAsset(Blueprint, false)))
					BlueprintEditor->OpenGraphAndBringToFront(FunctionGraph);
			}
		}
		if (IsBlueprintDirty)
		{
			Blueprint->Modify();
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		}
	}

	if (!NewNode->NodeGuid.IsValid())
		NewNode->NodeGuid = FGuid::NewGuid();

	UE_LOG(LogTemp, Log, TEXT("Added node of type %s to blueprint %s with GUID %s"), *NodeType, *BlueprintPath,
		   *NewNode->NodeGuid.ToString());
	return NewNode->NodeGuid.ToString();
}

FString UGenBlueprintNodeCreator::AddNodesBulk(const FString& BlueprintPath, const FString& FunctionGuid,
											   const FString& NodesJson)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
		return TEXT("");
	}

	FGuid GraphGuid;
	if (!FGuid::Parse(FunctionGuid, GraphGuid))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid GUID format: %s"), *FunctionGuid);
		return TEXT("");
	}

	UEdGraph* FunctionGraph = nullptr;
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
		if (Graph->GraphGuid == GraphGuid)
		{
			FunctionGraph = Graph;
			break;
		}
	if (!FunctionGraph)
		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
			if (Graph->GraphGuid == GraphGuid)
			{
				FunctionGraph = Graph;
				break;
			}
	if (!FunctionGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find function graph with GUID: %s"), *FunctionGuid);
		return TEXT("");
	}

	IsBlueprintDirty = false;

	TArray<TSharedPtr<FJsonValue>> NodesArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NodesJson);
	if (!FJsonSerializer::Deserialize(Reader, NodesArray))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse nodes JSON"));
		return TEXT("");
	}

	TSharedPtr<FJsonObject> ResultsObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> ResultsArray;

	for (auto& NodeValue : NodesArray)
	{
		TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
		if (!NodeObject.IsValid())
			continue;

		FString NodeType = NodeObject->GetStringField(TEXT("node_type"));
		TArray<TSharedPtr<FJsonValue>> PositionArray = NodeObject->GetArrayField(TEXT("node_position"));
		double NodeX = PositionArray.Num() > 0 ? PositionArray[0]->AsNumber() : 0.0;
		double NodeY = PositionArray.Num() > 1 ? PositionArray[1]->AsNumber() : 0.0;

		FString PropertiesJson;
		if (NodeObject->HasField(TEXT("node_properties")))
		{
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PropertiesJson);
			FJsonSerializer::Serialize(NodeObject->GetObjectField(TEXT("node_properties")).ToSharedRef(), Writer);
		}

		FString NodeRefId;
		if (NodeObject->HasField(TEXT("id")))
			NodeRefId = NodeObject->GetStringField(TEXT("id"));

		FString NodeGuid = AddNode(BlueprintPath, FunctionGuid, NodeType, NodeX, NodeY, PropertiesJson, false);
		if (!NodeGuid.IsEmpty())
		{
			TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
			ResultObject->SetStringField(TEXT("node_guid"), NodeGuid);
			if (!NodeRefId.IsEmpty())
				ResultObject->SetStringField(TEXT("ref_id"), NodeRefId);
			ResultsArray.Add(MakeShareable(new FJsonValueObject(ResultObject)));
		}
	}

	if (ResultsArray.Num() > 0)
	{
		if (GEditor)
		{
			UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
			if (AssetEditorSubsystem)
			{
				AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
				if (FBlueprintEditor* BlueprintEditor =
						static_cast<FBlueprintEditor*>(AssetEditorSubsystem->FindEditorForAsset(Blueprint, false)))
					BlueprintEditor->OpenGraphAndBringToFront(FunctionGraph);
			}
		}

		if (IsBlueprintDirty)
		{
			Blueprint->Modify();
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		}
	}

	FString ResultsJson;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultsJson);
	FJsonSerializer::Serialize(ResultsArray, Writer);

	UE_LOG(LogTemp, Log, TEXT("Added %d nodes to blueprint %s"), ResultsArray.Num(), *BlueprintPath);
	return ResultsJson;
}

// Delete a specific node
bool UGenBlueprintNodeCreator::DeleteNode(const FString& BlueprintPath, const FString& FunctionGuid,
										  const FString& NodeGuid)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load blueprint at path: %s"), *BlueprintPath);
		return false;
	}

	UEdGraph* FunctionGraph = nullptr;

	// Handle EventGraph special case
	if (FunctionGuid.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
	{
		if (Blueprint->UbergraphPages.Num() > 0)
			FunctionGraph = Blueprint->UbergraphPages[0];
	}
	else
	{
		// Parse GUID if not EventGraph
		FGuid GraphGuidObj;
		if (!FGuid::Parse(FunctionGuid, GraphGuidObj))
		{
			UE_LOG(LogTemp, Error, TEXT("Invalid graph GUID format: %s"), *FunctionGuid);
			return false;
		}

		FunctionGraph = FindGraphByGuid(Blueprint, GraphGuidObj);
	}

	if (!FunctionGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find graph with ID: %s"), *FunctionGuid);
		return false;
	}

	// Parse the node GUID
	FGuid NodeGuidObj;
	if (!FGuid::Parse(NodeGuid, NodeGuidObj))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid node GUID format: %s"), *NodeGuid);
		return false;
	}

	// Log all nodes for debugging
	UE_LOG(LogTemp, Log, TEXT("Looking for node with GUID: %s in graph with %d nodes"), *NodeGuidObj.ToString(),
		   FunctionGraph->Nodes.Num());

	UEdGraphNode* NodeToDelete = nullptr;
	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		UE_LOG(LogTemp, Log, TEXT("  - Found node of type %s with GUID: %s"), *Node->GetClass()->GetName(),
			   *Node->NodeGuid.ToString());

		if (Node->NodeGuid == NodeGuidObj)
		{
			NodeToDelete = Node;
			UE_LOG(LogTemp, Log, TEXT("  - MATCHED!"));
			break;
		}
	}

	if (!NodeToDelete)
	{
		UE_LOG(LogTemp, Error, TEXT("No node found with GUID: %s"), *NodeGuidObj.ToString());
		return false;
	}

	// Actually remove the node and mark blueprint as modified
	FBlueprintEditorUtils::RemoveNode(Blueprint, NodeToDelete, true);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	UE_LOG(LogTemp, Log, TEXT("Successfully deleted node with GUID: %s"), *NodeGuid);
	return true;
}

// Get all nodes in a graph with their positions
FString UGenBlueprintNodeCreator::GetAllNodesInGraph(const FString& BlueprintPath, const FString& FunctionGuid)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint)
		return TEXT("");

	UEdGraph* FunctionGraph = nullptr;
	// In GetAllNodesInGraph:
	if (FunctionGuid.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
	{
		if (Blueprint->UbergraphPages.Num() > 0)
			FunctionGraph = Blueprint->UbergraphPages[0];
	}
	else
	{
		FGuid GraphGuid;
		if (!FGuid::Parse(FunctionGuid, GraphGuid))
			return TEXT("");

		FunctionGraph = FindGraphByGuid(Blueprint, GraphGuid);
	}

	if (!FunctionGraph)
		return TEXT("");

	TArray<TSharedPtr<FJsonValue>> NodesArray;
	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject);
		NodeObject->SetStringField(TEXT("node_guid"), Node->NodeGuid.ToString());
		NodeObject->SetStringField(TEXT("node_type"), Node->GetClass()->GetName());

		TArray<TSharedPtr<FJsonValue>> PositionArray;
		PositionArray.Add(MakeShareable(new FJsonValueNumber(Node->NodePosX)));
		PositionArray.Add(MakeShareable(new FJsonValueNumber(Node->NodePosY)));
		NodeObject->SetArrayField(TEXT("position"), PositionArray);

		NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObject)));
	}

	FString ResultJson;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
	FJsonSerializer::Serialize(NodesArray, Writer);

	return ResultJson;
}

UEdGraph* UGenBlueprintNodeCreator::FindGraphByGuid(UBlueprint* Blueprint, const FGuid& GraphGuid)
{
	if (!Blueprint)
		return nullptr;

	// Look in UbergraphPages
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph && Graph->GraphGuid == GraphGuid)
		{
			return Graph;
		}
	}

	// Look in FunctionGraphs
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph && Graph->GraphGuid == GraphGuid)
		{
			return Graph;
		}
	}

	// Look in MacroGraphs (if needed)
	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		if (Graph && Graph->GraphGuid == GraphGuid)
		{
			return Graph;
		}
	}

	return nullptr;
}

// Initialize the map with practical node type mappings based on actual failures
void UGenBlueprintNodeCreator::InitNodeTypeMap()
{
	if (NodeTypeMap.Num() > 0)
		return; // Already initialized

	// Return node - this was the first failure in the test
	NodeTypeMap.Add(TEXT("returnnode"), TEXT("K2Node_FunctionResult"));

	// Math operation naming issues from the test
	NodeTypeMap.Add(TEXT("floatplusfloat"), TEXT("Add_FloatFloat"));
	NodeTypeMap.Add(TEXT("floatplus"), TEXT("Add_FloatFloat"));
	NodeTypeMap.Add(TEXT("k2_addfloat"), TEXT("Add_FloatFloat"));

	// Library prefix issues
	NodeTypeMap.Add(TEXT("kismetmathlibrary.multiply_floatfloat"), TEXT("Multiply_FloatFloat"));
	NodeTypeMap.Add(TEXT("kismetmathlibrary.add_floatfloat"), TEXT("Add_FloatFloat"));

	// Common function name variations
	NodeTypeMap.Add(TEXT("getlocation"), TEXT("GetActorLocation"));
	NodeTypeMap.Add(TEXT("setlocation"), TEXT("SetActorLocation"));

	// K2 prefix issues (sometimes needed, sometimes not)
	NodeTypeMap.Add(TEXT("k2_getactorlocation"), TEXT("GetActorLocation"));
	NodeTypeMap.Add(TEXT("k2_setactorlocation"), TEXT("SetActorLocation"));

	// Extended mappings
	NodeTypeMap.Add(TEXT("functionentry"), TEXT("K2Node_FunctionEntry"));
	NodeTypeMap.Add(TEXT("gettime"), TEXT("GetTimeSeconds"));
	NodeTypeMap.Add(TEXT("gettimeseconds"), TEXT("GetTimeSeconds"));
	NodeTypeMap.Add(TEXT("sin"), TEXT("Sin"));
	NodeTypeMap.Add(TEXT("cos"), TEXT("Cos"));
	NodeTypeMap.Add(TEXT("makevector"), TEXT("MakeVector"));
	NodeTypeMap.Add(TEXT("vector"), TEXT("MakeVector"));
	NodeTypeMap.Add(TEXT("addvector"), TEXT("Add_VectorVector"));
	NodeTypeMap.Add(TEXT("add_vectorvector"), TEXT("Add_VectorVector"));

	// Event node mappings
	NodeTypeMap.Add(TEXT("getvariable"), TEXT("VariableGet"));
	NodeTypeMap.Add(TEXT("setvariable"), TEXT("VariableSet"));
	NodeTypeMap.Add(TEXT("event beginplay"), TEXT("EventBeginPlay"));
	NodeTypeMap.Add(TEXT("beginplay"), TEXT("EventBeginPlay"));
	NodeTypeMap.Add(TEXT("receivebeginplay"), TEXT("EventBeginPlay"));
	NodeTypeMap.Add(TEXT("eventtick"), TEXT("EventTick"));

	// New mappings for InputAction
	NodeTypeMap.Add(TEXT("inputaction"), TEXT("K2Node_InputAction"));
	NodeTypeMap.Add(TEXT("input"), TEXT("K2Node_InputAction"));
	NodeTypeMap.Add(TEXT("actionevent"), TEXT("K2Node_InputAction"));
	NodeTypeMap.Add(TEXT("inputevent"), TEXT("K2Node_InputAction"));
}

bool UGenBlueprintNodeCreator::TryCreateKnownNodeType(UEdGraph* Graph, const FString& NodeType, UK2Node*& OutNode,
													  const FString& PropertiesJson)
{
	InitNodeTypeMap();
	FString ActualNodeType = NodeTypeMap.FindRef(NodeType.ToLower(), NodeType);

	if (ActualNodeType.Equals(TEXT("EventBeginPlay"), ESearchCase::IgnoreCase) ||
		ActualNodeType.Equals(TEXT("BeginPlay"), ESearchCase::IgnoreCase) ||
		ActualNodeType.Equals(TEXT("ReceiveBeginPlay"), ESearchCase::IgnoreCase))
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(Graph->GetOuter());
		if (!Blueprint || Blueprint->UbergraphPages.Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("No valid Blueprint or UbergraphPages found for EventBeginPlay"));
			return false;
		}

		UEdGraph* DefaultEventGraph = Blueprint->UbergraphPages[0];
		// Ensure we're adding to the default EventGraph
		if (Graph != DefaultEventGraph)
		{
			UE_LOG(LogTemp, Warning,
				   TEXT("EventBeginPlay can only be added to the default EventGraph, not a custom function graph"));
			return false;
		}

		// Find and delete ALL existing BeginPlay nodes first
		TArray<UK2Node_Event*> NodesToDelete;
		for (UEdGraphNode* Node : DefaultEventGraph->Nodes)
		{
			if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
			{
				if (EventNode->EventReference.GetMemberName().ToString().Equals(TEXT("ReceiveBeginPlay")))
				{
					NodesToDelete.Add(EventNode);
					UE_LOG(LogTemp, Warning, TEXT("Found BeginPlay node to delete with GUID %s"),
						   *EventNode->NodeGuid.ToString());
				}
			}
		}

		// Delete all found BeginPlay nodes
		for (UK2Node_Event* NodeToDelete : NodesToDelete)
		{
			UE_LOG(LogTemp, Warning, TEXT("Deleting BeginPlay node with GUID %s"), *NodeToDelete->NodeGuid.ToString());
			FBlueprintEditorUtils::RemoveNode(Blueprint, NodeToDelete);
		}

		UK2Node_Event* NewEventNode = NewObject<UK2Node_Event>(DefaultEventGraph);
		NewEventNode->EventReference.SetExternalMember(FName(TEXT("ReceiveBeginPlay")), AActor::StaticClass());
		NewEventNode->bOverrideFunction = true;

		if (!NewEventNode)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create EventBeginPlay node using FKismetEditorUtilities"));
			return false;
		}

		OutNode = NewEventNode;
		UE_LOG(LogTemp, Log, TEXT("Created new EventBeginPlay node in default EventGraph with GUID %s"),
			   *NewEventNode->NodeGuid.ToString());

		IsBlueprintDirty = true;
		return true;
	}
	else if (ActualNodeType.Equals(TEXT("EventTick"), ESearchCase::IgnoreCase))
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(Graph->GetOuter());
		if (!Blueprint || Blueprint->UbergraphPages.Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("No valid Blueprint or UbergraphPages found for EventTick"));
			return false;
		}

		UEdGraph* DefaultEventGraph = Blueprint->UbergraphPages[0];
		if (Graph != DefaultEventGraph)
		{
			UE_LOG(LogTemp, Warning,
				   TEXT("EventTick can only be added to the default EventGraph, not a custom function graph"));
			return false;
		}

		// Find and delete ALL existing Tick nodes first
		TArray<UK2Node_Event*> NodesToDelete;
		for (UEdGraphNode* Node : DefaultEventGraph->Nodes)
		{
			if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
			{
				if (EventNode->EventReference.GetMemberName().ToString().Equals(TEXT("ReceiveTick")))
				{
					NodesToDelete.Add(EventNode);
					UE_LOG(LogTemp, Warning, TEXT("Found Tick node to delete with GUID %s"),
						   *EventNode->NodeGuid.ToString());
				}
			}
		}

		// Delete all found Tick nodes
		for (UK2Node_Event* NodeToDelete : NodesToDelete)
		{
			UE_LOG(LogTemp, Warning, TEXT("Deleting Tick node with GUID %s"), *NodeToDelete->NodeGuid.ToString());
			FBlueprintEditorUtils::RemoveNode(Blueprint, NodeToDelete);
		}

		UK2Node_Event* NewEventNode = NewObject<UK2Node_Event>(DefaultEventGraph);
		NewEventNode->EventReference.SetExternalMember(FName(TEXT("ReceiveTick")), AActor::StaticClass());
		NewEventNode->bOverrideFunction = true;

		if (!NewEventNode)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create EventTick node"));
			return false;
		}

		OutNode = NewEventNode;
		UE_LOG(LogTemp, Log, TEXT("Created new EventTick node in default EventGraph with GUID %s"),
			   *NewEventNode->NodeGuid.ToString());

		IsBlueprintDirty = true;
		return true;
	}
	auto NodeTypeLower = ActualNodeType.ToLower();
	// Special handling for function entry nodes - find existing rather than create new
	if (NodeTypeLower.Contains(TEXT("functionentry")) || NodeTypeLower.Contains(TEXT("entrynode")))
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node);
			if (EntryNode)
			{
				OutNode = EntryNode;
				return true;
			}
		}
		// Even If we don't find one, don't create a new one as it would be a duplicate
		return false;
	}

	// New handling for InputAction node
	if (ActualNodeType.Equals(TEXT("K2Node_InputAction"), ESearchCase::IgnoreCase))
	{
		UK2Node_InputAction* InputNode = NewObject<UK2Node_InputAction>(Graph);
		if (!PropertiesJson.IsEmpty())
		{
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(PropertiesJson);
			if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
			{
				FString ActionName;
				if (JsonObject->TryGetStringField(TEXT("action_name"), ActionName) && !ActionName.IsEmpty())
				{
					InputNode->InputActionName = FName(*ActionName);
					UE_LOG(LogTemp, Log, TEXT("Created InputAction node for action '%s'"), *ActionName);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("InputAction node requires 'action_name' in PropertiesJson"));
					return false;
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to parse PropertiesJson for InputAction node"));
				return false;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("InputAction node requires PropertiesJson with 'action_name'"));
			return false;
		}
		OutNode = InputNode;
		IsBlueprintDirty = true;
		return true;
	}
	if (ActualNodeType.Equals(TEXT("Branch"), ESearchCase::IgnoreCase) ||
		ActualNodeType.Equals(TEXT("IfThenElse"), ESearchCase::IgnoreCase))
	{
		OutNode = NewObject<UK2Node_IfThenElse>(Graph);
		IsBlueprintDirty = true;
		return true;
	}
	else if (ActualNodeType.Equals(TEXT("Sequence"), ESearchCase::IgnoreCase) ||
			 ActualNodeType.Equals(TEXT("ExecutionSequence"), ESearchCase::IgnoreCase))
	{
		OutNode = NewObject<UK2Node_ExecutionSequence>(Graph);
		IsBlueprintDirty = true;
		return true;
	}
	else if (ActualNodeType.Equals(TEXT("SwitchEnum"), ESearchCase::IgnoreCase))
	{
		OutNode = NewObject<UK2Node_SwitchEnum>(Graph);
		IsBlueprintDirty = true;
		return true;
	}
	else if (ActualNodeType.Equals(TEXT("SwitchInteger"), ESearchCase::IgnoreCase) ||
			 ActualNodeType.Equals(TEXT("SwitchInt"), ESearchCase::IgnoreCase))
	{
		OutNode = NewObject<UK2Node_SwitchInteger>(Graph);
		IsBlueprintDirty = true;
		return true;
	}
	else if (ActualNodeType.Equals(TEXT("SwitchString"), ESearchCase::IgnoreCase))
	{
		OutNode = NewObject<UK2Node_SwitchString>(Graph);
		IsBlueprintDirty = true;
		return true;
	}
	else if (ActualNodeType.Equals(TEXT("VariableGet"), ESearchCase::IgnoreCase) ||
			 ActualNodeType.Equals(TEXT("Getter"), ESearchCase::IgnoreCase))
	{
		UK2Node_VariableGet* VarGet = NewObject<UK2Node_VariableGet>(Graph);
		if (!PropertiesJson.IsEmpty())
		{
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(PropertiesJson);
			if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
			{
				FString VarName;
				if (JsonObject->TryGetStringField(TEXT("VariableName"), VarName) && !VarName.IsEmpty())
				{
					FMemberReference VarRef;
					VarRef.SetSelfMember(FName(*VarName));
					VarGet->VariableReference = VarRef;
					VarGet->AllocateDefaultPins(); // Ensure pins are created after setting the reference
					OutNode = VarGet;
					IsBlueprintDirty = true;
					UE_LOG(LogTemp, Log, TEXT("Created VariableGet node for variable '%s'"), *VarName);
					return true;
				}
			}
		}
		UE_LOG(LogTemp, Error, TEXT("VariableGet requires 'VariableName' in PropertiesJson"));
		return false;
	}
	else if (ActualNodeType.Equals(TEXT("VariableSet"), ESearchCase::IgnoreCase) ||
			 ActualNodeType.Equals(TEXT("Setter"), ESearchCase::IgnoreCase))
	{
		UK2Node_VariableSet* VarSet = NewObject<UK2Node_VariableSet>(Graph);
		if (!PropertiesJson.IsEmpty())
		{
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(PropertiesJson);
			if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
			{
				FString VarName;
				if (JsonObject->TryGetStringField(TEXT("VariableName"), VarName) && !VarName.IsEmpty())
				{
					FMemberReference VarRef;
					VarRef.SetSelfMember(FName(*VarName));
					VarSet->VariableReference = VarRef;
					VarSet->AllocateDefaultPins(); // Ensure pins are created after setting the reference

					// Optionally set a default value if provided
					FString Value;
					if (JsonObject->TryGetStringField(TEXT("Value"), Value))
					{
						UEdGraphPin* ValuePin = VarSet->FindPin(FName(TEXT("Value")));
						if (ValuePin)
						{
							ValuePin->DefaultValue = Value;
						}
					}

					OutNode = VarSet;
					IsBlueprintDirty = true;
					UE_LOG(LogTemp, Log, TEXT("Created VariableSet node for variable '%s'"), *VarName);
					return true;
				}
			}
		}
		UE_LOG(LogTemp, Error, TEXT("VariableSet requires 'VariableName' in PropertiesJson"));
		return false;
	}
	else if (ActualNodeType.Equals(TEXT("Multiply"), ESearchCase::IgnoreCase) ||
			 ActualNodeType.Equals(TEXT("Multiply_Float"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetMathLibrary"), TEXT("Multiply_FloatFloat"), OutNode);
	}
	else if (ActualNodeType.Equals(TEXT("Add"), ESearchCase::IgnoreCase) ||
			 ActualNodeType.Equals(TEXT("Add_Float"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetMathLibrary"), TEXT("Add_FloatFloat"), OutNode);
	}
	else if (ActualNodeType.Equals(TEXT("Subtract"), ESearchCase::IgnoreCase) ||
			 ActualNodeType.Equals(TEXT("Subtract_Float"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetMathLibrary"), TEXT("Subtract_FloatFloat"), OutNode);
	}
	else if (ActualNodeType.Equals(TEXT("Divide"), ESearchCase::IgnoreCase) ||
			 ActualNodeType.Equals(TEXT("Divide_Float"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetMathLibrary"), TEXT("Divide_FloatFloat"), OutNode);
	}
	else if (ActualNodeType.Equals(TEXT("Print"), ESearchCase::IgnoreCase) ||
			 ActualNodeType.Equals(TEXT("PrintString"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetSystemLibrary"), TEXT("PrintString"), OutNode);
	}
	else if (ActualNodeType.Equals(TEXT("Delay"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetSystemLibrary"), TEXT("Delay"), OutNode);
	}
	else if (ActualNodeType.Equals(TEXT("GetActorLocation"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("Actor"), TEXT("K2_GetActorLocation"), OutNode);
	}
	else if (ActualNodeType.Equals(TEXT("SetActorLocation"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("Actor"), TEXT("K2_SetActorLocation"), OutNode);
	}
	// Add conversion nodes
	else if (NodeType.Equals(TEXT("FloatToDouble"), ESearchCase::IgnoreCase) ||
			 NodeType.Equals(TEXT("Conv_FloatToDouble"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetMathLibrary"), TEXT("Conv_FloatToDouble"), OutNode);
	}
	else if (NodeType.Equals(TEXT("FloatToInt"), ESearchCase::IgnoreCase) ||
			 NodeType.Equals(TEXT("Conv_FloatToInteger"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetMathLibrary"), TEXT("Conv_FloatToInteger"), OutNode);
	}
	else if (NodeType.Equals(TEXT("IntToFloat"), ESearchCase::IgnoreCase) ||
			 NodeType.Equals(TEXT("Conv_IntToFloat"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetMathLibrary"), TEXT("Conv_IntToFloat"), OutNode);
	}
	else if (NodeType.Equals(TEXT("DoubleToFloat"), ESearchCase::IgnoreCase) ||
			 NodeType.Equals(TEXT("Conv_DoubleToFloat"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetMathLibrary"), TEXT("Conv_DoubleToFloat"), OutNode);
	}

	UClass* NodeClass = FindObject<UClass>(ANY_PACKAGE, *(TEXT("UK2Node_") + ActualNodeType));
	if (!NodeClass || !NodeClass->IsChildOf(UK2Node::StaticClass()))
		NodeClass = FindObject<UClass>(ANY_PACKAGE, *ActualNodeType);
	if (NodeClass && NodeClass->IsChildOf(UK2Node::StaticClass()))
	{
		OutNode = NewObject<UK2Node>(Graph, NodeClass);
		IsBlueprintDirty = true;
		return OutNode != nullptr;
	}

	return false;
}

UEdGraph* UGenBlueprintNodeCreator::GetGraphFromFunctionId(UBlueprint* Blueprint, const FString& FunctionGuid)
{
	if (FunctionGuid.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase) ||
		FunctionGuid.Equals(TEXT("Event Graph"), ESearchCase::IgnoreCase))
	{
		if (Blueprint->UbergraphPages.Num() > 0)
		{
			UEdGraph* EventGraph = Blueprint->UbergraphPages[0]; // Typically the first Ubergraph page
			UE_LOG(LogTemp, Log, TEXT("Resolved 'EventGraph' to GUID %s"), *EventGraph->GraphGuid.ToString());
			return EventGraph;
		}
		UE_LOG(LogTemp, Error, TEXT("No Event Graph found in Blueprint"));
		return nullptr;
	}

	FGuid GraphGuid;
	if (FGuid::Parse(FunctionGuid, GraphGuid))
	{
		for (UEdGraph* Graph : Blueprint->UbergraphPages)
			if (Graph->GraphGuid == GraphGuid)
				return Graph;
		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
			if (Graph->GraphGuid == GraphGuid)
				return Graph;
	}
	UE_LOG(LogTemp, Error, TEXT("Could not resolve function ID %s to a graph"), *FunctionGuid);
	return nullptr;
}

FString UGenBlueprintNodeCreator::TryCreateNodeFromLibraries(UEdGraph* Graph, const FString& NodeType,
															 UK2Node*& OutNode, TArray<FString>& OutSuggestions)
{
	static const TArray<FString> CommonLibraries = {TEXT("KismetMathLibrary"),
													TEXT("KismetSystemLibrary"),
													TEXT("KismetStringLibrary"),
													TEXT("KismetArrayLibrary"),
													TEXT("KismetTextLibrary"),
													TEXT("GameplayStatics"),
													TEXT("BlueprintFunctionLibrary"),
													TEXT("Actor"),
													TEXT("Pawn"),
													TEXT("Character")};

	struct FFunctionMatch
	{
		FString LibraryName;
		FString FunctionName;
		FString DisplayName;
		int32 Score;
		UFunction* Function;
		UClass* Class;
	};

	TArray<FFunctionMatch> Matches;
	const FString NodeTypeLower = NodeType.ToLower();

	auto SplitName = [](const FString& Name) -> TArray<FString>
	{
		TArray<FString> Parts;
		Name.ParseIntoArray(Parts, TEXT("_"));
		if (Parts.Num() == 1)
		{
			FString Temp = Name;
			Parts.Empty();
			for (int32 i = 0; i < Temp.Len(); ++i)
			{
				if (i > 0 && FChar::IsUpper(Temp[i]))
				{
					Parts.Add(Temp.Mid(0, i));
					Temp = Temp.Mid(i);
					i = 0;
				}
			}
			if (!Temp.IsEmpty())
				Parts.Add(Temp);
		}
		return Parts;
	};

	TArray<FString> NodeTypeParts = SplitName(NodeTypeLower);

	for (const FString& LibraryName : CommonLibraries)
	{
		UClass* LibClass = FindObject<UClass>(ANY_PACKAGE, *LibraryName);
		if (!LibClass)
			continue;

		for (TFieldIterator<UFunction> FuncIt(LibClass); FuncIt; ++FuncIt)
		{
			UFunction* Function = *FuncIt;
			FString FuncName = Function->GetName();
			FString FuncNameLower = FuncName.ToLower();

			if (Function->HasMetaData(TEXT("DeprecatedFunction")) ||
				Function->HasMetaData(TEXT("BlueprintInternalUseOnly")))
				continue;

			int32 Score = 0;
			if (FuncNameLower == NodeTypeLower)
				Score = 120; // Exact match
			else if (FuncNameLower.Contains(NodeTypeLower))
				Score = 80; // Substring match

			TArray<FString> FuncParts = SplitName(FuncNameLower);
			for (const FString& Part : NodeTypeParts)
				if (FuncParts.Contains(Part))
					Score += 20;
			if (FuncNameLower.StartsWith(NodeTypeLower))
				Score += 10;
			if (FuncNameLower.Len() > NodeTypeLower.Len() * 2)
				Score -= 15;

			if (Score > 0)
			{
				FFunctionMatch Match;
				Match.LibraryName = LibraryName;
				Match.FunctionName = FuncName;
				Match.DisplayName = FString::Printf(TEXT("%s.%s"), *LibraryName, *FuncName);
				Match.Score = Score;
				Match.Function = Function;
				Match.Class = LibClass;
				Matches.Add(Match);
			}
		}
	}

	if (Matches.Num() == 0)
		return TEXT("");

	Matches.Sort([](const FFunctionMatch& A, const FFunctionMatch& B) { return A.Score > B.Score; });
	for (const FFunctionMatch& Match : Matches)
		OutSuggestions.Add(Match.DisplayName);

	const int32 ScoreThreshold = 80; // Raised threshold for better precision
	if (Matches[0].Score >= ScoreThreshold)
	{
		const FFunctionMatch& BestMatch = Matches[0];
		UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
		IsBlueprintDirty = true;
		if (FunctionNode)
		{
			FunctionNode->FunctionReference.SetExternalMember(BestMatch.Function->GetFName(), BestMatch.Class);
			OutNode = FunctionNode;
			OutSuggestions.Empty(); // Clear suggestions on success
			return BestMatch.DisplayName;
		}
		return TEXT("");
	}

	while (OutSuggestions.Num() > 10)
		OutSuggestions.RemoveAt(OutSuggestions.Num() - 1); // More suggestions
	FString SuggestionStr = FString::Join(OutSuggestions, TEXT(", "));
	return TEXT("SUGGESTIONS:") + SuggestionStr;
}

bool UGenBlueprintNodeCreator::CreateMathFunctionNode(UEdGraph* Graph, const FString& ClassName,
													  const FString& FunctionName, UK2Node*& OutNode)
{
	UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
	if (FunctionNode)
	{
		UClass* Class = FindObject<UClass>(ANY_PACKAGE, *ClassName);
		if (Class)
		{
			UFunction* Function = Class->FindFunctionByName(*FunctionName);
			if (Function)
			{
				FunctionNode->FunctionReference.SetExternalMember(Function->GetFName(), Class);
				OutNode = FunctionNode;
				IsBlueprintDirty = true;
				return true;
			}
		}
	}
	return false;
}

FString UGenBlueprintNodeCreator::GetNodeSuggestions(const FString& NodeType)
{
	static const TArray<FString> CommonLibraries = {TEXT("KismetMathLibrary"),
													TEXT("KismetSystemLibrary"),
													TEXT("KismetStringLibrary"),
													TEXT("KismetArrayLibrary"),
													TEXT("KismetTextLibrary"),
													TEXT("GameplayStatics"),
													TEXT("BlueprintFunctionLibrary"),
													TEXT("Actor"),
													TEXT("Pawn"),
													TEXT("Character")};

	struct FFunctionMatch
	{
		FString LibraryName;
		FString FunctionName;
		FString DisplayName;
		int32 Score;
	};

	TArray<FFunctionMatch> Matches;
	const FString NodeTypeLower = NodeType.ToLower();

	auto SplitName = [](const FString& Name) -> TArray<FString>
	{
		TArray<FString> Parts;
		Name.ParseIntoArray(Parts, TEXT("_"));
		if (Parts.Num() == 1)
		{
			FString Temp = Name;
			Parts.Empty();
			for (int32 i = 0; i < Temp.Len(); ++i)
			{
				if (i > 0 && FChar::IsUpper(Temp[i]))
				{
					Parts.Add(Temp.Mid(0, i));
					Temp = Temp.Mid(i);
					i = 0;
				}
			}
			if (!Temp.IsEmpty())
				Parts.Add(Temp);
		}
		return Parts;
	};

	TArray<FString> NodeTypeParts = SplitName(NodeTypeLower);

	for (const FString& LibraryName : CommonLibraries)
	{
		UClass* LibClass = FindObject<UClass>(ANY_PACKAGE, *LibraryName);
		if (!LibClass)
			continue;

		for (TFieldIterator<UFunction> FuncIt(LibClass); FuncIt; ++FuncIt)
		{
			UFunction* Function = *FuncIt;
			FString FuncName = Function->GetName();
			FString FuncNameLower = FuncName.ToLower();

			if (Function->HasMetaData(TEXT("DeprecatedFunction")) ||
				Function->HasMetaData(TEXT("BlueprintInternalUseOnly")))
				continue;

			int32 Score = 0;
			if (FuncNameLower == NodeTypeLower)
				Score = 100;
			else if (FuncNameLower.Contains(NodeTypeLower))
				Score = 50;

			TArray<FString> FuncParts = SplitName(FuncNameLower);
			for (const FString& Part : NodeTypeParts)
				if (FuncParts.Contains(Part))
					Score += 10;

			if (FuncNameLower.StartsWith(NodeTypeLower))
				Score += 5;
			if (FuncNameLower.Len() > NodeTypeLower.Len() * 2)
				Score -= 10;

			if (Score > 0)
			{
				FFunctionMatch Match;
				Match.LibraryName = LibraryName;
				Match.FunctionName = FuncName;
				Match.DisplayName = FString::Printf(TEXT("%s.%s"), *LibraryName, *FuncName);
				Match.Score = Score;
				Matches.Add(Match);
			}
		}
	}

	if (Matches.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No suggestions found for node type: %s"), *NodeType);
		return TEXT("");
	}

	Matches.Sort([](const FFunctionMatch& A, const FFunctionMatch& B) { return A.Score > B.Score; });
	TArray<FString> Suggestions;
	for (const FFunctionMatch& Match : Matches)
		Suggestions.Add(Match.DisplayName);
	while (Suggestions.Num() > 5)
		Suggestions.RemoveAt(Suggestions.Num() - 1);

	FString SuggestionStr = FString::Join(Suggestions, TEXT(", "));
	UE_LOG(LogTemp, Log, TEXT("Suggestions for %s: %s"), *NodeType, *SuggestionStr);
	return TEXT("SUGGESTIONS:") + SuggestionStr;
}
