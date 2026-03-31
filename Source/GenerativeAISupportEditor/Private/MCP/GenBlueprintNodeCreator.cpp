// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#include "MCP/GenBlueprintNodeCreator.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_SwitchInteger.h"
#include "K2Node_SwitchString.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "EdGraphSchema_K2.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Blueprint.h"
#include "BlueprintEditor.h"
#include "Editor.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_InputAction.h"
#include "K2Node_InputKey.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Literal.h"
#include "K2Node_FunctionResult.h"
#include "Components/ShapeComponent.h"
#include "GameFramework/Character.h"
#include "Engine/SCS_Node.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/UnrealTypePrivate.h"


TMap<FString, FString> UGenBlueprintNodeCreator::NodeTypeMap;

// Per-call dirty flag — set by TryCreateKnownNodeType/TryCreateNodeFromLibraries,
// read by AddNode/AddNodesBulk to decide whether to mark the Blueprint as modified.
// Only accessed on the game thread (all MCP commands are queued there), so no mutex needed.
static bool bNodeCreationDirty = false;

// ---------------------------------------------------------------------------
// Unified node member property initializer (A1)
// Called AFTER node creation, BEFORE AllocateDefaultPins.
// Sets non-pin member properties (CustomFunctionName, InputKey, etc.)
// Returns true if all required properties were set successfully.
// ---------------------------------------------------------------------------
static bool ApplyNodeCreationProperties(UK2Node* Node, const TSharedPtr<FJsonObject>& Props,
                                         TArray<FString>& OutIssues)
{
	if (!Node || !Props.IsValid()) return true;

	// --- CustomEvent: function_name → CustomFunctionName ---
	if (UK2Node_CustomEvent* CE = Cast<UK2Node_CustomEvent>(Node))
	{
		FString EventName;
		if (Props->TryGetStringField(TEXT("function_name"), EventName) && !EventName.IsEmpty())
		{
			CE->CustomFunctionName = FName(*EventName);
			UE_LOG(LogTemp, Log, TEXT("ApplyNodeCreationProperties: CustomEvent name set to '%s'"), *EventName);
		}
		else
		{
			OutIssues.Add(TEXT("K2Node_CustomEvent: missing function_name property"));
		}
	}

	// --- InputKey: key_name → InputKey ---
	if (UK2Node_InputKey* IK = Cast<UK2Node_InputKey>(Node))
	{
		FString KeyName;
		if (Props->TryGetStringField(TEXT("key_name"), KeyName) && !KeyName.IsEmpty())
		{
			FKey Key(*KeyName);
			if (Key.IsValid())
			{
				IK->InputKey = Key;
				UE_LOG(LogTemp, Log, TEXT("ApplyNodeCreationProperties: InputKey set to '%s'"), *KeyName);
			}
			else
			{
				OutIssues.Add(FString::Printf(TEXT("K2Node_InputKey: invalid key '%s'"), *KeyName));
			}
		}
	}

	// --- InputAction: action_name → InputActionName (normalize) ---
	if (UK2Node_InputAction* IA = Cast<UK2Node_InputAction>(Node))
	{
		FString ActionName;
		if (Props->TryGetStringField(TEXT("action_name"), ActionName) && !ActionName.IsEmpty())
		{
			IA->InputActionName = FName(*ActionName);
		}
	}

	// Future: add EnhancedInputAction, ComponentBoundEvent, Timeline, etc.
	// Each new node type is one Cast + property assignment — no structural change.

	return OutIssues.Num() == 0;
}

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

	// Reset the creation-dirty flag for this call
	bNodeCreationDirty = false;

	// Parse PropertiesJson once — reused by resolver path, bind path, and property-set path below.
	TSharedPtr<FJsonObject> PropertiesObject;
	if (!PropertiesJson.IsEmpty())
	{
		TSharedRef<TJsonReader<>> InitReader = TJsonReaderFactory<>::Create(PropertiesJson);
		FJsonSerializer::Deserialize(InitReader, PropertiesObject);
	}

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
		if (!NewNode)
		{
			// Library search failed. Try unified resolver if function_name is provided.
			if (PropertiesObject.IsValid() && PropertiesObject->HasField(TEXT("function_name")))
			{
				FString FuncName = PropertiesObject->GetStringField(TEXT("function_name"));
				FResolvedFunction Resolved = ResolveCallableFunction(FuncName, Blueprint);

				if (Resolved.bResolved)
				{
					UK2Node_CallFunction* FuncNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
					FuncNode->FunctionReference.SetExternalMember(
						Resolved.Function->GetFName(), Resolved.OwnerClass);
					FunctionGraph->AddNode(FuncNode, false, false);
					FuncNode->NodePosX = NodeX;
					FuncNode->NodePosY = NodeY;
					FuncNode->AllocateDefaultPins();

					if (!FuncNode->GetTargetFunction() || FuncNode->Pins.Num() == 0)
					{
						UE_LOG(LogTemp, Error, TEXT("AddNode: Post-bind validation failed for '%s'. "
							"0 pins or invalid FunctionReference. Removing."), *FuncName);
						FunctionGraph->RemoveNode(FuncNode);
						return TEXT("");
					}

					NewNode = FuncNode;
					bNodeCreationDirty = true;
					UE_LOG(LogTemp, Log, TEXT("AddNode: Resolver created %s via %s"),
						*Resolved.CanonicalName, *Resolved.ContextSource);

					for (auto& Pair : PropertiesObject->Values)
					{
						if (Pair.Key == TEXT("function_name")) continue;
						UEdGraphPin* Pin = FuncNode->FindPin(FName(*Pair.Key));
						if (Pin)
						{
							FString Val;
							if (Pair.Value->TryGetString(Val))
								Pin->DefaultValue = Val;
						}
					}

					if (!FuncNode->NodeGuid.IsValid())
						FuncNode->NodeGuid = FGuid::NewGuid();

					if (bFinalizeChanges)
					{
						Blueprint->Modify();
						FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
					}

					return FuncNode->NodeGuid.ToString();
				}
			}

			// Return suggestions if library search found close matches, else error
			if (!Result.IsEmpty() && Result.StartsWith(TEXT("SUGGESTIONS:")))
				return Result;
			UE_LOG(LogTemp, Error, TEXT("Failed to create node type: %s"), *NodeType);
			return TEXT("");
		}
	}

	FunctionGraph->AddNode(NewNode, false, false);
	NewNode->NodePosX = NodeX;
	NewNode->NodePosY = NodeY;

	// Bind function_name BEFORE AllocateDefaultPins — FunctionReference must be valid first.
	if (PropertiesObject.IsValid() && PropertiesObject->HasField(TEXT("function_name")))
	{
		UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(NewNode);
		if (FuncNode && !FuncNode->FunctionReference.GetMemberName().IsValid())
		{
			FString FuncName = PropertiesObject->GetStringField(TEXT("function_name"));
			bool bBound = BindFunctionByName(FuncNode, FuncName);
			if (!bBound)
			{
				UE_LOG(LogTemp, Error, TEXT("AddNode: BindFunctionByName failed for '%s'. "
					"Node will have incorrect pins. Aborting node creation."), *FuncName);
				FunctionGraph->RemoveNode(NewNode);
				return TEXT("");
			}
		}
	}

	// Apply non-pin node member properties (CustomFunctionName, InputKey, etc.)
	// Must run BEFORE AllocateDefaultPins since some properties affect pin generation.
	{
		TArray<FString> PropIssues;
		ApplyNodeCreationProperties(Cast<UK2Node>(NewNode), PropertiesObject, PropIssues);
		for (const FString& Issue : PropIssues)
			UE_LOG(LogTemp, Warning, TEXT("AddNode property issue: %s"), *Issue);
	}

	NewNode->AllocateDefaultPins();

	// Post-create validation for special node types
	if (PropertiesObject.IsValid())
	{
		for (auto& Prop : PropertiesObject->Values)
		{
			const FString& PropName = Prop.Key;
			if (PropName == TEXT("function_name")) continue;
			TSharedPtr<FJsonValue> PropValue = Prop.Value;
			UEdGraphPin* Pin = NewNode->FindPin(FName(*PropName));
			if (Pin)
			{
				if (PropValue->Type == EJson::String)
				{
					FString StrVal = PropValue->AsString();
					// For class/object pins, resolve asset path → UObject
					FName PinCat = Pin->PinType.PinCategory;
					if ((PinCat == UEdGraphSchema_K2::PC_Class ||
						 PinCat == UEdGraphSchema_K2::PC_Object) &&
						(StrVal.Contains(TEXT("/Game/")) || StrVal.Contains(TEXT("/Script/"))))
					{
						UObject* Obj = StaticLoadObject(UObject::StaticClass(), nullptr, *StrVal);
						if (Obj)
						{
							Pin->DefaultObject = Obj;
							UE_LOG(LogTemp, Log, TEXT("AddNode: Set DefaultObject on pin '%s' to %s"),
								*PropName, *Obj->GetPathName());
						}
						else
						{
							Pin->DefaultValue = StrVal;
						}
					}
					else
					{
						Pin->DefaultValue = StrVal;
					}
				}
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

	NewNode->ReconstructNode();

	if (bFinalizeChanges)
	{
		// Mark modified BEFORE opening the editor so the UI reflects the change
		if (bNodeCreationDirty)
		{
			Blueprint->Modify();
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		}
		if (GEditor)
		{
			UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
			if (AssetEditorSubsystem)
			{
				AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
				if (FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(AssetEditorSubsystem->
					FindEditorForAsset(Blueprint, false)))
					BlueprintEditor->OpenGraphAndBringToFront(FunctionGraph);
			}
		}
	}

	if (!NewNode->NodeGuid.IsValid()) NewNode->NodeGuid = FGuid::NewGuid();

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

	// Use the same graph resolution as AddNode — handles "EventGraph" string, GUIDs, and all graph types
	UEdGraph* FunctionGraph = GetGraphFromFunctionId(Blueprint, FunctionGuid);
	if (!FunctionGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("AddNodesBulk: Could not find graph for ID: %s"), *FunctionGuid);
		return TEXT("");
	}

	TArray<TSharedPtr<FJsonValue>> NodesArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NodesJson);
	if (!FJsonSerializer::Deserialize(Reader, NodesArray))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse nodes JSON"));
		return TEXT("");
	}

	TArray<TSharedPtr<FJsonValue>> ResultsArray;
	bool bAnyDirty = false;

	for (auto& NodeValue : NodesArray)
	{
		TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
		if (!NodeObject.IsValid()) continue;

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
		if (NodeObject->HasField(TEXT("id"))) NodeRefId = NodeObject->GetStringField(TEXT("id"));

		FString NodeGuid = AddNode(BlueprintPath, FunctionGuid, NodeType, NodeX, NodeY, PropertiesJson, false);

		TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
		if (!NodeRefId.IsEmpty()) ResultObject->SetStringField(TEXT("ref_id"), NodeRefId);

		if (!NodeGuid.IsEmpty() && !NodeGuid.StartsWith(TEXT("SUGGESTIONS:")))
		{
			ResultObject->SetBoolField(TEXT("ok"), true);
			ResultObject->SetStringField(TEXT("node_guid"), NodeGuid);
			bAnyDirty = true;  // At least one node was created
		}
		else
		{
			// Report failure with context so the caller can identify which node failed
			ResultObject->SetBoolField(TEXT("ok"), false);
			ResultObject->SetStringField(TEXT("error_code"), TEXT("NODE_CREATION_FAILED"));
			FString ErrorMsg = FString::Printf(TEXT("Failed to create node type '%s'"), *NodeType);
			if (NodeGuid.StartsWith(TEXT("SUGGESTIONS:")))
			{
				ErrorMsg += TEXT(". ") + NodeGuid;
			}
			ResultObject->SetStringField(TEXT("message"), ErrorMsg);
			ResultObject->SetStringField(TEXT("node_type"), NodeType);
		}
		ResultsArray.Add(MakeShareable(new FJsonValueObject(ResultObject)));
	}

	if (bAnyDirty)
	{
		// Mark modified BEFORE opening the editor
		Blueprint->Modify();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

		if (GEditor)
		{
			UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
			if (AssetEditorSubsystem)
			{
				AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
				if (FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(AssetEditorSubsystem->
					FindEditorForAsset(Blueprint, false)))
					BlueprintEditor->OpenGraphAndBringToFront(FunctionGraph);
			}
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
	UE_LOG(LogTemp, Log, TEXT("Looking for node with GUID: %s in graph with %d nodes"),
	       *NodeGuidObj.ToString(), FunctionGraph->Nodes.Num());

	UEdGraphNode* NodeToDelete = nullptr;
	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		UE_LOG(LogTemp, Log, TEXT("  - Found node of type %s with GUID: %s"),
		       *Node->GetClass()->GetName(), *Node->NodeGuid.ToString());

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

// Move a node to a new position
bool UGenBlueprintNodeCreator::MoveNode(const FString& BlueprintPath, const FString& FunctionGuid,
                                        const FString& NodeGuid, float NewX, float NewY)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint) return false;

	UEdGraph* FunctionGraph = nullptr;
	if (FunctionGuid.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
	{
		if (Blueprint->UbergraphPages.Num() > 0)
			FunctionGraph = Blueprint->UbergraphPages[0];
	}
	else
	{
		FGuid GraphGuidObj;
		if (!FGuid::Parse(FunctionGuid, GraphGuidObj)) return false;
		FunctionGraph = FindGraphByGuid(Blueprint, GraphGuidObj);
	}
	if (!FunctionGraph) return false;

	FGuid NodeGuidObj;
	if (!FGuid::Parse(NodeGuid, NodeGuidObj)) return false;

	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (Node && Node->NodeGuid == NodeGuidObj)
		{
			Node->Modify();
			Node->NodePosX = FMath::RoundToInt(NewX);
			Node->NodePosY = FMath::RoundToInt(NewY);
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			return true;
		}
	}
	return false;
}


