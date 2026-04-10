// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License.

#include "Commandlets/BlueprintExportCommandlet.h"
#include "MCP/GenBlueprintNodeCreator.h"
#include "MCP/GenBlueprintUtils.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_InputKey.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

UBlueprintExportCommandlet::UBlueprintExportCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UBlueprintExportCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Log, TEXT("BlueprintExportCommandlet: Starting..."));

	// Parse parameters
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamMap;
	ParseCommandLine(*Params, Tokens, Switches, ParamMap);

	bool bExportAll = Switches.Contains(TEXT("all"));
	bool bCompact = Switches.Contains(TEXT("compact"));
	FString OutputPath;
	ParamMap.RemoveAndCopyValue(TEXT("output"), OutputPath);

	// Collect blueprint paths to export
	TArray<FString> BlueprintPaths;

	FString SinglePath;
	if (ParamMap.RemoveAndCopyValue(TEXT("blueprint"), SinglePath))
	{
		BlueprintPaths.Add(SinglePath);
	}

	// Also collect from tokens (positional args that look like asset paths)
	for (const FString& Token : Tokens)
	{
		if (Token.StartsWith(TEXT("/Game/")))
		{
			BlueprintPaths.Add(Token);
		}
	}

	if (bExportAll)
	{
		// Scan all Blueprint assets under /Game
		FString ScanJson = UGenBlueprintUtils::ScanAllBlueprints();
		TSharedPtr<FJsonValue> ParsedArray;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ScanJson);
		if (FJsonSerializer::Deserialize(Reader, ParsedArray) && ParsedArray.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>& Items = ParsedArray->AsArray();
			for (const auto& Item : Items)
			{
				const TSharedPtr<FJsonObject>& Obj = Item->AsObject();
				if (Obj.IsValid())
				{
					FString Path = Obj->GetStringField(TEXT("package"));
					if (!Path.IsEmpty())
					{
						BlueprintPaths.Add(Path);
					}
				}
			}
		}
		UE_LOG(LogTemp, Log, TEXT("BlueprintExportCommandlet: Found %d blueprints to export"), BlueprintPaths.Num());
	}

	if (BlueprintPaths.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("BlueprintExportCommandlet: No blueprints specified. Use -blueprint=/Game/Path/BP_Name or -all"));
		return 1;
	}

	// Export each blueprint
	TArray<TSharedPtr<FJsonValue>> ExportedArray;

	for (const FString& BPPath : BlueprintPaths)
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BPPath);
		if (!Blueprint)
		{
			UE_LOG(LogTemp, Warning, TEXT("BlueprintExportCommandlet: Could not load '%s', skipping"), *BPPath);
			continue;
		}

		TSharedPtr<FJsonObject> IR = ExportBlueprintIR(Blueprint);
		if (IR.IsValid())
		{
			ExportedArray.Add(MakeShareable(new FJsonValueObject(IR)));
			UE_LOG(LogTemp, Log, TEXT("BlueprintExportCommandlet: Exported '%s'"), *BPPath);
		}
	}

	// Build final output
	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
	Root->SetStringField(TEXT("schema"), TEXT("blueprint_ir_v1"));
	Root->SetNumberField(TEXT("count"), ExportedArray.Num());
	Root->SetArrayField(TEXT("blueprints"), ExportedArray);

	// Serialize
	FString OutputJson;
	if (bCompact)
	{
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputJson);
		FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	}
	else
	{
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputJson);
		FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	}

	// Output
	if (OutputPath.IsEmpty())
	{
		// stdout
		UE_LOG(LogTemp, Display, TEXT("%s"), *OutputJson);
	}
	else
	{
		FString FullPath = OutputPath.StartsWith(TEXT("/")) || OutputPath.Contains(TEXT(":/"))
			? OutputPath
			: FPaths::Combine(FPaths::ProjectDir(), OutputPath);
		if (FFileHelper::SaveStringToFile(OutputJson, *FullPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			UE_LOG(LogTemp, Log, TEXT("BlueprintExportCommandlet: Saved to '%s' (%d bytes)"), *FullPath, OutputJson.Len());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("BlueprintExportCommandlet: Failed to save to '%s'"), *FullPath);
			return 1;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("BlueprintExportCommandlet: Done. Exported %d blueprints."), ExportedArray.Num());
	return 0;
}

