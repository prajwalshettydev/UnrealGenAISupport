// GenQuestUtils Implementation
// Part of GenerativeAISupport Plugin

#include "GenQuestUtils.h"
#include "Quest/QuestSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"

// Static member initialization
TMap<FString, FQuestTemplate> UGenQuestUtils::QuestTemplates;
int32 UGenQuestUtils::QuestIDCounter = 0;

// ============================================
// QUEST CREATION
// ============================================

FQuestData UGenQuestUtils::CreateQuest(
	const FString& QuestID,
	const FText& Title,
	const FText& Description,
	EQuestType QuestType,
	const FString& QuestGiverID)
{
	FQuestData Quest;
	Quest.QuestID = QuestID;
	Quest.Title = Title;
	Quest.Description = Description;
	Quest.QuestType = QuestType;
	Quest.QuestGiverID = QuestGiverID;
	Quest.Status = EQuestStatus::Locked;
	Quest.SuggestedLevel = 1;
	Quest.bRepeatable = false;

	return Quest;
}

bool UGenQuestUtils::CreateQuestFromJSON(const FString& JSONString, FQuestData& OutQuest)
{
	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("GenQuestUtils: Failed to parse quest JSON"));
		return false;
	}

	// Parse basic fields
	OutQuest.QuestID = JsonObj->GetStringField(TEXT("id"));
	OutQuest.Title = FText::FromString(JsonObj->GetStringField(TEXT("title")));
	OutQuest.Description = FText::FromString(JsonObj->GetStringField(TEXT("description")));
	OutQuest.QuestGiverID = JsonObj->GetStringField(TEXT("giver"));
	OutQuest.TurnInNPCID = JsonObj->GetStringField(TEXT("turnin"));
	OutQuest.SuggestedLevel = JsonObj->GetIntegerField(TEXT("level"));
	OutQuest.bRepeatable = JsonObj->GetBoolField(TEXT("repeatable"));
	OutQuest.TimeLimit = JsonObj->GetNumberField(TEXT("time_limit"));

	// Parse quest type
	FString TypeStr = JsonObj->GetStringField(TEXT("type"));
	if (TypeStr == TEXT("main")) OutQuest.QuestType = EQuestType::Main;
	else if (TypeStr == TEXT("side")) OutQuest.QuestType = EQuestType::Side;
	else if (TypeStr == TEXT("daily")) OutQuest.QuestType = EQuestType::Daily;
	else if (TypeStr == TEXT("discovery")) OutQuest.QuestType = EQuestType::Discovery;
	else if (TypeStr == TEXT("event")) OutQuest.QuestType = EQuestType::Event;
	else OutQuest.QuestType = EQuestType::Side;

	// Parse objectives
	const TArray<TSharedPtr<FJsonValue>>* ObjArray;
	if (JsonObj->TryGetArrayField(TEXT("objectives"), ObjArray))
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

				OutQuest.Objectives.Add(Obj);
			}
		}
	}

	// Parse rewards
	const TSharedPtr<FJsonObject>* RewardsObj;
	if (JsonObj->TryGetObjectField(TEXT("rewards"), RewardsObj))
	{
		OutQuest.Rewards.XP = (*RewardsObj)->GetIntegerField(TEXT("xp"));
		OutQuest.Rewards.Gold = (*RewardsObj)->GetIntegerField(TEXT("gold"));

		const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
		if ((*RewardsObj)->TryGetArrayField(TEXT("items"), ItemsArray))
		{
			for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArray)
			{
				OutQuest.Rewards.ItemIDs.Add(ItemVal->AsString());
			}
		}

		const TSharedPtr<FJsonObject>* RepObj;
		if ((*RewardsObj)->TryGetObjectField(TEXT("reputation"), RepObj))
		{
			for (const auto& Pair : (*RepObj)->Values)
			{
				OutQuest.Rewards.ReputationChanges.Add(Pair.Key, (int32)Pair.Value->AsNumber());
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* UnlocksArray;
		if ((*RewardsObj)->TryGetArrayField(TEXT("unlocks"), UnlocksArray))
		{
			for (const TSharedPtr<FJsonValue>& Val : *UnlocksArray)
			{
				OutQuest.Rewards.UnlocksQuests.Add(Val->AsString());
			}
		}
	}

	// Parse requirements
	const TSharedPtr<FJsonObject>* ReqObj;
	if (JsonObj->TryGetObjectField(TEXT("requirements"), ReqObj))
	{
		OutQuest.Requirements.RequiredLevel = (*ReqObj)->GetIntegerField(TEXT("level"));

		const TArray<TSharedPtr<FJsonValue>>* ReqQuests;
		if ((*ReqObj)->TryGetArrayField(TEXT("quests"), ReqQuests))
		{
			for (const TSharedPtr<FJsonValue>& QVal : *ReqQuests)
			{
				OutQuest.Requirements.RequiredQuests.Add(QVal->AsString());
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* ReqItems;
		if ((*ReqObj)->TryGetArrayField(TEXT("items"), ReqItems))
		{
			for (const TSharedPtr<FJsonValue>& IVal : *ReqItems)
			{
				OutQuest.Requirements.RequiredItems.Add(IVal->AsString());
			}
		}
	}

	return true;
}

TArray<FQuestData> UGenQuestUtils::CreateQuestsFromJSON(const FString& JSONString)
{
	TArray<FQuestData> Result;

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("GenQuestUtils: Failed to parse quests JSON"));
		return Result;
	}

	const TArray<TSharedPtr<FJsonValue>>* QuestsArray;
	if (!Root->TryGetArrayField(TEXT("quests"), QuestsArray))
	{
		// Try parsing as a direct array
		TSharedPtr<FJsonValue> ArrayValue;
		TSharedRef<TJsonReader<>> ArrayReader = TJsonReaderFactory<>::Create(JSONString);
		if (FJsonSerializer::Deserialize(ArrayReader, ArrayValue) && ArrayValue->Type == EJson::Array)
		{
			for (const TSharedPtr<FJsonValue>& Val : ArrayValue->AsArray())
			{
				FString QuestJSON;
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&QuestJSON);
				FJsonSerializer::Serialize(Val->AsObject().ToSharedRef(), Writer);

				FQuestData Quest;
				if (CreateQuestFromJSON(QuestJSON, Quest))
				{
					Result.Add(Quest);
				}
			}
		}
		return Result;
	}

	for (const TSharedPtr<FJsonValue>& QuestVal : *QuestsArray)
	{
		FString QuestJSON;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&QuestJSON);
		FJsonSerializer::Serialize(QuestVal->AsObject().ToSharedRef(), Writer);

		FQuestData Quest;
		if (CreateQuestFromJSON(QuestJSON, Quest))
		{
			Result.Add(Quest);
		}
	}

	return Result;
}

