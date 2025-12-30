// Combat Library Implementation
// Part of GenerativeAISupport Plugin

#include "Combat/CombatLibrary.h"

// Static member initialization
TMap<FString, TArray<FStatusEffect>> UCombatLibrary::EntityStatusEffects;
TMap<FString, FAggroTable> UCombatLibrary::NPCAggroTables;
TArray<FCombatLogEntry> UCombatLibrary::CombatLog;

// ============================================
// DAMAGE CALCULATION
// ============================================

FDamageResult UCombatLibrary::CalculateDamage(
	const FDamageInput& Input,
	const FCombatStats& DefenderStats,
	float DefenderCurrentHP,
	float DefenderMaxHP)
{
	FDamageResult Result;

	// Roll for combat result
	ECombatResult CombatResult = RollCombatResult(Input, DefenderStats);
	Result.Result = CombatResult;

	// Handle miss/dodge/parry
	if (CombatResult == ECombatResult::Miss ||
		CombatResult == ECombatResult::Dodge ||
		CombatResult == ECombatResult::Parry)
	{
		return Result;
	}

	// Calculate base damage
	float BaseDamage = Input.BaseDamage;

	// Add weapon damage variance
	if (Input.WeaponDamageMax > Input.WeaponDamageMin)
	{
		BaseDamage += FMath::RandRange(Input.WeaponDamageMin, Input.WeaponDamageMax);
	}

	// Calculate damage based on type
	float FinalDamage = 0.0f;

	if (Input.DamageType == EDamageType::Physical)
	{
		FinalDamage = CalculatePhysicalDamage(
			BaseDamage,
			Input.AttackerStats.AttackPower,
			DefenderStats.Armor,
			Input.AttackerStats.ArmorPenetration,
			Input.SkillCoefficient
		);
	}
	else if (Input.DamageType == EDamageType::Pure)
	{
		// True damage ignores all defenses
		FinalDamage = BaseDamage * Input.SkillCoefficient;
	}
	else
	{
		// Magical damage
		FinalDamage = CalculateMagicalDamage(
			BaseDamage,
			Input.AttackerStats.SpellPower,
			DefenderStats.MagicDefense,
			Input.AttackerStats.MagicPenetration,
			Input.SkillCoefficient
		);

		// Apply elemental resistance
		float Resistance = DefenderStats.Resistances.GetResistance(Input.DamageType);
		float ResistReduction = CalculateResistanceReduction(Resistance);
		FinalDamage *= ResistReduction;
		Result.ResistedDamage = FinalDamage * (1.0f - ResistReduction);
	}

	// Store raw damage
	Result.RawDamage = FinalDamage;

	// Apply bonus damage multiplier
	FinalDamage *= Input.BonusDamageMultiplier;

	// Check for critical hit
	if (Input.bCanCrit && RollCritical(Input.AttackerStats.CriticalChance))
	{
		Result.bCritical = true;
		Result.CritMultiplier = CalculateCritMultiplier(Input.AttackerStats.CriticalDamage);
		FinalDamage *= Result.CritMultiplier;
		Result.Result = ECombatResult::CriticalHit;
	}

	// Handle block
	if (CombatResult == ECombatResult::Block)
	{
		Result.BlockedDamage = FMath::Min(FinalDamage, DefenderStats.BlockAmount);
		FinalDamage = FMath::Max(0.0f, FinalDamage - Result.BlockedDamage);
	}

	// Level difference modifier
	float LevelMod = GetLevelDifferenceModifier(Input.AttackerStats.Level, DefenderStats.Level);
	FinalDamage *= LevelMod;

	// Store final damage
	Result.FinalDamage = FMath::Max(1.0f, FinalDamage); // Minimum 1 damage

	// Check for killing blow
	if (Result.FinalDamage >= DefenderCurrentHP)
	{
		Result.bKillingBlow = true;
		Result.OverkillDamage = Result.FinalDamage - DefenderCurrentHP;
	}

	// Add applied status effects
	Result.AppliedEffects = Input.AppliedEffects;

	// Store damage by type
	Result.DamageByType.Add(Input.DamageType, Result.FinalDamage);

	return Result;
}