// Get detailed information about a specific node
// ---------------------------------------------------------------------------
// Canonical ID: locale-independent node identifier
// ---------------------------------------------------------------------------
static FString ComputeCanonicalId(UEdGraphNode* Node)
{
	if (!Node) return TEXT("");

	// CallFunction: "CallFunction:ClassName.FunctionName"
	if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(Node))
	{
		UFunction* Func = FuncNode->GetTargetFunction();
		if (Func && Func->GetOwnerClass())
		{
			return FString::Printf(TEXT("CallFunction:%s.%s"),
				*(Func->GetOwnerClass() ? Func->GetOwnerClass()->GetName() : TEXT("Unknown")), *Func->GetName());
		}
		FName MemberName = FuncNode->FunctionReference.GetMemberName();
		if (MemberName.IsValid())
		{
			return FString::Printf(TEXT("CallFunction:%s"), *MemberName.ToString());
		}
		return TEXT("CallFunction:Unbound");
	}

	// Event: "Event:ReceiveBeginPlay"
	if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
	{
		return FString::Printf(TEXT("Event:%s"),
			*EventNode->EventReference.GetMemberName().ToString());
	}

	// CustomEvent: "CustomEvent:MyEventName"
	if (UK2Node_CustomEvent* CustomNode = Cast<UK2Node_CustomEvent>(Node))
	{
		return FString::Printf(TEXT("CustomEvent:%s"),
			*CustomNode->CustomFunctionName.ToString());
	}

	// VariableGet/Set: "VariableGet:VarName" / "VariableSet:VarName"
	if (UK2Node_VariableGet* VarGet = Cast<UK2Node_VariableGet>(Node))
	{
		return FString::Printf(TEXT("VariableGet:%s"),
			*VarGet->VariableReference.GetMemberName().ToString());
	}
	if (UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(Node))
	{
		return FString::Printf(TEXT("VariableSet:%s"),
			*VarSet->VariableReference.GetMemberName().ToString());
	}

	// All other K2Node types: class name (e.g. "K2Node_IfThenElse")
	return Node->GetClass()->GetName();
}

static TArray<TSharedPtr<FJsonValue>> SerializeNodePins(UEdGraphNode* Node, bool bSemantic = false)
{
	TArray<TSharedPtr<FJsonValue>> PinsArray;
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin) continue;
		TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
		PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
		PinObj->SetStringField(
			bSemantic ? TEXT("dir") : TEXT("direction"),
			bSemantic
				? (Pin->Direction == EGPD_Input ? TEXT("in") : TEXT("out"))
				: (Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output")));
		FString PinType = Pin->PinType.PinCategory.ToString();
		if (Pin->PinType.PinSubCategoryObject.IsValid())
			PinType += TEXT(":") + Pin->PinType.PinSubCategoryObject->GetName();
		PinObj->SetStringField(bSemantic ? TEXT("ptype") : TEXT("type"), PinType);
		// Strip empty default_value
		if (!Pin->DefaultValue.IsEmpty())
			PinObj->SetStringField(bSemantic ? TEXT("val") : TEXT("default_value"), Pin->DefaultValue);
		// Only output DefaultObject/DefaultClass when present
		if (Pin->DefaultObject)
		{
			PinObj->SetStringField(bSemantic ? TEXT("ref") : TEXT("default_object"),
				Pin->DefaultObject->GetPathName());
			if (UClass* AsClass = Cast<UClass>(Pin->DefaultObject))
			{
				PinObj->SetStringField(bSemantic ? TEXT("cls") : TEXT("default_class"), AsClass->GetName());
			}
		}
		// Only output connections when pin is linked
		if (Pin->LinkedTo.Num() > 0)
		{
			TArray<TSharedPtr<FJsonValue>> ConnArray;
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (!LinkedPin || !LinkedPin->GetOwningNode()) continue;
				TSharedPtr<FJsonObject> ConnObj = MakeShareable(new FJsonObject);
				ConnObj->SetStringField(bSemantic ? TEXT("guid") : TEXT("node_guid"),
					LinkedPin->GetOwningNode()->NodeGuid.ToString());
				ConnObj->SetStringField(bSemantic ? TEXT("pin") : TEXT("pin_name"),
					LinkedPin->PinName.ToString());
				ConnArray.Add(MakeShareable(new FJsonValueObject(ConnObj)));
			}
			PinObj->SetArrayField(bSemantic ? TEXT("links") : TEXT("connected_to"), ConnArray);
		}
		PinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
	}
	return PinsArray;
}

FString UGenBlueprintNodeCreator::GetNodeDetails(const FString& BlueprintPath, const FString& FunctionGuid,
                                                  const FString& NodeGuid, const FString& SchemaMode)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint) return TEXT("{\"error\":\"Blueprint not found\"}");

	UEdGraph* FunctionGraph = GetGraphFromFunctionId(Blueprint, FunctionGuid);
	if (!FunctionGraph) return TEXT("{\"error\":\"Graph not found\"}");

	FGuid NodeGuidObj;
	if (!FGuid::Parse(NodeGuid, NodeGuidObj)) return TEXT("{\"error\":\"Invalid node GUID\"}");

	UEdGraphNode* TargetNode = nullptr;
	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (Node && Node->NodeGuid == NodeGuidObj)
		{
			TargetNode = Node;
			break;
		}
	}
	if (!TargetNode) return TEXT("{\"error\":\"Node not found\"}");

	bool bSemantic = SchemaMode.Equals(TEXT("semantic"), ESearchCase::IgnoreCase);

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetStringField(bSemantic ? TEXT("guid") : TEXT("node_guid"), TargetNode->NodeGuid.ToString());
	Result->SetStringField(bSemantic ? TEXT("type") : TEXT("node_type"), TargetNode->GetClass()->GetName());
	Result->SetStringField(bSemantic ? TEXT("title") : TEXT("node_title"),
		TargetNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
	Result->SetStringField(bSemantic ? TEXT("cid") : TEXT("canonical_id"), ComputeCanonicalId(TargetNode));

	TArray<TSharedPtr<FJsonValue>> PosArray;
	PosArray.Add(MakeShareable(new FJsonValueNumber(TargetNode->NodePosX)));
	PosArray.Add(MakeShareable(new FJsonValueNumber(TargetNode->NodePosY)));
	Result->SetArrayField(bSemantic ? TEXT("pos") : TEXT("position"), PosArray);

	Result->SetArrayField(TEXT("pins"), SerializeNodePins(TargetNode, bSemantic));

	FString ResultJson;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return ResultJson;
}


// Get ALL nodes with full details in a single call (avoids N+1 round-trips)
FString UGenBlueprintNodeCreator::GetAllNodesWithDetails(const FString& BlueprintPath, const FString& FunctionGuid)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint) return TEXT("{\"error\":\"Blueprint not found\"}");

	UEdGraph* FunctionGraph = GetGraphFromFunctionId(Blueprint, FunctionGuid);
	if (!FunctionGraph) return TEXT("{\"error\":\"Graph not found\"}");

	TArray<TSharedPtr<FJsonValue>> NodesArray;
	TMap<FString, int32> CanonicalIdCount;  // For generating unique instance_id
	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (!Node) continue;

		FString CanonicalId = ComputeCanonicalId(Node);
		int32& Count = CanonicalIdCount.FindOrAdd(CanonicalId, 0);
		FString InstanceId = FString::Printf(TEXT("%s#%d"), *CanonicalId, Count);
		Count++;

		TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject);
		NodeObj->SetStringField(TEXT("node_guid"), Node->NodeGuid.ToString());
		NodeObj->SetStringField(TEXT("node_type"), Node->GetClass()->GetName());
		NodeObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
		NodeObj->SetStringField(TEXT("canonical_id"), CanonicalId);
		NodeObj->SetStringField(TEXT("instance_id"), InstanceId);

		TArray<TSharedPtr<FJsonValue>> PosArray;
		PosArray.Add(MakeShareable(new FJsonValueNumber(Node->NodePosX)));
		PosArray.Add(MakeShareable(new FJsonValueNumber(Node->NodePosY)));
		NodeObj->SetArrayField(TEXT("position"), PosArray);

		NodeObj->SetArrayField(TEXT("pins"), SerializeNodePins(Node));

		NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObj)));
	}

	FString ResultJson;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
	FJsonSerializer::Serialize(NodesArray, Writer);
	return ResultJson;
}


// List all graphs in a blueprint
FString UGenBlueprintNodeCreator::ListGraphs(const FString& BlueprintPath)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint) return TEXT("[]");

	TArray<TSharedPtr<FJsonValue>> GraphsArray;

	auto AddGraphs = [&](const TArray<UEdGraph*>& Graphs, const FString& GraphType)
	{
		for (UEdGraph* Graph : Graphs)
		{
			if (!Graph) continue;
			TSharedPtr<FJsonObject> Obj = MakeShareable(new FJsonObject);
			Obj->SetStringField(TEXT("graph_guid"), Graph->GraphGuid.ToString());
			Obj->SetStringField(TEXT("graph_name"), Graph->GetName());
			Obj->SetStringField(TEXT("graph_type"), GraphType);
			Obj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
			GraphsArray.Add(MakeShareable(new FJsonValueObject(Obj)));
		}
	};

	AddGraphs(Blueprint->UbergraphPages, TEXT("EventGraph"));
	AddGraphs(Blueprint->FunctionGraphs, TEXT("Function"));
	AddGraphs(Blueprint->MacroGraphs, TEXT("Macro"));

	FString ResultJson;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
	FJsonSerializer::Serialize(GraphsArray, Writer);
	return ResultJson;
}


// Set a pin's default value on an existing node
bool UGenBlueprintNodeCreator::SetNodePinValue(const FString& BlueprintPath, const FString& FunctionGuid,
                                                const FString& NodeGuid, const FString& PinName,
                                                const FString& NewValue)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint) return false;

	UEdGraph* FunctionGraph = GetGraphFromFunctionId(Blueprint, FunctionGuid);
	if (!FunctionGraph) return false;

	FGuid NodeGuidObj;
	if (!FGuid::Parse(NodeGuid, NodeGuidObj)) return false;

	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (Node && Node->NodeGuid == NodeGuidObj)
		{
			UEdGraphPin* Pin = Node->FindPin(FName(*PinName));
			if (!Pin)
			{
				UE_LOG(LogTemp, Error, TEXT("SetNodePinValue: Pin '%s' not found. Available pins:"), *PinName);
				for (UEdGraphPin* P : Node->Pins)
				{
					if (P) UE_LOG(LogTemp, Error, TEXT("  - %s (%s)"), *P->PinName.ToString(),
					              P->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
				}
				return false;
			}

			// For class/object pins, resolve asset path → UObject and set DefaultObject
			FName PinCat = Pin->PinType.PinCategory;
			if ((PinCat == UEdGraphSchema_K2::PC_Class ||
				 PinCat == UEdGraphSchema_K2::PC_SoftClass ||
				 PinCat == UEdGraphSchema_K2::PC_Object ||
				 PinCat == UEdGraphSchema_K2::PC_SoftObject) &&
				(NewValue.Contains(TEXT("/Game/")) || NewValue.Contains(TEXT("/Script/"))))
			{
				UObject* Obj = StaticLoadObject(UObject::StaticClass(), nullptr, *NewValue);
				if (Obj)
				{
					Pin->DefaultObject = Obj;
					Node->PinDefaultValueChanged(Pin);
					Node->ReconstructNode();
					FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
					UE_LOG(LogTemp, Log, TEXT("SetNodePinValue: Set DefaultObject on '%s' to %s"),
						*PinName, *Obj->GetPathName());
					return true;
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("SetNodePinValue: Could not load object '%s' for pin '%s'"),
						*NewValue, *PinName);
				}
			}

			// Fallback: string-based setting for literal pins
			const UEdGraphSchema* Schema = FunctionGraph->GetSchema();
			if (Schema)
			{
				Schema->TrySetDefaultValue(*Pin, NewValue);
			}
			else
			{
				Pin->DefaultValue = NewValue;
			}

			Node->PinDefaultValueChanged(Pin);
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			return true;
		}
	}
	return false;
}


