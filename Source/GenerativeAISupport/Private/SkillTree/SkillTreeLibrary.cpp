// Skill Tree Library Implementation
// Part of GenerativeAISupport Plugin

#include "SkillTree/SkillTreeLibrary.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

// Static member initialization
TMap<FString, FSkillTree> USkillTreeLibrary::TreeRegistry;
TMap<FString, FSkillNode> USkillTreeLibrary::SkillRegistry;
TMap<FString, FString> USkillTreeLibrary::SkillToTreeMap;

// ============================================
// SKILL TREE REGISTRY
// ============================================

int32 USkillTreeLibrary::LoadSkillTreesFromJSON(const FString& FilePath)
{
	FString JSONString;
	if (!FFileHelper::LoadFileToString(JSONString, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("SkillTree: Failed to load file: %s"), *FilePath);
		return 0;
	}

	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("SkillTree: Failed to parse JSON"));
		return 0;
	}

	const TArray<TSharedPtr<FJsonValue>>* TreesArray;
	if (!RootObject->TryGetArrayField(TEXT("skill_trees"), TreesArray))
	{
		UE_LOG(LogTemp, Error, TEXT("SkillTree: No skill_trees array found"));
		return 0;
	}

	int32 LoadedCount = 0;

	for (const TSharedPtr<FJsonValue>& TreeValue : *TreesArray)
	{
		const TSharedPtr<FJsonObject>* TreeObject;
		if (!TreeValue->TryGetObject(TreeObject)) continue;

		FSkillTree Tree;
		Tree.TreeID = (*TreeObject)->GetStringField(TEXT("tree_id"));
		Tree.DisplayName = FText::FromString((*TreeObject)->GetStringField(TEXT("display_name")));
		Tree.Description = FText::FromString((*TreeObject)->GetStringField(TEXT("description")));
		Tree.IconPath = (*TreeObject)->GetStringField(TEXT("icon_path"));
		Tree.RequiredLevel = (*TreeObject)->GetIntegerField(TEXT("required_level"));
		Tree.RequiredClass = (*TreeObject)->GetStringField(TEXT("required_class"));

		// Parse category
		FString CategoryStr = (*TreeObject)->GetStringField(TEXT("category"));
		if (CategoryStr == "Combat") Tree.Category = ESkillTreeCategory::Combat;
		else if (CategoryStr == "Magic") Tree.Category = ESkillTreeCategory::Magic;
		else if (CategoryStr == "Stealth") Tree.Category = ESkillTreeCategory::Stealth;
		else if (CategoryStr == "Crafting") Tree.Category = ESkillTreeCategory::Crafting;
		else if (CategoryStr == "Survival") Tree.Category = ESkillTreeCategory::Survival;
		else if (CategoryStr == "Social") Tree.Category = ESkillTreeCategory::Social;
		else if (CategoryStr == "Leadership") Tree.Category = ESkillTreeCategory::Leadership;

		// Parse skills
		const TArray<TSharedPtr<FJsonValue>>* SkillsArray;
		if ((*TreeObject)->TryGetArrayField(TEXT("skills"), SkillsArray))
		{
			for (const TSharedPtr<FJsonValue>& SkillValue : *SkillsArray)
			{
				const TSharedPtr<FJsonObject>* SkillObject;
				if (!SkillValue->TryGetObject(SkillObject)) continue;

				FSkillNode Skill;
				Skill.SkillID = (*SkillObject)->GetStringField(TEXT("skill_id"));
				Skill.DisplayName = FText::FromString((*SkillObject)->GetStringField(TEXT("display_name")));
				Skill.Description = FText::FromString((*SkillObject)->GetStringField(TEXT("description")));
				Skill.MaxRanks = (*SkillObject)->GetIntegerField(TEXT("max_ranks"));
				Skill.PointCost = (*SkillObject)->GetIntegerField(TEXT("point_cost"));
				Skill.IconPath = (*SkillObject)->GetStringField(TEXT("icon_path"));
				Skill.Category = Tree.Category;

				// Parse node type
				FString NodeTypeStr = (*SkillObject)->GetStringField(TEXT("node_type"));
				if (NodeTypeStr == "Passive") Skill.NodeType = ESkillNodeType::Passive;
				else if (NodeTypeStr == "Active") Skill.NodeType = ESkillNodeType::Active;
				else if (NodeTypeStr == "Ultimate") Skill.NodeType = ESkillNodeType::Ultimate;
				else if (NodeTypeStr == "Mastery") Skill.NodeType = ESkillNodeType::Mastery;
				else if (NodeTypeStr == "Perk") Skill.NodeType = ESkillNodeType::Perk;

				// Parse position
				const TSharedPtr<FJsonObject>* PositionObject;
				if ((*SkillObject)->TryGetObjectField(TEXT("position"), PositionObject))
				{
					Skill.Position.Row = (*PositionObject)->GetIntegerField(TEXT("row"));
					Skill.Position.Column = (*PositionObject)->GetIntegerField(TEXT("column"));
				}

				// Parse requirements
				const TArray<TSharedPtr<FJsonValue>>* ReqsArray;
				if ((*SkillObject)->TryGetArrayField(TEXT("requirements"), ReqsArray))
				{
					for (const TSharedPtr<FJsonValue>& ReqValue : *ReqsArray)
					{
						const TSharedPtr<FJsonObject>* ReqObject;
						if (!ReqValue->TryGetObject(ReqObject)) continue;

						FSkillRequirement Req;
						Req.RequiredSkillID = (*ReqObject)->GetStringField(TEXT("skill_id"));
						Req.RequiredRank = (*ReqObject)->GetIntegerField(TEXT("rank"));
						Req.RequiredLevel = (*ReqObject)->GetIntegerField(TEXT("level"));
						Skill.Requirements.Add(Req);
					}
				}

				// Parse connected skills
				const TArray<TSharedPtr<FJsonValue>>* ConnectedArray;
				if ((*SkillObject)->TryGetArrayField(TEXT("connected_skills"), ConnectedArray))
				{
					for (const TSharedPtr<FJsonValue>& ConnValue : *ConnectedArray)
					{
						Skill.ConnectedSkills.Add(ConnValue->AsString());
					}
				}

				// Parse modifiers
				const TArray<TSharedPtr<FJsonValue>>* ModsArray;
				if ((*SkillObject)->TryGetArrayField(TEXT("modifiers"), ModsArray))
				{
					for (const TSharedPtr<FJsonValue>& ModValue : *ModsArray)
					{
						const TSharedPtr<FJsonObject>* ModObject;
						if (!ModValue->TryGetObject(ModObject)) continue;

						FAttributeModifier Mod;
						Mod.Value = (*ModObject)->GetNumberField(TEXT("value"));
						Mod.bPerRank = (*ModObject)->GetBoolField(TEXT("per_rank"));

						FString AttrStr = (*ModObject)->GetStringField(TEXT("attribute"));
						if (AttrStr == "Health") Mod.Attribute = EAttributeType::Health;
						else if (AttrStr == "MaxHealth") Mod.Attribute = EAttributeType::MaxHealth;
						else if (AttrStr == "Mana") Mod.Attribute = EAttributeType::Mana;
						else if (AttrStr == "MaxMana") Mod.Attribute = EAttributeType::MaxMana;
						else if (AttrStr == "Stamina") Mod.Attribute = EAttributeType::Stamina;
						else if (AttrStr == "MaxStamina") Mod.Attribute = EAttributeType::MaxStamina;
						else if (AttrStr == "Strength") Mod.Attribute = EAttributeType::Strength;
						else if (AttrStr == "Dexterity") Mod.Attribute = EAttributeType::Dexterity;
						else if (AttrStr == "Intelligence") Mod.Attribute = EAttributeType::Intelligence;
						else if (AttrStr == "Wisdom") Mod.Attribute = EAttributeType::Wisdom;
						else if (AttrStr == "Constitution") Mod.Attribute = EAttributeType::Constitution;
						else if (AttrStr == "Charisma") Mod.Attribute = EAttributeType::Charisma;
						else if (AttrStr == "AttackPower") Mod.Attribute = EAttributeType::AttackPower;
						else if (AttrStr == "SpellPower") Mod.Attribute = EAttributeType::SpellPower;
						else if (AttrStr == "PhysicalDefense") Mod.Attribute = EAttributeType::PhysicalDefense;
						else if (AttrStr == "MagicDefense") Mod.Attribute = EAttributeType::MagicDefense;
						else if (AttrStr == "CriticalChance") Mod.Attribute = EAttributeType::CriticalChance;
						else if (AttrStr == "CriticalDamage") Mod.Attribute = EAttributeType::CriticalDamage;
						else if (AttrStr == "AttackSpeed") Mod.Attribute = EAttributeType::AttackSpeed;
						else if (AttrStr == "CastSpeed") Mod.Attribute = EAttributeType::CastSpeed;
						else if (AttrStr == "MovementSpeed") Mod.Attribute = EAttributeType::MovementSpeed;
						else if (AttrStr == "DodgeChance") Mod.Attribute = EAttributeType::DodgeChance;
						else if (AttrStr == "BlockChance") Mod.Attribute = EAttributeType::BlockChance;
						else if (AttrStr == "HealthRegen") Mod.Attribute = EAttributeType::HealthRegen;
						else if (AttrStr == "ManaRegen") Mod.Attribute = EAttributeType::ManaRegen;
						else if (AttrStr == "StaminaRegen") Mod.Attribute = EAttributeType::StaminaRegen;
						else if (AttrStr == "ExperienceGain") Mod.Attribute = EAttributeType::ExperienceGain;
						else if (AttrStr == "GoldFind") Mod.Attribute = EAttributeType::GoldFind;
						else if (AttrStr == "ItemFind") Mod.Attribute = EAttributeType::ItemFind;

						FString ModTypeStr = (*ModObject)->GetStringField(TEXT("type"));
						if (ModTypeStr == "Flat") Mod.ModifierType = EModifierType::Flat;
						else if (ModTypeStr == "Percent") Mod.ModifierType = EModifierType::Percent;
						else if (ModTypeStr == "Multiplicative") Mod.ModifierType = EModifierType::Multiplicative;

						Skill.Modifiers.Add(Mod);
					}
				}

				// Parse active effect
				const TSharedPtr<FJsonObject>* EffectObject;
				if ((*SkillObject)->TryGetObjectField(TEXT("active_effect"), EffectObject))
				{
					Skill.ActiveEffect.EffectID = (*EffectObject)->GetStringField(TEXT("effect_id"));
					Skill.ActiveEffect.EffectName = FText::FromString((*EffectObject)->GetStringField(TEXT("name")));
					Skill.ActiveEffect.BaseValue = (*EffectObject)->GetNumberField(TEXT("base_value"));
					Skill.ActiveEffect.ValuePerRank = (*EffectObject)->GetNumberField(TEXT("value_per_rank"));
					Skill.ActiveEffect.Duration = (*EffectObject)->GetNumberField(TEXT("duration"));
					Skill.ActiveEffect.Cooldown = (*EffectObject)->GetNumberField(TEXT("cooldown"));
					Skill.ActiveEffect.ResourceCost = (*EffectObject)->GetNumberField(TEXT("resource_cost"));
					Skill.ActiveEffect.Range = (*EffectObject)->GetNumberField(TEXT("range"));
					Skill.ActiveEffect.AoERadius = (*EffectObject)->GetNumberField(TEXT("aoe_radius"));
				}

				// Parse tags
				const TArray<TSharedPtr<FJsonValue>>* TagsArray;
				if ((*SkillObject)->TryGetArrayField(TEXT("tags"), TagsArray))
				{
					for (const TSharedPtr<FJsonValue>& TagValue : *TagsArray)
					{
						Skill.Tags.Add(TagValue->AsString());
					}
				}

				Tree.Skills.Add(Skill);
			}
		}

		RegisterSkillTree(Tree);
		LoadedCount++;
	}

	RebuildSkillLookup();
	UE_LOG(LogTemp, Log, TEXT("SkillTree: Loaded %d skill trees"), LoadedCount);
	return LoadedCount;
}

