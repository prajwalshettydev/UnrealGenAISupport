// Loot System Types - Data structures for loot and drops
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LootTypes.generated.h"

/**
 * Single loot drop entry
 */
USTRUCT(BlueprintType)
struct FLootEntry
{
	GENERATED_BODY()

	/** Item ID to drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	FString ItemID;

	/** Minimum quantity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	int32 MinQuantity = 1;

	/** Maximum quantity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	int32 MaxQuantity = 1;

	/** Drop chance (0.0 - 1.0, where 1.0 = 100%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.0f;

	/** Weight for weighted random selection (higher = more likely) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	float Weight = 1.0f;

	/** Is this a guaranteed drop? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	bool bGuaranteed = false;

	/** Minimum player level to drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	int32 MinPlayerLevel = 1;

	/** Maximum player level to drop (0 = no max) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	int32 MaxPlayerLevel = 0;
};

/**
 * Loot drop result
 */
USTRUCT(BlueprintType)
struct FLootDrop
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	FString ItemID;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	int32 Quantity = 1;
};

/**
 * Currency drop range
 */
USTRUCT(BlueprintType)
struct FCurrencyDrop
{
	GENERATED_BODY()

	/** Minimum gold to drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
	int32 MinGold = 0;

	/** Maximum gold to drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
	int32 MaxGold = 0;

	/** Chance to drop any gold (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.0f;
};

/**
 * Loot Table - Defines what can drop from an enemy/container
 */
USTRUCT(BlueprintType)
struct FLootTable
{
	GENERATED_BODY()

	/** Unique ID for this loot table */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Table")
	FString TableID;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Table")
	FText DisplayName;

	/** Guaranteed drops (always drop) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Table")
	TArray<FLootEntry> GuaranteedDrops;

	/** Random drops (chance-based) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Table")
	TArray<FLootEntry> RandomDrops;

	/** How many random items can drop (min) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Table")
	int32 MinRandomDrops = 0;

	/** How many random items can drop (max) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Table")
	int32 MaxRandomDrops = 3;

	/** Currency drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Table")
	FCurrencyDrop CurrencyDrop;

	/** Other loot tables to include (for combining tables) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Table")
	TArray<FString> IncludedTableIDs;

	/** Level scaling multiplier for quantities (0 = no scaling) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Table")
	float LevelScaling = 0.f;
};

/**
 * Loot Table Data Asset
 */
UCLASS(BlueprintType)
class GENERATIVEAISUPPORT_API ULootTableDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FLootTable LootTable;
};
