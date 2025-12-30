// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#include "MCP/GenDataTableUtils.h"
#include "Engine/DataTable.h"
#include "Engine/CurveTable.h"
#include "Engine/CompositeDataTable.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/DataTableFactory.h"
#include "EditorAssetLibrary.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "DataTableEditorUtils.h"
#include "UObject/UnrealType.h"
#include "Curves/RichCurve.h"

UDataTable* UGenDataTableUtils::LoadDataTableAsset(const FString& DataTablePath)
{
	return LoadObject<UDataTable>(nullptr, *DataTablePath);
}

TArray<FString> UGenDataTableUtils::GetColumnNames(UDataTable* DataTable)
{
	TArray<FString> Columns;
	if (!DataTable) return Columns;

	const UScriptStruct* RowStruct = DataTable->GetRowStruct();
	if (!RowStruct) return Columns;

	for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
	{
		Columns.Add(It->GetName());
	}

	return Columns;
}

TSharedPtr<FJsonObject> UGenDataTableUtils::RowToJson(UDataTable* DataTable, const FName& RowName)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	if (!DataTable) return JsonObject;

	const UScriptStruct* RowStruct = DataTable->GetRowStruct();
	if (!RowStruct) return JsonObject;

	void* RowData = DataTable->FindRowUnchecked(RowName);
	if (!RowData) return JsonObject;

	JsonObject->SetStringField("RowName", RowName.ToString());

	for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
	{
		FProperty* Property = *It;
		const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(RowData);

		FString ValueString;
		Property->ExportTextItem_Direct(ValueString, ValuePtr, nullptr, nullptr, PPF_None);
		JsonObject->SetStringField(Property->GetName(), ValueString);
	}

	return JsonObject;
}

bool UGenDataTableUtils::JsonToRow(UDataTable* DataTable, const FName& RowName, const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!DataTable || !JsonObject.IsValid()) return false;

	const UScriptStruct* RowStruct = DataTable->GetRowStruct();
	if (!RowStruct) return false;

	void* RowData = DataTable->FindRowUnchecked(RowName);
	if (!RowData) return false;

	for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
	{
		FProperty* Property = *It;
		FString PropertyName = Property->GetName();

		if (JsonObject->HasField(PropertyName))
		{
			FString ValueString = JsonObject->GetStringField(PropertyName);
			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(RowData);
			Property->ImportText_Direct(*ValueString, ValuePtr, nullptr, PPF_None);
		}
	}

	return true;
}