void USkillTreeLibrary::RegisterSkillTree(const FSkillTree& Tree)
{
	TreeRegistry.Add(Tree.TreeID, Tree);
}

bool USkillTreeLibrary::GetSkillTree(const FString& TreeID, FSkillTree& OutTree)
{
	if (const FSkillTree* Found = TreeRegistry.Find(TreeID))
	{
		OutTree = *Found;
		return true;
	}
	return false;
}

TArray<FSkillTree> USkillTreeLibrary::GetAllSkillTrees()
{
	TArray<FSkillTree> Result;
	TreeRegistry.GenerateValueArray(Result);
	return Result;
}

TArray<FSkillTree> USkillTreeLibrary::GetSkillTreesByCategory(ESkillTreeCategory Category)
{
	TArray<FSkillTree> Result;
	for (const auto& Pair : TreeRegistry)
	{
		if (Pair.Value.Category == Category)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

TArray<FSkillTree> USkillTreeLibrary::GetSkillTreesForClass(const FString& ClassName, int32 PlayerLevel)
{
	TArray<FSkillTree> Result;
	for (const auto& Pair : TreeRegistry)
	{
		const FSkillTree& Tree = Pair.Value;

		// Check level requirement
		if (PlayerLevel < Tree.RequiredLevel) continue;

		// Check class requirement
		if (!Tree.RequiredClass.IsEmpty() && Tree.RequiredClass != ClassName) continue;

		Result.Add(Tree);
	}
	return Result;
}

bool USkillTreeLibrary::GetSkillNode(const FString& SkillID, FSkillNode& OutSkill)
{
	if (const FSkillNode* Found = SkillRegistry.Find(SkillID))
	{
		OutSkill = *Found;
		return true;
	}
	return false;
}

void USkillTreeLibrary::RebuildSkillLookup()
{
	SkillRegistry.Empty();
	SkillToTreeMap.Empty();

	for (const auto& TreePair : TreeRegistry)
	{
		for (const FSkillNode& Skill : TreePair.Value.Skills)
		{
			SkillRegistry.Add(Skill.SkillID, Skill);
			SkillToTreeMap.Add(Skill.SkillID, TreePair.Key);
		}
	}
}

// ============================================
// SKILL UNLOCK & MANAGEMENT
// ============================================

bool USkillTreeLibrary::CanUnlockSkill(
	const FString& SkillID,
	const FPlayerSkillData& PlayerSkills,
	int32 PlayerLevel,
	const TMap<EAttributeType, float>& PlayerAttributes,
	FText& OutReason)
{
	FSkillNode Skill;
	if (!GetSkillNode(SkillID, Skill))
	{
		OutReason = FText::FromString(TEXT("Fähigkeit nicht gefunden"));
		return false;
	}

	// Check if already maxed
	int32 CurrentRank = PlayerSkills.GetSkillRank(SkillID);
	if (CurrentRank >= Skill.MaxRanks)
	{
		OutReason = FText::FromString(TEXT("Bereits maximiert"));
		return false;
	}

	// Check skill points
	if (PlayerSkills.AvailablePoints < Skill.PointCost)
	{
		OutReason = FText::FromString(FString::Printf(TEXT("Benötigt %d Fähigkeitspunkte"), Skill.PointCost));
		return false;
	}

	// Check requirements
	for (const FSkillRequirement& Req : Skill.Requirements)
	{
		// Check required skill
		if (!Req.RequiredSkillID.IsEmpty())
		{
			int32 ReqRank = PlayerSkills.GetSkillRank(Req.RequiredSkillID);
			if (ReqRank < Req.RequiredRank)
			{
				FSkillNode ReqSkill;
				if (GetSkillNode(Req.RequiredSkillID, ReqSkill))
				{
					OutReason = FText::FromString(FString::Printf(
						TEXT("Benötigt %s Rang %d"),
						*ReqSkill.DisplayName.ToString(),
						Req.RequiredRank));
				}
				return false;
			}
		}

		// Check level
		if (Req.RequiredLevel > 0 && PlayerLevel < Req.RequiredLevel)
		{
			OutReason = FText::FromString(FString::Printf(TEXT("Benötigt Stufe %d"), Req.RequiredLevel));
			return false;
		}

		// Check attribute
		if (Req.RequiredAttributeValue > 0)
		{
			const float* AttrValue = PlayerAttributes.Find(Req.RequiredAttribute);
			if (!AttrValue || *AttrValue < Req.RequiredAttributeValue)
			{
				OutReason = FText::FromString(FString::Printf(
					TEXT("Benötigt %s %d"),
					*GetAttributeDisplayName(Req.RequiredAttribute).ToString(),
					Req.RequiredAttributeValue));
				return false;
			}
		}
	}

	return true;
}

FSkillUnlockResult USkillTreeLibrary::UnlockSkill(
	const FString& SkillID,
	FPlayerSkillData& PlayerSkills,
	int32 PlayerLevel,
	const TMap<EAttributeType, float>& PlayerAttributes)
{
	FSkillUnlockResult Result;

	FText Reason;
	if (!CanUnlockSkill(SkillID, PlayerSkills, PlayerLevel, PlayerAttributes, Reason))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = Reason;
		return Result;
	}

	FSkillNode Skill;
	if (!GetSkillNode(SkillID, Skill))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FText::FromString(TEXT("Fähigkeit nicht gefunden"));
		return Result;
	}

	// Deduct skill points
	PlayerSkills.AvailablePoints -= Skill.PointCost;

	// Find or create progress entry
	FSkillProgress* Progress = nullptr;
	for (FSkillProgress& P : PlayerSkills.SkillProgress)
	{
		if (P.SkillID == SkillID)
		{
			Progress = &P;
			break;
		}
	}

	if (!Progress)
	{
		FSkillProgress NewProgress;
		NewProgress.SkillID = SkillID;
		NewProgress.CurrentRank = 0;
		NewProgress.UnlockTime = FDateTime::Now();
		PlayerSkills.SkillProgress.Add(NewProgress);
		Progress = &PlayerSkills.SkillProgress.Last();
	}

	// Increase rank
	Progress->CurrentRank++;

	Result.bSuccess = true;
	Result.NewRank = Progress->CurrentRank;
	Result.RemainingPoints = PlayerSkills.AvailablePoints;

	// Calculate gained modifiers
	for (const FAttributeModifier& Mod : Skill.Modifiers)
	{
		if (Mod.bPerRank)
		{
			// Only add the single rank gain
			FAttributeModifier SingleMod = Mod;
			Result.GainedModifiers.Add(SingleMod);
		}
		else if (Progress->CurrentRank == 1)
		{
			// Full modifier only on first unlock
			Result.GainedModifiers.Add(Mod);
		}
	}

	return Result;
}

