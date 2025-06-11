// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#include "MCP/GenWidgetUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/PanelWidget.h"
#include "EditorAssetLibrary.h"
#include "Engine/UserDefinedStruct.h" // Required for struct properties
#include "JsonObjectConverter.h"	  // For JSON responses
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/SavePackage.h"

// Include specific widget headers if needed for property access, though reflection should handle most cases
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "WidgetBlueprint.h"

// Helper to find widget by name recursively (WidgetTree::FindWidget is often sufficient)
UWidget* UGenWidgetUtils::FindWidgetByName(UWidgetTree* WidgetTree, const FName& Name)
{
	if (!WidgetTree)
		return nullptr;

	UWidget* FoundWidget = WidgetTree->FindWidget(Name);
	if (FoundWidget)
		return FoundWidget;

	// Fallback: Iterate if FindWidget fails (shouldn't usually be needed)
	UWidget* Root = WidgetTree->RootWidget;
	if (!Root)
		return nullptr;

	TArray<UWidget*> WidgetsToSearch;
	WidgetsToSearch.Add(Root);

	while (WidgetsToSearch.Num() > 0)
	{
		UWidget* Current = WidgetsToSearch.Pop();
		if (Current && Current->GetFName() == Name) // Compare FName
		{
			return Current;
		}

		if (UPanelWidget* Panel = Cast<UPanelWidget>(Current))
		{
			for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
			{
				if (UWidget* Child = Panel->GetChildAt(i))
				{
					WidgetsToSearch.Add(Child);
				}
			}
		}
	}
	return nullptr;
}

bool UGenWidgetUtils::SaveAndRecompileWidgetBlueprint(UBlueprint* WidgetBP)
{
	if (!WidgetBP)
		return false;

	// Mark dirty, compile, and save
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP); // Important for tree changes
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);					  // Compile to ensure changes are reflected

	UPackage* Package = WidgetBP->GetOutermost();
	if (Package)
	{
		FString PackageFileName =
			FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError; // Suppress success dialogs
		bool bSaved = UPackage::SavePackage(Package, WidgetBP, *PackageFileName, SaveArgs);

		if (!bSaved)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save User Widget Blueprint: %s"), *WidgetBP->GetName());
			return false;
		}
		// FAssetRegistryModule::AssetCreated(WidgetBP); // Use AssetSaved? Or handled by SavePackage? Test this.
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed get package for: %s"), *WidgetBP->GetName());
		return false;
	}
	return true;
}

