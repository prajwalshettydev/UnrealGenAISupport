// Combat Library - Blueprint functions for damage calculations
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatTypes.h"
#include "CombatLibrary.generated.h"

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageDealt, const FDamageResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealingDone, const FHealingResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatusEffectApplied, const FString&, TargetID, const FStatusEffect&, Effect);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatusEffectRemoved, const FString&, TargetID, EStatusEffectType, EffectType);

/**
 * Blueprint Function Library for Combat Calculations
 */
UCLASS()
class GENERATIVEAISUPPORT_API UCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// DAMAGE CALCULATION
	// ============================================

	/**
	 * Calculate damage from attack
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Damage")
	static FDamageResult CalculateDamage(
		const FDamageInput& Input,
		const FCombatStats& DefenderStats,
		float DefenderCurrentHP,
		float DefenderMaxHP);

	/**
	 * Calculate physical damage
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Damage")
	static float CalculatePhysicalDamage(
		float BaseDamage,
		float AttackPower,
		float TargetArmor,
		float ArmorPenetration,
		float SkillCoefficient = 1.0f);

	/**
	 * Calculate magical damage
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Damage")
	static float CalculateMagicalDamage(
		float BaseDamage,
		float SpellPower,
		float TargetMagicDefense,
		float MagicPenetration,
		float SpellCoefficient = 1.0f);

	/**
	 * Calculate armor mitigation
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Damage")
	static float CalculateArmorMitigation(float Armor, int32 AttackerLevel);

	/**
	 * Calculate resistance reduction
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Damage")
	static float CalculateResistanceReduction(float Resistance);

	/**
	 * Roll for combat result (hit/miss/dodge/etc)
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Damage")
	static ECombatResult RollCombatResult(
		const FDamageInput& Input,
		const FCombatStats& DefenderStats);

	/**
	 * Roll for critical hit
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Damage")
	static bool RollCritical(float CritChance);

	/**
	 * Calculate critical damage multiplier
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Damage")
	static float CalculateCritMultiplier(float BaseCritDamage, float BonusCritDamage = 0.0f);

	// ============================================
	// HEALING CALCULATION
	// ============================================

	/**
	 * Calculate healing
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Healing")
	static FHealingResult CalculateHealing(
		const FHealingInput& Input,
		float TargetCurrentHP,
		float TargetMaxHP,
		float HealingReceivedBonus = 0.0f);

	/**
	 * Calculate heal over time tick
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Healing")
	static float CalculateHOTTick(
		float TotalHealing,
		float Duration,
		float TickInterval);

	// ============================================
	// STATUS EFFECTS
	// ============================================

	/**
	 * Apply status effect
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Status")
	static bool ApplyStatusEffect(
		const FString& TargetID,
		const FStatusEffect& Effect);

	/**
	 * Remove status effect
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Status")
	static bool RemoveStatusEffect(
		const FString& TargetID,
		EStatusEffectType EffectType);

	/**
	 * Remove all status effects of category
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Status")
	static int32 RemoveAllEffectsOfType(
		const FString& TargetID,
		bool bRemoveDebuffs,
		bool bRemoveBuffs,
		bool bRemoveDOTs);

	/**
	 * Get active status effects
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Status")
	static TArray<FStatusEffect> GetActiveEffects(const FString& TargetID);

	/**
	 * Has specific status effect?
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Status")
	static bool HasStatusEffect(const FString& TargetID, EStatusEffectType EffectType);

	/**
	 * Update status effects (call every tick)
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Status")
	static TArray<FDamageResult> UpdateStatusEffects(const FString& TargetID, float DeltaTime);

	/**
	 * Calculate DOT damage tick
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Status")
	static float CalculateDOTTick(const FStatusEffect& Effect, const FCombatStats& SourceStats);

	/**
	 * Is effect a debuff?
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Status")
	static bool IsDebuff(EStatusEffectType EffectType);

	/**
	 * Is effect a DOT?
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Status")
	static bool IsDOT(EStatusEffectType EffectType);

	/**
	 * Is effect crowd control?
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Status")
	static bool IsCrowdControl(EStatusEffectType EffectType);

	// ============================================
	// THREAT & AGGRO
	// ============================================

	/**
	 * Add threat
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Threat")
	static void AddThreat(
		const FString& NPCID,
		const FString& PlayerID,
		const FString& PlayerName,
		float ThreatAmount);

	/**
	 * Reduce threat
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Threat")
	static void ReduceThreat(
		const FString& NPCID,
		const FString& PlayerID,
		float ReductionPercent);

	/**
	 * Get aggro table
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Threat")
	static FAggroTable GetAggroTable(const FString& NPCID);

	/**
	 * Get current target
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Threat")
	static FString GetCurrentTarget(const FString& NPCID);

	/**
	 * Clear aggro table
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Threat")
	static void ClearAggro(const FString& NPCID);

	/**
	 * Calculate threat from damage
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Threat")
	static float CalculateThreatFromDamage(float Damage, float ThreatModifier = 1.0f);

	/**
	 * Calculate threat from healing
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Threat")
	static float CalculateThreatFromHealing(float Healing, float ThreatModifier = 0.5f);

	// ============================================
	// STAT CALCULATIONS
	// ============================================

	/**
	 * Calculate attack power from strength
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	static float CalculateAttackPower(float Strength, int32 Level);

	/**
	 * Calculate spell power from intelligence
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	static float CalculateSpellPower(float Intelligence, int32 Level);

	/**
	 * Calculate max health from constitution
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	static float CalculateMaxHealth(float Constitution, int32 Level);

	/**
	 * Calculate max mana from wisdom
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	static float CalculateMaxMana(float Wisdom, int32 Level);

	/**
	 * Calculate crit chance from dexterity
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	static float CalculateCritChance(float Dexterity, int32 Level);

	/**
	 * Calculate dodge chance from dexterity
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	static float CalculateDodgeChance(float Dexterity, int32 Level);

	/**
	 * Get level difference modifier
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	static float GetLevelDifferenceModifier(int32 AttackerLevel, int32 DefenderLevel);

	// ============================================
	// COMBAT LOG
	// ============================================

	/**
	 * Add combat log entry
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Log")
	static void AddCombatLogEntry(
		const FString& SourceID,
		const FString& SourceName,
		const FString& TargetID,
		const FString& TargetName,
		const FString& AbilityName,
		const FDamageResult& DamageResult);

	/**
	 * Add healing log entry
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Log")
	static void AddHealingLogEntry(
		const FString& SourceID,
		const FString& SourceName,
		const FString& TargetID,
		const FString& TargetName,
		const FString& AbilityName,
		const FHealingResult& HealingResult);

	/**
	 * Get combat log
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Log")
	static TArray<FCombatLogEntry> GetCombatLog(int32 MaxEntries = 100);

	/**
	 * Clear combat log
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Log")
	static void ClearCombatLog();

	/**
	 * Get DPS summary from log
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Log")
	static TMap<FString, float> GetDPSSummary(float TimeWindowSeconds = 5.0f);

	/**
	 * Get HPS summary from log
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Log")
	static TMap<FString, float> GetHPSSummary(float TimeWindowSeconds = 5.0f);

	// ============================================
	// UTILITY FUNCTIONS
	// ============================================

	/**
	 * Get damage type display name
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Utility")
	static FText GetDamageTypeDisplayName(EDamageType Type);

	/**
	 * Get damage type color
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Utility")
	static FLinearColor GetDamageTypeColor(EDamageType Type);

	/**
	 * Get status effect display name
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Utility")
	static FText GetStatusEffectDisplayName(EStatusEffectType Type);

	/**
	 * Get combat result display name
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Utility")
	static FText GetCombatResultDisplayName(ECombatResult Result);

	/**
	 * Format damage number for display
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Utility")
	static FString FormatDamageNumber(float Damage, bool bCrit, EDamageType Type);

private:
	/** Status effects per entity */
	static TMap<FString, TArray<FStatusEffect>> EntityStatusEffects;

	/** Aggro tables per NPC */
	static TMap<FString, FAggroTable> NPCAggroTables;

	/** Combat log */
	static TArray<FCombatLogEntry> CombatLog;

	/** Max combat log entries */
	static constexpr int32 MaxLogEntries = 1000;
};
