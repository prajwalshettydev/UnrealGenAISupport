// Inventory Component Implementation
// Part of GenerativeAISupport Plugin

#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemRegistry.h"
#include "JsonObjectConverter.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeSlots();
	Gold = StartingGold;
}

void UInventoryComponent::InitializeSlots()
{
	Slots.SetNum(InventorySize);
	for (FInventorySlot& Slot : Slots)
	{
		Slot.Clear();
	}
}

// ============================================
// ITEM MANAGEMENT
// ============================================

bool UInventoryComponent::AddItem(const FString& ItemID, int32 Quantity, int32& OutRemainder)
{
	if (ItemID.IsEmpty() || Quantity <= 0)
	{
		OutRemainder = Quantity;
		return false;
	}

	const FItemDefinition* ItemDef = GetItemDefinition(ItemID);
	if (!ItemDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unknown item: %s"), *ItemID);
		OutRemainder = Quantity;
		return false;
	}

	int32 RemainingToAdd = Quantity;

	// First, try to stack with existing items
	if (ItemDef->IsStackable())
	{
		for (int32 i = 0; i < Slots.Num() && RemainingToAdd > 0; i++)
		{
			FInventorySlot& Slot = Slots[i];
			if (Slot.ItemID == ItemID && Slot.Quantity < ItemDef->MaxStackSize)
			{
				int32 SpaceInStack = ItemDef->MaxStackSize - Slot.Quantity;
				int32 ToAdd = FMath::Min(SpaceInStack, RemainingToAdd);

				int32 OldQty = Slot.Quantity;
				Slot.Quantity += ToAdd;
				RemainingToAdd -= ToAdd;

				NotifyChange(ItemID, OldQty, Slot.Quantity, i);
			}
		}
	}

	// Then, fill empty slots
	while (RemainingToAdd > 0)
	{
		int32 EmptySlot = GetFirstEmptySlot();
		if (EmptySlot < 0)
		{
			break; // No more space
		}

		int32 ToAdd = ItemDef->IsStackable() ? FMath::Min(ItemDef->MaxStackSize, RemainingToAdd) : 1;

		Slots[EmptySlot].ItemID = ItemID;
		Slots[EmptySlot].Quantity = ToAdd;
		RemainingToAdd -= ToAdd;

		NotifyChange(ItemID, 0, ToAdd, EmptySlot);
	}

	OutRemainder = RemainingToAdd;
	return RemainingToAdd < Quantity;
}

bool UInventoryComponent::AddItemToSlot(int32 SlotIndex, const FString& ItemID, int32 Quantity)
{
	if (!Slots.IsValidIndex(SlotIndex) || ItemID.IsEmpty() || Quantity <= 0)
	{
		return false;
	}

	FInventorySlot& Slot = Slots[SlotIndex];

	// If slot is empty, just add
	if (Slot.IsEmpty())
	{
		Slot.ItemID = ItemID;
		Slot.Quantity = Quantity;
		NotifyChange(ItemID, 0, Quantity, SlotIndex);
		return true;
	}

	// If same item, try to stack
	if (Slot.ItemID == ItemID)
	{
		const FItemDefinition* ItemDef = GetItemDefinition(ItemID);
		if (ItemDef && ItemDef->IsStackable())
		{
			int32 OldQty = Slot.Quantity;
			Slot.Quantity = FMath::Min(Slot.Quantity + Quantity, ItemDef->MaxStackSize);
			NotifyChange(ItemID, OldQty, Slot.Quantity, SlotIndex);
			return true;
		}
	}

	return false;
}

