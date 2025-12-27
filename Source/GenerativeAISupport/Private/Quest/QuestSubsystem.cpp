// Quest Subsystem Implementation
// Part of GenerativeAISupport Plugin

#include "Quest/QuestSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "JsonObjectConverter.h"

void UQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("QuestSubsystem initialized"));
}

void UQuestSubsystem::Deinitialize()
{
	Super::Deinitialize();
	UE_LOG(LogTemp, Log, TEXT("QuestSubsystem deinitialized"));
}

UQuestSubsystem* UQuestSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UQuestSubsystem>();
}

// ============================================
// QUEST REGISTRY
// ============================================

void UQuestSubsystem::RegisterQuest(const FQuestData& QuestData)
{
	if (QuestData.QuestID.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot register quest with empty ID"));
		return;
	}

	QuestRegistry.Add(QuestData.QuestID, QuestData);
	UE_LOG(LogTemp, Log, TEXT("Registered quest: %s"), *QuestData.QuestID);
}

const FQuestData* UQuestSubsystem::GetQuestDefinition(const FString& QuestID) const
{
	return QuestRegistry.Find(QuestID);
}

TArray<FQuestData> UQuestSubsystem::GetAllQuestDefinitions() const
{
	TArray<FQuestData> Result;
	QuestRegistry.GenerateValueArray(Result);
	return Result;
}

// ============================================
// ACTIVE QUESTS
// ============================================

bool UQuestSubsystem::StartQuest(const FString& QuestID)
{
	// Check if already active
	if (ActiveQuests.Contains(QuestID))
	{
		UE_LOG(LogTemp, Warning, TEXT("Quest %s is already active"), *QuestID);
		return false;
	}

	// Check if already completed (and not repeatable or on cooldown)
	if (CompletedQuests.Contains(QuestID))
	{
		const FQuestData* Definition = GetQuestDefinition(QuestID);
		if (Definition && !Definition->bRepeatable)
		{
			UE_LOG(LogTemp, Warning, TEXT("Quest %s is already completed and not repeatable"), *QuestID);
			return false;
		}

		// Check cooldown
		if (const FDateTime* Cooldown = RepeatCooldowns.Find(QuestID))
		{
			if (FDateTime::UtcNow() < *Cooldown)
			{
				UE_LOG(LogTemp, Warning, TEXT("Quest %s is on cooldown"), *QuestID);
				return false;
			}
		}
	}

	// Get quest definition
	const FQuestData* Definition = GetQuestDefinition(QuestID);
	if (!Definition)
	{
		UE_LOG(LogTemp, Warning, TEXT("Quest %s not found in registry"), *QuestID);
		return false;
	}

	// Check requirements
	if (!MeetsRequirements(QuestID))
	{
		UE_LOG(LogTemp, Warning, TEXT("Requirements not met for quest %s"), *QuestID);
		return false;
	}

	// Create active quest instance
	FQuestData ActiveQuest = *Definition;
	ActiveQuest.Status = EQuestStatus::Active;
	ActiveQuest.StartedAt = FDateTime::UtcNow();

	// Reset objective progress
	for (FQuestObjective& Obj : ActiveQuest.Objectives)
	{
		Obj.CurrentCount = 0;
	}

	ActiveQuests.Add(QuestID, ActiveQuest);

	// Broadcast event
	OnQuestStarted.Broadcast(ActiveQuest);

	UE_LOG(LogTemp, Log, TEXT("Started quest: %s"), *QuestID);
	return true;
}

