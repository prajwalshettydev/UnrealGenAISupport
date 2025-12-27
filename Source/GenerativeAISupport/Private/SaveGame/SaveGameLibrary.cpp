// SaveGame Library Implementation
// Part of GenerativeAISupport Plugin

#include "SaveGame/SaveGameLibrary.h"
#include "SaveGame/SaveGameTypes.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/InventoryComponent.h"
#include "Quest/QuestSubsystem.h"
#include "Achievement/AchievementLibrary.h"
#include "Shop/ShopLibrary.h"
#include "Async/Async.h"

// Static member initialization
FDateTime USaveGameLibrary::SessionStartTime = FDateTime::Now();
TMap<FString, int32> USaveGameLibrary::RuntimeStatistics;
int32 USaveGameLibrary::CurrentAutosaveIndex = 0;

// UTitanSaveGame constructor
UTitanSaveGame::UTitanSaveGame()
{
	SaveVersion = CURRENT_SAVE_VERSION;
}

// ============================================
// SAVE OPERATIONS
// ============================================

FSaveLoadResult USaveGameLibrary::SaveGame(
	int32 SlotIndex,
	const FString& SaveName,
	const FPlayerSaveData& PlayerData,
	const FWorldSaveData& WorldData,
	const UObject* WorldContext)
{
	FSaveLoadResult Result;
	Result.SlotIndex = SlotIndex;

	double StartTime = FPlatformTime::Seconds();

	// Validate slot index
	if (SlotIndex < 0 || (SlotIndex >= MAX_SAVE_SLOTS && SlotIndex != QUICKSAVE_SLOT &&
		(SlotIndex < AUTOSAVE_SLOT_START || SlotIndex >= AUTOSAVE_SLOT_START + AUTOSAVE_SLOT_COUNT)))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FText::FromString(TEXT("Invalid save slot index"));
		return Result;
	}

	// Create save game object
	UTitanSaveGame* SaveGame = Cast<UTitanSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UTitanSaveGame::StaticClass())
	);

	if (!SaveGame)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FText::FromString(TEXT("Failed to create save game object"));
		return Result;
	}

	// Populate slot info
	SaveGame->SlotInfo.SlotIndex = SlotIndex;
	SaveGame->SlotInfo.SaveName = SaveName.IsEmpty() ? FString::Printf(TEXT("Save %d"), SlotIndex + 1) : SaveName;
	SaveGame->SlotInfo.PlayerName = PlayerData.PlayerName;
	SaveGame->SlotInfo.PlayerLevel = PlayerData.Level;
	SaveGame->SlotInfo.LocationName = PlayerData.CurrentMap;
	SaveGame->SlotInfo.Timestamp = FDateTime::Now();
	SaveGame->SlotInfo.PlaytimeSeconds = GetSessionPlaytime();
	SaveGame->SlotInfo.SaveVersion = UTitanSaveGame::CURRENT_SAVE_VERSION;
	SaveGame->SlotInfo.bIsEmpty = false;
	SaveGame->SlotInfo.bIsAutosave = (SlotIndex >= AUTOSAVE_SLOT_START && SlotIndex < AUTOSAVE_SLOT_START + AUTOSAVE_SLOT_COUNT);

	// Store player and world data
	SaveGame->PlayerData = PlayerData;
	SaveGame->WorldData = WorldData;

	// Collect data from subsystems
	CollectSubsystemData(WorldContext, SaveGame);

	// Store runtime statistics
	SaveGame->Statistics = RuntimeStatistics;

	// Save to disk
	FString SlotName = GetSlotName(SlotIndex);
	bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, 0);

	Result.bSuccess = bSaved;
	if (!bSaved)
	{
		Result.ErrorMessage = FText::FromString(TEXT("Failed to write save file to disk"));
	}

	Result.OperationTimeMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	UE_LOG(LogTemp, Log, TEXT("SaveGame: Slot %d saved in %.2fms - %s"),
		SlotIndex, Result.OperationTimeMs, bSaved ? TEXT("Success") : TEXT("Failed"));

	return Result;
}