ESkillUnlockStatus USkillTreeLibrary::GetSkillStatus(
	const FString& SkillID,
	const FPlayerSkillData& PlayerSkills,
	int32 PlayerLevel,
	const TMap<EAttributeType, float>& PlayerAttributes)
{
	FSkillNode Skill;
	if (!GetSkillNode(SkillID, Skill))
	{
		return ESkillUnlockStatus::Locked;
	}

	int32 CurrentRank = PlayerSkills.GetSkillRank(SkillID);

	if (CurrentRank >= Skill.MaxRanks)
	{
		return ESkillUnlockStatus::Maxed;
	}

	if (CurrentRank > 0)
	{
		return ESkillUnlockStatus::Unlocked;
	}

	FText Reason;
	if (CanUnlockSkill(SkillID, PlayerSkills, PlayerLevel, PlayerAttributes, Reason))
	{
		return ESkillUnlockStatus::Available;
	}

	return ESkillUnlockStatus::Locked;
}

int32 USkillTreeLibrary::GetSkillRank(const FString& SkillID, const FPlayerSkillData& PlayerSkills)
{
	return PlayerSkills.GetSkillRank(SkillID);
}

bool USkillTreeLibrary::IsSkillMaxed(const FString& SkillID, const FPlayerSkillData& PlayerSkills)
{
	FSkillNode Skill;
	if (!GetSkillNode(SkillID, Skill))
	{
		return false;
	}
	return PlayerSkills.GetSkillRank(SkillID) >= Skill.MaxRanks;
}