TSharedPtr<FJsonObject> UBlueprintExportCommandlet::ExportBlueprintIR(UBlueprint* Blueprint)
{
	if (!Blueprint) return nullptr;

	TSharedPtr<FJsonObject> IR = MakeShareable(new FJsonObject);

	// Asset identity
	FString AssetPath = Blueprint->GetPathName();
	// Strip the _C suffix if present in the path
	AssetPath = AssetPath.Replace(TEXT(".Default__"), TEXT("."));
	IR->SetStringField(TEXT("asset"), Blueprint->GetOutermost()->GetName());
	IR->SetStringField(TEXT("name"), Blueprint->GetName());

	// Parent class
	UClass* ParentClass = Blueprint->ParentClass;
	IR->SetStringField(TEXT("parent_class"), ParentClass ? ParentClass->GetName() : TEXT("None"));
	IR->SetStringField(TEXT("parent_path"), ParentClass ? ParentClass->GetPathName() : TEXT(""));

	// Variables (members)
	TArray<TSharedPtr<FJsonValue>> VarsArray;
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		TSharedPtr<FJsonObject> VarObj = MakeShareable(new FJsonObject);
		VarObj->SetStringField(TEXT("name"), Var.VarName.ToString());
		VarObj->SetStringField(TEXT("type"), Var.VarType.PinCategory.ToString());
		if (Var.VarType.PinSubCategoryObject.IsValid())
		{
			VarObj->SetStringField(TEXT("subtype"), Var.VarType.PinSubCategoryObject->GetName());
		}
		VarObj->SetStringField(TEXT("category"), Var.Category.ToString());
		VarObj->SetBoolField(TEXT("editable"), Var.PropertyFlags & CPF_Edit ? true : false);
		if (!Var.DefaultValue.IsEmpty())
		{
			VarObj->SetStringField(TEXT("default"), Var.DefaultValue);
		}
		VarsArray.Add(MakeShareable(new FJsonValueObject(VarObj)));
	}
	IR->SetArrayField(TEXT("variables"), VarsArray);

	// Components (from SCS)
	TArray<TSharedPtr<FJsonValue>> CompsArray;
	if (Blueprint->SimpleConstructionScript)
	{
		TArray<USCS_Node*> SCSNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
		for (USCS_Node* SCSNode : SCSNodes)
		{
			if (!SCSNode) continue;
			TSharedPtr<FJsonObject> CompObj = MakeShareable(new FJsonObject);
			CompObj->SetStringField(TEXT("name"), SCSNode->GetVariableName().ToString());
			CompObj->SetStringField(TEXT("class"), SCSNode->ComponentClass ? SCSNode->ComponentClass->GetName() : TEXT("None"));
			CompsArray.Add(MakeShareable(new FJsonValueObject(CompObj)));
		}
	}
	IR->SetArrayField(TEXT("components"), CompsArray);

	// Graphs
	TArray<TSharedPtr<FJsonValue>> GraphsArray;

	auto ExportGraphSet = [&](const TArray<UEdGraph*>& Graphs, const FString& GraphType)
	{
		for (UEdGraph* Graph : Graphs)
		{
			if (!Graph) continue;
			TSharedPtr<FJsonObject> GraphIR = ExportGraphIR(Blueprint, Graph);
			if (GraphIR.IsValid())
			{
				GraphIR->SetStringField(TEXT("graph_type"), GraphType);
				GraphsArray.Add(MakeShareable(new FJsonValueObject(GraphIR)));
			}
		}
	};

	ExportGraphSet(Blueprint->UbergraphPages, TEXT("EventGraph"));
	ExportGraphSet(Blueprint->FunctionGraphs, TEXT("Function"));
	ExportGraphSet(Blueprint->MacroGraphs, TEXT("Macro"));

	IR->SetArrayField(TEXT("graphs"), GraphsArray);

	return IR;
}

