// Achievement Library - Blueprint callable functions for achievements
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Achievement/AchievementTypes.h"
#include "AchievementLibrary.generated.h"

// Delegate for achievement unlocked
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnAchievementUnlocked, const FString&, AchievementID, const FAchievementDefinition&, Achievement);

/**
 * Blueprint Function Library for Achievement System
 */
UCLASS()
class GENERATIVEAISUPPORT_API UAchievementLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// ACHIEVEMENT REGISTRY
	// ============================================

	/**
	 * Register an achievement
	 * @param Achievement - The achievement to register
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static void RegisterAchievement(const FAchievementDefinition& Achievement);

	/**
	 * Register achievement from data asset
	 * @param Asset - The achievement data asset
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static void RegisterAchievementAsset(UAchievementDataAsset* Asset);

	/**
	 * Load achievements from JSON file
	 * @param FilePath - Path to JSON file
	 * @return Number of achievements loaded
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static int32 LoadAchievementsFromJSON(const FString& FilePath);

	/**
	 * Get an achievement definition by ID
	 * @param AchievementID - The achievement ID
	 * @param OutAchievement - The achievement definition
	 * @return True if found
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static bool GetAchievement(const FString& AchievementID, FAchievementDefinition& OutAchievement);

	/**
	 * Get all achievements in a category
	 * @param Category - The category to filter by
	 * @return Array of matching achievements
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static TArray<FAchievementDefinition> GetAchievementsByCategory(EAchievementCategory Category);

	// ============================================
	// PROGRESS TRACKING
	// ============================================

	/**
	 * Report progress on an achievement stat
	 * @param StatName - The stat being tracked
	 * @param Amount - Amount to add (or set if bSetValue is true)
	 * @param bSetValue - If true, sets the value; if false, adds to current
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static void ReportProgress(const FString& StatName, int32 Amount, bool bSetValue = false);

	/**
	 * Report a flag/boolean achievement trigger
	 * @param FlagName - The flag that was triggered
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static void ReportFlag(const FString& FlagName);

	/**
	 * Report discovery of a location/secret
	 * @param DiscoveryID - The discovery identifier
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static void ReportDiscovery(const FString& DiscoveryID);

	/**
	 * Get progress for an achievement
	 * @param AchievementID - The achievement ID
	 * @param OutProgress - The progress data
	 * @return True if found
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static bool GetAchievementProgress(const FString& AchievementID, FAchievementProgress& OutProgress);

	/**
	 * Check if an achievement is unlocked
	 * @param AchievementID - The achievement ID
	 * @return True if unlocked
	 */
	UFUNCTION(BlueprintPure, Category = "Achievement System")
	static bool IsAchievementUnlocked(const FString& AchievementID);

	/**
	 * Get progress percentage for an achievement
	 * @param AchievementID - The achievement ID
	 * @return Progress percentage (0.0 - 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Achievement System")
	static float GetAchievementProgressPercent(const FString& AchievementID);

	// ============================================
	// REWARD SYSTEM
	// ============================================

	/**
	 * Claim reward for an achievement
	 * @param AchievementID - The achievement ID
	 * @param OutReward - The reward received
	 * @return True if reward was claimed
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static bool ClaimReward(const FString& AchievementID, FAchievementReward& OutReward);

	/**
	 * Check if reward is available to claim
	 * @param AchievementID - The achievement ID
	 * @return True if can claim
	 */
	UFUNCTION(BlueprintPure, Category = "Achievement System")
	static bool CanClaimReward(const FString& AchievementID);

	/**
	 * Get total achievement points earned
	 * @return Total points
	 */
	UFUNCTION(BlueprintPure, Category = "Achievement System")
	static int32 GetTotalAchievementPoints();

	// ============================================
	// QUERIES
	// ============================================

	/**
	 * Get all unlocked achievements
	 * @return Array of unlocked achievement IDs
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static TArray<FAchievementProgress> GetUnlockedAchievements();

	/**
	 * Get all in-progress achievements
	 * @return Array of achievements with progress
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static TArray<FAchievementProgress> GetInProgressAchievements();

	/**
	 * Get recently unlocked achievements
	 * @param Count - Number to return
	 * @return Array of recently unlocked
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static TArray<FAchievementProgress> GetRecentlyUnlocked(int32 Count = 5);

	/**
	 * Get completion statistics
	 * @param OutTotal - Total achievements
	 * @param OutUnlocked - Unlocked count
	 * @param OutPercent - Completion percentage
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static void GetCompletionStats(int32& OutTotal, int32& OutUnlocked, float& OutPercent);

	// ============================================
	// SAVE/LOAD
	// ============================================

	/**
	 * Save achievement progress to JSON string
	 * @return JSON string of progress
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static FString SaveProgressToJSON();

	/**
	 * Load achievement progress from JSON string
	 * @param JsonString - The JSON to parse
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static bool LoadProgressFromJSON(const FString& JsonString);

	// ============================================
	// UTILITY
	// ============================================

	/**
	 * Get rarity color for display
	 * @param Rarity - The achievement rarity
	 * @return Color value
	 */
	UFUNCTION(BlueprintPure, Category = "Achievement System")
	static FLinearColor GetRarityColor(EAchievementRarity Rarity);

	/**
	 * Get category display name
	 * @param Category - The category
	 * @return Display name
	 */
	UFUNCTION(BlueprintPure, Category = "Achievement System")
	static FText GetCategoryDisplayName(EAchievementCategory Category);

	/**
	 * Reset all progress (for new game)
	 */
	UFUNCTION(BlueprintCallable, Category = "Achievement System")
	static void ResetAllProgress();

private:
	static TMap<FString, FAchievementDefinition>& GetRegistry();
	static TMap<FString, FAchievementProgress>& GetProgressMap();
	static TMap<FString, int32>& GetStatMap();
	static TSet<FString>& GetFlagSet();
	static TSet<FString>& GetDiscoverySet();

	static void CheckAchievementUnlock(const FString& AchievementID);
	static void UnlockAchievement(const FString& AchievementID);
	static bool ArePrerequisitesMet(const FAchievementDefinition& Achievement);
};