float UCombatLibrary::CalculatePhysicalDamage(
	float BaseDamage,
	float AttackPower,
	float TargetArmor,
	float ArmorPenetration,
	float SkillCoefficient)
{
	// Damage formula: (Base + AP * Coeff) * ArmorMitigation
	float ScaledDamage = BaseDamage + (AttackPower * SkillCoefficient * 0.5f);

	// Apply armor penetration
	float EffectiveArmor = FMath::Max(0.0f, TargetArmor - ArmorPenetration);

	// Armor mitigation: 1 - (Armor / (Armor + 100 + Level*10))
	float ArmorMitigation = 1.0f - (EffectiveArmor / (EffectiveArmor + 200.0f));
	ArmorMitigation = FMath::Clamp(ArmorMitigation, 0.25f, 1.0f);

	return ScaledDamage * ArmorMitigation;
}

float UCombatLibrary::CalculateMagicalDamage(
	float BaseDamage,
	float SpellPower,
	float TargetMagicDefense,
	float MagicPenetration,
	float SpellCoefficient)
{
	// Damage formula: (Base + SP * Coeff) * MagicMitigation
	float ScaledDamage = BaseDamage + (SpellPower * SpellCoefficient * 0.6f);

	// Apply magic penetration
	float EffectiveMDef = FMath::Max(0.0f, TargetMagicDefense - MagicPenetration);

	// Magic defense mitigation
	float MagicMitigation = 1.0f - (EffectiveMDef / (EffectiveMDef + 150.0f));
	MagicMitigation = FMath::Clamp(MagicMitigation, 0.25f, 1.0f);

	return ScaledDamage * MagicMitigation;
}

float UCombatLibrary::CalculateArmorMitigation(float Armor, int32 AttackerLevel)
{
	float ArmorConstant = 100.0f + (AttackerLevel * 10.0f);
	float Mitigation = Armor / (Armor + ArmorConstant);
	return FMath::Clamp(Mitigation, 0.0f, 0.75f);
}

float UCombatLibrary::CalculateResistanceReduction(float Resistance)
{
	// Resistance reduces damage by percentage
	// 100 resistance = 50% reduction
	float Reduction = Resistance / (Resistance + 100.0f);
	return 1.0f - FMath::Clamp(Reduction, 0.0f, 0.75f);
}

ECombatResult UCombatLibrary::RollCombatResult(
	const FDamageInput& Input,
	const FCombatStats& DefenderStats)
{
	float Roll = FMath::FRand() * 100.0f;

	// Miss chance (base 5%, modified by level difference)
	float MissChance = 5.0f;
	int32 LevelDiff = DefenderStats.Level - Input.AttackerStats.Level;
	if (LevelDiff > 0)
	{
		MissChance += LevelDiff * 1.0f;
	}
	MissChance = FMath::Clamp(MissChance, 0.0f, 40.0f);

	if (Roll < MissChance)
	{
		return ECombatResult::Miss;
	}
	Roll -= MissChance;

	// Dodge chance
	if (Input.bCanDodge)
	{
		if (Roll < DefenderStats.DodgeChance)
		{
			return ECombatResult::Dodge;
		}
		Roll -= DefenderStats.DodgeChance;
	}

	// Parry chance (melee only)
	if (Input.AttackType == EAttackType::Melee && DefenderStats.ParryChance > 0)
	{
		if (Roll < DefenderStats.ParryChance)
		{
			return ECombatResult::Parry;
		}
		Roll -= DefenderStats.ParryChance;
	}

	// Block chance
	if (Input.bCanBlock && DefenderStats.BlockChance > 0)
	{
		if (Roll < DefenderStats.BlockChance)
		{
			return ECombatResult::Block;
		}
	}

	return ECombatResult::Hit;
}

