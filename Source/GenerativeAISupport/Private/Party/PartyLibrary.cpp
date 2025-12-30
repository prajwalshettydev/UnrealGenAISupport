// Party Library Implementation
// Part of GenerativeAISupport Plugin

#include "Party/PartyLibrary.h"
#include "Misc/Guid.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// Static member initialization
TMap<FString, FPartyData> UPartyLibrary::ActiveParties;
TMap<FString, FString> UPartyLibrary::PlayerPartyMap;
TMap<FString, FPartyInvitation> UPartyLibrary::PendingInvitations;
TMap<FString, FReadyCheckData> UPartyLibrary::ActiveReadyChecks;
TMap<FString, FVoteKickData> UPartyLibrary::ActiveVoteKicks;
TMap<FString, TMap<FString, FLootRollData>> UPartyLibrary::ActiveLootRolls;
TMap<FString, TArray<FPartyBuff>> UPartyLibrary::ActivePartyBuffs;
TMap<FString, FGroupFinderEntry> UPartyLibrary::GroupFinderListings;

// ============================================
// PARTY CREATION & MANAGEMENT
// ============================================

FPartyData UPartyLibrary::CreateParty(const FString& LeaderID, const FString& LeaderName, EPartyType Type)
{
	// Leave existing party if any
	LeaveParty(LeaderID);

	FPartyData NewParty;
	NewParty.PartyID = GenerateUniqueID();
	NewParty.Type = Type;
	NewParty.LeaderID = LeaderID;
	NewParty.CreatedTime = FDateTime::Now();

	// Set max members based on type
	switch (Type)
	{
	case EPartyType::Party:
		NewParty.MaxMembers = 5;
		break;
	case EPartyType::Raid:
		NewParty.MaxMembers = 40;
		break;
	case EPartyType::Battleground:
		NewParty.MaxMembers = 15;
		break;
	default:
		NewParty.MaxMembers = 5;
		break;
	}

	// Add leader as first member
	FPartyMember Leader;
	Leader.PlayerID = LeaderID;
	Leader.PlayerName = LeaderName;
	Leader.bIsLeader = true;
	Leader.Role = EPartyRole::Leader;
	NewParty.Members.Add(Leader);

	// Register party
	ActiveParties.Add(NewParty.PartyID, NewParty);
	PlayerPartyMap.Add(LeaderID, NewParty.PartyID);

	return NewParty;
}

bool UPartyLibrary::DisbandParty(const FString& PartyID, const FString& RequestingPlayerID)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	// Only leader can disband
	if (Party->LeaderID != RequestingPlayerID) return false;

	// Remove all members from party map
	for (const FPartyMember& Member : Party->Members)
	{
		PlayerPartyMap.Remove(Member.PlayerID);
	}

	// Clean up related data
	ActiveReadyChecks.Remove(PartyID);
	ActiveVoteKicks.Remove(PartyID);
	ActiveLootRolls.Remove(PartyID);
	ActivePartyBuffs.Remove(PartyID);
	GroupFinderListings.Remove(PartyID);

	// Remove party
	ActiveParties.Remove(PartyID);

	return true;
}

bool UPartyLibrary::GetParty(const FString& PartyID, FPartyData& OutParty)
{
	if (const FPartyData* Party = ActiveParties.Find(PartyID))
	{
		OutParty = *Party;
		return true;
	}
	return false;
}

bool UPartyLibrary::GetPlayerParty(const FString& PlayerID, FPartyData& OutParty)
{
	if (const FString* PartyID = PlayerPartyMap.Find(PlayerID))
	{
		return GetParty(*PartyID, OutParty);
	}
	return false;
}

bool UPartyLibrary::IsInParty(const FString& PlayerID)
{
	return PlayerPartyMap.Contains(PlayerID);
}

bool UPartyLibrary::IsPartyLeader(const FString& PlayerID)
{
	if (const FString* PartyID = PlayerPartyMap.Find(PlayerID))
	{
		if (const FPartyData* Party = ActiveParties.Find(*PartyID))
		{
			return Party->LeaderID == PlayerID;
		}
	}
	return false;
}

bool UPartyLibrary::ConvertToRaid(const FString& PartyID, const FString& RequestingPlayerID)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (Party->LeaderID != RequestingPlayerID) return false;
	if (Party->Type == EPartyType::Raid) return false;

	Party->Type = EPartyType::Raid;
	Party->MaxMembers = 40;

	return true;
}