// Break a specific connection between two node pins
bool UGenBlueprintNodeCreator::DisconnectNodes(const FString& BlueprintPath, const FString& FunctionGuid,
                                                const FString& SourceNodeGuid, const FString& SourcePinName,
                                                const FString& TargetNodeGuid, const FString& TargetPinName)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint) return false;

	UEdGraph* FunctionGraph = GetGraphFromFunctionId(Blueprint, FunctionGuid);
	if (!FunctionGraph) return false;

	FGuid SrcGuid, TgtGuid;
	if (!FGuid::Parse(SourceNodeGuid, SrcGuid) || !FGuid::Parse(TargetNodeGuid, TgtGuid)) return false;

	UEdGraphNode* SrcNode = nullptr;
	UEdGraphNode* TgtNode = nullptr;
	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (Node && Node->NodeGuid == SrcGuid) SrcNode = Node;
		if (Node && Node->NodeGuid == TgtGuid) TgtNode = Node;
		if (SrcNode && TgtNode) break;
	}
	if (!SrcNode || !TgtNode) return false;

	UEdGraphPin* SrcPin = SrcNode->FindPin(FName(*SourcePinName));
	UEdGraphPin* TgtPin = TgtNode->FindPin(FName(*TargetPinName));
	if (!SrcPin || !TgtPin) return false;

	if (!SrcPin->LinkedTo.Contains(TgtPin))
	{
		UE_LOG(LogTemp, Warning, TEXT("DisconnectNodes: Pins are not connected"));
		return false;
	}

	SrcPin->BreakLinkTo(TgtPin);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return true;
}