TSharedPtr<FJsonObject> UBlueprintExportCommandlet::ExportGraphIR(UBlueprint* Blueprint, UEdGraph* Graph)
{
	if (!Graph) return nullptr;

	TSharedPtr<FJsonObject> GraphObj = MakeShareable(new FJsonObject);
	GraphObj->SetStringField(TEXT("name"), Graph->GetName());
	GraphObj->SetStringField(TEXT("guid"), Graph->GraphGuid.ToString());
	GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());

	// Nodes
	TArray<TSharedPtr<FJsonValue>> NodesArray;
	TMap<FGuid, int32> GuidToIndex;  // For edge building

	int32 NodeIndex = 0;
	TMap<FString, int32> CanonicalCounter;  // For instance_id

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node) continue;
		GuidToIndex.Add(Node->NodeGuid, NodeIndex++);

		TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject);

		// Identity
		NodeObj->SetStringField(TEXT("guid"), Node->NodeGuid.ToString());
		NodeObj->SetStringField(TEXT("type"), Node->GetClass()->GetName());
		NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

		// Canonical ID (reuse existing logic pattern)
		FString CanonicalId;
		if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(Node))
		{
			UFunction* Func = FuncNode->GetTargetFunction();
			if (Func && Func->GetOwnerClass())
				CanonicalId = FString::Printf(TEXT("CallFunction:%s.%s"), *Func->GetOwnerClass()->GetName(), *Func->GetName());
			else
			{
				FName MemberName = FuncNode->FunctionReference.GetMemberName();
				CanonicalId = FString::Printf(TEXT("CallFunction:%s"), MemberName.IsValid() ? *MemberName.ToString() : TEXT("None"));
			}
		}
		else if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
			CanonicalId = FString::Printf(TEXT("Event:%s"), *EventNode->EventReference.GetMemberName().ToString());
		else if (UK2Node_CustomEvent* CE = Cast<UK2Node_CustomEvent>(Node))
			CanonicalId = FString::Printf(TEXT("CustomEvent:%s"), *CE->CustomFunctionName.ToString());
		else if (UK2Node_VariableGet* VG = Cast<UK2Node_VariableGet>(Node))
			CanonicalId = FString::Printf(TEXT("VariableGet:%s"), *VG->VariableReference.GetMemberName().ToString());
		else if (UK2Node_VariableSet* VS = Cast<UK2Node_VariableSet>(Node))
			CanonicalId = FString::Printf(TEXT("VariableSet:%s"), *VS->VariableReference.GetMemberName().ToString());
		else
			CanonicalId = Node->GetClass()->GetName();

		NodeObj->SetStringField(TEXT("cid"), CanonicalId);

		// Instance ID
		int32& Counter = CanonicalCounter.FindOrAdd(CanonicalId, 0);
		NodeObj->SetStringField(TEXT("inst"), FString::Printf(TEXT("%s#%d"), *CanonicalId, Counter));
		Counter++;

		// Fingerprint (for stable matching across recompiles)
		NodeObj->SetStringField(TEXT("fingerprint"), ComputeNodeFingerprint(Node));

		// Position
		TArray<TSharedPtr<FJsonValue>> PosArr;
		PosArr.Add(MakeShareable(new FJsonValueNumber(Node->NodePosX)));
		PosArr.Add(MakeShareable(new FJsonValueNumber(Node->NodePosY)));
		NodeObj->SetArrayField(TEXT("pos"), PosArr);

		// Pins (semantic mode)
		TArray<TSharedPtr<FJsonValue>> PinsArray;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin) continue;
			TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
			PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
			PinObj->SetStringField(TEXT("dir"), Pin->Direction == EGPD_Input ? TEXT("in") : TEXT("out"));

			FString PinType = Pin->PinType.PinCategory.ToString();
			if (Pin->PinType.PinSubCategoryObject.IsValid())
				PinType += TEXT(":") + Pin->PinType.PinSubCategoryObject->GetName();
			PinObj->SetStringField(TEXT("ptype"), PinType);

			if (!Pin->DefaultValue.IsEmpty())
				PinObj->SetStringField(TEXT("val"), Pin->DefaultValue);
			if (Pin->DefaultObject)
			{
				PinObj->SetStringField(TEXT("ref"), Pin->DefaultObject->GetPathName());
				if (UClass* C = Cast<UClass>(Pin->DefaultObject))
					PinObj->SetStringField(TEXT("cls"), C->GetName());
			}

			PinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
		}
		NodeObj->SetArrayField(TEXT("pins"), PinsArray);

		NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObj)));
	}
	GraphObj->SetArrayField(TEXT("nodes"), NodesArray);

	// Edges (built from pin connections)
	TArray<TSharedPtr<FJsonValue>> EdgesArray;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node) continue;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output) continue;
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (!LinkedPin || !LinkedPin->GetOwningNode()) continue;

				TSharedPtr<FJsonObject> EdgeObj = MakeShareable(new FJsonObject);
				EdgeObj->SetStringField(TEXT("from_node"), Node->NodeGuid.ToString());
				EdgeObj->SetStringField(TEXT("from_pin"), Pin->PinName.ToString());
				EdgeObj->SetStringField(TEXT("to_node"), LinkedPin->GetOwningNode()->NodeGuid.ToString());
				EdgeObj->SetStringField(TEXT("to_pin"), LinkedPin->PinName.ToString());

				// Edge type
				bool bIsExec = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec;
				EdgeObj->SetStringField(TEXT("edge_type"), bIsExec ? TEXT("exec") : TEXT("data"));

				if (!bIsExec)
				{
					FString DataType = Pin->PinType.PinCategory.ToString();
					if (Pin->PinType.PinSubCategoryObject.IsValid())
						DataType += TEXT(":") + Pin->PinType.PinSubCategoryObject->GetName();
					EdgeObj->SetStringField(TEXT("data_type"), DataType);
				}

				EdgesArray.Add(MakeShareable(new FJsonValueObject(EdgeObj)));
			}
		}
	}
	GraphObj->SetArrayField(TEXT("edges"), EdgesArray);

	return GraphObj;
}

