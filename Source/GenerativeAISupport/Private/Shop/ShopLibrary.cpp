// Shop Library Implementation
// Part of GenerativeAISupport Plugin

#include "Shop/ShopLibrary.h"
#include "Inventory/ItemRegistry.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"

TMap<FString, FShopDefinition>& UShopLibrary::GetRegistry()
{
	static TMap<FString, FShopDefinition> Registry;
	return Registry;
}

FShopItem* UShopLibrary::FindShopItem(const FString& ShopID, const FString& ItemID)
{
	FShopDefinition* Shop = GetRegistry().Find(ShopID);
	if (!Shop)
	{
		return nullptr;
	}

	for (FShopItem& Item : Shop->Items)
	{
		if (Item.ItemID == ItemID)
		{
			return &Item;
		}
	}

	return nullptr;
}

// ============================================
// SHOP REGISTRY
// ============================================

void UShopLibrary::RegisterShop(const FShopDefinition& Shop)
{
	if (Shop.ShopID.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot register shop with empty ID"));
		return;
	}

	GetRegistry().Add(Shop.ShopID, Shop);
	UE_LOG(LogTemp, Log, TEXT("Registered shop: %s (%s)"), *Shop.ShopID, *Shop.DisplayName.ToString());
}

void UShopLibrary::RegisterShopAsset(UShopDataAsset* Asset)
{
	if (Asset)
	{
		RegisterShop(Asset->Shop);
	}
}

int32 UShopLibrary::LoadShopsFromJSON(const FString& FilePath)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load shops file: %s"), *FilePath);
		return 0;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse shops file: %s"), *FilePath);
		return 0;
	}

	const TArray<TSharedPtr<FJsonValue>>* ShopsArray;
	if (!Root->TryGetArrayField(TEXT("shops"), ShopsArray))
	{
		UE_LOG(LogTemp, Warning, TEXT("No 'shops' array in file: %s"), *FilePath);
		return 0;
	}

	int32 LoadedCount = 0;

	for (const TSharedPtr<FJsonValue>& ShopVal : *ShopsArray)
	{
		const TSharedPtr<FJsonObject>* ShopObj;
		if (!ShopVal->TryGetObject(ShopObj))
		{
			continue;
		}

		FShopDefinition Shop;

		// Basic fields
		Shop.ShopID = (*ShopObj)->GetStringField(TEXT("id"));
		Shop.DisplayName = FText::FromString((*ShopObj)->GetStringField(TEXT("name")));
		Shop.Description = FText::FromString((*ShopObj)->GetStringField(TEXT("description")));
		Shop.OwnerNPCID = (*ShopObj)->GetStringField(TEXT("owner"));
		Shop.bBuysFromPlayer = (*ShopObj)->GetBoolField(TEXT("buys_from_player"));
		Shop.BuyFromPlayerMultiplier = (*ShopObj)->GetNumberField(TEXT("buy_multiplier"));
		Shop.RestockIntervalHours = (*ShopObj)->GetNumberField(TEXT("restock_hours"));
		Shop.FactionID = (*ShopObj)->GetStringField(TEXT("faction"));

		// Category
		FString CategoryStr = (*ShopObj)->GetStringField(TEXT("category"));
		if (CategoryStr == TEXT("weapons")) Shop.Category = EShopCategory::Weapons;
		else if (CategoryStr == TEXT("armor")) Shop.Category = EShopCategory::Armor;
		else if (CategoryStr == TEXT("consumables")) Shop.Category = EShopCategory::Consumables;
		else if (CategoryStr == TEXT("materials")) Shop.Category = EShopCategory::Materials;
		else if (CategoryStr == TEXT("special")) Shop.Category = EShopCategory::Special;
		else Shop.Category = EShopCategory::General;

		// Accepted sell categories
		const TArray<TSharedPtr<FJsonValue>>* AcceptedArray;
		if ((*ShopObj)->TryGetArrayField(TEXT("accepts"), AcceptedArray))
		{
			for (const TSharedPtr<FJsonValue>& Val : *AcceptedArray)
			{
				Shop.AcceptedSellCategories.Add(Val->AsString());
			}
		}

		// Items
		const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
		if ((*ShopObj)->TryGetArrayField(TEXT("items"), ItemsArray))
		{
			for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArray)
			{
				const TSharedPtr<FJsonObject>* ItemObj;
				if (!ItemVal->TryGetObject(ItemObj))
				{
					continue;
				}

				FShopItem Item;
				Item.ItemID = (*ItemObj)->GetStringField(TEXT("item_id"));
				Item.BuyPrice = (*ItemObj)->GetIntegerField(TEXT("buy_price"));
				Item.SellPrice = (*ItemObj)->GetIntegerField(TEXT("sell_price"));
				Item.Stock = (*ItemObj)->GetIntegerField(TEXT("stock"));
				Item.MaxStock = (*ItemObj)->GetIntegerField(TEXT("max_stock"));
				Item.RequiredLevel = (*ItemObj)->GetIntegerField(TEXT("required_level"));
				Item.RequiredReputation = (*ItemObj)->GetIntegerField(TEXT("required_reputation"));
				Item.bFeatured = (*ItemObj)->GetBoolField(TEXT("featured"));
				Item.bLimitedTime = (*ItemObj)->GetBoolField(TEXT("limited_time"));

				// Currency
				FString CurrencyStr = (*ItemObj)->GetStringField(TEXT("currency"));
				if (CurrencyStr == TEXT("reputation")) Item.Currency = ECurrencyType::Reputation;
				else if (CurrencyStr == TEXT("special")) Item.Currency = ECurrencyType::SpecialCurrency;
				else Item.Currency = ECurrencyType::Gold;

				Shop.Items.Add(Item);
			}
		}

		Shop.LastRestockTime = FDateTime::Now();
		RegisterShop(Shop);
		LoadedCount++;
	}

	UE_LOG(LogTemp, Log, TEXT("Loaded %d shops from %s"), LoadedCount, *FilePath);
	return LoadedCount;
}