// Get all nodes in a graph with their positions
FString UGenBlueprintNodeCreator::GetAllNodesInGraph(const FString& BlueprintPath, const FString& FunctionGuid)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint) return TEXT("");

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
		if (!FGuid::Parse(FunctionGuid, GraphGuid)) return TEXT("");

		FunctionGraph = FindGraphByGuid(Blueprint, GraphGuid);
	}

	if (!FunctionGraph) return TEXT("");

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
	if (!Blueprint) return nullptr;

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

	// CustomEvent aliases
	NodeTypeMap.Add(TEXT("customevent"), TEXT("K2Node_CustomEvent"));
	NodeTypeMap.Add(TEXT("custom_event"), TEXT("K2Node_CustomEvent"));

	// InputKey aliases
	NodeTypeMap.Add(TEXT("inputkey"), TEXT("K2Node_InputKey"));
	NodeTypeMap.Add(TEXT("keyboardevent"), TEXT("K2Node_InputKey"));
	NodeTypeMap.Add(TEXT("keyboard"), TEXT("K2Node_InputKey"));
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
			UE_LOG(LogTemp, Warning, TEXT("Deleting BeginPlay node with GUID %s"),
			       *NodeToDelete->NodeGuid.ToString());
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

		bNodeCreationDirty = true;
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
			UE_LOG(LogTemp, Warning, TEXT("Deleting Tick node with GUID %s"),
			       *NodeToDelete->NodeGuid.ToString());
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

		bNodeCreationDirty = true;
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
		bNodeCreationDirty = true;
		return true;
	}

	// CustomEvent: created here, CustomFunctionName set by ApplyNodeCreationProperties
	if (ActualNodeType.Equals(TEXT("K2Node_CustomEvent"), ESearchCase::IgnoreCase)
		|| ActualNodeType.Equals(TEXT("CustomEvent"), ESearchCase::IgnoreCase))
	{
		OutNode = NewObject<UK2Node_CustomEvent>(Graph);
		bNodeCreationDirty = true;
		return true;
	}

	// InputKey: created here, InputKey FKey set by ApplyNodeCreationProperties
	if (ActualNodeType.Equals(TEXT("K2Node_InputKey"), ESearchCase::IgnoreCase)
		|| ActualNodeType.Equals(TEXT("InputKey"), ESearchCase::IgnoreCase))
	{
		OutNode = NewObject<UK2Node_InputKey>(Graph);
		bNodeCreationDirty = true;
		return true;
	}

	if (ActualNodeType.Equals(TEXT("Branch"), ESearchCase::IgnoreCase) || ActualNodeType.Equals(
		TEXT("IfThenElse"), ESearchCase::IgnoreCase))
	{
		OutNode = NewObject<UK2Node_IfThenElse>(Graph);
		bNodeCreationDirty = true;
		return true;
	}
	else if (ActualNodeType.Equals(TEXT("Sequence"), ESearchCase::IgnoreCase) || ActualNodeType.Equals(
		TEXT("ExecutionSequence"), ESearchCase::IgnoreCase))
	{
		OutNode = NewObject<UK2Node_ExecutionSequence>(Graph);
		bNodeCreationDirty = true;
		return true;
	}
	else if (ActualNodeType.Equals(TEXT("SwitchEnum"), ESearchCase::IgnoreCase))
	{
		OutNode = NewObject<UK2Node_SwitchEnum>(Graph);
		bNodeCreationDirty = true;
		return true;
	}
	else if (ActualNodeType.Equals(TEXT("SwitchInteger"), ESearchCase::IgnoreCase) || ActualNodeType.Equals(
		TEXT("SwitchInt"), ESearchCase::IgnoreCase))
	{
		OutNode = NewObject<UK2Node_SwitchInteger>(Graph);
		bNodeCreationDirty = true;
		return true;
	}
	else if (ActualNodeType.Equals(TEXT("SwitchString"), ESearchCase::IgnoreCase))
	{
		OutNode = NewObject<UK2Node_SwitchString>(Graph);
		bNodeCreationDirty = true;
		return true;
	}
	else if (ActualNodeType.Equals(TEXT("VariableGet"), ESearchCase::IgnoreCase) || ActualNodeType.Equals(
		TEXT("Getter"), ESearchCase::IgnoreCase))
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
					bNodeCreationDirty = true;
					UE_LOG(LogTemp, Log, TEXT("Created VariableGet node for variable '%s'"), *VarName);
					return true;
				}
			}
		}
		UE_LOG(LogTemp, Error, TEXT("VariableGet requires 'VariableName' in PropertiesJson"));
		return false;
	}
	else if (ActualNodeType.Equals(TEXT("VariableSet"), ESearchCase::IgnoreCase) || ActualNodeType.Equals(
		TEXT("Setter"), ESearchCase::IgnoreCase))
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
					bNodeCreationDirty = true;
					UE_LOG(LogTemp, Log, TEXT("Created VariableSet node for variable '%s'"), *VarName);
					return true;
				}
			}
		}
		UE_LOG(LogTemp, Error, TEXT("VariableSet requires 'VariableName' in PropertiesJson"));
		return false;
	}
	else if (ActualNodeType.Equals(TEXT("Multiply"), ESearchCase::IgnoreCase) || ActualNodeType.Equals(
		TEXT("Multiply_Float"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetMathLibrary"), TEXT("Multiply_FloatFloat"), OutNode);
	}
	else if (ActualNodeType.Equals(TEXT("Add"), ESearchCase::IgnoreCase) || ActualNodeType.Equals(
		TEXT("Add_Float"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetMathLibrary"), TEXT("Add_FloatFloat"), OutNode);
	}
	else if (ActualNodeType.Equals(TEXT("Subtract"), ESearchCase::IgnoreCase) || ActualNodeType.Equals(
		TEXT("Subtract_Float"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetMathLibrary"), TEXT("Subtract_FloatFloat"), OutNode);
	}
	else if (ActualNodeType.Equals(TEXT("Divide"), ESearchCase::IgnoreCase) || ActualNodeType.Equals(
		TEXT("Divide_Float"), ESearchCase::IgnoreCase))
	{
		return CreateMathFunctionNode(Graph, TEXT("KismetMathLibrary"), TEXT("Divide_FloatFloat"), OutNode);
	}
	else if (ActualNodeType.Equals(TEXT("Print"), ESearchCase::IgnoreCase) || ActualNodeType.Equals(
		TEXT("PrintString"), ESearchCase::IgnoreCase))
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

	UClass* NodeClass = FindFirstObject<UClass>( *(TEXT("UK2Node_") + ActualNodeType));
	if (!NodeClass || !NodeClass->IsChildOf(UK2Node::StaticClass()))
		NodeClass = FindFirstObject<UClass>( *ActualNodeType);
	if (NodeClass && NodeClass->IsChildOf(UK2Node::StaticClass()))
	{
		OutNode = NewObject<UK2Node>(Graph, NodeClass);
		bNodeCreationDirty = true;
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
			if (Graph->GraphGuid == GraphGuid) return Graph;
		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
			if (Graph->GraphGuid == GraphGuid) return Graph;
	}
	UE_LOG(LogTemp, Error, TEXT("Could not resolve function ID %s to a graph"), *FunctionGuid);
	return nullptr;
}

// ---------------------------------------------------------------------------
// Shared matching infrastructure
// ---------------------------------------------------------------------------

const TArray<FString>& UGenBlueprintNodeCreator::GetCommonLibraries()
{
	static const TArray<FString> Libs = {
		TEXT("KismetMathLibrary"), TEXT("KismetSystemLibrary"), TEXT("KismetStringLibrary"),
		TEXT("KismetArrayLibrary"), TEXT("KismetTextLibrary"), TEXT("GameplayStatics"),
		TEXT("BlueprintFunctionLibrary"), TEXT("Actor"), TEXT("Pawn"), TEXT("Character")
	};
	return Libs;
}

TArray<FString> UGenBlueprintNodeCreator::SplitCamelCase(const FString& Name)
{
	TArray<FString> Parts;
	// Try underscore split first
	Name.ParseIntoArray(Parts, TEXT("_"));
	if (Parts.Num() > 1) return Parts;

	// CamelCase split
	Parts.Empty();
	FString Current;
	for (int32 i = 0; i < Name.Len(); ++i)
	{
		if (i > 0 && FChar::IsUpper(Name[i]) && !Current.IsEmpty())
		{
			Parts.Add(Current.ToLower());
			Current.Empty();
		}
		Current.AppendChar(Name[i]);
	}
	if (!Current.IsEmpty()) Parts.Add(Current.ToLower());
	return Parts;
}

UGenBlueprintNodeCreator::FMatchResult UGenBlueprintNodeCreator::ScoreFunctionMatch(
	const FString& Query, const FString& Candidate)
{
	FMatchResult Result;
	const FString QueryLower = Query.ToLower();
	const FString CandLower = Candidate.ToLower();
	const int32 QueryLen = QueryLower.Len();

	// Tier 1: Exact match (case-insensitive)
	if (CandLower == QueryLower)
	{
		Result.Type = EMatchType::Exact;
		Result.Score = 200;
		Result.bAccepted = true;
		return Result;
	}

	// Tier 2: Normalized token match — split both and compare token sets
	{
		TArray<FString> QueryTokens = SplitCamelCase(QueryLower);
		TArray<FString> CandTokens = SplitCamelCase(CandLower);
		if (QueryTokens.Num() > 0 && CandTokens.Num() > 0)
		{
			// Check if all query tokens appear in candidate tokens (order-independent)
			bool bAllMatch = true;
			for (const FString& QT : QueryTokens)
			{
				if (!CandTokens.Contains(QT)) { bAllMatch = false; break; }
			}
			if (bAllMatch && QueryTokens.Num() == CandTokens.Num())
			{
				Result.Type = EMatchType::NormalizedToken;
				Result.Score = 180;
				Result.bAccepted = true;
				return Result;
			}
		}
	}

	// Tier 3: Prefix match — candidate starts with query
	if (CandLower.StartsWith(QueryLower))
	{
		Result.Type = EMatchType::Prefix;
		Result.Score = 150;
		Result.bAccepted = true;
		return Result;
	}

	// Tier 4: Word-boundary substring — query appears at a CamelCase word boundary
	if (QueryLen >= 6 && CandLower.Contains(QueryLower))
	{
		// Check if match is at a word boundary
		int32 Pos = CandLower.Find(QueryLower);
		bool bAtBoundary = (Pos == 0);
		if (!bAtBoundary && Pos > 0)
		{
			// Check if char before match is lowercase and char at match is uppercase in original
			bAtBoundary = FChar::IsUpper(Candidate[Pos]);
		}
		if (bAtBoundary)
		{
			Result.Type = EMatchType::WordBoundary;
			Result.Score = 120;
			Result.bAccepted = true;
			return Result;
		}
	}

	// Tier 5: Plain substring — only auto-bind if query is long enough
	if (QueryLen >= 4 && CandLower.Contains(QueryLower))
	{
		Result.Type = EMatchType::Substring;
		Result.Score = 80;
		// Only auto-bind for longer queries to avoid false positives
		Result.bAccepted = (QueryLen >= 8);
		// Penalize if candidate is much longer than query
		if (CandLower.Len() > QueryLen * 3)
			Result.Score -= 20;
		return Result;
	}

	// Tier 6: Token overlap (for suggestions, not auto-bind)
	{
		TArray<FString> QueryTokens = SplitCamelCase(QueryLower);
		TArray<FString> CandTokens = SplitCamelCase(CandLower);
		int32 Overlap = 0;
		for (const FString& QT : QueryTokens)
		{
			if (CandTokens.Contains(QT)) Overlap++;
		}
		if (Overlap > 0 && QueryTokens.Num() > 0)
		{
			Result.Score = Overlap * 20;
			Result.bAccepted = false;  // Suggestion only
			return Result;
		}
	}

	return Result;  // No match
}

// ---------------------------------------------------------------------------

bool UGenBlueprintNodeCreator::BindFunctionByName(UK2Node_CallFunction* FuncNode, const FString& FunctionName)
{
	// Delegate to the unified resolver — avoids duplicating the multi-scope search logic here.
	UBlueprint* BP = FuncNode ? FuncNode->GetBlueprint() : nullptr;
	FResolvedFunction Resolved = ResolveCallableFunction(FunctionName, BP);
	if (Resolved.bResolved)
	{
		FuncNode->FunctionReference.SetExternalMember(Resolved.Function->GetFName(), Resolved.OwnerClass);
		UE_LOG(LogTemp, Log, TEXT("BindFunctionByName: bound '%s' → %s via %s"),
			*FunctionName, *Resolved.CanonicalName, *Resolved.ContextSource);
		return true;
	}
	UE_LOG(LogTemp, Warning, TEXT("BindFunctionByName: could not resolve '%s'"), *FunctionName);
	return false;
}

FString UGenBlueprintNodeCreator::TryCreateNodeFromLibraries(UEdGraph* Graph, const FString& NodeType,
                                                             UK2Node*& OutNode,
                                                             TArray<FString>& OutSuggestions)
{
	struct FFunctionMatch
	{
		FString DisplayName;
		FMatchResult Match;
		UFunction* Function;
		UClass* Class;
	};

	TArray<FFunctionMatch> Matches;

	for (const FString& LibraryName : GetCommonLibraries())
	{
		UClass* LibClass = FindFirstObject<UClass>(*LibraryName);
		if (!LibClass) continue;

		for (TFieldIterator<UFunction> FuncIt(LibClass); FuncIt; ++FuncIt)
		{
			UFunction* Function = *FuncIt;
			if (Function->HasMetaData(TEXT("DeprecatedFunction")) || Function->HasMetaData(
				TEXT("BlueprintInternalUseOnly")))
				continue;

			FMatchResult M = ScoreFunctionMatch(NodeType, Function->GetName());
			if (M.Score > 0)
			{
				FFunctionMatch FM;
				FM.DisplayName = FString::Printf(TEXT("%s.%s"), *LibraryName, *Function->GetName());
				FM.Match = M;
				FM.Function = Function;
				FM.Class = LibClass;
				Matches.Add(FM);
			}
		}
	}

	if (Matches.Num() == 0) return TEXT("");

	Matches.Sort([](const FFunctionMatch& A, const FFunctionMatch& B) { return A.Match.Score > B.Match.Score; });
	for (const FFunctionMatch& FM : Matches) OutSuggestions.Add(FM.DisplayName);

	if (Matches[0].Match.bAccepted)
	{
		const FFunctionMatch& Best = Matches[0];
		UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
		bNodeCreationDirty = true;
		if (FunctionNode)
		{
			FunctionNode->FunctionReference.SetExternalMember(Best.Function->GetFName(), Best.Class);
			OutNode = FunctionNode;
			OutSuggestions.Empty();
			return Best.DisplayName;
		}
		return TEXT("");
	}

	while (OutSuggestions.Num() > 10) OutSuggestions.RemoveAt(OutSuggestions.Num() - 1);
	FString SuggestionStr = FString::Join(OutSuggestions, TEXT(", "));
	return TEXT("SUGGESTIONS:") + SuggestionStr;
}

bool UGenBlueprintNodeCreator::CreateMathFunctionNode(UEdGraph* Graph, const FString& ClassName,
                                                      const FString& FunctionName,
                                                      UK2Node*& OutNode)
{
	UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
	if (FunctionNode)
	{
		UClass* Class = FindFirstObject<UClass>( *ClassName);
		if (Class)
		{
			UFunction* Function = Class->FindFunctionByName(*FunctionName);
			if (Function)
			{
				FunctionNode->FunctionReference.SetExternalMember(Function->GetFName(), Class);
				OutNode = FunctionNode;
				bNodeCreationDirty = true;
				return true;
			}
		}
	}
	return false;
}

FString UGenBlueprintNodeCreator::GetNodeSuggestions(const FString& NodeType)
{
	struct FSuggestion { FString DisplayName; int32 Score; };
	TArray<FSuggestion> Suggestions;

	for (const FString& LibraryName : GetCommonLibraries())
	{
		UClass* LibClass = FindFirstObject<UClass>(*LibraryName);
		if (!LibClass) continue;

		for (TFieldIterator<UFunction> FuncIt(LibClass); FuncIt; ++FuncIt)
		{
			UFunction* Function = *FuncIt;
			if (Function->HasMetaData(TEXT("DeprecatedFunction")) || Function->HasMetaData(
				TEXT("BlueprintInternalUseOnly")))
				continue;

			FMatchResult M = ScoreFunctionMatch(NodeType, Function->GetName());
			if (M.Score > 0)
			{
				FSuggestion S;
				S.DisplayName = FString::Printf(TEXT("%s.%s"), *LibraryName, *Function->GetName());
				S.Score = M.Score;
				Suggestions.Add(S);
			}
		}
	}

	if (Suggestions.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No suggestions found for node type: %s"), *NodeType);
		return TEXT("");
	}

	Suggestions.Sort([](const FSuggestion& A, const FSuggestion& B) { return A.Score > B.Score; });
	TArray<FString> Names;
	for (const FSuggestion& S : Suggestions) Names.Add(S.DisplayName);
	while (Names.Num() > 5) Names.RemoveAt(Names.Num() - 1);

	FString SuggestionStr = FString::Join(Names, TEXT(", "));
	UE_LOG(LogTemp, Log, TEXT("Suggestions for %s: %s"), *NodeType, *SuggestionStr);
	return TEXT("SUGGESTIONS:") + SuggestionStr;
}

// ---------------------------------------------------------------------------
// Pin type string helper — converts FProperty to human-readable type
// ---------------------------------------------------------------------------
static FString GetPinTypeString(FProperty* Prop)
{
	if (!Prop) return TEXT("unknown");

	if (Prop->IsA<FBoolProperty>()) return TEXT("bool");
	if (Prop->IsA<FIntProperty>() || Prop->IsA<FInt64Property>()) return TEXT("int32");
	if (Prop->IsA<FFloatProperty>()) return TEXT("float");
	if (Prop->IsA<FDoubleProperty>()) return TEXT("double");
	if (Prop->IsA<FStrProperty>()) return TEXT("FString");
	if (Prop->IsA<FNameProperty>()) return TEXT("FName");
	if (Prop->IsA<FTextProperty>()) return TEXT("FText");
	if (Prop->IsA<FByteProperty>()) return TEXT("byte");

	if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
	{
		if (ObjProp->PropertyClass)
			return ObjProp->PropertyClass->GetName() + TEXT("*");
		return TEXT("UObject*");
	}
	if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(Prop))
	{
		if (SoftProp->PropertyClass)
			return TEXT("TSoftObjectPtr<") + SoftProp->PropertyClass->GetName() + TEXT(">");
		return TEXT("TSoftObjectPtr");
	}
	if (FClassProperty* ClassProp = CastField<FClassProperty>(Prop))
	{
		if (ClassProp->MetaClass)
			return TEXT("TSubclassOf<") + ClassProp->MetaClass->GetName() + TEXT(">");
		return TEXT("UClass*");
	}
	if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
	{
		if (StructProp->Struct)
			return StructProp->Struct->GetName();
		return TEXT("struct");
	}
	if (Prop->IsA<FEnumProperty>() || Prop->IsA<FByteProperty>())
	{
		FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop);
		if (EnumProp && EnumProp->GetEnum())
			return EnumProp->GetEnum()->GetName();
		return TEXT("enum");
	}
	if (Prop->IsA<FArrayProperty>()) return TEXT("TArray");
	if (Prop->IsA<FSetProperty>()) return TEXT("TSet");
	if (Prop->IsA<FMapProperty>()) return TEXT("TMap");
	if (Prop->IsA<FDelegateProperty>()) return TEXT("delegate");
	if (Prop->IsA<FMulticastDelegateProperty>()) return TEXT("multicast_delegate");
	if (Prop->IsA<FInterfaceProperty>()) return TEXT("interface");

	return Prop->GetCPPType();
}

// ---------------------------------------------------------------------------
// Extract pin schema from a UFunction WITHOUT instantiating a node
// ---------------------------------------------------------------------------
static TArray<TSharedPtr<FJsonValue>> ExtractPinsFromFunction(UFunction* Func)
{
	TArray<TSharedPtr<FJsonValue>> PinsArray;

	if (!Func) return PinsArray;

	// Non-pure functions have exec input/output
	if (!Func->HasAnyFunctionFlags(FUNC_BlueprintPure))
	{
		TSharedPtr<FJsonObject> ExecIn = MakeShareable(new FJsonObject);
		ExecIn->SetStringField(TEXT("name"), TEXT("execute"));
		ExecIn->SetStringField(TEXT("direction"), TEXT("input"));
		ExecIn->SetStringField(TEXT("type"), TEXT("exec"));
		ExecIn->SetBoolField(TEXT("required"), true);
		PinsArray.Add(MakeShareable(new FJsonValueObject(ExecIn)));

		TSharedPtr<FJsonObject> ExecOut = MakeShareable(new FJsonObject);
		ExecOut->SetStringField(TEXT("name"), TEXT("then"));
		ExecOut->SetStringField(TEXT("direction"), TEXT("output"));
		ExecOut->SetStringField(TEXT("type"), TEXT("exec"));
		// required=false → don't output (strip empty)
		PinsArray.Add(MakeShareable(new FJsonValueObject(ExecOut)));
	}

	// Iterate function parameters
	for (TFieldIterator<FProperty> It(Func); It; ++It)
	{
		FProperty* Prop = *It;

		// Skip WorldContext params (hidden in Blueprint)
		if (Prop->HasMetaData(TEXT("WorldContext")))
			continue;

		TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);

		bool bIsReturn = Prop->HasAnyPropertyFlags(CPF_ReturnParm);
		bool bIsOutput = Prop->HasAnyPropertyFlags(CPF_OutParm) || bIsReturn;

		PinObj->SetStringField(TEXT("name"),
			bIsReturn ? TEXT("ReturnValue") : FString(Prop->GetName()));
		PinObj->SetStringField(TEXT("direction"),
			bIsOutput ? TEXT("output") : TEXT("input"));
		PinObj->SetStringField(TEXT("type"), GetPinTypeString(Prop));
		// Only output required when true (strip false)
		if (!bIsReturn && !bIsOutput)
			PinObj->SetBoolField(TEXT("required"), true);

		// Default value from metadata (strip empty)
		if (Prop->HasMetaData(TEXT("DefaultValue")))
		{
			FString DefVal = Prop->GetMetaData(TEXT("DefaultValue"));
			if (!DefVal.IsEmpty())
				PinObj->SetStringField(TEXT("default"), DefVal);
		}

		PinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
	}

	return PinsArray;
}

// ---------------------------------------------------------------------------
// Static pin table for known special node types (no UFunction available)
// ---------------------------------------------------------------------------
static TArray<TSharedPtr<FJsonValue>> GetKnownTypePins(const FString& NodeType)
{
	TArray<TSharedPtr<FJsonValue>> Pins;

	auto MakePin = [](const FString& Name, const FString& Dir, const FString& Type, bool bRequired = true) -> TSharedPtr<FJsonValue>
	{
		TSharedPtr<FJsonObject> Pin = MakeShareable(new FJsonObject);
		Pin->SetStringField(TEXT("name"), Name);
		Pin->SetStringField(TEXT("direction"), Dir);
		Pin->SetStringField(TEXT("type"), Type);
		Pin->SetBoolField(TEXT("required"), bRequired);
		return MakeShareable(new FJsonValueObject(Pin));
	};

	FString Lower = NodeType.ToLower();

	if (Lower == TEXT("branch") || Lower == TEXT("ifthenelse") || Lower == TEXT("k2node_ifthenelse"))
	{
		Pins.Add(MakePin(TEXT("execute"), TEXT("input"), TEXT("exec")));
		Pins.Add(MakePin(TEXT("Condition"), TEXT("input"), TEXT("bool")));
		Pins.Add(MakePin(TEXT("True"), TEXT("output"), TEXT("exec"), false));
		Pins.Add(MakePin(TEXT("False"), TEXT("output"), TEXT("exec"), false));
	}
	else if (Lower == TEXT("sequence") || Lower == TEXT("executionsequence") || Lower == TEXT("k2node_executionsequence"))
	{
		Pins.Add(MakePin(TEXT("execute"), TEXT("input"), TEXT("exec")));
		Pins.Add(MakePin(TEXT("Then 0"), TEXT("output"), TEXT("exec"), false));
		Pins.Add(MakePin(TEXT("Then 1"), TEXT("output"), TEXT("exec"), false));
	}
	else if (Lower == TEXT("eventbeginplay"))
	{
		Pins.Add(MakePin(TEXT("then"), TEXT("output"), TEXT("exec"), false));
	}
	else if (Lower == TEXT("eventtick"))
	{
		Pins.Add(MakePin(TEXT("then"), TEXT("output"), TEXT("exec"), false));
		Pins.Add(MakePin(TEXT("DeltaSeconds"), TEXT("output"), TEXT("float"), false));
	}
	else if (Lower == TEXT("customevent") || Lower == TEXT("k2node_customevent"))
	{
		Pins.Add(MakePin(TEXT("then"), TEXT("output"), TEXT("exec"), false));
	}
	else if (Lower == TEXT("inputkey") || Lower == TEXT("k2node_inputkey"))
	{
		Pins.Add(MakePin(TEXT("Pressed"), TEXT("output"), TEXT("exec"), false));
		Pins.Add(MakePin(TEXT("Released"), TEXT("output"), TEXT("exec"), false));
		Pins.Add(MakePin(TEXT("Key"), TEXT("output"), TEXT("struct:Key"), false));
	}
	else if (Lower == TEXT("switchinteger") || Lower == TEXT("switchint"))
	{
		Pins.Add(MakePin(TEXT("execute"), TEXT("input"), TEXT("exec")));
		Pins.Add(MakePin(TEXT("Selection"), TEXT("input"), TEXT("int32")));
		Pins.Add(MakePin(TEXT("Default"), TEXT("output"), TEXT("exec"), false));
	}
	else if (Lower == TEXT("switchstring"))
	{
		Pins.Add(MakePin(TEXT("execute"), TEXT("input"), TEXT("exec")));
		Pins.Add(MakePin(TEXT("Selection"), TEXT("input"), TEXT("FString")));
		Pins.Add(MakePin(TEXT("Default"), TEXT("output"), TEXT("exec"), false));
	}

	return Pins;
}

// ---------------------------------------------------------------------------
// NormalizeFunctionIdentifier — input canonicalization
// ---------------------------------------------------------------------------
static FString NormalizeFunctionIdentifier(const FString& Raw)
{
	FString Result = Raw.TrimStartAndEnd();

	// Strip ComputeCanonicalId node-type prefixes: "CallFunction:X.Y" → "X.Y"
	// These prefixes are used internally but are not valid function identifiers.
	static const TCHAR* NodeTypePrefixes[] = {
		TEXT("CallFunction:"), TEXT("Event:"), TEXT("CustomEvent:"),
		TEXT("VariableGet:"), TEXT("VariableSet:"), nullptr
	};
	for (int32 i = 0; NodeTypePrefixes[i]; ++i)
	{
		if (Result.StartsWith(NodeTypePrefixes[i]))
		{
			Result = Result.Mid(FCString::Strlen(NodeTypePrefixes[i]));
			break;
		}
	}

	// Strip /Script/Package. or /Game/ prefix
	// e.g. "/Script/AiNpcUE.RPNPCClient.SendSingleStimulus" → "RPNPCClient.SendSingleStimulus"
	if (Result.Contains(TEXT("/Script/")) || Result.Contains(TEXT("/Game/")))
	{
		int32 LastDot;
		if (Result.FindLastChar('.', LastDot))
		{
			FString MaybeFunc = Result.Mid(LastDot + 1);
			FString Prefix = Result.Left(LastDot);
			int32 SecondLastDot;
			if (Prefix.FindLastChar('.', SecondLastDot))
			{
				Result = Prefix.Mid(SecondLastDot + 1) + TEXT(".") + MaybeFunc;
			}
		}
	}

	// Strip U prefix from class name (UClass convention: URPNPCClient → RPNPCClient)
	FString ClassName, FuncName;
	if (Result.Split(TEXT("."), &ClassName, &FuncName))
	{
		if (ClassName.Len() > 1 && ClassName[0] == 'U' && FChar::IsUpper(ClassName[1]))
		{
			ClassName = ClassName.Mid(1);
		}
		Result = ClassName + TEXT(".") + FuncName;
	}

	return Result;
}

// ---------------------------------------------------------------------------
// ResolveCallableFunction — unified callable function resolver
// Only for spawn_strategy=call_function. Does NOT handle known types/events/variables.
// ---------------------------------------------------------------------------
UGenBlueprintNodeCreator::FResolvedFunction UGenBlueprintNodeCreator::ResolveCallableFunction(
	const FString& FunctionName, UBlueprint* Blueprint)
{
	FResolvedFunction Result;
	FString Normalized = NormalizeFunctionIdentifier(FunctionName);

	UFunction* BestFunc = nullptr;
	UClass* BestClass = nullptr;
	FMatchResult BestMatch;
	FString BestSource;

	// Helper: search a class for matching functions
	auto SearchClass = [&](UClass* SearchIn, bool bExcludeSuper, const FString& Source)
	{
		EFieldIteratorFlags::SuperClassFlags Flags = bExcludeSuper
			? EFieldIteratorFlags::ExcludeSuper : EFieldIteratorFlags::IncludeSuper;
		for (TFieldIterator<UFunction> It(SearchIn, Flags); It; ++It)
		{
			UFunction* Func = *It;
			if (!Func->HasAnyFunctionFlags(FUNC_BlueprintCallable)) continue;
			if (Func->HasMetaData(TEXT("DeprecatedFunction"))) continue;
			if (Func->HasMetaData(TEXT("BlueprintInternalUseOnly"))) continue;

			// Score against bare function name from normalized input
			FString QueryName = Normalized;
			FString DotClass, DotFunc;
			if (Normalized.Split(TEXT("."), &DotClass, &DotFunc))
				QueryName = DotFunc;

			FMatchResult M = ScoreFunctionMatch(QueryName, Func->GetName());
			if (M.Score > BestMatch.Score)
			{
				BestMatch = M;
				BestFunc = Func;
				BestClass = SearchIn;
				BestSource = Source;
			}
		}
	};

	// --- Scope 1: Class.Function direct lookup ---
	{
		FString TargetClass, TargetFunc;
		if (Normalized.Split(TEXT("."), &TargetClass, &TargetFunc))
		{
			UClass* Cls = FindFirstObject<UClass>(*TargetClass);
			if (Cls)
			{
				UFunction* Func = Cls->FindFunctionByName(FName(*TargetFunc));
				if (Func)
				{
					Result.Function = Func;
					Result.OwnerClass = Cls;
					Result.CanonicalName = FString::Printf(TEXT("%s.%s"), *Cls->GetName(), *Func->GetName());
					Result.ContextSource = TEXT("direct_lookup");
					Result.bResolved = true;
					UE_LOG(LogTemp, Log, TEXT("ResolveCallableFunction: '%s' → direct lookup %s::%s"),
						*FunctionName, *Cls->GetName(), *Func->GetName());
					return Result;
				}
			}
		}
	}

	// --- Scope 2: Common engine libraries ---
	for (const FString& LibName : GetCommonLibraries())
	{
		UClass* LibClass = FindFirstObject<UClass>(*LibName);
		if (LibClass) SearchClass(LibClass, false, TEXT("engine_lib"));
	}

	// Early return if we already have an exact/accepted match from libraries
	if (BestFunc && BestMatch.bAccepted)
	{
		Result.Function = BestFunc;
		Result.OwnerClass = BestClass;
		Result.CanonicalName = FString::Printf(TEXT("%s.%s"), *BestClass->GetName(), *BestFunc->GetName());
		Result.ContextSource = BestSource;
		Result.bResolved = true;
		UE_LOG(LogTemp, Log, TEXT("ResolveCallableFunction: '%s' → %s %s::%s (score %d)"),
			*FunctionName, *BestSource, *BestClass->GetName(), *BestFunc->GetName(), BestMatch.Score);
		return Result;
	}

	// --- Scope 3: BP class hierarchy ---
	if (Blueprint)
	{
		UClass* BPClass = Blueprint->GeneratedClass
			? Blueprint->GeneratedClass : Blueprint->ParentClass;
		while (BPClass && BPClass != UObject::StaticClass())
		{
			SearchClass(BPClass, true, TEXT("bp_class"));
			BPClass = BPClass->GetSuperClass();
		}
	}
	if (BestFunc && BestMatch.bAccepted)
	{
		Result.Function = BestFunc; Result.OwnerClass = BestClass;
		Result.CanonicalName = FString::Printf(TEXT("%s.%s"), *BestClass->GetName(), *BestFunc->GetName());
		Result.ContextSource = BestSource; Result.bResolved = true;
		UE_LOG(LogTemp, Log, TEXT("ResolveCallableFunction: '%s' → %s %s::%s (score %d)"),
			*FunctionName, *BestSource, *BestClass->GetName(), *BestFunc->GetName(), BestMatch.Score);
		return Result;
	}

	// --- Scope 4: Reflected component properties ---
	if (Blueprint)
	{
		UClass* BPClass = Blueprint->GeneratedClass
			? Blueprint->GeneratedClass : Blueprint->ParentClass;
		if (BPClass)
		{
			for (TFieldIterator<FObjectProperty> It(BPClass); It; ++It)
			{
				if (It->PropertyClass && It->PropertyClass->IsChildOf(UActorComponent::StaticClass()))
				{
					SearchClass(It->PropertyClass, false, TEXT("component_reflected"));
				}
			}
		}
	}
	if (BestFunc && BestMatch.bAccepted)
	{
		Result.Function = BestFunc; Result.OwnerClass = BestClass;
		Result.CanonicalName = FString::Printf(TEXT("%s.%s"), *BestClass->GetName(), *BestFunc->GetName());
		Result.ContextSource = BestSource; Result.bResolved = true;
		UE_LOG(LogTemp, Log, TEXT("ResolveCallableFunction: '%s' → %s %s::%s (score %d)"),
			*FunctionName, *BestSource, *BestClass->GetName(), *BestFunc->GetName(), BestMatch.Score);
		return Result;
	}

	// --- Scope 5: SCS (Blueprint-added) components ---
	if (Blueprint && Blueprint->SimpleConstructionScript)
	{
		TArray<USCS_Node*> SCSNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
		for (USCS_Node* SCSNode : SCSNodes)
		{
			if (SCSNode && SCSNode->ComponentClass &&
				SCSNode->ComponentClass->IsChildOf(UActorComponent::StaticClass()))
			{
				SearchClass(SCSNode->ComponentClass, false, TEXT("component_scs"));
			}
		}
	}

	// Return best match if accepted
	if (BestFunc && BestMatch.bAccepted)
	{
		Result.Function = BestFunc;
		Result.OwnerClass = BestClass;
		Result.CanonicalName = FString::Printf(TEXT("%s.%s"), *BestClass->GetName(), *BestFunc->GetName());
		Result.ContextSource = BestSource;
		Result.bResolved = true;
		UE_LOG(LogTemp, Log, TEXT("ResolveCallableFunction: '%s' → %s %s::%s (score %d)"),
			*FunctionName, *BestSource, *BestClass->GetName(), *BestFunc->GetName(), BestMatch.Score);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ResolveCallableFunction: could not resolve '%s' (best score %d, accepted=%d)"),
			*FunctionName, BestMatch.Score, BestMatch.bAccepted ? 1 : 0);
	}

	return Result;
}

// Backward-compat wrapper: used by PreflightPatch for suggestions
static UFunction* ResolveFunctionWithSuggestions(const FString& FunctionName, UBlueprint* Blueprint,
                                                 TArray<FString>& OutSuggestions)
{
	auto Resolved = UGenBlueprintNodeCreator::ResolveCallableFunction(FunctionName, Blueprint);
	if (Resolved.bResolved)
	{
		OutSuggestions.Empty();
		return Resolved.Function;
	}

	// Build suggestions from common libraries (for error messages)
	FString QueryName = NormalizeFunctionIdentifier(FunctionName);
	FString DotClass, DotFunc;
	if (QueryName.Split(TEXT("."), &DotClass, &DotFunc))
		QueryName = DotFunc;

	for (const FString& LibName : UGenBlueprintNodeCreator::GetCommonLibraries())
	{
		UClass* LibClass = FindFirstObject<UClass>(*LibName);
		if (!LibClass) continue;
		for (TFieldIterator<UFunction> It(LibClass); It; ++It)
		{
			UFunction* Func = *It;
			if (Func->HasMetaData(TEXT("DeprecatedFunction")) || Func->HasMetaData(TEXT("BlueprintInternalUseOnly")))
				continue;
			auto M = UGenBlueprintNodeCreator::ScoreFunctionMatch(QueryName, Func->GetName());
			if (M.Score > 0)
				OutSuggestions.Add(FString::Printf(TEXT("%s.%s"), *LibName, *Func->GetName()));
		}
	}
	OutSuggestions.Sort();
	while (OutSuggestions.Num() > 10) OutSuggestions.RemoveAt(OutSuggestions.Num() - 1);
	return nullptr;
}

// ---------------------------------------------------------------------------
// PreflightPatch — validate patch without mutating the graph
// ---------------------------------------------------------------------------
FString UGenBlueprintNodeCreator::PreflightPatch(const FString& BlueprintPath,
                                                  const FString& FunctionGuid,
                                                  const FString& PatchJson)
{
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> Issues;
	TArray<TSharedPtr<FJsonValue>> PredictedNodes;
	bool bValid = true;

	// Helper: append a structured issue to the Issues array.
	auto AddIssue = [&](const FString& Phase, const FString& Severity, const FString& Message,
	                    int32 Index = -1, const FString& Suggestion = TEXT(""))
	{
		bValid = bValid && (Severity != TEXT("error"));
		TSharedPtr<FJsonObject> Issue = MakeShareable(new FJsonObject);
		Issue->SetStringField(TEXT("phase"), Phase);
		Issue->SetStringField(TEXT("severity"), Severity);
		Issue->SetStringField(TEXT("message"), Message);
		if (Index >= 0) Issue->SetNumberField(TEXT("index"), Index);
		if (!Suggestion.IsEmpty()) Issue->SetStringField(TEXT("suggestion"), Suggestion);
		Issues.Add(MakeShareable(new FJsonValueObject(Issue)));
	};

	// Helper: serialize and return the result (used for early-exit error paths).
	auto SerializeResult = [&]() -> FString
	{
		Result->SetBoolField(TEXT("valid"), bValid);
		Result->SetArrayField(TEXT("issues"), Issues);
		Result->SetArrayField(TEXT("predicted_nodes"), PredictedNodes);
		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Result.ToSharedRef(), W);
		return Out;
	};

	// Parse patch
	TSharedPtr<FJsonObject> Patch;
	{
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(PatchJson);
		if (!FJsonSerializer::Deserialize(Reader, Patch) || !Patch.IsValid())
		{
			AddIssue(TEXT("parse"), TEXT("error"), TEXT("Invalid patch JSON"));
			return SerializeResult();
		}
	}

	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	UEdGraph* Graph = Blueprint ? GetGraphFromFunctionId(Blueprint, FunctionGuid) : nullptr;

	if (!Blueprint || !Graph)
	{
		AddIssue(TEXT("load"), TEXT("error"), !Blueprint
			? TEXT("Blueprint not found: ") + BlueprintPath
			: TEXT("Graph not found: ") + FunctionGuid);
		return SerializeResult();
	}

	// Track predicted pins per ref_id for connection validation
	TMap<FString, TArray<TSharedPtr<FJsonValue>>> RefPins;

	// Validate add_nodes
	const TArray<TSharedPtr<FJsonValue>>* AddNodes;
	if (Patch->TryGetArrayField(TEXT("add_nodes"), AddNodes))
	{
		for (int32 i = 0; i < AddNodes->Num(); i++)
		{
			const TSharedPtr<FJsonObject>* NodeSpec;
			if (!(*AddNodes)[i]->TryGetObject(NodeSpec)) continue;

			FString RefId = (*NodeSpec)->GetStringField(TEXT("ref_id"));
			FString NodeType = (*NodeSpec)->GetStringField(TEXT("node_type"));
			FString FuncName;
			(*NodeSpec)->TryGetStringField(TEXT("function_name"), FuncName);

			TArray<TSharedPtr<FJsonValue>> Pins;
			FString ResolvedType;

			// Try known type first
			Pins = GetKnownTypePins(NodeType);
			if (Pins.Num() > 0)
			{
				ResolvedType = NodeType;
			}
			else if (!FuncName.IsEmpty())
			{
				// Try resolving function
				TArray<FString> Suggestions;
				UFunction* Func = ResolveFunctionWithSuggestions(FuncName, Blueprint, Suggestions);
				if (Func)
				{
					Pins = ExtractPinsFromFunction(Func);
					ResolvedType = FString::Printf(TEXT("%s.%s"),
						*(Func->GetOwnerClass() ? Func->GetOwnerClass()->GetName() : TEXT("Unknown")), *Func->GetName());
				}
				else
				{
					AddIssue(TEXT("add_nodes"), TEXT("error"),
						FString::Printf(TEXT("Cannot resolve function '%s'"), *FuncName),
						i,
						Suggestions.Num() > 0
							? TEXT("Did you mean: ") + FString::Join(Suggestions, TEXT(", "))
							: TEXT(""));
				}
			}
			else
			{
				// Try resolving NodeType as a function name (like "PrintString")
				TArray<FString> Suggestions;
				UFunction* Func = ResolveFunctionWithSuggestions(NodeType, Blueprint, Suggestions);
				if (Func)
				{
					Pins = ExtractPinsFromFunction(Func);
					ResolvedType = FString::Printf(TEXT("%s.%s"),
						*(Func->GetOwnerClass() ? Func->GetOwnerClass()->GetName() : TEXT("Unknown")), *Func->GetName());
				}
				// else: unknown type, no pins predicted (not necessarily an error — could be generic K2Node)
			}

			RefPins.Add(RefId, Pins);

			TSharedPtr<FJsonObject> PredNode = MakeShareable(new FJsonObject);
			PredNode->SetStringField(TEXT("ref_id"), RefId);
			PredNode->SetStringField(TEXT("resolved_type"), ResolvedType);
			PredNode->SetArrayField(TEXT("pins"), Pins);
			PredictedNodes.Add(MakeShareable(new FJsonValueObject(PredNode)));
		}
	}

	// Validate add_connections
	const TArray<TSharedPtr<FJsonValue>>* AddConns;
	if (Patch->TryGetArrayField(TEXT("add_connections"), AddConns))
	{
		for (int32 i = 0; i < AddConns->Num(); i++)
		{
			const TSharedPtr<FJsonObject>* ConnSpec;
			if (!(*AddConns)[i]->TryGetObject(ConnSpec)) continue;

			FString From = (*ConnSpec)->GetStringField(TEXT("from"));
			FString To = (*ConnSpec)->GetStringField(TEXT("to"));

			// Parse endpoints
			auto ParseEndpoint = [](const FString& Ep) -> TPair<FString, FString>
			{
				if (Ep.Contains(TEXT("::")))
				{
					FString NodeRef, PinName;
					Ep.Split(TEXT("::"), &NodeRef, &PinName);
					return {NodeRef, PinName};
				}
				int32 DotIdx;
				if (Ep.FindLastChar('.', DotIdx))
				{
					return {Ep.Left(DotIdx), Ep.Mid(DotIdx + 1)};
				}
				return {Ep, TEXT("")};
			};

			TPair<FString, FString> SrcEp = ParseEndpoint(From);
			FString SrcRef = SrcEp.Key;
			FString SrcPin = SrcEp.Value;
			TPair<FString, FString> TgtEp = ParseEndpoint(To);
			FString TgtRef = TgtEp.Key;
			FString TgtPin = TgtEp.Value;

			// Validate source pin exists in predicted pins
			if (RefPins.Contains(SrcRef) && !SrcPin.IsEmpty())
			{
				bool bFound = false;
				TArray<FString> AvailablePins;
				for (const auto& PinVal : RefPins[SrcRef])
				{
					const TSharedPtr<FJsonObject>* PinObj;
					if (PinVal->TryGetObject(PinObj))
					{
						FString PinName = (*PinObj)->GetStringField(TEXT("name"));
						FString PinDir = (*PinObj)->GetStringField(TEXT("direction"));
						if (PinName == SrcPin && PinDir == TEXT("output"))
						{
							bFound = true;
							break;
						}
						if (PinDir == TEXT("output"))
							AvailablePins.Add(PinName + TEXT("(") + (*PinObj)->GetStringField(TEXT("type")) + TEXT(")"));
					}
				}
				if (!bFound && RefPins[SrcRef].Num() > 0)
				{
					AddIssue(TEXT("add_connections"), TEXT("error"),
						FString::Printf(TEXT("Output pin '%s' not found on node '%s'. Available output pins: %s"),
							*SrcPin, *SrcRef, *FString::Join(AvailablePins, TEXT(", "))),
						i,
						SrcPin.Equals(TEXT("Completed"), ESearchCase::IgnoreCase)
							? TEXT("Use pin name 'then' instead of 'Completed'") : TEXT(""));
				}
			}

			// Validate target pin exists in predicted pins
			if (RefPins.Contains(TgtRef) && !TgtPin.IsEmpty())
			{
				bool bFound = false;
				TArray<FString> AvailablePins;
				for (const auto& PinVal : RefPins[TgtRef])
				{
					const TSharedPtr<FJsonObject>* PinObj;
					if (PinVal->TryGetObject(PinObj))
					{
						FString PinName = (*PinObj)->GetStringField(TEXT("name"));
						FString PinDir = (*PinObj)->GetStringField(TEXT("direction"));
						if (PinName == TgtPin && PinDir == TEXT("input"))
						{
							bFound = true;
							break;
						}
						if (PinDir == TEXT("input"))
							AvailablePins.Add(PinName + TEXT("(") + (*PinObj)->GetStringField(TEXT("type")) + TEXT(")"));
					}
				}
				if (!bFound && RefPins[TgtRef].Num() > 0)
				{
					AddIssue(TEXT("add_connections"), TEXT("error"),
						FString::Printf(TEXT("Input pin '%s' not found on node '%s'. Available input pins: %s"),
							*TgtPin, *TgtRef, *FString::Join(AvailablePins, TEXT(", "))),
						i);
				}
			}
		}
	}

	// Validate remove_nodes
	const TArray<TSharedPtr<FJsonValue>>* RemoveNodes;
	if (Patch->TryGetArrayField(TEXT("remove_nodes"), RemoveNodes))
	{
		for (int32 i = 0; i < RemoveNodes->Num(); i++)
		{
			FString Guid = (*RemoveNodes)[i]->AsString();
			FGuid ParsedGuid;
			if (FGuid::Parse(Guid, ParsedGuid))
			{
				bool bFound = false;
				for (UEdGraphNode* Node : Graph->Nodes)
				{
					if (Node && Node->NodeGuid == ParsedGuid)
					{
						bFound = true;
						break;
					}
				}
				if (!bFound)
				{
					AddIssue(TEXT("remove_nodes"), TEXT("warning"),
						FString::Printf(TEXT("Node GUID '%s' not found in graph"), *Guid), i);
				}
			}
		}
	}

	return SerializeResult();
}