bool UPartyLibrary::ConvertToParty(const FString& PartyID, const FString& RequestingPlayerID)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (Party->LeaderID != RequestingPlayerID) return false;
	if (Party->Type != EPartyType::Raid) return false;
	if (Party->Members.Num() > 5) return false; // Can't convert if too many members

	Party->Type = EPartyType::Party;
	Party->MaxMembers = 5;

	return true;
}

// ============================================
// MEMBER MANAGEMENT
// ============================================

FPartyInvitation UPartyLibrary::InvitePlayer(
	const FString& PartyID,
	const FString& InviterID,
	const FString& InviteeID,
	const FString& InviteeName)
{
	FPartyInvitation Invitation;
	Invitation.InvitationID = GenerateUniqueID();
	Invitation.PartyID = PartyID;
	Invitation.InviterID = InviterID;
	Invitation.Status = EInvitationStatus::Pending;
	Invitation.SentTime = FDateTime::Now();

	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party)
	{
		Invitation.Status = EInvitationStatus::Expired;
		return Invitation;
	}

	// Check if party is full
	if (Party->IsFull())
	{
		Invitation.Status = EInvitationStatus::Expired;
		return Invitation;
	}

	// Check permissions
	bool bCanInvite = (Party->LeaderID == InviterID) ||
	                  Party->Settings.bMembersCanInvite;

	// Check if inviter is assistant
	for (const FPartyMember& Member : Party->Members)
	{
		if (Member.PlayerID == InviterID && Member.bIsAssistant)
		{
			bCanInvite = true;
			break;
		}
	}

	if (!bCanInvite)
	{
		Invitation.Status = EInvitationStatus::Cancelled;
		return Invitation;
	}

	// Get inviter name
	for (const FPartyMember& Member : Party->Members)
	{
		if (Member.PlayerID == InviterID)
		{
			Invitation.InviterName = Member.PlayerName;
			break;
		}
	}

	Invitation.PartyType = Party->Type;
	Invitation.CurrentPartySize = Party->Members.Num();

	PendingInvitations.Add(Invitation.InvitationID, Invitation);

	return Invitation;
}

bool UPartyLibrary::AcceptInvitation(const FString& InvitationID, const FPartyMember& JoiningMember)
{
	FPartyInvitation* Invitation = PendingInvitations.Find(InvitationID);
	if (!Invitation) return false;

	if (Invitation->Status != EInvitationStatus::Pending) return false;

	FPartyData* Party = ActiveParties.Find(Invitation->PartyID);
	if (!Party)
	{
		Invitation->Status = EInvitationStatus::Expired;
		return false;
	}

	if (Party->IsFull())
	{
		Invitation->Status = EInvitationStatus::Expired;
		return false;
	}

	// Leave any existing party
	LeaveParty(JoiningMember.PlayerID);

	// Add to party
	FPartyMember NewMember = JoiningMember;
	NewMember.bIsLeader = false;
	NewMember.bIsAssistant = false;
	Party->Members.Add(NewMember);

	PlayerPartyMap.Add(JoiningMember.PlayerID, Party->PartyID);

	Invitation->Status = EInvitationStatus::Accepted;

	return true;
}

bool UPartyLibrary::DeclineInvitation(const FString& InvitationID)
{
	FPartyInvitation* Invitation = PendingInvitations.Find(InvitationID);
	if (!Invitation) return false;

	Invitation->Status = EInvitationStatus::Declined;
	return true;
}

bool UPartyLibrary::LeaveParty(const FString& PlayerID)
{
	const FString* PartyIDPtr = PlayerPartyMap.Find(PlayerID);
	if (!PartyIDPtr) return false;

	FString PartyID = *PartyIDPtr;
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	// Remove from party
	for (int32 i = Party->Members.Num() - 1; i >= 0; i--)
	{
		if (Party->Members[i].PlayerID == PlayerID)
		{
			Party->Members.RemoveAt(i);
			break;
		}
	}

	PlayerPartyMap.Remove(PlayerID);

	// If party is empty, disband
	if (Party->Members.Num() == 0)
	{
		ActiveParties.Remove(PartyID);
		return true;
	}

	// If leader left, assign new leader
	if (Party->LeaderID == PlayerID && Party->Members.Num() > 0)
	{
		Party->LeaderID = Party->Members[0].PlayerID;
		Party->Members[0].bIsLeader = true;
		Party->Members[0].Role = EPartyRole::Leader;
	}

	return true;
}