bool UShopLibrary::GetShop(const FString& ShopID, FShopDefinition& OutShop)
{
	if (const FShopDefinition* Found = GetRegistry().Find(ShopID))
	{
		OutShop = *Found;
		return true;
	}
	return false;
}

bool UShopLibrary::GetShopByOwner(const FString& NPCID, FShopDefinition& OutShop)
{
	for (const auto& Pair : GetRegistry())
	{
		if (Pair.Value.OwnerNPCID == NPCID)
		{
			OutShop = Pair.Value;
			return true;
		}
	}
	return false;
}

TArray<FShopDefinition> UShopLibrary::GetAllShops()
{
	TArray<FShopDefinition> Result;
	GetRegistry().GenerateValueArray(Result);
	return Result;
}

// ============================================
// BUYING
// ============================================

bool UShopLibrary::BuyItem(const FString& ShopID, const FString& ItemID, int32 Quantity, int32& PlayerGold, FTransactionResult& OutResult)
{
	OutResult.bSuccess = false;
	OutResult.ItemID = ItemID;
	OutResult.Quantity = Quantity;

	FText Reason;
	if (!CanBuyItem(ShopID, ItemID, Quantity, PlayerGold, 1, 0, Reason))
	{
		OutResult.ErrorMessage = Reason;
		return false;
	}

	FShopItem* Item = FindShopItem(ShopID, ItemID);
	if (!Item)
	{
		OutResult.ErrorMessage = FText::FromString(TEXT("Item nicht im Shop gefunden"));
		return false;
	}

	int32 TotalPrice = GetBuyPrice(ShopID, ItemID, 0) * Quantity;

	// Process transaction
	PlayerGold -= TotalPrice;

	// Reduce stock if not unlimited
	if (Item->Stock > 0)
	{
		Item->Stock = FMath::Max(0, Item->Stock - Quantity);
	}

	OutResult.bSuccess = true;
	OutResult.TotalPrice = TotalPrice;
	OutResult.NewBalance = PlayerGold;

	UE_LOG(LogTemp, Log, TEXT("Bought %d x %s for %d gold"), Quantity, *ItemID, TotalPrice);

	return true;
}

bool UShopLibrary::CanBuyItem(const FString& ShopID, const FString& ItemID, int32 Quantity, int32 PlayerGold, int32 PlayerLevel, int32 PlayerReputation, FText& OutReason)
{
	const FShopItem* Item = FindShopItem(ShopID, ItemID);
	if (!Item)
	{
		OutReason = FText::FromString(TEXT("Item nicht verfügbar"));
		return false;
	}

	// Check stock
	if (Item->Stock >= 0 && Item->Stock < Quantity)
	{
		OutReason = FText::FromString(FString::Printf(TEXT("Nur noch %d auf Lager"), Item->Stock));
		return false;
	}

	// Check level
	if (PlayerLevel < Item->RequiredLevel)
	{
		OutReason = FText::FromString(FString::Printf(TEXT("Benötigt Level %d"), Item->RequiredLevel));
		return false;
	}

	// Check reputation
	if (PlayerReputation < Item->RequiredReputation)
	{
		OutReason = FText::FromString(FString::Printf(TEXT("Benötigt %d Reputation"), Item->RequiredReputation));
		return false;
	}

	// Check gold
	int32 TotalPrice = GetBuyPrice(ShopID, ItemID, PlayerReputation) * Quantity;
	if (PlayerGold < TotalPrice)
	{
		OutReason = FText::FromString(FString::Printf(TEXT("Nicht genug Gold (benötigt %d)"), TotalPrice));
		return false;
	}

	return true;
}

