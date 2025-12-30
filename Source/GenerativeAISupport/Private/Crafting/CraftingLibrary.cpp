// Crafting Library - Implementation
// Part of GenerativeAISupport Plugin

#include "Crafting/CraftingLibrary.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

// Static members
TMap<FString, FCraftingRecipe> UCraftingLibrary::RecipeRegistry;
TMap<ECraftingStation, FCraftingStationInfo> UCraftingLibrary::StationRegistry;
TMap<FString, FString> UCraftingLibrary::RecipeItemMapping;

// ============================================
// RECIPE REGISTRY
// ============================================

int32 UCraftingLibrary::LoadRecipesFromJSON(const FString& FilePath)
{
	InitializeDefaultStations();

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("CraftingLibrary: Failed to load recipes from %s"), *FilePath);
		return 0;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("CraftingLibrary: Failed to parse JSON"));
		return 0;
	}

	int32 LoadedCount = 0;
	const TArray<TSharedPtr<FJsonValue>>* RecipesArray;
	if (JsonObject->TryGetArrayField(TEXT("recipes"), RecipesArray))
	{
		for (const TSharedPtr<FJsonValue>& RecipeValue : *RecipesArray)
		{
			TSharedPtr<FJsonObject> RecipeObj = RecipeValue->AsObject();
			if (!RecipeObj.IsValid()) continue;

			FCraftingRecipe Recipe;
			Recipe.RecipeID = RecipeObj->GetStringField(TEXT("id"));
			Recipe.DisplayName = FText::FromString(RecipeObj->GetStringField(TEXT("name")));
			Recipe.Description = FText::FromString(RecipeObj->GetStringField(TEXT("description")));

			// Category
			FString CategoryStr = RecipeObj->GetStringField(TEXT("category"));
			if (CategoryStr == "weapons") Recipe.Category = ECraftingCategory::Weapons;
			else if (CategoryStr == "armor") Recipe.Category = ECraftingCategory::Armor;
			else if (CategoryStr == "consumables") Recipe.Category = ECraftingCategory::Consumables;
			else if (CategoryStr == "materials") Recipe.Category = ECraftingCategory::Materials;
			else if (CategoryStr == "tools") Recipe.Category = ECraftingCategory::Tools;
			else if (CategoryStr == "jewelry") Recipe.Category = ECraftingCategory::Jewelry;
			else if (CategoryStr == "food") Recipe.Category = ECraftingCategory::Food;
			else if (CategoryStr == "potions") Recipe.Category = ECraftingCategory::Potions;

			// Station
			FString StationStr = RecipeObj->GetStringField(TEXT("station"));
			if (StationStr == "workbench") Recipe.RequiredStation = ECraftingStation::Workbench;
			else if (StationStr == "forge") Recipe.RequiredStation = ECraftingStation::Forge;
			else if (StationStr == "anvil") Recipe.RequiredStation = ECraftingStation::Anvil;
			else if (StationStr == "alchemy") Recipe.RequiredStation = ECraftingStation::Alchemy;
			else if (StationStr == "cooking") Recipe.RequiredStation = ECraftingStation::Cooking;
			else if (StationStr == "tanning") Recipe.RequiredStation = ECraftingStation::Tanning;
			else Recipe.RequiredStation = ECraftingStation::None;

			// Difficulty
			FString DiffStr = RecipeObj->GetStringField(TEXT("difficulty"));
			if (DiffStr == "trivial") Recipe.Difficulty = ECraftingDifficulty::Trivial;
			else if (DiffStr == "easy") Recipe.Difficulty = ECraftingDifficulty::Easy;
			else if (DiffStr == "normal") Recipe.Difficulty = ECraftingDifficulty::Normal;
			else if (DiffStr == "hard") Recipe.Difficulty = ECraftingDifficulty::Hard;
			else if (DiffStr == "expert") Recipe.Difficulty = ECraftingDifficulty::Expert;
			else if (DiffStr == "master") Recipe.Difficulty = ECraftingDifficulty::Master;
			else if (DiffStr == "legendary") Recipe.Difficulty = ECraftingDifficulty::Legendary;

			Recipe.RequiredSkillLevel = RecipeObj->GetIntegerField(TEXT("required_level"));
			Recipe.SkillXPGain = RecipeObj->GetIntegerField(TEXT("xp_gain"));
			Recipe.CraftingTime = RecipeObj->GetNumberField(TEXT("crafting_time"));
			Recipe.GoldCost = RecipeObj->GetIntegerField(TEXT("gold_cost"));
			Recipe.bHidden = RecipeObj->GetBoolField(TEXT("hidden"));
			Recipe.RequiredQuest = RecipeObj->GetStringField(TEXT("required_quest"));
			Recipe.RequiredFaction = RecipeObj->GetStringField(TEXT("required_faction"));
			Recipe.RequiredReputation = RecipeObj->GetIntegerField(TEXT("required_reputation"));
			Recipe.IconPath = RecipeObj->GetStringField(TEXT("icon"));

			// Ingredients
			const TArray<TSharedPtr<FJsonValue>>* IngredientsArray;
			if (RecipeObj->TryGetArrayField(TEXT("ingredients"), IngredientsArray))
			{
				for (const TSharedPtr<FJsonValue>& IngValue : *IngredientsArray)
				{
					TSharedPtr<FJsonObject> IngObj = IngValue->AsObject();
					if (!IngObj.IsValid()) continue;

					FCraftingIngredient Ingredient;
					Ingredient.ItemID = IngObj->GetStringField(TEXT("item"));
					Ingredient.Quantity = IngObj->GetIntegerField(TEXT("quantity"));
					Ingredient.bOptional = IngObj->GetBoolField(TEXT("optional"));
					Ingredient.BonusEffect = IngObj->GetStringField(TEXT("bonus"));
					Recipe.Ingredients.Add(Ingredient);
				}
			}

			// Outputs
			const TArray<TSharedPtr<FJsonValue>>* OutputsArray;
			if (RecipeObj->TryGetArrayField(TEXT("outputs"), OutputsArray))
			{
				for (const TSharedPtr<FJsonValue>& OutValue : *OutputsArray)
				{
					TSharedPtr<FJsonObject> OutObj = OutValue->AsObject();
					if (!OutObj.IsValid()) continue;

					FCraftingOutput Output;
					Output.ItemID = OutObj->GetStringField(TEXT("item"));
					Output.Quantity = OutObj->GetIntegerField(TEXT("quantity"));
					Output.CriticalBonusQuantity = OutObj->GetIntegerField(TEXT("crit_bonus"));
					Recipe.Outputs.Add(Output);
				}
			}

			// Tags
			const TArray<TSharedPtr<FJsonValue>>* TagsArray;
			if (RecipeObj->TryGetArrayField(TEXT("tags"), TagsArray))
			{
				for (const TSharedPtr<FJsonValue>& TagValue : *TagsArray)
				{
					Recipe.Tags.Add(TagValue->AsString());
				}
			}

			// Recipe scroll mapping
			FString ScrollItemID = RecipeObj->GetStringField(TEXT("scroll_item"));
			if (!ScrollItemID.IsEmpty())
			{
				RecipeItemMapping.Add(ScrollItemID, Recipe.RecipeID);
			}

			RecipeRegistry.Add(Recipe.RecipeID, Recipe);
			LoadedCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("CraftingLibrary: Loaded %d recipes from %s"), LoadedCount, *FilePath);
	return LoadedCount;
}

