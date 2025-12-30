// GenQuestUtils - Editor utilities for Quest creation and management
// Part of GenerativeAISupport Plugin
// Use for AI-driven quest authoring and batch operations

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Quest/QuestTypes.h"
#include "GenQuestUtils.generated.h"

/**
 * Quest Template for bulk creation
 */
USTRUCT(BlueprintType)
struct FQuestTemplate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TemplateName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EQuestType DefaultType = EQuestType::Side;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EObjectiveType> ObjectiveTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DefaultXPReward = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DefaultGoldReward = 50;
};

/**
 * Quest Chain Definition
 */
USTRUCT(BlueprintType)
struct FQuestChainEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString QuestID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FQuestObjective> Objectives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FQuestReward Rewards;
};

/**
 * GenQuestUtils - Editor utilities for AI-driven quest creation
 * Provides functions to create, modify, validate, and export quests
 */
UCLASS()
class GENERATIVEAISUPPORT_API UGenQuestUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// QUEST CREATION
	// ============================================

	/**
	 * Create a new quest and return its data
	 * @param QuestID - Unique ID for the quest
	 * @param Title - Quest title
	 * @param Description - Quest description
	 * @param QuestType - Type of quest (Main, Side, Daily, etc.)
	 * @param QuestGiverID - NPC who gives this quest
	 * @return The created quest data
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Creation")
	static FQuestData CreateQuest(
		const FString& QuestID,
		const FText& Title,
		const FText& Description,
		EQuestType QuestType = EQuestType::Side,
		const FString& QuestGiverID = TEXT(""));

	/**
	 * Create a quest from a JSON string
	 * @param JSONString - JSON definition of the quest
	 * @param OutQuest - The parsed quest
	 * @return True if parsing succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Creation")
	static bool CreateQuestFromJSON(const FString& JSONString, FQuestData& OutQuest);

	/**
	 * Create multiple quests from a JSON array
	 * @param JSONString - JSON array of quest definitions
	 * @return Array of created quests
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Creation")
	static TArray<FQuestData> CreateQuestsFromJSON(const FString& JSONString);

	// ============================================
	// OBJECTIVE MANAGEMENT
	// ============================================

	/**
	 * Add an objective to a quest
	 * @param Quest - The quest to modify (modified in place)
	 * @param ObjectiveID - Unique ID for this objective
	 * @param Type - Type of objective (Kill, Collect, etc.)
	 * @param Description - Objective description
	 * @param TargetID - Target identifier (monster type, item type, etc.)
	 * @param RequiredCount - How many to complete
	 * @param bOptional - Is this objective optional?
	 * @param bHidden - Is this objective hidden initially?
	 * @return The modified quest
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Objectives")
	static FQuestData AddObjective(
		UPARAM(ref) FQuestData& Quest,
		const FString& ObjectiveID,
		EObjectiveType Type,
		const FText& Description,
		const FString& TargetID,
		int32 RequiredCount = 1,
		bool bOptional = false,
		bool bHidden = false);

	/**
	 * Create a kill objective
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Objectives")
	static FQuestObjective MakeKillObjective(const FString& ObjectiveID, const FText& Description, const FString& MonsterID, int32 Count = 1);

	/**
	 * Create a collect objective
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Objectives")
	static FQuestObjective MakeCollectObjective(const FString& ObjectiveID, const FText& Description, const FString& ItemID, int32 Count = 1);

	/**
	 * Create a talk objective
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Objectives")
	static FQuestObjective MakeTalkObjective(const FString& ObjectiveID, const FText& Description, const FString& NPCID);

	/**
	 * Create an explore objective
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Objectives")
	static FQuestObjective MakeExploreObjective(const FString& ObjectiveID, const FText& Description, const FString& LocationID);

	/**
	 * Create a deliver objective
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Objectives")
	static FQuestObjective MakeDeliverObjective(const FString& ObjectiveID, const FText& Description, const FString& ItemID, const FString& NPCID);

	/**
	 * Remove an objective from a quest
	 * @param Quest - The quest to modify
	 * @param ObjectiveID - ID of objective to remove
	 * @return True if objective was found and removed
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Objectives")
	static bool RemoveObjective(UPARAM(ref) FQuestData& Quest, const FString& ObjectiveID);

	// ============================================
	// REWARD MANAGEMENT
	// ============================================

	/**
	 * Set quest rewards
	 * @param Quest - The quest to modify
	 * @param XP - Experience points
	 * @param Gold - Currency reward
	 * @param ItemIDs - Array of item IDs to reward
	 * @return The modified quest
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Rewards")
	static FQuestData SetRewards(
		UPARAM(ref) FQuestData& Quest,
		int32 XP,
		int32 Gold,
		const TArray<FString>& ItemIDs);

	/**
	 * Add an item reward
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Rewards")
	static FQuestData AddItemReward(UPARAM(ref) FQuestData& Quest, const FString& ItemID);

	/**
	 * Add a reputation reward
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Rewards")
	static FQuestData AddReputationReward(UPARAM(ref) FQuestData& Quest, const FString& FactionID, int32 Amount);

	/**
	 * Set quest unlocks (quests that unlock upon completion)
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Rewards")
	static FQuestData SetQuestUnlocks(UPARAM(ref) FQuestData& Quest, const TArray<FString>& QuestIDs);

	// ============================================
	// REQUIREMENTS
	// ============================================

	/**
	 * Set quest requirements
	 * @param Quest - The quest to modify
	 * @param RequiredLevel - Minimum player level
	 * @param RequiredQuests - Quest IDs that must be completed first
	 * @return The modified quest
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Requirements")
	static FQuestData SetRequirements(
		UPARAM(ref) FQuestData& Quest,
		int32 RequiredLevel,
		const TArray<FString>& RequiredQuests);

	/**
	 * Add a prerequisite quest
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Requirements")
	static FQuestData AddPrerequisiteQuest(UPARAM(ref) FQuestData& Quest, const FString& QuestID);

	/**
	 * Add a required item
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Requirements")
	static FQuestData AddRequiredItem(UPARAM(ref) FQuestData& Quest, const FString& ItemID);

	// ============================================
	// QUEST CHAINS
	// ============================================

	/**
	 * Create a quest chain (multiple quests with sequential prerequisites)
	 * @param ChainID - Base ID for the chain (quests will be ChainID_01, ChainID_02, etc.)
	 * @param Entries - Array of quest chain entries
	 * @param QuestGiverID - NPC who gives all quests in the chain
	 * @param QuestType - Type for all quests in the chain
	 * @return Array of created quests with prerequisites set
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Chains")
	static TArray<FQuestData> CreateQuestChain(
		const FString& ChainID,
		const TArray<FQuestChainEntry>& Entries,
		const FString& QuestGiverID,
		EQuestType QuestType = EQuestType::Main);

	/**
	 * Link quests as a chain (set prerequisites sequentially)
	 * @param Quests - Array of quests to link (modified in place)
	 * @return The modified quests
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Chains")
	static TArray<FQuestData> LinkQuestsAsChain(UPARAM(ref) TArray<FQuestData>& Quests);

	// ============================================
	// EXPORT / SAVE
	// ============================================

	/**
	 * Export a quest to JSON string
	 * @param Quest - The quest to export
	 * @return JSON string representation
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Export")
	static FString ExportQuestToJSON(const FQuestData& Quest);

	/**
	 * Export multiple quests to JSON string
	 * @param Quests - Array of quests to export
	 * @return JSON string with quests array
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Export")
	static FString ExportQuestsToJSON(const TArray<FQuestData>& Quests);

	/**
	 * Save quests to a JSON file
	 * @param Quests - Quests to save
	 * @param FilePath - Absolute path to save to
	 * @return True if save succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Export")
	static bool SaveQuestsToFile(const TArray<FQuestData>& Quests, const FString& FilePath);

	/**
	 * Load quests from a JSON file
	 * @param FilePath - Path to JSON file
	 * @return Array of loaded quests
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Export")
	static TArray<FQuestData> LoadQuestsFromFile(const FString& FilePath);

	// ============================================
	// VALIDATION
	// ============================================

	/**
	 * Validate a quest for common issues
	 * @param Quest - The quest to validate
	 * @param OutErrors - Array of error messages
	 * @return True if quest is valid
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Validation")
	static bool ValidateQuest(const FQuestData& Quest, TArray<FString>& OutErrors);

	/**
	 * Validate a quest chain for issues (circular dependencies, etc.)
	 * @param Quests - Array of quests to validate
	 * @param OutErrors - Array of error messages
	 * @return True if chain is valid
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Validation")
	static bool ValidateQuestChain(const TArray<FQuestData>& Quests, TArray<FString>& OutErrors);

	/**
	 * Generate a unique quest ID
	 * @param Prefix - Optional prefix for the ID
	 * @return A unique quest ID
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Utilities")
	static FString GenerateQuestID(const FString& Prefix = TEXT("quest"));

	// ============================================
	// TEMPLATES
	// ============================================

	/**
	 * Get available quest templates
	 * @return Array of template names
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Templates")
	static TArray<FString> GetQuestTemplates();

	/**
	 * Create a quest from a template
	 * @param TemplateName - Name of the template
	 * @param QuestID - ID for the new quest
	 * @param Title - Quest title
	 * @param Description - Quest description
	 * @return The created quest
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Templates")
	static FQuestData CreateQuestFromTemplate(
		const FString& TemplateName,
		const FString& QuestID,
		const FText& Title,
		const FText& Description);

	// ============================================
	// BATCH OPERATIONS
	// ============================================

	/**
	 * Register multiple quests with the quest subsystem
	 * @param Quests - Quests to register
	 * @param WorldContextObject - World context
	 * @return Number of quests registered
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Batch", meta = (WorldContext = "WorldContextObject"))
	static int32 RegisterQuests(const TArray<FQuestData>& Quests, const UObject* WorldContextObject);

	/**
	 * Duplicate a quest with a new ID
	 * @param SourceQuest - Quest to duplicate
	 * @param NewQuestID - ID for the new quest
	 * @return The duplicated quest
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Batch")
	static FQuestData DuplicateQuest(const FQuestData& SourceQuest, const FString& NewQuestID);

	/**
	 * Scale quest rewards by a multiplier
	 * @param Quest - Quest to modify
	 * @param Multiplier - Reward multiplier (1.0 = no change)
	 * @return The modified quest
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Batch")
	static FQuestData ScaleRewards(UPARAM(ref) FQuestData& Quest, float Multiplier);

	/**
	 * Scale objective counts by a multiplier
	 * @param Quest - Quest to modify
	 * @param Multiplier - Count multiplier (1.0 = no change)
	 * @return The modified quest
	 */
	UFUNCTION(BlueprintCallable, Category = "GenQuestUtils|Batch")
	static FQuestData ScaleObjectiveCounts(UPARAM(ref) FQuestData& Quest, float Multiplier);

private:
	/** Built-in quest templates */
	static TMap<FString, FQuestTemplate> QuestTemplates;

	/** Initialize templates on first use */
	static void InitializeTemplates();

	/** Counter for unique ID generation */
	static int32 QuestIDCounter;
};