// ============================================
// SKILL POINTS
// ============================================

void USkillTreeLibrary::AddSkillPoints(FPlayerSkillData& PlayerSkills, int32 Points)
{
	PlayerSkills.AvailablePoints += Points;
	PlayerSkills.TotalPointsEarned += Points;
}

int32 USkillTreeLibrary::GetAvailableSkillPoints(const FPlayerSkillData& PlayerSkills)
{
	return PlayerSkills.AvailablePoints;
}

int32 USkillTreeLibrary::GetTotalPointsSpent(const FPlayerSkillData& PlayerSkills)
{
	int32 Total = 0;
	for (const FSkillProgress& Progress : PlayerSkills.SkillProgress)
	{
		FSkillNode Skill;
		if (GetSkillNode(Progress.SkillID, Skill))
		{
			Total += Progress.CurrentRank * Skill.PointCost;
		}
	}
	return Total;
}

int32 USkillTreeLibrary::GetPointsSpentInTree(const FString& TreeID, const FPlayerSkillData& PlayerSkills)
{
	FSkillTree Tree;
	if (!GetSkillTree(TreeID, Tree))
	{
		return 0;
	}

	int32 Total = 0;
	for (const FSkillNode& Skill : Tree.Skills)
	{
		int32 Rank = PlayerSkills.GetSkillRank(Skill.SkillID);
		Total += Rank * Skill.PointCost;
	}
	return Total;
}

int32 USkillTreeLibrary::GetSkillPointsForLevel(int32 Level)
{
	// 1 point per level, bonus at milestones
	int32 Points = Level;

	// Bonus points at levels 10, 20, 30, etc.
	Points += Level / 10;

	return Points;
}

// ============================================
// RESPEC & RESET
// ============================================

FSkillRespecResult USkillTreeLibrary::RespecAllSkills(
	FPlayerSkillData& PlayerSkills,
	int32& PlayerGold,
	int32 RespecCost)
{
	FSkillRespecResult Result;

	if (PlayerGold < RespecCost)
	{
		Result.bSuccess = false;
		return Result;
	}

	// Calculate points to refund
	int32 PointsToRefund = GetTotalPointsSpent(PlayerSkills);

	if (PointsToRefund == 0)
	{
		Result.bSuccess = false;
		return Result;
	}

	// Collect skill IDs
	for (const FSkillProgress& Progress : PlayerSkills.SkillProgress)
	{
		if (Progress.CurrentRank > 0)
		{
			Result.ResetSkillIDs.Add(Progress.SkillID);
		}
	}

	// Clear all progress
	PlayerSkills.SkillProgress.Empty();

	// Refund points
	PlayerSkills.AvailablePoints += PointsToRefund;

	// Deduct gold
	PlayerGold -= RespecCost;

	Result.bSuccess = true;
	Result.PointsRefunded = PointsToRefund;
	Result.GoldCost = RespecCost;

	return Result;
}

