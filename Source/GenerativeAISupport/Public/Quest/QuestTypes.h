// Quest System Types - Data structures for the Quest System
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "QuestTypes.generated.h"

/**
 * Quest Status
 */
UENUM(BlueprintType)
enum class EQuestStatus : uint8
{
	Locked		UMETA(DisplayName = "Locked"),
	Available	UMETA(DisplayName = "Available"),
	Active		UMETA(DisplayName = "Active"),
	Completed	UMETA(DisplayName = "Completed"),
	Failed		UMETA(DisplayName = "Failed")
};

/**
 * Quest Type
 */
UENUM(BlueprintType)
enum class EQuestType : uint8
{
	Main		UMETA(DisplayName = "Main Quest"),
	Side		UMETA(DisplayName = "Side Quest"),
	Daily		UMETA(DisplayName = "Daily Quest"),
	Discovery	UMETA(DisplayName = "Discovery"),
	Event		UMETA(DisplayName = "Event")
};

/**
 * Objective Type
 */
UENUM(BlueprintType)
enum class EObjectiveType : uint8
{
	Kill		UMETA(DisplayName = "Kill"),
	Collect		UMETA(DisplayName = "Collect"),
	Deliver		UMETA(DisplayName = "Deliver"),
	Escort		UMETA(DisplayName = "Escort"),
	Talk		UMETA(DisplayName = "Talk"),
	Explore		UMETA(DisplayName = "Explore"),
	Interact	UMETA(DisplayName = "Interact"),
	Survive		UMETA(DisplayName = "Survive")
};

/**
 * Quest Objective
 */
USTRUCT(BlueprintType)
struct FQuestObjective
{
	GENERATED_BODY()

	/** Unique ID for this objective */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FString ObjectiveID;

	/** Type of objective */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	EObjectiveType Type = EObjectiveType::Kill;

	/** Description shown to player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FText Description;

	/** Target ID (monster type, item type, NPC id, location id, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FString TargetID;

	/** Required count to complete */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	int32 RequiredCount = 1;

	/** Current progress */
	UPROPERTY(BlueprintReadOnly, Category = "Objective")
	int32 CurrentCount = 0;

	/** Is this objective optional? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	bool bOptional = false;

	/** Is this objective hidden until relevant? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	bool bHidden = false;

	/** Is this objective completed? */
	bool IsCompleted() const { return CurrentCount >= RequiredCount; }

	/** Get progress as percentage 0-1 */
	float GetProgress() const
	{
		return RequiredCount > 0 ? FMath::Clamp((float)CurrentCount / (float)RequiredCount, 0.f, 1.f) : 1.f;
	}
};

/**
 * Quest Reward
 */
USTRUCT(BlueprintType)
struct FQuestReward
{
	GENERATED_BODY()

	/** Experience points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 XP = 0;

	/** Gold/Currency */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 Gold = 0;

	/** Item IDs to give (guaranteed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	TArray<FString> ItemIDs;

	/** Reputation changes (FactionID -> Amount) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	TMap<FString, int32> ReputationChanges;

	/** Quest IDs to unlock */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	TArray<FString> UnlocksQuests;

	/** Achievement IDs to grant */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	TArray<FString> GrantsAchievements;
};

/**
 * Quest Requirements
 */
USTRUCT(BlueprintType)
struct FQuestRequirements
{
	GENERATED_BODY()

	/** Required player level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredLevel = 1;

	/** Required completed quest IDs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	TArray<FString> RequiredQuests;

	/** Required items (must possess) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	TArray<FString> RequiredItems;

	/** Required reputation (FactionID -> MinAmount) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	TMap<FString, int32> RequiredReputation;
};

/**
 * Quest Data - Complete quest definition
 */
USTRUCT(BlueprintType)
struct FQuestData
{
	GENERATED_BODY()

	/** Unique Quest ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString QuestID;

	/** Quest Title */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText Title;

	/** Quest Description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText Description;

	/** Quest Type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	EQuestType QuestType = EQuestType::Side;

	/** Current Status */
	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	EQuestStatus Status = EQuestStatus::Locked;

	/** Suggested level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 SuggestedLevel = 1;

	/** Quest giver NPC ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString QuestGiverID;

	/** Turn-in NPC ID (if different from giver) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString TurnInNPCID;

	/** Time limit in seconds (0 = no limit) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	float TimeLimit = 0.f;

	/** Is this quest repeatable? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	bool bRepeatable = false;

	/** Cooldown for repeatable quests (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	float RepeatCooldown = 86400.f; // 24 hours

	/** Quest objectives */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FQuestObjective> Objectives;

	/** Quest rewards */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FQuestReward Rewards;

	/** Requirements to accept quest */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FQuestRequirements Requirements;

	/** When quest was started */
	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FDateTime StartedAt;

	/** When quest was completed */
	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FDateTime CompletedAt;

	/** Check if all required objectives are completed */
	bool AreRequiredObjectivesComplete() const
	{
		for (const FQuestObjective& Obj : Objectives)
		{
			if (!Obj.bOptional && !Obj.IsCompleted())
			{
				return false;
			}
		}
		return true;
	}

	/** Get overall quest progress 0-1 */
	float GetOverallProgress() const
	{
		if (Objectives.Num() == 0) return 1.f;

		float TotalProgress = 0.f;
		int32 RequiredCount = 0;

		for (const FQuestObjective& Obj : Objectives)
		{
			if (!Obj.bOptional)
			{
				TotalProgress += Obj.GetProgress();
				RequiredCount++;
			}
		}

		return RequiredCount > 0 ? TotalProgress / RequiredCount : 1.f;
	}
};

/**
 * Active Quest Instance - Runtime data for an active quest
 */
USTRUCT(BlueprintType)
struct FActiveQuest
{
	GENERATED_BODY()

	/** The quest data */
	UPROPERTY(BlueprintReadOnly, Category = "Active Quest")
	FQuestData QuestData;

	/** Time remaining (if timed) */
	UPROPERTY(BlueprintReadOnly, Category = "Active Quest")
	float TimeRemaining = 0.f;

	/** Is being tracked on HUD */
	UPROPERTY(BlueprintReadWrite, Category = "Active Quest")
	bool bTracked = false;
};
