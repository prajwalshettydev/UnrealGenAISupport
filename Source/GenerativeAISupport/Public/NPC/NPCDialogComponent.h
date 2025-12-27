// NPC Dialog Component - Actor component for NPC dialog functionality
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPC/NPCDialogTypes.h"
#include "NPCDialogComponent.generated.h"

// Forward declarations
class UInventoryComponent;

// Delegates for dialog events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogStarted, const FNPCDialogInfo&, NPCInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogResponse, const FText&, Response, ENPCMood, Mood);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogEvent, const FDialogEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogChoicesUpdated, const TArray<FDialogChoice>&, Choices);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogWaitingForResponse);

/**
 * NPC Dialog Component
 * Add to any NPC actor to enable AI-powered dialog
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), BlueprintType)
class GENERATIVEAISUPPORT_API UNPCDialogComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPCDialogComponent();

	// ============================================
	// EVENTS
	// ============================================

	/** Called when dialog starts */
	UPROPERTY(BlueprintAssignable, Category = "NPC Dialog|Events")
	FOnDialogStarted OnDialogStarted;

	/** Called when NPC responds */
	UPROPERTY(BlueprintAssignable, Category = "NPC Dialog|Events")
	FOnDialogResponse OnDialogResponse;

	/** Called when dialog ends */
	UPROPERTY(BlueprintAssignable, Category = "NPC Dialog|Events")
	FOnDialogEnded OnDialogEnded;

	/** Called for various dialog events */
	UPROPERTY(BlueprintAssignable, Category = "NPC Dialog|Events")
	FOnDialogEvent OnDialogEvent;

	/** Called when choices are updated */
	UPROPERTY(BlueprintAssignable, Category = "NPC Dialog|Events")
	FOnDialogChoicesUpdated OnDialogChoicesUpdated;

	/** Called when waiting for AI response */
	UPROPERTY(BlueprintAssignable, Category = "NPC Dialog|Events")
	FOnDialogWaitingForResponse OnWaitingForResponse;

	// ============================================
	// CONFIGURATION
	// ============================================

	/** NPC unique identifier (matches personality in JSON) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialog|Config")
	FString NPCID;

	/** Display name for UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialog|Config")
	FText DisplayName;

	/** Title/Role text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialog|Config")
	FText Title;

	/** Portrait image path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialog|Config")
	FString PortraitPath;

	/** Faction ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialog|Config")
	FString FactionID;

	/** Shop ID if NPC is a merchant */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialog|Config")
	FString ShopID;

	/** Can this NPC trade? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialog|Config")
	bool bCanTrade = false;

	/** Greeting message (first message when dialog starts) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialog|Config", meta = (MultiLine = true))
	FText GreetingMessage;

	/** Farewell message (when dialog ends) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialog|Config")
	FText FarewellMessage;

	/** Quick dialog options (pre-defined responses) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialog|Config")
	TArray<FQuickDialogOption> QuickOptions;

	/** Maximum conversation history to keep */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialog|Config")
	int32 MaxHistoryLength = 20;

	/** Dialog interaction range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialog|Config")
	float InteractionRange = 300.f;

	// ============================================
	// DIALOG CONTROL
	// ============================================

	/**
	 * Start a dialog with this NPC
	 * @param PlayerActor - The player starting the dialog
	 * @return True if dialog started successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog")
	bool StartDialog(AActor* PlayerActor);

	/**
	 * End the current dialog
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog")
	void EndDialog();

	/**
	 * Send a message to the NPC
	 * @param Message - Player's message
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog")
	void SendMessage(const FString& Message);

	/**
	 * Send a message asynchronously (non-blocking)
	 * @param Message - Player's message
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog")
	void SendMessageAsync(const FString& Message);

	/**
	 * Select a predefined choice
	 * @param ChoiceIndex - Index of the choice to select
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog")
	void SelectChoice(int32 ChoiceIndex);

	/**
	 * Select a choice by ID
	 * @param ChoiceID - ID of the choice
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog")
	void SelectChoiceByID(const FString& ChoiceID);

	/**
	 * Use a quick dialog option
	 * @param OptionIndex - Index of quick option
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog")
	void UseQuickOption(int32 OptionIndex);

	// ============================================
	// STATE QUERIES
	// ============================================

	/** Is dialog currently active? */
	UFUNCTION(BlueprintPure, Category = "NPC Dialog")
	bool IsDialogActive() const { return bIsDialogActive; }

	/** Is waiting for NPC response? */
	UFUNCTION(BlueprintPure, Category = "NPC Dialog")
	bool IsWaitingForResponse() const { return bIsWaitingForResponse; }

	/** Get current dialog state */
	UFUNCTION(BlueprintPure, Category = "NPC Dialog")
	FDialogState GetDialogState() const { return CurrentState; }

	/** Get NPC info struct */
	UFUNCTION(BlueprintPure, Category = "NPC Dialog")
	FNPCDialogInfo GetNPCInfo() const;

	/** Get current mood */
	UFUNCTION(BlueprintPure, Category = "NPC Dialog")
	ENPCMood GetCurrentMood() const { return CurrentMood; }

	/** Get conversation history */
	UFUNCTION(BlueprintPure, Category = "NPC Dialog")
	TArray<FDialogMessage> GetHistory() const { return CurrentState.History; }

	/** Get available choices */
	UFUNCTION(BlueprintPure, Category = "NPC Dialog")
	TArray<FDialogChoice> GetAvailableChoices() const { return CurrentState.AvailableChoices; }

	/** Check if player is in range */
	UFUNCTION(BlueprintPure, Category = "NPC Dialog")
	bool IsPlayerInRange(AActor* PlayerActor) const;

	// ============================================
	// MOOD & REPUTATION
	// ============================================

	/** Set NPC mood */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog|Mood")
	void SetMood(ENPCMood NewMood);

	/** Modify reputation with this NPC */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog|Reputation")
	void ModifyReputation(int32 Amount);

	/** Get current reputation */
	UFUNCTION(BlueprintPure, Category = "NPC Dialog|Reputation")
	int32 GetReputation() const { return Reputation; }

	/** Get reputation rank text */
	UFUNCTION(BlueprintPure, Category = "NPC Dialog|Reputation")
	FText GetReputationRankText() const;

	// ============================================
	// CONVERSATION MANAGEMENT
	// ============================================

	/** Reset conversation history */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog")
	void ResetConversation();

	/** Add a system message to context (not shown to player) */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog")
	void AddContextMessage(const FString& Context);

	/** Set current topic */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog")
	void SetTopic(const FString& Topic);

	// ============================================
	// CHOICES MANAGEMENT
	// ============================================

	/** Set available choices manually */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog|Choices")
	void SetAvailableChoices(const TArray<FDialogChoice>& Choices);

	/** Add a choice */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog|Choices")
	void AddChoice(const FDialogChoice& Choice);

	/** Clear all choices */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog|Choices")
	void ClearChoices();

	/** Generate default choices based on NPC capabilities */
	UFUNCTION(BlueprintCallable, Category = "NPC Dialog|Choices")
	void GenerateDefaultChoices();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Current dialog state */
	UPROPERTY()
	FDialogState CurrentState;

	/** Is dialog currently active? */
	bool bIsDialogActive = false;

	/** Is waiting for AI response? */
	bool bIsWaitingForResponse = false;

	/** Current mood */
	ENPCMood CurrentMood = ENPCMood::Neutral;

	/** Player reputation with this NPC */
	int32 Reputation = 0;

	/** Current player in dialog */
	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentPlayer;

	/** Context messages for AI */
	TArray<FString> ContextMessages;

	/** Parse mood from AI response */
	ENPCMood ParseMoodFromResponse(const FString& Response);

	/** Add message to history */
	void AddToHistory(bool bIsPlayer, const FText& Message);

	/** Broadcast dialog event */
	void BroadcastEvent(EDialogEventType Type, const FString& ExtraData = TEXT(""));

	/** Handle async response */
	void OnAsyncResponseReceived(const FString& Response, bool bSuccess);
};
