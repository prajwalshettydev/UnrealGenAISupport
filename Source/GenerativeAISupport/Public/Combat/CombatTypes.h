// Combat System Types - Data structures for combat calculations
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

/**
 * Damage type
 */
UENUM(BlueprintType)
enum class EDamageType : uint8
{
	Physical UMETA(DisplayName = "Physisch"),
	Fire UMETA(DisplayName = "Feuer"),
	Ice UMETA(DisplayName = "Eis"),
	Lightning UMETA(DisplayName = "Blitz"),
	Poison UMETA(DisplayName = "Gift"),
	Holy UMETA(DisplayName = "Heilig"),
	Shadow UMETA(DisplayName = "Schatten"),
	Arcane UMETA(DisplayName = "Arkan"),
	Nature UMETA(DisplayName = "Natur"),
	Pure UMETA(DisplayName = "Reiner Schaden") // Ignores all defense
};

/**
 * Attack type
 */
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	Melee UMETA(DisplayName = "Nahkampf"),
	Ranged UMETA(DisplayName = "Fernkampf"),
	Spell UMETA(DisplayName = "Zauber"),
	DOT UMETA(DisplayName = "Schaden über Zeit"),
	AOE UMETA(DisplayName = "Flächenschaden"),
	Reflect UMETA(DisplayName = "Reflektion"),
	Environmental UMETA(DisplayName = "Umgebung")
};

/**
 * Combat result type
 */
UENUM(BlueprintType)
enum class ECombatResult : uint8
{
	Hit UMETA(DisplayName = "Treffer"),
	CriticalHit UMETA(DisplayName = "Kritischer Treffer"),
	Miss UMETA(DisplayName = "Verfehlt"),
	Dodge UMETA(DisplayName = "Ausgewichen"),
	Parry UMETA(DisplayName = "Pariert"),
	Block UMETA(DisplayName = "Geblockt"),
	Immune UMETA(DisplayName = "Immun"),
	Absorb UMETA(DisplayName = "Absorbiert"),
	Resist UMETA(DisplayName = "Widerstanden")
};

/**
 * Status effect type
 */
UENUM(BlueprintType)
enum class EStatusEffectType : uint8
{
	// Damage over time
	Burning UMETA(DisplayName = "Brennend"),
	Bleeding UMETA(DisplayName = "Blutend"),
	Poisoned UMETA(DisplayName = "Vergiftet"),
	Frozen UMETA(DisplayName = "Gefroren"),
	Shocked UMETA(DisplayName = "Schockiert"),

	// Crowd control
	Stunned UMETA(DisplayName = "Betäubt"),
	Silenced UMETA(DisplayName = "Verstummt"),
	Rooted UMETA(DisplayName = "Verwurzelt"),
	Slowed UMETA(DisplayName = "Verlangsamt"),
	Feared UMETA(DisplayName = "Verängstigt"),
	Charmed UMETA(DisplayName = "Bezaubert"),
	Confused UMETA(DisplayName = "Verwirrt"),
	Blinded UMETA(DisplayName = "Geblendet"),
	Sleeping UMETA(DisplayName = "Schlafend"),

	// Debuffs
	Weakened UMETA(DisplayName = "Geschwächt"),
	Cursed UMETA(DisplayName = "Verflucht"),
	ArmorBroken UMETA(DisplayName = "Rüstung gebrochen"),
	Exposed UMETA(DisplayName = "Entblößt"),

	// Buffs
	Regenerating UMETA(DisplayName = "Regenerierend"),
	Shielded UMETA(DisplayName = "Geschützt"),
	Empowered UMETA(DisplayName = "Ermächtigt"),
	Hasted UMETA(DisplayName = "Beschleunigt"),
	Invulnerable UMETA(DisplayName = "Unverwundbar"),
	Invisible UMETA(DisplayName = "Unsichtbar")
};

/**
 * Damage resistance data
 */
USTRUCT(BlueprintType)
struct FDamageResistances
{
	GENERATED_BODY()

	/** Physical resistance (flat reduction) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float PhysicalResistance = 0.0f;

	/** Fire resistance (percent reduction) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float FireResistance = 0.0f;

	/** Ice resistance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float IceResistance = 0.0f;

	/** Lightning resistance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float LightningResistance = 0.0f;

	/** Poison resistance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float PoisonResistance = 0.0f;

	/** Holy resistance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float HolyResistance = 0.0f;

	/** Shadow resistance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ShadowResistance = 0.0f;

	/** Arcane resistance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ArcaneResistance = 0.0f;

	/** Nature resistance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float NatureResistance = 0.0f;

	/** Get resistance for type */
	float GetResistance(EDamageType Type) const
	{
		switch (Type)
		{
		case EDamageType::Physical: return PhysicalResistance;
		case EDamageType::Fire: return FireResistance;
		case EDamageType::Ice: return IceResistance;
		case EDamageType::Lightning: return LightningResistance;
		case EDamageType::Poison: return PoisonResistance;
		case EDamageType::Holy: return HolyResistance;
		case EDamageType::Shadow: return ShadowResistance;
		case EDamageType::Arcane: return ArcaneResistance;
		case EDamageType::Nature: return NatureResistance;
		case EDamageType::Pure: return 0.0f; // Pure damage ignores resistance
		default: return 0.0f;
		}
	}
};

/**
 * Combat stats for an entity
 */
USTRUCT(BlueprintType)
struct FCombatStats
{
	GENERATED_BODY()

	// Base stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Strength = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Dexterity = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Intelligence = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Wisdom = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Constitution = 10.0f;

