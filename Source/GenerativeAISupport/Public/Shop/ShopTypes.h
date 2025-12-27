// Shop System Types - Data structures for shops and trading
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShopTypes.generated.h"

/**
 * Shop category
 */
UENUM(BlueprintType)
enum class EShopCategory : uint8
{
	General		UMETA(DisplayName = "General"),
	Weapons		UMETA(DisplayName = "Weapons"),
	Armor		UMETA(DisplayName = "Armor"),
	Consumables	UMETA(DisplayName = "Consumables"),
	Materials	UMETA(DisplayName = "Materials"),
	Special		UMETA(DisplayName = "Special")
};

/**
 * Currency type
 */
UENUM(BlueprintType)
enum class ECurrencyType : uint8
{
	Gold			UMETA(DisplayName = "Gold"),
	Reputation		UMETA(DisplayName = "Reputation"),
	SpecialCurrency	UMETA(DisplayName = "Special Currency")
};

/**
 * Price modification type
 */
UENUM(BlueprintType)
enum class EPriceModifier : uint8
{
	None		UMETA(DisplayName = "None"),
	Discount	UMETA(DisplayName = "Discount"),
	Markup		UMETA(DisplayName = "Markup"),
	Sale		UMETA(DisplayName = "Sale")
};

/**
 * Shop item entry
 */
USTRUCT(BlueprintType)
struct FShopItem
{
	GENERATED_BODY()

	/** Item ID from registry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Item")
	FString ItemID;

	/** Buy price (what player pays) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Item")
	int32 BuyPrice = 0;

	/** Sell price (what shop pays player) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Item")
	int32 SellPrice = 0;

	/** Currency type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Item")
	ECurrencyType Currency = ECurrencyType::Gold;

	/** Available stock (-1 = unlimited) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Item")
	int32 Stock = -1;

	/** Maximum stock (for restocking) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Item")
	int32 MaxStock = -1;

	/** Required player level to buy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Item")
	int32 RequiredLevel = 1;

	/** Required reputation with shop/faction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Item")
	int32 RequiredReputation = 0;

	/** Is this a featured/highlighted item? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Item")
	bool bFeatured = false;

	/** Is this a limited time item? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Item")
	bool bLimitedTime = false;

	/** Current price modifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Item")
	EPriceModifier PriceModifier = EPriceModifier::None;

	/** Discount/Markup percentage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Item")
	float ModifierPercent = 0.f;
};

/**
 * Shop definition
 */
USTRUCT(BlueprintType)
struct FShopDefinition
{
	GENERATED_BODY()

	/** Unique shop ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	FString ShopID;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	FText DisplayName;

	/** Shop description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	FText Description;

	/** Shop owner NPC ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	FString OwnerNPCID;

	/** Shop category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	EShopCategory Category = EShopCategory::General;

	/** Items for sale */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	TArray<FShopItem> Items;

	/** Does this shop buy items from player? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	bool bBuysFromPlayer = true;

	/** Buy price multiplier (0.5 = pays 50% of item value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	float BuyFromPlayerMultiplier = 0.5f;

	/** Categories of items shop will buy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	TArray<FString> AcceptedSellCategories;

	/** Restock interval in game hours (-1 = never) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	float RestockIntervalHours = 24.f;

	/** Last restock timestamp */
	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	FDateTime LastRestockTime;

	/** Faction/reputation ID associated with this shop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	FString FactionID;
};

/**
 * Transaction result
 */
USTRUCT(BlueprintType)
struct FTransactionResult
{
	GENERATED_BODY()

	/** Was the transaction successful? */
	UPROPERTY(BlueprintReadOnly, Category = "Transaction")
	bool bSuccess = false;

	/** Error message if failed */
	UPROPERTY(BlueprintReadOnly, Category = "Transaction")
	FText ErrorMessage;

	/** Item ID involved */
	UPROPERTY(BlueprintReadOnly, Category = "Transaction")
	FString ItemID;

	/** Quantity bought/sold */
	UPROPERTY(BlueprintReadOnly, Category = "Transaction")
	int32 Quantity = 0;

	/** Total price paid/received */
	UPROPERTY(BlueprintReadOnly, Category = "Transaction")
	int32 TotalPrice = 0;

	/** New gold balance */
	UPROPERTY(BlueprintReadOnly, Category = "Transaction")
	int32 NewBalance = 0;
};

/**
 * Shop Data Asset
 */
UCLASS(BlueprintType)
class GENERATIVEAISUPPORT_API UShopDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FShopDefinition Shop;
};
