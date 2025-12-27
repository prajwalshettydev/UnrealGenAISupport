// SaveGame Library - Blueprint callable functions for save/load system
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaveGame/SaveGameTypes.h"
#include "SaveGameLibrary.generated.h"

// Forward declarations
class UInventoryComponent;
class UQuestSubsystem;

// Delegates for async operations
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSaveComplete, const FSaveLoadResult&, Result);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnLoadComplete, const FSaveLoadResult&, Result);

/**
 * Blueprint Function Library for Save/Load System
 */
UCLASS()
class GENERATIVEAISUPPORT_API USaveGameLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// SAVE OPERATIONS
	// ============================================

	/**
	 * Save game to slot (synchronous)
	 * @param SlotIndex - Save slot (0-9)
	 * @param SaveName - Custom name for the save
	 * @param PlayerData - Player character data
	 * @param WorldData - World state data
	 * @param WorldContext - World context for subsystem access
	 * @return Result of save operation
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System", meta = (WorldContext = "WorldContext"))
	static FSaveLoadResult SaveGame(
		int32 SlotIndex,
		const FString& SaveName,
		const FPlayerSaveData& PlayerData,
		const FWorldSaveData& WorldData,
		const UObject* WorldContext
	);

	/**
	 * Quick save to dedicated quick save slot
	 * @param PlayerData - Player character data
	 * @param WorldData - World state data
	 * @param WorldContext - World context
	 * @return Result
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System", meta = (WorldContext = "WorldContext"))
	static FSaveLoadResult QuickSave(
		const FPlayerSaveData& PlayerData,
		const FWorldSaveData& WorldData,
		const UObject* WorldContext
	);

	/**
	 * Auto save (uses rotating autosave slots)
	 * @param PlayerData - Player character data
	 * @param WorldData - World state data
	 * @param WorldContext - World context
	 * @return Result
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System", meta = (WorldContext = "WorldContext"))
	static FSaveLoadResult AutoSave(
		const FPlayerSaveData& PlayerData,
		const FWorldSaveData& WorldData,
		const UObject* WorldContext
	);

	/**
	 * Save game asynchronously
	 * @param SlotIndex - Save slot
	 * @param SaveName - Save name
	 * @param PlayerData - Player data
	 * @param WorldData - World data
	 * @param WorldContext - World context
	 * @param OnComplete - Callback when done
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System", meta = (WorldContext = "WorldContext"))
	static void SaveGameAsync(
		int32 SlotIndex,
		const FString& SaveName,
		const FPlayerSaveData& PlayerData,
		const FWorldSaveData& WorldData,
		const UObject* WorldContext,
		FOnSaveComplete OnComplete
	);

	// ============================================
	// LOAD OPERATIONS
	// ============================================

	/**
	 * Load game from slot (synchronous)
	 * @param SlotIndex - Save slot to load
	 * @param OutPlayerData - Loaded player data
	 * @param OutWorldData - Loaded world data
	 * @param WorldContext - World context for subsystem access
	 * @return Result of load operation
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System", meta = (WorldContext = "WorldContext"))
	static FSaveLoadResult LoadGame(
		int32 SlotIndex,
		FPlayerSaveData& OutPlayerData,
		FWorldSaveData& OutWorldData,
		const UObject* WorldContext
	);

	/**
	 * Quick load from quick save slot
	 * @param OutPlayerData - Loaded player data
	 * @param OutWorldData - Loaded world data
	 * @param WorldContext - World context
	 * @return Result
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System", meta = (WorldContext = "WorldContext"))
	static FSaveLoadResult QuickLoad(
		FPlayerSaveData& OutPlayerData,
		FWorldSaveData& OutWorldData,
		const UObject* WorldContext
	);

	/**
	 * Load game asynchronously
	 * @param SlotIndex - Save slot
	 * @param WorldContext - World context
	 * @param OnComplete - Callback when done
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System", meta = (WorldContext = "WorldContext"))
	static void LoadGameAsync(
		int32 SlotIndex,
		const UObject* WorldContext,
		FOnLoadComplete OnComplete
	);

	// ============================================
	// SLOT MANAGEMENT
	// ============================================

	/**
	 * Get info about all save slots
	 * @return Array of slot info
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System")
	static TArray<FSaveSlotInfo> GetAllSaveSlots();

	/**
	 * Get info about a specific slot
	 * @param SlotIndex - Slot to query
	 * @return Slot info
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System")
	static FSaveSlotInfo GetSaveSlotInfo(int32 SlotIndex);

	/**
	 * Check if a slot has a save
	 * @param SlotIndex - Slot to check
	 * @return True if slot has save data
	 */
	UFUNCTION(BlueprintPure, Category = "Save System")
	static bool DoesSaveExist(int32 SlotIndex);

	/**
	 * Delete a save slot
	 * @param SlotIndex - Slot to delete
	 * @return True if deleted
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System")
	static bool DeleteSaveSlot(int32 SlotIndex);

	/**
	 * Get the most recent save slot
	 * @return Slot index (-1 if no saves)
	 */
	UFUNCTION(BlueprintPure, Category = "Save System")
	static int32 GetMostRecentSaveSlot();

	/**
	 * Get first empty save slot
	 * @return Slot index (-1 if all full)
	 */
	UFUNCTION(BlueprintPure, Category = "Save System")
	static int32 GetFirstEmptySlot();

	// ============================================
	// PLAYER DATA HELPERS
	// ============================================

	/**
	 * Create player save data from current player state
	 * @param PlayerPawn - The player pawn
	 * @param InventoryComponent - Player's inventory
	 * @return Populated player data struct
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System")
	static FPlayerSaveData CreatePlayerSaveData(
		APawn* PlayerPawn,
		UInventoryComponent* InventoryComponent
	);

	/**
	 * Apply loaded player data to player
	 * @param PlayerPawn - The player pawn to update
	 * @param PlayerData - Data to apply
	 * @param InventoryComponent - Inventory to restore
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System")
	static bool ApplyPlayerSaveData(
		APawn* PlayerPawn,
		const FPlayerSaveData& PlayerData,
		UInventoryComponent* InventoryComponent
	);

	// ============================================
	// WORLD DATA HELPERS
	// ============================================

	/**
	 * Mark a location as discovered
	 * @param WorldData - World data to update
	 * @param LocationID - Location ID to mark
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System")
	static void MarkLocationDiscovered(UPARAM(ref) FWorldSaveData& WorldData, const FString& LocationID);

	/**
	 * Check if location is discovered
	 * @param WorldData - World data to check
	 * @param LocationID - Location to check
	 * @return True if discovered
	 */
	UFUNCTION(BlueprintPure, Category = "Save System")
	static bool IsLocationDiscovered(const FWorldSaveData& WorldData, const FString& LocationID);

	/**
	 * Set a world flag
	 * @param WorldData - World data to update
	 * @param FlagName - Flag name
	 * @param Value - Flag value
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System")
	static void SetWorldFlag(UPARAM(ref) FWorldSaveData& WorldData, const FString& FlagName, bool Value);

	/**
	 * Get a world flag
	 * @param WorldData - World data to check
	 * @param FlagName - Flag name
	 * @return Flag value (false if not set)
	 */
	UFUNCTION(BlueprintPure, Category = "Save System")
	static bool GetWorldFlag(const FWorldSaveData& WorldData, const FString& FlagName);

	// ============================================
	// STATISTICS
	// ============================================

	/**
	 * Increment a statistic
	 * @param StatName - Statistic name
	 * @param Amount - Amount to add
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System|Statistics")
	static void IncrementStatistic(const FString& StatName, int32 Amount = 1);

	/**
	 * Get a statistic value
	 * @param StatName - Statistic name
	 * @return Current value
	 */
	UFUNCTION(BlueprintPure, Category = "Save System|Statistics")
	static int32 GetStatistic(const FString& StatName);

	/**
	 * Get all statistics
	 * @return Map of all statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System|Statistics")
	static TMap<FString, int32> GetAllStatistics();

	// ============================================
	// PLAYTIME TRACKING
	// ============================================

	/**
	 * Start tracking playtime
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System|Playtime")
	static void StartPlaytimeTracking();

	/**
	 * Get current session playtime
	 * @return Playtime in seconds
	 */
	UFUNCTION(BlueprintPure, Category = "Save System|Playtime")
	static float GetSessionPlaytime();

	/**
	 * Get total playtime from save
	 * @param SlotIndex - Save slot to check
	 * @return Total playtime in seconds
	 */
	UFUNCTION(BlueprintPure, Category = "Save System|Playtime")
	static float GetTotalPlaytime(int32 SlotIndex);

	/**
	 * Format playtime as string
	 * @param PlaytimeSeconds - Playtime in seconds
	 * @return Formatted string (e.g. "12:34:56")
	 */
	UFUNCTION(BlueprintPure, Category = "Save System|Playtime")
	static FString FormatPlaytime(float PlaytimeSeconds);

	// ============================================
	// CONFIGURATION
	// ============================================

	/** Maximum number of save slots */
	static const int32 MAX_SAVE_SLOTS = 10;

	/** Quick save slot index */
	static const int32 QUICKSAVE_SLOT = 100;

	/** Autosave slot indices (rotating) */
	static const int32 AUTOSAVE_SLOT_START = 101;
	static const int32 AUTOSAVE_SLOT_COUNT = 3;

private:
	/** Get slot name for saving */
	static FString GetSlotName(int32 SlotIndex);

	/** Collect JSON data from all subsystems */
	static void CollectSubsystemData(const UObject* WorldContext, UTitanSaveGame* SaveGame);

	/** Restore JSON data to all subsystems */
	static void RestoreSubsystemData(const UObject* WorldContext, UTitanSaveGame* SaveGame);

	/** Session start time for playtime tracking */
	static FDateTime SessionStartTime;

	/** Runtime statistics (saved on save) */
	static TMap<FString, int32> RuntimeStatistics;

	/** Current autosave rotation index */
	static int32 CurrentAutosaveIndex;
};