bool UQuestSubsystem::CompleteQuest(const FString& QuestID)
{
	FQuestData* Quest = ActiveQuests.Find(QuestID);
	if (!Quest)
	{
		UE_LOG(LogTemp, Warning, TEXT("Quest %s is not active"), *QuestID);
		return false;
	}

	// Check if objectives are complete
	if (!Quest->AreRequiredObjectivesComplete())
	{
		UE_LOG(LogTemp, Warning, TEXT("Quest %s objectives not complete"), *QuestID);
		return false;
	}

	// Mark as completed
	Quest->Status = EQuestStatus::Completed;
	Quest->CompletedAt = FDateTime::UtcNow();

	// Grant rewards
	GrantRewards(Quest->Rewards);

	// Add to completed set
	CompletedQuests.Add(QuestID);

	// Set cooldown if repeatable
	if (Quest->bRepeatable && Quest->RepeatCooldown > 0)
	{
		RepeatCooldowns.Add(QuestID, FDateTime::UtcNow() + FTimespan::FromSeconds(Quest->RepeatCooldown));
	}

	// Broadcast event
	OnQuestCompleted.Broadcast(*Quest);

	// Remove from active
	ActiveQuests.Remove(QuestID);

	// Remove from tracked if it was tracked
	TrackedQuestIDs.Remove(QuestID);

	UE_LOG(LogTemp, Log, TEXT("Completed quest: %s"), *QuestID);
	return true;
}

bool UQuestSubsystem::FailQuest(const FString& QuestID)
{
	FQuestData* Quest = ActiveQuests.Find(QuestID);
	if (!Quest)
	{
		UE_LOG(LogTemp, Warning, TEXT("Quest %s is not active"), *QuestID);
		return false;
	}

	Quest->Status = EQuestStatus::Failed;
	FailedQuests.Add(QuestID);

	OnQuestFailed.Broadcast(*Quest);

	ActiveQuests.Remove(QuestID);
	TrackedQuestIDs.Remove(QuestID);

	UE_LOG(LogTemp, Log, TEXT("Failed quest: %s"), *QuestID);
	return true;
}

bool UQuestSubsystem::AbandonQuest(const FString& QuestID)
{
	if (!ActiveQuests.Contains(QuestID))
	{
		return false;
	}

	ActiveQuests.Remove(QuestID);
	TrackedQuestIDs.Remove(QuestID);

	UE_LOG(LogTemp, Log, TEXT("Abandoned quest: %s"), *QuestID);
	return true;
}

FQuestData* UQuestSubsystem::GetActiveQuest(const FString& QuestID)
{
	return ActiveQuests.Find(QuestID);
}

TArray<FQuestData> UQuestSubsystem::GetActiveQuests() const
{
	TArray<FQuestData> Result;
	ActiveQuests.GenerateValueArray(Result);
	return Result;
}

bool UQuestSubsystem::IsQuestActive(const FString& QuestID) const
{
	return ActiveQuests.Contains(QuestID);
}

// ============================================
// COMPLETED QUESTS
// ============================================

bool UQuestSubsystem::IsQuestCompleted(const FString& QuestID) const
{
	return CompletedQuests.Contains(QuestID);
}

TArray<FString> UQuestSubsystem::GetCompletedQuestIDs() const
{
	return CompletedQuests.Array();
}

// ============================================
// OBJECTIVE TRACKING
// ============================================

void UQuestSubsystem::ReportProgress(EObjectiveType Type, const FString& TargetID, int32 Count)
{
	UpdateObjectives(Type, TargetID, Count);
}

void UQuestSubsystem::UpdateObjectives(EObjectiveType Type, const FString& TargetID, int32 Count)
{
	TArray<FString> QuestsToComplete;

	for (auto& Pair : ActiveQuests)
	{
		FQuestData& Quest = Pair.Value;
		bool bQuestUpdated = false;

		for (FQuestObjective& Obj : Quest.Objectives)
		{
			if (ObjectiveMatches(Obj, Type, TargetID) && !Obj.IsCompleted())
			{
				int32 OldCount = Obj.CurrentCount;
				Obj.CurrentCount = FMath::Min(Obj.CurrentCount + Count, Obj.RequiredCount);

				if (Obj.CurrentCount != OldCount)
				{
					bQuestUpdated = true;
					OnObjectiveProgress.Broadcast(Quest.QuestID, Obj);

					if (Obj.IsCompleted())
					{
						OnObjectiveCompleted.Broadcast(Quest.QuestID, Obj);
					}
				}
			}
		}

		// Check if quest is now completable
		if (bQuestUpdated && Quest.AreRequiredObjectivesComplete())
		{
			// Auto-complete if no turn-in NPC specified
			if (Quest.TurnInNPCID.IsEmpty() && Quest.QuestGiverID.IsEmpty())
			{
				QuestsToComplete.Add(Quest.QuestID);
			}
		}
	}

	// Complete any auto-completable quests
	for (const FString& QuestID : QuestsToComplete)
	{
		CompleteQuest(QuestID);
	}
}