// ===========================================================================
// B1-B4: Node Discovery — Index, Search, Inspect
// ===========================================================================

static TArray<UGenBlueprintNodeCreator::FNodeIndexEntry> GNodeIndex;
// O(1) lookup by canonical name — rebuilt alongside GNodeIndex in BuildNodeIndex()
static TMap<FString, int32> GNodeIndexByName;
static bool bNodeIndexBuilt = false;

bool UGenBlueprintNodeCreator::IsNodeIndexBuilt() { return bNodeIndexBuilt; }

void UGenBlueprintNodeCreator::InvalidateNodeIndex()
{
	int32 Count = GNodeIndex.Num();
	GNodeIndex.Empty();
	GNodeIndexByName.Empty();
	bNodeIndexBuilt = false;
	UE_LOG(LogTemp, Log, TEXT("InvalidateNodeIndex: cleared %d entries, will rebuild on next search"), Count);
}

int32 UGenBlueprintNodeCreator::BuildNodeIndex()
{
	GNodeIndex.Empty();
	GNodeIndexByName.Empty();

	// --- 1. Special nodes (hardcoded) ---
	auto AddSpecial = [](const FString& Name, const FString& Display, const FString& Cat, const FString& Kw)
	{
		FNodeIndexEntry E;
		E.CanonicalName = Name;
		E.DisplayName = Display;
		E.Category = Cat;
		E.Keywords = Kw;
		E.Kind = ENodeKind::Special;
		GNodeIndex.Add(E);
	};
	AddSpecial(TEXT("K2Node_IfThenElse"), TEXT("Branch"), TEXT("Flow Control"), TEXT("branch if else condition"));
	AddSpecial(TEXT("K2Node_ExecutionSequence"), TEXT("Sequence"), TEXT("Flow Control"), TEXT("sequence order"));
	AddSpecial(TEXT("K2Node_SwitchInteger"), TEXT("Switch on Int"), TEXT("Flow Control"), TEXT("switch int integer"));
	AddSpecial(TEXT("K2Node_SwitchString"), TEXT("Switch on String"), TEXT("Flow Control"), TEXT("switch string"));
	AddSpecial(TEXT("K2Node_SwitchEnum"), TEXT("Switch on Enum"), TEXT("Flow Control"), TEXT("switch enum"));

	// --- 2. Events ---
	auto AddEvent = [](const FString& Name, const FString& Display, const FString& Kw)
	{
		FNodeIndexEntry E;
		E.CanonicalName = Name;
		E.DisplayName = Display;
		E.Category = TEXT("Events");
		E.Keywords = Kw;
		E.Kind = ENodeKind::Event;
		GNodeIndex.Add(E);
	};
	AddEvent(TEXT("Event:ReceiveBeginPlay"), TEXT("Event BeginPlay"), TEXT("begin play start"));
	AddEvent(TEXT("Event:ReceiveTick"), TEXT("Event Tick"), TEXT("tick update frame"));
	AddEvent(TEXT("Event:ReceiveActorBeginOverlap"), TEXT("Actor Begin Overlap"), TEXT("overlap collision begin"));
	AddEvent(TEXT("Event:ReceiveActorEndOverlap"), TEXT("Actor End Overlap"), TEXT("overlap collision end"));
	AddEvent(TEXT("Event:ReceiveAnyDamage"), TEXT("Any Damage"), TEXT("damage hit health"));
	AddEvent(TEXT("Event:ReceiveDestroyed"), TEXT("Destroyed"), TEXT("destroy death remove"));

	// --- 3. All UBlueprintFunctionLibrary subclasses ---
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;
		if (!Class->IsChildOf(UBlueprintFunctionLibrary::StaticClass())) continue;
		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated)) continue;
		if (Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_"))) continue;

		for (TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
		{
			UFunction* Func = *FuncIt;
			if (!Func->HasAnyFunctionFlags(FUNC_BlueprintCallable)) continue;
			if (Func->HasMetaData(TEXT("BlueprintInternalUseOnly"))) continue;

			FNodeIndexEntry E;
			E.CanonicalName = FString::Printf(TEXT("%s.%s"), *Class->GetName(), *Func->GetName());
			E.DisplayName = Func->GetDisplayNameText().ToString();
			E.Category = Func->HasMetaData(TEXT("Category")) ? Func->GetMetaData(TEXT("Category")) : TEXT("");
			E.Keywords = Func->HasMetaData(TEXT("Keywords")) ? Func->GetMetaData(TEXT("Keywords")) : TEXT("");
			E.ToolTip = Func->HasMetaData(TEXT("ToolTip")) ? Func->GetMetaData(TEXT("ToolTip")) : TEXT("");
			E.Function = Func;
			E.OwnerClass = Class;
			E.Kind = ENodeKind::Function;
			E.bLatent = Func->HasMetaData(TEXT("Latent"));
			E.bPure = Func->HasAnyFunctionFlags(FUNC_BlueprintPure);
			E.bDeprecated = Func->HasMetaData(TEXT("DeprecatedFunction"));
			E.bWorldContext = false;

			for (TFieldIterator<FProperty> PropIt(Func); PropIt; ++PropIt)
			{
				if (PropIt->HasMetaData(TEXT("WorldContext")))
				{
					E.bWorldContext = true;
					break;
				}
			}

			// Append ToolTip words to keywords for broader matching
			if (!E.ToolTip.IsEmpty())
			{
				E.Keywords += TEXT(" ") + E.ToolTip;
			}

			GNodeIndex.Add(E);
		}
	}

	// --- 4. Actor, Pawn, Character methods (non-library) ---
	TArray<UClass*> ActorClasses;
	ActorClasses.Add(AActor::StaticClass());
	ActorClasses.Add(APawn::StaticClass());
	ActorClasses.Add(ACharacter::StaticClass());

	for (UClass* ActorClass : ActorClasses)
	{
		for (TFieldIterator<UFunction> FuncIt(ActorClass, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
		{
			UFunction* Func = *FuncIt;
			if (!Func->HasAnyFunctionFlags(FUNC_BlueprintCallable)) continue;
			if (Func->HasMetaData(TEXT("BlueprintInternalUseOnly"))) continue;
			if (Func->HasMetaData(TEXT("DeprecatedFunction"))) continue;

			FString CanonicalName = FString::Printf(TEXT("%s.%s"), *ActorClass->GetName(), *Func->GetName());
			// Skip if already indexed — O(1) map lookup
			if (GNodeIndexByName.Contains(CanonicalName)) continue;

			FNodeIndexEntry E;
			E.CanonicalName = CanonicalName;
			E.DisplayName = Func->GetDisplayNameText().ToString();
			E.Category = Func->HasMetaData(TEXT("Category")) ? Func->GetMetaData(TEXT("Category")) : TEXT("");
			E.Keywords = Func->HasMetaData(TEXT("Keywords")) ? Func->GetMetaData(TEXT("Keywords")) : TEXT("");
			E.ToolTip = Func->HasMetaData(TEXT("ToolTip")) ? Func->GetMetaData(TEXT("ToolTip")) : TEXT("");
			E.Function = Func;
			E.OwnerClass = ActorClass;
			E.Kind = ENodeKind::Function;
			E.bLatent = Func->HasMetaData(TEXT("Latent"));
			E.bPure = Func->HasAnyFunctionFlags(FUNC_BlueprintPure);
			E.bWorldContext = false;
			if (!E.ToolTip.IsEmpty()) E.Keywords += TEXT(" ") + E.ToolTip;
			GNodeIndex.Add(E);
		}
	}

	// Build O(1) name lookup map
	for (int32 i = 0; i < GNodeIndex.Num(); i++)
		GNodeIndexByName.Add(GNodeIndex[i].CanonicalName, i);

	bNodeIndexBuilt = true;
	UE_LOG(LogTemp, Log, TEXT("BuildNodeIndex: indexed %d entries"), GNodeIndex.Num());
	return GNodeIndex.Num();
}

// ---------------------------------------------------------------------------
// B2: SearchBlueprintNodes — lightweight shortlist
// ---------------------------------------------------------------------------

// Score query against a node index entry (combines text, category, keyword matching)
static float ScoreNodeEntry(const UGenBlueprintNodeCreator::FNodeIndexEntry& Entry, const FString& Query,
                            const TArray<FString>& QueryTokens, bool bBPContextMatch)
{
	float Score = 0.0f;

	// Text similarity against canonical name (weight 0.4)
	auto NameMatch = UGenBlueprintNodeCreator::ScoreFunctionMatch(Query, Entry.CanonicalName);
	// Also try against just the function part (after the dot)
	FString FuncPart = Entry.CanonicalName;
	int32 DotIdx;
	if (FuncPart.FindChar('.', DotIdx))
		FuncPart = FuncPart.Mid(DotIdx + 1);
	auto FuncMatch = UGenBlueprintNodeCreator::ScoreFunctionMatch(Query, FuncPart);
	float TextScore = FMath::Max(NameMatch.Score, FuncMatch.Score) / 200.0f;  // Normalize to 0-1
	Score += TextScore * 0.4f;

	// Keyword match (weight 0.3)
	if (!Entry.Keywords.IsEmpty() && QueryTokens.Num() > 0)
	{
		FString KeywordsLower = Entry.Keywords.ToLower();
		int32 Hits = 0;
		for (const FString& Token : QueryTokens)
		{
			if (KeywordsLower.Contains(Token))
				Hits++;
		}
		float KeywordScore = (float)Hits / (float)QueryTokens.Num();
		Score += KeywordScore * 0.3f;
	}

	// Category match (weight 0.1)
	if (!Entry.Category.IsEmpty() && QueryTokens.Num() > 0)
	{
		FString CatLower = Entry.Category.ToLower();
		for (const FString& Token : QueryTokens)
		{
			if (CatLower.Contains(Token))
			{
				Score += 0.1f;
				break;
			}
		}
	}

	// Context proximity (weight 0.2)
	if (bBPContextMatch)
	{
		Score += 0.2f;
	}

	return Score;
}

FString UGenBlueprintNodeCreator::SearchBlueprintNodes(const FString& Query, const FString& BlueprintPath,
                                                       const FString& CategoryFilter, int32 MaxResults,
                                                       const FString& SchemaMode)
{
	bool bSemantic = SchemaMode.Equals(TEXT("semantic"), ESearchCase::IgnoreCase);
	// Ensure index is built
	if (!bNodeIndexBuilt) BuildNodeIndex();

	MaxResults = FMath::Clamp(MaxResults, 1, 10);

	// Tokenize query for keyword matching
	TArray<FString> QueryTokens;
	FString QueryLower = Query.ToLower();
	QueryLower.ParseIntoArrayWS(QueryTokens);
	// Also add camelcase-split tokens
	TArray<FString> CamelTokens = SplitCamelCase(Query);
	for (const FString& CT : CamelTokens)
	{
		FString Lower = CT.ToLower();
		if (!QueryTokens.Contains(Lower))
			QueryTokens.Add(Lower);
	}

	// Load Blueprint once — reused for context scoring and context overlay below.
	UBlueprint* ContextBP = BlueprintPath.IsEmpty() ? nullptr
		: LoadObject<UBlueprint>(nullptr, *BlueprintPath);

	// Determine BP context class for proximity scoring
	TSet<FString> BPClassNames;
	if (ContextBP)
	{
		UClass* BPClass = ContextBP->GeneratedClass ? ContextBP->GeneratedClass : ContextBP->ParentClass;
		while (BPClass)
		{
			BPClassNames.Add(BPClass->GetName());
			BPClass = BPClass->GetSuperClass();
		}
	}

	struct FScoredEntry
	{
		int32 GlobalIndex = -1;    // Index into GNodeIndex, or -1 if from ContextEntries
		int32 ContextIndex = -1;   // Index into ContextEntries, or -1 if from GNodeIndex
		float Score = 0.0f;

		const FNodeIndexEntry& GetEntry(const TArray<FNodeIndexEntry>& ContextArr) const
		{
			return (GlobalIndex >= 0) ? GNodeIndex[GlobalIndex] : ContextArr[ContextIndex];
		}
	};
	TArray<FScoredEntry> Candidates;

	FString CategoryFilterLower = CategoryFilter.ToLower();

	for (int32 i = 0; i < GNodeIndex.Num(); i++)
	{
		const FNodeIndexEntry& Entry = GNodeIndex[i];
		// Hard filter: deprecated
		if (Entry.bDeprecated) continue;

		// Hard filter: category
		if (!CategoryFilterLower.IsEmpty() && !Entry.Category.ToLower().Contains(CategoryFilterLower))
			continue;

		// Determine context match
		bool bContextMatch = Entry.OwnerClass && BPClassNames.Contains(Entry.OwnerClass->GetName());

		float Score = ScoreNodeEntry(Entry, Query, QueryTokens, bContextMatch);

		// Only keep entries with meaningful score
		if (Score > 0.05f)
		{
			FScoredEntry SE;
			SE.GlobalIndex = i;
			SE.Score = Score;
			Candidates.Add(SE);
		}
	}

	// --- Context Overlay: dynamically add BP-specific candidates not in global index ---
	TArray<FNodeIndexEntry> ContextEntries;  // Safe: FScoredEntry uses indices, not pointers
	if (ContextBP)
	{
		{
			auto AddContextFunction = [&](UFunction* Func, UClass* Owner, const FString& Source)
			{
				if (!Func->HasAnyFunctionFlags(FUNC_BlueprintCallable)) return;
				if (Func->HasMetaData(TEXT("DeprecatedFunction"))) return;
				if (Func->HasMetaData(TEXT("BlueprintInternalUseOnly"))) return;

				FString CanonicalName = FString::Printf(TEXT("%s.%s"), *Owner->GetName(), *Func->GetName());
				if (GNodeIndexByName.Contains(CanonicalName)) return; // O(1) — already in global index

				FNodeIndexEntry E;
				E.CanonicalName = CanonicalName;
				E.DisplayName = Func->GetDisplayNameText().ToString();
				E.Category = Func->HasMetaData(TEXT("Category")) ? Func->GetMetaData(TEXT("Category")) : TEXT("");
				E.Keywords = Func->HasMetaData(TEXT("Keywords")) ? Func->GetMetaData(TEXT("Keywords")) : TEXT("");
				E.ToolTip = Func->HasMetaData(TEXT("ToolTip")) ? Func->GetMetaData(TEXT("ToolTip")) : TEXT("");
				if (!E.ToolTip.IsEmpty()) E.Keywords += TEXT(" ") + E.ToolTip;
				E.Function = Func;
				E.OwnerClass = Owner;
				E.Kind = ENodeKind::Function;
				E.bLatent = Func->HasMetaData(TEXT("Latent"));
				E.bPure = Func->HasAnyFunctionFlags(FUNC_BlueprintPure);
				E.bWorldContext = false;
				ContextEntries.Add(E);
			};

			UClass* BPClass = ContextBP->GeneratedClass ? ContextBP->GeneratedClass : ContextBP->ParentClass;

			// (a) BP class hierarchy own methods
			UClass* WalkClass = BPClass;
			while (WalkClass && WalkClass != UObject::StaticClass())
			{
				for (TFieldIterator<UFunction> It(WalkClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
				{
					AddContextFunction(*It, WalkClass, TEXT("bp_class"));
				}
				WalkClass = WalkClass->GetSuperClass();
			}

			// (b) Reflected component properties
			if (BPClass)
			{
				for (TFieldIterator<FObjectProperty> It(BPClass); It; ++It)
				{
					if (It->PropertyClass && It->PropertyClass->IsChildOf(UActorComponent::StaticClass()))
					{
						for (TFieldIterator<UFunction> FuncIt(It->PropertyClass, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
						{
							AddContextFunction(*FuncIt, It->PropertyClass, TEXT("component_reflected"));
						}
					}
				}
			}

			// (c) SCS (Blueprint-added) components
			if (ContextBP->SimpleConstructionScript)
			{
				TArray<USCS_Node*> SCSNodes = ContextBP->SimpleConstructionScript->GetAllNodes();
				for (USCS_Node* SCSNode : SCSNodes)
				{
					if (SCSNode && SCSNode->ComponentClass &&
						SCSNode->ComponentClass->IsChildOf(UActorComponent::StaticClass()))
					{
						for (TFieldIterator<UFunction> FuncIt(SCSNode->ComponentClass, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
						{
							AddContextFunction(*FuncIt, SCSNode->ComponentClass, TEXT("component_scs"));
						}
					}
				}
			}

			// Score context entries with context proximity boost (use index, not pointer)
			for (int32 ci = 0; ci < ContextEntries.Num(); ci++)
			{
				const FNodeIndexEntry& Entry = ContextEntries[ci];
				if (!CategoryFilterLower.IsEmpty() && !Entry.Category.ToLower().Contains(CategoryFilterLower))
					continue;
				float Score = ScoreNodeEntry(Entry, Query, QueryTokens, true);
				if (Score > 0.05f)
				{
					FScoredEntry SE;
					SE.ContextIndex = ci;
					SE.Score = Score;
					Candidates.Add(SE);
				}
			}
		}
	}

	// Sort by score descending
	Candidates.Sort([](const FScoredEntry& A, const FScoredEntry& B) { return A.Score > B.Score; });

	// Dedup consecutive entries with same canonical name (keep higher-scored one)
	for (int32 i = Candidates.Num() - 1; i > 0; i--)
	{
		if (Candidates[i].GetEntry(ContextEntries).CanonicalName ==
			Candidates[i - 1].GetEntry(ContextEntries).CanonicalName)
		{
			Candidates.RemoveAt(i);
		}
	}

	// Build result JSON
	TArray<TSharedPtr<FJsonValue>> CandidatesArray;
	int32 Count = FMath::Min(Candidates.Num(), MaxResults);
	for (int32 i = 0; i < Count; i++)
	{
		const FNodeIndexEntry& E = Candidates[i].GetEntry(ContextEntries);

		TSharedPtr<FJsonObject> Obj = MakeShareable(new FJsonObject);
		Obj->SetStringField(bSemantic ? TEXT("cname") : TEXT("canonical_name"), E.CanonicalName);
		Obj->SetStringField(bSemantic ? TEXT("label") : TEXT("display_name"), E.DisplayName);

		FString KindStr;
		switch (E.Kind)
		{
		case ENodeKind::Function: KindStr = TEXT("function"); break;
		case ENodeKind::Event: KindStr = TEXT("event"); break;
		case ENodeKind::Macro: KindStr = TEXT("macro"); break;
		case ENodeKind::VariableGet: KindStr = TEXT("variable_get"); break;
		case ENodeKind::VariableSet: KindStr = TEXT("variable_set"); break;
		case ENodeKind::Special: KindStr = TEXT("special"); break;
		}
		Obj->SetStringField(bSemantic ? TEXT("kind") : TEXT("node_kind"), KindStr);

		FString SpawnStrategy = E.Kind == ENodeKind::Special ? TEXT("known_type") :
		                        E.Kind == ENodeKind::Event ? TEXT("event") : TEXT("call_function");
		Obj->SetStringField(bSemantic ? TEXT("spawn") : TEXT("spawn_strategy"), SpawnStrategy);
		Obj->SetStringField(bSemantic ? TEXT("cat") : TEXT("category"), E.Category);
		FString ShortKw = E.Keywords.Left(100);
		Obj->SetStringField(bSemantic ? TEXT("kw") : TEXT("keywords"), ShortKw);
		Obj->SetNumberField(bSemantic ? TEXT("score") : TEXT("relevance_score"), Candidates[i].Score);
		Obj->SetBoolField(bSemantic ? TEXT("latent") : TEXT("is_latent"), E.bLatent);
		Obj->SetBoolField(bSemantic ? TEXT("pure") : TEXT("is_pure"), E.bPure);

		CandidatesArray.Add(MakeShareable(new FJsonValueObject(Obj)));
	}

	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
	Root->SetArrayField(TEXT("candidates"), CandidatesArray);
	Root->SetNumberField(bSemantic ? TEXT("total") : TEXT("total_indexed"), GNodeIndex.Num());
	Root->SetNumberField(bSemantic ? TEXT("filtered") : TEXT("filtered_out"), GNodeIndex.Num() - Candidates.Num());

	FString ResultJson;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	return ResultJson;
}

// ---------------------------------------------------------------------------
// B3: InspectBlueprintNode — full pin schema for a specific canonical name
// ---------------------------------------------------------------------------

FString UGenBlueprintNodeCreator::InspectBlueprintNode(const FString& CanonicalName, const FString& BlueprintPath,
                                                       const FString& SchemaMode)
{
	if (!bNodeIndexBuilt) BuildNodeIndex();
	bool bSemantic = SchemaMode.Equals(TEXT("semantic"), ESearchCase::IgnoreCase);

	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
	Root->SetStringField(bSemantic ? TEXT("cname") : TEXT("canonical_name"), CanonicalName);

	// O(1) map lookup instead of O(N) linear scan
	const FNodeIndexEntry* Found = nullptr;
	if (const int32* Idx = GNodeIndexByName.Find(CanonicalName))
		Found = &GNodeIndex[*Idx];

	// If not found in index, try resolving as function name
	UFunction* ResolvedFunc = nullptr;
	UClass* ResolvedClass = nullptr;

	if (Found && Found->Function)
	{
		ResolvedFunc = Found->Function;
		ResolvedClass = Found->OwnerClass;
	}
	else if (!Found)
	{
		// Use unified resolver — covers Class.Function, BP hierarchy, components, SCS
		UBlueprint* BP = !BlueprintPath.IsEmpty()
			? LoadObject<UBlueprint>(nullptr, *BlueprintPath) : nullptr;
		FResolvedFunction Resolved = ResolveCallableFunction(CanonicalName, BP);
		if (Resolved.bResolved)
		{
			ResolvedFunc = Resolved.Function;
			ResolvedClass = Resolved.OwnerClass;
		}
	}

	// Key names based on schema mode
	FString KHint = bSemantic ? TEXT("hint") : TEXT("patch_hint");
	FString KCtx = bSemantic ? TEXT("ctx") : TEXT("context_requirements");
	FString KNodeType = bSemantic ? TEXT("type") : TEXT("node_type");
	FString KFuncName = bSemantic ? TEXT("func") : TEXT("function_name");
	FString KReqWorld = bSemantic ? TEXT("req_world") : TEXT("needs_world_context");
	FString KReqLatent = bSemantic ? TEXT("req_latent") : TEXT("needs_latent_support");

	// Handle special/known types
	if (Found && Found->Kind == ENodeKind::Special)
	{
		TSharedPtr<FJsonObject> PatchHint = MakeShareable(new FJsonObject);
		PatchHint->SetStringField(KNodeType, CanonicalName);
		Root->SetObjectField(KHint, PatchHint);
		Root->SetArrayField(TEXT("pins"), GetKnownTypePins(CanonicalName));

		TSharedPtr<FJsonObject> Ctx = MakeShareable(new FJsonObject);
		if (!bSemantic) { Ctx->SetBoolField(KReqWorld, false); Ctx->SetBoolField(KReqLatent, false); }
		Root->SetObjectField(KCtx, Ctx);
	}
	else if (Found && Found->Kind == ENodeKind::Event)
	{
		TSharedPtr<FJsonObject> PatchHint = MakeShareable(new FJsonObject);
		FString EventType = CanonicalName;
		EventType.ReplaceInline(TEXT("Event:Receive"), TEXT("Event"));
		PatchHint->SetStringField(KNodeType, EventType);
		Root->SetObjectField(KHint, PatchHint);
		Root->SetArrayField(TEXT("pins"), GetKnownTypePins(EventType));

		TSharedPtr<FJsonObject> Ctx = MakeShareable(new FJsonObject);
		if (!bSemantic) { Ctx->SetBoolField(KReqWorld, false); Ctx->SetBoolField(KReqLatent, false); }
		Root->SetObjectField(KCtx, Ctx);
	}
	else if (ResolvedFunc)
	{
		TSharedPtr<FJsonObject> PatchHint = MakeShareable(new FJsonObject);
		PatchHint->SetStringField(KNodeType, TEXT("K2Node_CallFunction"));
		PatchHint->SetStringField(KFuncName,
			FString::Printf(TEXT("%s.%s"), *ResolvedClass->GetName(), *ResolvedFunc->GetName()));
		Root->SetObjectField(KHint, PatchHint);

		Root->SetArrayField(TEXT("pins"), ExtractPinsFromFunction(ResolvedFunc));

		TSharedPtr<FJsonObject> Ctx = MakeShareable(new FJsonObject);
		bool bWorld = Found ? Found->bWorldContext : false;
		bool bLatent = Found ? Found->bLatent : false;
		if (bWorld || !bSemantic) Ctx->SetBoolField(KReqWorld, bWorld);
		if (bLatent || !bSemantic) Ctx->SetBoolField(KReqLatent, bLatent);
		Root->SetObjectField(KCtx, Ctx);
	}
	else
	{
		Root->SetStringField(TEXT("error"),
			FString::Printf(TEXT("Cannot resolve '%s'. Use search_blueprint_nodes to find available nodes."),
				*CanonicalName));
	}

	FString ResultJson;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	return ResultJson;
}

