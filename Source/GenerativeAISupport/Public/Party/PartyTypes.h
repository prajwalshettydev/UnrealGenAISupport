// Party System Types - Data structures for group/party management
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "PartyTypes.generated.h"

/**
 * Party member role
 */
UENUM(BlueprintType)
enum class EPartyRole : uint8
{
	None UMETA(DisplayName = "Keine"),
	Tank UMETA(DisplayName = "Tank"),
	Healer UMETA(DisplayName = "Heiler"),
	DamageDealer UMETA(DisplayName = "Schadensverursacher"),
	Support UMETA(DisplayName = "Unterstützer"),
	Leader UMETA(DisplayName = "Anführer")
};

/**
 * Loot distribution mode
 */
UENUM(BlueprintType)
enum class ELootMode : uint8
{
	FreeForAll UMETA(DisplayName = "Freie Beute"),
	RoundRobin UMETA(DisplayName = "Reihum"),
	MasterLoot UMETA(DisplayName = "Meisterbeute"),
	NeedBeforeGreed UMETA(DisplayName = "Bedarf vor Gier"),
	GroupLoot UMETA(DisplayName = "Gruppenbeute"),
	PersonalLoot UMETA(DisplayName = "Persönliche Beute")
};

/**
 * Party type
 */
UENUM(BlueprintType)
enum class EPartyType : uint8
{
	Party UMETA(DisplayName = "Gruppe"),
	Raid UMETA(DisplayName = "Schlachtzug"),
	Guild UMETA(DisplayName = "Gilde"),
	Battleground UMETA(DisplayName = "Schlachtfeld")
};

/**
 * Member status
 */
UENUM(BlueprintType)
enum class EPartyMemberStatus : uint8
{
	Online UMETA(DisplayName = "Online"),
	Away UMETA(DisplayName = "Abwesend"),
	Busy UMETA(DisplayName = "Beschäftigt"),
	Offline UMETA(DisplayName = "Offline"),
	InCombat UMETA(DisplayName = "Im Kampf"),
	Dead UMETA(DisplayName = "Tot")
};

/**
 * Party invitation status
 */
UENUM(BlueprintType)
enum class EInvitationStatus : uint8
{
	Pending UMETA(DisplayName = "Ausstehend"),
	Accepted UMETA(DisplayName = "Angenommen"),
	Declined UMETA(DisplayName = "Abgelehnt"),
	Expired UMETA(DisplayName = "Abgelaufen"),
	Cancelled UMETA(DisplayName = "Abgebrochen")
};

/**
 * Ready check status
 */
UENUM(BlueprintType)
enum class EReadyCheckStatus : uint8
{
	NotResponded UMETA(DisplayName = "Keine Antwort"),
	Ready UMETA(DisplayName = "Bereit"),
	NotReady UMETA(DisplayName = "Nicht Bereit"),
	Away UMETA(DisplayName = "Abwesend")
};

/**
 * Vote kick status
 */
UENUM(BlueprintType)
enum class EVoteKickStatus : uint8
{
	InProgress UMETA(DisplayName = "Läuft"),
	Approved UMETA(DisplayName = "Genehmigt"),
	Denied UMETA(DisplayName = "Abgelehnt"),
	Expired UMETA(DisplayName = "Abgelaufen")
};

/**
 * Party member data
 */
USTRUCT(BlueprintType)
struct FPartyMember
{
	GENERATED_BODY()

	/** Unique player ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FString PlayerID;

	/** Display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FString PlayerName;

	/** Character class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FString CharacterClass;

	/** Character level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	int32 Level = 1;

	/** Assigned role */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	EPartyRole Role = EPartyRole::None;

	/** Current status */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	EPartyMemberStatus Status = EPartyMemberStatus::Online;

	/** Current health percent (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	float HealthPercent = 1.0f;

	/** Current mana percent (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	float ManaPercent = 1.0f;

	/** Current location (zone name) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FString CurrentZone;

	/** World position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FVector Position;

	/** Is party leader? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	bool bIsLeader = false;

	/** Is assistant/co-leader? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	bool bIsAssistant = false;

	/** Is loot master? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	bool bIsLootMaster = false;

	/** Active buffs count */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	int32 ActiveBuffsCount = 0;

	/** Active debuffs count */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	int32 ActiveDebuffsCount = 0;

	/** Ready check response */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	EReadyCheckStatus ReadyStatus = EReadyCheckStatus::NotResponded;

	/** Portrait/avatar path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FString PortraitPath;
};

/**
 * Party invitation
 */
USTRUCT(BlueprintType)
struct FPartyInvitation
{
	GENERATED_BODY()

	/** Invitation ID */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString InvitationID;

	/** Inviter player ID */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString InviterID;

	/** Inviter name */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString InviterName;

	/** Party ID to join */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString PartyID;

	/** Party type */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	EPartyType PartyType = EPartyType::Party;

	/** Current party size */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	int32 CurrentPartySize = 1;

	/** Invitation status */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	EInvitationStatus Status = EInvitationStatus::Pending;

	/** When invitation was sent */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FDateTime SentTime;

	/** Expiration time in seconds */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	float ExpirationSeconds = 60.0f;
};

/**
 * Party settings
 */
USTRUCT(BlueprintType)
struct FPartySettings
{
	GENERATED_BODY()

	/** Loot distribution mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	ELootMode LootMode = ELootMode::GroupLoot;

	/** Minimum item quality for group loot roll */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	int32 LootThreshold = 2; // 0=Common, 1=Uncommon, 2=Rare, 3=Epic, 4=Legendary

