// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Blueprint.h"
#include "K2Node.h"
#include "GenBlueprintUtils.generated.h"

/**
 * Utility functions for Blueprint manipulation from AI/LLM commands
 */
UCLASS()
class GENERATIVEAISUPPORTEDITOR_API  UGenBlueprintUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Create a new Blueprint class
	 * 
	 * @param BlueprintName - Name for the new Blueprint
	 * @param ParentClassName - Parent class name or path
	 * @param SavePath - Directory path to save the Blueprint
	 * @return Newly created Blueprint, or nullptr if creation failed
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static UBlueprint* CreateBlueprint(const FString& BlueprintName, const FString& ParentClassName,
	                                   const FString& SavePath);

	/**
	 * Add a component to a Blueprint
	 * 
	 * @param BlueprintPath - Path to the Blueprint asset
	 * @param ComponentClass - Component class to add
	 * @param ComponentName - Name for the new component
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static bool AddComponent(const FString& BlueprintPath, const FString& ComponentClass, const FString& ComponentName);

	/**
	 * Add a variable to a Blueprint
	 * 
	 * @param BlueprintPath - Path to the Blueprint asset
	 * @param VariableName - Name for the new variable
	 * @param VariableType - Type of the variable as a string (e.g., "float", "vector")
	 * @param DefaultValue - Default value for the variable as a string
	 * @param Category - Category for organizing variables
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static bool AddVariable(const FString& BlueprintPath, const FString& VariableName,
	                        const FString& VariableType, const FString& DefaultValue,
	                        const FString& Category);

	/**
	 * Add a function to a Blueprint
	 * 
	 * @param BlueprintPath - Path to the Blueprint asset
	 * @param FunctionName - Name for the new function
	 * @param Inputs - Array of input parameter descriptions (JSON format)
	 * @param Outputs - Array of output parameter descriptions (JSON format)
	 * @return Function GUID as string if successful, empty string if failed
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static FString AddFunction(const FString& BlueprintPath, const FString& FunctionName,
	                           const FString& InputsJson, const FString& OutputsJson);

	/**
	 * Connect nodes in a Blueprint graph
	 * 
	 * @param BlueprintPath - Path to the Blueprint asset
	 * @param FunctionGuid - GUID of the function containing the nodes
	 * @param SourceNodeGuid - GUID of the source node
	 * @param SourcePinName - Name of the source pin
	 * @param TargetNodeGuid - GUID of the target node
	 * @param TargetPinName - Name of the target pin
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static FString ConnectNodes(const FString& BlueprintPath, const FString& FunctionGuid,
	                            const FString& SourceNodeGuid, const FString& SourcePinName,
	                            const FString& TargetNodeGuid, const FString& TargetPinName);

	/**
	 * Compile a Blueprint
	 * 
	 * @param BlueprintPath - Path to the Blueprint asset
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static bool CompileBlueprint(const FString& BlueprintPath);

	/**
	 * Spawn a Blueprint actor in the level
	 * 
	 * @param BlueprintPath - Path to the Blueprint asset
	 * @param Location - Spawn location
	 * @param Rotation - Spawn rotation
	 * @param Scale - Spawn scale
	 * @param ActorLabel - Optional label for the actor
	 * @return Spawned actor, or nullptr if spawn failed
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static AActor* SpawnBlueprint(const FString& BlueprintPath, const FVector& Location,
	                              const FRotator& Rotation, const FVector& Scale,
	                              const FString& ActorLabel);
	

	/**
	 * Connect multiple pairs of nodes in a Blueprint graph in a single operation
	 * 
	 * @param BlueprintPath - Path to the Blueprint asset
	 * @param FunctionGuid - GUID of the function containing the nodes
	 * @param ConnectionsJson - JSON array of connection definitions
	 * @return True if all connections were successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static FString ConnectNodesBulk(const FString& BlueprintPath, const FString& FunctionGuid,
	                                const FString& ConnectionsJson);
	
	static bool OpenBlueprintGraph(UBlueprint* Blueprint, UEdGraph* Graph = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = "GenBlueprintUtils")
	static FString GetNodeGUID(const FString& BlueprintPath, const FString& GraphType, const FString& NodeName, const FString& FunctionGuid);
	
	UFUNCTION(BlueprintCallable, Category = "Blueprint")
	static FString AddComponentWithEvents(const FString& BlueprintPath, const FString& ComponentName,
	                               const FString& ComponentClassName);

	// Undo the last editor transaction (calls GEditor->UndoTransaction)
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static bool UndoTransaction();

	// Begin a named transaction for atomic blueprint operations
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static bool BeginBlueprintTransaction(const FString& TransactionName);

	// End (commit) the current transaction
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static bool EndBlueprintTransaction();

	// Cancel (rollback) the current transaction — undoes all changes since Begin
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static bool CancelBlueprintTransaction();

	// Get all variables defined in a Blueprint (reads NewVariables directly)
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static FString GetBlueprintVariables(const FString& BlueprintPath);

	// Scan all Blueprint assets under /Game and return lightweight metadata
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static FString ScanAllBlueprints();

	// Save all dirty (unsaved) packages in the editor.
	// Returns JSON: {"saved": [...], "failed": [...], "count": N}
	// Call this after any MCP operation that modifies assets to prevent
	// conflicts when git pull/merge runs on open files.
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils")
	static FString SaveAllDirtyPackages();

	/**
	 * Add a new named case to a K2Node_SwitchString (Switch on String) node.
	 *
	 * This is a STRUCTURAL mutation — it modifies the node's dynamic pin layout.
	 * set_editor_property("PinNames") alone does NOT work because ReconstructNode()
	 * is not triggered, leaving exec pins unmaterialized. This function calls
	 * ReconstructNode() explicitly so the new exec output pin is immediately usable
	 * by connect_nodes / apply_blueprint_patch.
	 *
	 * Note: K2Node_SwitchString::AddPinToSwitchNode() auto-generates a pin name and
	 * is intended for the editor "Add Pin" button — it does not accept a specific name.
	 * Therefore this function uses PinNames.AddUnique() + ReconstructNode() directly,
	 * which is the correct path for adding a named case programmatically.
	 *
	 * @param BlueprintPath  Asset path (e.g. "/Game/Blueprints/Core/BP_MyActor")
	 * @param GraphId        "EventGraph", other graph name, or graph GUID string
	 * @param NodeGuid       GUID of the K2Node_SwitchString node
	 * @param CaseName       Case string to add (e.g. "StepOn")
	 * @return JSON: {"success":bool, "case_added":str, "method":str, "pin_count":int}
	 *         method values: "PinNames+ReconstructNode" | "already_exists"
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils|Structural Mutation")
	static FString AddSwitchCase(const FString& BlueprintPath,
	                              const FString& GraphId,
	                              const FString& NodeGuid,
	                              const FString& CaseName);

	/**
	 * Find any node in a Blueprint graph by its UObject FName and return its NodeGuid.
	 *
	 * Solves the "ForEachLoop body node not reachable via exec-chain traversal" problem:
	 * the Python-side instance_id resolver only traverses exec-reachable nodes, so nodes
	 * inside ForEachLoop bodies (K2Node_SwitchString, K2Node_BreakStruct, etc.) cannot be
	 * found by it. This function iterates Graph->Nodes directly, bypassing exec-chain limits.
	 *
	 * Usage from Python:
	 *   fname = node_obj.get_fname()   # e.g. "K2Node_BreakStruct_0"
	 *   result = U.get_node_guid_by_fname(bp, "EventGraph", fname)
	 *   guid = result["node_guid"]     # pass to connect_nodes or add_switch_case
	 *
	 * @param BlueprintPath  Asset path
	 * @param GraphId        "EventGraph", other graph name, or GUID string
	 * @param NodeFName      UObject FName of the target node (from node.get_fname())
	 * @param NodeClassFilter Optional: only match if node class name contains this string
	 * @return JSON: {"success":bool, "node_guid":str, "node_class":str, "node_name":str}
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils|Query")
	static FString GetNodeGuidByFName(const FString& BlueprintPath,
	                                   const FString& GraphId,
	                                   const FString& NodeFName,
	                                   const FString& NodeClassFilter = TEXT(""));

	/**
	 * Connect two Blueprint graph nodes identified by their UObject FNames.
	 *
	 * Solves the GUID instability problem: ConnectNodes uses StaticLoadObject which
	 * may return a different blueprint version than the live in-editor object, causing
	 * GUID mismatches. ConnectNodesByFName performs lookup by FName within the SAME
	 * graph reference, avoiding GUIDs entirely.
	 *
	 * @param BlueprintPath  Asset path
	 * @param GraphId        "EventGraph", other graph name, or GUID string
	 * @param SrcFName       UObject FName of the source node (e.g. "K2Node_SwitchString_0")
	 * @param SrcPin         Output pin name on source node (e.g. "StepOn")
	 * @param TgtFName       UObject FName of the target node
	 * @param TgtPin         Input pin name on target node (e.g. "execute")
	 * @return JSON: {"success":bool, "error":str}
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Blueprint Utils|Structural Mutation")
	static FString ConnectNodesByFName(const FString& BlueprintPath,
	                                    const FString& GraphId,
	                                    const FString& SrcFName,
	                                    const FString& SrcPin,
	                                    const FString& TgtFName,
	                                    const FString& TgtPin);

private:
	// Helper functions for internal use
	static UBlueprint* LoadBlueprintAsset(const FString& BlueprintPath);
	static UClass* FindClassByName(const FString& ClassName);
	static UFunction* FindFunctionByName(UClass* Class, const FString& FunctionName);
};
