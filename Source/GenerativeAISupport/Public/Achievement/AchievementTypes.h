// Achievement System Types - Data structures for achievements
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AchievementTypes.generated.h"

/**
 * Achievement category
 */
UENUM(BlueprintType)
enum class EAchievementCategory : uint8
{
	Combat			UMETA(DisplayName = "Combat"),
	Exploration		UMETA(DisplayName = "Exploration"),
	Collection		UMETA(DisplayName = "Collection"),
	Social			UMETA(DisplayName = "Social"),
	Story			UMETA(DisplayName = "Story"),
	Crafting		UMETA(DisplayName = "Crafting"),
	Hidden			UMETA(DisplayName = "Hidden"),
	Miscellaneous	UMETA(DisplayName = "Miscellaneous")
};

/**
 * Achievement rarity/difficulty
 */
UENUM(BlueprintType)
enum class EAchievementRarity : uint8
{
	Common		UMETA(DisplayName = "Common"),
	Uncommon	UMETA(DisplayName = "Uncommon"),
	Rare		UMETA(DisplayName = "Rare"),
	Epic		UMETA(DisplayName = "Epic"),
	Legendary	UMETA(DisplayName = "Legendary")
};

/**
 * Trigger type for achievements
 */
UENUM(BlueprintType)
enum class EAchievementTrigger : uint8
{
	Count		UMETA(DisplayName = "Count"),		// Reach a number (kill X enemies)
	Threshold	UMETA(DisplayName = "Threshold"),	// Reach a threshold (level 10)
	Flag		UMETA(DisplayName = "Flag"),		// Boolean flag (complete quest)
	Collection	UMETA(DisplayName = "Collection"),	// Collect specific items
	Discovery	UMETA(DisplayName = "Discovery")	// Discover a location/secret
};

/**
 * Achievement reward
 */
USTRUCT(BlueprintType)
struct FAchievementReward
{
	GENERATED_BODY()

	/** Experience points reward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 XP = 0;

	/** Gold reward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 Gold = 0;

	/** Item reward IDs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	TArray<FString> ItemIDs;

	/** Title/Suffix unlocked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	FString TitleUnlocked;

	/** Achievement points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 AchievementPoints = 10;
};

/**
 * Achievement definition
 */
USTRUCT(BlueprintType)
struct FAchievementDefinition
{
	GENERATED_BODY()

	/** Unique achievement ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	FString AchievementID;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	FText DisplayName;

	/** Description (what to do) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	FText Description;

	/** Hidden description (before unlock) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	FText HiddenDescription;

	/** Category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	EAchievementCategory Category = EAchievementCategory::Miscellaneous;

	/** Rarity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	EAchievementRarity Rarity = EAchievementRarity::Common;

	/** Trigger type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	EAchievementTrigger TriggerType = EAchievementTrigger::Count;

	/** Stat/Event to track */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	FString TrackedStat;

	/** Target value to reach */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	int32 TargetValue = 1;

	/** Is this a hidden achievement? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	bool bHidden = false;

	/** Can show progress? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	bool bShowProgress = true;

	/** Prerequisites (other achievement IDs) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	TArray<FString> Prerequisites;

	/** Rewards */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	FAchievementReward Reward;

	/** Icon path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement")
	TSoftObjectPtr<UTexture2D> Icon;
};

/**
 * Player's achievement progress
 */
USTRUCT(BlueprintType)
struct FAchievementProgress
{
	GENERATED_BODY()

	/** Achievement ID */
	UPROPERTY(BlueprintReadOnly, Category = "Achievement")
	FString AchievementID;

	/** Current progress value */
	UPROPERTY(BlueprintReadOnly, Category = "Achievement")
	int32 CurrentValue = 0;

	/** Is completed? */
	UPROPERTY(BlueprintReadOnly, Category = "Achievement")
	bool bCompleted = false;

	/** Completion timestamp */
	UPROPERTY(BlueprintReadOnly, Category = "Achievement")
	FDateTime CompletionTime;

	/** Was reward claimed? */
	UPROPERTY(BlueprintReadOnly, Category = "Achievement")
	bool bRewardClaimed = false;
};

/**
 * Achievement Data Asset
 */
UCLASS(BlueprintType)
class GENERATIVEAISUPPORT_API UAchievementDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Achievement")
	FAchievementDefinition Achievement;
};
