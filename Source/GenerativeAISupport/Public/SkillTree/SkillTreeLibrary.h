// Skill Tree Library - Blueprint functions for skill/talent system
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SkillTreeTypes.h"
#include "SkillTreeLibrary.generated.h"

// Delegates
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSkillUnlocked, const FSkillUnlockResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillPointsChanged, int32, OldPoints, int32, NewPoints);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillTreeReset, const FSkillRespecResult&, Result);

/**
 * Blueprint Function Library for Skill Tree System
 */
UCLASS()
class GENERATIVEAISUPPORT_API USkillTreeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// SKILL TREE REGISTRY
	// ============================================

	/**
	 * Load skill trees from JSON file
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Registry")
	static int32 LoadSkillTreesFromJSON(const FString& FilePath);

	/**
	 * Register a single skill tree
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Registry")
	static void RegisterSkillTree(const FSkillTree& Tree);

	/**
	 * Get skill tree by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Registry")
	static bool GetSkillTree(const FString& TreeID, FSkillTree& OutTree);

	/**
	 * Get all skill trees
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Registry")
	static TArray<FSkillTree> GetAllSkillTrees();

	/**
	 * Get skill trees by category
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Registry")
	static TArray<FSkillTree> GetSkillTreesByCategory(ESkillTreeCategory Category);

	/**
	 * Get skill trees available for class
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Registry")
	static TArray<FSkillTree> GetSkillTreesForClass(const FString& ClassName, int32 PlayerLevel);

	/**
	 * Get skill node by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Registry")
	static bool GetSkillNode(const FString& SkillID, FSkillNode& OutSkill);

	// ============================================
	// SKILL UNLOCK & MANAGEMENT
	// ============================================

	/**
	 * Check if player can unlock a skill
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Unlock")
	static bool CanUnlockSkill(
		const FString& SkillID,
		const FPlayerSkillData& PlayerSkills,
		int32 PlayerLevel,
		const TMap<EAttributeType, float>& PlayerAttributes,
		FText& OutReason);

	/**
	 * Unlock/upgrade a skill
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Unlock")
	static FSkillUnlockResult UnlockSkill(
		const FString& SkillID,
		UPARAM(ref) FPlayerSkillData& PlayerSkills,
		int32 PlayerLevel,
		const TMap<EAttributeType, float>& PlayerAttributes);

	/**
	 * Get skill unlock status
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|Unlock")
	static ESkillUnlockStatus GetSkillStatus(
		const FString& SkillID,
		const FPlayerSkillData& PlayerSkills,
		int32 PlayerLevel,
		const TMap<EAttributeType, float>& PlayerAttributes);

	/**
	 * Get current rank in skill
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|Unlock")
	static int32 GetSkillRank(const FString& SkillID, const FPlayerSkillData& PlayerSkills);

	/**
	 * Check if skill is at max rank
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|Unlock")
	static bool IsSkillMaxed(const FString& SkillID, const FPlayerSkillData& PlayerSkills);

	// ============================================
	// SKILL POINTS
	// ============================================

	/**
	 * Add skill points (from leveling, quests, etc.)
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Points")
	static void AddSkillPoints(UPARAM(ref) FPlayerSkillData& PlayerSkills, int32 Points);

	/**
	 * Get available skill points
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|Points")
	static int32 GetAvailableSkillPoints(const FPlayerSkillData& PlayerSkills);

	/**
	 * Get total points spent
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|Points")
	static int32 GetTotalPointsSpent(const FPlayerSkillData& PlayerSkills);

	/**
	 * Get points spent in tree
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|Points")
	static int32 GetPointsSpentInTree(const FString& TreeID, const FPlayerSkillData& PlayerSkills);

	/**
	 * Get skill points for level
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|Points")
	static int32 GetSkillPointsForLevel(int32 Level);

	// ============================================
	// RESPEC & RESET
	// ============================================

	/**
	 * Respec all skills (refund all points)
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Respec")
	static FSkillRespecResult RespecAllSkills(
		UPARAM(ref) FPlayerSkillData& PlayerSkills,
		UPARAM(ref) int32& PlayerGold,
		int32 RespecCost = 100);

	/**
	 * Respec single tree
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Respec")
	static FSkillRespecResult RespecTree(
		const FString& TreeID,
		UPARAM(ref) FPlayerSkillData& PlayerSkills,
		UPARAM(ref) int32& PlayerGold,
		int32 RespecCost = 50);

	/**
	 * Calculate respec cost based on level/points
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|Respec")
	static int32 CalculateRespecCost(const FPlayerSkillData& PlayerSkills, int32 PlayerLevel);

	// ============================================
	// ATTRIBUTE BONUSES
	// ============================================

	/**
	 * Calculate all attribute bonuses from skills
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Attributes")
	static FSkillAttributeBonuses CalculateAttributeBonuses(const FPlayerSkillData& PlayerSkills);

	/**
	 * Get total bonus for specific attribute
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|Attributes")
	static float GetAttributeBonus(
		EAttributeType Attribute,
		const FPlayerSkillData& PlayerSkills,
		float BaseValue);

	/**
	 * Apply skill bonuses to base attributes
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Attributes")
	static TMap<EAttributeType, float> ApplySkillBonuses(
		const TMap<EAttributeType, float>& BaseAttributes,
		const FPlayerSkillData& PlayerSkills);

	// ============================================
	// ACTIVE SKILLS
	// ============================================

	/**
	 * Get all unlocked active skills
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Active")
	static TArray<FSkillNode> GetUnlockedActiveSkills(const FPlayerSkillData& PlayerSkills);

	/**
	 * Get skill effect values at current rank
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|Active")
	static FSkillEffect GetSkillEffectAtRank(const FString& SkillID, int32 Rank);

	/**
	 * Check if active skill is usable (has resources, not on cooldown)
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|Active")
	static bool CanUseActiveSkill(
		const FString& SkillID,
		int32 Rank,
		float CurrentMana,
		float CurrentStamina,
		float CurrentCooldown);

	// ============================================
	// TREE ANALYSIS
	// ============================================

	/**
	 * Get all skills connected to a skill
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Analysis")
	static TArray<FSkillNode> GetConnectedSkills(const FString& SkillID);

	/**
	 * Get path to unlock a skill
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Analysis")
	static TArray<FString> GetUnlockPath(
		const FString& SkillID,
		const FPlayerSkillData& PlayerSkills);

	/**
	 * Get total points needed to reach skill
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|Analysis")
	static int32 GetPointsToReachSkill(
		const FString& SkillID,
		const FPlayerSkillData& PlayerSkills);

	/**
	 * Get skills by tag
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Analysis")
	static TArray<FSkillNode> GetSkillsByTag(const FString& Tag);

	// ============================================
	// UI HELPERS
	// ============================================

	/**
	 * Get category display name
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|UI")
	static FText GetCategoryDisplayName(ESkillTreeCategory Category);

	/**
	 * Get node type display name
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|UI")
	static FText GetNodeTypeDisplayName(ESkillNodeType NodeType);

	/**
	 * Get attribute display name
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|UI")
	static FText GetAttributeDisplayName(EAttributeType Attribute);

	/**
	 * Get skill description with current rank values
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|UI")
	static FText GetSkillDescriptionWithValues(const FString& SkillID, int32 CurrentRank);

	/**
	 * Get color for skill status
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|UI")
	static FLinearColor GetStatusColor(ESkillUnlockStatus Status);

	/**
	 * Get color for node type
	 */
	UFUNCTION(BlueprintPure, Category = "SkillTree|UI")
	static FLinearColor GetNodeTypeColor(ESkillNodeType NodeType);

	// ============================================
	// SAVE/LOAD
	// ============================================

	/**
	 * Save skill progress to JSON
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Save")
	static FString SaveSkillProgressToJSON(const FPlayerSkillData& PlayerSkills);

	/**
	 * Load skill progress from JSON
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Save")
	static bool LoadSkillProgressFromJSON(const FString& JSONString, FPlayerSkillData& OutPlayerSkills);

	// ============================================
	// PRESETS & BUILDS
	// ============================================

	/**
	 * Get recommended skills for class
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Presets")
	static TArray<FString> GetRecommendedSkills(const FString& ClassName, int32 PlayerLevel);

	/**
	 * Apply skill preset/build
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Presets")
	static bool ApplySkillPreset(
		const TArray<FString>& SkillIDsToUnlock,
		UPARAM(ref) FPlayerSkillData& PlayerSkills,
		int32 PlayerLevel,
		const TMap<EAttributeType, float>& PlayerAttributes);

private:
	/** Skill tree registry */
	static TMap<FString, FSkillTree> TreeRegistry;

	/** Flat skill node lookup */
	static TMap<FString, FSkillNode> SkillRegistry;

	/** Skill to tree mapping */
	static TMap<FString, FString> SkillToTreeMap;

	/** Build skill lookup tables after loading */
	static void RebuildSkillLookup();
};