FString UGenWidgetUtils::AddWidgetToUserWidget(const FString& UserWidgetPath, const FString& WidgetClassName,
											   const FString& WidgetName, const FString& ParentWidgetName)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	FString OutputString; // Declare upfront for JSON serialization at the end
	auto CreateJsonReturn =
		[&](bool bSuccess, const FString& MessageOrError, const FString& ActualWidgetName = TEXT(""))
	{
		ResultJson->SetBoolField("success", bSuccess);
		if (bSuccess)
		{
			ResultJson->SetStringField("message", MessageOrError);
			if (!ActualWidgetName.IsEmpty())
			{
				ResultJson->SetStringField("widget_name", ActualWidgetName);
			}
		}
		else
		{
			ResultJson->SetStringField("error", MessageOrError);
		}
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);
		return OutputString;
	};

	// 1. Load and Validate Blueprint Asset
	UBlueprint* LoadedBP = LoadObject<UBlueprint>(nullptr, *UserWidgetPath);
	if (!LoadedBP)
	{
		return CreateJsonReturn(false,
								FString::Printf(TEXT("User Widget Blueprint asset not found at: %s"), *UserWidgetPath));
	}

	// 2. Cast to Widget Blueprint
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(LoadedBP);
	if (!WidgetBP)
	{
		return CreateJsonReturn(
			false, FString::Printf(TEXT("Asset at '%s' is not a User Widget Blueprint."), *UserWidgetPath));
	}

	// 3. Get or Create Widget Tree
	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
	if (!WidgetTree)
	{
		UE_LOG(LogTemp, Log, TEXT("WidgetTree not found for %s. Attempting creation."), *UserWidgetPath);
		WidgetTree = NewObject<UWidgetTree>(WidgetBP, TEXT("WidgetTree"), RF_Transactional);
		if (WidgetTree)
		{
			WidgetBP->WidgetTree = WidgetTree; // Assign it back
			UE_LOG(LogTemp, Log, TEXT("Created and assigned new WidgetTree for %s"), *UserWidgetPath);
			WidgetBP->Modify(); // Mark blueprint modified because we added the tree
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create WidgetTree for %s"), *UserWidgetPath);
			return CreateJsonReturn(
				false, FString::Printf(TEXT("WidgetTree not found and could not be created for: %s"), *UserWidgetPath));
		}
	}

	// 4. Find Widget Class to Add
	UClass* FoundClass = FindObject<UClass>(ANY_PACKAGE, *WidgetClassName);
	if (!FoundClass)
		FoundClass = LoadClass<UWidget>(nullptr, *FString::Printf(TEXT("/Script/UMG.%s"), *WidgetClassName));
	if (!FoundClass)
		FoundClass = LoadClass<UWidget>(nullptr, *FString::Printf(TEXT("/Script/CommonUI.%s"), *WidgetClassName));
	// Add more lookups if needed for custom widget libraries

	if (!FoundClass || !FoundClass->IsChildOf(UWidget::StaticClass()))
	{
		return CreateJsonReturn(false,
								FString::Printf(TEXT("Widget class not found or invalid: %s"), *WidgetClassName));
	}

	// 5. Determine Parent Panel
	UPanelWidget* ParentPanel = nullptr;
	if (!ParentWidgetName.IsEmpty())
	{
		// Try to find the specified parent by name
		UWidget* FoundParentWidget = FindWidgetByName(WidgetTree, FName(*ParentWidgetName));
		ParentPanel = Cast<UPanelWidget>(FoundParentWidget);
		if (!ParentPanel)
		{
			// Specified parent name was given but not found or wasn't a PanelWidget
			return CreateJsonReturn(
				false, FString::Printf(TEXT("Specified parent widget '%s' not found or is not a PanelWidget."),
									   *ParentWidgetName));
		}
		UE_LOG(LogTemp, Log, TEXT("Using specified parent panel: %s"), *ParentPanel->GetName());
	}
	else
	{
		// No parent specified, try default logic: root or first canvas
		if (WidgetTree->RootWidget != nullptr)
		{
			// Check if the existing root IS a panel widget
			ParentPanel = Cast<UPanelWidget>(WidgetTree->RootWidget);
			if (ParentPanel)
			{
				UE_LOG(LogTemp, Log, TEXT("Using existing root widget '%s' as parent panel."), *ParentPanel->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Log,
					   TEXT("Existing root widget '%s' is not a PanelWidget. Searching for first CanvasPanel."),
					   *WidgetTree->RootWidget->GetName());
			}
		}

		// If root wasn't a suitable panel, search for the first CanvasPanel in the tree
		if (!ParentPanel)
		{
			TArray<UWidget*> AllWidgets;
			WidgetTree->GetAllWidgets(AllWidgets);
			for (UWidget* W : AllWidgets)
			{
				if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(W))
				{
					ParentPanel = Canvas;
					UE_LOG(LogTemp, Log, TEXT("Found and using first CanvasPanel '%s' as parent panel."),
						   *ParentPanel->GetName());
					break;
				}
			}
		}
	}

	// --- At this point, ParentPanel is either set, or still nullptr ---

	// 6. Handle Adding Logic (Root or Child)
	if (!ParentPanel)
	{
		// --- FIX FOR PARADOX: Check if we can set the new widget as ROOT ---
		UE_LOG(LogTemp, Log,
			   TEXT("No suitable parent panel found. Checking if new widget can be root. Current RootWidget is: %s"),
			   WidgetTree->RootWidget ? *WidgetTree->RootWidget->GetName() : TEXT("nullptr"));

		if (FoundClass->IsChildOf(UPanelWidget::StaticClass()) && WidgetTree->RootWidget == nullptr)
		{
			// We ARE adding a panel, and the tree IS empty -> Set as root.
			UE_LOG(LogTemp, Log, TEXT("Setting new PanelWidget '%s' (%s) as RootWidget."), *WidgetName,
				   *WidgetClassName);

			FName ActualWidgetName = FBlueprintEditorUtils::FindUniqueKismetName(WidgetBP, WidgetName);
			UWidget* NewRootWidget = WidgetTree->ConstructWidget<UWidget>(FoundClass, ActualWidgetName);

			if (!NewRootWidget)
			{
				return CreateJsonReturn(
					false, FString::Printf(TEXT("Failed to construct root widget of type %s"), *WidgetClassName));
			}

			WidgetTree->RootWidget = NewRootWidget; // Assign as root
			WidgetTree->Modify();
			WidgetBP->Modify();

			// Save and return result for setting root
			if (SaveAndRecompileWidgetBlueprint(WidgetBP))
			{
				return CreateJsonReturn(
					true,
					FString::Printf(TEXT("Successfully added '%s' (%s) as the Root Widget to '%s'."),
									*NewRootWidget->GetName(), *WidgetClassName, *UserWidgetPath),
					NewRootWidget->GetName());
			}
			else
			{
				return CreateJsonReturn(
					false, FString::Printf(TEXT("Set '%s' as root but failed to save/recompile Blueprint '%s'."),
										   *NewRootWidget->GetName(), *UserWidgetPath));
			}
		}
		else
		{
			// Cannot set as root (either not adding a panel, or root already exists)
			// Return the original error about needing a parent.
			return CreateJsonReturn(
				false,
				TEXT("Could not find a suitable parent PanelWidget (root or specified parent), and cannot set the new "
					 "widget as root. Ensure the User Widget is empty or has a compatible root/parent panel."));
		}
		// --- END OF ROOT HANDLING ---
	}
	else
	{
		// --- STANDARD CHILD ADDING LOGIC ---
		// ParentPanel was found earlier.
		UE_LOG(LogTemp, Log, TEXT("Adding widget '%s' (%s) as child of '%s'."), *WidgetName, *WidgetClassName,
			   *ParentPanel->GetName());

		FName ActualWidgetName = FBlueprintEditorUtils::FindUniqueKismetName(WidgetBP, WidgetName);
		UWidget* NewChildWidget = WidgetTree->ConstructWidget<UWidget>(FoundClass, ActualWidgetName);

		if (!NewChildWidget)
		{
			return CreateJsonReturn(
				false, FString::Printf(TEXT("Failed to construct child widget of type %s"), *WidgetClassName));
		}

		// Add as child using UPanelSlot*
		UPanelSlot* NewSlot = ParentPanel->AddChild(NewChildWidget);
		if (!NewSlot)
		{
			// Maybe cleanup NewChildWidget? Difficult. Log error and fail.
			UE_LOG(LogTemp, Error, TEXT("ParentPanel->AddChild failed for '%s' -> '%s'. Slot incompatible?"),
				   *ParentPanel->GetName(), *NewChildWidget->GetName());
			return CreateJsonReturn(false, FString::Printf(TEXT("Failed to add '%s' as child of '%s'. Slot type might "
																"be incompatible or AddChild returned null."),
														   *NewChildWidget->GetName(), *ParentPanel->GetName()));
		}
		else
		{
			NewChildWidget->SetDesignerFlags(EWidgetDesignFlags::Designing); // Mark for designer
			UE_LOG(LogTemp, Log, TEXT("Successfully added '%s' to '%s'. Slot Type: %s"), *NewChildWidget->GetName(),
				   *ParentPanel->GetName(), *NewSlot->GetClass()->GetName());

			// Optional: Apply default layout properties by casting the UPanelSlot*
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(NewSlot))
			{
				CanvasSlot->SetAutoSize(true);
				CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				UE_LOG(LogTemp, Log, TEXT("Applied default CanvasPanelSlot properties to '%s'"),
					   *NewChildWidget->GetName());
			}
			else if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(NewSlot))
			{
				VBoxSlot->SetPadding(FMargin(0.f, 5.f));
				VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
				VBoxSlot->SetVerticalAlignment(VAlign_Center);
				UE_LOG(LogTemp, Log, TEXT("Applied default VerticalBoxSlot properties to '%s'"),
					   *NewChildWidget->GetName());
			}
			// Add other 'else if (Cast<SpecificSlotType>(NewSlot))' blocks as needed
		}

		// Save and return result for adding child
		if (SaveAndRecompileWidgetBlueprint(WidgetBP))
		{
			return CreateJsonReturn(
				true,
				FString::Printf(TEXT("Successfully added widget '%s' of type '%s' as child of '%s' in '%s'."),
								*ActualWidgetName.ToString(), *WidgetClassName, *ParentPanel->GetName(),
								*UserWidgetPath),
				ActualWidgetName.ToString());
		}
		else
		{
			return CreateJsonReturn(
				false, FString::Printf(TEXT("Added widget '%s' but failed to save/recompile Blueprint '%s'."),
									   *ActualWidgetName.ToString(), *UserWidgetPath));
		}
		// --- END OF CHILD ADDING LOGIC ---
	}
}