bool UCombatLibrary::RollCritical(float CritChance)
{
	return FMath::FRand() * 100.0f < CritChance;
}

float UCombatLibrary::CalculateCritMultiplier(float BaseCritDamage, float BonusCritDamage)
{
	return (BaseCritDamage + BonusCritDamage) / 100.0f;
}

// ============================================
// HEALING CALCULATION
// ============================================

FHealingResult UCombatLibrary::CalculateHealing(
	const FHealingInput& Input,
	float TargetCurrentHP,
	float TargetMaxHP,
	float HealingReceivedBonus)
{
	FHealingResult Result;

	// Calculate base healing
	float Healing = Input.BaseHealing + (Input.HealerStats.SpellPower * Input.SpellCoefficient * 0.5f);

	Result.RawHealing = Healing;

	// Apply bonus healing
	Healing *= Input.BonusHealingMultiplier;
	Healing *= (1.0f + HealingReceivedBonus);

	// Check for critical heal
	if (Input.bCanCrit && RollCritical(Input.HealerStats.CriticalChance))
	{
		Result.bCritical = true;
		Result.CritMultiplier = CalculateCritMultiplier(Input.HealerStats.CriticalDamage);
		Healing *= Result.CritMultiplier;
	}

	// Calculate overhealing
	float MaxHealing = TargetMaxHP - TargetCurrentHP;
	if (Healing > MaxHealing)
	{
		Result.OverHealing = Healing - MaxHealing;
		Healing = MaxHealing;
	}

	Result.FinalHealing = FMath::Max(0.0f, Healing);

	return Result;
}

float UCombatLibrary::CalculateHOTTick(float TotalHealing, float Duration, float TickInterval)
{
	int32 NumTicks = FMath::CeilToInt(Duration / TickInterval);
	if (NumTicks <= 0) return TotalHealing;
	return TotalHealing / static_cast<float>(NumTicks);
}

// ============================================
// STATUS EFFECTS
// ============================================

bool UCombatLibrary::ApplyStatusEffect(const FString& TargetID, const FStatusEffect& Effect)
{
	if (!EntityStatusEffects.Contains(TargetID))
	{
		EntityStatusEffects.Add(TargetID, TArray<FStatusEffect>());
	}

	TArray<FStatusEffect>& Effects = EntityStatusEffects[TargetID];

	// Check for existing effect of same type
	for (FStatusEffect& Existing : Effects)
	{
		if (Existing.EffectType == Effect.EffectType)
		{
			// Stack or refresh
			if (Existing.Stacks < Existing.MaxStacks)
			{
				Existing.Stacks++;
			}
			Existing.RemainingDuration = Effect.Duration;
			return true;
		}
	}

	// Add new effect
	FStatusEffect NewEffect = Effect;
	NewEffect.RemainingDuration = Effect.Duration;
	NewEffect.NextTickTime = Effect.TickInterval;
	Effects.Add(NewEffect);

	return true;
}

bool UCombatLibrary::RemoveStatusEffect(const FString& TargetID, EStatusEffectType EffectType)
{
	TArray<FStatusEffect>* Effects = EntityStatusEffects.Find(TargetID);
	if (!Effects) return false;

	for (int32 i = Effects->Num() - 1; i >= 0; i--)
	{
		if ((*Effects)[i].EffectType == EffectType)
		{
			Effects->RemoveAt(i);
			return true;
		}
	}

	return false;
}