FString UGenDataTableUtils::ListDataTables(const FString& DirectoryPath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByPath(FName(*DirectoryPath), AssetList, true);

	TArray<TSharedPtr<FJsonValue>> TablesArray;

	for (const FAssetData& Asset : AssetList)
	{
		if (Asset.AssetClassPath == UDataTable::StaticClass()->GetClassPathName() ||
			Asset.AssetClassPath == UCompositeDataTable::StaticClass()->GetClassPathName())
		{
			TSharedPtr<FJsonObject> TableJson = MakeShareable(new FJsonObject);
			TableJson->SetStringField("name", Asset.AssetName.ToString());
			TableJson->SetStringField("path", Asset.GetObjectPathString());

			// Get row struct name if possible
			FAssetTagValueRef RowStructTag = Asset.TagsAndValues.FindTag(FName("RowStructure"));
			if (RowStructTag.IsSet())
			{
				TableJson->SetStringField("row_struct", RowStructTag.GetValue());
			}

			TablesArray.Add(MakeShareable(new FJsonValueObject(TableJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", TablesArray.Num());
	ResultJson->SetArrayField("datatables", TablesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDataTableUtils::GetDataTableInfo(const FString& DataTablePath)
{
	UDataTable* DataTable = LoadDataTableAsset(DataTablePath);
	if (!DataTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"DataTable not found at '%s'\"}"), *DataTablePath);
	}

	const UScriptStruct* RowStruct = DataTable->GetRowStruct();

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetStringField("path", DataTablePath);
	ResultJson->SetStringField("row_struct", RowStruct ? RowStruct->GetName() : TEXT("Unknown"));
	ResultJson->SetNumberField("row_count", DataTable->GetRowNames().Num());

	// Columns
	TArray<TSharedPtr<FJsonValue>> ColumnsArray;
	if (RowStruct)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			TSharedPtr<FJsonObject> ColJson = MakeShareable(new FJsonObject);
			ColJson->SetStringField("name", It->GetName());
			ColJson->SetStringField("type", It->GetCPPType());

			ColumnsArray.Add(MakeShareable(new FJsonValueObject(ColJson)));
		}
	}
	ResultJson->SetArrayField("columns", ColumnsArray);

	// Row names
	TArray<TSharedPtr<FJsonValue>> RowNamesArray;
	for (const FName& RowName : DataTable->GetRowNames())
	{
		RowNamesArray.Add(MakeShareable(new FJsonValueString(RowName.ToString())));
	}
	ResultJson->SetArrayField("row_names", RowNamesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDataTableUtils::GetAllRows(const FString& DataTablePath)
{
	UDataTable* DataTable = LoadDataTableAsset(DataTablePath);
	if (!DataTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"DataTable not found at '%s'\"}"), *DataTablePath);
	}

	TArray<TSharedPtr<FJsonValue>> RowsArray;

	for (const FName& RowName : DataTable->GetRowNames())
	{
		TSharedPtr<FJsonObject> RowJson = RowToJson(DataTable, RowName);
		RowsArray.Add(MakeShareable(new FJsonValueObject(RowJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", RowsArray.Num());
	ResultJson->SetArrayField("rows", RowsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDataTableUtils::GetRow(const FString& DataTablePath, const FString& RowName)
{
	UDataTable* DataTable = LoadDataTableAsset(DataTablePath);
	if (!DataTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"DataTable not found at '%s'\"}"), *DataTablePath);
	}

	FName RowFName(*RowName);
	if (!DataTable->FindRowUnchecked(RowFName))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Row '%s' not found\"}"), *RowName);
	}

	TSharedPtr<FJsonObject> RowJson = RowToJson(DataTable, RowFName);

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetObjectField("row", RowJson);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDataTableUtils::SearchRows(const FString& DataTablePath, const FString& ColumnName, const FString& SearchValue)
{
	UDataTable* DataTable = LoadDataTableAsset(DataTablePath);
	if (!DataTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"DataTable not found at '%s'\"}"), *DataTablePath);
	}

	const UScriptStruct* RowStruct = DataTable->GetRowStruct();
	if (!RowStruct)
	{
		return TEXT("{\"success\": false, \"error\": \"Row struct not found\"}");
	}

	FProperty* SearchProperty = RowStruct->FindPropertyByName(FName(*ColumnName));
	if (!SearchProperty)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Column '%s' not found\"}"), *ColumnName);
	}

	TArray<TSharedPtr<FJsonValue>> MatchingRows;

	for (const FName& RowName : DataTable->GetRowNames())
	{
		void* RowData = DataTable->FindRowUnchecked(RowName);
		if (!RowData) continue;

		const void* ValuePtr = SearchProperty->ContainerPtrToValuePtr<void>(RowData);
		FString ValueString;
		SearchProperty->ExportTextItem_Direct(ValueString, ValuePtr, nullptr, nullptr, PPF_None);

		if (ValueString.Contains(SearchValue))
		{
			TSharedPtr<FJsonObject> RowJson = RowToJson(DataTable, RowName);
			MatchingRows.Add(MakeShareable(new FJsonValueObject(RowJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", MatchingRows.Num());
	ResultJson->SetArrayField("rows", MatchingRows);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDataTableUtils::AddRow(const FString& DataTablePath, const FString& RowName, const FString& RowDataJson)
{
	UDataTable* DataTable = LoadDataTableAsset(DataTablePath);
	if (!DataTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"DataTable not found at '%s'\"}"), *DataTablePath);
	}

	FName RowFName(*RowName);
	if (DataTable->FindRowUnchecked(RowFName))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Row '%s' already exists\"}"), *RowName);
	}

	const UScriptStruct* RowStruct = DataTable->GetRowStruct();
	if (!RowStruct)
	{
		return TEXT("{\"success\": false, \"error\": \"Row struct not found\"}");
	}

	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RowDataJson);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return TEXT("{\"success\": false, \"error\": \"Invalid JSON data\"}");
	}

	// Add empty row
	DataTable->AddRow(RowFName, *RowStruct->GetDefaultObject<UScriptStruct>());

	// Populate from JSON
	JsonToRow(DataTable, RowFName, JsonObject);

	DataTable->MarkPackageDirty();
	UEditorAssetLibrary::SaveAsset(DataTablePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Row '%s' added\"}"), *RowName);
}

