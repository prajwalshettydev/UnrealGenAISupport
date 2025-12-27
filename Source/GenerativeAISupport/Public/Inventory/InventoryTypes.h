// Inventory System Types - Data structures for Items and Inventory
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InventoryTypes.generated.h"

/**
 * Item Category
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	None		UMETA(DisplayName = "None"),
	Weapon		UMETA(DisplayName = "Weapon"),
	Armor		UMETA(DisplayName = "Armor"),
	Consumable	UMETA(DisplayName = "Consumable"),
	Material	UMETA(DisplayName = "Material"),
	QuestItem	UMETA(DisplayName = "Quest Item"),
	Key			UMETA(DisplayName = "Key"),
	Misc		UMETA(DisplayName = "Miscellaneous")
};

/**
 * Item Rarity
 */
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common		UMETA(DisplayName = "Common"),
	Uncommon	UMETA(DisplayName = "Uncommon"),
	Rare		UMETA(DisplayName = "Rare"),
	Epic		UMETA(DisplayName = "Epic"),
	Legendary	UMETA(DisplayName = "Legendary")
};

/**
 * Equipment Slot
 */
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None		UMETA(DisplayName = "None"),
	MainHand	UMETA(DisplayName = "Main Hand"),
	OffHand		UMETA(DisplayName = "Off Hand"),
	Head		UMETA(DisplayName = "Head"),
	Chest		UMETA(DisplayName = "Chest"),
	Hands		UMETA(DisplayName = "Hands"),
	Legs		UMETA(DisplayName = "Legs"),
	Feet		UMETA(DisplayName = "Feet"),
	Accessory1	UMETA(DisplayName = "Accessory 1"),
	Accessory2	UMETA(DisplayName = "Accessory 2")
};

/**
 * Item Definition - Base item data (static, from data assets)
 */
USTRUCT(BlueprintType)
struct FItemDefinition
{
	GENERATED_BODY()

	/** Unique Item ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FString ItemID;

	/** Display Name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	/** Description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText Description;

	/** Category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemCategory Category = EItemCategory::Misc;

	/** Rarity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemRarity Rarity = EItemRarity::Common;

	/** Equipment slot (if equippable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EEquipmentSlot EquipSlot = EEquipmentSlot::None;

	/** Icon texture path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 3D mesh for world/preview */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** Maximum stack size (1 = not stackable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 MaxStackSize = 1;

	/** Base value in gold */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 BaseValue = 0;

	/** Weight per unit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float Weight = 0.1f;

	/** Required level to use/equip */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 RequiredLevel = 1;

	/** Can be sold to merchants */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bSellable = true;

	/** Can be dropped/destroyed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bDroppable = true;

	/** Is this a quest item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bQuestItem = false;

	/** Stat bonuses when equipped (StatName -> Value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stats")
	TMap<FString, float> StatBonuses;

	/** Effect to apply when used (for consumables) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effects")
	FString UseEffectID;

	/** Sound to play when used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effects")
	TSoftObjectPtr<USoundBase> UseSound;

	bool IsStackable() const { return MaxStackSize > 1; }
	bool IsEquippable() const { return EquipSlot != EEquipmentSlot::None; }
	bool IsConsumable() const { return Category == EItemCategory::Consumable; }
};

/**
 * Inventory Slot - A slot in the inventory (item + quantity)
 */
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	/** Item ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	FString ItemID;

	/** Quantity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	int32 Quantity = 0;

	/** Is this slot empty */
	bool IsEmpty() const { return ItemID.IsEmpty() || Quantity <= 0; }

	/** Clear the slot */
	void Clear()
	{
		ItemID.Empty();
		Quantity = 0;
	}
};

/**
 * Item Instance - Runtime item with possible modifications
 */
USTRUCT(BlueprintType)
struct FItemInstance
{
	GENERATED_BODY()

	/** Base item ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
	FString ItemID;

	/** Unique instance ID (for tracking modified items) */
	UPROPERTY(BlueprintReadOnly, Category = "Instance")
	FGuid InstanceID;

	/** Current durability (if applicable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
	float Durability = 100.f;

	/** Additional stat modifiers (enchantments, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
	TMap<FString, float> BonusStats;

	/** Custom name (if renamed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
	FText CustomName;

	FItemInstance()
	{
		InstanceID = FGuid::NewGuid();
	}
};

/**
 * Inventory Changed Event Data
 */
USTRUCT(BlueprintType)
struct FInventoryChangeInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Change")
	FString ItemID;

	UPROPERTY(BlueprintReadOnly, Category = "Change")
	int32 OldQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Change")
	int32 NewQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Change")
	int32 SlotIndex = -1;

	int32 GetDelta() const { return NewQuantity - OldQuantity; }
};

/**
 * Item Data Asset - For defining items in the editor
 */
UCLASS(BlueprintType)
class GENERATIVEAISUPPORT_API UItemDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FItemDefinition ItemData;
};