int32 UInventoryComponent::RemoveItem(const FString& ItemID, int32 Quantity)
{
	if (ItemID.IsEmpty() || Quantity <= 0)
	{
		return 0;
	}

	int32 RemainingToRemove = Quantity;
	int32 TotalRemoved = 0;

	for (int32 i = Slots.Num() - 1; i >= 0 && RemainingToRemove > 0; i--)
	{
		FInventorySlot& Slot = Slots[i];
		if (Slot.ItemID == ItemID)
		{
			int32 ToRemove = FMath::Min(Slot.Quantity, RemainingToRemove);
			int32 OldQty = Slot.Quantity;

			Slot.Quantity -= ToRemove;
			RemainingToRemove -= ToRemove;
			TotalRemoved += ToRemove;

			if (Slot.Quantity <= 0)
			{
				Slot.Clear();
			}

			NotifyChange(ItemID, OldQty, Slot.Quantity, i);
		}
	}

	return TotalRemoved;
}

int32 UInventoryComponent::RemoveItemFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return 0;
	}

	FInventorySlot& Slot = Slots[SlotIndex];
	if (Slot.IsEmpty())
	{
		return 0;
	}

	int32 ToRemove = (Quantity <= 0) ? Slot.Quantity : FMath::Min(Quantity, Slot.Quantity);
	int32 OldQty = Slot.Quantity;
	FString ItemID = Slot.ItemID;

	Slot.Quantity -= ToRemove;
	if (Slot.Quantity <= 0)
	{
		Slot.Clear();
	}

	NotifyChange(ItemID, OldQty, Slot.Quantity, SlotIndex);
	return ToRemove;
}

bool UInventoryComponent::MoveItem(int32 FromSlot, int32 ToSlot)
{
	if (!Slots.IsValidIndex(FromSlot) || !Slots.IsValidIndex(ToSlot) || FromSlot == ToSlot)
	{
		return false;
	}

	FInventorySlot& Source = Slots[FromSlot];
	FInventorySlot& Dest = Slots[ToSlot];

	if (Source.IsEmpty())
	{
		return false;
	}

	// If dest is empty, just move
	if (Dest.IsEmpty())
	{
		Dest = Source;
		Source.Clear();
		NotifyChange(Dest.ItemID, 0, Dest.Quantity, ToSlot);
		NotifyChange(Dest.ItemID, Dest.Quantity, 0, FromSlot);
		return true;
	}

	// If same item, try to combine stacks
	if (Source.ItemID == Dest.ItemID)
	{
		const FItemDefinition* ItemDef = GetItemDefinition(Source.ItemID);
		if (ItemDef && ItemDef->IsStackable())
		{
			int32 SpaceInDest = ItemDef->MaxStackSize - Dest.Quantity;
			int32 ToMove = FMath::Min(SpaceInDest, Source.Quantity);

			if (ToMove > 0)
			{
				int32 OldDestQty = Dest.Quantity;
				int32 OldSrcQty = Source.Quantity;

				Dest.Quantity += ToMove;
				Source.Quantity -= ToMove;

				if (Source.Quantity <= 0)
				{
					Source.Clear();
				}

				NotifyChange(Dest.ItemID, OldDestQty, Dest.Quantity, ToSlot);
				NotifyChange(Source.ItemID.IsEmpty() ? Dest.ItemID : Source.ItemID, OldSrcQty, Source.Quantity, FromSlot);
				return true;
			}
		}
	}

	// Swap items
	FInventorySlot Temp = Source;
	Source = Dest;
	Dest = Temp;

	NotifyChange(Dest.ItemID, 0, Dest.Quantity, ToSlot);
	NotifyChange(Source.ItemID, 0, Source.Quantity, FromSlot);

	return true;
}

bool UInventoryComponent::SplitStack(int32 SlotIndex, int32 Amount, int32 TargetSlot)
{
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	FInventorySlot& Source = Slots[SlotIndex];
	if (Source.IsEmpty() || Source.Quantity <= Amount || Amount <= 0)
	{
		return false;
	}

	int32 DestSlot = TargetSlot >= 0 ? TargetSlot : GetFirstEmptySlot();
	if (DestSlot < 0 || !Slots.IsValidIndex(DestSlot) || !Slots[DestSlot].IsEmpty())
	{
		return false;
	}

	int32 OldQty = Source.Quantity;
	FString ItemID = Source.ItemID;

	Slots[DestSlot].ItemID = ItemID;
	Slots[DestSlot].Quantity = Amount;
	Source.Quantity -= Amount;

	NotifyChange(ItemID, 0, Amount, DestSlot);
	NotifyChange(ItemID, OldQty, Source.Quantity, SlotIndex);

	return true;
}

