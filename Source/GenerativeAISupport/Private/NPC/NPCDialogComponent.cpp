// NPC Dialog Component Implementation
// Part of GenerativeAISupport Plugin

#include "NPC/NPCDialogComponent.h"
#include "NPC/NPCChatLibrary.h"
#include "GameFramework/Actor.h"
#include "Async/Async.h"

UNPCDialogComponent::UNPCDialogComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UNPCDialogComponent::BeginPlay()
{
	Super::BeginPlay();

	// Generate default choices if none set
	if (CurrentState.AvailableChoices.Num() == 0)
	{
		GenerateDefaultChoices();
	}
}

void UNPCDialogComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Check if player went out of range during dialog
	if (bIsDialogActive && CurrentPlayer.IsValid())
	{
		if (!IsPlayerInRange(CurrentPlayer.Get()))
		{
			EndDialog();
		}
	}
}

// ============================================
// DIALOG CONTROL
// ============================================

bool UNPCDialogComponent::StartDialog(AActor* PlayerActor)
{
	if (!PlayerActor || bIsDialogActive)
	{
		return false;
	}

	if (!IsPlayerInRange(PlayerActor))
	{
		return false;
	}

	// Check if NPC server is running
	if (!UNPCChatLibrary::IsNPCServerRunning())
	{
		BroadcastEvent(EDialogEventType::Error, TEXT("NPC Server not running"));
		return false;
	}

	bIsDialogActive = true;
	CurrentPlayer = PlayerActor;
	CurrentState.bIsFirstMessage = true;
	CurrentState.NPCInfo = GetNPCInfo();
	CurrentState.History.Empty();

	// Enable tick for range checking
	SetComponentTickEnabled(true);

	// Broadcast start event
	OnDialogStarted.Broadcast(CurrentState.NPCInfo);
	BroadcastEvent(EDialogEventType::Started);

	// Send greeting if available
	if (!GreetingMessage.IsEmpty())
	{
		CurrentState.CurrentResponse = GreetingMessage;
		AddToHistory(false, GreetingMessage);
		OnDialogResponse.Broadcast(GreetingMessage, CurrentMood);
	}

	// Generate default choices
	GenerateDefaultChoices();

	return true;
}

void UNPCDialogComponent::EndDialog()
{
	if (!bIsDialogActive)
	{
		return;
	}

	// Send farewell if available
	if (!FarewellMessage.IsEmpty())
	{
		CurrentState.CurrentResponse = FarewellMessage;
		AddToHistory(false, FarewellMessage);
	}

	bIsDialogActive = false;
	bIsWaitingForResponse = false;
	CurrentPlayer.Reset();

	// Disable tick
	SetComponentTickEnabled(false);

	// Broadcast end event
	OnDialogEnded.Broadcast();
	BroadcastEvent(EDialogEventType::Ended);
}

void UNPCDialogComponent::SendMessage(const FString& Message)
{
	if (!bIsDialogActive || bIsWaitingForResponse)
	{
		return;
	}

	// Add player message to history
	AddToHistory(true, FText::FromString(Message));

	bIsWaitingForResponse = true;
	OnWaitingForResponse.Broadcast();
	CurrentState.bIsWaitingForResponse = true;

	// Build context-enhanced message
	FString EnhancedMessage = Message;
	if (ContextMessages.Num() > 0)
	{
		// Prepend context (invisible to player, helps AI)
		EnhancedMessage = FString::Printf(TEXT("[Context: %s] %s"),
			*FString::Join(ContextMessages, TEXT(", ")), *Message);
	}

	// Send to NPC server
	FString Response;
	bool bSuccess = UNPCChatLibrary::SendNPCChat(EnhancedMessage, NPCID, Response);

	if (bSuccess)
	{
		// Parse mood from response if indicated
		CurrentMood = ParseMoodFromResponse(Response);

		// Update state
		CurrentState.CurrentResponse = FText::FromString(Response);
		CurrentState.bIsFirstMessage = false;
		CurrentState.bIsWaitingForResponse = false;

		// Add to history
		AddToHistory(false, FText::FromString(Response));

		// Broadcast response
		OnDialogResponse.Broadcast(FText::FromString(Response), CurrentMood);
		BroadcastEvent(EDialogEventType::NPCResponded);
	}
	else
	{
		BroadcastEvent(EDialogEventType::Error, TEXT("Failed to get response"));
	}

	bIsWaitingForResponse = false;
	CurrentState.bIsWaitingForResponse = false;
}