int32 UCombatLibrary::RemoveAllEffectsOfType(
	const FString& TargetID,
	bool bRemoveDebuffs,
	bool bRemoveBuffs,
	bool bRemoveDOTs)
{
	TArray<FStatusEffect>* Effects = EntityStatusEffects.Find(TargetID);
	if (!Effects) return 0;

	int32 RemovedCount = 0;

	for (int32 i = Effects->Num() - 1; i >= 0; i--)
	{
		const FStatusEffect& Effect = (*Effects)[i];

		bool bShouldRemove = false;

		if (bRemoveDebuffs && IsDebuff(Effect.EffectType)) bShouldRemove = true;
		if (bRemoveDOTs && IsDOT(Effect.EffectType)) bShouldRemove = true;
		if (bRemoveBuffs && !IsDebuff(Effect.EffectType) && !IsDOT(Effect.EffectType)) bShouldRemove = true;

		if (bShouldRemove && Effect.bDispellable)
		{
			Effects->RemoveAt(i);
			RemovedCount++;
		}
	}

	return RemovedCount;
}

TArray<FStatusEffect> UCombatLibrary::GetActiveEffects(const FString& TargetID)
{
	if (const TArray<FStatusEffect>* Effects = EntityStatusEffects.Find(TargetID))
	{
		return *Effects;
	}
	return TArray<FStatusEffect>();
}

bool UCombatLibrary::HasStatusEffect(const FString& TargetID, EStatusEffectType EffectType)
{
	const TArray<FStatusEffect>* Effects = EntityStatusEffects.Find(TargetID);
	if (!Effects) return false;

	for (const FStatusEffect& Effect : *Effects)
	{
		if (Effect.EffectType == EffectType) return true;
	}
	return false;
}

TArray<FDamageResult> UCombatLibrary::UpdateStatusEffects(const FString& TargetID, float DeltaTime)
{
	TArray<FDamageResult> DamageResults;

	TArray<FStatusEffect>* Effects = EntityStatusEffects.Find(TargetID);
	if (!Effects) return DamageResults;

	for (int32 i = Effects->Num() - 1; i >= 0; i--)
	{
		FStatusEffect& Effect = (*Effects)[i];

		// Update duration
		if (Effect.Duration > 0)
		{
			Effect.RemainingDuration -= DeltaTime;

			if (Effect.RemainingDuration <= 0)
			{
				Effects->RemoveAt(i);
				continue;
			}
		}

		// Process DOT ticks
		if (IsDOT(Effect.EffectType))
		{
			Effect.NextTickTime -= DeltaTime;

			if (Effect.NextTickTime <= 0)
			{
				Effect.NextTickTime = Effect.TickInterval;

				FDamageResult TickResult;
				TickResult.Result = ECombatResult::Hit;
				TickResult.FinalDamage = Effect.Value * Effect.Stacks;

				// Determine damage type from effect type
				switch (Effect.EffectType)
				{
				case EStatusEffectType::Burning:
					TickResult.DamageByType.Add(EDamageType::Fire, TickResult.FinalDamage);
					break;
				case EStatusEffectType::Poisoned:
					TickResult.DamageByType.Add(EDamageType::Poison, TickResult.FinalDamage);
					break;
				case EStatusEffectType::Bleeding:
					TickResult.DamageByType.Add(EDamageType::Physical, TickResult.FinalDamage);
					break;
				case EStatusEffectType::Frozen:
					TickResult.DamageByType.Add(EDamageType::Ice, TickResult.FinalDamage);
					break;
				case EStatusEffectType::Shocked:
					TickResult.DamageByType.Add(EDamageType::Lightning, TickResult.FinalDamage);
					break;
				default:
					break;
				}

				DamageResults.Add(TickResult);
			}
		}
	}

	return DamageResults;
}

float UCombatLibrary::CalculateDOTTick(const FStatusEffect& Effect, const FCombatStats& SourceStats)
{
	return Effect.Value * Effect.Stacks;
}