bool UInventoryComponent::UseItem(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	FInventorySlot& Slot = Slots[SlotIndex];
	if (Slot.IsEmpty())
	{
		return false;
	}

	const FItemDefinition* ItemDef = GetItemDefinition(Slot.ItemID);
	if (!ItemDef || !ItemDef->IsConsumable())
	{
		return false;
	}

	// TODO: Apply item effect via UseEffectID
	UE_LOG(LogTemp, Log, TEXT("Used item: %s (Effect: %s)"), *Slot.ItemID, *ItemDef->UseEffectID);

	// Remove one from stack
	RemoveItemFromSlot(SlotIndex, 1);

	return true;
}

bool UInventoryComponent::DropItem(int32 SlotIndex, int32 Quantity)
{
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FInventorySlot& Slot = Slots[SlotIndex];
	if (Slot.IsEmpty())
	{
		return false;
	}

	const FItemDefinition* ItemDef = GetItemDefinition(Slot.ItemID);
	if (ItemDef && !ItemDef->bDroppable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item cannot be dropped: %s"), *Slot.ItemID);
		return false;
	}

	// TODO: Spawn dropped item in world

	RemoveItemFromSlot(SlotIndex, Quantity);
	return true;
}

// ============================================
// QUERIES
// ============================================

FInventorySlot UInventoryComponent::GetSlot(int32 SlotIndex) const
{
	if (Slots.IsValidIndex(SlotIndex))
	{
		return Slots[SlotIndex];
	}
	return FInventorySlot();
}

TArray<FInventorySlot> UInventoryComponent::GetAllSlots() const
{
	return Slots;
}

bool UInventoryComponent::HasItem(const FString& ItemID, int32 MinQuantity) const
{
	return GetItemCount(ItemID) >= MinQuantity;
}

int32 UInventoryComponent::GetItemCount(const FString& ItemID) const
{
	int32 Total = 0;
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.ItemID == ItemID)
		{
			Total += Slot.Quantity;
		}
	}
	return Total;
}

int32 UInventoryComponent::FindItemSlot(const FString& ItemID) const
{
	for (int32 i = 0; i < Slots.Num(); i++)
	{
		if (Slots[i].ItemID == ItemID)
		{
			return i;
		}
	}
	return -1;
}

TArray<int32> UInventoryComponent::FindAllItemSlots(const FString& ItemID) const
{
	TArray<int32> Result;
	for (int32 i = 0; i < Slots.Num(); i++)
	{
		if (Slots[i].ItemID == ItemID)
		{
			Result.Add(i);
		}
	}
	return Result;
}

int32 UInventoryComponent::GetFirstEmptySlot() const
{
	for (int32 i = 0; i < Slots.Num(); i++)
	{
		if (Slots[i].IsEmpty())
		{
			return i;
		}
	}
	return -1;
}

int32 UInventoryComponent::GetEmptySlotCount() const
{
	int32 Count = 0;
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.IsEmpty())
		{
			Count++;
		}
	}
	return Count;
}

bool UInventoryComponent::IsFull() const
{
	return GetFirstEmptySlot() < 0;
}

float UInventoryComponent::GetCurrentWeight() const
{
	float TotalWeight = 0.f;
	for (const FInventorySlot& Slot : Slots)
	{
		if (!Slot.IsEmpty())
		{
			if (const FItemDefinition* ItemDef = GetItemDefinition(Slot.ItemID))
			{
				TotalWeight += ItemDef->Weight * Slot.Quantity;
			}
		}
	}
	return TotalWeight;
}