bool UPartyLibrary::KickMember(const FString& PartyID, const FString& KickerID, const FString& TargetID)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	// Only leader or assistant can kick
	if (!CanModifyParty(PartyID, KickerID)) return false;

	// Can't kick the leader
	if (TargetID == Party->LeaderID) return false;

	// Can't kick yourself
	if (TargetID == KickerID) return false;

	// Remove member
	for (int32 i = Party->Members.Num() - 1; i >= 0; i--)
	{
		if (Party->Members[i].PlayerID == TargetID)
		{
			Party->Members.RemoveAt(i);
			PlayerPartyMap.Remove(TargetID);
			return true;
		}
	}

	return false;
}

bool UPartyLibrary::GetMember(const FString& PartyID, const FString& PlayerID, FPartyMember& OutMember)
{
	const FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	for (const FPartyMember& Member : Party->Members)
	{
		if (Member.PlayerID == PlayerID)
		{
			OutMember = Member;
			return true;
		}
	}

	return false;
}

bool UPartyLibrary::UpdateMemberData(const FString& PartyID, const FPartyMember& UpdatedMember)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	for (FPartyMember& Member : Party->Members)
	{
		if (Member.PlayerID == UpdatedMember.PlayerID)
		{
			// Preserve leadership flags
			bool bWasLeader = Member.bIsLeader;
			bool bWasAssistant = Member.bIsAssistant;
			bool bWasLootMaster = Member.bIsLootMaster;

			Member = UpdatedMember;
			Member.bIsLeader = bWasLeader;
			Member.bIsAssistant = bWasAssistant;
			Member.bIsLootMaster = bWasLootMaster;

			return true;
		}
	}

	return false;
}

TArray<FPartyMember> UPartyLibrary::GetAllMembers(const FString& PartyID)
{
	if (const FPartyData* Party = ActiveParties.Find(PartyID))
	{
		return Party->Members;
	}
	return TArray<FPartyMember>();
}

int32 UPartyLibrary::GetMemberCount(const FString& PartyID)
{
	if (const FPartyData* Party = ActiveParties.Find(PartyID))
	{
		return Party->Members.Num();
	}
	return 0;
}

// ============================================
// ROLES & LEADERSHIP
// ============================================

bool UPartyLibrary::SetMemberRole(
	const FString& PartyID,
	const FString& SetterID,
	const FString& TargetID,
	EPartyRole NewRole)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	// Anyone can set their own role, leader/assistant can set others
	bool bCanSet = (SetterID == TargetID) || CanModifyParty(PartyID, SetterID);
	if (!bCanSet) return false;

	for (FPartyMember& Member : Party->Members)
	{
		if (Member.PlayerID == TargetID)
		{
			Member.Role = NewRole;
			return true;
		}
	}

	return false;
}

TArray<FPartyMember> UPartyLibrary::GetMembersByRole(const FString& PartyID, EPartyRole Role)
{
	TArray<FPartyMember> Result;

	if (const FPartyData* Party = ActiveParties.Find(PartyID))
	{
		for (const FPartyMember& Member : Party->Members)
		{
			if (Member.Role == Role)
			{
				Result.Add(Member);
			}
		}
	}

	return Result;
}

bool UPartyLibrary::TransferLeadership(const FString& PartyID, const FString& CurrentLeaderID, const FString& NewLeaderID)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (Party->LeaderID != CurrentLeaderID) return false;
	if (CurrentLeaderID == NewLeaderID) return false;

	// Find both members
	FPartyMember* OldLeader = nullptr;
	FPartyMember* NewLeader = nullptr;

	for (FPartyMember& Member : Party->Members)
	{
		if (Member.PlayerID == CurrentLeaderID) OldLeader = &Member;
		if (Member.PlayerID == NewLeaderID) NewLeader = &Member;
	}

	if (!OldLeader || !NewLeader) return false;

	OldLeader->bIsLeader = false;
	if (OldLeader->Role == EPartyRole::Leader) OldLeader->Role = EPartyRole::None;

	NewLeader->bIsLeader = true;
	NewLeader->Role = EPartyRole::Leader;

	Party->LeaderID = NewLeaderID;

	return true;
}

bool UPartyLibrary::PromoteToAssistant(const FString& PartyID, const FString& LeaderID, const FString& TargetID)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (Party->LeaderID != LeaderID) return false;

	for (FPartyMember& Member : Party->Members)
	{
		if (Member.PlayerID == TargetID)
		{
			Member.bIsAssistant = true;
			return true;
		}
	}

	return false;
}

bool UPartyLibrary::DemoteFromAssistant(const FString& PartyID, const FString& LeaderID, const FString& TargetID)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (Party->LeaderID != LeaderID) return false;

	for (FPartyMember& Member : Party->Members)
	{
		if (Member.PlayerID == TargetID)
		{
			Member.bIsAssistant = false;
			return true;
		}
	}

	return false;
}

