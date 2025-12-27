// Quest Library Implementation
// Part of GenerativeAISupport Plugin

#include "Quest/QuestLibrary.h"
#include "Quest/QuestSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"

// Helper to get the quest subsystem
static UQuestSubsystem* GetQuestSubsystem()
{
	// Try to get from game instance
	if (GEngine && GEngine->GameViewport)
	{
		UWorld* World = GEngine->GameViewport->GetWorld();
		if (World)
		{
			return UQuestSubsystem::Get(World);
		}
	}
	return nullptr;
}

// ============================================
// QUEST MANAGEMENT
// ============================================

bool UQuestLibrary::StartQuest(const FString& QuestID)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		return QS->StartQuest(QuestID);
	}
	return false;
}

bool UQuestLibrary::CompleteQuest(const FString& QuestID)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		return QS->CompleteQuest(QuestID);
	}
	return false;
}

bool UQuestLibrary::FailQuest(const FString& QuestID)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		return QS->FailQuest(QuestID);
	}
	return false;
}

bool UQuestLibrary::AbandonQuest(const FString& QuestID)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		return QS->AbandonQuest(QuestID);
	}
	return false;
}

bool UQuestLibrary::GetQuest(const FString& QuestID, FQuestData& OutQuestData)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		// First check active quests
		if (FQuestData* ActiveQuest = QS->GetActiveQuest(QuestID))
		{
			OutQuestData = *ActiveQuest;
			return true;
		}

		// Then check registry
		if (const FQuestData* Definition = QS->GetQuestDefinition(QuestID))
		{
			OutQuestData = *Definition;
			return true;
		}
	}
	return false;
}

bool UQuestLibrary::IsQuestActive(const FString& QuestID)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		return QS->IsQuestActive(QuestID);
	}
	return false;
}

bool UQuestLibrary::IsQuestCompleted(const FString& QuestID)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		return QS->IsQuestCompleted(QuestID);
	}
	return false;
}

bool UQuestLibrary::CanAcceptQuest(const FString& QuestID)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		// Must meet requirements and not already active/completed
		if (QS->IsQuestActive(QuestID))
		{
			return false;
		}

		const FQuestData* Definition = QS->GetQuestDefinition(QuestID);
		if (!Definition)
		{
			return false;
		}

		if (QS->IsQuestCompleted(QuestID) && !Definition->bRepeatable)
		{
			return false;
		}

		return QS->MeetsRequirements(QuestID);
	}
	return false;
}

// ============================================
// OBJECTIVE TRACKING
// ============================================

void UQuestLibrary::ReportProgress(EObjectiveType ObjectiveType, const FString& TargetID, int32 Count)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		QS->ReportProgress(ObjectiveType, TargetID, Count);
	}
}

void UQuestLibrary::ReportLocationEntered(const FString& LocationID)
{
	ReportProgress(EObjectiveType::Explore, LocationID, 1);
}

void UQuestLibrary::ReportNPCTalked(const FString& NPCID)
{
	ReportProgress(EObjectiveType::Talk, NPCID, 1);
}

void UQuestLibrary::ReportItemDelivered(const FString& ItemID, const FString& NPCID)
{
	// Report as Deliver objective with combined target
	ReportProgress(EObjectiveType::Deliver, ItemID + TEXT("@") + NPCID, 1);
}

// ============================================
// QUEST QUERIES
// ============================================

TArray<FQuestData> UQuestLibrary::GetActiveQuests()
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		return QS->GetActiveQuests();
	}
	return TArray<FQuestData>();
}

TArray<FString> UQuestLibrary::GetCompletedQuestIDs()
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		return QS->GetCompletedQuestIDs();
	}
	return TArray<FString>();
}

TArray<FQuestData> UQuestLibrary::GetNPCAvailableQuests(const FString& NPCID)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		return QS->GetQuestsFromNPC(NPCID);
	}
	return TArray<FQuestData>();
}