bool UCombatLibrary::IsDebuff(EStatusEffectType EffectType)
{
	switch (EffectType)
	{
	case EStatusEffectType::Burning:
	case EStatusEffectType::Bleeding:
	case EStatusEffectType::Poisoned:
	case EStatusEffectType::Frozen:
	case EStatusEffectType::Shocked:
	case EStatusEffectType::Stunned:
	case EStatusEffectType::Silenced:
	case EStatusEffectType::Rooted:
	case EStatusEffectType::Slowed:
	case EStatusEffectType::Feared:
	case EStatusEffectType::Charmed:
	case EStatusEffectType::Confused:
	case EStatusEffectType::Blinded:
	case EStatusEffectType::Sleeping:
	case EStatusEffectType::Weakened:
	case EStatusEffectType::Cursed:
	case EStatusEffectType::ArmorBroken:
	case EStatusEffectType::Exposed:
		return true;
	default:
		return false;
	}
}

bool UCombatLibrary::IsDOT(EStatusEffectType EffectType)
{
	switch (EffectType)
	{
	case EStatusEffectType::Burning:
	case EStatusEffectType::Bleeding:
	case EStatusEffectType::Poisoned:
	case EStatusEffectType::Frozen:
	case EStatusEffectType::Shocked:
		return true;
	default:
		return false;
	}
}

bool UCombatLibrary::IsCrowdControl(EStatusEffectType EffectType)
{
	switch (EffectType)
	{
	case EStatusEffectType::Stunned:
	case EStatusEffectType::Silenced:
	case EStatusEffectType::Rooted:
	case EStatusEffectType::Slowed:
	case EStatusEffectType::Feared:
	case EStatusEffectType::Charmed:
	case EStatusEffectType::Confused:
	case EStatusEffectType::Blinded:
	case EStatusEffectType::Sleeping:
		return true;
	default:
		return false;
	}
}

// ============================================
// THREAT & AGGRO
// ============================================

void UCombatLibrary::AddThreat(
	const FString& NPCID,
	const FString& PlayerID,
	const FString& PlayerName,
	float ThreatAmount)
{
	if (!NPCAggroTables.Contains(NPCID))
	{
		FAggroTable NewTable;
		NewTable.OwnerID = NPCID;
		NewTable.bInCombat = true;
		NewTable.CombatStartTime = FDateTime::Now();
		NPCAggroTables.Add(NPCID, NewTable);
	}

	FAggroTable& Table = NPCAggroTables[NPCID];
	Table.bInCombat = true;

	// Find or create threat entry
	FThreatEntry* Entry = nullptr;
	for (FThreatEntry& Existing : Table.ThreatList)
	{
		if (Existing.EntityID == PlayerID)
		{
			Entry = &Existing;
			break;
		}
	}

	if (!Entry)
	{
		FThreatEntry NewEntry;
		NewEntry.EntityID = PlayerID;
		NewEntry.EntityName = PlayerName;
		Table.ThreatList.Add(NewEntry);
		Entry = &Table.ThreatList.Last();
	}

	Entry->ThreatValue += ThreatAmount;

	// Recalculate percentages and target
	float TotalThreat = 0.0f;
	float HighestThreat = 0.0f;
	FString HighestID;

	for (const FThreatEntry& E : Table.ThreatList)
	{
		TotalThreat += E.ThreatValue;
		if (E.ThreatValue > HighestThreat)
		{
			HighestThreat = E.ThreatValue;
			HighestID = E.EntityID;
		}
	}

	for (FThreatEntry& E : Table.ThreatList)
	{
		E.ThreatPercent = (TotalThreat > 0) ? (E.ThreatValue / TotalThreat * 100.0f) : 0.0f;
	}

	// Aggro switch requires 10% more threat
	if (HighestID != Table.CurrentTargetID)
	{
		float CurrentTargetThreat = 0.0f;
		for (const FThreatEntry& E : Table.ThreatList)
		{
			if (E.EntityID == Table.CurrentTargetID)
			{
				CurrentTargetThreat = E.ThreatValue;
				break;
			}
		}

		if (HighestThreat > CurrentTargetThreat * 1.1f || Table.CurrentTargetID.IsEmpty())
		{
			Table.CurrentTargetID = HighestID;
		}
	}
}

