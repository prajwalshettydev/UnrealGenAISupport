// SaveGame Types - Data structures for save/load system
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveGameTypes.generated.h"

/**
 * Player character data for saving
 */
USTRUCT(BlueprintType)
struct FPlayerSaveData
{
	GENERATED_BODY()

	/** Player display name */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString PlayerName;

	/** Character class/type */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString CharacterClass;

	/** Current level */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 Level = 1;

	/** Current experience points */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 Experience = 0;

	/** Experience needed for next level */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 ExperienceToNextLevel = 100;

	/** Current health */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	float CurrentHealth = 100.f;

	/** Maximum health */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	float MaxHealth = 100.f;

	/** Current mana/stamina */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	float CurrentMana = 100.f;

	/** Maximum mana/stamina */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	float MaxMana = 100.f;

	/** Player world location */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FVector Location = FVector::ZeroVector;

	/** Player rotation */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FRotator Rotation = FRotator::ZeroRotator;

	/** Current map/level name */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString CurrentMap;

	/** Attribute points available */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 AttributePoints = 0;

	/** Skill points available */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 SkillPoints = 0;

	/** Base attributes (Strength, Dexterity, etc.) */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TMap<FString, int32> Attributes;

	/** Learned skills/abilities */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FString> LearnedSkills;

	/** Unlocked titles */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FString> UnlockedTitles;

	/** Currently active title */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString ActiveTitle;
};

/**
 * World state data for saving
 */
USTRUCT(BlueprintType)
struct FWorldSaveData
{
	GENERATED_BODY()

	/** Current in-game time (hours since start) */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	float GameTimeHours = 8.f;

	/** Current day number */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 DayNumber = 1;

	/** Current weather state */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString CurrentWeather = TEXT("Clear");

	/** Discovered locations */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FString> DiscoveredLocations;

	/** Unlocked fast travel points */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FString> UnlockedFastTravel;

	/** World flags (story triggers, one-time events, etc.) */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TMap<FString, bool> WorldFlags;

	/** Killed unique enemies (won't respawn) */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FString> KilledUniqueEnemies;

	/** Looted unique chests */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FString> LootedUniqueChests;
};

/**
 * Reputation data with factions
 */
USTRUCT(BlueprintType)
struct FFactionReputation
{
	GENERATED_BODY()

	/** Faction ID */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString FactionID;

	/** Reputation value (-1000 to 1000) */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 Reputation = 0;

	/** Reputation rank (Hostile, Unfriendly, Neutral, Friendly, Honored, Exalted) */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString Rank = TEXT("Neutral");
};

/**
 * Save slot metadata (for save slot UI)
 */
USTRUCT(BlueprintType)
struct FSaveSlotInfo
{
	GENERATED_BODY()

	/** Slot index (0-9) */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 SlotIndex = 0;

	/** Custom save name */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString SaveName;

	/** Player name at time of save */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString PlayerName;

	/** Player level at time of save */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 PlayerLevel = 1;

	/** Location name at time of save */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString LocationName;

	/** Real-world timestamp */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FDateTime Timestamp;

	/** Total playtime in seconds */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	float PlaytimeSeconds = 0.f;

	/** Screenshot path (optional) */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString ScreenshotPath;

	/** Save game version for compatibility */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 SaveVersion = 1;

	/** Is this slot empty? */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	bool bIsEmpty = true;

	/** Is this an autosave? */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	bool bIsAutosave = false;
};

/**
 * Complete save game data
 */
UCLASS()
class GENERATIVEAISUPPORT_API UTitanSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UTitanSaveGame();

	/** Save file version for migration */
	UPROPERTY()
	int32 SaveVersion = 1;

	/** Slot metadata */
	UPROPERTY()
	FSaveSlotInfo SlotInfo;

	/** Player character data */
	UPROPERTY()
	FPlayerSaveData PlayerData;

	/** World state */
	UPROPERTY()
	FWorldSaveData WorldData;

	/** Quest system state (JSON) */
	UPROPERTY()
	FString QuestDataJSON;

	/** Inventory state (JSON) */
	UPROPERTY()
	FString InventoryDataJSON;

	/** Achievement progress (JSON) */
	UPROPERTY()
	FString AchievementDataJSON;

	/** Shop states (JSON) */
	UPROPERTY()
	FString ShopDataJSON;

	/** NPC conversation histories (JSON) */
	UPROPERTY()
	FString NPCConversationJSON;

	/** Faction reputations */
	UPROPERTY()
	TArray<FFactionReputation> FactionReputations;

	/** Statistics (kills, deaths, items collected, etc.) */
	UPROPERTY()
	TMap<FString, int32> Statistics;

	/** Settings that are per-save (difficulty, etc.) */
	UPROPERTY()
	TMap<FString, FString> SaveSettings;

	// Current save version for compatibility checks
	static const int32 CURRENT_SAVE_VERSION = 1;
};

/**
 * Result of save/load operations
 */
USTRUCT(BlueprintType)
struct FSaveLoadResult
{
	GENERATED_BODY()

	/** Was operation successful? */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	bool bSuccess = false;

	/** Error message if failed */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FText ErrorMessage;

	/** Slot that was affected */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 SlotIndex = -1;

	/** Time taken for operation */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	float OperationTimeMs = 0.f;
};
