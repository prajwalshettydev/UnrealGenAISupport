// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#include "MCP/GenWidgetUtils.h"
#include "ScopedTransaction.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/PanelWidget.h"
#include "EditorAssetLibrary.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "TimerManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/SavePackage.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/UserDefinedStruct.h" // Required for struct properties
#include "JsonObjectConverter.h"      // For JSON responses

// Include specific widget headers if needed for property access, though reflection should handle most cases
#include "WidgetBlueprint.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_ComponentBoundEvent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"

// Helper to find widget by name recursively (WidgetTree::FindWidget is often sufficient)
UWidget* UGenWidgetUtils::FindWidgetByName(UWidgetTree* WidgetTree, const FName& Name)
{
	if (!WidgetTree) return nullptr;

	UWidget* FoundWidget = WidgetTree->FindWidget(Name);
	if (FoundWidget) return FoundWidget;

	// Fallback: Iterate if FindWidget fails (shouldn't usually be needed)
	UWidget* Root = WidgetTree->RootWidget;
	if (!Root) return nullptr;

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
	if (!WidgetBP) return false;

	// Mark dirty, compile, and save
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP); // Important for tree changes
	FKismetEditorUtilities::CompileBlueprint(WidgetBP); // Compile to ensure changes are reflected

	UPackage* Package = WidgetBP->GetOutermost();
	if (Package)
	{
		FString PackageFileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
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


FString UGenWidgetUtils::AddWidgetToUserWidget(const FString& UserWidgetPath, const FString& WidgetClassName, const FString& WidgetName, const FString& ParentWidgetName)
{
    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
    FString OutputString; // Declare upfront for JSON serialization at the end
    auto CreateJsonReturn = [&](bool bSuccess, const FString& MessageOrError, const FString& ActualWidgetName = TEXT(""))
    {
        ResultJson->SetBoolField("success", bSuccess);
        if(bSuccess)
        {
            ResultJson->SetStringField("message", MessageOrError);
            if(!ActualWidgetName.IsEmpty())
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
        return CreateJsonReturn(false, FString::Printf(TEXT("User Widget Blueprint asset not found at: %s"), *UserWidgetPath));
    }

    // 2. Cast to Widget Blueprint
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(LoadedBP);
    if (!WidgetBP)
    {
        return CreateJsonReturn(false, FString::Printf(TEXT("Asset at '%s' is not a User Widget Blueprint."), *UserWidgetPath));
    }

    // 3. Get or Create Widget Tree
    UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Log, TEXT("WidgetTree not found for %s. Attempting creation."), *UserWidgetPath);
    // Undo/Redo support
    FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Add Widget")));
        WidgetTree = NewObject<UWidgetTree>(WidgetBP, TEXT("WidgetTree"), RF_Transactional);
        if(WidgetTree)
        {
            WidgetBP->WidgetTree = WidgetTree; // Assign it back
            UE_LOG(LogTemp, Log, TEXT("Created and assigned new WidgetTree for %s"), *UserWidgetPath);
            WidgetBP->Modify(); // Mark blueprint modified because we added the tree
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create WidgetTree for %s"), *UserWidgetPath);
            return CreateJsonReturn(false, FString::Printf(TEXT("WidgetTree not found and could not be created for: %s"), *UserWidgetPath));
        }
    }

    // 4. Find Widget Class to Add
    UClass* FoundClass = FindObject<UClass>(nullptr, *WidgetClassName);
    if (!FoundClass) FoundClass = LoadClass<UWidget>(nullptr, *FString::Printf(TEXT("/Script/UMG.%s"), *WidgetClassName));
    if (!FoundClass) FoundClass = LoadClass<UWidget>(nullptr, *FString::Printf(TEXT("/Script/CommonUI.%s"), *WidgetClassName));
    // Add more lookups if needed for custom widget libraries

    if (!FoundClass || !FoundClass->IsChildOf(UWidget::StaticClass()))
    {
        return CreateJsonReturn(false, FString::Printf(TEXT("Widget class not found or invalid: %s"), *WidgetClassName));
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
             return CreateJsonReturn(false, FString::Printf(TEXT("Specified parent widget '%s' not found or is not a PanelWidget."), *ParentWidgetName));
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
             if(ParentPanel)
             {
                UE_LOG(LogTemp, Log, TEXT("Using existing root widget '%s' as parent panel."), *ParentPanel->GetName());
             }
             else
             {
                UE_LOG(LogTemp, Log, TEXT("Existing root widget '%s' is not a PanelWidget. Searching for first CanvasPanel."), *WidgetTree->RootWidget->GetName());
             }
        }

        // If root wasn't a suitable panel, search for the first CanvasPanel in the tree
        if(!ParentPanel)
        {
             TArray<UWidget*> AllWidgets;
             WidgetTree->GetAllWidgets(AllWidgets);
             for(UWidget* W : AllWidgets)
             {
                 if(UCanvasPanel* Canvas = Cast<UCanvasPanel>(W))
                 {
                     ParentPanel = Canvas;
                     UE_LOG(LogTemp, Log, TEXT("Found and using first CanvasPanel '%s' as parent panel."), *ParentPanel->GetName());
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
        UE_LOG(LogTemp, Log, TEXT("No suitable parent panel found. Checking if new widget can be root. Current RootWidget is: %s"),
            WidgetTree->RootWidget ? *WidgetTree->RootWidget->GetName() : TEXT("nullptr"));

        if (FoundClass->IsChildOf(UPanelWidget::StaticClass()) && WidgetTree->RootWidget == nullptr)
        {
            // We ARE adding a panel, and the tree IS empty -> Set as root.
            UE_LOG(LogTemp, Log, TEXT("Setting new PanelWidget '%s' (%s) as RootWidget."), *WidgetName, *WidgetClassName);

            FName ActualWidgetName = FBlueprintEditorUtils::FindUniqueKismetName(WidgetBP, WidgetName);
            UWidget* NewRootWidget = WidgetTree->ConstructWidget<UWidget>(FoundClass, ActualWidgetName);

            if (!NewRootWidget)
            {
                 return CreateJsonReturn(false, FString::Printf(TEXT("Failed to construct root widget of type %s"), *WidgetClassName));
            }

            WidgetTree->RootWidget = NewRootWidget; // Assign as root
            WidgetTree->Modify();
            WidgetBP->Modify();

            // Save and return result for setting root
            if (SaveAndRecompileWidgetBlueprint(WidgetBP))
            {
                return CreateJsonReturn(true, FString::Printf(TEXT("Successfully added '%s' (%s) as the Root Widget to '%s'."), *NewRootWidget->GetName(), *WidgetClassName, *UserWidgetPath), NewRootWidget->GetName());
            }
            else
            {
                return CreateJsonReturn(false, FString::Printf(TEXT("Set '%s' as root but failed to save/recompile Blueprint '%s'."), *NewRootWidget->GetName(), *UserWidgetPath));
            }
        }
        else
        {
            // Cannot set as root (either not adding a panel, or root already exists)
            // Return the original error about needing a parent.
            return CreateJsonReturn(false, TEXT("Could not find a suitable parent PanelWidget (root or specified parent), and cannot set the new widget as root. Ensure the User Widget is empty or has a compatible root/parent panel."));
        }
        // --- END OF ROOT HANDLING ---
    }
    else
    {
        // --- STANDARD CHILD ADDING LOGIC ---
        // ParentPanel was found earlier.
        UE_LOG(LogTemp, Log, TEXT("Adding widget '%s' (%s) as child of '%s'."), *WidgetName, *WidgetClassName, *ParentPanel->GetName());

        FName ActualWidgetName = FBlueprintEditorUtils::FindUniqueKismetName(WidgetBP, WidgetName);
        UWidget* NewChildWidget = WidgetTree->ConstructWidget<UWidget>(FoundClass, ActualWidgetName);

        if (!NewChildWidget)
        {
             return CreateJsonReturn(false, FString::Printf(TEXT("Failed to construct child widget of type %s"), *WidgetClassName));
        }

        // Add as child using UPanelSlot*
        UPanelSlot* NewSlot = ParentPanel->AddChild(NewChildWidget);
        if (!NewSlot)
        {
            // Maybe cleanup NewChildWidget? Difficult. Log error and fail.
            UE_LOG(LogTemp, Error, TEXT("ParentPanel->AddChild failed for '%s' -> '%s'. Slot incompatible?"), *ParentPanel->GetName(), *NewChildWidget->GetName());
            return CreateJsonReturn(false, FString::Printf(TEXT("Failed to add '%s' as child of '%s'. Slot type might be incompatible or AddChild returned null."), *NewChildWidget->GetName(), *ParentPanel->GetName()));
        }
        else
        {
            NewChildWidget->SetDesignerFlags(EWidgetDesignFlags::Designing); // Mark for designer
            UE_LOG(LogTemp, Log, TEXT("Successfully added '%s' to '%s'. Slot Type: %s"), *NewChildWidget->GetName(), *ParentPanel->GetName(), *NewSlot->GetClass()->GetName());

            // Optional: Apply default layout properties by casting the UPanelSlot*
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(NewSlot))
            {
                CanvasSlot->SetAutoSize(true);
                CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
                CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
                UE_LOG(LogTemp, Log, TEXT("Applied default CanvasPanelSlot properties to '%s'"), *NewChildWidget->GetName());
            }
            else if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(NewSlot))
            {
                 VBoxSlot->SetPadding(FMargin(0.f, 5.f));
                 VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
                 VBoxSlot->SetVerticalAlignment(VAlign_Center);
                 UE_LOG(LogTemp, Log, TEXT("Applied default VerticalBoxSlot properties to '%s'"), *NewChildWidget->GetName());
            }
             // Add other 'else if (Cast<SpecificSlotType>(NewSlot))' blocks as needed
        }

        // Save and return result for adding child
        if (SaveAndRecompileWidgetBlueprint(WidgetBP))
        {
            return CreateJsonReturn(true, FString::Printf(TEXT("Successfully added widget '%s' of type '%s' as child of '%s' in '%s'."), *ActualWidgetName.ToString(), *WidgetClassName, *ParentPanel->GetName(), *UserWidgetPath), ActualWidgetName.ToString());
        }
        else
        {
            return CreateJsonReturn(false, FString::Printf(TEXT("Added widget '%s' but failed to save/recompile Blueprint '%s'."), *ActualWidgetName.ToString(), *UserWidgetPath));
        }
        // --- END OF CHILD ADDING LOGIC ---
    }
}