bool UInventoryComponent::CanAddItem(const FString& ItemID, int32 Quantity) const
{
	const FItemDefinition* ItemDef = GetItemDefinition(ItemID);
	if (!ItemDef)
	{
		return false;
	}

	// Check weight
	if (MaxWeight > 0)
	{
		float AdditionalWeight = ItemDef->Weight * Quantity;
		if (GetCurrentWeight() + AdditionalWeight > MaxWeight)
		{
			return false;
		}
	}

	// Count available space
	int32 AvailableSpace = 0;

	// Space in existing stacks
	if (ItemDef->IsStackable())
	{
		for (const FInventorySlot& Slot : Slots)
		{
			if (Slot.ItemID == ItemID)
			{
				AvailableSpace += ItemDef->MaxStackSize - Slot.Quantity;
			}
		}
	}

	// Space in empty slots
	int32 EmptySlots = GetEmptySlotCount();
	if (ItemDef->IsStackable())
	{
		AvailableSpace += EmptySlots * ItemDef->MaxStackSize;
	}
	else
	{
		AvailableSpace += EmptySlots;
	}

	return AvailableSpace >= Quantity;
}

// ============================================
// EQUIPMENT
// ============================================

bool UInventoryComponent::EquipItemFromSlot(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FInventorySlot& Slot = Slots[SlotIndex];
	if (Slot.IsEmpty())
	{
		return false;
	}

	const FItemDefinition* ItemDef = GetItemDefinition(Slot.ItemID);
	if (!ItemDef || !ItemDef->IsEquippable())
	{
		return false;
	}

	EEquipmentSlot EquipSlot = ItemDef->EquipSlot;

	// Unequip current item in slot (if any)
	if (Equipment.Contains(EquipSlot))
	{
		UnequipItem(EquipSlot);
	}

	// Equip new item
	FString ItemID = Slot.ItemID;
	Equipment.Add(EquipSlot, ItemID);

	// Remove from inventory
	RemoveItemFromSlot(SlotIndex, 1);

	OnItemEquipped.Broadcast(EquipSlot, ItemID);

	UE_LOG(LogTemp, Log, TEXT("Equipped %s to slot %d"), *ItemID, (int32)EquipSlot);
	return true;
}

bool UInventoryComponent::UnequipItem(EEquipmentSlot Slot)
{
	FString* ItemID = Equipment.Find(Slot);
	if (!ItemID || ItemID->IsEmpty())
	{
		return false;
	}

	// Add back to inventory
	int32 Remainder;
	if (!AddItem(*ItemID, 1, Remainder))
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot unequip - inventory full"));
		return false;
	}

	FString UnequippedID = *ItemID;
	Equipment.Remove(Slot);

	OnItemUnequipped.Broadcast(Slot, UnequippedID);

	UE_LOG(LogTemp, Log, TEXT("Unequipped %s from slot %d"), *UnequippedID, (int32)Slot);
	return true;
}

FString UInventoryComponent::GetEquippedItem(EEquipmentSlot Slot) const
{
	const FString* ItemID = Equipment.Find(Slot);
	return ItemID ? *ItemID : FString();
}

TMap<EEquipmentSlot, FString> UInventoryComponent::GetAllEquippedItems() const
{
	return Equipment;
}

TMap<FString, float> UInventoryComponent::GetEquipmentStatBonuses() const
{
	TMap<FString, float> TotalBonuses;

	for (const auto& Pair : Equipment)
	{
		if (const FItemDefinition* ItemDef = GetItemDefinition(Pair.Value))
		{
			for (const auto& StatPair : ItemDef->StatBonuses)
			{
				float& Value = TotalBonuses.FindOrAdd(StatPair.Key);
				Value += StatPair.Value;
			}
		}
	}

	return TotalBonuses;
}

// ============================================
// GOLD
// ============================================

void UInventoryComponent::AddGold(int32 Amount)
{
	if (Amount > 0)
	{
		int32 OldGold = Gold;
		Gold += Amount;
		OnGoldChanged.Broadcast(OldGold, Gold);
	}
}