FSkillRespecResult USkillTreeLibrary::RespecTree(
	const FString& TreeID,
	FPlayerSkillData& PlayerSkills,
	int32& PlayerGold,
	int32 RespecCost)
{
	FSkillRespecResult Result;

	if (PlayerGold < RespecCost)
	{
		Result.bSuccess = false;
		return Result;
	}

	FSkillTree Tree;
	if (!GetSkillTree(TreeID, Tree))
	{
		Result.bSuccess = false;
		return Result;
	}

	int32 PointsToRefund = 0;

	// Find and reset skills in this tree
	for (int32 i = PlayerSkills.SkillProgress.Num() - 1; i >= 0; i--)
	{
		FSkillProgress& Progress = PlayerSkills.SkillProgress[i];

		// Check if this skill belongs to the tree
		for (const FSkillNode& Skill : Tree.Skills)
		{
			if (Skill.SkillID == Progress.SkillID && Progress.CurrentRank > 0)
			{
				PointsToRefund += Progress.CurrentRank * Skill.PointCost;
				Result.ResetSkillIDs.Add(Progress.SkillID);
				PlayerSkills.SkillProgress.RemoveAt(i);
				break;
			}
		}
	}

	if (PointsToRefund == 0)
	{
		Result.bSuccess = false;
		return Result;
	}

	// Refund points
	PlayerSkills.AvailablePoints += PointsToRefund;

	// Deduct gold
	PlayerGold -= RespecCost;

	Result.bSuccess = true;
	Result.PointsRefunded = PointsToRefund;
	Result.GoldCost = RespecCost;

	return Result;
}

int32 USkillTreeLibrary::CalculateRespecCost(const FPlayerSkillData& PlayerSkills, int32 PlayerLevel)
{
	int32 PointsSpent = GetTotalPointsSpent(PlayerSkills);
	// Base cost: 10 gold per point spent, scaled by level
	return (PointsSpent * 10) + (PlayerLevel * 5);
}

// ============================================
// ATTRIBUTE BONUSES
// ============================================

FSkillAttributeBonuses USkillTreeLibrary::CalculateAttributeBonuses(const FPlayerSkillData& PlayerSkills)
{
	FSkillAttributeBonuses Bonuses;

	for (const FSkillProgress& Progress : PlayerSkills.SkillProgress)
	{
		if (Progress.CurrentRank <= 0) continue;

		FSkillNode Skill;
		if (!GetSkillNode(Progress.SkillID, Skill)) continue;

		for (const FAttributeModifier& Mod : Skill.Modifiers)
		{
			float Value = Mod.bPerRank ? Mod.Value * Progress.CurrentRank : Mod.Value;

			switch (Mod.ModifierType)
			{
			case EModifierType::Flat:
				if (float* Existing = Bonuses.FlatBonuses.Find(Mod.Attribute))
				{
					*Existing += Value;
				}
				else
				{
					Bonuses.FlatBonuses.Add(Mod.Attribute, Value);
				}
				break;

			case EModifierType::Percent:
				if (float* Existing = Bonuses.PercentBonuses.Find(Mod.Attribute))
				{
					*Existing += Value;
				}
				else
				{
					Bonuses.PercentBonuses.Add(Mod.Attribute, Value);
				}
				break;

			case EModifierType::Multiplicative:
				if (float* Existing = Bonuses.MultiplierBonuses.Find(Mod.Attribute))
				{
					*Existing *= (1.0f + Value);
				}
				else
				{
					Bonuses.MultiplierBonuses.Add(Mod.Attribute, 1.0f + Value);
				}
				break;
			}
		}
	}

	return Bonuses;
}

float USkillTreeLibrary::GetAttributeBonus(
	EAttributeType Attribute,
	const FPlayerSkillData& PlayerSkills,
	float BaseValue)
{
	FSkillAttributeBonuses Bonuses = CalculateAttributeBonuses(PlayerSkills);

	float Result = BaseValue;

	// Add flat bonuses
	if (const float* Flat = Bonuses.FlatBonuses.Find(Attribute))
	{
		Result += *Flat;
	}

	// Apply percent bonuses
	if (const float* Percent = Bonuses.PercentBonuses.Find(Attribute))
	{
		Result *= (1.0f + *Percent / 100.0f);
	}

	// Apply multipliers
	if (const float* Mult = Bonuses.MultiplierBonuses.Find(Attribute))
	{
		Result *= *Mult;
	}

	return Result;
}

TMap<EAttributeType, float> USkillTreeLibrary::ApplySkillBonuses(
	const TMap<EAttributeType, float>& BaseAttributes,
	const FPlayerSkillData& PlayerSkills)
{
	TMap<EAttributeType, float> Result;

	for (const auto& Pair : BaseAttributes)
	{
		Result.Add(Pair.Key, GetAttributeBonus(Pair.Key, PlayerSkills, Pair.Value));
	}

	return Result;
}

// ============================================
// ACTIVE SKILLS
// ============================================

TArray<FSkillNode> USkillTreeLibrary::GetUnlockedActiveSkills(const FPlayerSkillData& PlayerSkills)
{
	TArray<FSkillNode> Result;

	for (const FSkillProgress& Progress : PlayerSkills.SkillProgress)
	{
		if (Progress.CurrentRank <= 0) continue;

		FSkillNode Skill;
		if (!GetSkillNode(Progress.SkillID, Skill)) continue;

		if (Skill.NodeType == ESkillNodeType::Active || Skill.NodeType == ESkillNodeType::Ultimate)
		{
			Result.Add(Skill);
		}
	}

	return Result;
}