// Helper to find the property and the object it belongs to (widget or its slot)
bool UGenWidgetUtils::FindPropertyAndObject(UWidget* TargetWidget, const FString& PropertyName, UObject*& OutObject,
                                            FProperty*& OutProperty)
{
	if (!TargetWidget) return false;

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

	if (!OutObject) return false;

	// Find the property using reflection
	OutProperty = FindFProperty<FProperty>(OutObject->GetClass(), FName(*TargetPropertyName));

	if (!OutProperty)
	{
		UE_LOG(LogTemp, Warning, TEXT("Property '%s' not found on object '%s' (Class: %s)."),
		       *TargetPropertyName, *OutObject->GetName(), *OutObject->GetClass()->GetName());
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
		ResultJson->SetStringField(
			"error", FString::Printf(
				TEXT("Property '%s' not found on widget '%s' or its slot."), *PropertyName, *WidgetName));
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
		FString ImportErrorOutput;
		const TCHAR* Result = TargetProperty->ImportText_Direct(
			*ValueString, // Buffer (const TCHAR*)
			PropertyValueAddress, // Data (void*)
			TargetObject, // OwnerObject (UObject*)
			PPF_None, // PortFlags (int32)
			nullptr // ErrorText (FOutputDevice*)
		);

		// Use ImportText to set the value. This handles various types including structs, enums, objects etc.
		//       FEditPropertyChain PropertyChain;
		// PropertyChain.AddHead(TargetProperty);
		//       TargetProperty->ImportText(*ValueString, PropertyValueAddress, PPF_None, TargetObject, &ErrorMessage, &PropertyChain); // Pass error message ptr

		// ImportText_Direct returns:
		// - nullptr if parsing failed completely (no characters consumed)
		// - pointer to character AFTER the last successfully parsed character
		// For successful parse of "200", Result points to '\0' (null terminator)

		if (Result == nullptr)
		{
			// Complete failure - no characters were parsed
			ErrorMessage = FString::Printf(TEXT("Failed to parse value '%s' for property '%s'"), *ValueString, *PropertyName);
			UE_LOG(LogTemp, Error, TEXT("%s on object '%s'"), *ErrorMessage, *TargetObject->GetName());
			bSuccess = false;
		}
		else
		{
			// Parsing succeeded - Result points to remaining unparsed text (or '\0' if fully parsed)
			FString Remaining(Result);
			Remaining.TrimStartAndEndInline();

			if (Remaining.Len() > 0)
			{
				// There's unparsed content - log warning but still consider success
				UE_LOG(LogTemp, Warning, TEXT("Partial parse for '%s': remaining='%s'"), *PropertyName, *Remaining);
			}

			// Success - the value was imported
			bSuccess = true;
			UE_LOG(LogTemp, Log, TEXT("Set property '%s' on '%s' to '%s'"), *PropertyName, *TargetObject->GetName(),
			       *ValueString);

			// Notify about the property change
			FPropertyChangedEvent ChangeEvent(TargetProperty, EPropertyChangeType::ValueSet);
			TargetObject->PostEditChangeProperty(ChangeEvent);

			// Mark slot's widget as dirty if property was on the slot
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
		ErrorMessage = FString::Printf(
			TEXT("Unknown exception during property setting for '%s' on '%s'"), *PropertyName,
			*TargetObject->GetName());
		UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMessage);
	}


	if (bSuccess)
	{
		if (SaveAndRecompileWidgetBlueprint(WidgetBP))
		{
			ResultJson->SetBoolField("success", true);
			ResultJson->SetStringField("message", FString::Printf(
				                           TEXT("Successfully set property '%s' on widget '%s' in '%s'."),
				                           *PropertyName, *WidgetName, *UserWidgetPath));
		}
		else
		{
			ResultJson->SetBoolField("success", false);
			ResultJson->SetStringField("error", FString::Printf(
				                           TEXT("Set property '%s' but failed to save/recompile Blueprint '%s'."),
				                           *PropertyName, *UserWidgetPath));
		}
	}
	else
	{
		ResultJson->SetBoolField("success", false);
		ResultJson->SetStringField("error", FString::Printf(
			                           TEXT("Failed to set property '%s' on widget '%s'. Error: %s"), *PropertyName,
			                           *WidgetName, *ErrorMessage));
	}


	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);
	return OutputString;
}