FString UGenDataTableUtils::UpdateRow(const FString& DataTablePath, const FString& RowName, const FString& RowDataJson)
{
	UDataTable* DataTable = LoadDataTableAsset(DataTablePath);
	if (!DataTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"DataTable not found at '%s'\"}"), *DataTablePath);
	}

	FName RowFName(*RowName);
	if (!DataTable->FindRowUnchecked(RowFName))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Row '%s' not found\"}"), *RowName);
	}

	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RowDataJson);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return TEXT("{\"success\": false, \"error\": \"Invalid JSON data\"}");
	}

	JsonToRow(DataTable, RowFName, JsonObject);

	DataTable->MarkPackageDirty();
	UEditorAssetLibrary::SaveAsset(DataTablePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Row '%s' updated\"}"), *RowName);
}

FString UGenDataTableUtils::DeleteRow(const FString& DataTablePath, const FString& RowName)
{
	UDataTable* DataTable = LoadDataTableAsset(DataTablePath);
	if (!DataTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"DataTable not found at '%s'\"}"), *DataTablePath);
	}

	FName RowFName(*RowName);
	if (!DataTable->FindRowUnchecked(RowFName))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Row '%s' not found\"}"), *RowName);
	}

	DataTable->RemoveRow(RowFName);
	DataTable->MarkPackageDirty();
	UEditorAssetLibrary::SaveAsset(DataTablePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Row '%s' deleted\"}"), *RowName);
}

FString UGenDataTableUtils::RenameRow(const FString& DataTablePath, const FString& OldRowName, const FString& NewRowName)
{
	UDataTable* DataTable = LoadDataTableAsset(DataTablePath);
	if (!DataTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"DataTable not found at '%s'\"}"), *DataTablePath);
	}

	FName OldFName(*OldRowName);
	FName NewFName(*NewRowName);

	if (!DataTable->FindRowUnchecked(OldFName))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Row '%s' not found\"}"), *OldRowName);
	}

	if (DataTable->FindRowUnchecked(NewFName))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Row '%s' already exists\"}"), *NewRowName);
	}

	FDataTableEditorUtils::RenameRow(DataTable, OldFName, NewFName);
	DataTable->MarkPackageDirty();
	UEditorAssetLibrary::SaveAsset(DataTablePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Row renamed from '%s' to '%s'\"}"), *OldRowName, *NewRowName);
}

FString UGenDataTableUtils::CreateDataTable(const FString& DataTablePath, const FString& RowStructPath)
{
	if (UEditorAssetLibrary::DoesAssetExist(DataTablePath))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Asset already exists at '%s'\"}"), *DataTablePath);
	}

	UScriptStruct* RowStruct = LoadObject<UScriptStruct>(nullptr, *RowStructPath);
	if (!RowStruct)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Row struct not found at '%s'\"}"), *RowStructPath);
	}

	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FString PackagePath = FPackageName::GetLongPackagePath(DataTablePath);
	FString AssetName = FPackageName::GetShortName(DataTablePath);

	UDataTableFactory* Factory = NewObject<UDataTableFactory>();
	Factory->Struct = RowStruct;

	UObject* NewAsset = AssetTools.CreateAsset(AssetName, PackagePath, UDataTable::StaticClass(), Factory);
	if (!NewAsset)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to create DataTable\"}");
	}

	UEditorAssetLibrary::SaveAsset(DataTablePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"DataTable created at '%s'\"}"), *DataTablePath);
}

FString UGenDataTableUtils::DuplicateDataTable(const FString& SourcePath, const FString& DestPath)
{
	if (!UEditorAssetLibrary::DoesAssetExist(SourcePath))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Source not found at '%s'\"}"), *SourcePath);
	}

	if (UEditorAssetLibrary::DoesAssetExist(DestPath))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Destination already exists at '%s'\"}"), *DestPath);
	}

	UObject* DuplicatedAsset = UEditorAssetLibrary::DuplicateAsset(SourcePath, DestPath);
	if (!DuplicatedAsset)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to duplicate\"}");
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"DataTable duplicated to '%s'\"}"), *DestPath);
}