	// Offensive stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackPower = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SpellPower = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float CastSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float CriticalChance = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float CriticalDamage = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ArmorPenetration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MagicPenetration = 0.0f;

	// Defensive stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Armor = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MagicDefense = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DodgeChance = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ParryChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BlockChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BlockAmount = 0.0f;

	// Level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 Level = 1;

	// Resistances
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FDamageResistances Resistances;
};

/**
 * Damage event input
 */
USTRUCT(BlueprintType)
struct FDamageInput
{
	GENERATED_BODY()

	/** Base damage amount */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseDamage = 0.0f;

	/** Damage type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	EDamageType DamageType = EDamageType::Physical;

	/** Attack type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	EAttackType AttackType = EAttackType::Melee;

	/** Attacker stats */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FCombatStats AttackerStats;

	/** Can this attack crit? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bCanCrit = true;

	/** Can this attack be dodged? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bCanDodge = true;

	/** Can this attack be blocked? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bCanBlock = true;

	/** Bonus damage multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BonusDamageMultiplier = 1.0f;

	/** Weapon damage range (min) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float WeaponDamageMin = 0.0f;

	/** Weapon damage range (max) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float WeaponDamageMax = 0.0f;

	/** Skill coefficient (how much of attack power applies) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SkillCoefficient = 1.0f;

	/** Status effects to apply on hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<EStatusEffectType> AppliedEffects;
};

/**
 * Damage calculation result
 */
USTRUCT(BlueprintType)
struct FDamageResult
{
	GENERATED_BODY()

	/** Combat result type */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	ECombatResult Result = ECombatResult::Hit;

	/** Final damage dealt */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float FinalDamage = 0.0f;

	/** Raw damage before mitigation */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float RawDamage = 0.0f;

	/** Damage absorbed by shields */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float AbsorbedDamage = 0.0f;

	/** Damage blocked */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float BlockedDamage = 0.0f;

	/** Damage resisted */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float ResistedDamage = 0.0f;

	/** Was this a critical hit? */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bCritical = false;

	/** Critical multiplier used */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float CritMultiplier = 1.0f;

	/** Overkill damage (damage beyond lethal) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float OverkillDamage = 0.0f;

	/** Is target killed? */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bKillingBlow = false;

	/** Status effects applied */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TArray<EStatusEffectType> AppliedEffects;

	/** Damage breakdown per type */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TMap<EDamageType, float> DamageByType;
};

/**
 * Status effect data
 */
USTRUCT(BlueprintType)
struct FStatusEffect
{
	GENERATED_BODY()

	/** Effect type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	EStatusEffectType EffectType = EStatusEffectType::Burning;

	/** Effect source (who applied it) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FString SourceID;

	/** Duration in seconds (-1 = permanent) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Duration = 5.0f;

	/** Remaining duration */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float RemainingDuration = 5.0f;

	/** Effect strength/value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Value = 10.0f;

	/** Tick interval for DOT effects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float TickInterval = 1.0f;

	/** Time until next tick */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float NextTickTime = 1.0f;

	/** Can be dispelled? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bDispellable = true;

	/** Stack count */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 Stacks = 1;

	/** Max stacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 MaxStacks = 1;

	/** Icon path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FString IconPath;
};

/**
 * Healing event input
 */
USTRUCT(BlueprintType)
struct FHealingInput
{
	GENERATED_BODY()

	/** Base heal amount */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseHealing = 0.0f;

	/** Healer stats */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FCombatStats HealerStats;

	/** Spell power coefficient */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SpellCoefficient = 1.0f;

	/** Can this heal crit? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bCanCrit = true;

	/** Bonus healing multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BonusHealingMultiplier = 1.0f;
};

/**
 * Healing result
 */
USTRUCT(BlueprintType)
struct FHealingResult
{
	GENERATED_BODY()

	/** Final healing done */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float FinalHealing = 0.0f;

	/** Raw healing before modifiers */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float RawHealing = 0.0f;

	/** Overhealing (healing beyond max HP) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float OverHealing = 0.0f;

	/** Was this a critical heal? */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bCritical = false;

	/** Critical multiplier */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float CritMultiplier = 1.0f;
};

/**
 * Combat log entry
 */
USTRUCT(BlueprintType)
struct FCombatLogEntry
{
	GENERATED_BODY()

	/** Timestamp */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FDateTime Timestamp;

	/** Source entity ID */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FString SourceID;

	/** Source name */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FString SourceName;

	/** Target entity ID */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FString TargetID;

	/** Target name */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FString TargetName;

	/** Ability/action used */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FString AbilityName;

	/** Damage result (if damage) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FDamageResult DamageResult;

	/** Healing result (if healing) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FHealingResult HealingResult;

	/** Is this a damage event? */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsDamage = true;
};

/**
 * Threat data
 */
USTRUCT(BlueprintType)
struct FThreatEntry
{
	GENERATED_BODY()

	/** Player/entity ID */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FString EntityID;

	/** Entity name */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FString EntityName;

	/** Current threat value */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float ThreatValue = 0.0f;

	/** Threat percent (of total) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float ThreatPercent = 0.0f;
};

/**
 * Aggro table for NPCs
 */
USTRUCT(BlueprintType)
struct FAggroTable
{
	GENERATED_BODY()

	/** NPC entity ID */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FString OwnerID;

	/** All threat entries */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TArray<FThreatEntry> ThreatList;

	/** Current target (highest threat) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FString CurrentTargetID;

	/** Is in combat? */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bInCombat = false;

	/** Combat start time */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FDateTime CombatStartTime;
};