// ============================================
// OBJECTIVE MANAGEMENT
// ============================================

FQuestData UGenQuestUtils::AddObjective(
	FQuestData& Quest,
	const FString& ObjectiveID,
	EObjectiveType Type,
	const FText& Description,
	const FString& TargetID,
	int32 RequiredCount,
	bool bOptional,
	bool bHidden)
{
	FQuestObjective Obj;
	Obj.ObjectiveID = ObjectiveID;
	Obj.Type = Type;
	Obj.Description = Description;
	Obj.TargetID = TargetID;
	Obj.RequiredCount = RequiredCount;
	Obj.bOptional = bOptional;
	Obj.bHidden = bHidden;
	Obj.CurrentCount = 0;

	Quest.Objectives.Add(Obj);
	return Quest;
}

FQuestObjective UGenQuestUtils::MakeKillObjective(const FString& ObjectiveID, const FText& Description, const FString& MonsterID, int32 Count)
{
	FQuestObjective Obj;
	Obj.ObjectiveID = ObjectiveID;
	Obj.Type = EObjectiveType::Kill;
	Obj.Description = Description;
	Obj.TargetID = MonsterID;
	Obj.RequiredCount = Count;
	return Obj;
}

FQuestObjective UGenQuestUtils::MakeCollectObjective(const FString& ObjectiveID, const FText& Description, const FString& ItemID, int32 Count)
{
	FQuestObjective Obj;
	Obj.ObjectiveID = ObjectiveID;
	Obj.Type = EObjectiveType::Collect;
	Obj.Description = Description;
	Obj.TargetID = ItemID;
	Obj.RequiredCount = Count;
	return Obj;
}