void UCraftingLibrary::RegisterRecipe(const FCraftingRecipe& Recipe)
{
	RecipeRegistry.Add(Recipe.RecipeID, Recipe);
}

bool UCraftingLibrary::GetRecipe(const FString& RecipeID, FCraftingRecipe& OutRecipe)
{
	if (FCraftingRecipe* Found = RecipeRegistry.Find(RecipeID))
	{
		OutRecipe = *Found;
		return true;
	}
	return false;
}

TArray<FCraftingRecipe> UCraftingLibrary::GetAllRecipes()
{
	TArray<FCraftingRecipe> Result;
	RecipeRegistry.GenerateValueArray(Result);
	return Result;
}

TArray<FCraftingRecipe> UCraftingLibrary::GetRecipesByCategory(ECraftingCategory Category)
{
	TArray<FCraftingRecipe> Result;
	for (const auto& Pair : RecipeRegistry)
	{
		if (Pair.Value.Category == Category)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

TArray<FCraftingRecipe> UCraftingLibrary::GetRecipesByStation(ECraftingStation Station)
{
	TArray<FCraftingRecipe> Result;
	for (const auto& Pair : RecipeRegistry)
	{
		if (Pair.Value.RequiredStation == Station)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

TArray<FCraftingRecipe> UCraftingLibrary::GetAvailableRecipes(
	ECraftingStation Station,
	const TArray<FCraftingSkill>& PlayerSkills,
	const FKnownRecipes& KnownRecipes,
	bool bIncludeHidden)
{
	TArray<FCraftingRecipe> Result;
	for (const auto& Pair : RecipeRegistry)
	{
		const FCraftingRecipe& Recipe = Pair.Value;

		// Check station
		if (Recipe.RequiredStation != Station && Recipe.RequiredStation != ECraftingStation::None)
			continue;

		// Check if known (or not hidden)
		if (Recipe.bHidden && !bIncludeHidden && !KnownRecipes.IsKnown(Recipe.RecipeID))
			continue;

		// Check skill level
		int32 SkillLevel = GetSkillLevel(Recipe.Category, PlayerSkills);
		if (SkillLevel < Recipe.RequiredSkillLevel)
			continue;

		Result.Add(Recipe);
	}
	return Result;
}

TArray<FCraftingRecipe> UCraftingLibrary::SearchRecipes(const FString& SearchTerm)
{
	TArray<FCraftingRecipe> Result;
	FString LowerSearch = SearchTerm.ToLower();

	for (const auto& Pair : RecipeRegistry)
	{
		const FCraftingRecipe& Recipe = Pair.Value;

		// Search in name
		if (Recipe.DisplayName.ToString().ToLower().Contains(LowerSearch))
		{
			Result.Add(Recipe);
			continue;
		}

		// Search in tags
		for (const FString& Tag : Recipe.Tags)
		{
			if (Tag.ToLower().Contains(LowerSearch))
			{
				Result.Add(Recipe);
				break;
			}
		}
	}
	return Result;
}

// ============================================
// CRAFTING OPERATIONS
// ============================================

bool UCraftingLibrary::CanCraft(
	const FString& RecipeID,
	const TMap<FString, int32>& PlayerInventory,
	int32 PlayerGold,
	const TArray<FCraftingSkill>& PlayerSkills,
	const FKnownRecipes& KnownRecipes,
	ECraftingStation AvailableStation,
	FText& OutReason)
{
	FCraftingRecipe Recipe;
	if (!GetRecipe(RecipeID, Recipe))
	{
		OutReason = FText::FromString(TEXT("Rezept unbekannt"));
		return false;
	}

	// Check if recipe is known (if hidden)
	if (Recipe.bHidden && !KnownRecipes.IsKnown(RecipeID))
	{
		OutReason = FText::FromString(TEXT("Rezept noch nicht gelernt"));
		return false;
	}

	// Check station
	if (Recipe.RequiredStation != ECraftingStation::None && Recipe.RequiredStation != AvailableStation)
	{
		OutReason = FText::Format(
			FText::FromString(TEXT("Benötigt: {0}")),
			GetStationDisplayName(Recipe.RequiredStation));
		return false;
	}

	// Check skill level
	int32 SkillLevel = GetSkillLevel(Recipe.Category, PlayerSkills);
	if (SkillLevel < Recipe.RequiredSkillLevel)
	{
		OutReason = FText::Format(
			FText::FromString(TEXT("Benötigt {0} Stufe {1}")),
			GetCategoryDisplayName(Recipe.Category),
			FText::AsNumber(Recipe.RequiredSkillLevel));
		return false;
	}

	// Check gold
	if (PlayerGold < Recipe.GoldCost)
	{
		OutReason = FText::Format(
			FText::FromString(TEXT("Benötigt {0} Gold")),
			FText::AsNumber(Recipe.GoldCost));
		return false;
	}

	// Check materials
	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		if (Ingredient.bOptional) continue;

		const int32* PlayerCount = PlayerInventory.Find(Ingredient.ItemID);
		int32 HasCount = PlayerCount ? *PlayerCount : 0;

		if (HasCount < Ingredient.Quantity)
		{
			OutReason = FText::Format(
				FText::FromString(TEXT("Fehlt: {0} x{1}")),
				FText::FromString(Ingredient.ItemID),
				FText::AsNumber(Ingredient.Quantity - HasCount));
			return false;
		}
	}

	return true;
}

FCraftingAttemptResult UCraftingLibrary::CraftItem(
	const FString& RecipeID,
	TMap<FString, int32>& PlayerInventory,
	int32& PlayerGold,
	TArray<FCraftingSkill>& PlayerSkills,
	const FKnownRecipes& KnownRecipes,
	ECraftingStation AvailableStation,
	float LuckBonus)
{
	FCraftingAttemptResult Result;
	Result.RecipeID = RecipeID;

	FText Reason;
	if (!CanCraft(RecipeID, PlayerInventory, PlayerGold, PlayerSkills, KnownRecipes, AvailableStation, Reason))
	{
		Result.Result = ECraftingResult::MissingMaterials;
		Result.ErrorMessage = Reason;
		return Result;
	}

	FCraftingRecipe Recipe;
	GetRecipe(RecipeID, Recipe);

	// Consume materials
	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		if (Ingredient.bOptional) continue;

		int32& Count = PlayerInventory.FindOrAdd(Ingredient.ItemID);
		Count -= Ingredient.Quantity;
		if (Count <= 0)
		{
			PlayerInventory.Remove(Ingredient.ItemID);
		}
	}

	// Consume gold
	PlayerGold -= Recipe.GoldCost;

	// Calculate success
	float SuccessChance = CalculateSuccessChance(Recipe, PlayerSkills);
	float CritChance = CalculateCriticalChance(Recipe, PlayerSkills, 0.0f, LuckBonus);

	float Roll = FMath::FRand();

	if (Roll > SuccessChance)
	{
		// Failure
		float CritFailRoll = FMath::FRand();
		if (CritFailRoll < 0.1f) // 10% chance of critical failure
		{
			Result.Result = ECraftingResult::CriticalFailure;
			Result.ErrorMessage = FText::FromString(TEXT("Kritischer Fehlschlag! Materialien verloren."));
		}
		else
		{
			Result.Result = ECraftingResult::Failure;
			Result.ErrorMessage = FText::FromString(TEXT("Herstellung fehlgeschlagen."));

			// Return 50% materials on normal failure
			for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
			{
				if (Ingredient.bOptional) continue;
				int32 ReturnAmount = Ingredient.Quantity / 2;
				if (ReturnAmount > 0)
				{
					PlayerInventory.FindOrAdd(Ingredient.ItemID) += ReturnAmount;
				}
			}
		}

		// Still gain some XP on failure
		Result.XPGained = Recipe.SkillXPGain / 4;
	}
	else
	{
		// Success!
		bool bCritical = FMath::FRand() < CritChance;
		Result.bCritical = bCritical;

		if (bCritical)
		{
			Result.Result = ECraftingResult::CriticalSuccess;
			Result.XPGained = Recipe.SkillXPGain * 2;
		}
		else
		{
			Result.Result = ECraftingResult::Success;
			Result.XPGained = Recipe.SkillXPGain;
		}

		// Add outputs
		for (const FCraftingOutput& Output : Recipe.Outputs)
		{
			FCraftingOutput ProducedItem = Output;
			if (bCritical)
			{
				ProducedItem.Quantity += Output.CriticalBonusQuantity;
			}
			Result.ProducedItems.Add(ProducedItem);

			// Add to inventory
			PlayerInventory.FindOrAdd(Output.ItemID) += ProducedItem.Quantity;
		}
	}

	// Add skill XP
	int32 NewLevel;
	bool bLeveledUp;
	AddSkillXP(Recipe.Category, Result.XPGained, PlayerSkills, NewLevel, bLeveledUp);

	return Result;
}

TArray<FCraftingAttemptResult> UCraftingLibrary::CraftMultiple(
	const FString& RecipeID,
	int32 Quantity,
	TMap<FString, int32>& PlayerInventory,
	int32& PlayerGold,
	TArray<FCraftingSkill>& PlayerSkills,
	const FKnownRecipes& KnownRecipes,
	ECraftingStation AvailableStation,
	float LuckBonus)
{
	TArray<FCraftingAttemptResult> Results;

	for (int32 i = 0; i < Quantity; i++)
	{
		FCraftingAttemptResult Result = CraftItem(
			RecipeID, PlayerInventory, PlayerGold, PlayerSkills,
			KnownRecipes, AvailableStation, LuckBonus);

		Results.Add(Result);

		// Stop if we can't craft anymore
		if (Result.Result == ECraftingResult::MissingMaterials ||
			Result.Result == ECraftingResult::InsufficientSkill)
		{
			break;
		}
	}

	return Results;
}

int32 UCraftingLibrary::GetMaxCraftableQuantity(
	const FString& RecipeID,
	const TMap<FString, int32>& PlayerInventory,
	int32 PlayerGold)
{
	FCraftingRecipe Recipe;
	if (!GetRecipe(RecipeID, Recipe))
	{
		return 0;
	}

	int32 MaxByMaterials = INT32_MAX;
	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		if (Ingredient.bOptional) continue;

		const int32* PlayerCount = PlayerInventory.Find(Ingredient.ItemID);
		int32 HasCount = PlayerCount ? *PlayerCount : 0;

		int32 CanMake = HasCount / Ingredient.Quantity;
		MaxByMaterials = FMath::Min(MaxByMaterials, CanMake);
	}

	int32 MaxByGold = Recipe.GoldCost > 0 ? PlayerGold / Recipe.GoldCost : INT32_MAX;

	return FMath::Min(MaxByMaterials, MaxByGold);
}

float UCraftingLibrary::CalculateSuccessChance(
	const FCraftingRecipe& Recipe,
	const TArray<FCraftingSkill>& PlayerSkills,
	float StationBonus)
{
	int32 SkillLevel = GetSkillLevel(Recipe.Category, PlayerSkills);
	int32 LevelDiff = SkillLevel - Recipe.RequiredSkillLevel;

	// Base chance based on difficulty
	float BaseChance = 0.0f;
	switch (Recipe.Difficulty)
	{
	case ECraftingDifficulty::Trivial: BaseChance = 1.0f; break;
	case ECraftingDifficulty::Easy: BaseChance = 0.95f; break;
	case ECraftingDifficulty::Normal: BaseChance = 0.85f; break;
	case ECraftingDifficulty::Hard: BaseChance = 0.70f; break;
	case ECraftingDifficulty::Expert: BaseChance = 0.55f; break;
	case ECraftingDifficulty::Master: BaseChance = 0.40f; break;
	case ECraftingDifficulty::Legendary: BaseChance = 0.25f; break;
	}

	// Bonus from skill level difference (+5% per level above required)
	float SkillBonus = LevelDiff * 0.05f;

	return FMath::Clamp(BaseChance + SkillBonus + StationBonus, 0.05f, 1.0f);
}

float UCraftingLibrary::CalculateCriticalChance(
	const FCraftingRecipe& Recipe,
	const TArray<FCraftingSkill>& PlayerSkills,
	float StationBonus,
	float LuckBonus)
{
	int32 SkillLevel = GetSkillLevel(Recipe.Category, PlayerSkills);
	int32 LevelDiff = SkillLevel - Recipe.RequiredSkillLevel;

	// Base 5% crit chance, +2% per level above required
	float CritChance = 0.05f + (LevelDiff * 0.02f) + StationBonus + LuckBonus;

	return FMath::Clamp(CritChance, 0.01f, 0.5f);
}

// ============================================
// RECIPE LEARNING
// ============================================

bool UCraftingLibrary::LearnRecipe(const FString& RecipeID, FKnownRecipes& KnownRecipes)
{
	if (KnownRecipes.IsKnown(RecipeID))
	{
		return false; // Already known
	}

	FCraftingRecipe Recipe;
	if (!GetRecipe(RecipeID, Recipe))
	{
		return false; // Recipe doesn't exist
	}

	KnownRecipes.LearnRecipe(RecipeID);
	return true;
}

bool UCraftingLibrary::LearnRecipeFromItem(
	const FString& ItemID,
	FKnownRecipes& KnownRecipes,
	TMap<FString, int32>& PlayerInventory,
	FString& OutLearnedRecipeID)
{
	// Check if item teaches a recipe
	FString* RecipeID = RecipeItemMapping.Find(ItemID);
	if (!RecipeID)
	{
		return false;
	}

	// Check if player has item
	int32* ItemCount = PlayerInventory.Find(ItemID);
	if (!ItemCount || *ItemCount < 1)
	{
		return false;
	}

	// Check if already known
	if (KnownRecipes.IsKnown(*RecipeID))
	{
		return false;
	}

	// Consume item and learn recipe
	(*ItemCount)--;
	if (*ItemCount <= 0)
	{
		PlayerInventory.Remove(ItemID);
	}

	KnownRecipes.LearnRecipe(*RecipeID);
	OutLearnedRecipeID = *RecipeID;
	return true;
}

bool UCraftingLibrary::IsRecipeKnown(const FString& RecipeID, const FKnownRecipes& KnownRecipes)
{
	return KnownRecipes.IsKnown(RecipeID);
}

TArray<FCraftingRecipe> UCraftingLibrary::GetTrainerRecipes(
	const FString& TrainerID,
	const TArray<FCraftingSkill>& PlayerSkills,
	const FKnownRecipes& KnownRecipes)
{
	// TODO: Implement trainer-specific recipe lists
	return TArray<FCraftingRecipe>();
}

// ============================================
// SKILL MANAGEMENT
// ============================================

int32 UCraftingLibrary::GetSkillLevel(ECraftingCategory Category, const TArray<FCraftingSkill>& Skills)
{
	for (const FCraftingSkill& Skill : Skills)
	{
		if (Skill.Category == Category)
		{
			return Skill.Level;
		}
	}
	return 1; // Default level
}

bool UCraftingLibrary::AddSkillXP(
	ECraftingCategory Category,
	int32 XPAmount,
	TArray<FCraftingSkill>& Skills,
	int32& OutNewLevel,
	bool& OutLeveledUp)
{
	OutLeveledUp = false;

	for (FCraftingSkill& Skill : Skills)
	{
		if (Skill.Category == Category)
		{
			Skill.CurrentXP += XPAmount;
			Skill.TotalCrafted++;

			// Check for level up
			while (Skill.CurrentXP >= Skill.XPToNextLevel && Skill.Level < 100)
			{
				Skill.CurrentXP -= Skill.XPToNextLevel;
				Skill.Level++;
				Skill.XPToNextLevel = GetXPRequiredForLevel(Skill.Level + 1);
				OutLeveledUp = true;
			}

			OutNewLevel = Skill.Level;
			return true;
		}
	}

	// Skill not found, create it
	FCraftingSkill NewSkill;
	NewSkill.Category = Category;
	NewSkill.Level = 1;
	NewSkill.CurrentXP = XPAmount;
	NewSkill.XPToNextLevel = GetXPRequiredForLevel(2);
	NewSkill.TotalCrafted = 1;

	while (NewSkill.CurrentXP >= NewSkill.XPToNextLevel && NewSkill.Level < 100)
	{
		NewSkill.CurrentXP -= NewSkill.XPToNextLevel;
		NewSkill.Level++;
		NewSkill.XPToNextLevel = GetXPRequiredForLevel(NewSkill.Level + 1);
		OutLeveledUp = true;
	}

	Skills.Add(NewSkill);
	OutNewLevel = NewSkill.Level;
	return true;
}

float UCraftingLibrary::GetSkillProgress(ECraftingCategory Category, const TArray<FCraftingSkill>& Skills)
{
	for (const FCraftingSkill& Skill : Skills)
	{
		if (Skill.Category == Category)
		{
			if (Skill.XPToNextLevel <= 0) return 1.0f;
			return (float)Skill.CurrentXP / (float)Skill.XPToNextLevel;
		}
	}
	return 0.0f;
}

TArray<FCraftingSkill> UCraftingLibrary::CreateDefaultSkills()
{
	TArray<FCraftingSkill> Skills;

	TArray<ECraftingCategory> Categories = {
		ECraftingCategory::Weapons,
		ECraftingCategory::Armor,
		ECraftingCategory::Consumables,
		ECraftingCategory::Materials,
		ECraftingCategory::Tools,
		ECraftingCategory::Jewelry,
		ECraftingCategory::Food,
		ECraftingCategory::Potions
	};

	for (ECraftingCategory Category : Categories)
	{
		FCraftingSkill Skill;
		Skill.Category = Category;
		Skill.Level = 1;
		Skill.CurrentXP = 0;
		Skill.XPToNextLevel = GetXPRequiredForLevel(2);
		Skill.TotalCrafted = 0;
		Skills.Add(Skill);
	}

	return Skills;
}

int32 UCraftingLibrary::GetXPRequiredForLevel(int32 Level)
{
	// Exponential scaling: 100 * 1.5^(Level-1)
	return FMath::RoundToInt(100.0f * FMath::Pow(1.5f, Level - 1));
}

// ============================================
// MATERIAL REQUIREMENTS
// ============================================

TArray<FCraftingIngredient> UCraftingLibrary::GetRequiredMaterials(const FString& RecipeID)
{
	FCraftingRecipe Recipe;
	if (GetRecipe(RecipeID, Recipe))
	{
		return Recipe.Ingredients;
	}
	return TArray<FCraftingIngredient>();
}

bool UCraftingLibrary::HasRequiredMaterials(
	const FString& RecipeID,
	const TMap<FString, int32>& PlayerInventory,
	TArray<FCraftingIngredient>& OutMissingMaterials)
{
	OutMissingMaterials.Empty();

	FCraftingRecipe Recipe;
	if (!GetRecipe(RecipeID, Recipe))
	{
		return false;
	}

	bool bHasAll = true;
	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		if (Ingredient.bOptional) continue;

		const int32* PlayerCount = PlayerInventory.Find(Ingredient.ItemID);
		int32 HasCount = PlayerCount ? *PlayerCount : 0;

		if (HasCount < Ingredient.Quantity)
		{
			FCraftingIngredient Missing = Ingredient;
			Missing.Quantity = Ingredient.Quantity - HasCount;
			OutMissingMaterials.Add(Missing);
			bHasAll = false;
		}
	}

	return bHasAll;
}