void UNPCDialogComponent::SendMessageAsync(const FString& Message)
{
	if (!bIsDialogActive || bIsWaitingForResponse)
	{
		return;
	}

	// Add player message to history
	AddToHistory(true, FText::FromString(Message));

	bIsWaitingForResponse = true;
	OnWaitingForResponse.Broadcast();
	CurrentState.bIsWaitingForResponse = true;

	// Capture data for async
	FString CapturedNPCID = NPCID;
	FString EnhancedMessage = Message;

	if (ContextMessages.Num() > 0)
	{
		EnhancedMessage = FString::Printf(TEXT("[Context: %s] %s"),
			*FString::Join(ContextMessages, TEXT(", ")), *Message);
	}

	// Run async
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, EnhancedMessage, CapturedNPCID]()
	{
		FString Response;
		bool bSuccess = UNPCChatLibrary::SendNPCChat(EnhancedMessage, CapturedNPCID, Response);

		// Return to game thread
		AsyncTask(ENamedThreads::GameThread, [this, Response, bSuccess]()
		{
			OnAsyncResponseReceived(Response, bSuccess);
		});
	});
}

void UNPCDialogComponent::OnAsyncResponseReceived(const FString& Response, bool bSuccess)
{
	bIsWaitingForResponse = false;
	CurrentState.bIsWaitingForResponse = false;

	if (bSuccess)
	{
		CurrentMood = ParseMoodFromResponse(Response);
		CurrentState.CurrentResponse = FText::FromString(Response);
		CurrentState.bIsFirstMessage = false;

		AddToHistory(false, FText::FromString(Response));

		OnDialogResponse.Broadcast(FText::FromString(Response), CurrentMood);
		BroadcastEvent(EDialogEventType::NPCResponded);
	}
	else
	{
		BroadcastEvent(EDialogEventType::Error, TEXT("Failed to get response"));
	}
}

void UNPCDialogComponent::SelectChoice(int32 ChoiceIndex)
{
	if (ChoiceIndex < 0 || ChoiceIndex >= CurrentState.AvailableChoices.Num())
	{
		return;
	}

	const FDialogChoice& Choice = CurrentState.AvailableChoices[ChoiceIndex];

	if (!Choice.bIsAvailable)
	{
		return;
	}

	// Handle special choice types
	switch (Choice.ChoiceType)
	{
	case EDialogChoiceType::Goodbye:
		EndDialog();
		return;

	case EDialogChoiceType::Trade:
		if (!ShopID.IsEmpty())
		{
			BroadcastEvent(EDialogEventType::TradeOpened, ShopID);
		}
		break;

	case EDialogChoiceType::Quest:
		if (!Choice.ChoiceID.IsEmpty())
		{
			BroadcastEvent(EDialogEventType::QuestOffered, Choice.ChoiceID);
		}
		break;

	default:
		break;
	}

	// Send as message
	SendMessage(Choice.DisplayText.ToString());
}

void UNPCDialogComponent::SelectChoiceByID(const FString& ChoiceID)
{
	for (int32 i = 0; i < CurrentState.AvailableChoices.Num(); i++)
	{
		if (CurrentState.AvailableChoices[i].ChoiceID == ChoiceID)
		{
			SelectChoice(i);
			return;
		}
	}
}

void UNPCDialogComponent::UseQuickOption(int32 OptionIndex)
{
	if (OptionIndex < 0 || OptionIndex >= QuickOptions.Num())
	{
		return;
	}

	SendMessage(QuickOptions[OptionIndex].Message);
}

// ============================================
// STATE QUERIES
// ============================================

