// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/DataTable.h"
#include "GenDataTableUtils.generated.h"

/**
 * Editor utilities for DataTables and CurveTables
 * Allows reading, modifying, and creating data assets via Python/MCP
 */
UCLASS()
class GENERATIVEAISUPPORTEDITOR_API UGenDataTableUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// DATATABLE DISCOVERY
	// ============================================

	/**
	 * List all DataTables in a directory.
	 * @param DirectoryPath - Directory to search (default /Game)
	 * @return JSON array of DataTables with row struct info
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString ListDataTables(const FString& DirectoryPath = TEXT("/Game"));

	/**
	 * Get DataTable structure info (row struct, columns).
	 * @param DataTablePath - Path to the DataTable asset
	 * @return JSON with table structure
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString GetDataTableInfo(const FString& DataTablePath);

	/**
	 * Get all rows from a DataTable as JSON.
	 * @param DataTablePath - Path to the DataTable asset
	 * @return JSON array of all rows
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString GetAllRows(const FString& DataTablePath);

	/**
	 * Get a specific row from a DataTable.
	 * @param DataTablePath - Path to the DataTable asset
	 * @param RowName - Name of the row
	 * @return JSON with row data
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString GetRow(const FString& DataTablePath, const FString& RowName);

	/**
	 * Search rows by column value.
	 * @param DataTablePath - Path to the DataTable asset
	 * @param ColumnName - Column to search
	 * @param SearchValue - Value to match (partial match)
	 * @return JSON array of matching rows
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString SearchRows(const FString& DataTablePath, const FString& ColumnName, const FString& SearchValue);

	// ============================================
	// ROW MANIPULATION
	// ============================================

	/**
	 * Add a new row to a DataTable.
	 * @param DataTablePath - Path to the DataTable asset
	 * @param RowName - Name for the new row
	 * @param RowDataJson - JSON object with column values
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString AddRow(const FString& DataTablePath, const FString& RowName, const FString& RowDataJson);

	/**
	 * Update an existing row in a DataTable.
	 * @param DataTablePath - Path to the DataTable asset
	 * @param RowName - Name of the row to update
	 * @param RowDataJson - JSON object with column values to update
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString UpdateRow(const FString& DataTablePath, const FString& RowName, const FString& RowDataJson);

	/**
	 * Delete a row from a DataTable.
	 * @param DataTablePath - Path to the DataTable asset
	 * @param RowName - Name of the row to delete
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString DeleteRow(const FString& DataTablePath, const FString& RowName);

	/**
	 * Rename a row in a DataTable.
	 * @param DataTablePath - Path to the DataTable asset
	 * @param OldRowName - Current row name
	 * @param NewRowName - New row name
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString RenameRow(const FString& DataTablePath, const FString& OldRowName, const FString& NewRowName);

	// ============================================
	// DATATABLE CREATION
	// ============================================

	/**
	 * Create a new DataTable asset.
	 * @param DataTablePath - Path for the new DataTable
	 * @param RowStructPath - Path to the row struct (e.g., /Script/MyGame.FMyRowStruct)
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString CreateDataTable(const FString& DataTablePath, const FString& RowStructPath);

	/**
	 * Duplicate a DataTable.
	 * @param SourcePath - Path to source DataTable
	 * @param DestPath - Path for new DataTable
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString DuplicateDataTable(const FString& SourcePath, const FString& DestPath);

	/**
	 * Delete a DataTable asset.
	 * @param DataTablePath - Path to the DataTable
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString DeleteDataTable(const FString& DataTablePath);

	// ============================================
	// IMPORT/EXPORT
	// ============================================

	/**
	 * Export DataTable to JSON file.
	 * @param DataTablePath - Path to the DataTable asset
	 * @param OutputFilePath - File path for JSON output
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString ExportToJson(const FString& DataTablePath, const FString& OutputFilePath);

	/**
	 * Export DataTable to CSV file.
	 * @param DataTablePath - Path to the DataTable asset
	 * @param OutputFilePath - File path for CSV output
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString ExportToCsv(const FString& DataTablePath, const FString& OutputFilePath);

	/**
	 * Import rows from JSON file.
	 * @param DataTablePath - Path to the DataTable asset
	 * @param InputFilePath - Path to JSON file
	 * @param bClearExisting - Clear existing rows before import
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString ImportFromJson(const FString& DataTablePath, const FString& InputFilePath, bool bClearExisting = false);

	/**
	 * Import rows from CSV file.
	 * @param DataTablePath - Path to the DataTable asset
	 * @param InputFilePath - Path to CSV file
	 * @param bClearExisting - Clear existing rows before import
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString ImportFromCsv(const FString& DataTablePath, const FString& InputFilePath, bool bClearExisting = false);

	// ============================================
	// ROW STRUCT UTILITIES
	// ============================================

	/**
	 * List available row structs.
	 * @param FilterPattern - Optional pattern to filter struct names
	 * @return JSON array of struct paths
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString ListRowStructs(const FString& FilterPattern = TEXT(""));

	/**
	 * Get properties of a row struct.
	 * @param StructPath - Path to the struct
	 * @return JSON with struct properties
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString GetStructProperties(const FString& StructPath);

	// ============================================
	// CURVE TABLES
	// ============================================

	/**
	 * List all CurveTables in a directory.
	 * @param DirectoryPath - Directory to search
	 * @return JSON array of CurveTables
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString ListCurveTables(const FString& DirectoryPath = TEXT("/Game"));

	/**
	 * Get curve data from a CurveTable.
	 * @param CurveTablePath - Path to the CurveTable
	 * @param RowName - Name of the curve row
	 * @return JSON with curve points
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString GetCurveData(const FString& CurveTablePath, const FString& RowName);

	/**
	 * Set curve data in a CurveTable.
	 * @param CurveTablePath - Path to the CurveTable
	 * @param RowName - Name of the curve row
	 * @param PointsJson - JSON array of {x, y} points
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString SetCurveData(const FString& CurveTablePath, const FString& RowName, const FString& PointsJson);

	// ============================================
	// COMPOSITE TABLES
	// ============================================

	/**
	 * List all CompositeDataTables.
	 * @param DirectoryPath - Directory to search
	 * @return JSON array
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString ListCompositeTables(const FString& DirectoryPath = TEXT("/Game"));

	/**
	 * Get parent tables of a CompositeDataTable.
	 * @param CompositeTablePath - Path to composite table
	 * @return JSON array of parent table paths
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|DataTable Utils")
	static FString GetParentTables(const FString& CompositeTablePath);

private:
	/** Load DataTable asset */
	static UDataTable* LoadDataTableAsset(const FString& DataTablePath);

	/** Convert row to JSON object */
	static TSharedPtr<FJsonObject> RowToJson(UDataTable* DataTable, const FName& RowName);

	/** Convert JSON to row data */
	static bool JsonToRow(UDataTable* DataTable, const FName& RowName, const TSharedPtr<FJsonObject>& JsonObject);

	/** Get column names from row struct */
	static TArray<FString> GetColumnNames(UDataTable* DataTable);
};