int32 UShopLibrary::GetBuyPrice(const FString& ShopID, const FString& ItemID, int32 PlayerReputation)
{
	const FShopItem* Item = FindShopItem(ShopID, ItemID);
	if (!Item)
	{
		return 0;
	}

	int32 Price = Item->BuyPrice;

	// Apply price modifier
	if (Item->PriceModifier == EPriceModifier::Discount || Item->PriceModifier == EPriceModifier::Sale)
	{
		Price = FMath::RoundToInt(Price * (1.f - Item->ModifierPercent));
	}
	else if (Item->PriceModifier == EPriceModifier::Markup)
	{
		Price = FMath::RoundToInt(Price * (1.f + Item->ModifierPercent));
	}

	// Apply reputation discount
	Price = ApplyReputationModifier(Price, PlayerReputation, true);

	return FMath::Max(1, Price);
}

// ============================================
// SELLING
// ============================================

bool UShopLibrary::SellItem(const FString& ShopID, const FString& ItemID, int32 Quantity, int32& PlayerGold, FTransactionResult& OutResult)
{
	OutResult.bSuccess = false;
	OutResult.ItemID = ItemID;
	OutResult.Quantity = Quantity;

	FText Reason;
	if (!ShopWillBuyItem(ShopID, ItemID, Reason))
	{
		OutResult.ErrorMessage = Reason;
		return false;
	}

	int32 TotalPrice = GetSellPrice(ShopID, ItemID, 0) * Quantity;

	PlayerGold += TotalPrice;

	OutResult.bSuccess = true;
	OutResult.TotalPrice = TotalPrice;
	OutResult.NewBalance = PlayerGold;

	UE_LOG(LogTemp, Log, TEXT("Sold %d x %s for %d gold"), Quantity, *ItemID, TotalPrice);

	return true;
}

bool UShopLibrary::ShopWillBuyItem(const FString& ShopID, const FString& ItemID, FText& OutReason)
{
	const FShopDefinition* Shop = GetRegistry().Find(ShopID);
	if (!Shop)
	{
		OutReason = FText::FromString(TEXT("Shop nicht gefunden"));
		return false;
	}

	if (!Shop->bBuysFromPlayer)
	{
		OutReason = FText::FromString(TEXT("Dieser Händler kauft keine Waren"));
		return false;
	}

	// Check if item category is accepted
	FItemDefinition ItemDef;
	if (!UItemRegistry::GetItem(ItemID, ItemDef))
	{
		OutReason = FText::FromString(TEXT("Unbekannter Gegenstand"));
		return false;
	}

	// Check if item is sellable
	if (!ItemDef.bSellable)
	{
		OutReason = FText::FromString(TEXT("Dieser Gegenstand kann nicht verkauft werden"));
		return false;
	}

	// If shop has accepted categories, check them
	if (Shop->AcceptedSellCategories.Num() > 0)
	{
		FString CategoryStr;
		switch (ItemDef.Category)
		{
		case EItemCategory::Weapon: CategoryStr = TEXT("weapon"); break;
		case EItemCategory::Armor: CategoryStr = TEXT("armor"); break;
		case EItemCategory::Consumable: CategoryStr = TEXT("consumable"); break;
		case EItemCategory::Material: CategoryStr = TEXT("material"); break;
		default: CategoryStr = TEXT("misc"); break;
		}

		if (!Shop->AcceptedSellCategories.Contains(CategoryStr) && !Shop->AcceptedSellCategories.Contains(TEXT("all")))
		{
			OutReason = FText::FromString(TEXT("Der Händler interessiert sich nicht für diesen Gegenstand"));
			return false;
		}
	}

	return true;
}