bool UInventoryComponent::RemoveGold(int32 Amount)
{
	if (Amount > 0 && Gold >= Amount)
	{
		int32 OldGold = Gold;
		Gold -= Amount;
		OnGoldChanged.Broadcast(OldGold, Gold);
		return true;
	}
	return false;
}

// ============================================
// SAVE/LOAD
// ============================================

FString UInventoryComponent::SaveToJSON() const
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	// Slots
	TArray<TSharedPtr<FJsonValue>> SlotsArray;
	for (int32 i = 0; i < Slots.Num(); i++)
	{
		const FInventorySlot& Slot = Slots[i];
		if (!Slot.IsEmpty())
		{
			TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
			SlotObj->SetNumberField(TEXT("index"), i);
			SlotObj->SetStringField(TEXT("item"), Slot.ItemID);
			SlotObj->SetNumberField(TEXT("quantity"), Slot.Quantity);
			SlotsArray.Add(MakeShared<FJsonValueObject>(SlotObj));
		}
	}
	Root->SetArrayField(TEXT("slots"), SlotsArray);

	// Equipment
	TSharedPtr<FJsonObject> EquipObj = MakeShared<FJsonObject>();
	for (const auto& Pair : Equipment)
	{
		EquipObj->SetStringField(FString::FromInt((int32)Pair.Key), Pair.Value);
	}
	Root->SetObjectField(TEXT("equipment"), EquipObj);

	// Gold
	Root->SetNumberField(TEXT("gold"), Gold);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	return OutputString;
}

bool UInventoryComponent::LoadFromJSON(const FString& JSONString)
{
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	// Clear current state
	ClearInventory();

	// Load slots
	const TArray<TSharedPtr<FJsonValue>>* SlotsArray;
	if (Root->TryGetArrayField(TEXT("slots"), SlotsArray))
	{
		for (const TSharedPtr<FJsonValue>& SlotVal : *SlotsArray)
		{
			const TSharedPtr<FJsonObject>* SlotObj;
			if (SlotVal->TryGetObject(SlotObj))
			{
				int32 Index = (*SlotObj)->GetIntegerField(TEXT("index"));
				FString ItemID = (*SlotObj)->GetStringField(TEXT("item"));
				int32 Quantity = (*SlotObj)->GetIntegerField(TEXT("quantity"));

				if (Slots.IsValidIndex(Index))
				{
					Slots[Index].ItemID = ItemID;
					Slots[Index].Quantity = Quantity;
				}
			}
		}
	}

	// Load equipment
	const TSharedPtr<FJsonObject>* EquipObj;
	if (Root->TryGetObjectField(TEXT("equipment"), EquipObj))
	{
		for (const auto& Pair : (*EquipObj)->Values)
		{
			int32 SlotIndex = FCString::Atoi(*Pair.Key);
			FString ItemID = Pair.Value->AsString();
			Equipment.Add((EEquipmentSlot)SlotIndex, ItemID);
		}
	}

	// Load gold
	Gold = Root->GetIntegerField(TEXT("gold"));

	return true;
}

void UInventoryComponent::ClearInventory()
{
	for (FInventorySlot& Slot : Slots)
	{
		Slot.Clear();
	}
	Equipment.Empty();
	Gold = StartingGold;
}

// ============================================
// HELPERS
// ============================================

void UInventoryComponent::NotifyChange(const FString& ItemID, int32 OldQty, int32 NewQty, int32 SlotIndex)
{
	FInventoryChangeInfo Info;
	Info.ItemID = ItemID;
	Info.OldQuantity = OldQty;
	Info.NewQuantity = NewQty;
	Info.SlotIndex = SlotIndex;

	OnInventoryChanged.Broadcast(Info);
}

const FItemDefinition* UInventoryComponent::GetItemDefinition(const FString& ItemID) const
{
	// Get from item registry
	return UItemRegistry::GetItemDefinition(ItemID);
}