void UCombatLibrary::ReduceThreat(
	const FString& NPCID,
	const FString& PlayerID,
	float ReductionPercent)
{
	FAggroTable* Table = NPCAggroTables.Find(NPCID);
	if (!Table) return;

	for (FThreatEntry& Entry : Table->ThreatList)
	{
		if (Entry.EntityID == PlayerID)
		{
			Entry.ThreatValue *= (1.0f - ReductionPercent / 100.0f);
			Entry.ThreatValue = FMath::Max(0.0f, Entry.ThreatValue);
			break;
		}
	}
}

FAggroTable UCombatLibrary::GetAggroTable(const FString& NPCID)
{
	if (const FAggroTable* Table = NPCAggroTables.Find(NPCID))
	{
		return *Table;
	}
	return FAggroTable();
}

FString UCombatLibrary::GetCurrentTarget(const FString& NPCID)
{
	if (const FAggroTable* Table = NPCAggroTables.Find(NPCID))
	{
		return Table->CurrentTargetID;
	}
	return FString();
}

void UCombatLibrary::ClearAggro(const FString& NPCID)
{
	NPCAggroTables.Remove(NPCID);
}

float UCombatLibrary::CalculateThreatFromDamage(float Damage, float ThreatModifier)
{
	return Damage * ThreatModifier;
}

float UCombatLibrary::CalculateThreatFromHealing(float Healing, float ThreatModifier)
{
	return Healing * ThreatModifier;
}

// ============================================
// STAT CALCULATIONS
// ============================================

float UCombatLibrary::CalculateAttackPower(float Strength, int32 Level)
{
	return Strength * 2.0f + Level * 5.0f;
}

float UCombatLibrary::CalculateSpellPower(float Intelligence, int32 Level)
{
	return Intelligence * 2.0f + Level * 5.0f;
}

float UCombatLibrary::CalculateMaxHealth(float Constitution, int32 Level)
{
	return 100.0f + (Constitution * 10.0f) + (Level * 15.0f);
}

float UCombatLibrary::CalculateMaxMana(float Wisdom, int32 Level)
{
	return 50.0f + (Wisdom * 8.0f) + (Level * 10.0f);
}

float UCombatLibrary::CalculateCritChance(float Dexterity, int32 Level)
{
	return 5.0f + (Dexterity / 10.0f);
}

float UCombatLibrary::CalculateDodgeChance(float Dexterity, int32 Level)
{
	return 5.0f + (Dexterity / 15.0f);
}

float UCombatLibrary::GetLevelDifferenceModifier(int32 AttackerLevel, int32 DefenderLevel)
{
	int32 LevelDiff = AttackerLevel - DefenderLevel;

	if (LevelDiff >= 3)
	{
		return FMath::Min(1.5f, 1.0f + (LevelDiff - 2) * 0.1f);
	}
	else if (LevelDiff <= -3)
	{
		return FMath::Max(0.5f, 1.0f + (LevelDiff + 2) * 0.1f);
	}

	return 1.0f;
}

// ============================================
// COMBAT LOG
// ============================================

void UCombatLibrary::AddCombatLogEntry(
	const FString& SourceID,
	const FString& SourceName,
	const FString& TargetID,
	const FString& TargetName,
	const FString& AbilityName,
	const FDamageResult& DamageResult)
{
	FCombatLogEntry Entry;
	Entry.Timestamp = FDateTime::Now();
	Entry.SourceID = SourceID;
	Entry.SourceName = SourceName;
	Entry.TargetID = TargetID;
	Entry.TargetName = TargetName;
	Entry.AbilityName = AbilityName;
	Entry.DamageResult = DamageResult;
	Entry.bIsDamage = true;

	CombatLog.Add(Entry);

	// Trim log if too long
	while (CombatLog.Num() > MaxLogEntries)
	{
		CombatLog.RemoveAt(0);
	}
}

