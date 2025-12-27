// Loot Library - Blueprint callable functions for loot generation
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Loot/LootTypes.h"
#include "LootLibrary.generated.h"

/**
 * Blueprint Function Library for Loot System
 */
UCLASS()
class GENERATIVEAISUPPORT_API ULootLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// LOOT TABLE REGISTRY
	// ============================================

	/**
	 * Register a loot table
	 * @param LootTable - The loot table to register
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot System")
	static void RegisterLootTable(const FLootTable& LootTable);

	/**
	 * Register loot table from data asset
	 * @param Asset - The loot table data asset
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot System")
	static void RegisterLootTableAsset(ULootTableDataAsset* Asset);

	/**
	 * Load loot tables from JSON file
	 * @param FilePath - Path to JSON file
	 * @return Number of tables loaded
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot System")
	static int32 LoadLootTablesFromJSON(const FString& FilePath);

	/**
	 * Get a loot table by ID
	 * @param TableID - The table ID
	 * @param OutTable - The loot table
	 * @return True if found
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot System")
	static bool GetLootTable(const FString& TableID, FLootTable& OutTable);

	// ============================================
	// LOOT GENERATION
	// ============================================

	/**
	 * Generate loot from a loot table
	 * @param TableID - The loot table ID
	 * @param PlayerLevel - Current player level (for scaling)
	 * @param OutDrops - Generated item drops
	 * @param OutGold - Generated gold amount
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot System")
	static bool GenerateLoot(const FString& TableID, int32 PlayerLevel, TArray<FLootDrop>& OutDrops, int32& OutGold);

	/**
	 * Generate loot from a loot table struct
	 * @param LootTable - The loot table
	 * @param PlayerLevel - Current player level
	 * @param OutDrops - Generated item drops
	 * @param OutGold - Generated gold amount
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot System")
	static void GenerateLootFromTable(const FLootTable& LootTable, int32 PlayerLevel, TArray<FLootDrop>& OutDrops, int32& OutGold);

	/**
	 * Roll a single loot entry
	 * @param Entry - The loot entry
	 * @param PlayerLevel - Current player level
	 * @param OutDrop - Generated drop (if successful)
	 * @return True if item dropped
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot System")
	static bool RollLootEntry(const FLootEntry& Entry, int32 PlayerLevel, FLootDrop& OutDrop);

	/**
	 * Apply luck modifier to drop chance
	 * @param BaseChance - Original chance (0-1)
	 * @param LuckModifier - Luck stat modifier
	 * @return Modified chance
	 */
	UFUNCTION(BlueprintPure, Category = "Loot System")
	static float ApplyLuckModifier(float BaseChance, float LuckModifier);

	// ============================================
	// MONSTER LOOT TABLES
	// ============================================

	/**
	 * Get loot table ID for a monster type
	 * @param MonsterID - The monster type ID
	 * @return Loot table ID or empty
	 */
	UFUNCTION(BlueprintPure, Category = "Loot System|Monsters")
	static FString GetMonsterLootTableID(const FString& MonsterID);

	/**
	 * Register monster to loot table mapping
	 * @param MonsterID - Monster type ID
	 * @param TableID - Loot table ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot System|Monsters")
	static void RegisterMonsterLootTable(const FString& MonsterID, const FString& TableID);

	/**
	 * Generate loot for a defeated monster
	 * @param MonsterID - The monster type
	 * @param MonsterLevel - Monster's level
	 * @param PlayerLevel - Player's level
	 * @param OutDrops - Generated drops
	 * @param OutGold - Generated gold
	 * @return True if any loot generated
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot System|Monsters")
	static bool GenerateMonsterLoot(const FString& MonsterID, int32 MonsterLevel, int32 PlayerLevel, TArray<FLootDrop>& OutDrops, int32& OutGold);

private:
	static TMap<FString, FLootTable>& GetTableRegistry();
	static TMap<FString, FString>& GetMonsterTableMap();

	static void ProcessLootTable(const FLootTable& Table, int32 PlayerLevel, TArray<FLootDrop>& OutDrops, int32& OutGold, TSet<FString>& ProcessedTables);
	static FLootEntry SelectWeightedRandom(const TArray<FLootEntry>& Entries);
};
