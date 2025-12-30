// Skill Tree Types - Data structures for character skill progression
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "SkillTreeTypes.generated.h"

/**
 * Skill category/tree type
 */
UENUM(BlueprintType)
enum class ESkillTreeCategory : uint8
{
	Combat UMETA(DisplayName = "Kampf"),
	Magic UMETA(DisplayName = "Magie"),
	Stealth UMETA(DisplayName = "Schleichen"),
	Crafting UMETA(DisplayName = "Handwerk"),
	Survival UMETA(DisplayName = "Überleben"),
	Social UMETA(DisplayName = "Sozial"),
	Leadership UMETA(DisplayName = "Führung")
};

/**
 * Skill node type
 */
UENUM(BlueprintType)
enum class ESkillNodeType : uint8
{
	Passive UMETA(DisplayName = "Passiv"),
	Active UMETA(DisplayName = "Aktiv"),
	Ultimate UMETA(DisplayName = "Ultimativ"),
	Mastery UMETA(DisplayName = "Meisterschaft"),
	Perk UMETA(DisplayName = "Perk")
};

/**
 * Attribute type that can be modified
 */
UENUM(BlueprintType)
enum class EAttributeType : uint8
{
	Health UMETA(DisplayName = "Leben"),
	MaxHealth UMETA(DisplayName = "Max Leben"),
	Mana UMETA(DisplayName = "Mana"),
	MaxMana UMETA(DisplayName = "Max Mana"),
	Stamina UMETA(DisplayName = "Ausdauer"),
	MaxStamina UMETA(DisplayName = "Max Ausdauer"),
	Strength UMETA(DisplayName = "Stärke"),
	Dexterity UMETA(DisplayName = "Geschick"),
	Intelligence UMETA(DisplayName = "Intelligenz"),
	Wisdom UMETA(DisplayName = "Weisheit"),
	Constitution UMETA(DisplayName = "Konstitution"),
	Charisma UMETA(DisplayName = "Charisma"),
	AttackPower UMETA(DisplayName = "Angriffskraft"),
	SpellPower UMETA(DisplayName = "Zauberkraft"),
	PhysicalDefense UMETA(DisplayName = "Physische Verteidigung"),
	MagicDefense UMETA(DisplayName = "Magische Verteidigung"),
	CriticalChance UMETA(DisplayName = "Kritische Chance"),
	CriticalDamage UMETA(DisplayName = "Kritischer Schaden"),
	AttackSpeed UMETA(DisplayName = "Angriffsgeschwindigkeit"),
	CastSpeed UMETA(DisplayName = "Zaubergeschwindigkeit"),
	MovementSpeed UMETA(DisplayName = "Bewegungsgeschwindigkeit"),
	DodgeChance UMETA(DisplayName = "Ausweichchance"),
	BlockChance UMETA(DisplayName = "Blockchance"),
	HealthRegen UMETA(DisplayName = "Leben-Regeneration"),
	ManaRegen UMETA(DisplayName = "Mana-Regeneration"),
	StaminaRegen UMETA(DisplayName = "Ausdauer-Regeneration"),
	ExperienceGain UMETA(DisplayName = "Erfahrungsgewinn"),
	GoldFind UMETA(DisplayName = "Goldfund"),
	ItemFind UMETA(DisplayName = "Gegenstandsfund")
};

/**
 * Modifier type for attribute changes
 */
UENUM(BlueprintType)
enum class EModifierType : uint8
{
	Flat UMETA(DisplayName = "Flach"),
	Percent UMETA(DisplayName = "Prozent"),
	Multiplicative UMETA(DisplayName = "Multiplikativ")
};

/**
 * Skill unlock status
 */
UENUM(BlueprintType)
enum class ESkillUnlockStatus : uint8
{
	Locked UMETA(DisplayName = "Gesperrt"),
	Available UMETA(DisplayName = "Verfügbar"),
	Unlocked UMETA(DisplayName = "Freigeschaltet"),
	Maxed UMETA(DisplayName = "Maximiert")
};

/**
 * Single attribute modifier
 */
USTRUCT(BlueprintType)
struct FAttributeModifier
{
	GENERATED_BODY()

	/** Attribute to modify */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	EAttributeType Attribute = EAttributeType::Strength;

	/** Value of modifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	float Value = 0.0f;

	/** Type of modifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	EModifierType ModifierType = EModifierType::Flat;

	/** Modifier per skill rank (stacks) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	bool bPerRank = true;
};

/**
 * Skill unlock requirement
 */
USTRUCT(BlueprintType)
struct FSkillRequirement
{
	GENERATED_BODY()

	/** Required skill ID (empty = no skill required) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FString RequiredSkillID;

	/** Required rank in that skill */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	int32 RequiredRank = 1;

	/** Required player level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	int32 RequiredLevel = 1;

	/** Required attribute value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	EAttributeType RequiredAttribute = EAttributeType::Strength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	int32 RequiredAttributeValue = 0;
};

/**
 * Position in skill tree for UI
 */
USTRUCT(BlueprintType)
struct FSkillTreePosition
{
	GENERATED_BODY()