FString UGenWidgetUtils::BindWidgetEvent(const FString& UserWidgetPath, const FString& WidgetName, const FString& EventName)
{
	// Load the Widget Blueprint
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(
		UEditorAssetLibrary::LoadAsset(UserWidgetPath));
	if (!WidgetBP)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Widget Blueprint not found: %s\"}"), *UserWidgetPath);
	}

	// Get the WidgetTree
	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
	if (!WidgetTree)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"WidgetTree not found in: %s\"}"), *UserWidgetPath);
	}

	// Find the widget by name
	UWidget* TargetWidget = FindWidgetByName(WidgetTree, FName(*WidgetName));
	if (!TargetWidget)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Widget '%s' not found in: %s\"}"), *WidgetName, *UserWidgetPath);
	}

	// Mark the widget as a variable so it can be accessed in the Event Graph
	TargetWidget->bIsVariable = true;

	// Get the widget's class and find the delegate property
	UClass* WidgetClass = TargetWidget->GetClass();
	FMulticastDelegateProperty* DelegateProperty = CastField<FMulticastDelegateProperty>(
		WidgetClass->FindPropertyByName(FName(*EventName)));

	if (!DelegateProperty)
	{
		// Try with common prefixes/suffixes
		FString AlternativeNames[] = {
			EventName,
			FString::Printf(TEXT("On%s"), *EventName),
			FString::Printf(TEXT("%sEvent"), *EventName)
		};

		for (const FString& AltName : AlternativeNames)
		{
			DelegateProperty = CastField<FMulticastDelegateProperty>(
				WidgetClass->FindPropertyByName(FName(*AltName)));
			if (DelegateProperty)
			{
				break;
			}
		}
	}

	if (!DelegateProperty)
	{
		// List available delegates for debugging
		FString AvailableDelegates;
		for (TFieldIterator<FMulticastDelegateProperty> It(WidgetClass); It; ++It)
		{
			AvailableDelegates += It->GetName() + TEXT(", ");
		}
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Delegate '%s' not found on widget class '%s'. Available: %s\"}"),
			*EventName, *WidgetClass->GetName(), *AvailableDelegates);
	}

	// Get or create the Event Graph
	UEdGraph* EventGraph = nullptr;
	if (WidgetBP->UbergraphPages.Num() > 0)
	{
		EventGraph = WidgetBP->UbergraphPages[0];
	}
	else
	{
		EventGraph = FBlueprintEditorUtils::CreateNewGraph(WidgetBP, FName(TEXT("EventGraph")),
			UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
		WidgetBP->UbergraphPages.Add(EventGraph);
	}

	if (!EventGraph)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to get or create EventGraph\"}");
	}

	// Create the ComponentBoundEvent node
	UK2Node_ComponentBoundEvent* EventNode = NewObject<UK2Node_ComponentBoundEvent>(EventGraph);
	if (!EventNode)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to create K2Node_ComponentBoundEvent\"}");
	}

	// Configure the event node
	EventNode->ComponentPropertyName = FName(*WidgetName);
	EventNode->DelegatePropertyName = DelegateProperty->GetFName();
	EventNode->DelegateOwnerClass = WidgetClass;

	// Set position
	EventNode->NodePosX = 0;
	EventNode->NodePosY = EventGraph->Nodes.Num() * 200;

	// Initialize and allocate pins
	EventNode->AllocateDefaultPins();

	// Generate GUID if needed
	if (!EventNode->NodeGuid.IsValid())
	{
		EventNode->NodeGuid = FGuid::NewGuid();
	}

	// Add to graph
	EventGraph->AddNode(EventNode, false, false);

	UE_LOG(LogTemp, Log, TEXT("Created widget event node for %s.%s with GUID %s"),
		*WidgetName, *EventName, *EventNode->NodeGuid.ToString());

	// Mark as modified and compile
	WidgetBP->Modify();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	// Save
	SaveAndRecompileWidgetBlueprint(WidgetBP);

	return FString::Printf(TEXT("{\"success\": true, \"node_id\": \"%s\", \"message\": \"Created event node for %s.%s\"}"),
		*EventNode->NodeGuid.ToString(), *WidgetName, *EventName);
}

