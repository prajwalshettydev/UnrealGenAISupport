// GenWidgetUtils.h - Widget Blueprint Manipulation Utilities
// Part of GenerativeAISupport Plugin for Unreal Engine

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenWidgetUtils.generated.h"

class UWidget;
class UWidgetTree;
class UBlueprint;

/**
 * Utility class for manipulating Widget Blueprints programmatically.
 * Provides functions to add widgets, edit properties, and bind events.
 */
UCLASS()
class GENERATIVEAISUPPORTEDITOR_API UGenWidgetUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Add a new widget to a User Widget Blueprint.
	 * @param UserWidgetPath - The asset path to the Widget Blueprint (e.g., "/Game/UI/MyWidget.MyWidget")
	 * @param WidgetClassName - The class name of the widget to add (e.g., "SizeBox", "Image", "TextBlock")
	 * @param WidgetName - The name to give the new widget
	 * @param ParentWidgetName - The name of the parent widget (empty string for root)
	 * @return JSON result with success status and message
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Widget Utils")
	static FString AddWidgetToUserWidget(
		const FString& UserWidgetPath,
		const FString& WidgetClassName,
		const FString& WidgetName,
		const FString& ParentWidgetName);

	/**
	 * Edit a property on a widget within a Widget Blueprint.
	 * @param UserWidgetPath - The asset path to the Widget Blueprint
	 * @param WidgetName - The name of the widget to modify
	 * @param PropertyName - The name of the property to set (use "Slot." prefix for slot properties)
	 * @param ValueString - The value as a string (will be parsed appropriately)
	 * @return JSON result with success status and message
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Widget Utils")
	static FString EditWidgetProperty(
		const FString& UserWidgetPath,
		const FString& WidgetName,
		const FString& PropertyName,
		const FString& ValueString);

	/**
	 * Bind a widget event to a function in the Widget Blueprint.
	 * @param UserWidgetPath - The asset path to the Widget Blueprint
	 * @param WidgetName - The name of the widget
	 * @param EventName - The name of the event to bind (e.g., "OnClicked", "OnHovered")
	 * @return JSON result with success status and node_id
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Widget Utils")
	static FString BindWidgetEvent(
		const FString& UserWidgetPath,
		const FString& WidgetName,
		const FString& EventName);

	/**
	 * Safely open a Widget Blueprint in the editor without crashing.
	 * Uses deferred execution to avoid UE5.5 crash bug with OpenEditorForAssets.
	 * @param WidgetPath - The asset path to the Widget Blueprint
	 * @return JSON result with success status and message
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Widget Utils")
	static FString SafeOpenWidgetEditor(const FString& WidgetPath);

	/**
	 * List all widgets in a Widget Blueprint.
	 * @param UserWidgetPath - The asset path to the Widget Blueprint
	 * @return JSON array with widget names, types, and hierarchy info
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Widget Utils")
	static FString ListWidgets(const FString& UserWidgetPath);

	/**
	 * Get a property value from a widget.
	 * @param UserWidgetPath - The asset path to the Widget Blueprint
	 * @param WidgetName - The name of the widget
	 * @param PropertyName - The name of the property to get
	 * @return JSON result with property value
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Widget Utils")
	static FString GetWidgetProperty(
		const FString& UserWidgetPath,
		const FString& WidgetName,
		const FString& PropertyName);

	/**
	 * Delete a widget from a Widget Blueprint.
	 * @param UserWidgetPath - The asset path to the Widget Blueprint
	 * @param WidgetName - The name of the widget to delete
	 * @return JSON result with success status
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Widget Utils")
	static FString DeleteWidget(
		const FString& UserWidgetPath,
		const FString& WidgetName);

	/**
	 * Duplicate a widget within a Widget Blueprint.
	 * @param UserWidgetPath - The asset path to the Widget Blueprint
	 * @param WidgetName - The name of the widget to duplicate
	 * @param NewWidgetName - The name for the duplicated widget
	 * @return JSON result with new widget name
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Widget Utils")
	static FString DuplicateWidget(
		const FString& UserWidgetPath,
		const FString& WidgetName,
		const FString& NewWidgetName);

private:
	/**
	 * Helper to find widget by name recursively in the WidgetTree.
	 */
	static UWidget* FindWidgetByName(UWidgetTree* WidgetTree, const FName& Name);

	/**
	 * Helper to save and recompile a Widget Blueprint after modifications.
	 */
	static bool SaveAndRecompileWidgetBlueprint(UBlueprint* WidgetBP);

	/**
	 * Helper to find the property and the object it belongs to (widget or its slot).
	 */
	static bool FindPropertyAndObject(UWidget* TargetWidget, const FString& PropertyName, 
		UObject*& OutObject, FProperty*& OutProperty);
};