TArray<FQuestData> UQuestLibrary::GetNPCCompletableQuests(const FString& NPCID)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		return QS->GetQuestsCompletableAtNPC(NPCID);
	}
	return TArray<FQuestData>();
}

void UQuestLibrary::GetNPCQuestStatus(const FString& NPCID, bool& OutHasNewQuest, bool& OutHasActiveQuest, bool& OutHasCompletableQuest)
{
	OutHasNewQuest = false;
	OutHasActiveQuest = false;
	OutHasCompletableQuest = false;

	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		OutHasNewQuest = QS->GetQuestsFromNPC(NPCID).Num() > 0;
		OutHasCompletableQuest = QS->GetQuestsCompletableAtNPC(NPCID).Num() > 0;

		// Check if NPC is involved in any active quest
		for (const FQuestData& Quest : QS->GetActiveQuests())
		{
			if (Quest.QuestGiverID == NPCID || Quest.TurnInNPCID == NPCID)
			{
				OutHasActiveQuest = true;
				break;
			}

			for (const FQuestObjective& Obj : Quest.Objectives)
			{
				if (Obj.Type == EObjectiveType::Talk && Obj.TargetID == NPCID)
				{
					OutHasActiveQuest = true;
					break;
				}
			}
		}
	}
}

// ============================================
// HUD TRACKING
// ============================================

void UQuestLibrary::TrackQuest(const FString& QuestID)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		QS->TrackQuest(QuestID);
	}
}

void UQuestLibrary::UntrackQuest(const FString& QuestID)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		QS->UntrackQuest(QuestID);
	}
}

TArray<FQuestData> UQuestLibrary::GetTrackedQuests()
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		return QS->GetTrackedQuests();
	}
	return TArray<FQuestData>();
}

// ============================================
// QUEST DATA MANAGEMENT
// ============================================

void UQuestLibrary::RegisterQuest(const FQuestData& QuestData)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		QS->RegisterQuest(QuestData);
	}
}