FString UGenWidgetUtils::SafeOpenWidgetEditor(const FString& WidgetPath)
{
	// Load the Widget Blueprint
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(
		UEditorAssetLibrary::LoadAsset(WidgetPath));

	if (!WidgetBP)
	{
		return TEXT("{\"success\": false, \"error\": \"Widget Blueprint not found or invalid path\"}");
	}

	if (!GEditor)
	{
		return TEXT("{\"success\": false, \"error\": \"GEditor not available\"}");
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
	{
		return TEXT("{\"success\": false, \"error\": \"AssetEditorSubsystem not available\"}");
	}

	// Check if editor is already open
	IAssetEditorInstance* ExistingEditor = AssetEditorSubsystem->FindEditorForAsset(WidgetBP, false);
	if (ExistingEditor)
	{
		// Editor already open, just focus it
		ExistingEditor->FocusWindow();
		return TEXT("{\"success\": true, \"message\": \"Widget editor already open, focused window\"}");
	}

	// Use deferred execution to avoid crash in UE5.5
	// Schedule the open operation for next tick
	FString WidgetPathCopy = WidgetPath;
	GEditor->GetTimerManager()->SetTimerForNextTick([WidgetPathCopy]()
	{
		if (!GEditor) return;

		UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(
			UEditorAssetLibrary::LoadAsset(WidgetPathCopy));
		if (!WBP) return;

		UAssetEditorSubsystem* AES = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (!AES) return;

		// Open with Standalone mode which is more stable for Widget Blueprints
		AES->OpenEditorForAsset(WBP, EToolkitMode::Standalone);

		UE_LOG(LogTemp, Log, TEXT("SafeOpenWidgetEditor: Opened %s in next tick"), *WidgetPathCopy);
	});

	return TEXT("{\"success\": true, \"message\": \"Widget editor opening scheduled (deferred execution)\"}");
}

FString UGenWidgetUtils::ListWidgets(const FString& UserWidgetPath)
{
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(
		UEditorAssetLibrary::LoadAsset(UserWidgetPath));

	if (!WidgetBP)
	{
		return TEXT("{\"success\": false, \"error\": \"Widget Blueprint not found\"}");
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
	if (!WidgetTree)
	{
		return TEXT("{\"success\": false, \"error\": \"Widget tree not found\"}");
	}

	TArray<TSharedPtr<FJsonValue>> WidgetsArray;

	// Helper lambda to recursively collect widgets
	TFunction<void(UWidget*, const FString&)> CollectWidgets = [&](UWidget* Widget, const FString& ParentName)
	{
		if (!Widget) return;

		TSharedPtr<FJsonObject> WidgetJson = MakeShareable(new FJsonObject);
		WidgetJson->SetStringField("name", Widget->GetName());
		WidgetJson->SetStringField("type", Widget->GetClass()->GetName());
		WidgetJson->SetStringField("parent", ParentName);
		WidgetJson->SetBoolField("is_variable", Widget->bIsVariable);

		WidgetsArray.Add(MakeShareable(new FJsonValueObject(WidgetJson)));

		// If it's a panel widget, collect children
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			for (int32 i = 0; i < Panel->GetChildrenCount(); i++)
			{
				CollectWidgets(Panel->GetChildAt(i), Widget->GetName());
			}
		}
	};

	// Start from root
	if (WidgetTree->RootWidget)
	{
		CollectWidgets(WidgetTree->RootWidget, TEXT(""));
	}

	// Build result JSON
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", WidgetsArray.Num());
	ResultJson->SetArrayField("widgets", WidgetsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenWidgetUtils::GetWidgetProperty(const FString& UserWidgetPath, const FString& WidgetName, const FString& PropertyName)
{
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(
		UEditorAssetLibrary::LoadAsset(UserWidgetPath));

	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		return TEXT("{\"success\": false, \"error\": \"Widget Blueprint not found\"}");
	}

	UWidget* TargetWidget = FindWidgetByName(WidgetBP->WidgetTree, FName(*WidgetName));
	if (!TargetWidget)
	{
		return TEXT("{\"success\": false, \"error\": \"Widget not found\"}");
	}

	UObject* TargetObject = nullptr;
	FProperty* Property = nullptr;

	if (!FindPropertyAndObject(TargetWidget, PropertyName, TargetObject, Property))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Property '%s' not found\"}"), *PropertyName);
	}

	// Get the property value as string
	FString ValueString;
	const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(TargetObject);
	Property->ExportTextItem_Direct(ValueString, ValuePtr, nullptr, nullptr, PPF_None);

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetStringField("widget", WidgetName);
	ResultJson->SetStringField("property", PropertyName);
	ResultJson->SetStringField("value", ValueString);
	ResultJson->SetStringField("type", Property->GetCPPType());

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenWidgetUtils::DeleteWidget(const FString& UserWidgetPath, const FString& WidgetName)
{
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(
		UEditorAssetLibrary::LoadAsset(UserWidgetPath));

	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		return TEXT("{\"success\": false, \"error\": \"Widget Blueprint not found\"}");
	}

	UWidget* TargetWidget = FindWidgetByName(WidgetBP->WidgetTree, FName(*WidgetName));
	if (!TargetWidget)
	{
		return TEXT("{\"success\": false, \"error\": \"Widget not found\"}");
	}

	// Cannot delete root widget
	if (TargetWidget == WidgetBP->WidgetTree->RootWidget)
	{
		return TEXT("{\"success\": false, \"error\": \"Cannot delete root widget\"}");
	}

	// Remove from parent if it has one
	if (UPanelWidget* ParentPanel = TargetWidget->GetParent())
	{
		ParentPanel->RemoveChild(TargetWidget);
	}

	// Remove from widget tree
	WidgetBP->WidgetTree->RemoveWidget(TargetWidget);

	// Mark as modified
	WidgetBP->Modify();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	if (SaveAndRecompileWidgetBlueprint(WidgetBP))
	{
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Widget '%s' deleted\"}"), *WidgetName);
	}

	return TEXT("{\"success\": false, \"error\": \"Failed to save after deletion\"}");
}

