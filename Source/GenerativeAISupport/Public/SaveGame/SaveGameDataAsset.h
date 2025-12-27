// SaveGame Data Asset - Designer-configurable save settings
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SaveGameDataAsset.generated.h"

/**
 * Autosave trigger types
 */
UENUM(BlueprintType)
enum class EAutosaveTrigger : uint8
{
	/** Autosave on timer */
	Timer UMETA(DisplayName = "Timer"),

	/** Autosave on zone change */
	ZoneChange UMETA(DisplayName = "Zone Change"),

	/** Autosave after quest completion */
	QuestComplete UMETA(DisplayName = "Quest Complete"),

	/** Autosave after major event */
	MajorEvent UMETA(DisplayName = "Major Event"),

	/** Autosave on rest/sleep */
	Rest UMETA(DisplayName = "Rest/Sleep")
};

/**
 * Data asset for save system configuration
 * Create one of these in Content/Data/ and reference in GameMode
 */
UCLASS(BlueprintType)
class GENERATIVEAISUPPORT_API USaveSettingsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// ============================================
	// SLOT CONFIGURATION
	// ============================================

	/** Number of available save slots (1-20) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slots", meta = (ClampMin = "1", ClampMax = "20"))
	int32 MaxSaveSlots = 10;

	/** Allow quick save/load? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slots")
	bool bEnableQuickSave = true;

	/** Quick save key binding (for UI reference) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slots")
	FKey QuickSaveKey = EKeys::F5;

	/** Quick load key binding (for UI reference) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slots")
	FKey QuickLoadKey = EKeys::F9;

	// ============================================
	// AUTOSAVE
	// ============================================

	/** Enable autosave? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Autosave")
	bool bEnableAutosave = true;

	/** Number of rotating autosave slots */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Autosave", meta = (ClampMin = "1", ClampMax = "5", EditCondition = "bEnableAutosave"))
	int32 AutosaveSlotCount = 3;

	/** Autosave triggers */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Autosave", meta = (EditCondition = "bEnableAutosave"))
	TArray<EAutosaveTrigger> AutosaveTriggers;

	/** Timer interval in minutes (if Timer trigger enabled) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Autosave", meta = (ClampMin = "1", ClampMax = "60", EditCondition = "bEnableAutosave"))
	int32 AutosaveIntervalMinutes = 10;

	/** Show notification on autosave? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Autosave", meta = (EditCondition = "bEnableAutosave"))
	bool bShowAutosaveNotification = true;

	// ============================================
	// SCREENSHOTS
	// ============================================

	/** Capture screenshot for save slots? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Screenshots")
	bool bCaptureScreenshots = true;

	/** Screenshot resolution (width) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Screenshots", meta = (ClampMin = "128", ClampMax = "512", EditCondition = "bCaptureScreenshots"))
	int32 ScreenshotWidth = 256;

	/** Screenshot resolution (height) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Screenshots", meta = (ClampMin = "72", ClampMax = "288", EditCondition = "bCaptureScreenshots"))
	int32 ScreenshotHeight = 144;

	// ============================================
	// RESTRICTIONS
	// ============================================

	/** Locations where saving is NOT allowed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Restrictions")
	TArray<FString> NoSaveZones;

	/** Disable saving during combat? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Restrictions")
	bool bDisableSaveDuringCombat = false;

	/** Disable saving during cutscenes? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Restrictions")
	bool bDisableSaveDuringCutscene = true;

	/** Minimum time between saves (anti-savescum, in seconds, 0 = no limit) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Restrictions", meta = (ClampMin = "0", ClampMax = "300"))
	float MinTimeBetweenSaves = 0.f;

	// ============================================
	// UI TEXT
	// ============================================

	/** Text shown during save */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FText SavingText = NSLOCTEXT("SaveGame", "Saving", "Saving...");

	/** Text shown during load */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FText LoadingText = NSLOCTEXT("SaveGame", "Loading", "Loading...");

	/** Text for empty slot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FText EmptySlotText = NSLOCTEXT("SaveGame", "EmptySlot", "Empty Slot");

	/** Confirm overwrite text */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FText ConfirmOverwriteText = NSLOCTEXT("SaveGame", "ConfirmOverwrite", "Overwrite existing save?");

	/** Confirm delete text */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FText ConfirmDeleteText = NSLOCTEXT("SaveGame", "ConfirmDelete", "Delete this save?");

	// ============================================
	// HELPERS
	// ============================================

	/** Check if autosave trigger is enabled */
	UFUNCTION(BlueprintPure, Category = "SaveGame|Settings")
	bool IsAutosaveTriggerEnabled(EAutosaveTrigger Trigger) const
	{
		return bEnableAutosave && AutosaveTriggers.Contains(Trigger);
	}

	/** Check if zone is a no-save zone */
	UFUNCTION(BlueprintPure, Category = "SaveGame|Settings")
	bool IsNoSaveZone(const FString& ZoneName) const
	{
		return NoSaveZones.Contains(ZoneName);
	}
};
