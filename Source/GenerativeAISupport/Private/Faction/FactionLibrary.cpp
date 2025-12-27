// Faction Library Implementation
// Part of GenerativeAISupport Plugin

#include "Faction/FactionLibrary.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

// Static delegate definitions
FOnRankUp UFactionLibrary::OnRankUp;
FOnFactionDiscovered UFactionLibrary::OnFactionDiscovered;

// ============================================
// STATIC DATA ACCESS
// ============================================

TMap<FString, FFactionDefinition>& UFactionLibrary::GetFactionRegistry()
{
	static TMap<FString, FFactionDefinition> Registry;
	return Registry;
}

TMap<FString, FPlayerFactionStanding>& UFactionLibrary::GetPlayerStandings()
{
	static TMap<FString, FPlayerFactionStanding> Standings;
	return Standings;
}

TMap<FString, FString>& UFactionLibrary::GetNPCFactionMap()
{
	static TMap<FString, FString> Map;
	return Map;
}

// ============================================
// FACTION REGISTRY
// ============================================

void UFactionLibrary::RegisterFaction(const FFactionDefinition& Faction)
{
	GetFactionRegistry().Add(Faction.FactionID, Faction);

	// Build NPC to faction map
	for (const FString& NPCID : Faction.MemberNPCIDs)
	{
		GetNPCFactionMap().Add(NPCID, Faction.FactionID);
	}

	// Initialize player standing if not exists
	if (!GetPlayerStandings().Contains(Faction.FactionID))
	{
		FPlayerFactionStanding Standing;
		Standing.FactionID = Faction.FactionID;
		Standing.Reputation = Faction.StartingReputation;
		Standing.bDiscovered = !Faction.bHiddenUntilDiscovered;
		Standing.CurrentRank = GetRankFromReputation(Standing.Reputation);
		GetPlayerStandings().Add(Faction.FactionID, Standing);
	}
}