void UCombatLibrary::AddHealingLogEntry(
	const FString& SourceID,
	const FString& SourceName,
	const FString& TargetID,
	const FString& TargetName,
	const FString& AbilityName,
	const FHealingResult& HealingResult)
{
	FCombatLogEntry Entry;
	Entry.Timestamp = FDateTime::Now();
	Entry.SourceID = SourceID;
	Entry.SourceName = SourceName;
	Entry.TargetID = TargetID;
	Entry.TargetName = TargetName;
	Entry.AbilityName = AbilityName;
	Entry.HealingResult = HealingResult;
	Entry.bIsDamage = false;

	CombatLog.Add(Entry);

	while (CombatLog.Num() > MaxLogEntries)
	{
		CombatLog.RemoveAt(0);
	}
}

TArray<FCombatLogEntry> UCombatLibrary::GetCombatLog(int32 MaxEntries)
{
	if (MaxEntries <= 0 || MaxEntries >= CombatLog.Num())
	{
		return CombatLog;
	}

	TArray<FCombatLogEntry> Result;
	int32 StartIndex = CombatLog.Num() - MaxEntries;
	for (int32 i = StartIndex; i < CombatLog.Num(); i++)
	{
		Result.Add(CombatLog[i]);
	}
	return Result;
}

void UCombatLibrary::ClearCombatLog()
{
	CombatLog.Empty();
}

TMap<FString, float> UCombatLibrary::GetDPSSummary(float TimeWindowSeconds)
{
	TMap<FString, float> DPSSummary;
	TMap<FString, float> TotalDamage;

	FDateTime Now = FDateTime::Now();

	for (const FCombatLogEntry& Entry : CombatLog)
	{
		if (!Entry.bIsDamage) continue;

		FTimespan TimeDiff = Now - Entry.Timestamp;
		if (TimeDiff.GetTotalSeconds() > TimeWindowSeconds) continue;

		float& Damage = TotalDamage.FindOrAdd(Entry.SourceID);
		Damage += Entry.DamageResult.FinalDamage;
	}

	for (const auto& Pair : TotalDamage)
	{
		DPSSummary.Add(Pair.Key, Pair.Value / TimeWindowSeconds);
	}

	return DPSSummary;
}

TMap<FString, float> UCombatLibrary::GetHPSSummary(float TimeWindowSeconds)
{
	TMap<FString, float> HPSSummary;
	TMap<FString, float> TotalHealing;

	FDateTime Now = FDateTime::Now();

	for (const FCombatLogEntry& Entry : CombatLog)
	{
		if (Entry.bIsDamage) continue;

		FTimespan TimeDiff = Now - Entry.Timestamp;
		if (TimeDiff.GetTotalSeconds() > TimeWindowSeconds) continue;

		float& Healing = TotalHealing.FindOrAdd(Entry.SourceID);
		Healing += Entry.HealingResult.FinalHealing;
	}

	for (const auto& Pair : TotalHealing)
	{
		HPSSummary.Add(Pair.Key, Pair.Value / TimeWindowSeconds);
	}

	return HPSSummary;
}

// ============================================
// UTILITY FUNCTIONS
// ============================================

