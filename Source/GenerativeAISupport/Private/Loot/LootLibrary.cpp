// Loot Library Implementation
// Part of GenerativeAISupport Plugin

#include "Loot/LootLibrary.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"

TMap<FString, FLootTable>& ULootLibrary::GetTableRegistry()
{
	static TMap<FString, FLootTable> Registry;
	return Registry;
}

TMap<FString, FString>& ULootLibrary::GetMonsterTableMap()
{
	static TMap<FString, FString> MonsterMap;
	return MonsterMap;
}

// ============================================
// LOOT TABLE REGISTRY
// ============================================

void ULootLibrary::RegisterLootTable(const FLootTable& LootTable)
{
	if (LootTable.TableID.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot register loot table with empty ID"));
		return;
	}

	GetTableRegistry().Add(LootTable.TableID, LootTable);
	UE_LOG(LogTemp, Log, TEXT("Registered loot table: %s"), *LootTable.TableID);
}

void ULootLibrary::RegisterLootTableAsset(ULootTableDataAsset* Asset)
{
	if (Asset)
	{
		RegisterLootTable(Asset->LootTable);
	}
}

int32 ULootLibrary::LoadLootTablesFromJSON(const FString& FilePath)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load loot tables file: %s"), *FilePath);
		return 0;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse loot tables file: %s"), *FilePath);
		return 0;
	}

	int32 LoadedCount = 0;

	// Load loot tables
	const TArray<TSharedPtr<FJsonValue>>* TablesArray;
	if (Root->TryGetArrayField(TEXT("loot_tables"), TablesArray))
	{
		for (const TSharedPtr<FJsonValue>& TableVal : *TablesArray)
		{
			const TSharedPtr<FJsonObject>* TableObj;
			if (!TableVal->TryGetObject(TableObj))
			{
				continue;
			}

			FLootTable Table;
			Table.TableID = (*TableObj)->GetStringField(TEXT("id"));
			Table.DisplayName = FText::FromString((*TableObj)->GetStringField(TEXT("name")));
			Table.MinRandomDrops = (*TableObj)->GetIntegerField(TEXT("min_drops"));
			Table.MaxRandomDrops = (*TableObj)->GetIntegerField(TEXT("max_drops"));
			Table.LevelScaling = (*TableObj)->GetNumberField(TEXT("level_scaling"));

			// Currency
			const TSharedPtr<FJsonObject>* CurrencyObj;
			if ((*TableObj)->TryGetObjectField(TEXT("currency"), CurrencyObj))
			{
				Table.CurrencyDrop.MinGold = (*CurrencyObj)->GetIntegerField(TEXT("min"));
				Table.CurrencyDrop.MaxGold = (*CurrencyObj)->GetIntegerField(TEXT("max"));
				Table.CurrencyDrop.DropChance = (*CurrencyObj)->GetNumberField(TEXT("chance"));
			}

			// Guaranteed drops
			const TArray<TSharedPtr<FJsonValue>>* GuaranteedArray;
			if ((*TableObj)->TryGetArrayField(TEXT("guaranteed"), GuaranteedArray))
			{
				for (const TSharedPtr<FJsonValue>& EntryVal : *GuaranteedArray)
				{
					const TSharedPtr<FJsonObject>* EntryObj;
					if (EntryVal->TryGetObject(EntryObj))
					{
						FLootEntry Entry;
						Entry.ItemID = (*EntryObj)->GetStringField(TEXT("item"));
						Entry.MinQuantity = (*EntryObj)->GetIntegerField(TEXT("min_qty"));
						Entry.MaxQuantity = (*EntryObj)->GetIntegerField(TEXT("max_qty"));
						Entry.bGuaranteed = true;
						Entry.DropChance = 1.0f;
						Table.GuaranteedDrops.Add(Entry);
					}
				}
			}

			// Random drops
			const TArray<TSharedPtr<FJsonValue>>* RandomArray;
			if ((*TableObj)->TryGetArrayField(TEXT("random"), RandomArray))
			{
				for (const TSharedPtr<FJsonValue>& EntryVal : *RandomArray)
				{
					const TSharedPtr<FJsonObject>* EntryObj;
					if (EntryVal->TryGetObject(EntryObj))
					{
						FLootEntry Entry;
						Entry.ItemID = (*EntryObj)->GetStringField(TEXT("item"));
						Entry.MinQuantity = (*EntryObj)->GetIntegerField(TEXT("min_qty"));
						Entry.MaxQuantity = (*EntryObj)->GetIntegerField(TEXT("max_qty"));
						Entry.DropChance = (*EntryObj)->GetNumberField(TEXT("chance"));
						Entry.Weight = (*EntryObj)->GetNumberField(TEXT("weight"));
						Entry.MinPlayerLevel = (*EntryObj)->GetIntegerField(TEXT("min_level"));
						Entry.MaxPlayerLevel = (*EntryObj)->GetIntegerField(TEXT("max_level"));
						Entry.bGuaranteed = false;
						Table.RandomDrops.Add(Entry);
					}
				}
			}

			// Included tables
			const TArray<TSharedPtr<FJsonValue>>* IncludesArray;
			if ((*TableObj)->TryGetArrayField(TEXT("includes"), IncludesArray))
			{
				for (const TSharedPtr<FJsonValue>& Val : *IncludesArray)
				{
					Table.IncludedTableIDs.Add(Val->AsString());
				}
			}

			RegisterLootTable(Table);
			LoadedCount++;
		}
	}

	// Load monster mappings
	const TSharedPtr<FJsonObject>* MonstersObj;
	if (Root->TryGetObjectField(TEXT("monster_tables"), MonstersObj))
	{
		for (const auto& Pair : (*MonstersObj)->Values)
		{
			RegisterMonsterLootTable(Pair.Key, Pair.Value->AsString());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Loaded %d loot tables from %s"), LoadedCount, *FilePath);
	return LoadedCount;
}

bool ULootLibrary::GetLootTable(const FString& TableID, FLootTable& OutTable)
{
	if (const FLootTable* Found = GetTableRegistry().Find(TableID))
	{
		OutTable = *Found;
		return true;
	}
	return false;
}

// ============================================
// LOOT GENERATION
// ============================================

bool ULootLibrary::GenerateLoot(const FString& TableID, int32 PlayerLevel, TArray<FLootDrop>& OutDrops, int32& OutGold)
{
	OutDrops.Empty();
	OutGold = 0;

	FLootTable Table;
	if (!GetLootTable(TableID, Table))
	{
		UE_LOG(LogTemp, Warning, TEXT("Loot table not found: %s"), *TableID);
		return false;
	}

	GenerateLootFromTable(Table, PlayerLevel, OutDrops, OutGold);
	return true;
}

void ULootLibrary::GenerateLootFromTable(const FLootTable& LootTable, int32 PlayerLevel, TArray<FLootDrop>& OutDrops, int32& OutGold)
{
	TSet<FString> ProcessedTables;
	ProcessLootTable(LootTable, PlayerLevel, OutDrops, OutGold, ProcessedTables);
}

void ULootLibrary::ProcessLootTable(const FLootTable& Table, int32 PlayerLevel, TArray<FLootDrop>& OutDrops, int32& OutGold, TSet<FString>& ProcessedTables)
{
	// Prevent infinite recursion
	if (ProcessedTables.Contains(Table.TableID))
	{
		return;
	}
	ProcessedTables.Add(Table.TableID);

	// Process guaranteed drops
	for (const FLootEntry& Entry : Table.GuaranteedDrops)
	{
		FLootDrop Drop;
		if (RollLootEntry(Entry, PlayerLevel, Drop))
		{
			// Apply level scaling
			if (Table.LevelScaling > 0)
			{
				Drop.Quantity = FMath::Max(1, FMath::RoundToInt(Drop.Quantity * (1.0f + Table.LevelScaling * PlayerLevel)));
			}
			OutDrops.Add(Drop);
		}
	}

	// Determine how many random drops
	int32 NumRandomDrops = FMath::RandRange(Table.MinRandomDrops, Table.MaxRandomDrops);

	// Filter valid random drops by level
	TArray<FLootEntry> ValidRandomDrops;
	for (const FLootEntry& Entry : Table.RandomDrops)
	{
		if (PlayerLevel >= Entry.MinPlayerLevel)
		{
			if (Entry.MaxPlayerLevel == 0 || PlayerLevel <= Entry.MaxPlayerLevel)
			{
				ValidRandomDrops.Add(Entry);
			}
		}
	}

	// Roll random drops
	for (int32 i = 0; i < NumRandomDrops && ValidRandomDrops.Num() > 0; i++)
	{
		// Select weighted random entry
		FLootEntry SelectedEntry = SelectWeightedRandom(ValidRandomDrops);

		FLootDrop Drop;
		if (RollLootEntry(SelectedEntry, PlayerLevel, Drop))
		{
			if (Table.LevelScaling > 0)
			{
				Drop.Quantity = FMath::Max(1, FMath::RoundToInt(Drop.Quantity * (1.0f + Table.LevelScaling * PlayerLevel)));
			}
			OutDrops.Add(Drop);
		}
	}

	// Roll currency
	if (Table.CurrencyDrop.MaxGold > 0)
	{
		if (FMath::FRand() < Table.CurrencyDrop.DropChance)
		{
			int32 GoldDrop = FMath::RandRange(Table.CurrencyDrop.MinGold, Table.CurrencyDrop.MaxGold);
			if (Table.LevelScaling > 0)
			{
				GoldDrop = FMath::RoundToInt(GoldDrop * (1.0f + Table.LevelScaling * PlayerLevel * 0.5f));
			}
			OutGold += GoldDrop;
		}
	}

	// Process included tables
	for (const FString& IncludedID : Table.IncludedTableIDs)
	{
		FLootTable IncludedTable;
		if (GetLootTable(IncludedID, IncludedTable))
		{
			ProcessLootTable(IncludedTable, PlayerLevel, OutDrops, OutGold, ProcessedTables);
		}
	}
}

bool ULootLibrary::RollLootEntry(const FLootEntry& Entry, int32 PlayerLevel, FLootDrop& OutDrop)
{
	// Check level requirements
	if (PlayerLevel < Entry.MinPlayerLevel)
	{
		return false;
	}
	if (Entry.MaxPlayerLevel > 0 && PlayerLevel > Entry.MaxPlayerLevel)
	{
		return false;
	}

	// Roll drop chance (guaranteed always succeeds)
	if (!Entry.bGuaranteed && FMath::FRand() > Entry.DropChance)
	{
		return false;
	}

	// Determine quantity
	OutDrop.ItemID = Entry.ItemID;
	OutDrop.Quantity = FMath::RandRange(Entry.MinQuantity, Entry.MaxQuantity);

	return true;
}

FLootEntry ULootLibrary::SelectWeightedRandom(const TArray<FLootEntry>& Entries)
{
	if (Entries.Num() == 0)
	{
		return FLootEntry();
	}

	if (Entries.Num() == 1)
	{
		return Entries[0];
	}

	// Calculate total weight
	float TotalWeight = 0.f;
	for (const FLootEntry& Entry : Entries)
	{
		TotalWeight += FMath::Max(0.001f, Entry.Weight);
	}

	// Roll random value
	float Roll = FMath::FRandRange(0.f, TotalWeight);

	// Find entry
	float CurrentWeight = 0.f;
	for (const FLootEntry& Entry : Entries)
	{
		CurrentWeight += FMath::Max(0.001f, Entry.Weight);
		if (Roll <= CurrentWeight)
		{
			return Entry;
		}
	}

	// Fallback
	return Entries.Last();
}

float ULootLibrary::ApplyLuckModifier(float BaseChance, float LuckModifier)
{
	// Luck increases chance by percentage
	// e.g., 10% chance with 0.5 luck = 10% * 1.5 = 15%
	return FMath::Clamp(BaseChance * (1.0f + LuckModifier), 0.f, 1.f);
}

// ============================================
// MONSTER LOOT
// ============================================

FString ULootLibrary::GetMonsterLootTableID(const FString& MonsterID)
{
	if (const FString* TableID = GetMonsterTableMap().Find(MonsterID))
	{
		return *TableID;
	}

	// Try prefix match (e.g., "wolf_alpha" -> "wolf")
	for (const auto& Pair : GetMonsterTableMap())
	{
		if (MonsterID.StartsWith(Pair.Key))
		{
			return Pair.Value;
		}
	}

	return FString();
}

void ULootLibrary::RegisterMonsterLootTable(const FString& MonsterID, const FString& TableID)
{
	GetMonsterTableMap().Add(MonsterID, TableID);
	UE_LOG(LogTemp, Log, TEXT("Registered monster loot: %s -> %s"), *MonsterID, *TableID);
}

bool ULootLibrary::GenerateMonsterLoot(const FString& MonsterID, int32 MonsterLevel, int32 PlayerLevel, TArray<FLootDrop>& OutDrops, int32& OutGold)
{
	FString TableID = GetMonsterLootTableID(MonsterID);
	if (TableID.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("No loot table for monster: %s"), *MonsterID);
		return false;
	}

	// Use average of monster and player level for scaling
	int32 EffectiveLevel = (MonsterLevel + PlayerLevel) / 2;

	return GenerateLoot(TableID, EffectiveLevel, OutDrops, OutGold);
}