FSaveLoadResult USaveGameLibrary::QuickSave(
	const FPlayerSaveData& PlayerData,
	const FWorldSaveData& WorldData,
	const UObject* WorldContext)
{
	return SaveGame(QUICKSAVE_SLOT, TEXT("Quick Save"), PlayerData, WorldData, WorldContext);
}

FSaveLoadResult USaveGameLibrary::AutoSave(
	const FPlayerSaveData& PlayerData,
	const FWorldSaveData& WorldData,
	const UObject* WorldContext)
{
	int32 SlotIndex = AUTOSAVE_SLOT_START + CurrentAutosaveIndex;
	CurrentAutosaveIndex = (CurrentAutosaveIndex + 1) % AUTOSAVE_SLOT_COUNT;

	return SaveGame(SlotIndex, TEXT("Autosave"), PlayerData, WorldData, WorldContext);
}

void USaveGameLibrary::SaveGameAsync(
	int32 SlotIndex,
	const FString& SaveName,
	const FPlayerSaveData& PlayerData,
	const FWorldSaveData& WorldData,
	const UObject* WorldContext,
	FOnSaveComplete OnComplete)
{
	// Capture data for async execution
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [=]()
	{
		FSaveLoadResult Result = SaveGame(SlotIndex, SaveName, PlayerData, WorldData, WorldContext);

		// Return to game thread for callback
		AsyncTask(ENamedThreads::GameThread, [OnComplete, Result]()
		{
			OnComplete.ExecuteIfBound(Result);
		});
	});
}

// ============================================
// LOAD OPERATIONS
// ============================================

FSaveLoadResult USaveGameLibrary::LoadGame(
	int32 SlotIndex,
	FPlayerSaveData& OutPlayerData,
	FWorldSaveData& OutWorldData,
	const UObject* WorldContext)
{
	FSaveLoadResult Result;
	Result.SlotIndex = SlotIndex;

	double StartTime = FPlatformTime::Seconds();

	FString SlotName = GetSlotName(SlotIndex);

	// Check if save exists
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FText::FromString(TEXT("Save file does not exist"));
		return Result;
	}

	// Load save game
	UTitanSaveGame* SaveGame = Cast<UTitanSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SlotName, 0)
	);

	if (!SaveGame)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FText::FromString(TEXT("Failed to load save file"));
		return Result;
	}

	// Check version compatibility
	if (SaveGame->SaveVersion > UTitanSaveGame::CURRENT_SAVE_VERSION)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FText::FromString(TEXT("Save file is from a newer version"));
		return Result;
	}

	// Extract player and world data
	OutPlayerData = SaveGame->PlayerData;
	OutWorldData = SaveGame->WorldData;

	// Restore subsystem data
	RestoreSubsystemData(WorldContext, SaveGame);

	// Restore statistics
	RuntimeStatistics = SaveGame->Statistics;

	// Update session start for playtime tracking
	SessionStartTime = FDateTime::Now();

	Result.bSuccess = true;
	Result.OperationTimeMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	UE_LOG(LogTemp, Log, TEXT("LoadGame: Slot %d loaded in %.2fms"), SlotIndex, Result.OperationTimeMs);

	return Result;
}

FSaveLoadResult USaveGameLibrary::QuickLoad(
	FPlayerSaveData& OutPlayerData,
	FWorldSaveData& OutWorldData,
	const UObject* WorldContext)
{
	return LoadGame(QUICKSAVE_SLOT, OutPlayerData, OutWorldData, WorldContext);
}

void USaveGameLibrary::LoadGameAsync(
	int32 SlotIndex,
	const UObject* WorldContext,
	FOnLoadComplete OnComplete)
{
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [=]()
	{
		FPlayerSaveData PlayerData;
		FWorldSaveData WorldData;
		FSaveLoadResult Result = LoadGame(SlotIndex, PlayerData, WorldData, WorldContext);

		AsyncTask(ENamedThreads::GameThread, [OnComplete, Result]()
		{
			OnComplete.ExecuteIfBound(Result);
		});
	});
}

// ============================================
// SLOT MANAGEMENT
// ============================================

