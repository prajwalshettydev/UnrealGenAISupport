// Shop Library - Blueprint callable functions for shops and trading
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Shop/ShopTypes.h"
#include "ShopLibrary.generated.h"

// Delegate for transaction events
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnTransactionComplete, const FTransactionResult&, Result);

/**
 * Blueprint Function Library for Shop System
 */
UCLASS()
class GENERATIVEAISUPPORT_API UShopLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// SHOP REGISTRY
	// ============================================

	/**
	 * Register a shop
	 * @param Shop - The shop to register
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static void RegisterShop(const FShopDefinition& Shop);

	/**
	 * Register shop from data asset
	 * @param Asset - The shop data asset
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static void RegisterShopAsset(UShopDataAsset* Asset);

	/**
	 * Load shops from JSON file
	 * @param FilePath - Path to JSON file
	 * @return Number of shops loaded
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static int32 LoadShopsFromJSON(const FString& FilePath);

	/**
	 * Get a shop by ID
	 * @param ShopID - The shop ID
	 * @param OutShop - The shop definition
	 * @return True if found
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static bool GetShop(const FString& ShopID, FShopDefinition& OutShop);

	/**
	 * Get shop by owner NPC
	 * @param NPCID - The NPC's ID
	 * @param OutShop - The shop definition
	 * @return True if found
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static bool GetShopByOwner(const FString& NPCID, FShopDefinition& OutShop);

	/**
	 * Get all shops
	 * @return Array of all registered shops
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static TArray<FShopDefinition> GetAllShops();

	// ============================================
	// BUYING
	// ============================================

	/**
	 * Buy an item from a shop
	 * @param ShopID - The shop to buy from
	 * @param ItemID - The item to buy
	 * @param Quantity - How many to buy
	 * @param PlayerGold - Current player gold (will be modified)
	 * @param OutResult - Transaction result
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static bool BuyItem(const FString& ShopID, const FString& ItemID, int32 Quantity, UPARAM(ref) int32& PlayerGold, FTransactionResult& OutResult);

	/**
	 * Check if player can buy an item
	 * @param ShopID - The shop
	 * @param ItemID - The item
	 * @param Quantity - How many
	 * @param PlayerGold - Player's current gold
	 * @param PlayerLevel - Player's level
	 * @param PlayerReputation - Player's reputation with shop
	 * @param OutReason - Reason if can't buy
	 * @return True if can buy
	 */
	UFUNCTION(BlueprintPure, Category = "Shop System")
	static bool CanBuyItem(const FString& ShopID, const FString& ItemID, int32 Quantity, int32 PlayerGold, int32 PlayerLevel, int32 PlayerReputation, FText& OutReason);

	/**
	 * Get the effective buy price for an item
	 * @param ShopID - The shop
	 * @param ItemID - The item
	 * @param PlayerReputation - Player's reputation (for discounts)
	 * @return Final price per unit
	 */
	UFUNCTION(BlueprintPure, Category = "Shop System")
	static int32 GetBuyPrice(const FString& ShopID, const FString& ItemID, int32 PlayerReputation = 0);

	// ============================================
	// SELLING
	// ============================================

	/**
	 * Sell an item to a shop
	 * @param ShopID - The shop to sell to
	 * @param ItemID - The item to sell
	 * @param Quantity - How many to sell
	 * @param PlayerGold - Current player gold (will be modified)
	 * @param OutResult - Transaction result
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static bool SellItem(const FString& ShopID, const FString& ItemID, int32 Quantity, UPARAM(ref) int32& PlayerGold, FTransactionResult& OutResult);

	/**
	 * Check if shop will buy an item
	 * @param ShopID - The shop
	 * @param ItemID - The item
	 * @param OutReason - Reason if shop won't buy
	 * @return True if shop accepts this item
	 */
	UFUNCTION(BlueprintPure, Category = "Shop System")
	static bool ShopWillBuyItem(const FString& ShopID, const FString& ItemID, FText& OutReason);

	/**
	 * Get the sell price for an item
	 * @param ShopID - The shop
	 * @param ItemID - The item
	 * @param PlayerReputation - Player's reputation (for better prices)
	 * @return Price shop will pay per unit
	 */
	UFUNCTION(BlueprintPure, Category = "Shop System")
	static int32 GetSellPrice(const FString& ShopID, const FString& ItemID, int32 PlayerReputation = 0);

	// ============================================
	// STOCK MANAGEMENT
	// ============================================

	/**
	 * Get current stock for an item
	 * @param ShopID - The shop
	 * @param ItemID - The item
	 * @return Current stock (-1 = unlimited)
	 */
	UFUNCTION(BlueprintPure, Category = "Shop System")
	static int32 GetItemStock(const FString& ShopID, const FString& ItemID);

	/**
	 * Check if shop needs restocking
	 * @param ShopID - The shop
	 * @param CurrentGameTime - Current game time
	 * @return True if restock is needed
	 */
	UFUNCTION(BlueprintPure, Category = "Shop System")
	static bool NeedsRestock(const FString& ShopID, const FDateTime& CurrentGameTime);

	/**
	 * Restock a shop
	 * @param ShopID - The shop to restock
	 * @param CurrentGameTime - Current game time
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static void RestockShop(const FString& ShopID, const FDateTime& CurrentGameTime);

	/**
	 * Restock all shops that need it
	 * @param CurrentGameTime - Current game time
	 * @return Number of shops restocked
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static int32 RestockAllShops(const FDateTime& CurrentGameTime);

	// ============================================
	// SHOP ITEMS QUERY
	// ============================================

	/**
	 * Get all items in a shop
	 * @param ShopID - The shop
	 * @return Array of shop items
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static TArray<FShopItem> GetShopItems(const FString& ShopID);

	/**
	 * Get featured items in a shop
	 * @param ShopID - The shop
	 * @return Array of featured items
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static TArray<FShopItem> GetFeaturedItems(const FString& ShopID);

	/**
	 * Get items on sale
	 * @param ShopID - The shop
	 * @return Array of items with discounts
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static TArray<FShopItem> GetItemsOnSale(const FString& ShopID);

	/**
	 * Get items player can afford
	 * @param ShopID - The shop
	 * @param PlayerGold - Player's gold
	 * @param PlayerLevel - Player's level
	 * @return Array of affordable items
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static TArray<FShopItem> GetAffordableItems(const FString& ShopID, int32 PlayerGold, int32 PlayerLevel);

	// ============================================
	// SPECIAL OFFERS
	// ============================================

	/**
	 * Set a discount on an item
	 * @param ShopID - The shop
	 * @param ItemID - The item
	 * @param DiscountPercent - Discount percentage (0.1 = 10% off)
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static void SetItemDiscount(const FString& ShopID, const FString& ItemID, float DiscountPercent);

	/**
	 * Set a markup on an item
	 * @param ShopID - The shop
	 * @param ItemID - The item
	 * @param MarkupPercent - Markup percentage (0.1 = 10% more)
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static void SetItemMarkup(const FString& ShopID, const FString& ItemID, float MarkupPercent);

	/**
	 * Clear all price modifiers for a shop
	 * @param ShopID - The shop
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static void ClearPriceModifiers(const FString& ShopID);

	// ============================================
	// REPUTATION
	// ============================================

	/**
	 * Calculate reputation discount/bonus
	 * @param BasePrice - Original price
	 * @param Reputation - Player's reputation with shop
	 * @param bIsBuying - True if player is buying, false if selling
	 * @return Modified price
	 */
	UFUNCTION(BlueprintPure, Category = "Shop System")
	static int32 ApplyReputationModifier(int32 BasePrice, int32 Reputation, bool bIsBuying);

	// ============================================
	// SAVE/LOAD
	// ============================================

	/**
	 * Save shop states to JSON
	 * @return JSON string of shop states
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static FString SaveShopStatesToJSON();

	/**
	 * Load shop states from JSON
	 * @param JsonString - The JSON to parse
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop System")
	static bool LoadShopStatesFromJSON(const FString& JsonString);

private:
	static TMap<FString, FShopDefinition>& GetRegistry();
	static FShopItem* FindShopItem(const FString& ShopID, const FString& ItemID);
};