	/** Can members invite others? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	bool bMembersCanInvite = false;

	/** Auto-accept guild members? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	bool bAutoAcceptGuildMembers = false;

	/** Auto-accept friends? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	bool bAutoAcceptFriends = false;

	/** Restrict to same faction? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	bool bSameFactionOnly = true;

	/** Maximum level difference for XP sharing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	int32 MaxLevelDifferenceForXP = 10;

	/** Share quest progress? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	bool bShareQuestProgress = true;

	/** Enable voice chat? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	bool bVoiceChatEnabled = true;

	/** Private party (not visible in finder)? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	bool bPrivate = false;
};

/**
 * Party data
 */
USTRUCT(BlueprintType)
struct FPartyData
{
	GENERATED_BODY()

	/** Unique party ID */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString PartyID;

	/** Party type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	EPartyType Type = EPartyType::Party;

	/** Party name (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FString PartyName;

	/** Party members */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	TArray<FPartyMember> Members;

	/** Maximum members allowed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	int32 MaxMembers = 5;

	/** Leader player ID */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString LeaderID;

	/** Party settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FPartySettings Settings;

	/** When party was created */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FDateTime CreatedTime;

	/** Current dungeon/instance (if any) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FString CurrentInstance;

	/** Party marks/targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	TMap<int32, FString> TargetMarks;

	/** Get member count */
	int32 GetMemberCount() const { return Members.Num(); }

	/** Is party full? */
	bool IsFull() const { return Members.Num() >= MaxMembers; }

	/** Is raid group? */
	bool IsRaid() const { return Type == EPartyType::Raid; }
};

/**
 * Ready check data
 */
USTRUCT(BlueprintType)
struct FReadyCheckData
{
	GENERATED_BODY()

	/** Is ready check active? */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	bool bIsActive = false;

	/** Who initiated the check */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString InitiatorID;

	/** When it started */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FDateTime StartTime;

	/** Duration in seconds */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	float Duration = 30.0f;

	/** Member responses */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	TMap<FString, EReadyCheckStatus> Responses;
};

/**
 * Vote kick data
 */
USTRUCT(BlueprintType)
struct FVoteKickData
{
	GENERATED_BODY()

	/** Target player ID */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString TargetID;

	/** Target name */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString TargetName;

	/** Reason for kick */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString Reason;

	/** Who initiated */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString InitiatorID;

	/** Votes for (yes) */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	TArray<FString> VotesFor;

	/** Votes against (no) */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	TArray<FString> VotesAgainst;

	/** When it started */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FDateTime StartTime;

	/** Voting duration in seconds */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	float VoteDuration = 60.0f;

	/** Current status */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	EVoteKickStatus Status = EVoteKickStatus::InProgress;
};

/**
 * Loot roll data
 */
USTRUCT(BlueprintType)
struct FLootRollData
{
	GENERATED_BODY()

	/** Item being rolled for */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString ItemID;

	/** Item display name */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString ItemName;

	/** Item quality tier */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	int32 ItemQuality = 0;

	/** Who can roll (player IDs) */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	TArray<FString> EligiblePlayers;

	/** Need rolls (playerID -> roll value) */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	TMap<FString, int32> NeedRolls;

	/** Greed rolls */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	TMap<FString, int32> GreedRolls;

	/** Players who passed */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	TArray<FString> PassedPlayers;

	/** Roll duration in seconds */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	float RollDuration = 30.0f;

	/** When roll started */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FDateTime StartTime;
};

/**
 * Party buff data
 */
USTRUCT(BlueprintType)
struct FPartyBuff
{
	GENERATED_BODY()

	/** Buff ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FString BuffID;

	/** Buff display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FText DisplayName;

	/** Source player ID */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString SourcePlayerID;

	/** Stat modifiers (stat name -> value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	TMap<FString, float> StatModifiers;

	/** Duration in seconds (-1 = permanent until cancelled) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	float Duration = 300.0f;

	/** Remaining time */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	float RemainingTime = 300.0f;

	/** Icon path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FString IconPath;

	/** Is aura (constantly reapplies when near source)? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	bool bIsAura = false;

	/** Aura range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	float AuraRange = 3000.0f;
};

/**
 * Group finder entry
 */
USTRUCT(BlueprintType)
struct FGroupFinderEntry
{
	GENERATED_BODY()

	/** Party ID */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString PartyID;

	/** Party name/description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FString Description;

	/** Activity/dungeon name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	FString ActivityName;

	/** Leader name */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString LeaderName;

	/** Current/max members */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	int32 CurrentMembers = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Party")
	int32 MaxMembers = 5;

	/** Required item level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	int32 RequiredItemLevel = 0;

	/** Required roles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	TArray<EPartyRole> LookingForRoles;

	/** Average party level */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	int32 AverageLevel = 1;

	/** Voice chat required? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	bool bVoiceChatRequired = false;

	/** When listing was created */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FDateTime ListedTime;
};

/**
 * XP sharing result
 */
USTRUCT(BlueprintType)
struct FXPShareResult
{
	GENERATED_BODY()

	/** Total XP to distribute */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	int32 TotalXP = 0;

	/** XP per member (player ID -> XP amount) */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	TMap<FString, int32> MemberXP;

	/** Players who got reduced XP due to level difference */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	TArray<FString> ReducedXPPlayers;

	/** Bonus XP from party synergy */
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	int32 SynergyBonus = 0;
};