FString UBlueprintExportCommandlet::ComputeNodeFingerprint(UEdGraphNode* Node)
{
	if (!Node) return TEXT("");

	// Build fingerprint from: canonical_id + pin signature + neighbor canonical_ids
	FString Parts;

	// 1. Node class + canonical identity
	Parts += Node->GetClass()->GetName();
	Parts += TEXT("|");

	// 2. Function reference (for CallFunction nodes)
	if (UK2Node_CallFunction* FN = Cast<UK2Node_CallFunction>(Node))
	{
		UFunction* Func = FN->GetTargetFunction();
		if (Func)
			Parts += FString::Printf(TEXT("%s.%s"), *Func->GetOwnerClass()->GetName(), *Func->GetName());
	}
	else if (UK2Node_CustomEvent* CE = Cast<UK2Node_CustomEvent>(Node))
		Parts += CE->CustomFunctionName.ToString();
	else if (UK2Node_Event* EV = Cast<UK2Node_Event>(Node))
		Parts += EV->EventReference.GetMemberName().ToString();
	Parts += TEXT("|");

	// 3. Pin signature (sorted names + types, excluding transient pins)
	TArray<FString> PinSigs;
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin) continue;
		FString Sig = FString::Printf(TEXT("%s:%s:%s"),
			*Pin->PinName.ToString(),
			Pin->Direction == EGPD_Input ? TEXT("I") : TEXT("O"),
			*Pin->PinType.PinCategory.ToString());
		PinSigs.Add(Sig);
	}
	PinSigs.Sort();
	Parts += FString::Join(PinSigs, TEXT(","));
	Parts += TEXT("|");

	// 4. Immediate neighbor canonical IDs (exec-connected only, for topology context)
	TArray<FString> Neighbors;
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin || Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec) continue;
		for (UEdGraphPin* Linked : Pin->LinkedTo)
		{
			if (!Linked || !Linked->GetOwningNode()) continue;
			Neighbors.Add(Linked->GetOwningNode()->GetClass()->GetName());
		}
	}
	Neighbors.Sort();
	Parts += FString::Join(Neighbors, TEXT(","));

	// Hash it
	uint32 Hash = GetTypeHash(Parts);
	return FString::Printf(TEXT("%08X"), Hash);
}
