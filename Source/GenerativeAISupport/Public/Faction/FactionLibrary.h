// Faction Library - Blueprint callable functions for faction/reputation system
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Faction/FactionTypes.h"
#include "FactionLibrary.generated.h"

// Delegates for faction events
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnReputationChanged, const FReputationChangeEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRankUp, const FReputationChangeEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFactionDiscovered, const FString&, FactionID, const FFactionDefinition&, Faction);

/**
 * Blueprint Function Library for Faction/Reputation System
 */
UCLASS()
class GENERATIVEAISUPPORT_API UFactionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// FACTION REGISTRY
	// ============================================

	/**
	 * Register a faction
	 * @param Faction - The faction to register
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	static void RegisterFaction(const FFactionDefinition& Faction);

	/**
	 * Load factions from JSON file
	 * @param FilePath - Path to JSON file
	 * @return Number of factions loaded
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	static int32 LoadFactionsFromJSON(const FString& FilePath);

	/**
	 * Get a faction by ID
	 * @param FactionID - The faction ID
	 * @param OutFaction - The faction definition
	 * @return True if found
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	static bool GetFaction(const FString& FactionID, FFactionDefinition& OutFaction);

	/**
	 * Get all registered factions
	 * @return Array of all factions
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	static TArray<FFactionDefinition> GetAllFactions();

	/**
	 * Get all discovered factions for player
	 * @return Array of discovered faction IDs
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System")
	static TArray<FString> GetDiscoveredFactions();

	// ============================================
	// REPUTATION MANAGEMENT
	// ============================================

	/**
	 * Modify reputation with a faction
	 * @param FactionID - The faction
	 * @param Amount - Amount to change (positive or negative)
	 * @param Reason - Why reputation changed
	 * @param Source - What caused it (quest ID, NPC ID, etc.)
	 * @return The reputation change event
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Reputation")
	static FReputationChangeEvent ModifyReputation(const FString& FactionID, int32 Amount, const FText& Reason, const FString& Source = TEXT(""));

	/**
	 * Set reputation to specific value
	 * @param FactionID - The faction
	 * @param NewReputation - New reputation value
	 * @return The reputation change event
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Reputation")
	static FReputationChangeEvent SetReputation(const FString& FactionID, int32 NewReputation);

	/**
	 * Get current reputation with faction
	 * @param FactionID - The faction
	 * @return Current reputation value
	 */
	UFUNCTION(BlueprintPure, Category = "Faction System|Reputation")
	static int32 GetReputation(const FString& FactionID);

	/**
	 * Get current rank with faction
	 * @param FactionID - The faction
	 * @return Current reputation rank
	 */
	UFUNCTION(BlueprintPure, Category = "Faction System|Reputation")
	static EReputationRank GetRank(const FString& FactionID);

	/**
	 * Get player standing with faction
	 * @param FactionID - The faction
	 * @param OutStanding - The player's standing
	 * @return True if faction exists
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Reputation")
	static bool GetPlayerStanding(const FString& FactionID, FPlayerFactionStanding& OutStanding);

	/**
	 * Get all player standings
	 * @return Array of all standings
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Reputation")
	static TArray<FPlayerFactionStanding> GetAllPlayerStandings();

	// ============================================
	// RANK UTILITIES
	// ============================================

	/**
	 * Get rank from reputation value
	 * @param Reputation - The reputation value
	 * @return The corresponding rank
	 */
	UFUNCTION(BlueprintPure, Category = "Faction System|Rank")
	static EReputationRank GetRankFromReputation(int32 Reputation);

	/**
	 * Get reputation range for a rank
	 * @param Rank - The rank to query
	 * @param OutMin - Minimum reputation for rank
	 * @param OutMax - Maximum reputation for rank
	 */
	UFUNCTION(BlueprintPure, Category = "Faction System|Rank")
	static void GetRankThresholds(EReputationRank Rank, int32& OutMin, int32& OutMax);

	/**
	 * Get display name for rank
	 * @param Rank - The rank
	 * @return Localized display name
	 */
	UFUNCTION(BlueprintPure, Category = "Faction System|Rank")
	static FText GetRankDisplayName(EReputationRank Rank);

	/**
	 * Get color for rank (for UI)
	 * @param Rank - The rank
	 * @return Color representing the rank
	 */
	UFUNCTION(BlueprintPure, Category = "Faction System|Rank")
	static FLinearColor GetRankColor(EReputationRank Rank);

	/**
	 * Get progress to next rank
	 * @param FactionID - The faction
	 * @param OutCurrentRep - Current reputation
	 * @param OutNextRankRep - Reputation needed for next rank
	 * @param OutProgress - Progress percentage (0.0 - 1.0)
	 * @return True if not at max rank
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Rank")
	static bool GetProgressToNextRank(const FString& FactionID, int32& OutCurrentRep, int32& OutNextRankRep, float& OutProgress);

	// ============================================
	// FACTION RELATIONSHIPS
	// ============================================

	/**
	 * Get relationship between two factions
	 * @param FactionA - First faction
	 * @param FactionB - Second faction
	 * @return Relationship type
	 */
	UFUNCTION(BlueprintPure, Category = "Faction System|Relationships")
	static EFactionRelation GetFactionRelationship(const FString& FactionA, const FString& FactionB);

	/**
	 * Compare two factions for the player
	 * @param FactionA - First faction
	 * @param FactionB - Second faction
	 * @return Comparison result
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Relationships")
	static FFactionCompareResult CompareFactions(const FString& FactionA, const FString& FactionB);

	/**
	 * Get allied factions
	 * @param FactionID - The faction to check
	 * @return Array of allied faction IDs
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Relationships")
	static TArray<FString> GetAlliedFactions(const FString& FactionID);

	/**
	 * Get enemy factions
	 * @param FactionID - The faction to check
	 * @return Array of enemy faction IDs
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Relationships")
	static TArray<FString> GetEnemyFactions(const FString& FactionID);

	// ============================================
	// REWARDS
	// ============================================

	/**
	 * Get available rank rewards
	 * @param FactionID - The faction
	 * @return Array of available rewards
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Rewards")
	static TArray<FFactionRankReward> GetAvailableRewards(const FString& FactionID);

	/**
	 * Get unclaimed rewards
	 * @param FactionID - The faction
	 * @return Array of unclaimed rewards
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Rewards")
	static TArray<FFactionRankReward> GetUnclaimedRewards(const FString& FactionID);

	/**
	 * Claim a rank reward
	 * @param FactionID - The faction
	 * @param Rank - The rank reward to claim
	 * @param OutReward - The claimed reward
	 * @return True if claimed
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Rewards")
	static bool ClaimRankReward(const FString& FactionID, EReputationRank Rank, FFactionRankReward& OutReward);

	/**
	 * Get current shop discount for faction
	 * @param FactionID - The faction
	 * @return Discount percentage (0.0 - 0.5)
	 */
	UFUNCTION(BlueprintPure, Category = "Faction System|Rewards")
	static float GetShopDiscount(const FString& FactionID);

	// ============================================
	// DISCOVERY
	// ============================================

	/**
	 * Discover a faction
	 * @param FactionID - The faction to discover
	 * @return True if newly discovered
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Discovery")
	static bool DiscoverFaction(const FString& FactionID);

	/**
	 * Check if faction is discovered
	 * @param FactionID - The faction to check
	 * @return True if discovered
	 */
	UFUNCTION(BlueprintPure, Category = "Faction System|Discovery")
	static bool IsFactionDiscovered(const FString& FactionID);

	// ============================================
	// NPC/ENTITY QUERIES
	// ============================================

	/**
	 * Get faction of an NPC
	 * @param NPCID - The NPC ID
	 * @return Faction ID (empty if none)
	 */
	UFUNCTION(BlueprintPure, Category = "Faction System|Query")
	static FString GetNPCFaction(const FString& NPCID);

	/**
	 * Get all NPCs in a faction
	 * @param FactionID - The faction
	 * @return Array of NPC IDs
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Query")
	static TArray<FString> GetFactionNPCs(const FString& FactionID);

	/**
	 * Check if player is hostile with faction
	 * @param FactionID - The faction
	 * @return True if Hated or Hostile rank
	 */
	UFUNCTION(BlueprintPure, Category = "Faction System|Query")
	static bool IsHostileWithFaction(const FString& FactionID);

	/**
	 * Check if player can trade with faction
	 * @param FactionID - The faction
	 * @return True if rank allows trading
	 */
	UFUNCTION(BlueprintPure, Category = "Faction System|Query")
	static bool CanTradeWithFaction(const FString& FactionID);

	// ============================================
	// SAVE/LOAD
	// ============================================

	/**
	 * Save faction progress to JSON
	 * @return JSON string of player standings
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Save")
	static FString SaveProgressToJSON();

	/**
	 * Load faction progress from JSON
	 * @param JSONString - The JSON to parse
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Save")
	static bool LoadProgressFromJSON(const FString& JSONString);

	/**
	 * Reset all faction progress
	 */
	UFUNCTION(BlueprintCallable, Category = "Faction System|Save")
	static void ResetAllProgress();

	// ============================================
	// EVENTS
	// ============================================

	/** Called when player ranks up */
	static FOnRankUp OnRankUp;

	/** Called when faction is discovered */
	static FOnFactionDiscovered OnFactionDiscovered;

private:
	/** Faction registry */
	static TMap<FString, FFactionDefinition>& GetFactionRegistry();

	/** Player standings */
	static TMap<FString, FPlayerFactionStanding>& GetPlayerStandings();

	/** NPC to faction mapping (cached) */
	static TMap<FString, FString>& GetNPCFactionMap();

	/** Apply reputation to related factions */
	static void ApplyReputationToRelatedFactions(const FString& SourceFactionID, int32 BaseAmount, const FText& Reason, const FString& Source);

	/** Update rank from reputation */
	static void UpdateRank(FPlayerFactionStanding& Standing, const FFactionDefinition& Faction);
};