FQuestObjective UGenQuestUtils::MakeTalkObjective(const FString& ObjectiveID, const FText& Description, const FString& NPCID)
{
	FQuestObjective Obj;
	Obj.ObjectiveID = ObjectiveID;
	Obj.Type = EObjectiveType::Talk;
	Obj.Description = Description;
	Obj.TargetID = NPCID;
	Obj.RequiredCount = 1;
	return Obj;
}

FQuestObjective UGenQuestUtils::MakeExploreObjective(const FString& ObjectiveID, const FText& Description, const FString& LocationID)
{
	FQuestObjective Obj;
	Obj.ObjectiveID = ObjectiveID;
	Obj.Type = EObjectiveType::Explore;
	Obj.Description = Description;
	Obj.TargetID = LocationID;
	Obj.RequiredCount = 1;
	return Obj;
}

FQuestObjective UGenQuestUtils::MakeDeliverObjective(const FString& ObjectiveID, const FText& Description, const FString& ItemID, const FString& NPCID)
{
	FQuestObjective Obj;
	Obj.ObjectiveID = ObjectiveID;
	Obj.Type = EObjectiveType::Deliver;
	Obj.Description = Description;
	Obj.TargetID = ItemID + TEXT("@") + NPCID;
	Obj.RequiredCount = 1;
	return Obj;
}

bool UGenQuestUtils::RemoveObjective(FQuestData& Quest, const FString& ObjectiveID)
{
	int32 IndexToRemove = -1;
	for (int32 i = 0; i < Quest.Objectives.Num(); ++i)
	{
		if (Quest.Objectives[i].ObjectiveID == ObjectiveID)
		{
			IndexToRemove = i;
			break;
		}
	}

	if (IndexToRemove >= 0)
	{
		Quest.Objectives.RemoveAt(IndexToRemove);
		return true;
	}

	return false;
}

// ============================================
// REWARD MANAGEMENT
// ============================================

FQuestData UGenQuestUtils::SetRewards(FQuestData& Quest, int32 XP, int32 Gold, const TArray<FString>& ItemIDs)
{
	Quest.Rewards.XP = XP;
	Quest.Rewards.Gold = Gold;
	Quest.Rewards.ItemIDs = ItemIDs;
	return Quest;
}

FQuestData UGenQuestUtils::AddItemReward(FQuestData& Quest, const FString& ItemID)
{
	Quest.Rewards.ItemIDs.AddUnique(ItemID);
	return Quest;
}

FQuestData UGenQuestUtils::AddReputationReward(FQuestData& Quest, const FString& FactionID, int32 Amount)
{
	Quest.Rewards.ReputationChanges.Add(FactionID, Amount);
	return Quest;
}

FQuestData UGenQuestUtils::SetQuestUnlocks(FQuestData& Quest, const TArray<FString>& QuestIDs)
{
	Quest.Rewards.UnlocksQuests = QuestIDs;
	return Quest;
}

// ============================================
// REQUIREMENTS
// ============================================

FQuestData UGenQuestUtils::SetRequirements(FQuestData& Quest, int32 RequiredLevel, const TArray<FString>& RequiredQuests)
{
	Quest.Requirements.RequiredLevel = RequiredLevel;
	Quest.Requirements.RequiredQuests = RequiredQuests;
	return Quest;
}

FQuestData UGenQuestUtils::AddPrerequisiteQuest(FQuestData& Quest, const FString& QuestID)
{
	Quest.Requirements.RequiredQuests.AddUnique(QuestID);
	return Quest;
}

FQuestData UGenQuestUtils::AddRequiredItem(FQuestData& Quest, const FString& ItemID)
{
	Quest.Requirements.RequiredItems.AddUnique(ItemID);
	return Quest;
}

// ============================================
// QUEST CHAINS
// ============================================