bool UQuestSubsystem::ObjectiveMatches(const FQuestObjective& Objective, EObjectiveType Type, const FString& TargetID) const
{
	if (Objective.Type != Type)
	{
		return false;
	}

	// Empty target matches any
	if (Objective.TargetID.IsEmpty())
	{
		return true;
	}

	// Exact match or wildcard
	if (Objective.TargetID == TargetID || Objective.TargetID == TEXT("*"))
	{
		return true;
	}

	// Check for prefix match (e.g., "wolf_*" matches "wolf_alpha")
	if (Objective.TargetID.EndsWith(TEXT("*")))
	{
		FString Prefix = Objective.TargetID.LeftChop(1);
		return TargetID.StartsWith(Prefix);
	}

	return false;
}

// ============================================
// REQUIREMENTS CHECK
// ============================================

bool UQuestSubsystem::MeetsRequirements(const FString& QuestID) const
{
	const FQuestData* Quest = GetQuestDefinition(QuestID);
	if (!Quest)
	{
		return false;
	}

	const FQuestRequirements& Req = Quest->Requirements;

	// Check required quests
	for (const FString& RequiredQuestID : Req.RequiredQuests)
	{
		if (!CompletedQuests.Contains(RequiredQuestID))
		{
			return false;
		}
	}

	// TODO: Check level, items, reputation when those systems are implemented

	return true;
}

TArray<FQuestData> UQuestSubsystem::GetAvailableQuests() const
{
	TArray<FQuestData> Result;

	for (const auto& Pair : QuestRegistry)
	{
		const FString& QuestID = Pair.Key;
		const FQuestData& Quest = Pair.Value;

		// Skip if already active or completed (and not repeatable)
		if (IsQuestActive(QuestID))
		{
			continue;
		}

		if (IsQuestCompleted(QuestID) && !Quest.bRepeatable)
		{
			continue;
		}

		// Check requirements
		if (MeetsRequirements(QuestID))
		{
			FQuestData AvailableQuest = Quest;
			AvailableQuest.Status = EQuestStatus::Available;
			Result.Add(AvailableQuest);
		}
	}

	return Result;
}

// ============================================
// NPC INTEGRATION
// ============================================

TArray<FQuestData> UQuestSubsystem::GetQuestsFromNPC(const FString& NPCID) const
{
	TArray<FQuestData> Result;

	for (const FQuestData& Quest : GetAvailableQuests())
	{
		if (Quest.QuestGiverID == NPCID)
		{
			Result.Add(Quest);
		}
	}

	return Result;
}

TArray<FQuestData> UQuestSubsystem::GetQuestsCompletableAtNPC(const FString& NPCID) const
{
	TArray<FQuestData> Result;

	for (const auto& Pair : ActiveQuests)
	{
		const FQuestData& Quest = Pair.Value;

		FString TurnInNPC = Quest.TurnInNPCID.IsEmpty() ? Quest.QuestGiverID : Quest.TurnInNPCID;

		if (TurnInNPC == NPCID && Quest.AreRequiredObjectivesComplete())
		{
			Result.Add(Quest);
		}
	}

	return Result;
}

