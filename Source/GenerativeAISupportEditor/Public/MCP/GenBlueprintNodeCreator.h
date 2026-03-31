// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "K2Node.h"
#include "K2Node_CallFunction.h"
#include "GenBlueprintNodeCreator.generated.h"

/**
 * 
 */
UCLASS()
class GENERATIVEAISUPPORTEDITOR_API  UGenBlueprintNodeCreator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	// Adds a single node to a Blueprint function graph
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Nodes")
	static FString AddNode(const FString& BlueprintPath, const FString& FunctionGuid, const FString& NodeType,
						   float NodeX, float NodeY, const FString& PropertiesJson, bool bFinalizeChanges = true);

	// Adds multiple nodes to a Blueprint function graph in bulk
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Nodes")
	static FString AddNodesBulk(const FString& BlueprintPath, const FString& FunctionGuid, const FString& NodesJson);

	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static bool DeleteNode(const FString& BlueprintPath, const FString& FunctionGuid, const FString& NodeGuid);

	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static FString GetAllNodesInGraph(const FString& BlueprintPath, const FString& FunctionGuid);
	
	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static UEdGraph* FindGraphByGuid(UBlueprint* Blueprint, const FGuid& GraphGuid);
	
	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static FString GetNodeSuggestions(const FString& NodeType);

	// Move a node to a new position in the graph
	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static bool MoveNode(const FString& BlueprintPath, const FString& FunctionGuid, const FString& NodeGuid, float NewX, float NewY);

	// Get detailed info about a node: pins, connections, default values, title
	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static FString GetNodeDetails(const FString& BlueprintPath, const FString& FunctionGuid,
	                              const FString& NodeGuid, const FString& SchemaMode = TEXT("verbose"));

	// List all graphs (EventGraph, Functions, Macros) in a blueprint
	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static FString ListGraphs(const FString& BlueprintPath);

	// Get all nodes with full details (pins, connections, titles) in one call
	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static FString GetAllNodesWithDetails(const FString& BlueprintPath, const FString& FunctionGuid);

	// Set the default value of a pin on an existing node
	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static bool SetNodePinValue(const FString& BlueprintPath, const FString& FunctionGuid,
	                            const FString& NodeGuid, const FString& PinName, const FString& NewValue);

	// Break a specific connection between two node pins
	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static bool DisconnectNodes(const FString& BlueprintPath, const FString& FunctionGuid,
	                            const FString& SourceNodeGuid, const FString& SourcePinName,
	                            const FString& TargetNodeGuid, const FString& TargetPinName);

	// Preflight: validate a patch without mutating the graph.
	// Returns JSON with {valid, issues[], predicted_nodes[{ref_id, resolved_type, pins[]}]}
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Nodes")
	static FString PreflightPatch(const FString& BlueprintPath, const FString& FunctionGuid,
	                              const FString& PatchJson);

	// --- Unified Callable Function Resolver ---

	struct FResolvedFunction
	{
		UFunction* Function = nullptr;
		UClass* OwnerClass = nullptr;
		FString CanonicalName;       // Normalized: "RPNPCClient.SendSingleStimulus"
		FString ContextSource;       // "direct_lookup" | "engine_lib" | "bp_class" | "component_reflected" | "component_scs"
		bool bResolved = false;
	};

	// Unified resolver for callable functions (spawn_strategy = call_function ONLY).
	// Does NOT handle known types (Branch, Sequence), events, or variables.
	// Search order: Class.Function direct → common libraries → BP class hierarchy → reflected components → SCS components
	static FResolvedFunction ResolveCallableFunction(const FString& FunctionName, UBlueprint* Blueprint = nullptr);

	// --- Node Discovery (B1-B4) ---

	enum class ENodeKind : uint8
	{
		Function,
		Event,
		Macro,
		VariableGet,
		VariableSet,
		Special,
	};

	struct FNodeIndexEntry
	{
		FString CanonicalName;
		FString DisplayName;
		FString Category;
		FString Keywords;
		FString ToolTip;
		UFunction* Function = nullptr;
		UClass* OwnerClass = nullptr;
		ENodeKind Kind = ENodeKind::Function;
		bool bLatent = false;
		bool bPure = false;
		bool bWorldContext = false;
		bool bDeprecated = false;
	};

	// Build the full engine node index. Returns count of indexed entries.
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Nodes")
	static int32 BuildNodeIndex();

	// Check if index has been built
	static bool IsNodeIndexBuilt();

	// Clear the index and force a rebuild on next search (call after hot-reload or Blueprint changes)
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Nodes")
	static void InvalidateNodeIndex();

	// Search all indexed nodes. Returns lightweight shortlist JSON.
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Nodes")
	static FString SearchBlueprintNodes(const FString& Query, const FString& BlueprintPath,
	                                    const FString& CategoryFilter, int32 MaxResults,
	                                    const FString& SchemaMode = TEXT("verbose"));

	// Inspect a specific node by canonical name. Returns full pin schema + patch_hint JSON.
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Nodes")
	static FString InspectBlueprintNode(const FString& CanonicalName, const FString& BlueprintPath,
	                                    const FString& SchemaMode = TEXT("verbose"));

	// --- Shared matching infrastructure (R2) ---

	// Match type tiers (higher = better match). Controls both ranking and auto-bind decisions.
	enum class EMatchType : uint8
	{
		None = 0,
		Substring,        // plain substring, query >= 8 chars
		WordBoundary,     // substring at word boundary, query >= 6 chars
		Prefix,           // candidate starts with query
		NormalizedToken,  // case/underscore normalized exact
		Exact,            // case-insensitive exact
	};

	struct FMatchResult
	{
		int32 Score = 0;
		EMatchType Type = EMatchType::None;
		bool bAccepted = false;   // Whether to auto-bind (separate from sort order)
	};

	// Score a single query against a candidate function name.
	// Sort order and acceptance threshold are separate — see EMatchType tiers.
	static FMatchResult ScoreFunctionMatch(const FString& Query, const FString& Candidate);

	// Shared library list used by BindFunctionByName, TryCreateNodeFromLibraries, and GetNodeSuggestions
	static const TArray<FString>& GetCommonLibraries();

	// Split "SetActorLocation" → {"Set","Actor","Location"} for token matching
	static TArray<FString> SplitCamelCase(const FString& Name);

private:
	// Static map of friendly node names to Unreal Engine node types
	static TMap<FString, FString> NodeTypeMap;

	// Function to initialize the map (called once)
	static void InitNodeTypeMap();
	// Attempts to create a node of a known type
	static bool TryCreateKnownNodeType(UEdGraph* Graph, const FString& NodeType, UK2Node*& OutNode,
									   const FString& PropertiesJson = TEXT(""));
	static UEdGraph* GetGraphFromFunctionId(UBlueprint* Blueprint, const FString& FunctionGuid);

	// Attempts to create a node by searching Blueprint libraries and actor classes
	static FString TryCreateNodeFromLibraries(UEdGraph* Graph, const FString& NodeType, UK2Node*& OutNode,
	                                          TArray<FString>& OutSuggestions);

	// Helper to create function call nodes
	static bool CreateMathFunctionNode(UEdGraph* Graph, const FString& ClassName, const FString& FunctionName,
									   UK2Node*& OutNode);

	// Bind a function by name to a K2Node_CallFunction by searching common libraries
	static bool BindFunctionByName(UK2Node_CallFunction* FuncNode, const FString& FunctionName);

};