TMap<FString, FString> UCraftingLibrary::GetMaterialSources(const FString& RecipeID)
{
	// TODO: Implement material source lookup
	return TMap<FString, FString>();
}

// ============================================
// STATION MANAGEMENT
// ============================================

void UCraftingLibrary::InitializeDefaultStations()
{
	if (StationRegistry.Num() > 0) return;

	auto AddStation = [](ECraftingStation Type, const FString& Name, const FString& Desc, TArray<ECraftingCategory> Cats)
	{
		FCraftingStationInfo Info;
		Info.StationType = Type;
		Info.DisplayName = FText::FromString(Name);
		Info.Description = FText::FromString(Desc);
		Info.SpeedMultiplier = 1.0f;
		Info.SuccessBonus = 0.0f;
		Info.CriticalBonus = 0.0f;
		Info.SupportedCategories = Cats;
		StationRegistry.Add(Type, Info);
	};

	AddStation(ECraftingStation::Workbench, TEXT("Werkbank"), TEXT("Basis-Werkbank für einfache Gegenstände"),
		{ECraftingCategory::Materials, ECraftingCategory::Tools});

	AddStation(ECraftingStation::Forge, TEXT("Schmiede"), TEXT("Heiße Schmiede für Metallarbeiten"),
		{ECraftingCategory::Weapons, ECraftingCategory::Armor, ECraftingCategory::Tools});

	AddStation(ECraftingStation::Anvil, TEXT("Amboss"), TEXT("Schwerer Amboss für Präzisionsarbeit"),
		{ECraftingCategory::Weapons, ECraftingCategory::Armor});

	AddStation(ECraftingStation::Alchemy, TEXT("Alchemietisch"), TEXT("Mystischer Tisch für Tränke"),
		{ECraftingCategory::Potions, ECraftingCategory::Consumables});

	AddStation(ECraftingStation::Cooking, TEXT("Kochstelle"), TEXT("Feuer zum Kochen"),
		{ECraftingCategory::Food, ECraftingCategory::Consumables});

	AddStation(ECraftingStation::Tanning, TEXT("Gerbergestell"), TEXT("Rahmen zum Gerben von Leder"),
		{ECraftingCategory::Armor, ECraftingCategory::Materials});

	AddStation(ECraftingStation::Loom, TEXT("Webstuhl"), TEXT("Webstuhl für Stoffe"),
		{ECraftingCategory::Armor, ECraftingCategory::Materials});

	AddStation(ECraftingStation::Jeweler, TEXT("Juwelierbank"), TEXT("Feine Werkbank für Schmuck"),
		{ECraftingCategory::Jewelry});

	AddStation(ECraftingStation::Enchanting, TEXT("Verzauberungstisch"), TEXT("Magischer Tisch für Verzauberungen"),
		{ECraftingCategory::Enchantments});
}