int32 UShopLibrary::GetSellPrice(const FString& ShopID, const FString& ItemID, int32 PlayerReputation)
{
	const FShopDefinition* Shop = GetRegistry().Find(ShopID);
	if (!Shop)
	{
		return 0;
	}

	// Check if item is in shop's inventory (use shop price)
	const FShopItem* ShopItem = FindShopItem(ShopID, ItemID);
	if (ShopItem)
	{
		int32 Price = ShopItem->SellPrice;
		Price = ApplyReputationModifier(Price, PlayerReputation, false);
		return FMath::Max(1, Price);
	}

	// Otherwise use item's base value with shop's multiplier
	FItemDefinition ItemDef;
	if (!UItemRegistry::GetItem(ItemID, ItemDef))
	{
		return 0;
	}

	int32 Price = FMath::RoundToInt(ItemDef.BaseValue * Shop->BuyFromPlayerMultiplier);
	Price = ApplyReputationModifier(Price, PlayerReputation, false);

	return FMath::Max(1, Price);
}

// ============================================
// STOCK MANAGEMENT
// ============================================

int32 UShopLibrary::GetItemStock(const FString& ShopID, const FString& ItemID)
{
	const FShopItem* Item = FindShopItem(ShopID, ItemID);
	return Item ? Item->Stock : 0;
}

bool UShopLibrary::NeedsRestock(const FString& ShopID, const FDateTime& CurrentGameTime)
{
	const FShopDefinition* Shop = GetRegistry().Find(ShopID);
	if (!Shop || Shop->RestockIntervalHours < 0)
	{
		return false;
	}

	FTimespan TimeSinceRestock = CurrentGameTime - Shop->LastRestockTime;
	return TimeSinceRestock.GetTotalHours() >= Shop->RestockIntervalHours;
}

void UShopLibrary::RestockShop(const FString& ShopID, const FDateTime& CurrentGameTime)
{
	FShopDefinition* Shop = GetRegistry().Find(ShopID);
	if (!Shop)
	{
		return;
	}

	for (FShopItem& Item : Shop->Items)
	{
		if (Item.MaxStock > 0)
		{
			Item.Stock = Item.MaxStock;
		}
	}

	Shop->LastRestockTime = CurrentGameTime;

	UE_LOG(LogTemp, Log, TEXT("Restocked shop: %s"), *ShopID);
}

int32 UShopLibrary::RestockAllShops(const FDateTime& CurrentGameTime)
{
	int32 Count = 0;

	for (auto& Pair : GetRegistry())
	{
		if (NeedsRestock(Pair.Key, CurrentGameTime))
		{
			RestockShop(Pair.Key, CurrentGameTime);
			Count++;
		}
	}

	return Count;
}

// ============================================
// SHOP ITEMS QUERY
// ============================================

TArray<FShopItem> UShopLibrary::GetShopItems(const FString& ShopID)
{
	const FShopDefinition* Shop = GetRegistry().Find(ShopID);
	if (Shop)
	{
		return Shop->Items;
	}
	return TArray<FShopItem>();
}

TArray<FShopItem> UShopLibrary::GetFeaturedItems(const FString& ShopID)
{
	TArray<FShopItem> Result;
	const FShopDefinition* Shop = GetRegistry().Find(ShopID);
	if (!Shop)
	{
		return Result;
	}

	for (const FShopItem& Item : Shop->Items)
	{
		if (Item.bFeatured)
		{
			Result.Add(Item);
		}
	}

	return Result;
}

TArray<FShopItem> UShopLibrary::GetItemsOnSale(const FString& ShopID)
{
	TArray<FShopItem> Result;
	const FShopDefinition* Shop = GetRegistry().Find(ShopID);
	if (!Shop)
	{
		return Result;
	}

	for (const FShopItem& Item : Shop->Items)
	{
		if (Item.PriceModifier == EPriceModifier::Discount || Item.PriceModifier == EPriceModifier::Sale)
		{
			Result.Add(Item);
		}
	}

	return Result;
}

TArray<FShopItem> UShopLibrary::GetAffordableItems(const FString& ShopID, int32 PlayerGold, int32 PlayerLevel)
{
	TArray<FShopItem> Result;
	const FShopDefinition* Shop = GetRegistry().Find(ShopID);
	if (!Shop)
	{
		return Result;
	}

	for (const FShopItem& Item : Shop->Items)
	{
		FText Reason;
		if (CanBuyItem(ShopID, Item.ItemID, 1, PlayerGold, PlayerLevel, 0, Reason))
		{
			Result.Add(Item);
		}
	}

	return Result;
}

// ============================================
// SPECIAL OFFERS
// ============================================

void UShopLibrary::SetItemDiscount(const FString& ShopID, const FString& ItemID, float DiscountPercent)
{
	FShopItem* Item = FindShopItem(ShopID, ItemID);
	if (Item)
	{
		Item->PriceModifier = EPriceModifier::Discount;
		Item->ModifierPercent = FMath::Clamp(DiscountPercent, 0.f, 0.9f);
	}
}