bool UPartyLibrary::SetLootMaster(const FString& PartyID, const FString& LeaderID, const FString& NewLootMasterID)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (Party->LeaderID != LeaderID) return false;

	// Clear old loot master
	for (FPartyMember& Member : Party->Members)
	{
		Member.bIsLootMaster = (Member.PlayerID == NewLootMasterID);
	}

	return true;
}

// ============================================
// PARTY SETTINGS
// ============================================

bool UPartyLibrary::UpdatePartySettings(const FString& PartyID, const FString& LeaderID, const FPartySettings& NewSettings)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (Party->LeaderID != LeaderID) return false;

	Party->Settings = NewSettings;
	return true;
}

FPartySettings UPartyLibrary::GetPartySettings(const FString& PartyID)
{
	if (const FPartyData* Party = ActiveParties.Find(PartyID))
	{
		return Party->Settings;
	}
	return FPartySettings();
}

bool UPartyLibrary::SetLootMode(const FString& PartyID, const FString& LeaderID, ELootMode NewMode)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (Party->LeaderID != LeaderID) return false;

	Party->Settings.LootMode = NewMode;
	return true;
}

bool UPartyLibrary::SetLootThreshold(const FString& PartyID, const FString& LeaderID, int32 NewThreshold)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (Party->LeaderID != LeaderID) return false;

	Party->Settings.LootThreshold = FMath::Clamp(NewThreshold, 0, 4);
	return true;
}

// ============================================
// READY CHECK
// ============================================

bool UPartyLibrary::StartReadyCheck(const FString& PartyID, const FString& InitiatorID, float Duration)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (!CanModifyParty(PartyID, InitiatorID)) return false;

	if (ActiveReadyChecks.Contains(PartyID)) return false; // Already active

	FReadyCheckData ReadyCheck;
	ReadyCheck.bIsActive = true;
	ReadyCheck.InitiatorID = InitiatorID;
	ReadyCheck.StartTime = FDateTime::Now();
	ReadyCheck.Duration = Duration;

	// Initialize all members as not responded
	for (const FPartyMember& Member : Party->Members)
	{
		ReadyCheck.Responses.Add(Member.PlayerID, EReadyCheckStatus::NotResponded);
	}

	ActiveReadyChecks.Add(PartyID, ReadyCheck);
	return true;
}

bool UPartyLibrary::RespondToReadyCheck(const FString& PartyID, const FString& PlayerID, bool bIsReady)
{
	FReadyCheckData* ReadyCheck = ActiveReadyChecks.Find(PartyID);
	if (!ReadyCheck || !ReadyCheck->bIsActive) return false;

	if (EReadyCheckStatus* Response = ReadyCheck->Responses.Find(PlayerID))
	{
		*Response = bIsReady ? EReadyCheckStatus::Ready : EReadyCheckStatus::NotReady;
		return true;
	}

	return false;
}

FReadyCheckData UPartyLibrary::GetReadyCheckStatus(const FString& PartyID)
{
	if (const FReadyCheckData* ReadyCheck = ActiveReadyChecks.Find(PartyID))
	{
		return *ReadyCheck;
	}
	return FReadyCheckData();
}

bool UPartyLibrary::IsReadyCheckActive(const FString& PartyID)
{
	if (const FReadyCheckData* ReadyCheck = ActiveReadyChecks.Find(PartyID))
	{
		return ReadyCheck->bIsActive;
	}
	return false;
}

bool UPartyLibrary::CancelReadyCheck(const FString& PartyID, const FString& LeaderID)
{
	if (!CanModifyParty(PartyID, LeaderID)) return false;

	return ActiveReadyChecks.Remove(PartyID) > 0;
}

// ============================================
// VOTE KICK
// ============================================

bool UPartyLibrary::InitiateVoteKick(
	const FString& PartyID,
	const FString& InitiatorID,
	const FString& TargetID,
	const FString& Reason)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	// Can't kick leader
	if (TargetID == Party->LeaderID) return false;

	// Already vote kick in progress
	if (ActiveVoteKicks.Contains(PartyID)) return false;

	FVoteKickData VoteKick;
	VoteKick.TargetID = TargetID;
	VoteKick.Reason = Reason;
	VoteKick.InitiatorID = InitiatorID;
	VoteKick.StartTime = FDateTime::Now();

	// Get target name
	for (const FPartyMember& Member : Party->Members)
	{
		if (Member.PlayerID == TargetID)
		{
			VoteKick.TargetName = Member.PlayerName;
			break;
		}
	}

	// Initiator votes yes
	VoteKick.VotesFor.Add(InitiatorID);

	ActiveVoteKicks.Add(PartyID, VoteKick);
	return true;
}

