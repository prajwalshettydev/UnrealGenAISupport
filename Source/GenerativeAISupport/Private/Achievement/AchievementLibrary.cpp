// Achievement Library Implementation
// Part of GenerativeAISupport Plugin

#include "Achievement/AchievementLibrary.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"

TMap<FString, FAchievementDefinition>& UAchievementLibrary::GetRegistry()
{
	static TMap<FString, FAchievementDefinition> Registry;
	return Registry;
}

TMap<FString, FAchievementProgress>& UAchievementLibrary::GetProgressMap()
{
	static TMap<FString, FAchievementProgress> ProgressMap;
	return ProgressMap;
}

TMap<FString, int32>& UAchievementLibrary::GetStatMap()
{
	static TMap<FString, int32> StatMap;
	return StatMap;
}

TSet<FString>& UAchievementLibrary::GetFlagSet()
{
	static TSet<FString> FlagSet;
	return FlagSet;
}

TSet<FString>& UAchievementLibrary::GetDiscoverySet()
{
	static TSet<FString> DiscoverySet;
	return DiscoverySet;
}

// ============================================
// ACHIEVEMENT REGISTRY
// ============================================

void UAchievementLibrary::RegisterAchievement(const FAchievementDefinition& Achievement)
{
	if (Achievement.AchievementID.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot register achievement with empty ID"));
		return;
	}

	GetRegistry().Add(Achievement.AchievementID, Achievement);

	// Initialize progress if not exists
	if (!GetProgressMap().Contains(Achievement.AchievementID))
	{
		FAchievementProgress Progress;
		Progress.AchievementID = Achievement.AchievementID;
		Progress.CurrentValue = 0;
		Progress.bCompleted = false;
		GetProgressMap().Add(Achievement.AchievementID, Progress);
	}

	UE_LOG(LogTemp, Log, TEXT("Registered achievement: %s"), *Achievement.AchievementID);
}

void UAchievementLibrary::RegisterAchievementAsset(UAchievementDataAsset* Asset)
{
	if (Asset)
	{
		RegisterAchievement(Asset->Achievement);
	}
}

