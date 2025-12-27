// Inventory Component - Manages player/NPC inventory
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/InventoryTypes.h"
#include "InventoryComponent.generated.h"

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, const FInventoryChangeInfo&, ChangeInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemEquipped, EEquipmentSlot, Slot, const FString&, ItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUnequipped, EEquipmentSlot, Slot, const FString&, ItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGoldChanged, int32, OldAmount, int32, NewAmount);

/**
 * Inventory Component
 * Add to any actor that should have an inventory (Player, NPC, Chest, etc.)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), BlueprintType)
class GENERATIVEAISUPPORT_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// ============================================
	// EVENTS
	// ============================================

	/** Called when inventory content changes */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	/** Called when an item is equipped */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemEquipped OnItemEquipped;

	/** Called when an item is unequipped */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemUnequipped OnItemUnequipped;

	/** Called when gold amount changes */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnGoldChanged OnGoldChanged;

	// ============================================
	// CONFIGURATION
	// ============================================

	/** Number of inventory slots */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Config")
	int32 InventorySize = 30;

	/** Maximum weight capacity (0 = unlimited) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Config")
	float MaxWeight = 0.f;

	/** Starting gold */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Config")
	int32 StartingGold = 0;

	// ============================================
	// ITEM MANAGEMENT
	// ============================================

	/**
	 * Add item to inventory
	 * @param ItemID - Item to add
	 * @param Quantity - Amount to add
	 * @param OutRemainder - Items that couldn't be added (no space)
	 * @return True if at least some items were added
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FString& ItemID, int32 Quantity, int32& OutRemainder);

	/**
	 * Add item to specific slot
	 * @param SlotIndex - Target slot
	 * @param ItemID - Item to add
	 * @param Quantity - Amount to add
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemToSlot(int32 SlotIndex, const FString& ItemID, int32 Quantity);

	/**
	 * Remove item from inventory
	 * @param ItemID - Item to remove
	 * @param Quantity - Amount to remove
	 * @return Actual amount removed
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveItem(const FString& ItemID, int32 Quantity);

	/**
	 * Remove item from specific slot
	 * @param SlotIndex - Slot to remove from
	 * @param Quantity - Amount to remove (0 = all)
	 * @return Actual amount removed
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveItemFromSlot(int32 SlotIndex, int32 Quantity = 0);

	/**
	 * Move item between slots
	 * @param FromSlot - Source slot
	 * @param ToSlot - Destination slot
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItem(int32 FromSlot, int32 ToSlot);

	/**
	 * Split a stack
	 * @param SlotIndex - Slot to split
	 * @param Amount - Amount to split off
	 * @param TargetSlot - Destination slot (-1 = first empty)
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SplitStack(int32 SlotIndex, int32 Amount, int32 TargetSlot = -1);

	/**
	 * Use/consume an item
	 * @param SlotIndex - Slot of item to use
	 * @return True if item was used
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseItem(int32 SlotIndex);

	/**
	 * Drop item from inventory
	 * @param SlotIndex - Slot to drop
	 * @param Quantity - Amount to drop (0 = all)
	 * @return True if dropped
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DropItem(int32 SlotIndex, int32 Quantity = 0);

	// ============================================
	// QUERIES
	// ============================================

	/** Get slot at index */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventorySlot GetSlot(int32 SlotIndex) const;

	/** Get all slots */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventorySlot> GetAllSlots() const;

	/** Check if has item */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(const FString& ItemID, int32 MinQuantity = 1) const;

	/** Get total quantity of item */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(const FString& ItemID) const;

	/** Find first slot with item */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 FindItemSlot(const FString& ItemID) const;

	/** Find all slots with item */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<int32> FindAllItemSlots(const FString& ItemID) const;

	/** Get first empty slot */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetFirstEmptySlot() const;

	/** Get number of empty slots */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetEmptySlotCount() const;

	/** Check if inventory is full */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsFull() const;

	/** Get current total weight */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetCurrentWeight() const;

	/** Check if can add item (has space) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAddItem(const FString& ItemID, int32 Quantity) const;

	// ============================================
	// EQUIPMENT
	// ============================================

	/**
	 * Equip item from inventory slot
	 * @param SlotIndex - Inventory slot with item to equip
	 * @return True if equipped
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment")
	bool EquipItemFromSlot(int32 SlotIndex);

	/**
	 * Unequip item from equipment slot
	 * @param Slot - Equipment slot to unequip
	 * @return True if unequipped
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment")
	bool UnequipItem(EEquipmentSlot Slot);

	/**
	 * Get equipped item in slot
	 * @param Slot - Equipment slot to check
	 * @return Item ID or empty
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Equipment")
	FString GetEquippedItem(EEquipmentSlot Slot) const;

	/**
	 * Get all equipped items
	 * @return Map of slot to item ID
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Equipment")
	TMap<EEquipmentSlot, FString> GetAllEquippedItems() const;

	/**
	 * Get total stat bonuses from equipment
	 * @return Map of stat name to total bonus
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Equipment")
	TMap<FString, float> GetEquipmentStatBonuses() const;

	// ============================================
	// GOLD/CURRENCY
	// ============================================

	/** Get current gold */
	UFUNCTION(BlueprintPure, Category = "Inventory|Gold")
	int32 GetGold() const { return Gold; }

	/** Add gold */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Gold")
	void AddGold(int32 Amount);

	/** Remove gold */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Gold")
	bool RemoveGold(int32 Amount);

	/** Check if has enough gold */
	UFUNCTION(BlueprintPure, Category = "Inventory|Gold")
	bool HasGold(int32 Amount) const { return Gold >= Amount; }

	// ============================================
	// SAVE/LOAD
	// ============================================

	/** Serialize inventory to JSON */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Save")
	FString SaveToJSON() const;

	/** Load inventory from JSON */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Save")
	bool LoadFromJSON(const FString& JSONString);

	/** Clear entire inventory */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventory();

protected:
	virtual void BeginPlay() override;

	/** Inventory slots */
	UPROPERTY()
	TArray<FInventorySlot> Slots;

	/** Equipped items */
	UPROPERTY()
	TMap<EEquipmentSlot, FString> Equipment;

	/** Current gold */
	UPROPERTY()
	int32 Gold = 0;

	/** Initialize slots */
	void InitializeSlots();

	/** Broadcast inventory change */
	void NotifyChange(const FString& ItemID, int32 OldQty, int32 NewQty, int32 SlotIndex);

	/** Get item definition from registry */
	const FItemDefinition* GetItemDefinition(const FString& ItemID) const;
};