FNPCDialogInfo UNPCDialogComponent::GetNPCInfo() const
{
	FNPCDialogInfo Info;
	Info.NPCID = NPCID;
	Info.DisplayName = DisplayName;
	Info.Title = Title;
	Info.PortraitPath = PortraitPath;
	Info.FactionID = FactionID;
	Info.Mood = CurrentMood;
	Info.Reputation = Reputation;
	Info.RelationshipStatus = GetReputationRankText();
	Info.bCanTrade = bCanTrade;
	Info.ShopID = ShopID;
	// bHasQuestAvailable would be set by quest system integration

	return Info;
}

bool UNPCDialogComponent::IsPlayerInRange(AActor* PlayerActor) const
{
	if (!PlayerActor || !GetOwner())
	{
		return false;
	}

	float Distance = FVector::Dist(PlayerActor->GetActorLocation(), GetOwner()->GetActorLocation());
	return Distance <= InteractionRange;
}

// ============================================
// MOOD & REPUTATION
// ============================================

void UNPCDialogComponent::SetMood(ENPCMood NewMood)
{
	CurrentMood = NewMood;
	CurrentState.NPCInfo.Mood = NewMood;
}

void UNPCDialogComponent::ModifyReputation(int32 Amount)
{
	int32 OldReputation = Reputation;
	Reputation = FMath::Clamp(Reputation + Amount, -1000, 1000);

	if (Reputation != OldReputation)
	{
		FDialogEvent Event;
		Event.EventType = EDialogEventType::ReputationChanged;
		Event.NPCID = NPCID;
		Event.ReputationChange = Amount;
		OnDialogEvent.Broadcast(Event);
	}
}

FText UNPCDialogComponent::GetReputationRankText() const
{
	if (Reputation >= 900) return FText::FromString(TEXT("Exalted"));
	if (Reputation >= 600) return FText::FromString(TEXT("Honored"));
	if (Reputation >= 300) return FText::FromString(TEXT("Friendly"));
	if (Reputation >= 0) return FText::FromString(TEXT("Neutral"));
	if (Reputation >= -300) return FText::FromString(TEXT("Unfriendly"));
	if (Reputation >= -600) return FText::FromString(TEXT("Hostile"));
	return FText::FromString(TEXT("Hated"));
}

// ============================================
// CONVERSATION MANAGEMENT
// ============================================

void UNPCDialogComponent::ResetConversation()
{
	CurrentState.History.Empty();
	CurrentState.bIsFirstMessage = true;
	ContextMessages.Empty();

	// Also reset on server
	UNPCChatLibrary::ResetNPCConversation(NPCID);
}

void UNPCDialogComponent::AddContextMessage(const FString& Context)
{
	ContextMessages.Add(Context);
}

void UNPCDialogComponent::SetTopic(const FString& Topic)
{
	CurrentState.CurrentTopic = Topic;
	AddContextMessage(FString::Printf(TEXT("Topic: %s"), *Topic));
}

// ============================================
// CHOICES MANAGEMENT
// ============================================

void UNPCDialogComponent::SetAvailableChoices(const TArray<FDialogChoice>& Choices)
{
	CurrentState.AvailableChoices = Choices;
	OnDialogChoicesUpdated.Broadcast(CurrentState.AvailableChoices);
}

void UNPCDialogComponent::AddChoice(const FDialogChoice& Choice)
{
	CurrentState.AvailableChoices.Add(Choice);
	OnDialogChoicesUpdated.Broadcast(CurrentState.AvailableChoices);
}

void UNPCDialogComponent::ClearChoices()
{
	CurrentState.AvailableChoices.Empty();
	OnDialogChoicesUpdated.Broadcast(CurrentState.AvailableChoices);
}