int32 UAchievementLibrary::LoadAchievementsFromJSON(const FString& FilePath)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load achievements file: %s"), *FilePath);
		return 0;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse achievements file: %s"), *FilePath);
		return 0;
	}

	const TArray<TSharedPtr<FJsonValue>>* AchievementsArray;
	if (!Root->TryGetArrayField(TEXT("achievements"), AchievementsArray))
	{
		UE_LOG(LogTemp, Warning, TEXT("No 'achievements' array in file: %s"), *FilePath);
		return 0;
	}

	int32 LoadedCount = 0;

	for (const TSharedPtr<FJsonValue>& AchVal : *AchievementsArray)
	{
		const TSharedPtr<FJsonObject>* AchObj;
		if (!AchVal->TryGetObject(AchObj))
		{
			continue;
		}

		FAchievementDefinition Achievement;

		// Basic fields
		Achievement.AchievementID = (*AchObj)->GetStringField(TEXT("id"));
		Achievement.DisplayName = FText::FromString((*AchObj)->GetStringField(TEXT("name")));
		Achievement.Description = FText::FromString((*AchObj)->GetStringField(TEXT("description")));
		Achievement.HiddenDescription = FText::FromString((*AchObj)->GetStringField(TEXT("hidden_description")));
		Achievement.TrackedStat = (*AchObj)->GetStringField(TEXT("tracked_stat"));
		Achievement.TargetValue = (*AchObj)->GetIntegerField(TEXT("target_value"));
		Achievement.bHidden = (*AchObj)->GetBoolField(TEXT("hidden"));
		Achievement.bShowProgress = (*AchObj)->GetBoolField(TEXT("show_progress"));

		// Category
		FString CategoryStr = (*AchObj)->GetStringField(TEXT("category"));
		if (CategoryStr == TEXT("combat")) Achievement.Category = EAchievementCategory::Combat;
		else if (CategoryStr == TEXT("exploration")) Achievement.Category = EAchievementCategory::Exploration;
		else if (CategoryStr == TEXT("collection")) Achievement.Category = EAchievementCategory::Collection;
		else if (CategoryStr == TEXT("social")) Achievement.Category = EAchievementCategory::Social;
		else if (CategoryStr == TEXT("story")) Achievement.Category = EAchievementCategory::Story;
		else if (CategoryStr == TEXT("crafting")) Achievement.Category = EAchievementCategory::Crafting;
		else if (CategoryStr == TEXT("hidden")) Achievement.Category = EAchievementCategory::Hidden;
		else Achievement.Category = EAchievementCategory::Miscellaneous;

		// Rarity
		FString RarityStr = (*AchObj)->GetStringField(TEXT("rarity"));
		if (RarityStr == TEXT("uncommon")) Achievement.Rarity = EAchievementRarity::Uncommon;
		else if (RarityStr == TEXT("rare")) Achievement.Rarity = EAchievementRarity::Rare;
		else if (RarityStr == TEXT("epic")) Achievement.Rarity = EAchievementRarity::Epic;
		else if (RarityStr == TEXT("legendary")) Achievement.Rarity = EAchievementRarity::Legendary;
		else Achievement.Rarity = EAchievementRarity::Common;

		// Trigger type
		FString TriggerStr = (*AchObj)->GetStringField(TEXT("trigger"));
		if (TriggerStr == TEXT("threshold")) Achievement.TriggerType = EAchievementTrigger::Threshold;
		else if (TriggerStr == TEXT("flag")) Achievement.TriggerType = EAchievementTrigger::Flag;
		else if (TriggerStr == TEXT("collection")) Achievement.TriggerType = EAchievementTrigger::Collection;
		else if (TriggerStr == TEXT("discovery")) Achievement.TriggerType = EAchievementTrigger::Discovery;
		else Achievement.TriggerType = EAchievementTrigger::Count;

		// Prerequisites
		const TArray<TSharedPtr<FJsonValue>>* PrereqsArray;
		if ((*AchObj)->TryGetArrayField(TEXT("prerequisites"), PrereqsArray))
		{
			for (const TSharedPtr<FJsonValue>& Val : *PrereqsArray)
			{
				Achievement.Prerequisites.Add(Val->AsString());
			}
		}

		// Rewards
		const TSharedPtr<FJsonObject>* RewardObj;
		if ((*AchObj)->TryGetObjectField(TEXT("reward"), RewardObj))
		{
			Achievement.Reward.XP = (*RewardObj)->GetIntegerField(TEXT("xp"));
			Achievement.Reward.Gold = (*RewardObj)->GetIntegerField(TEXT("gold"));
			Achievement.Reward.AchievementPoints = (*RewardObj)->GetIntegerField(TEXT("points"));
			Achievement.Reward.TitleUnlocked = (*RewardObj)->GetStringField(TEXT("title"));

			const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
			if ((*RewardObj)->TryGetArrayField(TEXT("items"), ItemsArray))
			{
				for (const TSharedPtr<FJsonValue>& Val : *ItemsArray)
				{
					Achievement.Reward.ItemIDs.Add(Val->AsString());
				}
			}
		}

		RegisterAchievement(Achievement);
		LoadedCount++;
	}

	UE_LOG(LogTemp, Log, TEXT("Loaded %d achievements from %s"), LoadedCount, *FilePath);
	return LoadedCount;
}

bool UAchievementLibrary::GetAchievement(const FString& AchievementID, FAchievementDefinition& OutAchievement)
{
	if (const FAchievementDefinition* Found = GetRegistry().Find(AchievementID))
	{
		OutAchievement = *Found;
		return true;
	}
	return false;
}