FString UQuestSubsystem::GetQuestContextForNPC(const FString& NPCID) const
{
	TSharedPtr<FJsonObject> Context = MakeShared<FJsonObject>();

	// Available quests
	TArray<TSharedPtr<FJsonValue>> AvailableArray;
	for (const FQuestData& Quest : GetQuestsFromNPC(NPCID))
	{
		TSharedPtr<FJsonObject> QuestObj = MakeShared<FJsonObject>();
		QuestObj->SetStringField(TEXT("id"), Quest.QuestID);
		QuestObj->SetStringField(TEXT("title"), Quest.Title.ToString());
		AvailableArray.Add(MakeShared<FJsonValueObject>(QuestObj));
	}
	Context->SetArrayField(TEXT("available_quests"), AvailableArray);

	// Completable quests
	TArray<TSharedPtr<FJsonValue>> CompletableArray;
	for (const FQuestData& Quest : GetQuestsCompletableAtNPC(NPCID))
	{
		TSharedPtr<FJsonObject> QuestObj = MakeShared<FJsonObject>();
		QuestObj->SetStringField(TEXT("id"), Quest.QuestID);
		QuestObj->SetStringField(TEXT("title"), Quest.Title.ToString());
		CompletableArray.Add(MakeShared<FJsonValueObject>(QuestObj));
	}
	Context->SetArrayField(TEXT("completable_quests"), CompletableArray);

	// Active quests involving this NPC
	TArray<TSharedPtr<FJsonValue>> ActiveArray;
	for (const auto& Pair : ActiveQuests)
	{
		const FQuestData& Quest = Pair.Value;
		// Check if any objective involves this NPC
		for (const FQuestObjective& Obj : Quest.Objectives)
		{
			if (Obj.Type == EObjectiveType::Talk && Obj.TargetID == NPCID)
			{
				TSharedPtr<FJsonObject> QuestObj = MakeShared<FJsonObject>();
				QuestObj->SetStringField(TEXT("id"), Quest.QuestID);
				QuestObj->SetStringField(TEXT("title"), Quest.Title.ToString());
				QuestObj->SetNumberField(TEXT("progress"), Quest.GetOverallProgress());
				ActiveArray.Add(MakeShared<FJsonValueObject>(QuestObj));
				break;
			}
		}
	}
	Context->SetArrayField(TEXT("active_quests"), ActiveArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Context.ToSharedRef(), Writer);

	return OutputString;
}

// ============================================
// HUD TRACKING
// ============================================

void UQuestSubsystem::TrackQuest(const FString& QuestID)
{
	if (!IsQuestActive(QuestID))
	{
		return;
	}

	if (TrackedQuestIDs.Contains(QuestID))
	{
		return;
	}

	if (TrackedQuestIDs.Num() >= MAX_TRACKED_QUESTS)
	{
		TrackedQuestIDs.RemoveAt(0);
	}

	TrackedQuestIDs.Add(QuestID);
}

void UQuestSubsystem::UntrackQuest(const FString& QuestID)
{
	TrackedQuestIDs.Remove(QuestID);
}

TArray<FQuestData> UQuestSubsystem::GetTrackedQuests() const
{
	TArray<FQuestData> Result;

	for (const FString& QuestID : TrackedQuestIDs)
	{
		if (const FQuestData* Quest = ActiveQuests.Find(QuestID))
		{
			Result.Add(*Quest);
		}
	}

	return Result;
}

// ============================================
// SAVE/LOAD
// ============================================

FString UQuestSubsystem::SaveToJSON() const
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	// Active quests
	TSharedPtr<FJsonObject> ActiveObj = MakeShared<FJsonObject>();
	for (const auto& Pair : ActiveQuests)
	{
		TSharedPtr<FJsonObject> QuestObj = MakeShared<FJsonObject>();
		QuestObj->SetStringField(TEXT("status"), TEXT("active"));

		TArray<TSharedPtr<FJsonValue>> ObjArray;
		for (const FQuestObjective& Obj : Pair.Value.Objectives)
		{
			TSharedPtr<FJsonObject> ObjData = MakeShared<FJsonObject>();
			ObjData->SetStringField(TEXT("id"), Obj.ObjectiveID);
			ObjData->SetNumberField(TEXT("progress"), Obj.CurrentCount);
			ObjArray.Add(MakeShared<FJsonValueObject>(ObjData));
		}
		QuestObj->SetArrayField(TEXT("objectives"), ObjArray);

		ActiveObj->SetObjectField(Pair.Key, QuestObj);
	}
	Root->SetObjectField(TEXT("active_quests"), ActiveObj);

	// Completed quests
	TArray<TSharedPtr<FJsonValue>> CompletedArray;
	for (const FString& QuestID : CompletedQuests)
	{
		CompletedArray.Add(MakeShared<FJsonValueString>(QuestID));
	}
	Root->SetArrayField(TEXT("completed_quests"), CompletedArray);

	// Tracked quests
	TArray<TSharedPtr<FJsonValue>> TrackedArray;
	for (const FString& QuestID : TrackedQuestIDs)
	{
		TrackedArray.Add(MakeShared<FJsonValueString>(QuestID));
	}
	Root->SetArrayField(TEXT("tracked_quests"), TrackedArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	return OutputString;
}