	/** Row in tree (0 = top/root) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	int32 Row = 0;

	/** Column in tree */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	int32 Column = 0;
};

/**
 * Active skill effect
 */
USTRUCT(BlueprintType)
struct FSkillEffect
{
	GENERATED_BODY()

	/** Effect ID for gameplay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FString EffectID;

	/** Effect display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FText EffectName;

	/** Base damage/healing value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	float BaseValue = 0.0f;

	/** Scaling per rank */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	float ValuePerRank = 0.0f;

	/** Duration in seconds (0 = instant) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	float Duration = 0.0f;

	/** Cooldown in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	float Cooldown = 0.0f;

	/** Mana/Stamina cost */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	float ResourceCost = 0.0f;

	/** Range in units (0 = self/melee) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	float Range = 0.0f;

	/** Area of effect radius (0 = single target) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	float AoERadius = 0.0f;
};

/**
 * Single skill node in a tree
 */
USTRUCT(BlueprintType)
struct FSkillNode
{
	GENERATED_BODY()

	/** Unique skill ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FString SkillID;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FText DisplayName;

	/** Description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FText Description;

	/** Node type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	ESkillNodeType NodeType = ESkillNodeType::Passive;

	/** Category this skill belongs to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	ESkillTreeCategory Category = ESkillTreeCategory::Combat;

	/** Maximum ranks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	int32 MaxRanks = 1;

	/** Skill point cost per rank */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	int32 PointCost = 1;

	/** Position in tree UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FSkillTreePosition Position;

	/** Requirements to unlock */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	TArray<FSkillRequirement> Requirements;

	/** Connected skill IDs (for UI lines) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	TArray<FString> ConnectedSkills;

	/** Attribute modifiers this skill provides */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	TArray<FAttributeModifier> Modifiers;

	/** Active skill effects (for Active/Ultimate skills) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FSkillEffect ActiveEffect;

	/** Icon path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FString IconPath;

	/** Tags for filtering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	TArray<FString> Tags;
};

/**
 * Player's progress in a single skill
 */
USTRUCT(BlueprintType)
struct FSkillProgress
{
	GENERATED_BODY()

	/** Skill ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FString SkillID;

	/** Current rank */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	int32 CurrentRank = 0;

	/** When the skill was unlocked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FDateTime UnlockTime;
};

/**
 * Complete skill tree definition
 */
USTRUCT(BlueprintType)
struct FSkillTree
{
	GENERATED_BODY()

	/** Tree ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FString TreeID;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FText DisplayName;

	/** Description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FText Description;

	/** Tree category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	ESkillTreeCategory Category = ESkillTreeCategory::Combat;

	/** Icon path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FString IconPath;

	/** All skills in this tree */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	TArray<FSkillNode> Skills;

	/** Required level to access tree */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	int32 RequiredLevel = 1;

	/** Required class (empty = all) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	FString RequiredClass;
};

/**
 * Player's complete skill data
 */
USTRUCT(BlueprintType)
struct FPlayerSkillData
{
	GENERATED_BODY()

	/** Available skill points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	int32 AvailablePoints = 0;

	/** Total skill points ever earned */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	int32 TotalPointsEarned = 0;

	/** Progress in each skill */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
	TArray<FSkillProgress> SkillProgress;

	/** Check if skill is unlocked */
	bool IsSkillUnlocked(const FString& SkillID) const
	{
		for (const FSkillProgress& Progress : SkillProgress)
		{
			if (Progress.SkillID == SkillID && Progress.CurrentRank > 0)
			{
				return true;
			}
		}
		return false;
	}

	/** Get current rank in skill */
	int32 GetSkillRank(const FString& SkillID) const
	{
		for (const FSkillProgress& Progress : SkillProgress)
		{
			if (Progress.SkillID == SkillID)
			{
				return Progress.CurrentRank;
			}
		}
		return 0;
	}
};

/**
 * Calculated attribute bonuses from skills
 */
USTRUCT(BlueprintType)
struct FSkillAttributeBonuses
{
	GENERATED_BODY()

	/** Flat bonuses per attribute */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	TMap<EAttributeType, float> FlatBonuses;

	/** Percent bonuses per attribute */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	TMap<EAttributeType, float> PercentBonuses;

	/** Multiplicative bonuses per attribute */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	TMap<EAttributeType, float> MultiplierBonuses;
};

/**
 * Skill unlock result
 */
USTRUCT(BlueprintType)
struct FSkillUnlockResult
{
	GENERATED_BODY()

	/** Was unlock successful */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	bool bSuccess = false;

	/** New rank in skill */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	int32 NewRank = 0;

	/** Remaining skill points */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	int32 RemainingPoints = 0;

	/** Error message if failed */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	FText ErrorMessage;

	/** Modifiers gained */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	TArray<FAttributeModifier> GainedModifiers;
};

/**
 * Skill respec result
 */
USTRUCT(BlueprintType)
struct FSkillRespecResult
{
	GENERATED_BODY()

	/** Was respec successful */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	bool bSuccess = false;

	/** Points refunded */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	int32 PointsRefunded = 0;

	/** Gold cost */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	int32 GoldCost = 0;

	/** Skills that were reset */
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	TArray<FString> ResetSkillIDs;
};