TArray<FQuestData> UGenQuestUtils::CreateQuestChain(
	const FString& ChainID,
	const TArray<FQuestChainEntry>& Entries,
	const FString& QuestGiverID,
	EQuestType QuestType)
{
	TArray<FQuestData> Quests;
	FString PreviousQuestID;

	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FQuestChainEntry& Entry = Entries[i];

		FQuestData Quest;
		Quest.QuestID = Entry.QuestID.IsEmpty() ? FString::Printf(TEXT("%s_%02d"), *ChainID, i + 1) : Entry.QuestID;
		Quest.Title = Entry.Title;
		Quest.Description = Entry.Description;
		Quest.QuestType = QuestType;
		Quest.QuestGiverID = QuestGiverID;
		Quest.Objectives = Entry.Objectives;
		Quest.Rewards = Entry.Rewards;

		// Set prerequisite to previous quest
		if (!PreviousQuestID.IsEmpty())
		{
			Quest.Requirements.RequiredQuests.Add(PreviousQuestID);
		}

		// Add unlock for next quest in rewards
		if (i < Entries.Num() - 1)
		{
			FString NextQuestID = Entries[i + 1].QuestID.IsEmpty()
				? FString::Printf(TEXT("%s_%02d"), *ChainID, i + 2)
				: Entries[i + 1].QuestID;
			Quest.Rewards.UnlocksQuests.Add(NextQuestID);
		}

		Quests.Add(Quest);
		PreviousQuestID = Quest.QuestID;
	}

	return Quests;
}

TArray<FQuestData> UGenQuestUtils::LinkQuestsAsChain(TArray<FQuestData>& Quests)
{
	for (int32 i = 1; i < Quests.Num(); ++i)
	{
		// Each quest requires the previous one
		Quests[i].Requirements.RequiredQuests.AddUnique(Quests[i - 1].QuestID);

		// Previous quest unlocks current one
		Quests[i - 1].Rewards.UnlocksQuests.AddUnique(Quests[i].QuestID);
	}

	return Quests;
}

// ============================================
// EXPORT / SAVE
// ============================================

