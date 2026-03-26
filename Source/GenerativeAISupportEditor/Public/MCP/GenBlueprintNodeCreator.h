// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "K2Node.h"
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
	static FString GetNodeDetails(const FString& BlueprintPath, const FString& FunctionGuid, const FString& NodeGuid);

	// List all graphs (EventGraph, Functions, Macros) in a blueprint
	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static FString ListGraphs(const FString& BlueprintPath);

	// Set the default value of a pin on an existing node
	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static bool SetNodePinValue(const FString& BlueprintPath, const FString& FunctionGuid,
	                            const FString& NodeGuid, const FString& PinName, const FString& NewValue);

	// Break a specific connection between two node pins
	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static bool DisconnectNodes(const FString& BlueprintPath, const FString& FunctionGuid,
	                            const FString& SourceNodeGuid, const FString& SourcePinName,
	                            const FString& TargetNodeGuid, const FString& TargetPinName);

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

};