// Helper to find the property and the object it belongs to (widget or its slot)
bool UGenWidgetUtils::FindPropertyAndObject(UWidget* TargetWidget, const FString& PropertyName, UObject*& OutObject,
											FProperty*& OutProperty)
{
	if (!TargetWidget)
		return false;

	OutObject = nullptr;
	OutProperty = nullptr;

	FString TargetObjectName;
	FString TargetPropertyName;

	if (PropertyName.StartsWith(TEXT("Slot.")))
	{
		// Property is on the slot
		if (!TargetWidget->Slot)
		{
			UE_LOG(LogTemp, Warning, TEXT("Widget '%s' has no Slot object."), *TargetWidget->GetName());
			return false;
		}
		OutObject = TargetWidget->Slot;
		TargetPropertyName = PropertyName.RightChop(5); // Remove "Slot."
	}
	else
	{
		// Property is on the widget itself
		OutObject = TargetWidget;
		TargetPropertyName = PropertyName;
	}

	if (!OutObject)
		return false;

	// Find the property using reflection
	OutProperty = FindFProperty<FProperty>(OutObject->GetClass(), FName(*TargetPropertyName));

	if (!OutProperty)
	{
		UE_LOG(LogTemp, Warning, TEXT("Property '%s' not found on object '%s' (Class: %s)."), *TargetPropertyName,
			   *OutObject->GetName(), *OutObject->GetClass()->GetName());
		// Log available properties for debugging
		FString AvailableProps;
		for (TFieldIterator<FProperty> PropIt(OutObject->GetClass()); PropIt; ++PropIt)
		{
			AvailableProps += (*PropIt)->GetName() + TEXT(", ");
		}
		UE_LOG(LogTemp, Warning, TEXT("Available properties: %s"), *AvailableProps);

		return false;
	}

	return true;
}

