// Crafting Library - Blueprint functions for crafting system
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CraftingTypes.h"
#include "CraftingLibrary.generated.h"

// Delegates
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnCraftingComplete, const FCraftingAttemptResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRecipeLearned, const FString&, RecipeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftingSkillUp, ECraftingCategory, Category, int32, NewLevel);

/**
 * Blueprint Function Library for Crafting System
 */
UCLASS()
class GENERATIVEAISUPPORT_API UCraftingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// RECIPE REGISTRY
	// ============================================

	/**
	 * Load recipes from JSON file
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Registry")
	static int32 LoadRecipesFromJSON(const FString& FilePath);

	/**
	 * Register a single recipe
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Registry")
	static void RegisterRecipe(const FCraftingRecipe& Recipe);

	/**
	 * Get recipe by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Registry")
	static bool GetRecipe(const FString& RecipeID, FCraftingRecipe& OutRecipe);

	/**
	 * Get all recipes
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Registry")
	static TArray<FCraftingRecipe> GetAllRecipes();

	/**
	 * Get recipes by category
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Registry")
	static TArray<FCraftingRecipe> GetRecipesByCategory(ECraftingCategory Category);

	/**
	 * Get recipes by station
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Registry")
	static TArray<FCraftingRecipe> GetRecipesByStation(ECraftingStation Station);

	/**
	 * Get recipes craftable at station with player skills
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Registry")
	static TArray<FCraftingRecipe> GetAvailableRecipes(
		ECraftingStation Station,
		const TArray<FCraftingSkill>& PlayerSkills,
		const FKnownRecipes& KnownRecipes,
		bool bIncludeHidden = false);

	/**
	 * Search recipes by name or tag
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Registry")
	static TArray<FCraftingRecipe> SearchRecipes(const FString& SearchTerm);

	// ============================================
	// CRAFTING OPERATIONS
	// ============================================

	/**
	 * Check if player can craft recipe
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Operations")
	static bool CanCraft(
		const FString& RecipeID,
		const TMap<FString, int32>& PlayerInventory,
		int32 PlayerGold,
		const TArray<FCraftingSkill>& PlayerSkills,
		const FKnownRecipes& KnownRecipes,
		ECraftingStation AvailableStation,
		FText& OutReason);

	/**
	 * Attempt to craft an item (instant)
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Operations")
	static FCraftingAttemptResult CraftItem(
		const FString& RecipeID,
		UPARAM(ref) TMap<FString, int32>& PlayerInventory,
		UPARAM(ref) int32& PlayerGold,
		UPARAM(ref) TArray<FCraftingSkill>& PlayerSkills,
		const FKnownRecipes& KnownRecipes,
		ECraftingStation AvailableStation,
		float LuckBonus = 0.0f);

	/**
	 * Craft multiple items
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Operations")
	static TArray<FCraftingAttemptResult> CraftMultiple(
		const FString& RecipeID,
		int32 Quantity,
		UPARAM(ref) TMap<FString, int32>& PlayerInventory,
		UPARAM(ref) int32& PlayerGold,
		UPARAM(ref) TArray<FCraftingSkill>& PlayerSkills,
		const FKnownRecipes& KnownRecipes,
		ECraftingStation AvailableStation,
		float LuckBonus = 0.0f);

	/**
	 * Get max craftable quantity
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Operations")
	static int32 GetMaxCraftableQuantity(
		const FString& RecipeID,
		const TMap<FString, int32>& PlayerInventory,
		int32 PlayerGold);

	/**
	 * Calculate success chance for recipe
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Operations")
	static float CalculateSuccessChance(
		const FCraftingRecipe& Recipe,
		const TArray<FCraftingSkill>& PlayerSkills,
		float StationBonus = 0.0f);

	/**
	 * Calculate critical chance for recipe
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Operations")
	static float CalculateCriticalChance(
		const FCraftingRecipe& Recipe,
		const TArray<FCraftingSkill>& PlayerSkills,
		float StationBonus = 0.0f,
		float LuckBonus = 0.0f);

	// ============================================
	// RECIPE LEARNING
	// ============================================

	/**
	 * Learn a recipe
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Learning")
	static bool LearnRecipe(const FString& RecipeID, UPARAM(ref) FKnownRecipes& KnownRecipes);

	/**
	 * Learn recipe from item (recipe scroll)
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Learning")
	static bool LearnRecipeFromItem(
		const FString& ItemID,
		UPARAM(ref) FKnownRecipes& KnownRecipes,
		UPARAM(ref) TMap<FString, int32>& PlayerInventory,
		FString& OutLearnedRecipeID);

	/**
	 * Check if recipe is known
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Learning")
	static bool IsRecipeKnown(const FString& RecipeID, const FKnownRecipes& KnownRecipes);

	/**
	 * Get recipes learnable from trainer
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Learning")
	static TArray<FCraftingRecipe> GetTrainerRecipes(
		const FString& TrainerID,
		const TArray<FCraftingSkill>& PlayerSkills,
		const FKnownRecipes& KnownRecipes);

	// ============================================
	// SKILL MANAGEMENT
	// ============================================

	/**
	 * Get skill level for category
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Skills")
	static int32 GetSkillLevel(ECraftingCategory Category, const TArray<FCraftingSkill>& Skills);

	/**
	 * Add XP to crafting skill
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Skills")
	static bool AddSkillXP(
		ECraftingCategory Category,
		int32 XPAmount,
		UPARAM(ref) TArray<FCraftingSkill>& Skills,
		int32& OutNewLevel,
		bool& OutLeveledUp);

	/**
	 * Get skill progress (0.0 - 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Skills")
	static float GetSkillProgress(ECraftingCategory Category, const TArray<FCraftingSkill>& Skills);

	/**
	 * Initialize default skills
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Skills")
	static TArray<FCraftingSkill> CreateDefaultSkills();

	/**
	 * Get XP required for level
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Skills")
	static int32 GetXPRequiredForLevel(int32 Level);

	// ============================================
	// MATERIAL REQUIREMENTS
	// ============================================

	/**
	 * Get required materials for recipe
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Materials")
	static TArray<FCraftingIngredient> GetRequiredMaterials(const FString& RecipeID);

	/**
	 * Check if player has materials
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Materials")
	static bool HasRequiredMaterials(
		const FString& RecipeID,
		const TMap<FString, int32>& PlayerInventory,
		TArray<FCraftingIngredient>& OutMissingMaterials);

	/**
	 * Get material sources (where to get materials)
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Materials")
	static TMap<FString, FString> GetMaterialSources(const FString& RecipeID);

	// ============================================
	// STATION MANAGEMENT
	// ============================================

	/**
	 * Register crafting station
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Stations")
	static void RegisterStation(const FCraftingStationInfo& StationInfo);

	/**
	 * Get station info
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Stations")
	static FCraftingStationInfo GetStationInfo(ECraftingStation Station);

	/**
	 * Get station display name
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Stations")
	static FText GetStationDisplayName(ECraftingStation Station);

	// ============================================
	// UTILITIES
	// ============================================

	/**
	 * Get category display name
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Utilities")
	static FText GetCategoryDisplayName(ECraftingCategory Category);

	/**
	 * Get difficulty display name
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Utilities")
	static FText GetDifficultyDisplayName(ECraftingDifficulty Difficulty);

	/**
	 * Get difficulty color
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Utilities")
	static FLinearColor GetDifficultyColor(ECraftingDifficulty Difficulty);

	/**
	 * Format crafting time
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting|Utilities")
	static FString FormatCraftingTime(float Seconds);

	// ============================================
	// SAVE/LOAD
	// ============================================

	/**
	 * Save crafting progress to JSON
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Save")
	static FString SaveCraftingProgressToJSON(
		const TArray<FCraftingSkill>& Skills,
		const FKnownRecipes& KnownRecipes);

	/**
	 * Load crafting progress from JSON
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting|Save")
	static bool LoadCraftingProgressFromJSON(
		const FString& JSONString,
		TArray<FCraftingSkill>& OutSkills,
		FKnownRecipes& OutKnownRecipes);

private:
	/** Recipe registry */
	static TMap<FString, FCraftingRecipe> RecipeRegistry;

	/** Station info registry */
	static TMap<ECraftingStation, FCraftingStationInfo> StationRegistry;

	/** Recipe to item mapping (for learning from scrolls) */
	static TMap<FString, FString> RecipeItemMapping;

	/** Initialize default stations */
	static void InitializeDefaultStations();
};