void UCraftingLibrary::RegisterStation(const FCraftingStationInfo& StationInfo)
{
	StationRegistry.Add(StationInfo.StationType, StationInfo);
}

FCraftingStationInfo UCraftingLibrary::GetStationInfo(ECraftingStation Station)
{
	InitializeDefaultStations();
	if (FCraftingStationInfo* Info = StationRegistry.Find(Station))
	{
		return *Info;
	}
	return FCraftingStationInfo();
}

FText UCraftingLibrary::GetStationDisplayName(ECraftingStation Station)
{
	FCraftingStationInfo Info = GetStationInfo(Station);
	return Info.DisplayName;
}

// ============================================
// UTILITIES
// ============================================

FText UCraftingLibrary::GetCategoryDisplayName(ECraftingCategory Category)
{
	switch (Category)
	{
	case ECraftingCategory::Weapons: return FText::FromString(TEXT("Waffen"));
	case ECraftingCategory::Armor: return FText::FromString(TEXT("Rüstung"));
	case ECraftingCategory::Consumables: return FText::FromString(TEXT("Verbrauchsgüter"));
	case ECraftingCategory::Materials: return FText::FromString(TEXT("Materialien"));
	case ECraftingCategory::Tools: return FText::FromString(TEXT("Werkzeuge"));
	case ECraftingCategory::Jewelry: return FText::FromString(TEXT("Schmuck"));
	case ECraftingCategory::Furniture: return FText::FromString(TEXT("Möbel"));
	case ECraftingCategory::Enchantments: return FText::FromString(TEXT("Verzauberungen"));
	case ECraftingCategory::Food: return FText::FromString(TEXT("Essen"));
	case ECraftingCategory::Potions: return FText::FromString(TEXT("Tränke"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FText UCraftingLibrary::GetDifficultyDisplayName(ECraftingDifficulty Difficulty)
{
	switch (Difficulty)
	{
	case ECraftingDifficulty::Trivial: return FText::FromString(TEXT("Trivial"));
	case ECraftingDifficulty::Easy: return FText::FromString(TEXT("Einfach"));
	case ECraftingDifficulty::Normal: return FText::FromString(TEXT("Normal"));
	case ECraftingDifficulty::Hard: return FText::FromString(TEXT("Schwer"));
	case ECraftingDifficulty::Expert: return FText::FromString(TEXT("Experte"));
	case ECraftingDifficulty::Master: return FText::FromString(TEXT("Meister"));
	case ECraftingDifficulty::Legendary: return FText::FromString(TEXT("Legendär"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FLinearColor UCraftingLibrary::GetDifficultyColor(ECraftingDifficulty Difficulty)
{
	switch (Difficulty)
	{
	case ECraftingDifficulty::Trivial: return FLinearColor(0.5f, 0.5f, 0.5f); // Gray
	case ECraftingDifficulty::Easy: return FLinearColor(0.0f, 1.0f, 0.0f); // Green
	case ECraftingDifficulty::Normal: return FLinearColor(1.0f, 1.0f, 0.0f); // Yellow
	case ECraftingDifficulty::Hard: return FLinearColor(1.0f, 0.5f, 0.0f); // Orange
	case ECraftingDifficulty::Expert: return FLinearColor(1.0f, 0.0f, 0.0f); // Red
	case ECraftingDifficulty::Master: return FLinearColor(0.5f, 0.0f, 1.0f); // Purple
	case ECraftingDifficulty::Legendary: return FLinearColor(1.0f, 0.84f, 0.0f); // Gold
	default: return FLinearColor::White;
	}
}

FString UCraftingLibrary::FormatCraftingTime(float Seconds)
{
	if (Seconds <= 0.0f)
	{
		return TEXT("Sofort");
	}

	int32 TotalSeconds = FMath::RoundToInt(Seconds);
	int32 Hours = TotalSeconds / 3600;
	int32 Minutes = (TotalSeconds % 3600) / 60;
	int32 Secs = TotalSeconds % 60;

	if (Hours > 0)
	{
		return FString::Printf(TEXT("%d:%02d:%02d"), Hours, Minutes, Secs);
	}
	else if (Minutes > 0)
	{
		return FString::Printf(TEXT("%d:%02d"), Minutes, Secs);
	}
	else
	{
		return FString::Printf(TEXT("%d Sek."), Secs);
	}
}

// ============================================
// SAVE/LOAD
// ============================================

FString UCraftingLibrary::SaveCraftingProgressToJSON(
	const TArray<FCraftingSkill>& Skills,
	const FKnownRecipes& KnownRecipes)
{
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);

	// Save skills
	TArray<TSharedPtr<FJsonValue>> SkillsArray;
	for (const FCraftingSkill& Skill : Skills)
	{
		TSharedPtr<FJsonObject> SkillObj = MakeShareable(new FJsonObject);
		SkillObj->SetNumberField(TEXT("category"), (int32)Skill.Category);
		SkillObj->SetNumberField(TEXT("level"), Skill.Level);
		SkillObj->SetNumberField(TEXT("xp"), Skill.CurrentXP);
		SkillObj->SetNumberField(TEXT("total_crafted"), Skill.TotalCrafted);
		SkillsArray.Add(MakeShareable(new FJsonValueObject(SkillObj)));
	}
	RootObject->SetArrayField(TEXT("skills"), SkillsArray);

	// Save known recipes
	TArray<TSharedPtr<FJsonValue>> RecipesArray;
	for (const FString& RecipeID : KnownRecipes.RecipeIDs)
	{
		RecipesArray.Add(MakeShareable(new FJsonValueString(RecipeID)));
	}
	RootObject->SetArrayField(TEXT("known_recipes"), RecipesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	return OutputString;
}

bool UCraftingLibrary::LoadCraftingProgressFromJSON(
	const FString& JSONString,
	TArray<FCraftingSkill>& OutSkills,
	FKnownRecipes& OutKnownRecipes)
{
	OutSkills.Empty();
	OutKnownRecipes.RecipeIDs.Empty();

	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		return false;
	}

	// Load skills
	const TArray<TSharedPtr<FJsonValue>>* SkillsArray;
	if (RootObject->TryGetArrayField(TEXT("skills"), SkillsArray))
	{
		for (const TSharedPtr<FJsonValue>& SkillValue : *SkillsArray)
		{
			TSharedPtr<FJsonObject> SkillObj = SkillValue->AsObject();
			if (!SkillObj.IsValid()) continue;

			FCraftingSkill Skill;
			Skill.Category = (ECraftingCategory)SkillObj->GetIntegerField(TEXT("category"));
			Skill.Level = SkillObj->GetIntegerField(TEXT("level"));
			Skill.CurrentXP = SkillObj->GetIntegerField(TEXT("xp"));
			Skill.TotalCrafted = SkillObj->GetIntegerField(TEXT("total_crafted"));
			Skill.XPToNextLevel = GetXPRequiredForLevel(Skill.Level + 1);
			OutSkills.Add(Skill);
		}
	}

	// Load known recipes
	const TArray<TSharedPtr<FJsonValue>>* RecipesArray;
	if (RootObject->TryGetArrayField(TEXT("known_recipes"), RecipesArray))
	{
		for (const TSharedPtr<FJsonValue>& RecipeValue : *RecipesArray)
		{
			OutKnownRecipes.RecipeIDs.Add(RecipeValue->AsString());
		}
	}

	return true;
}