int32 UQuestLibrary::LoadQuestsFromJSON(const FString& FilePath)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load quest file: %s"), *FilePath);
		return 0;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse quest file: %s"), *FilePath);
		return 0;
	}

	UQuestSubsystem* QS = GetQuestSubsystem();
	if (!QS)
	{
		return 0;
	}

	const TArray<TSharedPtr<FJsonValue>>* QuestsArray;
	if (!Root->TryGetArrayField(TEXT("quests"), QuestsArray))
	{
		UE_LOG(LogTemp, Warning, TEXT("No 'quests' array in file: %s"), *FilePath);
		return 0;
	}

	int32 LoadedCount = 0;

	for (const TSharedPtr<FJsonValue>& QuestVal : *QuestsArray)
	{
		const TSharedPtr<FJsonObject>* QuestObj;
		if (!QuestVal->TryGetObject(QuestObj))
		{
			continue;
		}

		FQuestData Quest;

		// Parse basic fields
		Quest.QuestID = (*QuestObj)->GetStringField(TEXT("id"));
		Quest.Title = FText::FromString((*QuestObj)->GetStringField(TEXT("title")));
		Quest.Description = FText::FromString((*QuestObj)->GetStringField(TEXT("description")));
		Quest.QuestGiverID = (*QuestObj)->GetStringField(TEXT("giver"));
		Quest.TurnInNPCID = (*QuestObj)->GetStringField(TEXT("turnin"));
		Quest.SuggestedLevel = (*QuestObj)->GetIntegerField(TEXT("level"));
		Quest.bRepeatable = (*QuestObj)->GetBoolField(TEXT("repeatable"));

		// Parse quest type
		FString TypeStr = (*QuestObj)->GetStringField(TEXT("type"));
		if (TypeStr == TEXT("main")) Quest.QuestType = EQuestType::Main;
		else if (TypeStr == TEXT("side")) Quest.QuestType = EQuestType::Side;
		else if (TypeStr == TEXT("daily")) Quest.QuestType = EQuestType::Daily;
		else if (TypeStr == TEXT("discovery")) Quest.QuestType = EQuestType::Discovery;
		else if (TypeStr == TEXT("event")) Quest.QuestType = EQuestType::Event;

		// Parse objectives
		const TArray<TSharedPtr<FJsonValue>>* ObjArray;
		if ((*QuestObj)->TryGetArrayField(TEXT("objectives"), ObjArray))
		{
			for (const TSharedPtr<FJsonValue>& ObjVal : *ObjArray)
			{
				const TSharedPtr<FJsonObject>* ObjData;
				if (ObjVal->TryGetObject(ObjData))
				{
					FQuestObjective Obj;
					Obj.ObjectiveID = (*ObjData)->GetStringField(TEXT("id"));
					Obj.Description = FText::FromString((*ObjData)->GetStringField(TEXT("description")));
					Obj.TargetID = (*ObjData)->GetStringField(TEXT("target"));
					Obj.RequiredCount = (*ObjData)->GetIntegerField(TEXT("count"));
					Obj.bOptional = (*ObjData)->GetBoolField(TEXT("optional"));
					Obj.bHidden = (*ObjData)->GetBoolField(TEXT("hidden"));

					FString ObjType = (*ObjData)->GetStringField(TEXT("type"));
					if (ObjType == TEXT("kill")) Obj.Type = EObjectiveType::Kill;
					else if (ObjType == TEXT("collect")) Obj.Type = EObjectiveType::Collect;
					else if (ObjType == TEXT("deliver")) Obj.Type = EObjectiveType::Deliver;
					else if (ObjType == TEXT("escort")) Obj.Type = EObjectiveType::Escort;
					else if (ObjType == TEXT("talk")) Obj.Type = EObjectiveType::Talk;
					else if (ObjType == TEXT("explore")) Obj.Type = EObjectiveType::Explore;
					else if (ObjType == TEXT("interact")) Obj.Type = EObjectiveType::Interact;
					else if (ObjType == TEXT("survive")) Obj.Type = EObjectiveType::Survive;

					Quest.Objectives.Add(Obj);
				}
			}
		}

		// Parse rewards
		const TSharedPtr<FJsonObject>* RewardsObj;
		if ((*QuestObj)->TryGetObjectField(TEXT("rewards"), RewardsObj))
		{
			Quest.Rewards.XP = (*RewardsObj)->GetIntegerField(TEXT("xp"));
			Quest.Rewards.Gold = (*RewardsObj)->GetIntegerField(TEXT("gold"));

			const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
			if ((*RewardsObj)->TryGetArrayField(TEXT("items"), ItemsArray))
			{
				for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArray)
				{
					Quest.Rewards.ItemIDs.Add(ItemVal->AsString());
				}
			}
		}

		// Parse requirements
		const TSharedPtr<FJsonObject>* ReqObj;
		if ((*QuestObj)->TryGetObjectField(TEXT("requirements"), ReqObj))
		{
			Quest.Requirements.RequiredLevel = (*ReqObj)->GetIntegerField(TEXT("level"));

			const TArray<TSharedPtr<FJsonValue>>* ReqQuests;
			if ((*ReqObj)->TryGetArrayField(TEXT("quests"), ReqQuests))
			{
				for (const TSharedPtr<FJsonValue>& QVal : *ReqQuests)
				{
					Quest.Requirements.RequiredQuests.Add(QVal->AsString());
				}
			}
		}

		QS->RegisterQuest(Quest);
		LoadedCount++;
	}

	UE_LOG(LogTemp, Log, TEXT("Loaded %d quests from %s"), LoadedCount, *FilePath);
	return LoadedCount;
}

FString UQuestLibrary::GetQuestContextForNPC(const FString& NPCID)
{
	if (UQuestSubsystem* QS = GetQuestSubsystem())
	{
		return QS->GetQuestContextForNPC(NPCID);
	}
	return TEXT("{}");
}