void UShopLibrary::SetItemMarkup(const FString& ShopID, const FString& ItemID, float MarkupPercent)
{
	FShopItem* Item = FindShopItem(ShopID, ItemID);
	if (Item)
	{
		Item->PriceModifier = EPriceModifier::Markup;
		Item->ModifierPercent = FMath::Max(0.f, MarkupPercent);
	}
}

void UShopLibrary::ClearPriceModifiers(const FString& ShopID)
{
	FShopDefinition* Shop = GetRegistry().Find(ShopID);
	if (!Shop)
	{
		return;
	}

	for (FShopItem& Item : Shop->Items)
	{
		Item.PriceModifier = EPriceModifier::None;
		Item.ModifierPercent = 0.f;
	}
}

// ============================================
// REPUTATION
// ============================================

int32 UShopLibrary::ApplyReputationModifier(int32 BasePrice, int32 Reputation, bool bIsBuying)
{
	if (Reputation == 0)
	{
		return BasePrice;
	}

	// Every 100 reputation = 1% price change
	float ReputationModifier = Reputation / 10000.f;

	if (bIsBuying)
	{
		// Positive reputation = lower buy prices
		return FMath::RoundToInt(BasePrice * (1.f - ReputationModifier));
	}
	else
	{
		// Positive reputation = higher sell prices
		return FMath::RoundToInt(BasePrice * (1.f + ReputationModifier));
	}
}

// ============================================
// SAVE/LOAD
// ============================================

FString UShopLibrary::SaveShopStatesToJSON()
{
	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);

	TArray<TSharedPtr<FJsonValue>> ShopsArray;

	for (const auto& Pair : GetRegistry())
	{
		TSharedPtr<FJsonObject> ShopObj = MakeShareable(new FJsonObject);
		ShopObj->SetStringField(TEXT("id"), Pair.Key);
		ShopObj->SetStringField(TEXT("last_restock"), Pair.Value.LastRestockTime.ToString());

		// Save stock levels
		TArray<TSharedPtr<FJsonValue>> StockArray;
		for (const FShopItem& Item : Pair.Value.Items)
		{
			TSharedPtr<FJsonObject> ItemObj = MakeShareable(new FJsonObject);
			ItemObj->SetStringField(TEXT("item_id"), Item.ItemID);
			ItemObj->SetNumberField(TEXT("stock"), Item.Stock);
			ItemObj->SetNumberField(TEXT("modifier"), (int32)Item.PriceModifier);
			ItemObj->SetNumberField(TEXT("modifier_percent"), Item.ModifierPercent);
			StockArray.Add(MakeShareable(new FJsonValueObject(ItemObj)));
		}
		ShopObj->SetArrayField(TEXT("items"), StockArray);

		ShopsArray.Add(MakeShareable(new FJsonValueObject(ShopObj)));
	}

	Root->SetArrayField(TEXT("shops"), ShopsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	return OutputString;
}

bool UShopLibrary::LoadShopStatesFromJSON(const FString& JsonString)
{
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* ShopsArray;
	if (!Root->TryGetArrayField(TEXT("shops"), ShopsArray))
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& ShopVal : *ShopsArray)
	{
		const TSharedPtr<FJsonObject>* ShopObj;
		if (!ShopVal->TryGetObject(ShopObj))
		{
			continue;
		}

		FString ShopID = (*ShopObj)->GetStringField(TEXT("id"));
		FShopDefinition* Shop = GetRegistry().Find(ShopID);
		if (!Shop)
		{
			continue;
		}

		FString TimeStr = (*ShopObj)->GetStringField(TEXT("last_restock"));
		FDateTime::Parse(TimeStr, Shop->LastRestockTime);

		const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
		if ((*ShopObj)->TryGetArrayField(TEXT("items"), ItemsArray))
		{
			for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArray)
			{
				const TSharedPtr<FJsonObject>* ItemObj;
				if (!ItemVal->TryGetObject(ItemObj))
				{
					continue;
				}

				FString ItemID = (*ItemObj)->GetStringField(TEXT("item_id"));
				FShopItem* Item = FindShopItem(ShopID, ItemID);
				if (Item)
				{
					Item->Stock = (*ItemObj)->GetIntegerField(TEXT("stock"));
					Item->PriceModifier = (EPriceModifier)(*ItemObj)->GetIntegerField(TEXT("modifier"));
					Item->ModifierPercent = (*ItemObj)->GetNumberField(TEXT("modifier_percent"));
				}
			}
		}
	}

	return true;
}
