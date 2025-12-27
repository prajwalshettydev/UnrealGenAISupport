// NPC Dialog Types - Data structures for NPC dialog UI
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "NPCDialogTypes.generated.h"

/**
 * NPC mood/emotion state for UI visualization
 */
UENUM(BlueprintType)
enum class ENPCMood : uint8
{
	Neutral UMETA(DisplayName = "Neutral"),
	Happy UMETA(DisplayName = "Happy"),
	Sad UMETA(DisplayName = "Sad"),
	Angry UMETA(DisplayName = "Angry"),
	Surprised UMETA(DisplayName = "Surprised"),
	Thoughtful UMETA(DisplayName = "Thoughtful"),
	Afraid UMETA(DisplayName = "Afraid"),
	Suspicious UMETA(DisplayName = "Suspicious")
};

/**
 * Dialog choice type
 */
UENUM(BlueprintType)
enum class EDialogChoiceType : uint8
{
	/** Normal response option */
	Normal UMETA(DisplayName = "Normal"),
	/** Requires item or currency */
	Trade UMETA(DisplayName = "Trade"),
	/** Starts a quest */
	Quest UMETA(DisplayName = "Quest"),
	/** Aggressive option */
	Threaten UMETA(DisplayName = "Threaten"),
	/** Charm/persuade option */
	Persuade UMETA(DisplayName = "Persuade"),
	/** Exit conversation */
	Goodbye UMETA(DisplayName = "Goodbye"),
	/** Back to previous topic */
	Back UMETA(DisplayName = "Back")
};

/**
 * A single dialog choice for the player
 */
USTRUCT(BlueprintType)
struct FDialogChoice
{
	GENERATED_BODY()

	/** Unique ID for this choice */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FString ChoiceID;

	/** Text shown to player */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FText DisplayText;

	/** Type of choice (affects icon/styling) */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	EDialogChoiceType ChoiceType = EDialogChoiceType::Normal;

	/** Is this choice currently available? */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	bool bIsAvailable = true;

	/** Reason if not available (shown as tooltip) */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FText UnavailableReason;

	/** Required item ID (for Trade type) */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FString RequiredItemID;

	/** Required item quantity */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	int32 RequiredItemQuantity = 0;

	/** Required gold amount */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	int32 RequiredGold = 0;

	/** Skill check name (for Persuade/Threaten) */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FString SkillCheck;

	/** Skill check difficulty */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	int32 SkillDifficulty = 0;
};

/**
 * NPC display information for dialog UI
 */
USTRUCT(BlueprintType)
struct FNPCDialogInfo
{
	GENERATED_BODY()

	/** NPC unique ID */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FString NPCID;

	/** Display name */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FText DisplayName;

	/** Title/Role (e.g., "Blacksmith", "Village Elder") */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FText Title;

	/** Portrait texture path or reference */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FString PortraitPath;

	/** Current mood */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	ENPCMood Mood = ENPCMood::Neutral;

	/** Faction affiliation */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FString FactionID;

	/** Player's reputation with this NPC (-1000 to 1000) */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	int32 Reputation = 0;

	/** Relationship status text */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FText RelationshipStatus;

	/** Has active quest for player? */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	bool bHasQuestAvailable = false;

	/** Can trade with player? */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	bool bCanTrade = false;

	/** Shop ID if can trade */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FString ShopID;
};

/**
 * A single message in the dialog history
 */
USTRUCT(BlueprintType)
struct FDialogMessage
{
	GENERATED_BODY()

	/** Is this from the player (true) or NPC (false)? */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	bool bIsPlayerMessage = false;

	/** The message text */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FText MessageText;

	/** Timestamp (game time) */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	float Timestamp = 0.f;

	/** NPC mood at time of message */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	ENPCMood NPCMood = ENPCMood::Neutral;
};

/**
 * Complete dialog state for UI
 */
USTRUCT(BlueprintType)
struct FDialogState
{
	GENERATED_BODY()

	/** NPC we're talking to */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FNPCDialogInfo NPCInfo;

	/** Current NPC response text */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FText CurrentResponse;

	/** Available player choices */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	TArray<FDialogChoice> AvailableChoices;

	/** Conversation history */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	TArray<FDialogMessage> History;

	/** Is dialog currently waiting for AI response? */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	bool bIsWaitingForResponse = false;

	/** Current topic/context */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FString CurrentTopic;

	/** Is this the first message? */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	bool bIsFirstMessage = true;
};

/**
 * Dialog event types for UI notification
 */
UENUM(BlueprintType)
enum class EDialogEventType : uint8
{
	/** Dialog started */
	Started UMETA(DisplayName = "Started"),
	/** NPC responded */
	NPCResponded UMETA(DisplayName = "NPC Responded"),
	/** Player selected choice */
	PlayerChose UMETA(DisplayName = "Player Chose"),
	/** Quest offered */
	QuestOffered UMETA(DisplayName = "Quest Offered"),
	/** Quest accepted */
	QuestAccepted UMETA(DisplayName = "Quest Accepted"),
	/** Trade opened */
	TradeOpened UMETA(DisplayName = "Trade Opened"),
	/** Dialog ended */
	Ended UMETA(DisplayName = "Ended"),
	/** Reputation changed */
	ReputationChanged UMETA(DisplayName = "Reputation Changed"),
	/** Connection error */
	Error UMETA(DisplayName = "Error")
};

/**
 * Dialog event data
 */
USTRUCT(BlueprintType)
struct FDialogEvent
{
	GENERATED_BODY()

	/** Event type */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	EDialogEventType EventType = EDialogEventType::Started;

	/** NPC ID involved */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FString NPCID;

	/** Optional quest ID */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FString QuestID;

	/** Optional item ID */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FString ItemID;

	/** Reputation change amount */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	int32 ReputationChange = 0;

	/** Error message if applicable */
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
	FText ErrorMessage;
};

/**
 * Quick dialog option (for radial menu or quick actions)
 */
USTRUCT(BlueprintType)
struct FQuickDialogOption
{
	GENERATED_BODY()

	/** Display text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	FText DisplayText;

	/** Message to send to NPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	FString Message;

	/** Icon for UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	FString IconPath;

	/** Hotkey (1-9) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	int32 Hotkey = 0;
};