FString UGenQuestUtils::ExportQuestToJSON(const FQuestData& Quest)
{
	TSharedPtr<FJsonObject> QuestObj = MakeShared<FJsonObject>();

	// Basic fields
	QuestObj->SetStringField(TEXT("id"), Quest.QuestID);
	QuestObj->SetStringField(TEXT("title"), Quest.Title.ToString());
	QuestObj->SetStringField(TEXT("description"), Quest.Description.ToString());
	QuestObj->SetStringField(TEXT("giver"), Quest.QuestGiverID);
	QuestObj->SetStringField(TEXT("turnin"), Quest.TurnInNPCID);
	QuestObj->SetNumberField(TEXT("level"), Quest.SuggestedLevel);
	QuestObj->SetBoolField(TEXT("repeatable"), Quest.bRepeatable);
	QuestObj->SetNumberField(TEXT("time_limit"), Quest.TimeLimit);

	// Quest type
	FString TypeStr;
	switch (Quest.QuestType)
	{
		case EQuestType::Main: TypeStr = TEXT("main"); break;
		case EQuestType::Side: TypeStr = TEXT("side"); break;
		case EQuestType::Daily: TypeStr = TEXT("daily"); break;
		case EQuestType::Discovery: TypeStr = TEXT("discovery"); break;
		case EQuestType::Event: TypeStr = TEXT("event"); break;
		default: TypeStr = TEXT("side"); break;
	}
	QuestObj->SetStringField(TEXT("type"), TypeStr);

	// Objectives
	TArray<TSharedPtr<FJsonValue>> ObjArray;
	for (const FQuestObjective& Obj : Quest.Objectives)
	{
		TSharedPtr<FJsonObject> ObjData = MakeShared<FJsonObject>();
		ObjData->SetStringField(TEXT("id"), Obj.ObjectiveID);
		ObjData->SetStringField(TEXT("description"), Obj.Description.ToString());
		ObjData->SetStringField(TEXT("target"), Obj.TargetID);
		ObjData->SetNumberField(TEXT("count"), Obj.RequiredCount);
		ObjData->SetBoolField(TEXT("optional"), Obj.bOptional);
		ObjData->SetBoolField(TEXT("hidden"), Obj.bHidden);

		FString ObjType;
		switch (Obj.Type)
		{
			case EObjectiveType::Kill: ObjType = TEXT("kill"); break;
			case EObjectiveType::Collect: ObjType = TEXT("collect"); break;
			case EObjectiveType::Deliver: ObjType = TEXT("deliver"); break;
			case EObjectiveType::Escort: ObjType = TEXT("escort"); break;
			case EObjectiveType::Talk: ObjType = TEXT("talk"); break;
			case EObjectiveType::Explore: ObjType = TEXT("explore"); break;
			case EObjectiveType::Interact: ObjType = TEXT("interact"); break;
			case EObjectiveType::Survive: ObjType = TEXT("survive"); break;
			default: ObjType = TEXT("kill"); break;
		}
		ObjData->SetStringField(TEXT("type"), ObjType);

		ObjArray.Add(MakeShared<FJsonValueObject>(ObjData));
	}
	QuestObj->SetArrayField(TEXT("objectives"), ObjArray);

	// Rewards
	TSharedPtr<FJsonObject> RewardsObj = MakeShared<FJsonObject>();
	RewardsObj->SetNumberField(TEXT("xp"), Quest.Rewards.XP);
	RewardsObj->SetNumberField(TEXT("gold"), Quest.Rewards.Gold);

	TArray<TSharedPtr<FJsonValue>> ItemsArray;
	for (const FString& ItemID : Quest.Rewards.ItemIDs)
	{
		ItemsArray.Add(MakeShared<FJsonValueString>(ItemID));
	}
	RewardsObj->SetArrayField(TEXT("items"), ItemsArray);

	TSharedPtr<FJsonObject> RepObj = MakeShared<FJsonObject>();
	for (const auto& Pair : Quest.Rewards.ReputationChanges)
	{
		RepObj->SetNumberField(Pair.Key, Pair.Value);
	}
	RewardsObj->SetObjectField(TEXT("reputation"), RepObj);

	TArray<TSharedPtr<FJsonValue>> UnlocksArray;
	for (const FString& QID : Quest.Rewards.UnlocksQuests)
	{
		UnlocksArray.Add(MakeShared<FJsonValueString>(QID));
	}
	RewardsObj->SetArrayField(TEXT("unlocks"), UnlocksArray);

	QuestObj->SetObjectField(TEXT("rewards"), RewardsObj);

	// Requirements
	TSharedPtr<FJsonObject> ReqObj = MakeShared<FJsonObject>();
	ReqObj->SetNumberField(TEXT("level"), Quest.Requirements.RequiredLevel);

	TArray<TSharedPtr<FJsonValue>> ReqQuests;
	for (const FString& QID : Quest.Requirements.RequiredQuests)
	{
		ReqQuests.Add(MakeShared<FJsonValueString>(QID));
	}
	ReqObj->SetArrayField(TEXT("quests"), ReqQuests);

	TArray<TSharedPtr<FJsonValue>> ReqItems;
	for (const FString& IID : Quest.Requirements.RequiredItems)
	{
		ReqItems.Add(MakeShared<FJsonValueString>(IID));
	}
	ReqObj->SetArrayField(TEXT("items"), ReqItems);

	QuestObj->SetObjectField(TEXT("requirements"), ReqObj);

	// Serialize to string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(QuestObj.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenQuestUtils::ExportQuestsToJSON(const TArray<FQuestData>& Quests)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	TArray<TSharedPtr<FJsonValue>> QuestsArray;
	for (const FQuestData& Quest : Quests)
	{
		FString QuestJSON = ExportQuestToJSON(Quest);

		TSharedPtr<FJsonObject> QuestObj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(QuestJSON);
		FJsonSerializer::Deserialize(Reader, QuestObj);

		QuestsArray.Add(MakeShared<FJsonValueObject>(QuestObj));
	}

	Root->SetArrayField(TEXT("quests"), QuestsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	return OutputString;
}

bool UGenQuestUtils::SaveQuestsToFile(const TArray<FQuestData>& Quests, const FString& FilePath)
{
	FString JSONString = ExportQuestsToJSON(Quests);
	return FFileHelper::SaveStringToFile(JSONString, *FilePath);
}

TArray<FQuestData> UGenQuestUtils::LoadQuestsFromFile(const FString& FilePath)
{
	FString JSONString;
	if (!FFileHelper::LoadFileToString(JSONString, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("GenQuestUtils: Failed to load file: %s"), *FilePath);
		return TArray<FQuestData>();
	}

	return CreateQuestsFromJSON(JSONString);
}

// ============================================
// VALIDATION
// ============================================

bool UGenQuestUtils::ValidateQuest(const FQuestData& Quest, TArray<FString>& OutErrors)
{
	OutErrors.Empty();

	// Check required fields
	if (Quest.QuestID.IsEmpty())
	{
		OutErrors.Add(TEXT("Quest ID is empty"));
	}

	if (Quest.Title.IsEmpty())
	{
		OutErrors.Add(TEXT("Quest title is empty"));
	}

	if (Quest.Description.IsEmpty())
	{
		OutErrors.Add(TEXT("Quest description is empty"));
	}

	// Check objectives
	if (Quest.Objectives.Num() == 0)
	{
		OutErrors.Add(TEXT("Quest has no objectives"));
	}

	// Check for duplicate objective IDs
	TSet<FString> ObjectiveIDs;
	for (const FQuestObjective& Obj : Quest.Objectives)
	{
		if (Obj.ObjectiveID.IsEmpty())
		{
			OutErrors.Add(TEXT("Objective has empty ID"));
		}
		else if (ObjectiveIDs.Contains(Obj.ObjectiveID))
		{
			OutErrors.Add(FString::Printf(TEXT("Duplicate objective ID: %s"), *Obj.ObjectiveID));
		}
		else
		{
			ObjectiveIDs.Add(Obj.ObjectiveID);
		}

		if (Obj.RequiredCount <= 0)
		{
			OutErrors.Add(FString::Printf(TEXT("Objective %s has invalid count: %d"), *Obj.ObjectiveID, Obj.RequiredCount));
		}
	}

	// Check if all objectives are optional
	bool bHasRequiredObjective = false;
	for (const FQuestObjective& Obj : Quest.Objectives)
	{
		if (!Obj.bOptional)
		{
			bHasRequiredObjective = true;
			break;
		}
	}

	if (!bHasRequiredObjective && Quest.Objectives.Num() > 0)
	{
		OutErrors.Add(TEXT("All objectives are optional - quest cannot be completed"));
	}

	return OutErrors.Num() == 0;
}

bool UGenQuestUtils::ValidateQuestChain(const TArray<FQuestData>& Quests, TArray<FString>& OutErrors)
{
	OutErrors.Empty();

	// Check for duplicate IDs
	TSet<FString> QuestIDs;
	for (const FQuestData& Quest : Quests)
	{
		if (QuestIDs.Contains(Quest.QuestID))
		{
			OutErrors.Add(FString::Printf(TEXT("Duplicate quest ID: %s"), *Quest.QuestID));
		}
		else
		{
			QuestIDs.Add(Quest.QuestID);
		}

		// Validate each quest
		TArray<FString> QuestErrors;
		if (!ValidateQuest(Quest, QuestErrors))
		{
			for (const FString& Error : QuestErrors)
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] %s"), *Quest.QuestID, *Error));
			}
		}
	}

	// Check for circular dependencies
	for (const FQuestData& Quest : Quests)
	{
		for (const FString& ReqQuestID : Quest.Requirements.RequiredQuests)
		{
			// Check if required quest also requires this quest
			for (const FQuestData& OtherQuest : Quests)
			{
				if (OtherQuest.QuestID == ReqQuestID)
				{
					if (OtherQuest.Requirements.RequiredQuests.Contains(Quest.QuestID))
					{
						OutErrors.Add(FString::Printf(TEXT("Circular dependency: %s <-> %s"), *Quest.QuestID, *ReqQuestID));
					}
					break;
				}
			}
		}
	}

	// Check for missing prerequisites
	for (const FQuestData& Quest : Quests)
	{
		for (const FString& ReqQuestID : Quest.Requirements.RequiredQuests)
		{
			if (!QuestIDs.Contains(ReqQuestID))
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] Required quest not in chain: %s"), *Quest.QuestID, *ReqQuestID));
			}
		}
	}

	return OutErrors.Num() == 0;
}