FString UGenDataTableUtils::DeleteDataTable(const FString& DataTablePath)
{
	if (!UEditorAssetLibrary::DoesAssetExist(DataTablePath))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"DataTable not found at '%s'\"}"), *DataTablePath);
	}

	bool bDeleted = UEditorAssetLibrary::DeleteAsset(DataTablePath);
	if (bDeleted)
	{
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"DataTable deleted: '%s'\"}"), *DataTablePath);
	}

	return TEXT("{\"success\": false, \"error\": \"Failed to delete\"}");
}

FString UGenDataTableUtils::ExportToJson(const FString& DataTablePath, const FString& OutputFilePath)
{
	UDataTable* DataTable = LoadDataTableAsset(DataTablePath);
	if (!DataTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"DataTable not found at '%s'\"}"), *DataTablePath);
	}

	FString JsonContent = GetAllRows(DataTablePath);

	if (FFileHelper::SaveStringToFile(JsonContent, *OutputFilePath))
	{
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Exported to '%s'\"}"), *OutputFilePath);
	}

	return TEXT("{\"success\": false, \"error\": \"Failed to write file\"}");
}

FString UGenDataTableUtils::ExportToCsv(const FString& DataTablePath, const FString& OutputFilePath)
{
	UDataTable* DataTable = LoadDataTableAsset(DataTablePath);
	if (!DataTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"DataTable not found at '%s'\"}"), *DataTablePath);
	}

	FString CsvContent = DataTable->GetTableAsCSV();

	if (FFileHelper::SaveStringToFile(CsvContent, *OutputFilePath))
	{
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Exported to '%s'\"}"), *OutputFilePath);
	}

	return TEXT("{\"success\": false, \"error\": \"Failed to write file\"}");
}

FString UGenDataTableUtils::ImportFromJson(const FString& DataTablePath, const FString& InputFilePath, bool bClearExisting)
{
	// Simplified implementation
	return TEXT("{\"success\": false, \"error\": \"JSON import not yet fully implemented\"}");
}

FString UGenDataTableUtils::ImportFromCsv(const FString& DataTablePath, const FString& InputFilePath, bool bClearExisting)
{
	UDataTable* DataTable = LoadDataTableAsset(DataTablePath);
	if (!DataTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"DataTable not found at '%s'\"}"), *DataTablePath);
	}

	FString CsvContent;
	if (!FFileHelper::LoadFileToString(CsvContent, *InputFilePath))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Failed to read file '%s'\"}"), *InputFilePath);
	}

	if (bClearExisting)
	{
		DataTable->EmptyTable();
	}

	TArray<FString> Errors = DataTable->CreateTableFromCSVString(CsvContent);

	DataTable->MarkPackageDirty();
	UEditorAssetLibrary::SaveAsset(DataTablePath, false);

	if (Errors.Num() > 0)
	{
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Imported with %d errors\", \"errors\": \"%s\"}"),
			Errors.Num(), *FString::Join(Errors, TEXT("; ")));
	}

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"CSV imported to '%s'\"}"), *DataTablePath);
}

