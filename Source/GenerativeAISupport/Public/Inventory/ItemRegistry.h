// Item Registry - Global item definition storage
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Inventory/InventoryTypes.h"
#include "ItemRegistry.generated.h"

/**
 * Item Registry - Static class for managing item definitions
 * Items can be registered from DataAssets or JSON files
 */
UCLASS()
class GENERATIVEAISUPPORT_API UItemRegistry : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Register an item definition
	 * @param ItemData - The item to register
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Registry")
	static void RegisterItem(const FItemDefinition& ItemData);

	/**
	 * Register item from DataAsset
	 * @param ItemAsset - The item data asset
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Registry")
	static void RegisterItemAsset(UItemDataAsset* ItemAsset);

	/**
	 * Load items from JSON file
	 * @param FilePath - Path to JSON file
	 * @return Number of items loaded
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Registry")
	static int32 LoadItemsFromJSON(const FString& FilePath);

	/**
	 * Get item definition by ID
	 * @param ItemID - The item ID
	 * @return Pointer to item definition or nullptr
	 */
	static const FItemDefinition* GetItemDefinition(const FString& ItemID);

	/**
	 * Get item definition (Blueprint version)
	 * @param ItemID - The item ID
	 * @param OutItemData - The item definition
	 * @return True if found
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Registry", meta = (DisplayName = "Get Item Definition"))
	static bool GetItem(const FString& ItemID, FItemDefinition& OutItemData);

	/**
	 * Check if item exists
	 * @param ItemID - The item ID
	 * @return True if registered
	 */
	UFUNCTION(BlueprintPure, Category = "Item Registry")
	static bool ItemExists(const FString& ItemID);

	/**
	 * Get all registered items
	 * @return Array of all item definitions
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Registry")
	static TArray<FItemDefinition> GetAllItems();

	/**
	 * Get items by category
	 * @param Category - Category to filter by
	 * @return Array of matching items
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Registry")
	static TArray<FItemDefinition> GetItemsByCategory(EItemCategory Category);

	/**
	 * Get items by rarity
	 * @param Rarity - Rarity to filter by
	 * @return Array of matching items
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Registry")
	static TArray<FItemDefinition> GetItemsByRarity(EItemRarity Rarity);

	/**
	 * Clear all registered items
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Registry")
	static void ClearRegistry();

	/**
	 * Get rarity color
	 * @param Rarity - The rarity
	 * @return Color for the rarity
	 */
	UFUNCTION(BlueprintPure, Category = "Item Registry")
	static FLinearColor GetRarityColor(EItemRarity Rarity);

private:
	static TMap<FString, FItemDefinition>& GetRegistry();
};