FString UGenWidgetUtils::DuplicateWidget(const FString& UserWidgetPath, const FString& WidgetName, const FString& NewWidgetName)
{
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(
		UEditorAssetLibrary::LoadAsset(UserWidgetPath));

	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		return TEXT("{\"success\": false, \"error\": \"Widget Blueprint not found\"}");
	}

	UWidget* SourceWidget = FindWidgetByName(WidgetBP->WidgetTree, FName(*WidgetName));
	if (!SourceWidget)
	{
		return TEXT("{\"success\": false, \"error\": \"Source widget not found\"}");
	}

	UPanelWidget* ParentPanel = SourceWidget->GetParent();
	if (!ParentPanel)
	{
		return TEXT("{\"success\": false, \"error\": \"Cannot duplicate root widget or widget without parent\"}");
	}

	// Duplicate the widget
	UWidget* NewWidget = DuplicateObject<UWidget>(SourceWidget, WidgetBP->WidgetTree, FName(*NewWidgetName));
	if (!NewWidget)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to duplicate widget\"}");
	}

	// Rename it
	NewWidget->Rename(*NewWidgetName, WidgetBP->WidgetTree);

	// Add to parent
	UPanelSlot* NewSlot = ParentPanel->AddChild(NewWidget);
	if (!NewSlot)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to add duplicated widget to parent\"}");
	}

	// Mark as modified
	WidgetBP->Modify();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	if (SaveAndRecompileWidgetBlueprint(WidgetBP))
	{
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Widget duplicated as '%s'\", \"new_widget\": \"%s\"}"),
			*NewWidgetName, *NewWidget->GetName());
	}

	return TEXT("{\"success\": false, \"error\": \"Failed to save after duplication\"}");
}