TArray<FAchievementDefinition> UAchievementLibrary::GetAchievementsByCategory(EAchievementCategory Category)
{
	TArray<FAchievementDefinition> Result;
	for (const auto& Pair : GetRegistry())
	{
		if (Pair.Value.Category == Category)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

// ============================================
// PROGRESS TRACKING
// ============================================

void UAchievementLibrary::ReportProgress(const FString& StatName, int32 Amount, bool bSetValue)
{
	int32& StatValue = GetStatMap().FindOrAdd(StatName, 0);

	if (bSetValue)
	{
		StatValue = Amount;
	}
	else
	{
		StatValue += Amount;
	}

	// Check all achievements that track this stat
	for (const auto& Pair : GetRegistry())
	{
		if (Pair.Value.TrackedStat == StatName)
		{
			CheckAchievementUnlock(Pair.Key);
		}
	}
}

void UAchievementLibrary::ReportFlag(const FString& FlagName)
{
	GetFlagSet().Add(FlagName);

	// Check all achievements that track this flag
	for (const auto& Pair : GetRegistry())
	{
		if (Pair.Value.TriggerType == EAchievementTrigger::Flag && Pair.Value.TrackedStat == FlagName)
		{
			CheckAchievementUnlock(Pair.Key);
		}
	}
}

void UAchievementLibrary::ReportDiscovery(const FString& DiscoveryID)
{
	GetDiscoverySet().Add(DiscoveryID);

	// Check all achievements that track this discovery
	for (const auto& Pair : GetRegistry())
	{
		if (Pair.Value.TriggerType == EAchievementTrigger::Discovery && Pair.Value.TrackedStat == DiscoveryID)
		{
			CheckAchievementUnlock(Pair.Key);
		}
	}
}

void UAchievementLibrary::CheckAchievementUnlock(const FString& AchievementID)
{
	const FAchievementDefinition* Achievement = GetRegistry().Find(AchievementID);
	if (!Achievement)
	{
		return;
	}

	FAchievementProgress* Progress = GetProgressMap().Find(AchievementID);
	if (!Progress || Progress->bCompleted)
	{
		return;
	}

	// Check prerequisites
	if (!ArePrerequisitesMet(*Achievement))
	{
		return;
	}

	bool bShouldUnlock = false;
	int32 CurrentValue = 0;

	switch (Achievement->TriggerType)
	{
	case EAchievementTrigger::Count:
	case EAchievementTrigger::Threshold:
		{
			const int32* StatVal = GetStatMap().Find(Achievement->TrackedStat);
			CurrentValue = StatVal ? *StatVal : 0;
			bShouldUnlock = CurrentValue >= Achievement->TargetValue;
		}
		break;

	case EAchievementTrigger::Flag:
		{
			bShouldUnlock = GetFlagSet().Contains(Achievement->TrackedStat);
			CurrentValue = bShouldUnlock ? 1 : 0;
		}
		break;

	case EAchievementTrigger::Discovery:
		{
			bShouldUnlock = GetDiscoverySet().Contains(Achievement->TrackedStat);
			CurrentValue = bShouldUnlock ? 1 : 0;
		}
		break;

	case EAchievementTrigger::Collection:
		// TODO: Implement collection check with inventory
		break;
	}

	Progress->CurrentValue = CurrentValue;

	if (bShouldUnlock)
	{
		UnlockAchievement(AchievementID);
	}
}

void UAchievementLibrary::UnlockAchievement(const FString& AchievementID)
{
	FAchievementProgress* Progress = GetProgressMap().Find(AchievementID);
	if (!Progress || Progress->bCompleted)
	{
		return;
	}

	Progress->bCompleted = true;
	Progress->CompletionTime = FDateTime::Now();

	const FAchievementDefinition* Achievement = GetRegistry().Find(AchievementID);
	if (Achievement)
	{
		Progress->CurrentValue = Achievement->TargetValue;
		UE_LOG(LogTemp, Log, TEXT("Achievement Unlocked: %s - %s"),
			*AchievementID, *Achievement->DisplayName.ToString());
	}
}

bool UAchievementLibrary::ArePrerequisitesMet(const FAchievementDefinition& Achievement)
{
	for (const FString& PrereqID : Achievement.Prerequisites)
	{
		if (!IsAchievementUnlocked(PrereqID))
		{
			return false;
		}
	}
	return true;
}

bool UAchievementLibrary::GetAchievementProgress(const FString& AchievementID, FAchievementProgress& OutProgress)
{
	if (const FAchievementProgress* Progress = GetProgressMap().Find(AchievementID))
	{
		OutProgress = *Progress;
		return true;
	}
	return false;
}

bool UAchievementLibrary::IsAchievementUnlocked(const FString& AchievementID)
{
	if (const FAchievementProgress* Progress = GetProgressMap().Find(AchievementID))
	{
		return Progress->bCompleted;
	}
	return false;
}

float UAchievementLibrary::GetAchievementProgressPercent(const FString& AchievementID)
{
	const FAchievementDefinition* Achievement = GetRegistry().Find(AchievementID);
	const FAchievementProgress* Progress = GetProgressMap().Find(AchievementID);

	if (!Achievement || !Progress || Achievement->TargetValue <= 0)
	{
		return 0.f;
	}

	if (Progress->bCompleted)
	{
		return 1.f;
	}

	return FMath::Clamp((float)Progress->CurrentValue / (float)Achievement->TargetValue, 0.f, 1.f);
}

// ============================================
// REWARD SYSTEM
// ============================================

bool UAchievementLibrary::ClaimReward(const FString& AchievementID, FAchievementReward& OutReward)
{
	if (!CanClaimReward(AchievementID))
	{
		return false;
	}

	FAchievementProgress* Progress = GetProgressMap().Find(AchievementID);
	const FAchievementDefinition* Achievement = GetRegistry().Find(AchievementID);

	if (Progress && Achievement)
	{
		Progress->bRewardClaimed = true;
		OutReward = Achievement->Reward;
		return true;
	}

	return false;
}

bool UAchievementLibrary::CanClaimReward(const FString& AchievementID)
{
	const FAchievementProgress* Progress = GetProgressMap().Find(AchievementID);
	return Progress && Progress->bCompleted && !Progress->bRewardClaimed;
}

int32 UAchievementLibrary::GetTotalAchievementPoints()
{
	int32 Total = 0;

	for (const auto& Pair : GetProgressMap())
	{
		if (Pair.Value.bCompleted)
		{
			const FAchievementDefinition* Achievement = GetRegistry().Find(Pair.Key);
			if (Achievement)
			{
				Total += Achievement->Reward.AchievementPoints;
			}
		}
	}

	return Total;
}

// ============================================
// QUERIES
// ============================================

TArray<FAchievementProgress> UAchievementLibrary::GetUnlockedAchievements()
{
	TArray<FAchievementProgress> Result;
	for (const auto& Pair : GetProgressMap())
	{
		if (Pair.Value.bCompleted)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

TArray<FAchievementProgress> UAchievementLibrary::GetInProgressAchievements()
{
	TArray<FAchievementProgress> Result;
	for (const auto& Pair : GetProgressMap())
	{
		if (!Pair.Value.bCompleted && Pair.Value.CurrentValue > 0)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

TArray<FAchievementProgress> UAchievementLibrary::GetRecentlyUnlocked(int32 Count)
{
	TArray<FAchievementProgress> Unlocked = GetUnlockedAchievements();

	// Sort by completion time (newest first)
	Unlocked.Sort([](const FAchievementProgress& A, const FAchievementProgress& B)
	{
		return A.CompletionTime > B.CompletionTime;
	});

	// Return only requested count
	if (Unlocked.Num() > Count)
	{
		Unlocked.SetNum(Count);
	}

	return Unlocked;
}

void UAchievementLibrary::GetCompletionStats(int32& OutTotal, int32& OutUnlocked, float& OutPercent)
{
	OutTotal = GetRegistry().Num();
	OutUnlocked = 0;

	for (const auto& Pair : GetProgressMap())
	{
		if (Pair.Value.bCompleted)
		{
			OutUnlocked++;
		}
	}

	OutPercent = OutTotal > 0 ? (float)OutUnlocked / (float)OutTotal : 0.f;
}

// ============================================
// SAVE/LOAD
// ============================================

FString UAchievementLibrary::SaveProgressToJSON()
{
	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);

	// Save stat values
	TSharedPtr<FJsonObject> StatsObj = MakeShareable(new FJsonObject);
	for (const auto& Pair : GetStatMap())
	{
		StatsObj->SetNumberField(Pair.Key, Pair.Value);
	}
	Root->SetObjectField(TEXT("stats"), StatsObj);

	// Save flags
	TArray<TSharedPtr<FJsonValue>> FlagsArray;
	for (const FString& Flag : GetFlagSet())
	{
		FlagsArray.Add(MakeShareable(new FJsonValueString(Flag)));
	}
	Root->SetArrayField(TEXT("flags"), FlagsArray);

	// Save discoveries
	TArray<TSharedPtr<FJsonValue>> DiscoveriesArray;
	for (const FString& Discovery : GetDiscoverySet())
	{
		DiscoveriesArray.Add(MakeShareable(new FJsonValueString(Discovery)));
	}
	Root->SetArrayField(TEXT("discoveries"), DiscoveriesArray);

	// Save progress
	TArray<TSharedPtr<FJsonValue>> ProgressArray;
	for (const auto& Pair : GetProgressMap())
	{
		TSharedPtr<FJsonObject> ProgObj = MakeShareable(new FJsonObject);
		ProgObj->SetStringField(TEXT("id"), Pair.Value.AchievementID);
		ProgObj->SetNumberField(TEXT("value"), Pair.Value.CurrentValue);
		ProgObj->SetBoolField(TEXT("completed"), Pair.Value.bCompleted);
		ProgObj->SetBoolField(TEXT("claimed"), Pair.Value.bRewardClaimed);
		ProgObj->SetStringField(TEXT("time"), Pair.Value.CompletionTime.ToString());

		ProgressArray.Add(MakeShareable(new FJsonValueObject(ProgObj)));
	}
	Root->SetArrayField(TEXT("progress"), ProgressArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	return OutputString;
}

bool UAchievementLibrary::LoadProgressFromJSON(const FString& JsonString)
{
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	// Load stats
	const TSharedPtr<FJsonObject>* StatsObj;
	if (Root->TryGetObjectField(TEXT("stats"), StatsObj))
	{
		GetStatMap().Empty();
		for (const auto& Pair : (*StatsObj)->Values)
		{
			GetStatMap().Add(Pair.Key, (int32)Pair.Value->AsNumber());
		}
	}

	// Load flags
	const TArray<TSharedPtr<FJsonValue>>* FlagsArray;
	if (Root->TryGetArrayField(TEXT("flags"), FlagsArray))
	{
		GetFlagSet().Empty();
		for (const TSharedPtr<FJsonValue>& Val : *FlagsArray)
		{
			GetFlagSet().Add(Val->AsString());
		}
	}

	// Load discoveries
	const TArray<TSharedPtr<FJsonValue>>* DiscoveriesArray;
	if (Root->TryGetArrayField(TEXT("discoveries"), DiscoveriesArray))
	{
		GetDiscoverySet().Empty();
		for (const TSharedPtr<FJsonValue>& Val : *DiscoveriesArray)
		{
			GetDiscoverySet().Add(Val->AsString());
		}
	}

	// Load progress
	const TArray<TSharedPtr<FJsonValue>>* ProgressArray;
	if (Root->TryGetArrayField(TEXT("progress"), ProgressArray))
	{
		for (const TSharedPtr<FJsonValue>& Val : *ProgressArray)
		{
			const TSharedPtr<FJsonObject>* ProgObj;
			if (Val->TryGetObject(ProgObj))
			{
				FString ID = (*ProgObj)->GetStringField(TEXT("id"));
				FAchievementProgress* Progress = GetProgressMap().Find(ID);
				if (Progress)
				{
					Progress->CurrentValue = (*ProgObj)->GetIntegerField(TEXT("value"));
					Progress->bCompleted = (*ProgObj)->GetBoolField(TEXT("completed"));
					Progress->bRewardClaimed = (*ProgObj)->GetBoolField(TEXT("claimed"));

					FString TimeStr = (*ProgObj)->GetStringField(TEXT("time"));
					FDateTime::Parse(TimeStr, Progress->CompletionTime);
				}
			}
		}
	}

	return true;
}

// ============================================
// UTILITY
// ============================================

FLinearColor UAchievementLibrary::GetRarityColor(EAchievementRarity Rarity)
{
	switch (Rarity)
	{
	case EAchievementRarity::Common:
		return FLinearColor(0.8f, 0.8f, 0.8f); // Gray
	case EAchievementRarity::Uncommon:
		return FLinearColor(0.2f, 0.8f, 0.2f); // Green
	case EAchievementRarity::Rare:
		return FLinearColor(0.2f, 0.4f, 1.0f); // Blue
	case EAchievementRarity::Epic:
		return FLinearColor(0.6f, 0.2f, 0.8f); // Purple
	case EAchievementRarity::Legendary:
		return FLinearColor(1.0f, 0.6f, 0.0f); // Orange
	default:
		return FLinearColor::White;
	}
}

FText UAchievementLibrary::GetCategoryDisplayName(EAchievementCategory Category)
{
	switch (Category)
	{
	case EAchievementCategory::Combat:
		return FText::FromString(TEXT("Kampf"));
	case EAchievementCategory::Exploration:
		return FText::FromString(TEXT("Erkundung"));
	case EAchievementCategory::Collection:
		return FText::FromString(TEXT("Sammlung"));
	case EAchievementCategory::Social:
		return FText::FromString(TEXT("Sozial"));
	case EAchievementCategory::Story:
		return FText::FromString(TEXT("Geschichte"));
	case EAchievementCategory::Crafting:
		return FText::FromString(TEXT("Handwerk"));
	case EAchievementCategory::Hidden:
		return FText::FromString(TEXT("Versteckt"));
	default:
		return FText::FromString(TEXT("Sonstiges"));
	}
}

void UAchievementLibrary::ResetAllProgress()
{
	GetStatMap().Empty();
	GetFlagSet().Empty();
	GetDiscoverySet().Empty();

	// Reset all progress entries
	for (auto& Pair : GetProgressMap())
	{
		Pair.Value.CurrentValue = 0;
		Pair.Value.bCompleted = false;
		Pair.Value.bRewardClaimed = false;
		Pair.Value.CompletionTime = FDateTime();
	}

	UE_LOG(LogTemp, Log, TEXT("All achievement progress reset"));
}