FString UGenWidgetUtils::EditWidgetProperty(const FString& UserWidgetPath, const FString& WidgetName,
											const FString& PropertyName, const FString& ValueString)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);

	UBlueprint* LoadedBP = LoadObject<UBlueprint>(nullptr, *UserWidgetPath);
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(LoadedBP);
	if (!WidgetBP || !WidgetBP->GeneratedClass || !WidgetBP->GeneratedClass->IsChildOf(UUserWidget::StaticClass()))
	{
		ResultJson->SetBoolField("success", false);
		ResultJson->SetStringField(
			"error", FString::Printf(TEXT("User Widget Blueprint not found or invalid: %s"), *UserWidgetPath));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);
		return OutputString;
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
	if (!WidgetTree)
	{
		ResultJson->SetBoolField("success", false);
		ResultJson->SetStringField("error", FString::Printf(TEXT("WidgetTree not found for: %s"), *UserWidgetPath));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);
		return OutputString;
	}

	UWidget* TargetWidget = FindWidgetByName(WidgetTree, FName(*WidgetName));
	if (!TargetWidget)
	{
		ResultJson->SetBoolField("success", false);
		ResultJson->SetStringField(
			"error", FString::Printf(TEXT("Widget '%s' not found in User Widget '%s'."), *WidgetName, *UserWidgetPath));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);
		return OutputString;
	}

	UObject* TargetObject = nullptr;
	FProperty* TargetProperty = nullptr;

	if (!FindPropertyAndObject(TargetWidget, PropertyName, TargetObject, TargetProperty))
	{
		ResultJson->SetBoolField("success", false);
		ResultJson->SetStringField("error", FString::Printf(TEXT("Property '%s' not found on widget '%s' or its slot."),
															*PropertyName, *WidgetName));
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);
		return OutputString;
	}

	// Attempt to set the property value from the string
	bool bSuccess = false;
	FString ErrorMessage;
	try
	{
		// Mark the owning object (widget or slot) as modified
		TargetObject->Modify();

		// Get the address of the property within the TargetObject instance
		void* PropertyValueAddress = TargetProperty->ContainerPtrToValuePtr<void>(TargetObject);
		FStringOutputDevice ImportErrorOutput;
		const TCHAR* Result = TargetProperty->ImportText_Direct(*ValueString,		  // Buffer (const TCHAR*)
																PropertyValueAddress, // Data (void*)
																TargetObject,		  // OwnerObject (UObject*)
																PPF_None,			  // PortFlags (int32)
																&ImportErrorOutput	  // ErrorText (FOutputDevice*)
		);

		// Use ImportText to set the value. This handles various types including structs, enums, objects etc.
		//       FEditPropertyChain PropertyChain;
		// PropertyChain.AddHead(TargetProperty);
		//       TargetProperty->ImportText(*ValueString, PropertyValueAddress, PPF_None, TargetObject, &ErrorMessage,
		//       &PropertyChain); // Pass error message ptr

		// Check the result: nullptr indicates success, otherwise it points to the end of successfully parsed text
		if (Result != nullptr && ImportErrorOutput.Len() > 0) // Check if parsing stopped *and* an error was reported
		{
			// An error occurred during import
			ErrorMessage = ImportErrorOutput; // Get the error message from the output device
			UE_LOG(LogTemp, Error, TEXT("Failed to set property '%s' on '%s'. ImportText error: %s"), *PropertyName,
				   *TargetObject->GetName(), *ErrorMessage);
			bSuccess = false;
		}
		// Handle case where Result is not null but no error string was generated (partial parse?) - Treat as error for
		// simplicity
		else if (Result != nullptr && ValueString.Len() > 0)
		// Only treat as error if there was input and it wasn't fully parsed
		{
			ErrorMessage = FString::Printf(TEXT("Failed to fully parse value '%s' for property '%s'"), *ValueString,
										   *PropertyName);
			UE_LOG(LogTemp, Warning, TEXT("%s on object '%s'"), *ErrorMessage, *TargetObject->GetName());
			bSuccess = false;
		}
		else
		{
			// Success
			bSuccess = true;
			UE_LOG(LogTemp, Log, TEXT("Set property '%s' on '%s' to '%s'"), *PropertyName, *TargetObject->GetName(),
				   *ValueString);

			// Notify about the change, especially important for structs and objects
			FPropertyChangedEvent ChangeEvent(TargetProperty, EPropertyChangeType::ValueSet);
			// Use ValueSet for direct changes
			TargetObject->PostEditChangeProperty(ChangeEvent);

			// Also notify the WidgetBlueprint itself potentially? Might not be necessary.
			// WidgetBP->PostEditChange(); // Usually handled by Modify + Save/Compile cycle

			// Ensure the owning widget is marked dirty as well if the property was on the slot
			if (TargetObject == TargetWidget->Slot)
			{
				TargetWidget->Modify();
			}
		}
	}
	catch (const std::exception& e)
	{
		ErrorMessage = FString::Printf(TEXT("Exception during property setting: %s"), UTF8_TO_TCHAR(e.what()));
		UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMessage);
	}
	catch (...)
	{
		ErrorMessage = FString::Printf(TEXT("Unknown exception during property setting for '%s' on '%s'"),
									   *PropertyName, *TargetObject->GetName());
		UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMessage);
	}

	if (bSuccess)
	{
		if (SaveAndRecompileWidgetBlueprint(WidgetBP))
		{
			ResultJson->SetBoolField("success", true);
			ResultJson->SetStringField("message",
									   FString::Printf(TEXT("Successfully set property '%s' on widget '%s' in '%s'."),
													   *PropertyName, *WidgetName, *UserWidgetPath));
		}
		else
		{
			ResultJson->SetBoolField("success", false);
			ResultJson->SetStringField(
				"error", FString::Printf(TEXT("Set property '%s' but failed to save/recompile Blueprint '%s'."),
										 *PropertyName, *UserWidgetPath));
		}
	}
	else
	{
		ResultJson->SetBoolField("success", false);
		ResultJson->SetStringField("error",
								   FString::Printf(TEXT("Failed to set property '%s' on widget '%s'. Error: %s"),
												   *PropertyName, *WidgetName, *ErrorMessage));
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);
	return OutputString;
}