bool UPartyLibrary::VoteOnKick(const FString& PartyID, const FString& VoterID, bool bVoteYes)
{
	FVoteKickData* VoteKick = ActiveVoteKicks.Find(PartyID);
	if (!VoteKick) return false;

	// Can't vote on yourself
	if (VoterID == VoteKick->TargetID) return false;

	// Already voted?
	if (VoteKick->VotesFor.Contains(VoterID) || VoteKick->VotesAgainst.Contains(VoterID))
	{
		return false;
	}

	if (bVoteYes)
	{
		VoteKick->VotesFor.Add(VoterID);
	}
	else
	{
		VoteKick->VotesAgainst.Add(VoterID);
	}

	// Check if vote is decided
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (Party)
	{
		int32 VotingMembers = Party->Members.Num() - 1; // Exclude target
		int32 MajorityNeeded = (VotingMembers / 2) + 1;

		if (VoteKick->VotesFor.Num() >= MajorityNeeded)
		{
			VoteKick->Status = EVoteKickStatus::Approved;
			KickMember(PartyID, Party->LeaderID, VoteKick->TargetID);
		}
		else if (VoteKick->VotesAgainst.Num() >= MajorityNeeded)
		{
			VoteKick->Status = EVoteKickStatus::Denied;
		}
	}

	return true;
}

FVoteKickData UPartyLibrary::GetVoteKickStatus(const FString& PartyID)
{
	if (const FVoteKickData* VoteKick = ActiveVoteKicks.Find(PartyID))
	{
		return *VoteKick;
	}
	return FVoteKickData();
}

bool UPartyLibrary::IsVoteKickActive(const FString& PartyID)
{
	if (const FVoteKickData* VoteKick = ActiveVoteKicks.Find(PartyID))
	{
		return VoteKick->Status == EVoteKickStatus::InProgress;
	}
	return false;
}

// ============================================
// LOOT MANAGEMENT
// ============================================

FLootRollData UPartyLibrary::StartLootRoll(
	const FString& PartyID,
	const FString& ItemID,
	const FString& ItemName,
	int32 ItemQuality,
	float RollDuration)
{
	FLootRollData RollData;
	RollData.ItemID = ItemID;
	RollData.ItemName = ItemName;
	RollData.ItemQuality = ItemQuality;
	RollData.RollDuration = RollDuration;
	RollData.StartTime = FDateTime::Now();

	if (const FPartyData* Party = ActiveParties.Find(PartyID))
	{
		for (const FPartyMember& Member : Party->Members)
		{
			if (Member.Status != EPartyMemberStatus::Offline)
			{
				RollData.EligiblePlayers.Add(Member.PlayerID);
			}
		}
	}

	if (!ActiveLootRolls.Contains(PartyID))
	{
		ActiveLootRolls.Add(PartyID, TMap<FString, FLootRollData>());
	}

	ActiveLootRolls[PartyID].Add(ItemID, RollData);

	return RollData;
}

bool UPartyLibrary::RollForLoot(
	const FString& PartyID,
	const FString& ItemID,
	const FString& PlayerID,
	bool bNeed)
{
	if (!ActiveLootRolls.Contains(PartyID)) return false;

	FLootRollData* RollData = ActiveLootRolls[PartyID].Find(ItemID);
	if (!RollData) return false;

	if (!RollData->EligiblePlayers.Contains(PlayerID)) return false;

	// Already rolled?
	if (RollData->NeedRolls.Contains(PlayerID) ||
	    RollData->GreedRolls.Contains(PlayerID) ||
	    RollData->PassedPlayers.Contains(PlayerID))
	{
		return false;
	}

	int32 Roll = FMath::RandRange(1, 100);

	if (bNeed)
	{
		RollData->NeedRolls.Add(PlayerID, Roll);
	}
	else
	{
		RollData->GreedRolls.Add(PlayerID, Roll);
	}

	return true;
}

bool UPartyLibrary::PassOnLoot(const FString& PartyID, const FString& ItemID, const FString& PlayerID)
{
	if (!ActiveLootRolls.Contains(PartyID)) return false;

	FLootRollData* RollData = ActiveLootRolls[PartyID].Find(ItemID);
	if (!RollData) return false;

	if (!RollData->EligiblePlayers.Contains(PlayerID)) return false;

	RollData->PassedPlayers.Add(PlayerID);
	return true;
}

