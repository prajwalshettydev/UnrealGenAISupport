// Party Library - Blueprint functions for group/party management
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PartyTypes.h"
#include "PartyLibrary.generated.h"

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyCreated, const FPartyData&, Party);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyDisbanded, const FString&, PartyID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMemberJoined, const FString&, PartyID, const FPartyMember&, Member);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMemberLeft, const FString&, PartyID, const FString&, PlayerID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLeaderChanged, const FString&, PartyID, const FString&, NewLeaderID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvitationReceived, const FPartyInvitation&, Invitation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReadyCheckStarted, const FReadyCheckData&, ReadyCheck);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLootRollStarted, const FLootRollData&, LootRoll);

/**
 * Blueprint Function Library for Party/Group System
 */
UCLASS()
class GENERATIVEAISUPPORT_API UPartyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// PARTY CREATION & MANAGEMENT
	// ============================================

	/**
	 * Create a new party
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Management")
	static FPartyData CreateParty(
		const FString& LeaderID,
		const FString& LeaderName,
		EPartyType Type = EPartyType::Party);

	/**
	 * Disband party
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Management")
	static bool DisbandParty(const FString& PartyID, const FString& RequestingPlayerID);

	/**
	 * Get party by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Management")
	static bool GetParty(const FString& PartyID, FPartyData& OutParty);

	/**
	 * Get player's current party
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Management")
	static bool GetPlayerParty(const FString& PlayerID, FPartyData& OutParty);

	/**
	 * Is player in a party?
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Management")
	static bool IsInParty(const FString& PlayerID);

	/**
	 * Is player party leader?
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Management")
	static bool IsPartyLeader(const FString& PlayerID);

	/**
	 * Convert party to raid
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Management")
	static bool ConvertToRaid(const FString& PartyID, const FString& RequestingPlayerID);

	/**
	 * Convert raid back to party
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Management")
	static bool ConvertToParty(const FString& PartyID, const FString& RequestingPlayerID);

	// ============================================
	// MEMBER MANAGEMENT
	// ============================================

	/**
	 * Invite player to party
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Members")
	static FPartyInvitation InvitePlayer(
		const FString& PartyID,
		const FString& InviterID,
		const FString& InviteeID,
		const FString& InviteeName);

	/**
	 * Accept party invitation
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Members")
	static bool AcceptInvitation(const FString& InvitationID, const FPartyMember& JoiningMember);

	/**
	 * Decline party invitation
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Members")
	static bool DeclineInvitation(const FString& InvitationID);

	/**
	 * Leave party
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Members")
	static bool LeaveParty(const FString& PlayerID);

	/**
	 * Kick member from party
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Members")
	static bool KickMember(const FString& PartyID, const FString& KickerID, const FString& TargetID);

	/**
	 * Get party member by ID
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Members")
	static bool GetMember(const FString& PartyID, const FString& PlayerID, FPartyMember& OutMember);

	/**
	 * Update member data
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Members")
	static bool UpdateMemberData(const FString& PartyID, const FPartyMember& UpdatedMember);

	/**
	 * Get all party members
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Members")
	static TArray<FPartyMember> GetAllMembers(const FString& PartyID);

	/**
	 * Get party member count
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Members")
	static int32 GetMemberCount(const FString& PartyID);

	// ============================================
	// ROLES & LEADERSHIP
	// ============================================

	/**
	 * Set member role
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Roles")
	static bool SetMemberRole(
		const FString& PartyID,
		const FString& SetterID,
		const FString& TargetID,
		EPartyRole NewRole);

	/**
	 * Get members by role
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Roles")
	static TArray<FPartyMember> GetMembersByRole(const FString& PartyID, EPartyRole Role);

	/**
	 * Transfer leadership
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Roles")
	static bool TransferLeadership(const FString& PartyID, const FString& CurrentLeaderID, const FString& NewLeaderID);

	/**
	 * Promote to assistant
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Roles")
	static bool PromoteToAssistant(const FString& PartyID, const FString& LeaderID, const FString& TargetID);

	/**
	 * Demote from assistant
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Roles")
	static bool DemoteFromAssistant(const FString& PartyID, const FString& LeaderID, const FString& TargetID);

	/**
	 * Set loot master
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Roles")
	static bool SetLootMaster(const FString& PartyID, const FString& LeaderID, const FString& NewLootMasterID);

	// ============================================
	// PARTY SETTINGS
	// ============================================

	/**
	 * Update party settings
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Settings")
	static bool UpdatePartySettings(const FString& PartyID, const FString& LeaderID, const FPartySettings& NewSettings);

	/**
	 * Get party settings
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Settings")
	static FPartySettings GetPartySettings(const FString& PartyID);

	/**
	 * Set loot mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Settings")
	static bool SetLootMode(const FString& PartyID, const FString& LeaderID, ELootMode NewMode);

	/**
	 * Set loot threshold
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Settings")
	static bool SetLootThreshold(const FString& PartyID, const FString& LeaderID, int32 NewThreshold);

	// ============================================
	// READY CHECK
	// ============================================

	/**
	 * Start ready check
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|ReadyCheck")
	static bool StartReadyCheck(const FString& PartyID, const FString& InitiatorID, float Duration = 30.0f);

	/**
	 * Respond to ready check
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|ReadyCheck")
	static bool RespondToReadyCheck(const FString& PartyID, const FString& PlayerID, bool bIsReady);

	/**
	 * Get ready check status
	 */
	UFUNCTION(BlueprintPure, Category = "Party|ReadyCheck")
	static FReadyCheckData GetReadyCheckStatus(const FString& PartyID);

	/**
	 * Is ready check active?
	 */
	UFUNCTION(BlueprintPure, Category = "Party|ReadyCheck")
	static bool IsReadyCheckActive(const FString& PartyID);

	/**
	 * Cancel ready check
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|ReadyCheck")
	static bool CancelReadyCheck(const FString& PartyID, const FString& LeaderID);

	// ============================================
	// VOTE KICK
	// ============================================

	/**
	 * Initiate vote kick
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|VoteKick")
	static bool InitiateVoteKick(
		const FString& PartyID,
		const FString& InitiatorID,
		const FString& TargetID,
		const FString& Reason);

	/**
	 * Vote on kick
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|VoteKick")
	static bool VoteOnKick(const FString& PartyID, const FString& VoterID, bool bVoteYes);

	/**
	 * Get vote kick status
	 */
	UFUNCTION(BlueprintPure, Category = "Party|VoteKick")
	static FVoteKickData GetVoteKickStatus(const FString& PartyID);

	/**
	 * Is vote kick active?
	 */
	UFUNCTION(BlueprintPure, Category = "Party|VoteKick")
	static bool IsVoteKickActive(const FString& PartyID);

	// ============================================
	// LOOT MANAGEMENT
	// ============================================

	/**
	 * Start loot roll
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Loot")
	static FLootRollData StartLootRoll(
		const FString& PartyID,
		const FString& ItemID,
		const FString& ItemName,
		int32 ItemQuality,
		float RollDuration = 30.0f);

	/**
	 * Roll for loot (Need/Greed/Pass)
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Loot")
	static bool RollForLoot(
		const FString& PartyID,
		const FString& ItemID,
		const FString& PlayerID,
		bool bNeed); // true = Need, false = Greed

	/**
	 * Pass on loot
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Loot")
	static bool PassOnLoot(const FString& PartyID, const FString& ItemID, const FString& PlayerID);

	/**
	 * Get loot roll winner
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Loot")
	static FString GetLootRollWinner(const FString& PartyID, const FString& ItemID);

	/**
	 * Distribute loot (master looter)
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Loot")
	static bool DistributeLoot(
		const FString& PartyID,
		const FString& LootMasterID,
		const FString& ItemID,
		const FString& RecipientID);

	// ============================================
	// XP & REWARDS
	// ============================================

	/**
	 * Calculate XP share for party
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Rewards")
	static FXPShareResult CalculateXPShare(const FString& PartyID, int32 BaseXP, int32 EnemyLevel);

	/**
	 * Get party synergy bonus (bonus XP for balanced roles)
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Rewards")
	static float GetPartySynergyBonus(const FString& PartyID);

	// ============================================
	// PARTY BUFFS
	// ============================================

	/**
	 * Apply party buff
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Buffs")
	static bool ApplyPartyBuff(const FString& PartyID, const FString& SourcePlayerID, const FPartyBuff& Buff);

	/**
	 * Remove party buff
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Buffs")
	static bool RemovePartyBuff(const FString& PartyID, const FString& BuffID);

	/**
	 * Get active party buffs
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Buffs")
	static TArray<FPartyBuff> GetActivePartyBuffs(const FString& PartyID);

	/**
	 * Update party buff timers
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Buffs")
	static void UpdatePartyBuffs(const FString& PartyID, float DeltaTime);

	// ============================================
	// TARGET MARKERS
	// ============================================

	/**
	 * Set target mark
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Markers")
	static bool SetTargetMark(
		const FString& PartyID,
		const FString& SetterID,
		int32 MarkIndex,
		const FString& TargetID);

	/**
	 * Clear target mark
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Markers")
	static bool ClearTargetMark(const FString& PartyID, const FString& SetterID, int32 MarkIndex);

	/**
	 * Get all target marks
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Markers")
	static TMap<int32, FString> GetTargetMarks(const FString& PartyID);

	// ============================================
	// GROUP FINDER
	// ============================================

	/**
	 * List party in group finder
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|GroupFinder")
	static bool ListInGroupFinder(const FString& PartyID, const FGroupFinderEntry& Listing);

	/**
	 * Unlist from group finder
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|GroupFinder")
	static bool UnlistFromGroupFinder(const FString& PartyID);

	/**
	 * Search group finder
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|GroupFinder")
	static TArray<FGroupFinderEntry> SearchGroupFinder(
		const FString& ActivityFilter,
		EPartyRole RoleFilter,
		int32 MinLevel,
		int32 MaxLevel);

	/**
	 * Apply to join group
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|GroupFinder")
	static bool ApplyToGroup(const FString& PartyID, const FPartyMember& Applicant);

	// ============================================
	// DISPLAY HELPERS
	// ============================================

	/**
	 * Get role display name
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Display")
	static FText GetRoleDisplayName(EPartyRole Role);

	/**
	 * Get role icon path
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Display")
	static FString GetRoleIconPath(EPartyRole Role);

	/**
	 * Get role color
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Display")
	static FLinearColor GetRoleColor(EPartyRole Role);

	/**
	 * Get status display name
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Display")
	static FText GetStatusDisplayName(EPartyMemberStatus Status);

	/**
	 * Get party type display name
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Display")
	static FText GetPartyTypeDisplayName(EPartyType Type);

	/**
	 * Get loot mode display name
	 */
	UFUNCTION(BlueprintPure, Category = "Party|Display")
	static FText GetLootModeDisplayName(ELootMode Mode);

	// ============================================
	// SAVE/LOAD
	// ============================================

	/**
	 * Save party data to JSON
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Save")
	static FString SavePartyToJSON(const FString& PartyID);

	/**
	 * Load party data from JSON
	 */
	UFUNCTION(BlueprintCallable, Category = "Party|Save")
	static bool LoadPartyFromJSON(const FString& JSONString, FString& OutPartyID);

private:
	/** Active parties */
	static TMap<FString, FPartyData> ActiveParties;

	/** Player to party mapping */
	static TMap<FString, FString> PlayerPartyMap;

	/** Pending invitations */
	static TMap<FString, FPartyInvitation> PendingInvitations;

	/** Active ready checks */
	static TMap<FString, FReadyCheckData> ActiveReadyChecks;

	/** Active vote kicks */
	static TMap<FString, FVoteKickData> ActiveVoteKicks;

	/** Active loot rolls */
	static TMap<FString, TMap<FString, FLootRollData>> ActiveLootRolls;

	/** Active party buffs */
	static TMap<FString, TArray<FPartyBuff>> ActivePartyBuffs;

	/** Group finder listings */
	static TMap<FString, FGroupFinderEntry> GroupFinderListings;

	/** Generate unique ID */
	static FString GenerateUniqueID();

	/** Check if player can modify party */
	static bool CanModifyParty(const FString& PartyID, const FString& PlayerID);
};