bool UQuestSubsystem::LoadFromJSON(const FString& JSONString)
{
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse quest save data"));
		return false;
	}

	// Load completed quests
	CompletedQuests.Empty();
	const TArray<TSharedPtr<FJsonValue>>* CompletedArray;
	if (Root->TryGetArrayField(TEXT("completed_quests"), CompletedArray))
	{
		for (const TSharedPtr<FJsonValue>& Val : *CompletedArray)
		{
			CompletedQuests.Add(Val->AsString());
		}
	}

	// Load active quests
	ActiveQuests.Empty();
	const TSharedPtr<FJsonObject>* ActiveObj;
	if (Root->TryGetObjectField(TEXT("active_quests"), ActiveObj))
	{
		for (const auto& Pair : (*ActiveObj)->Values)
		{
			const FString& QuestID = Pair.Key;
			const TSharedPtr<FJsonObject>* QuestObj;

			if (Pair.Value->TryGetObject(QuestObj))
			{
				// Get quest definition
				const FQuestData* Definition = GetQuestDefinition(QuestID);
				if (Definition)
				{
					FQuestData Quest = *Definition;
					Quest.Status = EQuestStatus::Active;

					// Load objective progress
					const TArray<TSharedPtr<FJsonValue>>* ObjArray;
					if ((*QuestObj)->TryGetArrayField(TEXT("objectives"), ObjArray))
					{
						for (const TSharedPtr<FJsonValue>& ObjVal : *ObjArray)
						{
							const TSharedPtr<FJsonObject>* ObjData;
							if (ObjVal->TryGetObject(ObjData))
							{
								FString ObjID = (*ObjData)->GetStringField(TEXT("id"));
								int32 Progress = (*ObjData)->GetIntegerField(TEXT("progress"));

								for (FQuestObjective& Obj : Quest.Objectives)
								{
									if (Obj.ObjectiveID == ObjID)
									{
										Obj.CurrentCount = Progress;
										break;
									}
								}
							}
						}
					}

					ActiveQuests.Add(QuestID, Quest);
				}
			}
		}
	}

	// Load tracked quests
	TrackedQuestIDs.Empty();
	const TArray<TSharedPtr<FJsonValue>>* TrackedArray;
	if (Root->TryGetArrayField(TEXT("tracked_quests"), TrackedArray))
	{
		for (const TSharedPtr<FJsonValue>& Val : *TrackedArray)
		{
			TrackedQuestIDs.Add(Val->AsString());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Loaded quest data: %d active, %d completed"), ActiveQuests.Num(), CompletedQuests.Num());
	return true;
}

// ============================================
// REWARDS
// ============================================

void UQuestSubsystem::GrantRewards(const FQuestReward& Rewards)
{
	// TODO: Implement when XP/Gold/Inventory systems exist
	// For now, just log what would be granted

	if (Rewards.XP > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Quest Reward: +%d XP"), Rewards.XP);
	}

	if (Rewards.Gold > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Quest Reward: +%d Gold"), Rewards.Gold);
	}

	for (const FString& ItemID : Rewards.ItemIDs)
	{
		UE_LOG(LogTemp, Log, TEXT("Quest Reward: Item %s"), *ItemID);
	}

	for (const auto& RepPair : Rewards.ReputationChanges)
	{
		UE_LOG(LogTemp, Log, TEXT("Quest Reward: +%d Reputation with %s"), RepPair.Value, *RepPair.Key);
	}

	for (const FString& QuestID : Rewards.UnlocksQuests)
	{
		UE_LOG(LogTemp, Log, TEXT("Quest Reward: Unlocked quest %s"), *QuestID);
	}
}