FString UGenDataTableUtils::ListRowStructs(const FString& FilterPattern)
{
	TArray<TSharedPtr<FJsonValue>> StructsArray;

	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		UScriptStruct* Struct = *It;
		if (!Struct) continue;

		// Check if it's a valid row struct (has TableRowBase in hierarchy)
		bool bIsRowStruct = false;
		const UScriptStruct* Parent = Struct;
		while (Parent)
		{
			if (Parent->GetName() == TEXT("TableRowBase"))
			{
				bIsRowStruct = true;
				break;
			}
			Parent = Cast<UScriptStruct>(Parent->GetSuperStruct());
		}

		if (!bIsRowStruct) continue;

		FString StructName = Struct->GetName();
		if (!FilterPattern.IsEmpty() && !StructName.Contains(FilterPattern)) continue;

		TSharedPtr<FJsonObject> StructJson = MakeShareable(new FJsonObject);
		StructJson->SetStringField("name", StructName);
		StructJson->SetStringField("path", Struct->GetPathName());

		StructsArray.Add(MakeShareable(new FJsonValueObject(StructJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", StructsArray.Num());
	ResultJson->SetArrayField("structs", StructsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDataTableUtils::GetStructProperties(const FString& StructPath)
{
	UScriptStruct* Struct = LoadObject<UScriptStruct>(nullptr, *StructPath);
	if (!Struct)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Struct not found at '%s'\"}"), *StructPath);
	}

	TArray<TSharedPtr<FJsonValue>> PropsArray;

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		TSharedPtr<FJsonObject> PropJson = MakeShareable(new FJsonObject);
		PropJson->SetStringField("name", It->GetName());
		PropJson->SetStringField("type", It->GetCPPType());

		PropsArray.Add(MakeShareable(new FJsonValueObject(PropJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetStringField("struct_name", Struct->GetName());
	ResultJson->SetArrayField("properties", PropsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDataTableUtils::ListCurveTables(const FString& DirectoryPath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByPath(FName(*DirectoryPath), AssetList, true);

	TArray<TSharedPtr<FJsonValue>> TablesArray;

	for (const FAssetData& Asset : AssetList)
	{
		if (Asset.AssetClassPath == UCurveTable::StaticClass()->GetClassPathName())
		{
			TSharedPtr<FJsonObject> TableJson = MakeShareable(new FJsonObject);
			TableJson->SetStringField("name", Asset.AssetName.ToString());
			TableJson->SetStringField("path", Asset.GetObjectPathString());

			TablesArray.Add(MakeShareable(new FJsonValueObject(TableJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", TablesArray.Num());
	ResultJson->SetArrayField("curvetables", TablesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDataTableUtils::GetCurveData(const FString& CurveTablePath, const FString& RowName)
{
	UCurveTable* CurveTable = LoadObject<UCurveTable>(nullptr, *CurveTablePath);
	if (!CurveTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"CurveTable not found at '%s'\"}"), *CurveTablePath);
	}

	FRealCurve* Curve = CurveTable->FindCurve(FName(*RowName), TEXT(""));
	if (!Curve)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Curve '%s' not found\"}"), *RowName);
	}

	TArray<TSharedPtr<FJsonValue>> PointsArray;

	for (auto It = Curve->GetKeyIterator(); It; ++It)
	{
		TSharedPtr<FJsonObject> PointJson = MakeShareable(new FJsonObject);
		PointJson->SetNumberField("x", It->Time);
		PointJson->SetNumberField("y", It->Value);

		PointsArray.Add(MakeShareable(new FJsonValueObject(PointJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetStringField("curve_name", RowName);
	ResultJson->SetNumberField("point_count", PointsArray.Num());
	ResultJson->SetArrayField("points", PointsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDataTableUtils::SetCurveData(const FString& CurveTablePath, const FString& RowName, const FString& PointsJson)
{
	// Simplified implementation
	return TEXT("{\"success\": false, \"error\": \"SetCurveData not yet implemented\"}");
}

FString UGenDataTableUtils::ListCompositeTables(const FString& DirectoryPath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByPath(FName(*DirectoryPath), AssetList, true);

	TArray<TSharedPtr<FJsonValue>> TablesArray;

	for (const FAssetData& Asset : AssetList)
	{
		if (Asset.AssetClassPath == UCompositeDataTable::StaticClass()->GetClassPathName())
		{
			TSharedPtr<FJsonObject> TableJson = MakeShareable(new FJsonObject);
			TableJson->SetStringField("name", Asset.AssetName.ToString());
			TableJson->SetStringField("path", Asset.GetObjectPathString());

			TablesArray.Add(MakeShareable(new FJsonValueObject(TableJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", TablesArray.Num());
	ResultJson->SetArrayField("composite_tables", TablesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDataTableUtils::GetParentTables(const FString& CompositeTablePath)
{
	UCompositeDataTable* CompositeTable = LoadObject<UCompositeDataTable>(nullptr, *CompositeTablePath);
	if (!CompositeTable)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"CompositeDataTable not found at '%s'\"}"), *CompositeTablePath);
	}

	TArray<TSharedPtr<FJsonValue>> ParentsArray;
	for (const TSoftObjectPtr<UDataTable>& Parent : CompositeTable->ParentTables)
	{
		if (Parent.IsValid())
		{
			ParentsArray.Add(MakeShareable(new FJsonValueString(Parent.ToString())));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", ParentsArray.Num());
	ResultJson->SetArrayField("parent_tables", ParentsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}
