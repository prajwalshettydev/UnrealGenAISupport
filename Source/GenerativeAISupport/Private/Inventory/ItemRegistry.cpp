// Item Registry Implementation
// Part of GenerativeAISupport Plugin

#include "Inventory/ItemRegistry.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"

TMap<FString, FItemDefinition>& UItemRegistry::GetRegistry()
{
	static TMap<FString, FItemDefinition> Registry;
	return Registry;
}

void UItemRegistry::RegisterItem(const FItemDefinition& ItemData)
{
	if (ItemData.ItemID.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot register item with empty ID"));
		return;
	}

	GetRegistry().Add(ItemData.ItemID, ItemData);
	UE_LOG(LogTemp, Log, TEXT("Registered item: %s"), *ItemData.ItemID);
}

void UItemRegistry::RegisterItemAsset(UItemDataAsset* ItemAsset)
{
	if (ItemAsset)
	{
		RegisterItem(ItemAsset->ItemData);
	}
}

int32 UItemRegistry::LoadItemsFromJSON(const FString& FilePath)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load items file: %s"), *FilePath);
		return 0;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse items file: %s"), *FilePath);
		return 0;
	}

	const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
	if (!Root->TryGetArrayField(TEXT("items"), ItemsArray))
	{
		UE_LOG(LogTemp, Warning, TEXT("No 'items' array in file: %s"), *FilePath);
		return 0;
	}

	int32 LoadedCount = 0;

	for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArray)
	{
		const TSharedPtr<FJsonObject>* ItemObj;
		if (!ItemVal->TryGetObject(ItemObj))
		{
			continue;
		}

		FItemDefinition Item;

		// Basic fields
		Item.ItemID = (*ItemObj)->GetStringField(TEXT("id"));
		Item.DisplayName = FText::FromString((*ItemObj)->GetStringField(TEXT("name")));
		Item.Description = FText::FromString((*ItemObj)->GetStringField(TEXT("description")));
		Item.MaxStackSize = (*ItemObj)->GetIntegerField(TEXT("stack_size"));
		Item.BaseValue = (*ItemObj)->GetIntegerField(TEXT("value"));
		Item.Weight = (*ItemObj)->GetNumberField(TEXT("weight"));
		Item.RequiredLevel = (*ItemObj)->GetIntegerField(TEXT("level"));
		Item.bSellable = (*ItemObj)->GetBoolField(TEXT("sellable"));
		Item.bDroppable = (*ItemObj)->GetBoolField(TEXT("droppable"));
		Item.bQuestItem = (*ItemObj)->GetBoolField(TEXT("quest_item"));
		Item.UseEffectID = (*ItemObj)->GetStringField(TEXT("use_effect"));

		// Category
		FString CategoryStr = (*ItemObj)->GetStringField(TEXT("category"));
		if (CategoryStr == TEXT("weapon")) Item.Category = EItemCategory::Weapon;
		else if (CategoryStr == TEXT("armor")) Item.Category = EItemCategory::Armor;
		else if (CategoryStr == TEXT("consumable")) Item.Category = EItemCategory::Consumable;
		else if (CategoryStr == TEXT("material")) Item.Category = EItemCategory::Material;
		else if (CategoryStr == TEXT("quest")) Item.Category = EItemCategory::QuestItem;
		else if (CategoryStr == TEXT("key")) Item.Category = EItemCategory::Key;
		else Item.Category = EItemCategory::Misc;

		// Rarity
		FString RarityStr = (*ItemObj)->GetStringField(TEXT("rarity"));
		if (RarityStr == TEXT("uncommon")) Item.Rarity = EItemRarity::Uncommon;
		else if (RarityStr == TEXT("rare")) Item.Rarity = EItemRarity::Rare;
		else if (RarityStr == TEXT("epic")) Item.Rarity = EItemRarity::Epic;
		else if (RarityStr == TEXT("legendary")) Item.Rarity = EItemRarity::Legendary;
		else Item.Rarity = EItemRarity::Common;

		// Equipment slot
		FString SlotStr = (*ItemObj)->GetStringField(TEXT("equip_slot"));
		if (SlotStr == TEXT("main_hand")) Item.EquipSlot = EEquipmentSlot::MainHand;
		else if (SlotStr == TEXT("off_hand")) Item.EquipSlot = EEquipmentSlot::OffHand;
		else if (SlotStr == TEXT("head")) Item.EquipSlot = EEquipmentSlot::Head;
		else if (SlotStr == TEXT("chest")) Item.EquipSlot = EEquipmentSlot::Chest;
		else if (SlotStr == TEXT("hands")) Item.EquipSlot = EEquipmentSlot::Hands;
		else if (SlotStr == TEXT("legs")) Item.EquipSlot = EEquipmentSlot::Legs;
		else if (SlotStr == TEXT("feet")) Item.EquipSlot = EEquipmentSlot::Feet;
		else if (SlotStr == TEXT("accessory1")) Item.EquipSlot = EEquipmentSlot::Accessory1;
		else if (SlotStr == TEXT("accessory2")) Item.EquipSlot = EEquipmentSlot::Accessory2;
		else Item.EquipSlot = EEquipmentSlot::None;

		// Stat bonuses
		const TSharedPtr<FJsonObject>* StatsObj;
		if ((*ItemObj)->TryGetObjectField(TEXT("stats"), StatsObj))
		{
			for (const auto& StatPair : (*StatsObj)->Values)
			{
				Item.StatBonuses.Add(StatPair.Key, StatPair.Value->AsNumber());
			}
		}

		// Icon path
		FString IconPath = (*ItemObj)->GetStringField(TEXT("icon"));
		if (!IconPath.IsEmpty())
		{
			Item.Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconPath));
		}

		// Mesh path
		FString MeshPath = (*ItemObj)->GetStringField(TEXT("mesh"));
		if (!MeshPath.IsEmpty())
		{
			Item.Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath));
		}

		RegisterItem(Item);
		LoadedCount++;
	}

	UE_LOG(LogTemp, Log, TEXT("Loaded %d items from %s"), LoadedCount, *FilePath);
	return LoadedCount;
}

