// Faction Types - Data structures for faction and reputation system
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "FactionTypes.generated.h"

/**
 * Reputation rank thresholds and names
 */
UENUM(BlueprintType)
enum class EReputationRank : uint8
{
	Hated UMETA(DisplayName = "Hated"),           // -1000 to -600
	Hostile UMETA(DisplayName = "Hostile"),       // -599 to -300
	Unfriendly UMETA(DisplayName = "Unfriendly"), // -299 to -1
	Neutral UMETA(DisplayName = "Neutral"),       // 0 to 299
	Friendly UMETA(DisplayName = "Friendly"),     // 300 to 599
	Honored UMETA(DisplayName = "Honored"),       // 600 to 899
	Exalted UMETA(DisplayName = "Exalted")        // 900 to 1000
};

/**
 * Faction relationship type
 */
UENUM(BlueprintType)
enum class EFactionRelation : uint8
{
	/** Hated enemy - attacks on sight */
	Enemy UMETA(DisplayName = "Enemy"),

	/** Unfriendly but not hostile */
	Unfriendly UMETA(DisplayName = "Unfriendly"),

	/** No special relationship */
	Neutral UMETA(DisplayName = "Neutral"),

	/** Friendly - shares reputation gains */
	Friendly UMETA(DisplayName = "Friendly"),

	/** Allied - strong reputation sharing */
	Allied UMETA(DisplayName = "Allied")
};

/**
 * Relationship between two factions
 */
USTRUCT(BlueprintType)
struct FFactionRelationship
{
	GENERATED_BODY()

	/** The other faction ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FString OtherFactionID;

	/** Relationship type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	EFactionRelation Relation = EFactionRelation::Neutral;

	/** Reputation sharing percentage (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReputationSharePercent = 0.0f;
};

/**
 * Faction rank reward
 */
USTRUCT(BlueprintType)
struct FFactionRankReward
{
	GENERATED_BODY()

	/** Rank required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	EReputationRank RequiredRank = EReputationRank::Neutral;

	/** Title unlocked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FString TitleUnlocked;

	/** Discount percentage at faction shops */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float ShopDiscountPercent = 0.0f;

	/** Items unlocked for purchase */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	TArray<FString> UnlockedItems;

	/** Quests unlocked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	TArray<FString> UnlockedQuests;

	/** Unique reward item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FString UniqueRewardItem;

	/** XP bonus for faction quests */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float QuestXPBonus = 0.0f;
};

/**
 * Complete faction definition
 */
USTRUCT(BlueprintType)
struct FFactionDefinition
{
	GENERATED_BODY()

	/** Unique faction identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FString FactionID;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FText DisplayName;

	/** Short description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction", meta = (MultiLine = true))
	FText Description;

	/** Faction icon/emblem path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FString IconPath;

	/** Faction color for UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FLinearColor FactionColor = FLinearColor::White;

	/** Starting reputation for new players */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	int32 StartingReputation = 0;

	/** Is this faction hidden until discovered? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	bool bHiddenUntilDiscovered = false;

	/** Home region/location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FString HomeRegion;

	/** NPCs belonging to this faction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	TArray<FString> MemberNPCIDs;

	/** Shops owned by this faction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	TArray<FString> FactionShopIDs;

	/** Relationships with other factions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	TArray<FFactionRelationship> Relationships;

	/** Rank rewards */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	TArray<FFactionRankReward> RankRewards;

	/** Reputation decay per day (if any) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	int32 DailyReputationDecay = 0;

	/** Minimum reputation (can't go below this) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	int32 MinReputation = -1000;

	/** Maximum reputation (can't exceed this) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	int32 MaxReputation = 1000;
};

/**
 * Player's standing with a faction
 */
USTRUCT(BlueprintType)
struct FPlayerFactionStanding
{
	GENERATED_BODY()

	/** Faction ID */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	FString FactionID;

	/** Current reputation value */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	int32 Reputation = 0;

	/** Has player discovered this faction? */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	bool bDiscovered = false;

	/** Current rank */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	EReputationRank CurrentRank = EReputationRank::Neutral;

	/** Highest rank ever achieved */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	EReputationRank HighestRankAchieved = EReputationRank::Neutral;

	/** Total reputation earned (lifetime) */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	int32 TotalReputationEarned = 0;

	/** Total reputation lost (lifetime) */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	int32 TotalReputationLost = 0;

	/** Claimed rank rewards */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	TArray<EReputationRank> ClaimedRewards;
};

/**
 * Reputation change event
 */
USTRUCT(BlueprintType)
struct FReputationChangeEvent
{
	GENERATED_BODY()

	/** Faction affected */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	FString FactionID;

	/** Amount changed */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	int32 Amount = 0;

	/** New total reputation */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	int32 NewReputation = 0;

	/** Old rank */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	EReputationRank OldRank = EReputationRank::Neutral;

	/** New rank */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	EReputationRank NewRank = EReputationRank::Neutral;

	/** Did rank change? */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	bool bRankChanged = false;

	/** Reason for change */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	FText Reason;

	/** Source (quest, NPC, action, etc.) */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	FString Source;
};

/**
 * Faction comparison result
 */
USTRUCT(BlueprintType)
struct FFactionCompareResult
{
	GENERATED_BODY()

	/** First faction ID */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	FString FactionA;

	/** Second faction ID */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	FString FactionB;

	/** Relationship between them */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	EFactionRelation Relation = EFactionRelation::Neutral;

	/** Will helping A hurt B? */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	bool bHelpingAHurtsB = false;

	/** Percentage of reputation shared */
	UPROPERTY(BlueprintReadWrite, Category = "Faction")
	float SharedReputationPercent = 0.0f;
};