TArray<FSaveSlotInfo> USaveGameLibrary::GetAllSaveSlots()
{
	TArray<FSaveSlotInfo> Slots;

	for (int32 i = 0; i < MAX_SAVE_SLOTS; i++)
	{
		Slots.Add(GetSaveSlotInfo(i));
	}

	return Slots;
}

FSaveSlotInfo USaveGameLibrary::GetSaveSlotInfo(int32 SlotIndex)
{
	FSaveSlotInfo Info;
	Info.SlotIndex = SlotIndex;
	Info.bIsEmpty = true;

	FString SlotName = GetSlotName(SlotIndex);

	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		UTitanSaveGame* SaveGame = Cast<UTitanSaveGame>(
			UGameplayStatics::LoadGameFromSlot(SlotName, 0)
		);

		if (SaveGame)
		{
			Info = SaveGame->SlotInfo;
		}
	}

	return Info;
}

bool USaveGameLibrary::DoesSaveExist(int32 SlotIndex)
{
	return UGameplayStatics::DoesSaveGameExist(GetSlotName(SlotIndex), 0);
}

bool USaveGameLibrary::DeleteSaveSlot(int32 SlotIndex)
{
	FString SlotName = GetSlotName(SlotIndex);

	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		return UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	}

	return true; // Already doesn't exist
}

int32 USaveGameLibrary::GetMostRecentSaveSlot()
{
	int32 MostRecentSlot = -1;
	FDateTime MostRecentTime = FDateTime::MinValue();

	for (int32 i = 0; i < MAX_SAVE_SLOTS; i++)
	{
		FSaveSlotInfo Info = GetSaveSlotInfo(i);
		if (!Info.bIsEmpty && Info.Timestamp > MostRecentTime)
		{
			MostRecentTime = Info.Timestamp;
			MostRecentSlot = i;
		}
	}

	return MostRecentSlot;
}

int32 USaveGameLibrary::GetFirstEmptySlot()
{
	for (int32 i = 0; i < MAX_SAVE_SLOTS; i++)
	{
		if (!DoesSaveExist(i))
		{
			return i;
		}
	}

	return -1;
}

// ============================================
// PLAYER DATA HELPERS
// ============================================

FPlayerSaveData USaveGameLibrary::CreatePlayerSaveData(
	APawn* PlayerPawn,
	UInventoryComponent* InventoryComponent)
{
	FPlayerSaveData Data;

	if (PlayerPawn)
	{
		Data.Location = PlayerPawn->GetActorLocation();
		Data.Rotation = PlayerPawn->GetActorRotation();

		if (UWorld* World = PlayerPawn->GetWorld())
		{
			Data.CurrentMap = World->GetMapName();
		}
	}

	// Note: Level, XP, Health, etc. should be set by the game-specific code
	// as this plugin doesn't know about those systems

	return Data;
}

bool USaveGameLibrary::ApplyPlayerSaveData(
	APawn* PlayerPawn,
	const FPlayerSaveData& PlayerData,
	UInventoryComponent* InventoryComponent)
{
	if (!PlayerPawn)
	{
		return false;
	}

	// Restore position
	PlayerPawn->SetActorLocation(PlayerData.Location);
	PlayerPawn->SetActorRotation(PlayerData.Rotation);

	// Note: Inventory restoration is handled via InventoryComponent->LoadFromJSON
	// which is called in RestoreSubsystemData

	return true;
}

// ============================================
// WORLD DATA HELPERS
// ============================================

void USaveGameLibrary::MarkLocationDiscovered(FWorldSaveData& WorldData, const FString& LocationID)
{
	if (!WorldData.DiscoveredLocations.Contains(LocationID))
	{
		WorldData.DiscoveredLocations.Add(LocationID);
	}
}

bool USaveGameLibrary::IsLocationDiscovered(const FWorldSaveData& WorldData, const FString& LocationID)
{
	return WorldData.DiscoveredLocations.Contains(LocationID);
}

void USaveGameLibrary::SetWorldFlag(FWorldSaveData& WorldData, const FString& FlagName, bool Value)
{
	WorldData.WorldFlags.Add(FlagName, Value);
}