FText UCombatLibrary::GetDamageTypeDisplayName(EDamageType Type)
{
	switch (Type)
	{
	case EDamageType::Physical: return FText::FromString(TEXT("Physisch"));
	case EDamageType::Fire: return FText::FromString(TEXT("Feuer"));
	case EDamageType::Ice: return FText::FromString(TEXT("Eis"));
	case EDamageType::Lightning: return FText::FromString(TEXT("Blitz"));
	case EDamageType::Poison: return FText::FromString(TEXT("Gift"));
	case EDamageType::Holy: return FText::FromString(TEXT("Heilig"));
	case EDamageType::Shadow: return FText::FromString(TEXT("Schatten"));
	case EDamageType::Arcane: return FText::FromString(TEXT("Arkan"));
	case EDamageType::Nature: return FText::FromString(TEXT("Natur"));
	case EDamageType::Pure: return FText::FromString(TEXT("Reiner Schaden"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FLinearColor UCombatLibrary::GetDamageTypeColor(EDamageType Type)
{
	switch (Type)
	{
	case EDamageType::Physical: return FLinearColor(0.8f, 0.8f, 0.8f);
	case EDamageType::Fire: return FLinearColor(1.0f, 0.4f, 0.0f);
	case EDamageType::Ice: return FLinearColor(0.4f, 0.8f, 1.0f);
	case EDamageType::Lightning: return FLinearColor(1.0f, 1.0f, 0.2f);
	case EDamageType::Poison: return FLinearColor(0.2f, 0.8f, 0.2f);
	case EDamageType::Holy: return FLinearColor(1.0f, 1.0f, 0.8f);
	case EDamageType::Shadow: return FLinearColor(0.5f, 0.2f, 0.8f);
	case EDamageType::Arcane: return FLinearColor(0.8f, 0.4f, 1.0f);
	case EDamageType::Nature: return FLinearColor(0.4f, 0.8f, 0.4f);
	case EDamageType::Pure: return FLinearColor(1.0f, 1.0f, 1.0f);
	default: return FLinearColor::White;
	}
}

FText UCombatLibrary::GetStatusEffectDisplayName(EStatusEffectType Type)
{
	switch (Type)
	{
	case EStatusEffectType::Burning: return FText::FromString(TEXT("Brennend"));
	case EStatusEffectType::Bleeding: return FText::FromString(TEXT("Blutend"));
	case EStatusEffectType::Poisoned: return FText::FromString(TEXT("Vergiftet"));
	case EStatusEffectType::Frozen: return FText::FromString(TEXT("Gefroren"));
	case EStatusEffectType::Shocked: return FText::FromString(TEXT("Schockiert"));
	case EStatusEffectType::Stunned: return FText::FromString(TEXT("Betäubt"));
	case EStatusEffectType::Silenced: return FText::FromString(TEXT("Verstummt"));
	case EStatusEffectType::Rooted: return FText::FromString(TEXT("Verwurzelt"));
	case EStatusEffectType::Slowed: return FText::FromString(TEXT("Verlangsamt"));
	case EStatusEffectType::Feared: return FText::FromString(TEXT("Verängstigt"));
	case EStatusEffectType::Regenerating: return FText::FromString(TEXT("Regenerierend"));
	case EStatusEffectType::Shielded: return FText::FromString(TEXT("Geschützt"));
	case EStatusEffectType::Empowered: return FText::FromString(TEXT("Ermächtigt"));
	case EStatusEffectType::Hasted: return FText::FromString(TEXT("Beschleunigt"));
	case EStatusEffectType::Invulnerable: return FText::FromString(TEXT("Unverwundbar"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FText UCombatLibrary::GetCombatResultDisplayName(ECombatResult Result)
{
	switch (Result)
	{
	case ECombatResult::Hit: return FText::FromString(TEXT("Treffer"));
	case ECombatResult::CriticalHit: return FText::FromString(TEXT("Kritisch!"));
	case ECombatResult::Miss: return FText::FromString(TEXT("Verfehlt"));
	case ECombatResult::Dodge: return FText::FromString(TEXT("Ausgewichen"));
	case ECombatResult::Parry: return FText::FromString(TEXT("Pariert"));
	case ECombatResult::Block: return FText::FromString(TEXT("Geblockt"));
	case ECombatResult::Immune: return FText::FromString(TEXT("Immun"));
	case ECombatResult::Absorb: return FText::FromString(TEXT("Absorbiert"));
	case ECombatResult::Resist: return FText::FromString(TEXT("Widerstanden"));
	default: return FText::FromString(TEXT(""));
	}
}

FString UCombatLibrary::FormatDamageNumber(float Damage, bool bCrit, EDamageType Type)
{
	if (bCrit)
	{
		return FString::Printf(TEXT("*%.0f*"), Damage);
	}
	return FString::Printf(TEXT("%.0f"), Damage);
}
