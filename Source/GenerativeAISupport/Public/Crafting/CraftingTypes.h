// Crafting System Types - Data structures for crafting
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "CraftingTypes.generated.h"

/**
 * Crafting station type
 */
UENUM(BlueprintType)
enum class ECraftingStation : uint8
{
	None UMETA(DisplayName = "Anywhere"),
	Workbench UMETA(DisplayName = "Workbench"),
	Forge UMETA(DisplayName = "Forge"),
	Anvil UMETA(DisplayName = "Anvil"),
	Alchemy UMETA(DisplayName = "Alchemy Table"),
	Enchanting UMETA(DisplayName = "Enchanting Table"),
	Cooking UMETA(DisplayName = "Cooking Fire"),
	Tanning UMETA(DisplayName = "Tanning Rack"),
	Loom UMETA(DisplayName = "Loom"),
	Jeweler UMETA(DisplayName = "Jeweler's Bench")
};

/**
 * Crafting category
 */
UENUM(BlueprintType)
enum class ECraftingCategory : uint8
{
	Weapons UMETA(DisplayName = "Weapons"),
	Armor UMETA(DisplayName = "Armor"),
	Consumables UMETA(DisplayName = "Consumables"),
	Materials UMETA(DisplayName = "Materials"),
	Tools UMETA(DisplayName = "Tools"),
	Jewelry UMETA(DisplayName = "Jewelry"),
	Furniture UMETA(DisplayName = "Furniture"),
	Enchantments UMETA(DisplayName = "Enchantments"),
	Food UMETA(DisplayName = "Food"),
	Potions UMETA(DisplayName = "Potions")
};

/**
 * Crafting difficulty
 */
UENUM(BlueprintType)
enum class ECraftingDifficulty : uint8
{
	Trivial UMETA(DisplayName = "Trivial"),
	Easy UMETA(DisplayName = "Easy"),
	Normal UMETA(DisplayName = "Normal"),
	Hard UMETA(DisplayName = "Hard"),
	Expert UMETA(DisplayName = "Expert"),
	Master UMETA(DisplayName = "Master"),
	Legendary UMETA(DisplayName = "Legendary")
};

/**
 * Crafting result status
 */
UENUM(BlueprintType)
enum class ECraftingResult : uint8
{
	Success UMETA(DisplayName = "Success"),
	CriticalSuccess UMETA(DisplayName = "Critical Success"),
	Failure UMETA(DisplayName = "Failure"),
	CriticalFailure UMETA(DisplayName = "Critical Failure"),
	MissingMaterials UMETA(DisplayName = "Missing Materials"),
	MissingStation UMETA(DisplayName = "Missing Station"),
	InsufficientSkill UMETA(DisplayName = "Insufficient Skill"),
	RecipeUnknown UMETA(DisplayName = "Recipe Unknown")
};

/**
 * Single ingredient in a recipe
 */
USTRUCT(BlueprintType)
struct FCraftingIngredient
{
	GENERATED_BODY()

	/** Item ID required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FString ItemID;

	/** Quantity required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 Quantity = 1;

	/** Is this optional? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	bool bOptional = false;

	/** Bonus if included (for optional ingredients) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FString BonusEffect;
};

/**
 * Recipe output item
 */
USTRUCT(BlueprintType)
struct FCraftingOutput
{
	GENERATED_BODY()

	/** Item ID produced */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FString ItemID;

	/** Base quantity produced */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 Quantity = 1;

	/** Bonus quantity on critical success */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 CriticalBonusQuantity = 0;
};

/**
 * Complete crafting recipe
 */
USTRUCT(BlueprintType)
struct FCraftingRecipe
{
	GENERATED_BODY()

	/** Unique recipe ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FString RecipeID;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FText DisplayName;

	/** Description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FText Description;

	/** Category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	ECraftingCategory Category = ECraftingCategory::Materials;

	/** Required station */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	ECraftingStation RequiredStation = ECraftingStation::None;

	/** Difficulty level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	ECraftingDifficulty Difficulty = ECraftingDifficulty::Normal;

	/** Required crafting skill level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 RequiredSkillLevel = 1;

	/** Skill XP gained on craft */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 SkillXPGain = 10;

	/** Crafting time in seconds (0 = instant) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	float CraftingTime = 0.f;

	/** Required ingredients */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	TArray<FCraftingIngredient> Ingredients;

	/** Output items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	TArray<FCraftingOutput> Outputs;

	/** Gold cost to craft */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 GoldCost = 0;

	/** Is recipe hidden until discovered? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	bool bHidden = false;

	/** Required quest to unlock */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FString RequiredQuest;

	/** Required faction reputation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FString RequiredFaction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 RequiredReputation = 0;

	/** Icon path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FString IconPath;

	/** Tags for filtering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	TArray<FString> Tags;
};

/**
 * Result of a crafting attempt
 */
USTRUCT(BlueprintType)
struct FCraftingAttemptResult
{
	GENERATED_BODY()

	/** Result status */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	ECraftingResult Result = ECraftingResult::Failure;

	/** Items produced */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	TArray<FCraftingOutput> ProducedItems;

	/** XP gained */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	int32 XPGained = 0;

	/** Error message if failed */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	FText ErrorMessage;

	/** Was this a critical result? */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	bool bCritical = false;

	/** Recipe that was crafted */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	FString RecipeID;
};

/**
 * Player's crafting skill in a category
 */
USTRUCT(BlueprintType)
struct FCraftingSkill
{
	GENERATED_BODY()

	/** Skill category (matches ECraftingCategory) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	ECraftingCategory Category = ECraftingCategory::Materials;

	/** Current skill level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 Level = 1;

	/** Current XP */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 CurrentXP = 0;

	/** XP needed for next level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 XPToNextLevel = 100;

	/** Total items crafted in this category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 TotalCrafted = 0;
};

/**
 * Player's known recipes
 */
USTRUCT(BlueprintType)
struct FKnownRecipes
{
	GENERATED_BODY()

	/** List of known recipe IDs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	TArray<FString> RecipeIDs;

	/** Check if recipe is known */
	bool IsKnown(const FString& RecipeID) const
	{
		return RecipeIDs.Contains(RecipeID);
	}

	/** Add recipe */
	void LearnRecipe(const FString& RecipeID)
	{
		if (!RecipeIDs.Contains(RecipeID))
		{
			RecipeIDs.Add(RecipeID);
		}
	}
};

/**
 * Crafting queue entry
 */
USTRUCT(BlueprintType)
struct FCraftingQueueEntry
{
	GENERATED_BODY()

	/** Recipe being crafted */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	FString RecipeID;

	/** Quantity to craft */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	int32 Quantity = 1;

	/** Time remaining */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	float TimeRemaining = 0.f;

	/** Total time for this craft */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	float TotalTime = 0.f;

	/** Unique queue ID */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	int32 QueueID = 0;
};

/**
 * Crafting station actor info
 */
USTRUCT(BlueprintType)
struct FCraftingStationInfo
{
	GENERATED_BODY()

	/** Station type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	ECraftingStation StationType = ECraftingStation::Workbench;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FText DisplayName;

	/** Description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FText Description;

	/** Bonus to crafting speed (1.0 = normal) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	float SpeedMultiplier = 1.0f;

	/** Bonus to success chance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	float SuccessBonus = 0.0f;

	/** Bonus to critical chance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	float CriticalBonus = 0.0f;

	/** Categories this station can craft */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	TArray<ECraftingCategory> SupportedCategories;
};