int32 UFactionLibrary::LoadFactionsFromJSON(const FString& FilePath)
{
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("FactionLibrary: Could not load file %s"), *FilePath);
		return 0;
	}

	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);

	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("FactionLibrary: Failed to parse JSON from %s"), *FilePath);
		return 0;
	}

	const TArray<TSharedPtr<FJsonValue>>* FactionsArray;
	if (!RootObject->TryGetArrayField(TEXT("factions"), FactionsArray))
	{
		return 0;
	}

	int32 LoadedCount = 0;

	for (const TSharedPtr<FJsonValue>& FactionValue : *FactionsArray)
	{
		TSharedPtr<FJsonObject> FactionObj = FactionValue->AsObject();
		if (!FactionObj.IsValid()) continue;

		FFactionDefinition Faction;
		Faction.FactionID = FactionObj->GetStringField(TEXT("id"));
		Faction.DisplayName = FText::FromString(FactionObj->GetStringField(TEXT("name")));
		Faction.Description = FText::FromString(FactionObj->GetStringField(TEXT("description")));
		Faction.IconPath = FactionObj->GetStringField(TEXT("icon"));
		Faction.HomeRegion = FactionObj->GetStringField(TEXT("home_region"));
		Faction.StartingReputation = FactionObj->GetIntegerField(TEXT("starting_reputation"));
		Faction.bHiddenUntilDiscovered = FactionObj->GetBoolField(TEXT("hidden"));

		// Parse color
		if (FactionObj->HasField(TEXT("color")))
		{
			FString ColorStr = FactionObj->GetStringField(TEXT("color"));
			Faction.FactionColor = FLinearColor::FromSRGBColor(FColor::FromHex(ColorStr));
		}

		// Parse member NPCs
		const TArray<TSharedPtr<FJsonValue>>* MembersArray;
		if (FactionObj->TryGetArrayField(TEXT("members"), MembersArray))
		{
			for (const TSharedPtr<FJsonValue>& MemberValue : *MembersArray)
			{
				Faction.MemberNPCIDs.Add(MemberValue->AsString());
			}
		}

		// Parse shops
		const TArray<TSharedPtr<FJsonValue>>* ShopsArray;
		if (FactionObj->TryGetArrayField(TEXT("shops"), ShopsArray))
		{
			for (const TSharedPtr<FJsonValue>& ShopValue : *ShopsArray)
			{
				Faction.FactionShopIDs.Add(ShopValue->AsString());
			}
		}

		// Parse relationships
		const TArray<TSharedPtr<FJsonValue>>* RelationsArray;
		if (FactionObj->TryGetArrayField(TEXT("relationships"), RelationsArray))
		{
			for (const TSharedPtr<FJsonValue>& RelValue : *RelationsArray)
			{
				TSharedPtr<FJsonObject> RelObj = RelValue->AsObject();
				if (!RelObj.IsValid()) continue;

				FFactionRelationship Rel;
				Rel.OtherFactionID = RelObj->GetStringField(TEXT("faction"));

				FString RelType = RelObj->GetStringField(TEXT("type")).ToLower();
				if (RelType == TEXT("enemy")) Rel.Relation = EFactionRelation::Enemy;
				else if (RelType == TEXT("unfriendly")) Rel.Relation = EFactionRelation::Unfriendly;
				else if (RelType == TEXT("friendly")) Rel.Relation = EFactionRelation::Friendly;
				else if (RelType == TEXT("allied")) Rel.Relation = EFactionRelation::Allied;
				else Rel.Relation = EFactionRelation::Neutral;

				Rel.ReputationSharePercent = RelObj->GetNumberField(TEXT("rep_share"));
				Faction.Relationships.Add(Rel);
			}
		}

		// Parse rank rewards
		const TArray<TSharedPtr<FJsonValue>>* RewardsArray;
		if (FactionObj->TryGetArrayField(TEXT("rank_rewards"), RewardsArray))
		{
			for (const TSharedPtr<FJsonValue>& RewardValue : *RewardsArray)
			{
				TSharedPtr<FJsonObject> RewardObj = RewardValue->AsObject();
				if (!RewardObj.IsValid()) continue;

				FFactionRankReward Reward;

				FString RankStr = RewardObj->GetStringField(TEXT("rank")).ToLower();
				if (RankStr == TEXT("friendly")) Reward.RequiredRank = EReputationRank::Friendly;
				else if (RankStr == TEXT("honored")) Reward.RequiredRank = EReputationRank::Honored;
				else if (RankStr == TEXT("exalted")) Reward.RequiredRank = EReputationRank::Exalted;
				else Reward.RequiredRank = EReputationRank::Neutral;

				Reward.TitleUnlocked = RewardObj->GetStringField(TEXT("title"));
				Reward.ShopDiscountPercent = RewardObj->GetNumberField(TEXT("discount"));
				Reward.UniqueRewardItem = RewardObj->GetStringField(TEXT("reward_item"));
				Reward.QuestXPBonus = RewardObj->GetNumberField(TEXT("xp_bonus"));

				const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
				if (RewardObj->TryGetArrayField(TEXT("unlocked_items"), ItemsArray))
				{
					for (const TSharedPtr<FJsonValue>& ItemValue : *ItemsArray)
					{
						Reward.UnlockedItems.Add(ItemValue->AsString());
					}
				}

				Faction.RankRewards.Add(Reward);
			}
		}

		RegisterFaction(Faction);
		LoadedCount++;
	}

	UE_LOG(LogTemp, Log, TEXT("FactionLibrary: Loaded %d factions from %s"), LoadedCount, *FilePath);
	return LoadedCount;
}

bool UFactionLibrary::GetFaction(const FString& FactionID, FFactionDefinition& OutFaction)
{
	const FFactionDefinition* Found = GetFactionRegistry().Find(FactionID);
	if (Found)
	{
		OutFaction = *Found;
		return true;
	}
	return false;
}