const FItemDefinition* UItemRegistry::GetItemDefinition(const FString& ItemID)
{
	return GetRegistry().Find(ItemID);
}

bool UItemRegistry::GetItem(const FString& ItemID, FItemDefinition& OutItemData)
{
	if (const FItemDefinition* Item = GetItemDefinition(ItemID))
	{
		OutItemData = *Item;
		return true;
	}
	return false;
}

bool UItemRegistry::ItemExists(const FString& ItemID)
{
	return GetRegistry().Contains(ItemID);
}

TArray<FItemDefinition> UItemRegistry::GetAllItems()
{
	TArray<FItemDefinition> Result;
	GetRegistry().GenerateValueArray(Result);
	return Result;
}

TArray<FItemDefinition> UItemRegistry::GetItemsByCategory(EItemCategory Category)
{
	TArray<FItemDefinition> Result;
	for (const auto& Pair : GetRegistry())
	{
		if (Pair.Value.Category == Category)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

TArray<FItemDefinition> UItemRegistry::GetItemsByRarity(EItemRarity Rarity)
{
	TArray<FItemDefinition> Result;
	for (const auto& Pair : GetRegistry())
	{
		if (Pair.Value.Rarity == Rarity)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

void UItemRegistry::ClearRegistry()
{
	GetRegistry().Empty();
}

FLinearColor UItemRegistry::GetRarityColor(EItemRarity Rarity)
{
	switch (Rarity)
	{
	case EItemRarity::Common:
		return FLinearColor(0.8f, 0.8f, 0.8f); // Gray
	case EItemRarity::Uncommon:
		return FLinearColor(0.2f, 0.8f, 0.2f); // Green
	case EItemRarity::Rare:
		return FLinearColor(0.2f, 0.4f, 1.0f); // Blue
	case EItemRarity::Epic:
		return FLinearColor(0.6f, 0.2f, 0.8f); // Purple
	case EItemRarity::Legendary:
		return FLinearColor(1.0f, 0.6f, 0.0f); // Orange
	default:
		return FLinearColor::White;
	}
}
