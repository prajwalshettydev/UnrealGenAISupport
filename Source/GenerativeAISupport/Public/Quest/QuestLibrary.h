// Quest Library - Blueprint callable functions for Quest System
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Quest/QuestTypes.h"
#include "QuestLibrary.generated.h"

// Delegate for quest events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestStarted, const FQuestData&, Quest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestCompleted, const FQuestData&, Quest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestFailed, const FQuestData&, Quest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnObjectiveProgress, const FString&, QuestID, const FQuestObjective&, Objective);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnObjectiveCompleted, const FString&, QuestID, const FQuestObjective&, Objective);

/**
 * Blueprint Function Library for Quest System
 * Provides functions to manage quests, objectives, and rewards
 */
UCLASS()
class GENERATIVEAISUPPORT_API UQuestLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// QUEST MANAGEMENT
	// ============================================

	/**
	 * Start a quest by ID
	 * @param QuestID - The quest to start
	 * @return True if quest was started successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Start Quest"))
	static bool StartQuest(const FString& QuestID);

	/**
	 * Complete a quest by ID
	 * @param QuestID - The quest to complete
	 * @return True if quest was completed successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Complete Quest"))
	static bool CompleteQuest(const FString& QuestID);

	/**
	 * Fail a quest by ID
	 * @param QuestID - The quest to fail
	 * @return True if quest was failed successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Fail Quest"))
	static bool FailQuest(const FString& QuestID);

	/**
	 * Abandon an active quest
	 * @param QuestID - The quest to abandon
	 * @return True if quest was abandoned
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Abandon Quest"))
	static bool AbandonQuest(const FString& QuestID);

	/**
	 * Get quest data by ID
	 * @param QuestID - The quest ID
	 * @param OutQuestData - The quest data
	 * @return True if quest was found
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Get Quest"))
	static bool GetQuest(const FString& QuestID, FQuestData& OutQuestData);

	/**
	 * Check if a quest is active
	 * @param QuestID - The quest ID
	 * @return True if quest is currently active
	 */
	UFUNCTION(BlueprintPure, Category = "Quest System", meta = (DisplayName = "Is Quest Active"))
	static bool IsQuestActive(const FString& QuestID);

	/**
	 * Check if a quest is completed
	 * @param QuestID - The quest ID
	 * @return True if quest is completed
	 */
	UFUNCTION(BlueprintPure, Category = "Quest System", meta = (DisplayName = "Is Quest Completed"))
	static bool IsQuestCompleted(const FString& QuestID);

	/**
	 * Check if player can accept a quest
	 * @param QuestID - The quest ID
	 * @return True if all requirements are met
	 */
	UFUNCTION(BlueprintPure, Category = "Quest System", meta = (DisplayName = "Can Accept Quest"))
	static bool CanAcceptQuest(const FString& QuestID);

	// ============================================
	// OBJECTIVE TRACKING
	// ============================================

	/**
	 * Report progress on a quest objective
	 * Call this when player kills a monster, collects an item, etc.
	 * @param ObjectiveType - Type of objective (Kill, Collect, etc.)
	 * @param TargetID - What was killed/collected/etc.
	 * @param Count - How many (default 1)
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Report Objective Progress"))
	static void ReportProgress(EObjectiveType ObjectiveType, const FString& TargetID, int32 Count = 1);

	/**
	 * Report entering a location (for Explore objectives)
	 * @param LocationID - The location ID entered
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Report Location Entered"))
	static void ReportLocationEntered(const FString& LocationID);

	/**
	 * Report talking to an NPC (for Talk objectives)
	 * @param NPCID - The NPC talked to
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Report NPC Talked"))
	static void ReportNPCTalked(const FString& NPCID);

	/**
	 * Report item delivered (for Deliver objectives)
	 * @param ItemID - The item delivered
	 * @param NPCID - The NPC delivered to
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Report Item Delivered"))
	static void ReportItemDelivered(const FString& ItemID, const FString& NPCID);

	// ============================================
	// QUEST QUERIES
	// ============================================

	/**
	 * Get all active quests
	 * @return Array of active quests
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Get Active Quests"))
	static TArray<FQuestData> GetActiveQuests();

	/**
	 * Get all completed quests
	 * @return Array of completed quest IDs
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Get Completed Quests"))
	static TArray<FString> GetCompletedQuestIDs();

	/**
	 * Get available quests for an NPC
	 * @param NPCID - The NPC's ID
	 * @return Array of quests this NPC can give
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Get NPC Available Quests"))
	static TArray<FQuestData> GetNPCAvailableQuests(const FString& NPCID);

	/**
	 * Get quests that can be turned in to an NPC
	 * @param NPCID - The NPC's ID
	 * @return Array of completable quests
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Get NPC Completable Quests"))
	static TArray<FQuestData> GetNPCCompletableQuests(const FString& NPCID);

	/**
	 * Check if NPC has quest marker (yellow !, blue ?, green ✓)
	 * @param NPCID - The NPC's ID
	 * @param OutHasNewQuest - True if NPC has new quest available
	 * @param OutHasActiveQuest - True if NPC is involved in active quest
	 * @param OutHasCompletableQuest - True if quest can be turned in here
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System", meta = (DisplayName = "Get NPC Quest Status"))
	static void GetNPCQuestStatus(const FString& NPCID, bool& OutHasNewQuest, bool& OutHasActiveQuest, bool& OutHasCompletableQuest);

	// ============================================
	// HUD TRACKING
	// ============================================

	/**
	 * Track a quest on the HUD
	 * @param QuestID - Quest to track
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System|HUD", meta = (DisplayName = "Track Quest"))
	static void TrackQuest(const FString& QuestID);

	/**
	 * Untrack a quest from HUD
	 * @param QuestID - Quest to untrack
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System|HUD", meta = (DisplayName = "Untrack Quest"))
	static void UntrackQuest(const FString& QuestID);

	/**
	 * Get currently tracked quests (max 3)
	 * @return Array of tracked quests
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System|HUD", meta = (DisplayName = "Get Tracked Quests"))
	static TArray<FQuestData> GetTrackedQuests();

	// ============================================
	// QUEST DATA MANAGEMENT
	// ============================================

	/**
	 * Register a quest definition (call during game init)
	 * @param QuestData - The quest to register
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System|Data", meta = (DisplayName = "Register Quest"))
	static void RegisterQuest(const FQuestData& QuestData);

	/**
	 * Load quests from JSON file
	 * @param FilePath - Path to JSON file
	 * @return Number of quests loaded
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System|Data", meta = (DisplayName = "Load Quests From JSON"))
	static int32 LoadQuestsFromJSON(const FString& FilePath);

	/**
	 * Get quest context for NPC dialog (for AI integration)
	 * @param NPCID - The NPC's ID
	 * @return JSON string with quest context
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest System|AI", meta = (DisplayName = "Get Quest Context For NPC"))
	static FString GetQuestContextForNPC(const FString& NPCID);
};