FString UGenQuestUtils::GenerateQuestID(const FString& Prefix)
{
	++QuestIDCounter;
	return FString::Printf(TEXT("%s_%08X_%04d"), *Prefix, FGuid::NewGuid().A, QuestIDCounter);
}

// ============================================
// TEMPLATES
// ============================================

void UGenQuestUtils::InitializeTemplates()
{
	if (QuestTemplates.Num() > 0)
	{
		return; // Already initialized
	}

	// Kill Quest Template
	FQuestTemplate KillTemplate;
	KillTemplate.TemplateName = TEXT("kill");
	KillTemplate.DefaultType = EQuestType::Side;
	KillTemplate.ObjectiveTypes.Add(EObjectiveType::Kill);
	KillTemplate.DefaultXPReward = 100;
	KillTemplate.DefaultGoldReward = 25;
	QuestTemplates.Add(TEXT("kill"), KillTemplate);

	// Fetch Quest Template
	FQuestTemplate FetchTemplate;
	FetchTemplate.TemplateName = TEXT("fetch");
	FetchTemplate.DefaultType = EQuestType::Side;
	FetchTemplate.ObjectiveTypes.Add(EObjectiveType::Collect);
	FetchTemplate.ObjectiveTypes.Add(EObjectiveType::Deliver);
	FetchTemplate.DefaultXPReward = 75;
	FetchTemplate.DefaultGoldReward = 50;
	QuestTemplates.Add(TEXT("fetch"), FetchTemplate);

	// Exploration Template
	FQuestTemplate ExploreTemplate;
	ExploreTemplate.TemplateName = TEXT("explore");
	ExploreTemplate.DefaultType = EQuestType::Discovery;
	ExploreTemplate.ObjectiveTypes.Add(EObjectiveType::Explore);
	ExploreTemplate.DefaultXPReward = 150;
	ExploreTemplate.DefaultGoldReward = 0;
	QuestTemplates.Add(TEXT("explore"), ExploreTemplate);

	// Talk/Messenger Template
	FQuestTemplate TalkTemplate;
	TalkTemplate.TemplateName = TEXT("messenger");
	TalkTemplate.DefaultType = EQuestType::Side;
	TalkTemplate.ObjectiveTypes.Add(EObjectiveType::Talk);
	TalkTemplate.DefaultXPReward = 50;
	TalkTemplate.DefaultGoldReward = 30;
	QuestTemplates.Add(TEXT("messenger"), TalkTemplate);

	// Escort Template
	FQuestTemplate EscortTemplate;
	EscortTemplate.TemplateName = TEXT("escort");
	EscortTemplate.DefaultType = EQuestType::Side;
	EscortTemplate.ObjectiveTypes.Add(EObjectiveType::Escort);
	EscortTemplate.DefaultXPReward = 200;
	EscortTemplate.DefaultGoldReward = 75;
	QuestTemplates.Add(TEXT("escort"), EscortTemplate);

	// Main Story Template
	FQuestTemplate MainTemplate;
	MainTemplate.TemplateName = TEXT("main_story");
	MainTemplate.DefaultType = EQuestType::Main;
	MainTemplate.ObjectiveTypes.Add(EObjectiveType::Talk);
	MainTemplate.ObjectiveTypes.Add(EObjectiveType::Kill);
	MainTemplate.ObjectiveTypes.Add(EObjectiveType::Explore);
	MainTemplate.DefaultXPReward = 500;
	MainTemplate.DefaultGoldReward = 200;
	QuestTemplates.Add(TEXT("main_story"), MainTemplate);

	// Daily Quest Template
	FQuestTemplate DailyTemplate;
	DailyTemplate.TemplateName = TEXT("daily");
	DailyTemplate.DefaultType = EQuestType::Daily;
	DailyTemplate.ObjectiveTypes.Add(EObjectiveType::Kill);
	DailyTemplate.DefaultXPReward = 50;
	DailyTemplate.DefaultGoldReward = 100;
	QuestTemplates.Add(TEXT("daily"), DailyTemplate);
}