FString UPartyLibrary::GetLootRollWinner(const FString& PartyID, const FString& ItemID)
{
	if (!ActiveLootRolls.Contains(PartyID)) return TEXT("");

	const FLootRollData* RollData = ActiveLootRolls[PartyID].Find(ItemID);
	if (!RollData) return TEXT("");

	// Need rolls win over greed
	FString Winner;
	int32 HighestRoll = -1;

	for (const auto& Pair : RollData->NeedRolls)
	{
		if (Pair.Value > HighestRoll)
		{
			HighestRoll = Pair.Value;
			Winner = Pair.Key;
		}
	}

	if (Winner.IsEmpty())
	{
		for (const auto& Pair : RollData->GreedRolls)
		{
			if (Pair.Value > HighestRoll)
			{
				HighestRoll = Pair.Value;
				Winner = Pair.Key;
			}
		}
	}

	return Winner;
}

bool UPartyLibrary::DistributeLoot(
	const FString& PartyID,
	const FString& LootMasterID,
	const FString& ItemID,
	const FString& RecipientID)
{
	const FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	// Check if loot master
	bool bIsLootMaster = false;
	for (const FPartyMember& Member : Party->Members)
	{
		if (Member.PlayerID == LootMasterID && (Member.bIsLootMaster || Member.bIsLeader))
		{
			bIsLootMaster = true;
			break;
		}
	}

	if (!bIsLootMaster) return false;

	// Loot distribution happens here (integration with inventory system)
	// For now, just return success
	return true;
}

// ============================================
// XP & REWARDS
// ============================================

FXPShareResult UPartyLibrary::CalculateXPShare(const FString& PartyID, int32 BaseXP, int32 EnemyLevel)
{
	FXPShareResult Result;
	Result.TotalXP = BaseXP;

	const FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party || Party->Members.Num() == 0) return Result;

	// Calculate synergy bonus
	float SynergyBonus = GetPartySynergyBonus(PartyID);
	Result.SynergyBonus = FMath::RoundToInt(BaseXP * SynergyBonus);
	Result.TotalXP += Result.SynergyBonus;

	// Calculate level range
	int32 MinLevel = INT32_MAX;
	int32 MaxLevel = 0;
	for (const FPartyMember& Member : Party->Members)
	{
		MinLevel = FMath::Min(MinLevel, Member.Level);
		MaxLevel = FMath::Max(MaxLevel, Member.Level);
	}

	int32 XPPerMember = Result.TotalXP / Party->Members.Num();

	for (const FPartyMember& Member : Party->Members)
	{
		int32 MemberXP = XPPerMember;

		// Level difference penalty
		int32 LevelDiff = FMath::Abs(Member.Level - EnemyLevel);
		if (LevelDiff > Party->Settings.MaxLevelDifferenceForXP)
		{
			float Penalty = 1.0f - (0.1f * (LevelDiff - Party->Settings.MaxLevelDifferenceForXP));
			Penalty = FMath::Max(0.1f, Penalty);
			MemberXP = FMath::RoundToInt(MemberXP * Penalty);
			Result.ReducedXPPlayers.Add(Member.PlayerID);
		}

		Result.MemberXP.Add(Member.PlayerID, MemberXP);
	}

	return Result;
}

float UPartyLibrary::GetPartySynergyBonus(const FString& PartyID)
{
	const FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return 0.0f;

	bool bHasTank = false;
	bool bHasHealer = false;
	bool bHasDPS = false;
	bool bHasSupport = false;

	for (const FPartyMember& Member : Party->Members)
	{
		switch (Member.Role)
		{
		case EPartyRole::Tank: bHasTank = true; break;
		case EPartyRole::Healer: bHasHealer = true; break;
		case EPartyRole::DamageDealer: bHasDPS = true; break;
		case EPartyRole::Support: bHasSupport = true; break;
		default: break;
		}
	}

	float Bonus = 0.0f;
	if (bHasTank) Bonus += 0.05f;
	if (bHasHealer) Bonus += 0.05f;
	if (bHasDPS) Bonus += 0.03f;
	if (bHasSupport) Bonus += 0.02f;

	// Full synergy bonus for having all roles
	if (bHasTank && bHasHealer && bHasDPS)
	{
		Bonus += 0.10f;
	}

	return Bonus;
}

// ============================================
// DISPLAY HELPERS
// ============================================

