// Quest Subsystem - Runtime quest management
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Quest/QuestTypes.h"
#include "QuestSubsystem.generated.h"

// Delegates for quest events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestEvent, const FQuestData&, Quest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnObjectiveEvent, const FString&, QuestID, const FQuestObjective&, Objective);

/**
 * Quest Subsystem - Manages all quest state at runtime
 * Access via UGameInstance::GetSubsystem<UQuestSubsystem>()
 */
UCLASS()
class GENERATIVEAISUPPORT_API UQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ============================================
	// SUBSYSTEM LIFECYCLE
	// ============================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Get the Quest Subsystem from any world context */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (WorldContext = "WorldContextObject"))
	static UQuestSubsystem* Get(const UObject* WorldContextObject);

	// ============================================
	// EVENTS
	// ============================================

	/** Called when a quest is started */
	UPROPERTY(BlueprintAssignable, Category = "Quest System|Events")
	FOnQuestEvent OnQuestStarted;

	/** Called when a quest is completed */
	UPROPERTY(BlueprintAssignable, Category = "Quest System|Events")
	FOnQuestEvent OnQuestCompleted;

	/** Called when a quest fails */
	UPROPERTY(BlueprintAssignable, Category = "Quest System|Events")
	FOnQuestEvent OnQuestFailed;

	/** Called when objective progress is made */
	UPROPERTY(BlueprintAssignable, Category = "Quest System|Events")
	FOnObjectiveEvent OnObjectiveProgress;

	/** Called when an objective is completed */
	UPROPERTY(BlueprintAssignable, Category = "Quest System|Events")
	FOnObjectiveEvent OnObjectiveCompleted;

	// ============================================
	// QUEST REGISTRY (All available quests)
	// ============================================

	/** Register a quest definition */
	void RegisterQuest(const FQuestData& QuestData);

	/** Get a registered quest by ID */
	const FQuestData* GetQuestDefinition(const FString& QuestID) const;

	/** Get all registered quests */
	TArray<FQuestData> GetAllQuestDefinitions() const;

	// ============================================
	// ACTIVE QUESTS
	// ============================================

	/** Start a quest */
	bool StartQuest(const FString& QuestID);

	/** Complete a quest */
	bool CompleteQuest(const FString& QuestID);

	/** Fail a quest */
	bool FailQuest(const FString& QuestID);

	/** Abandon a quest */
	bool AbandonQuest(const FString& QuestID);

	/** Get an active quest */
	FQuestData* GetActiveQuest(const FString& QuestID);

	/** Get all active quests */
	TArray<FQuestData> GetActiveQuests() const;

	/** Check if quest is active */
	bool IsQuestActive(const FString& QuestID) const;

	// ============================================
	// COMPLETED QUESTS
	// ============================================

	/** Check if quest is completed */
	bool IsQuestCompleted(const FString& QuestID) const;

	/** Get all completed quest IDs */
	TArray<FString> GetCompletedQuestIDs() const;

	// ============================================
	// OBJECTIVE TRACKING
	// ============================================

	/** Report progress on objectives */
	void ReportProgress(EObjectiveType Type, const FString& TargetID, int32 Count = 1);

	// ============================================
	// REQUIREMENTS CHECK
	// ============================================

	/** Check if player meets requirements for a quest */
	bool MeetsRequirements(const FString& QuestID) const;

	/** Get available quests (requirements met, not started) */
	TArray<FQuestData> GetAvailableQuests() const;

	// ============================================
	// NPC INTEGRATION
	// ============================================

	/** Get quests available from an NPC */
	TArray<FQuestData> GetQuestsFromNPC(const FString& NPCID) const;

	/** Get quests completable at an NPC */
	TArray<FQuestData> GetQuestsCompletableAtNPC(const FString& NPCID) const;

	/** Get quest context for NPC AI dialog */
	FString GetQuestContextForNPC(const FString& NPCID) const;

	// ============================================
	// HUD TRACKING
	// ============================================

	/** Track a quest on HUD */
	void TrackQuest(const FString& QuestID);

	/** Untrack a quest */
	void UntrackQuest(const FString& QuestID);

	/** Get tracked quests */
	TArray<FQuestData> GetTrackedQuests() const;

	// ============================================
	// SAVE/LOAD
	// ============================================

	/** Serialize quest state to JSON */
	FString SaveToJSON() const;

	/** Load quest state from JSON */
	bool LoadFromJSON(const FString& JSONString);

protected:
	/** All registered quest definitions */
	UPROPERTY()
	TMap<FString, FQuestData> QuestRegistry;

	/** Currently active quests */
	UPROPERTY()
	TMap<FString, FQuestData> ActiveQuests;

	/** Completed quest IDs */
	UPROPERTY()
	TSet<FString> CompletedQuests;

	/** Failed quest IDs */
	UPROPERTY()
	TSet<FString> FailedQuests;

	/** Currently tracked quest IDs (max 3) */
	UPROPERTY()
	TArray<FString> TrackedQuestIDs;

	/** Cooldowns for repeatable quests */
	UPROPERTY()
	TMap<FString, FDateTime> RepeatCooldowns;

	/** Update objectives for all active quests */
	void UpdateObjectives(EObjectiveType Type, const FString& TargetID, int32 Count);

	/** Check if a specific objective matches the criteria */
	bool ObjectiveMatches(const FQuestObjective& Objective, EObjectiveType Type, const FString& TargetID) const;

	/** Grant rewards for completing a quest */
	void GrantRewards(const FQuestReward& Rewards);

	/** Maximum tracked quests */
	static const int32 MAX_TRACKED_QUESTS = 3;
};