void UNPCDialogComponent::GenerateDefaultChoices()
{
	ClearChoices();

	// Trade option if merchant
	if (bCanTrade && !ShopID.IsEmpty())
	{
		FDialogChoice TradeChoice;
		TradeChoice.ChoiceID = TEXT("trade");
		TradeChoice.DisplayText = FText::FromString(TEXT("Show me your wares."));
		TradeChoice.ChoiceType = EDialogChoiceType::Trade;
		TradeChoice.bIsAvailable = true;
		CurrentState.AvailableChoices.Add(TradeChoice);
	}

	// Add quick options as choices
	for (int32 i = 0; i < QuickOptions.Num(); i++)
	{
		FDialogChoice QuickChoice;
		QuickChoice.ChoiceID = FString::Printf(TEXT("quick_%d"), i);
		QuickChoice.DisplayText = QuickOptions[i].DisplayText;
		QuickChoice.ChoiceType = EDialogChoiceType::Normal;
		QuickChoice.bIsAvailable = true;
		CurrentState.AvailableChoices.Add(QuickChoice);
	}

	// Goodbye option
	FDialogChoice GoodbyeChoice;
	GoodbyeChoice.ChoiceID = TEXT("goodbye");
	GoodbyeChoice.DisplayText = FText::FromString(TEXT("Goodbye."));
	GoodbyeChoice.ChoiceType = EDialogChoiceType::Goodbye;
	GoodbyeChoice.bIsAvailable = true;
	CurrentState.AvailableChoices.Add(GoodbyeChoice);

	OnDialogChoicesUpdated.Broadcast(CurrentState.AvailableChoices);
}

// ============================================
// PRIVATE HELPERS
// ============================================

ENPCMood UNPCDialogComponent::ParseMoodFromResponse(const FString& Response)
{
	// Simple mood detection based on keywords
	// This could be enhanced with AI analysis
	FString LowerResponse = Response.ToLower();

	if (LowerResponse.Contains(TEXT("laugh")) || LowerResponse.Contains(TEXT("smile")) ||
		LowerResponse.Contains(TEXT("happy")) || LowerResponse.Contains(TEXT("joy")))
	{
		return ENPCMood::Happy;
	}
	if (LowerResponse.Contains(TEXT("sad")) || LowerResponse.Contains(TEXT("sigh")) ||
		LowerResponse.Contains(TEXT("unfortunate")))
	{
		return ENPCMood::Sad;
	}
	if (LowerResponse.Contains(TEXT("angry")) || LowerResponse.Contains(TEXT("furious")) ||
		LowerResponse.Contains(TEXT("outrage")))
	{
		return ENPCMood::Angry;
	}
	if (LowerResponse.Contains(TEXT("surprise")) || LowerResponse.Contains(TEXT("shock")) ||
		LowerResponse.Contains(TEXT("what?!")))
	{
		return ENPCMood::Surprised;
	}
	if (LowerResponse.Contains(TEXT("think")) || LowerResponse.Contains(TEXT("hmm")) ||
		LowerResponse.Contains(TEXT("ponder")))
	{
		return ENPCMood::Thoughtful;
	}
	if (LowerResponse.Contains(TEXT("afraid")) || LowerResponse.Contains(TEXT("fear")) ||
		LowerResponse.Contains(TEXT("scared")))
	{
		return ENPCMood::Afraid;
	}
	if (LowerResponse.Contains(TEXT("suspicious")) || LowerResponse.Contains(TEXT("doubt")) ||
		LowerResponse.Contains(TEXT("trust")))
	{
		return ENPCMood::Suspicious;
	}

	return ENPCMood::Neutral;
}

void UNPCDialogComponent::AddToHistory(bool bIsPlayer, const FText& Message)
{
	FDialogMessage HistoryMessage;
	HistoryMessage.bIsPlayerMessage = bIsPlayer;
	HistoryMessage.MessageText = Message;
	HistoryMessage.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	HistoryMessage.NPCMood = CurrentMood;

	CurrentState.History.Add(HistoryMessage);

	// Trim history if too long
	while (CurrentState.History.Num() > MaxHistoryLength)
	{
		CurrentState.History.RemoveAt(0);
	}
}

void UNPCDialogComponent::BroadcastEvent(EDialogEventType Type, const FString& ExtraData)
{
	FDialogEvent Event;
	Event.EventType = Type;
	Event.NPCID = NPCID;

	switch (Type)
	{
	case EDialogEventType::TradeOpened:
		// ShopID passed as extra data
		break;
	case EDialogEventType::QuestOffered:
		Event.QuestID = ExtraData;
		break;
	case EDialogEventType::Error:
		Event.ErrorMessage = FText::FromString(ExtraData);
		break;
	default:
		break;
	}

	OnDialogEvent.Broadcast(Event);
}
