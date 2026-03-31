// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "BlueprintExportCommandlet.generated.h"

/**
 * Headless commandlet for exporting Blueprint graph semantic IR.
 *
 * Exports one or more Blueprint assets to a structured JSON IR that can be
 * used for three-way diff/merge without opening the UE Editor GUI.
 *
 * Usage:
 *   UnrealEditor-Cmd <Project> -run=BlueprintExport -blueprint=/Game/Path/BP_Name [-output=output.json] [-all]
 *
 * Parameters:
 *   -blueprint=<path>   Asset path of the Blueprint to export (repeatable)
 *   -output=<file>      Output file path (default: stdout)
 *   -all                Export all Blueprint assets under /Game
 *   -compact            Use compact JSON (no indent)
 */
UCLASS()
class GENERATIVEAISUPPORTEDITOR_API UBlueprintExportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UBlueprintExportCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	// Export a single Blueprint to JSON IR
	TSharedPtr<FJsonObject> ExportBlueprintIR(UBlueprint* Blueprint);

	// Export a single graph within a Blueprint
	TSharedPtr<FJsonObject> ExportGraphIR(UBlueprint* Blueprint, UEdGraph* Graph);

	// Build a node fingerprint for stable matching across recompiles
	FString ComputeNodeFingerprint(UEdGraphNode* Node);
};