TArray<FString> UGenQuestUtils::GetQuestTemplates()
{
	InitializeTemplates();

	TArray<FString> TemplateNames;
	QuestTemplates.GetKeys(TemplateNames);
	return TemplateNames;
}

FQuestData UGenQuestUtils::CreateQuestFromTemplate(
	const FString& TemplateName,
	const FString& QuestID,
	const FText& Title,
	const FText& Description)
{
	InitializeTemplates();

	FQuestData Quest = CreateQuest(QuestID, Title, Description);

	const FQuestTemplate* Template = QuestTemplates.Find(TemplateName);
	if (Template)
	{
		Quest.QuestType = Template->DefaultType;
		Quest.Rewards.XP = Template->DefaultXPReward;
		Quest.Rewards.Gold = Template->DefaultGoldReward;

		// Add placeholder objectives based on template
		int32 ObjIndex = 1;
		for (EObjectiveType ObjType : Template->ObjectiveTypes)
		{
			FQuestObjective Obj;
			Obj.ObjectiveID = FString::Printf(TEXT("obj_%02d"), ObjIndex++);
			Obj.Type = ObjType;
			Obj.Description = FText::FromString(TEXT("TODO: Set objective description"));
			Obj.TargetID = TEXT("TODO_TARGET");
			Obj.RequiredCount = 1;
			Quest.Objectives.Add(Obj);
		}

		// Daily quests are repeatable
		if (Template->DefaultType == EQuestType::Daily)
		{
			Quest.bRepeatable = true;
			Quest.RepeatCooldown = 86400.f; // 24 hours
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GenQuestUtils: Template not found: %s"), *TemplateName);
	}

	return Quest;
}

// ============================================
// BATCH OPERATIONS
// ============================================

int32 UGenQuestUtils::RegisterQuests(const TArray<FQuestData>& Quests, const UObject* WorldContextObject)
{
	UQuestSubsystem* QS = UQuestSubsystem::Get(WorldContextObject);
	if (!QS)
	{
		UE_LOG(LogTemp, Warning, TEXT("GenQuestUtils: Quest Subsystem not available"));
		return 0;
	}

	int32 RegisteredCount = 0;
	for (const FQuestData& Quest : Quests)
	{
		QS->RegisterQuest(Quest);
		RegisteredCount++;
	}

	return RegisteredCount;
}

FQuestData UGenQuestUtils::DuplicateQuest(const FQuestData& SourceQuest, const FString& NewQuestID)
{
	FQuestData NewQuest = SourceQuest;
	NewQuest.QuestID = NewQuestID;
	NewQuest.Status = EQuestStatus::Locked;

	// Reset objective progress
	for (FQuestObjective& Obj : NewQuest.Objectives)
	{
		Obj.CurrentCount = 0;
	}

	// Clear requirements/unlocks that reference the old quest
	NewQuest.Requirements.RequiredQuests.Remove(SourceQuest.QuestID);
	NewQuest.Rewards.UnlocksQuests.Remove(SourceQuest.QuestID);

	return NewQuest;
}

FQuestData UGenQuestUtils::ScaleRewards(FQuestData& Quest, float Multiplier)
{
	Quest.Rewards.XP = FMath::RoundToInt(Quest.Rewards.XP * Multiplier);
	Quest.Rewards.Gold = FMath::RoundToInt(Quest.Rewards.Gold * Multiplier);

	for (auto& Pair : Quest.Rewards.ReputationChanges)
	{
		Pair.Value = FMath::RoundToInt(Pair.Value * Multiplier);
	}

	return Quest;
}

FQuestData UGenQuestUtils::ScaleObjectiveCounts(FQuestData& Quest, float Multiplier)
{
	for (FQuestObjective& Obj : Quest.Objectives)
	{
		Obj.RequiredCount = FMath::Max(1, FMath::RoundToInt(Obj.RequiredCount * Multiplier));
	}

	return Quest;
}