TArray<FFactionDefinition> UFactionLibrary::GetAllFactions()
{
	TArray<FFactionDefinition> Result;
	for (const auto& Pair : GetFactionRegistry())
	{
		Result.Add(Pair.Value);
	}
	return Result;
}

TArray<FString> UFactionLibrary::GetDiscoveredFactions()
{
	TArray<FString> Result;
	for (const auto& Pair : GetPlayerStandings())
	{
		if (Pair.Value.bDiscovered)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

// ============================================
// REPUTATION MANAGEMENT
// ============================================

FReputationChangeEvent UFactionLibrary::ModifyReputation(const FString& FactionID, int32 Amount, const FText& Reason, const FString& Source)
{
	FReputationChangeEvent Event;
	Event.FactionID = FactionID;
	Event.Amount = Amount;
	Event.Reason = Reason;
	Event.Source = Source;

	FFactionDefinition Faction;
	if (!GetFaction(FactionID, Faction))
	{
		return Event;
	}

	FPlayerFactionStanding& Standing = GetPlayerStandings().FindOrAdd(FactionID);
	Event.OldRank = Standing.CurrentRank;

	// Track totals
	if (Amount > 0)
	{
		Standing.TotalReputationEarned += Amount;
	}
	else
	{
		Standing.TotalReputationLost += FMath::Abs(Amount);
	}

	// Apply change
	Standing.Reputation = FMath::Clamp(Standing.Reputation + Amount, Faction.MinReputation, Faction.MaxReputation);
	Event.NewReputation = Standing.Reputation;

	// Update rank
	UpdateRank(Standing, Faction);
	Event.NewRank = Standing.CurrentRank;
	Event.bRankChanged = Event.OldRank != Event.NewRank;

	// Broadcast rank up
	if (Event.bRankChanged && Event.NewRank > Event.OldRank)
	{
		OnRankUp.Broadcast(Event);
	}

	// Apply to related factions
	ApplyReputationToRelatedFactions(FactionID, Amount, Reason, Source);

	return Event;
}

FReputationChangeEvent UFactionLibrary::SetReputation(const FString& FactionID, int32 NewReputation)
{
	int32 CurrentRep = GetReputation(FactionID);
	int32 Difference = NewReputation - CurrentRep;
	return ModifyReputation(FactionID, Difference, FText::FromString(TEXT("Direct set")), TEXT("system"));
}

int32 UFactionLibrary::GetReputation(const FString& FactionID)
{
	const FPlayerFactionStanding* Standing = GetPlayerStandings().Find(FactionID);
	return Standing ? Standing->Reputation : 0;
}

EReputationRank UFactionLibrary::GetRank(const FString& FactionID)
{
	const FPlayerFactionStanding* Standing = GetPlayerStandings().Find(FactionID);
	return Standing ? Standing->CurrentRank : EReputationRank::Neutral;
}

bool UFactionLibrary::GetPlayerStanding(const FString& FactionID, FPlayerFactionStanding& OutStanding)
{
	const FPlayerFactionStanding* Standing = GetPlayerStandings().Find(FactionID);
	if (Standing)
	{
		OutStanding = *Standing;
		return true;
	}
	return false;
}

TArray<FPlayerFactionStanding> UFactionLibrary::GetAllPlayerStandings()
{
	TArray<FPlayerFactionStanding> Result;
	for (const auto& Pair : GetPlayerStandings())
	{
		Result.Add(Pair.Value);
	}
	return Result;
}

// ============================================
// RANK UTILITIES
// ============================================

EReputationRank UFactionLibrary::GetRankFromReputation(int32 Reputation)
{
	if (Reputation >= 900) return EReputationRank::Exalted;
	if (Reputation >= 600) return EReputationRank::Honored;
	if (Reputation >= 300) return EReputationRank::Friendly;
	if (Reputation >= 0) return EReputationRank::Neutral;
	if (Reputation >= -300) return EReputationRank::Unfriendly;
	if (Reputation >= -600) return EReputationRank::Hostile;
	return EReputationRank::Hated;
}

void UFactionLibrary::GetRankThresholds(EReputationRank Rank, int32& OutMin, int32& OutMax)
{
	switch (Rank)
	{
	case EReputationRank::Hated: OutMin = -1000; OutMax = -601; break;
	case EReputationRank::Hostile: OutMin = -600; OutMax = -301; break;
	case EReputationRank::Unfriendly: OutMin = -300; OutMax = -1; break;
	case EReputationRank::Neutral: OutMin = 0; OutMax = 299; break;
	case EReputationRank::Friendly: OutMin = 300; OutMax = 599; break;
	case EReputationRank::Honored: OutMin = 600; OutMax = 899; break;
	case EReputationRank::Exalted: OutMin = 900; OutMax = 1000; break;
	default: OutMin = 0; OutMax = 0; break;
	}
}

FText UFactionLibrary::GetRankDisplayName(EReputationRank Rank)
{
	switch (Rank)
	{
	case EReputationRank::Hated: return FText::FromString(TEXT("Hated"));
	case EReputationRank::Hostile: return FText::FromString(TEXT("Hostile"));
	case EReputationRank::Unfriendly: return FText::FromString(TEXT("Unfriendly"));
	case EReputationRank::Neutral: return FText::FromString(TEXT("Neutral"));
	case EReputationRank::Friendly: return FText::FromString(TEXT("Friendly"));
	case EReputationRank::Honored: return FText::FromString(TEXT("Honored"));
	case EReputationRank::Exalted: return FText::FromString(TEXT("Exalted"));
	default: return FText::FromString(TEXT("Unknown"));
	}
}

FLinearColor UFactionLibrary::GetRankColor(EReputationRank Rank)
{
	switch (Rank)
	{
	case EReputationRank::Hated: return FLinearColor(0.8f, 0.0f, 0.0f);      // Dark Red
	case EReputationRank::Hostile: return FLinearColor(1.0f, 0.2f, 0.2f);    // Red
	case EReputationRank::Unfriendly: return FLinearColor(1.0f, 0.5f, 0.0f); // Orange
	case EReputationRank::Neutral: return FLinearColor(0.8f, 0.8f, 0.8f);    // Gray
	case EReputationRank::Friendly: return FLinearColor(0.2f, 0.8f, 0.2f);   // Green
	case EReputationRank::Honored: return FLinearColor(0.2f, 0.6f, 1.0f);    // Blue
	case EReputationRank::Exalted: return FLinearColor(0.8f, 0.6f, 1.0f);    // Purple
	default: return FLinearColor::White;
	}
}

bool UFactionLibrary::GetProgressToNextRank(const FString& FactionID, int32& OutCurrentRep, int32& OutNextRankRep, float& OutProgress)
{
	OutCurrentRep = GetReputation(FactionID);
	EReputationRank CurrentRank = GetRankFromReputation(OutCurrentRep);

	if (CurrentRank == EReputationRank::Exalted)
	{
		OutNextRankRep = 1000;
		OutProgress = 1.0f;
		return false;
	}

	int32 CurrentMin, CurrentMax, NextMin, NextMax;
	GetRankThresholds(CurrentRank, CurrentMin, CurrentMax);

	EReputationRank NextRank = static_cast<EReputationRank>(static_cast<uint8>(CurrentRank) + 1);
	GetRankThresholds(NextRank, NextMin, NextMax);

	OutNextRankRep = NextMin;
	float Range = static_cast<float>(NextMin - CurrentMin);
	OutProgress = Range > 0 ? (OutCurrentRep - CurrentMin) / Range : 0.0f;

	return true;
}

// ============================================
// FACTION RELATIONSHIPS
// ============================================

EFactionRelation UFactionLibrary::GetFactionRelationship(const FString& FactionA, const FString& FactionB)
{
	FFactionDefinition Faction;
	if (!GetFaction(FactionA, Faction))
	{
		return EFactionRelation::Neutral;
	}

	for (const FFactionRelationship& Rel : Faction.Relationships)
	{
		if (Rel.OtherFactionID == FactionB)
		{
			return Rel.Relation;
		}
	}

	return EFactionRelation::Neutral;
}

FFactionCompareResult UFactionLibrary::CompareFactions(const FString& FactionA, const FString& FactionB)
{
	FFactionCompareResult Result;
	Result.FactionA = FactionA;
	Result.FactionB = FactionB;
	Result.Relation = GetFactionRelationship(FactionA, FactionB);

	Result.bHelpingAHurtsB = (Result.Relation == EFactionRelation::Enemy);

	FFactionDefinition Faction;
	if (GetFaction(FactionA, Faction))
	{
		for (const FFactionRelationship& Rel : Faction.Relationships)
		{
			if (Rel.OtherFactionID == FactionB)
			{
				Result.SharedReputationPercent = Rel.ReputationSharePercent;
				break;
			}
		}
	}

	return Result;
}

TArray<FString> UFactionLibrary::GetAlliedFactions(const FString& FactionID)
{
	TArray<FString> Result;
	FFactionDefinition Faction;
	if (GetFaction(FactionID, Faction))
	{
		for (const FFactionRelationship& Rel : Faction.Relationships)
		{
			if (Rel.Relation == EFactionRelation::Allied || Rel.Relation == EFactionRelation::Friendly)
			{
				Result.Add(Rel.OtherFactionID);
			}
		}
	}
	return Result;
}

TArray<FString> UFactionLibrary::GetEnemyFactions(const FString& FactionID)
{
	TArray<FString> Result;
	FFactionDefinition Faction;
	if (GetFaction(FactionID, Faction))
	{
		for (const FFactionRelationship& Rel : Faction.Relationships)
		{
			if (Rel.Relation == EFactionRelation::Enemy)
			{
				Result.Add(Rel.OtherFactionID);
			}
		}
	}
	return Result;
}

// ============================================
// REWARDS
// ============================================

TArray<FFactionRankReward> UFactionLibrary::GetAvailableRewards(const FString& FactionID)
{
	TArray<FFactionRankReward> Result;
	FFactionDefinition Faction;
	if (!GetFaction(FactionID, Faction)) return Result;

	EReputationRank CurrentRank = GetRank(FactionID);

	for (const FFactionRankReward& Reward : Faction.RankRewards)
	{
		if (Reward.RequiredRank <= CurrentRank)
		{
			Result.Add(Reward);
		}
	}

	return Result;
}

TArray<FFactionRankReward> UFactionLibrary::GetUnclaimedRewards(const FString& FactionID)
{
	TArray<FFactionRankReward> Result;
	FPlayerFactionStanding Standing;
	if (!GetPlayerStanding(FactionID, Standing)) return Result;

	FFactionDefinition Faction;
	if (!GetFaction(FactionID, Faction)) return Result;

	for (const FFactionRankReward& Reward : Faction.RankRewards)
	{
		if (Reward.RequiredRank <= Standing.CurrentRank && !Standing.ClaimedRewards.Contains(Reward.RequiredRank))
		{
			Result.Add(Reward);
		}
	}

	return Result;
}

bool UFactionLibrary::ClaimRankReward(const FString& FactionID, EReputationRank Rank, FFactionRankReward& OutReward)
{
	FPlayerFactionStanding* Standing = GetPlayerStandings().Find(FactionID);
	if (!Standing || Standing->CurrentRank < Rank) return false;
	if (Standing->ClaimedRewards.Contains(Rank)) return false;

	FFactionDefinition Faction;
	if (!GetFaction(FactionID, Faction)) return false;

	for (const FFactionRankReward& Reward : Faction.RankRewards)
	{
		if (Reward.RequiredRank == Rank)
		{
			OutReward = Reward;
			Standing->ClaimedRewards.Add(Rank);
			return true;
		}
	}

	return false;
}

float UFactionLibrary::GetShopDiscount(const FString& FactionID)
{
	EReputationRank Rank = GetRank(FactionID);
	FFactionDefinition Faction;
	if (!GetFaction(FactionID, Faction)) return 0.0f;

	float MaxDiscount = 0.0f;
	for (const FFactionRankReward& Reward : Faction.RankRewards)
	{
		if (Reward.RequiredRank <= Rank && Reward.ShopDiscountPercent > MaxDiscount)
		{
			MaxDiscount = Reward.ShopDiscountPercent;
		}
	}

	return MaxDiscount;
}

// ============================================
// DISCOVERY
// ============================================

bool UFactionLibrary::DiscoverFaction(const FString& FactionID)
{
	FPlayerFactionStanding* Standing = GetPlayerStandings().Find(FactionID);
	if (!Standing) return false;
	if (Standing->bDiscovered) return false;

	Standing->bDiscovered = true;

	FFactionDefinition Faction;
	if (GetFaction(FactionID, Faction))
	{
		OnFactionDiscovered.Broadcast(FactionID, Faction);
	}

	return true;
}

bool UFactionLibrary::IsFactionDiscovered(const FString& FactionID)
{
	const FPlayerFactionStanding* Standing = GetPlayerStandings().Find(FactionID);
	return Standing ? Standing->bDiscovered : false;
}

// ============================================
// NPC/ENTITY QUERIES
// ============================================

FString UFactionLibrary::GetNPCFaction(const FString& NPCID)
{
	const FString* FactionID = GetNPCFactionMap().Find(NPCID);
	return FactionID ? *FactionID : FString();
}

TArray<FString> UFactionLibrary::GetFactionNPCs(const FString& FactionID)
{
	FFactionDefinition Faction;
	if (GetFaction(FactionID, Faction))
	{
		return Faction.MemberNPCIDs;
	}
	return TArray<FString>();
}

bool UFactionLibrary::IsHostileWithFaction(const FString& FactionID)
{
	EReputationRank Rank = GetRank(FactionID);
	return Rank == EReputationRank::Hated || Rank == EReputationRank::Hostile;
}

bool UFactionLibrary::CanTradeWithFaction(const FString& FactionID)
{
	EReputationRank Rank = GetRank(FactionID);
	return Rank >= EReputationRank::Neutral;
}

// ============================================
// SAVE/LOAD
// ============================================

FString UFactionLibrary::SaveProgressToJSON()
{
	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> StandingsArray;

	for (const auto& Pair : GetPlayerStandings())
	{
		const FPlayerFactionStanding& Standing = Pair.Value;

		TSharedRef<FJsonObject> StandingObj = MakeShared<FJsonObject>();
		StandingObj->SetStringField(TEXT("faction_id"), Standing.FactionID);
		StandingObj->SetNumberField(TEXT("reputation"), Standing.Reputation);
		StandingObj->SetBoolField(TEXT("discovered"), Standing.bDiscovered);
		StandingObj->SetNumberField(TEXT("current_rank"), static_cast<int32>(Standing.CurrentRank));
		StandingObj->SetNumberField(TEXT("highest_rank"), static_cast<int32>(Standing.HighestRankAchieved));
		StandingObj->SetNumberField(TEXT("total_earned"), Standing.TotalReputationEarned);
		StandingObj->SetNumberField(TEXT("total_lost"), Standing.TotalReputationLost);

		TArray<TSharedPtr<FJsonValue>> ClaimedArray;
		for (EReputationRank Claimed : Standing.ClaimedRewards)
		{
			ClaimedArray.Add(MakeShared<FJsonValueNumber>(static_cast<int32>(Claimed)));
		}
		StandingObj->SetArrayField(TEXT("claimed_rewards"), ClaimedArray);

		StandingsArray.Add(MakeShared<FJsonValueObject>(StandingObj));
	}

	RootObject->SetArrayField(TEXT("standings"), StandingsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject, Writer);

	return OutputString;
}

bool UFactionLibrary::LoadProgressFromJSON(const FString& JSONString)
{
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* StandingsArray;
	if (!RootObject->TryGetArrayField(TEXT("standings"), StandingsArray))
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& StandingValue : *StandingsArray)
	{
		TSharedPtr<FJsonObject> StandingObj = StandingValue->AsObject();
		if (!StandingObj.IsValid()) continue;

		FPlayerFactionStanding Standing;
		Standing.FactionID = StandingObj->GetStringField(TEXT("faction_id"));
		Standing.Reputation = StandingObj->GetIntegerField(TEXT("reputation"));
		Standing.bDiscovered = StandingObj->GetBoolField(TEXT("discovered"));
		Standing.CurrentRank = static_cast<EReputationRank>(StandingObj->GetIntegerField(TEXT("current_rank")));
		Standing.HighestRankAchieved = static_cast<EReputationRank>(StandingObj->GetIntegerField(TEXT("highest_rank")));
		Standing.TotalReputationEarned = StandingObj->GetIntegerField(TEXT("total_earned"));
		Standing.TotalReputationLost = StandingObj->GetIntegerField(TEXT("total_lost"));

		const TArray<TSharedPtr<FJsonValue>>* ClaimedArray;
		if (StandingObj->TryGetArrayField(TEXT("claimed_rewards"), ClaimedArray))
		{
			for (const TSharedPtr<FJsonValue>& ClaimedValue : *ClaimedArray)
			{
				Standing.ClaimedRewards.Add(static_cast<EReputationRank>(ClaimedValue->AsNumber()));
			}
		}

		GetPlayerStandings().Add(Standing.FactionID, Standing);
	}

	return true;
}

void UFactionLibrary::ResetAllProgress()
{
	GetPlayerStandings().Empty();

	// Re-initialize from registered factions
	for (const auto& Pair : GetFactionRegistry())
	{
		const FFactionDefinition& Faction = Pair.Value;
		FPlayerFactionStanding Standing;
		Standing.FactionID = Faction.FactionID;
		Standing.Reputation = Faction.StartingReputation;
		Standing.bDiscovered = !Faction.bHiddenUntilDiscovered;
		Standing.CurrentRank = GetRankFromReputation(Standing.Reputation);
		GetPlayerStandings().Add(Faction.FactionID, Standing);
	}
}

// ============================================
// PRIVATE HELPERS
// ============================================

void UFactionLibrary::ApplyReputationToRelatedFactions(const FString& SourceFactionID, int32 BaseAmount, const FText& Reason, const FString& Source)
{
	FFactionDefinition SourceFaction;
	if (!GetFaction(SourceFactionID, SourceFaction)) return;

	for (const FFactionRelationship& Rel : SourceFaction.Relationships)
	{
		if (Rel.ReputationSharePercent <= 0.0f) continue;

		int32 SharedAmount = FMath::RoundToInt(BaseAmount * Rel.ReputationSharePercent);
		if (SharedAmount == 0) continue;

		// For enemies, invert the reputation
		if (Rel.Relation == EFactionRelation::Enemy)
		{
			SharedAmount = -SharedAmount;
		}

		// Apply (without cascading to prevent infinite loops)
		FPlayerFactionStanding* Standing = GetPlayerStandings().Find(Rel.OtherFactionID);
		if (Standing)
		{
			FFactionDefinition OtherFaction;
			if (GetFaction(Rel.OtherFactionID, OtherFaction))
			{
				Standing->Reputation = FMath::Clamp(Standing->Reputation + SharedAmount, OtherFaction.MinReputation, OtherFaction.MaxReputation);
				UpdateRank(*Standing, OtherFaction);
			}
		}
	}
}

void UFactionLibrary::UpdateRank(FPlayerFactionStanding& Standing, const FFactionDefinition& Faction)
{
	EReputationRank NewRank = GetRankFromReputation(Standing.Reputation);

	if (NewRank > Standing.HighestRankAchieved)
	{
		Standing.HighestRankAchieved = NewRank;
	}

	Standing.CurrentRank = NewRank;
}