FSkillEffect USkillTreeLibrary::GetSkillEffectAtRank(const FString& SkillID, int32 Rank)
{
	FSkillNode Skill;
	if (!GetSkillNode(SkillID, Skill))
	{
		return FSkillEffect();
	}

	FSkillEffect Effect = Skill.ActiveEffect;

	// Scale values by rank
	if (Rank > 1)
	{
		Effect.BaseValue += Effect.ValuePerRank * (Rank - 1);
	}

	return Effect;
}

bool USkillTreeLibrary::CanUseActiveSkill(
	const FString& SkillID,
	int32 Rank,
	float CurrentMana,
	float CurrentStamina,
	float CurrentCooldown)
{
	if (Rank <= 0) return false;

	FSkillEffect Effect = GetSkillEffectAtRank(SkillID, Rank);

	// Check cooldown
	if (CurrentCooldown > 0.0f) return false;

	// Check resource cost (assuming mana for spells, stamina for physical)
	if (Effect.ResourceCost > 0)
	{
		FSkillNode Skill;
		if (GetSkillNode(SkillID, Skill))
		{
			if (Skill.Category == ESkillTreeCategory::Magic)
			{
				if (CurrentMana < Effect.ResourceCost) return false;
			}
			else
			{
				if (CurrentStamina < Effect.ResourceCost) return false;
			}
		}
	}

	return true;
}

// ============================================
// TREE ANALYSIS
// ============================================

TArray<FSkillNode> USkillTreeLibrary::GetConnectedSkills(const FString& SkillID)
{
	TArray<FSkillNode> Result;

	FSkillNode Skill;
	if (!GetSkillNode(SkillID, Skill)) return Result;

	for (const FString& ConnectedID : Skill.ConnectedSkills)
	{
		FSkillNode Connected;
		if (GetSkillNode(ConnectedID, Connected))
		{
			Result.Add(Connected);
		}
	}

	return Result;
}

TArray<FString> USkillTreeLibrary::GetUnlockPath(
	const FString& SkillID,
	const FPlayerSkillData& PlayerSkills)
{
	TArray<FString> Path;

	// Simple BFS to find requirements
	TArray<FString> ToCheck;
	ToCheck.Add(SkillID);

	while (ToCheck.Num() > 0)
	{
		FString Current = ToCheck[0];
		ToCheck.RemoveAt(0);

		// Skip if already unlocked
		if (PlayerSkills.IsSkillUnlocked(Current) && Current != SkillID)
		{
			continue;
		}

		FSkillNode Skill;
		if (!GetSkillNode(Current, Skill)) continue;

		// Add to path
		if (!Path.Contains(Current))
		{
			Path.Insert(Current, 0);
		}

		// Add requirements
		for (const FSkillRequirement& Req : Skill.Requirements)
		{
			if (!Req.RequiredSkillID.IsEmpty() && !PlayerSkills.IsSkillUnlocked(Req.RequiredSkillID))
			{
				ToCheck.Add(Req.RequiredSkillID);
			}
		}
	}

	return Path;
}

int32 USkillTreeLibrary::GetPointsToReachSkill(
	const FString& SkillID,
	const FPlayerSkillData& PlayerSkills)
{
	TArray<FString> Path = GetUnlockPath(SkillID, PlayerSkills);
	int32 TotalPoints = 0;

	for (const FString& ID : Path)
	{
		FSkillNode Skill;
		if (GetSkillNode(ID, Skill))
		{
			int32 CurrentRank = PlayerSkills.GetSkillRank(ID);
			int32 RanksNeeded = 1 - CurrentRank; // Need at least 1 rank
			if (ID == SkillID)
			{
				RanksNeeded = Skill.MaxRanks - CurrentRank; // Full rank for target
			}
			if (RanksNeeded > 0)
			{
				TotalPoints += RanksNeeded * Skill.PointCost;
			}
		}
	}

	return TotalPoints;
}

TArray<FSkillNode> USkillTreeLibrary::GetSkillsByTag(const FString& Tag)
{
	TArray<FSkillNode> Result;

	for (const auto& Pair : SkillRegistry)
	{
		if (Pair.Value.Tags.Contains(Tag))
		{
			Result.Add(Pair.Value);
		}
	}

	return Result;
}

// ============================================
// UI HELPERS
// ============================================