FText UPartyLibrary::GetRoleDisplayName(EPartyRole Role)
{
	switch (Role)
	{
	case EPartyRole::None: return FText::FromString(TEXT("Keine Rolle"));
	case EPartyRole::Tank: return FText::FromString(TEXT("Tank"));
	case EPartyRole::Healer: return FText::FromString(TEXT("Heiler"));
	case EPartyRole::DamageDealer: return FText::FromString(TEXT("Schadensverursacher"));
	case EPartyRole::Support: return FText::FromString(TEXT("Unterstützer"));
	case EPartyRole::Leader: return FText::FromString(TEXT("Anführer"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FString UPartyLibrary::GetRoleIconPath(EPartyRole Role)
{
	switch (Role)
	{
	case EPartyRole::Tank: return TEXT("/Game/UI/Icons/Roles/tank");
	case EPartyRole::Healer: return TEXT("/Game/UI/Icons/Roles/healer");
	case EPartyRole::DamageDealer: return TEXT("/Game/UI/Icons/Roles/dps");
	case EPartyRole::Support: return TEXT("/Game/UI/Icons/Roles/support");
	case EPartyRole::Leader: return TEXT("/Game/UI/Icons/Roles/leader");
	default: return TEXT("/Game/UI/Icons/Roles/none");
	}
}

FLinearColor UPartyLibrary::GetRoleColor(EPartyRole Role)
{
	switch (Role)
	{
	case EPartyRole::Tank: return FLinearColor(0.2f, 0.4f, 0.8f); // Blue
	case EPartyRole::Healer: return FLinearColor(0.2f, 0.8f, 0.2f); // Green
	case EPartyRole::DamageDealer: return FLinearColor(0.8f, 0.2f, 0.2f); // Red
	case EPartyRole::Support: return FLinearColor(0.8f, 0.6f, 0.2f); // Orange
	case EPartyRole::Leader: return FLinearColor(1.0f, 0.84f, 0.0f); // Gold
	default: return FLinearColor::Gray;
	}
}

FText UPartyLibrary::GetStatusDisplayName(EPartyMemberStatus Status)
{
	switch (Status)
	{
	case EPartyMemberStatus::Online: return FText::FromString(TEXT("Online"));
	case EPartyMemberStatus::Away: return FText::FromString(TEXT("Abwesend"));
	case EPartyMemberStatus::Busy: return FText::FromString(TEXT("Beschäftigt"));
	case EPartyMemberStatus::Offline: return FText::FromString(TEXT("Offline"));
	case EPartyMemberStatus::InCombat: return FText::FromString(TEXT("Im Kampf"));
	case EPartyMemberStatus::Dead: return FText::FromString(TEXT("Tot"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FText UPartyLibrary::GetPartyTypeDisplayName(EPartyType Type)
{
	switch (Type)
	{
	case EPartyType::Party: return FText::FromString(TEXT("Gruppe"));
	case EPartyType::Raid: return FText::FromString(TEXT("Schlachtzug"));
	case EPartyType::Guild: return FText::FromString(TEXT("Gilde"));
	case EPartyType::Battleground: return FText::FromString(TEXT("Schlachtfeld"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FText UPartyLibrary::GetLootModeDisplayName(ELootMode Mode)
{
	switch (Mode)
	{
	case ELootMode::FreeForAll: return FText::FromString(TEXT("Freie Beute"));
	case ELootMode::RoundRobin: return FText::FromString(TEXT("Reihum"));
	case ELootMode::MasterLoot: return FText::FromString(TEXT("Meisterbeute"));
	case ELootMode::NeedBeforeGreed: return FText::FromString(TEXT("Bedarf vor Gier"));
	case ELootMode::GroupLoot: return FText::FromString(TEXT("Gruppenbeute"));
	case ELootMode::PersonalLoot: return FText::FromString(TEXT("Persönliche Beute"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

// ============================================
// PRIVATE HELPERS
// ============================================

FString UPartyLibrary::GenerateUniqueID()
{
	return FGuid::NewGuid().ToString();
}

bool UPartyLibrary::CanModifyParty(const FString& PartyID, const FString& PlayerID)
{
	const FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (Party->LeaderID == PlayerID) return true;

	for (const FPartyMember& Member : Party->Members)
	{
		if (Member.PlayerID == PlayerID && Member.bIsAssistant)
		{
			return true;
		}
	}

	return false;
}

// Stub implementations for remaining functions
bool UPartyLibrary::ApplyPartyBuff(const FString& PartyID, const FString& SourcePlayerID, const FPartyBuff& Buff)
{
	if (!ActivePartyBuffs.Contains(PartyID))
	{
		ActivePartyBuffs.Add(PartyID, TArray<FPartyBuff>());
	}
	ActivePartyBuffs[PartyID].Add(Buff);
	return true;
}

bool UPartyLibrary::RemovePartyBuff(const FString& PartyID, const FString& BuffID)
{
	if (TArray<FPartyBuff>* Buffs = ActivePartyBuffs.Find(PartyID))
	{
		for (int32 i = Buffs->Num() - 1; i >= 0; i--)
		{
			if ((*Buffs)[i].BuffID == BuffID)
			{
				Buffs->RemoveAt(i);
				return true;
			}
		}
	}
	return false;
}

TArray<FPartyBuff> UPartyLibrary::GetActivePartyBuffs(const FString& PartyID)
{
	if (const TArray<FPartyBuff>* Buffs = ActivePartyBuffs.Find(PartyID))
	{
		return *Buffs;
	}
	return TArray<FPartyBuff>();
}

void UPartyLibrary::UpdatePartyBuffs(const FString& PartyID, float DeltaTime)
{
	if (TArray<FPartyBuff>* Buffs = ActivePartyBuffs.Find(PartyID))
	{
		for (int32 i = Buffs->Num() - 1; i >= 0; i--)
		{
			if ((*Buffs)[i].Duration > 0)
			{
				(*Buffs)[i].RemainingTime -= DeltaTime;
				if ((*Buffs)[i].RemainingTime <= 0)
				{
					Buffs->RemoveAt(i);
				}
			}
		}
	}
}

bool UPartyLibrary::SetTargetMark(const FString& PartyID, const FString& SetterID, int32 MarkIndex, const FString& TargetID)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (!CanModifyParty(PartyID, SetterID)) return false;

	Party->TargetMarks.Add(MarkIndex, TargetID);
	return true;
}

bool UPartyLibrary::ClearTargetMark(const FString& PartyID, const FString& SetterID, int32 MarkIndex)
{
	FPartyData* Party = ActiveParties.Find(PartyID);
	if (!Party) return false;

	if (!CanModifyParty(PartyID, SetterID)) return false;

	return Party->TargetMarks.Remove(MarkIndex) > 0;
}

TMap<int32, FString> UPartyLibrary::GetTargetMarks(const FString& PartyID)
{
	if (const FPartyData* Party = ActiveParties.Find(PartyID))
	{
		return Party->TargetMarks;
	}
	return TMap<int32, FString>();
}

bool UPartyLibrary::ListInGroupFinder(const FString& PartyID, const FGroupFinderEntry& Listing)
{
	FGroupFinderEntry Entry = Listing;
	Entry.PartyID = PartyID;
	Entry.ListedTime = FDateTime::Now();

	GroupFinderListings.Add(PartyID, Entry);
	return true;
}

bool UPartyLibrary::UnlistFromGroupFinder(const FString& PartyID)
{
	return GroupFinderListings.Remove(PartyID) > 0;
}

TArray<FGroupFinderEntry> UPartyLibrary::SearchGroupFinder(
	const FString& ActivityFilter,
	EPartyRole RoleFilter,
	int32 MinLevel,
	int32 MaxLevel)
{
	TArray<FGroupFinderEntry> Results;

	for (const auto& Pair : GroupFinderListings)
	{
		const FGroupFinderEntry& Entry = Pair.Value;

		// Filter by activity
		if (!ActivityFilter.IsEmpty() && !Entry.ActivityName.Contains(ActivityFilter))
		{
			continue;
		}

		// Filter by level
		if (Entry.AverageLevel < MinLevel || Entry.AverageLevel > MaxLevel)
		{
			continue;
		}

		// Filter by role
		if (RoleFilter != EPartyRole::None && !Entry.LookingForRoles.Contains(RoleFilter))
		{
			continue;
		}

		Results.Add(Entry);
	}

	return Results;
}

bool UPartyLibrary::ApplyToGroup(const FString& PartyID, const FPartyMember& Applicant)
{
	// Implementation would send application to party leader
	return true;
}

FString UPartyLibrary::SavePartyToJSON(const FString& PartyID)
{
	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();

	if (const FPartyData* Party = ActiveParties.Find(PartyID))
	{
		RootObject->SetStringField(TEXT("party_id"), Party->PartyID);
		RootObject->SetStringField(TEXT("leader_id"), Party->LeaderID);
		RootObject->SetNumberField(TEXT("type"), static_cast<int32>(Party->Type));
		RootObject->SetNumberField(TEXT("max_members"), Party->MaxMembers);
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject, Writer);

	return OutputString;
}

bool UPartyLibrary::LoadPartyFromJSON(const FString& JSONString, FString& OutPartyID)
{
	// Implementation for loading party state
	return false;
}