bool USaveGameLibrary::GetWorldFlag(const FWorldSaveData& WorldData, const FString& FlagName)
{
	const bool* Value = WorldData.WorldFlags.Find(FlagName);
	return Value ? *Value : false;
}

// ============================================
// STATISTICS
// ============================================

void USaveGameLibrary::IncrementStatistic(const FString& StatName, int32 Amount)
{
	int32& Value = RuntimeStatistics.FindOrAdd(StatName);
	Value += Amount;
}

int32 USaveGameLibrary::GetStatistic(const FString& StatName)
{
	const int32* Value = RuntimeStatistics.Find(StatName);
	return Value ? *Value : 0;
}

TMap<FString, int32> USaveGameLibrary::GetAllStatistics()
{
	return RuntimeStatistics;
}

// ============================================
// PLAYTIME TRACKING
// ============================================

void USaveGameLibrary::StartPlaytimeTracking()
{
	SessionStartTime = FDateTime::Now();
}

float USaveGameLibrary::GetSessionPlaytime()
{
	FTimespan Elapsed = FDateTime::Now() - SessionStartTime;
	return static_cast<float>(Elapsed.GetTotalSeconds());
}

float USaveGameLibrary::GetTotalPlaytime(int32 SlotIndex)
{
	FSaveSlotInfo Info = GetSaveSlotInfo(SlotIndex);
	return Info.PlaytimeSeconds;
}

FString USaveGameLibrary::FormatPlaytime(float PlaytimeSeconds)
{
	int32 TotalSeconds = FMath::FloorToInt(PlaytimeSeconds);
	int32 Hours = TotalSeconds / 3600;
	int32 Minutes = (TotalSeconds % 3600) / 60;
	int32 Seconds = TotalSeconds % 60;

	return FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Seconds);
}

// ============================================
// PRIVATE HELPERS
// ============================================

FString USaveGameLibrary::GetSlotName(int32 SlotIndex)
{
	if (SlotIndex == QUICKSAVE_SLOT)
	{
		return TEXT("TitanQuickSave");
	}
	else if (SlotIndex >= AUTOSAVE_SLOT_START && SlotIndex < AUTOSAVE_SLOT_START + AUTOSAVE_SLOT_COUNT)
	{
		return FString::Printf(TEXT("TitanAutosave_%d"), SlotIndex - AUTOSAVE_SLOT_START);
	}

	return FString::Printf(TEXT("TitanSave_%d"), SlotIndex);
}

void USaveGameLibrary::CollectSubsystemData(const UObject* WorldContext, UTitanSaveGame* SaveGame)
{
	if (!WorldContext || !SaveGame)
	{
		return;
	}

	// Get Quest Subsystem data
	if (UQuestSubsystem* QuestSys = UQuestSubsystem::Get(WorldContext))
	{
		SaveGame->QuestDataJSON = QuestSys->SaveToJSON();
	}

	// Get Achievement data
	SaveGame->AchievementDataJSON = UAchievementLibrary::SaveProgressToJSON();

	// Get Shop states
	SaveGame->ShopDataJSON = UShopLibrary::SaveShopStatesToJSON();

	// Note: Inventory is per-component and should be collected separately
	// The game should call InventoryComponent->SaveToJSON() and include it
}

void USaveGameLibrary::RestoreSubsystemData(const UObject* WorldContext, UTitanSaveGame* SaveGame)
{
	if (!WorldContext || !SaveGame)
	{
		return;
	}

	// Restore Quest Subsystem data
	if (UQuestSubsystem* QuestSys = UQuestSubsystem::Get(WorldContext))
	{
		if (!SaveGame->QuestDataJSON.IsEmpty())
		{
			QuestSys->LoadFromJSON(SaveGame->QuestDataJSON);
		}
	}

	// Restore Achievement data
	if (!SaveGame->AchievementDataJSON.IsEmpty())
	{
		UAchievementLibrary::LoadProgressFromJSON(SaveGame->AchievementDataJSON);
	}

	// Restore Shop states
	if (!SaveGame->ShopDataJSON.IsEmpty())
	{
		UShopLibrary::LoadShopStatesFromJSON(SaveGame->ShopDataJSON);
	}

	// Note: Inventory restoration should be done separately per-component
}