FText USkillTreeLibrary::GetCategoryDisplayName(ESkillTreeCategory Category)
{
	switch (Category)
	{
	case ESkillTreeCategory::Combat: return FText::FromString(TEXT("Kampf"));
	case ESkillTreeCategory::Magic: return FText::FromString(TEXT("Magie"));
	case ESkillTreeCategory::Stealth: return FText::FromString(TEXT("Schleichen"));
	case ESkillTreeCategory::Crafting: return FText::FromString(TEXT("Handwerk"));
	case ESkillTreeCategory::Survival: return FText::FromString(TEXT("Überleben"));
	case ESkillTreeCategory::Social: return FText::FromString(TEXT("Sozial"));
	case ESkillTreeCategory::Leadership: return FText::FromString(TEXT("Führung"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FText USkillTreeLibrary::GetNodeTypeDisplayName(ESkillNodeType NodeType)
{
	switch (NodeType)
	{
	case ESkillNodeType::Passive: return FText::FromString(TEXT("Passiv"));
	case ESkillNodeType::Active: return FText::FromString(TEXT("Aktiv"));
	case ESkillNodeType::Ultimate: return FText::FromString(TEXT("Ultimativ"));
	case ESkillNodeType::Mastery: return FText::FromString(TEXT("Meisterschaft"));
	case ESkillNodeType::Perk: return FText::FromString(TEXT("Perk"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FText USkillTreeLibrary::GetAttributeDisplayName(EAttributeType Attribute)
{
	switch (Attribute)
	{
	case EAttributeType::Health: return FText::FromString(TEXT("Leben"));
	case EAttributeType::MaxHealth: return FText::FromString(TEXT("Max Leben"));
	case EAttributeType::Mana: return FText::FromString(TEXT("Mana"));
	case EAttributeType::MaxMana: return FText::FromString(TEXT("Max Mana"));
	case EAttributeType::Stamina: return FText::FromString(TEXT("Ausdauer"));
	case EAttributeType::MaxStamina: return FText::FromString(TEXT("Max Ausdauer"));
	case EAttributeType::Strength: return FText::FromString(TEXT("Stärke"));
	case EAttributeType::Dexterity: return FText::FromString(TEXT("Geschick"));
	case EAttributeType::Intelligence: return FText::FromString(TEXT("Intelligenz"));
	case EAttributeType::Wisdom: return FText::FromString(TEXT("Weisheit"));
	case EAttributeType::Constitution: return FText::FromString(TEXT("Konstitution"));
	case EAttributeType::Charisma: return FText::FromString(TEXT("Charisma"));
	case EAttributeType::AttackPower: return FText::FromString(TEXT("Angriffskraft"));
	case EAttributeType::SpellPower: return FText::FromString(TEXT("Zauberkraft"));
	case EAttributeType::PhysicalDefense: return FText::FromString(TEXT("Physische Verteidigung"));
	case EAttributeType::MagicDefense: return FText::FromString(TEXT("Magische Verteidigung"));
	case EAttributeType::CriticalChance: return FText::FromString(TEXT("Kritische Chance"));
	case EAttributeType::CriticalDamage: return FText::FromString(TEXT("Kritischer Schaden"));
	case EAttributeType::AttackSpeed: return FText::FromString(TEXT("Angriffsgeschwindigkeit"));
	case EAttributeType::CastSpeed: return FText::FromString(TEXT("Zaubergeschwindigkeit"));
	case EAttributeType::MovementSpeed: return FText::FromString(TEXT("Bewegungsgeschwindigkeit"));
	case EAttributeType::DodgeChance: return FText::FromString(TEXT("Ausweichchance"));
	case EAttributeType::BlockChance: return FText::FromString(TEXT("Blockchance"));
	case EAttributeType::HealthRegen: return FText::FromString(TEXT("Leben-Regeneration"));
	case EAttributeType::ManaRegen: return FText::FromString(TEXT("Mana-Regeneration"));
	case EAttributeType::StaminaRegen: return FText::FromString(TEXT("Ausdauer-Regeneration"));
	case EAttributeType::ExperienceGain: return FText::FromString(TEXT("Erfahrungsgewinn"));
	case EAttributeType::GoldFind: return FText::FromString(TEXT("Goldfund"));
	case EAttributeType::ItemFind: return FText::FromString(TEXT("Gegenstandsfund"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FText USkillTreeLibrary::GetSkillDescriptionWithValues(const FString& SkillID, int32 CurrentRank)
{
	FSkillNode Skill;
	if (!GetSkillNode(SkillID, Skill))
	{
		return FText::GetEmpty();
	}

	FString Description = Skill.Description.ToString();

	// Replace placeholders with actual values
	if (Skill.NodeType == ESkillNodeType::Active || Skill.NodeType == ESkillNodeType::Ultimate)
	{
		FSkillEffect Effect = GetSkillEffectAtRank(SkillID, FMath::Max(CurrentRank, 1));
		Description = Description.Replace(TEXT("{damage}"), *FString::Printf(TEXT("%.0f"), Effect.BaseValue));
		Description = Description.Replace(TEXT("{duration}"), *FString::Printf(TEXT("%.1f"), Effect.Duration));
		Description = Description.Replace(TEXT("{cooldown}"), *FString::Printf(TEXT("%.0f"), Effect.Cooldown));
		Description = Description.Replace(TEXT("{cost}"), *FString::Printf(TEXT("%.0f"), Effect.ResourceCost));
	}

	// Replace modifier placeholders
	for (const FAttributeModifier& Mod : Skill.Modifiers)
	{
		float Value = Mod.bPerRank ? Mod.Value * FMath::Max(CurrentRank, 1) : Mod.Value;
		FString Placeholder = FString::Printf(TEXT("{%s}"), *GetAttributeDisplayName(Mod.Attribute).ToString());
		Description = Description.Replace(*Placeholder, *FString::Printf(TEXT("%.0f"), Value));
	}

	return FText::FromString(Description);
}

FLinearColor USkillTreeLibrary::GetStatusColor(ESkillUnlockStatus Status)
{
	switch (Status)
	{
	case ESkillUnlockStatus::Locked: return FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);
	case ESkillUnlockStatus::Available: return FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
	case ESkillUnlockStatus::Unlocked: return FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);
	case ESkillUnlockStatus::Maxed: return FLinearColor(1.0f, 0.84f, 0.0f, 1.0f);
	default: return FLinearColor::White;
	}
}

FLinearColor USkillTreeLibrary::GetNodeTypeColor(ESkillNodeType NodeType)
{
	switch (NodeType)
	{
	case ESkillNodeType::Passive: return FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	case ESkillNodeType::Active: return FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
	case ESkillNodeType::Ultimate: return FLinearColor(0.8f, 0.2f, 0.8f, 1.0f);
	case ESkillNodeType::Mastery: return FLinearColor(1.0f, 0.84f, 0.0f, 1.0f);
	case ESkillNodeType::Perk: return FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);
	default: return FLinearColor::White;
	}
}

// ============================================
// SAVE/LOAD
// ============================================

FString USkillTreeLibrary::SaveSkillProgressToJSON(const FPlayerSkillData& PlayerSkills)
{
	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();

	RootObject->SetNumberField(TEXT("available_points"), PlayerSkills.AvailablePoints);
	RootObject->SetNumberField(TEXT("total_earned"), PlayerSkills.TotalPointsEarned);

	TArray<TSharedPtr<FJsonValue>> ProgressArray;
	for (const FSkillProgress& Progress : PlayerSkills.SkillProgress)
	{
		TSharedRef<FJsonObject> ProgressObj = MakeShared<FJsonObject>();
		ProgressObj->SetStringField(TEXT("skill_id"), Progress.SkillID);
		ProgressObj->SetNumberField(TEXT("rank"), Progress.CurrentRank);
		ProgressObj->SetStringField(TEXT("unlock_time"), Progress.UnlockTime.ToIso8601());
		ProgressArray.Add(MakeShared<FJsonValueObject>(ProgressObj));
	}
	RootObject->SetArrayField(TEXT("progress"), ProgressArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject, Writer);

	return OutputString;
}

bool USkillTreeLibrary::LoadSkillProgressFromJSON(const FString& JSONString, FPlayerSkillData& OutPlayerSkills)
{
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		return false;
	}

	OutPlayerSkills = FPlayerSkillData();
	OutPlayerSkills.AvailablePoints = RootObject->GetIntegerField(TEXT("available_points"));
	OutPlayerSkills.TotalPointsEarned = RootObject->GetIntegerField(TEXT("total_earned"));

	const TArray<TSharedPtr<FJsonValue>>* ProgressArray;
	if (RootObject->TryGetArrayField(TEXT("progress"), ProgressArray))
	{
		for (const TSharedPtr<FJsonValue>& ProgressValue : *ProgressArray)
		{
			const TSharedPtr<FJsonObject>* ProgressObject;
			if (ProgressValue->TryGetObject(ProgressObject))
			{
				FSkillProgress Progress;
				Progress.SkillID = (*ProgressObject)->GetStringField(TEXT("skill_id"));
				Progress.CurrentRank = (*ProgressObject)->GetIntegerField(TEXT("rank"));
				FDateTime::ParseIso8601(*(*ProgressObject)->GetStringField(TEXT("unlock_time")), Progress.UnlockTime);
				OutPlayerSkills.SkillProgress.Add(Progress);
			}
		}
	}

	return true;
}

// ============================================
// PRESETS & BUILDS
// ============================================

TArray<FString> USkillTreeLibrary::GetRecommendedSkills(const FString& ClassName, int32 PlayerLevel)
{
	TArray<FString> Recommended;

	// Basic recommendations based on class
	if (ClassName == TEXT("Warrior") || ClassName == TEXT("Krieger"))
	{
		Recommended.Add(TEXT("combat_strength_1"));
		Recommended.Add(TEXT("combat_vitality_1"));
		Recommended.Add(TEXT("combat_armor_1"));
		if (PlayerLevel >= 10)
		{
			Recommended.Add(TEXT("combat_power_strike"));
			Recommended.Add(TEXT("combat_shield_bash"));
		}
	}
	else if (ClassName == TEXT("Mage") || ClassName == TEXT("Magier"))
	{
		Recommended.Add(TEXT("magic_intellect_1"));
		Recommended.Add(TEXT("magic_mana_1"));
		Recommended.Add(TEXT("magic_focus_1"));
		if (PlayerLevel >= 10)
		{
			Recommended.Add(TEXT("magic_fireball"));
			Recommended.Add(TEXT("magic_frost_nova"));
		}
	}
	else if (ClassName == TEXT("Rogue") || ClassName == TEXT("Schurke"))
	{
		Recommended.Add(TEXT("stealth_agility_1"));
		Recommended.Add(TEXT("stealth_precision_1"));
		Recommended.Add(TEXT("stealth_evasion_1"));
		if (PlayerLevel >= 10)
		{
			Recommended.Add(TEXT("stealth_backstab"));
			Recommended.Add(TEXT("stealth_vanish"));
		}
	}

	return Recommended;
}

bool USkillTreeLibrary::ApplySkillPreset(
	const TArray<FString>& SkillIDsToUnlock,
	FPlayerSkillData& PlayerSkills,
	int32 PlayerLevel,
	const TMap<EAttributeType, float>& PlayerAttributes)
{
	// Sort skills by dependency order first
	TArray<FString> SortedSkills;
	TSet<FString> Added;

	while (SortedSkills.Num() < SkillIDsToUnlock.Num())
	{
		bool bAddedAny = false;

		for (const FString& SkillID : SkillIDsToUnlock)
		{
			if (Added.Contains(SkillID)) continue;

			FSkillNode Skill;
			if (!GetSkillNode(SkillID, Skill)) continue;

			// Check if all requirements are satisfied or will be unlocked
			bool bCanAdd = true;
			for (const FSkillRequirement& Req : Skill.Requirements)
			{
				if (!Req.RequiredSkillID.IsEmpty())
				{
					if (!Added.Contains(Req.RequiredSkillID) && !PlayerSkills.IsSkillUnlocked(Req.RequiredSkillID))
					{
						bCanAdd = false;
						break;
					}
				}
			}

			if (bCanAdd)
			{
				SortedSkills.Add(SkillID);
				Added.Add(SkillID);
				bAddedAny = true;
			}
		}

		if (!bAddedAny)
		{
			// Circular dependency or unsatisfiable requirements
			break;
		}
	}

	// Apply in order
	for (const FString& SkillID : SortedSkills)
	{
		FText Reason;
		while (CanUnlockSkill(SkillID, PlayerSkills, PlayerLevel, PlayerAttributes, Reason))
		{
			FSkillUnlockResult Result = UnlockSkill(SkillID, PlayerSkills, PlayerLevel, PlayerAttributes);
			if (!Result.bSuccess) break;
		}
	}

	return true;
}
