# Party Commands - Python Handler for PartyLibrary
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# PARTY MANAGEMENT
# ============================================

def handle_create_party(command: Dict[str, Any]) -> Dict[str, Any]:
    """Create a new party"""
    try:
        leader_id = command.get("leader_id", "")
        leader_name = command.get("leader_name", "Player")
        party_type = command.get("type", "PARTY")

        if not leader_id:
            return {"success": False, "error": "leader_id required"}

        type_enum = getattr(unreal.EPartyType, party_type.upper(), unreal.EPartyType.PARTY)

        party = unreal.PartyLibrary.create_party(leader_id, leader_name, type_enum)

        return {
            "success": True,
            "party_id": party.party_id,
            "leader_id": party.leader_id,
            "type": str(party.party_type)
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_disband_party(command: Dict[str, Any]) -> Dict[str, Any]:
    """Disband a party"""
    try:
        party_id = command.get("party_id", "")
        player_id = command.get("player_id", "")

        if not party_id or not player_id:
            return {"success": False, "error": "party_id and player_id required"}

        disbanded = unreal.PartyLibrary.disband_party(party_id, player_id)

        return {"success": disbanded}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_party(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get party by ID"""
    try:
        party_id = command.get("party_id", "")

        if not party_id:
            return {"success": False, "error": "party_id required"}

        found, party = unreal.PartyLibrary.get_party(party_id)

        if not found:
            return {"success": False, "error": f"Party not found: {party_id}"}

        members = []
        for m in party.members:
            members.append({
                "player_id": m.player_id,
                "name": m.name,
                "role": str(m.role),
                "status": str(m.status),
                "level": m.level,
                "health_percent": m.health_percent
            })

        return {
            "success": True,
            "party": {
                "party_id": party.party_id,
                "leader_id": party.leader_id,
                "type": str(party.party_type),
                "member_count": len(members),
                "members": members
            }
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_player_party(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get player's current party"""
    try:
        player_id = command.get("player_id", "")

        if not player_id:
            return {"success": False, "error": "player_id required"}

        found, party = unreal.PartyLibrary.get_player_party(player_id)

        if not found:
            return {"success": True, "in_party": False, "party": None}

        return {
            "success": True,
            "in_party": True,
            "party_id": party.party_id,
            "is_leader": party.leader_id == player_id
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_is_in_party(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if player is in a party"""
    try:
        player_id = command.get("player_id", "")

        if not player_id:
            return {"success": False, "error": "player_id required"}

        in_party = unreal.PartyLibrary.is_in_party(player_id)

        return {"success": True, "in_party": in_party}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_is_party_leader(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if player is party leader"""
    try:
        player_id = command.get("player_id", "")

        if not player_id:
            return {"success": False, "error": "player_id required"}

        is_leader = unreal.PartyLibrary.is_party_leader(player_id)

        return {"success": True, "is_leader": is_leader}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_convert_to_raid(command: Dict[str, Any]) -> Dict[str, Any]:
    """Convert party to raid"""
    try:
        party_id = command.get("party_id", "")
        player_id = command.get("player_id", "")

        if not party_id or not player_id:
            return {"success": False, "error": "party_id and player_id required"}

        converted = unreal.PartyLibrary.convert_to_raid(party_id, player_id)

        return {"success": converted}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# MEMBER MANAGEMENT
# ============================================

def handle_invite_player(command: Dict[str, Any]) -> Dict[str, Any]:
    """Invite player to party"""
    try:
        party_id = command.get("party_id", "")
        inviter_id = command.get("inviter_id", "")
        invitee_id = command.get("invitee_id", "")
        invitee_name = command.get("invitee_name", "Player")

        if not all([party_id, inviter_id, invitee_id]):
            return {"success": False, "error": "party_id, inviter_id, invitee_id required"}

        invitation = unreal.PartyLibrary.invite_player(
            party_id, inviter_id, invitee_id, invitee_name
        )

        return {
            "success": True,
            "invitation_id": invitation.invitation_id,
            "expires_at": invitation.expires_at
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_accept_invitation(command: Dict[str, Any]) -> Dict[str, Any]:
    """Accept party invitation"""
    try:
        invitation_id = command.get("invitation_id", "")
        player_id = command.get("player_id", "")
        player_name = command.get("player_name", "Player")
        player_class = command.get("player_class", "Warrior")
        level = command.get("level", 1)

        if not invitation_id or not player_id:
            return {"success": False, "error": "invitation_id and player_id required"}

        member = unreal.PartyMember()
        member.player_id = player_id
        member.name = player_name
        member.player_class = player_class
        member.level = level
        member.role = unreal.EPartyRole.DAMAGE
        member.status = unreal.EPartyMemberStatus.ONLINE

        accepted = unreal.PartyLibrary.accept_invitation(invitation_id, member)

        return {"success": accepted}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_decline_invitation(command: Dict[str, Any]) -> Dict[str, Any]:
    """Decline party invitation"""
    try:
        invitation_id = command.get("invitation_id", "")

        if not invitation_id:
            return {"success": False, "error": "invitation_id required"}

        declined = unreal.PartyLibrary.decline_invitation(invitation_id)

        return {"success": declined}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_leave_party(command: Dict[str, Any]) -> Dict[str, Any]:
    """Leave party"""
    try:
        player_id = command.get("player_id", "")

        if not player_id:
            return {"success": False, "error": "player_id required"}

        left = unreal.PartyLibrary.leave_party(player_id)

        return {"success": left}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_kick_member(command: Dict[str, Any]) -> Dict[str, Any]:
    """Kick member from party"""
    try:
        party_id = command.get("party_id", "")
        kicker_id = command.get("kicker_id", "")
        target_id = command.get("target_id", "")

        if not all([party_id, kicker_id, target_id]):
            return {"success": False, "error": "party_id, kicker_id, target_id required"}

        kicked = unreal.PartyLibrary.kick_member(party_id, kicker_id, target_id)

        return {"success": kicked}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_all_members(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all party members"""
    try:
        party_id = command.get("party_id", "")

        if not party_id:
            return {"success": False, "error": "party_id required"}

        members = unreal.PartyLibrary.get_all_members(party_id)

        member_list = []
        for m in members:
            member_list.append({
                "player_id": m.player_id,
                "name": m.name,
                "role": str(m.role),
                "status": str(m.status),
                "level": m.level,
                "player_class": m.player_class
            })

        return {"success": True, "members": member_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# ROLES & LEADERSHIP
# ============================================

def handle_set_member_role(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set member role"""
    try:
        party_id = command.get("party_id", "")
        setter_id = command.get("setter_id", "")
        target_id = command.get("target_id", "")
        role = command.get("role", "DAMAGE")

        if not all([party_id, setter_id, target_id]):
            return {"success": False, "error": "party_id, setter_id, target_id required"}

        role_enum = getattr(unreal.EPartyRole, role.upper(), unreal.EPartyRole.DAMAGE)

        set_role = unreal.PartyLibrary.set_member_role(party_id, setter_id, target_id, role_enum)

        return {"success": set_role}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_transfer_leadership(command: Dict[str, Any]) -> Dict[str, Any]:
    """Transfer party leadership"""
    try:
        party_id = command.get("party_id", "")
        current_leader_id = command.get("current_leader_id", "")
        new_leader_id = command.get("new_leader_id", "")

        if not all([party_id, current_leader_id, new_leader_id]):
            return {"success": False, "error": "party_id, current_leader_id, new_leader_id required"}

        transferred = unreal.PartyLibrary.transfer_leadership(
            party_id, current_leader_id, new_leader_id
        )

        return {"success": transferred}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_set_loot_master(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set loot master"""
    try:
        party_id = command.get("party_id", "")
        leader_id = command.get("leader_id", "")
        loot_master_id = command.get("loot_master_id", "")

        if not all([party_id, leader_id, loot_master_id]):
            return {"success": False, "error": "party_id, leader_id, loot_master_id required"}

        success = unreal.PartyLibrary.set_loot_master(party_id, leader_id, loot_master_id)

        return {"success": success}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# PARTY SETTINGS
# ============================================

def handle_set_loot_mode(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set loot mode"""
    try:
        party_id = command.get("party_id", "")
        leader_id = command.get("leader_id", "")
        mode = command.get("mode", "GROUP_LOOT")

        if not party_id or not leader_id:
            return {"success": False, "error": "party_id and leader_id required"}

        mode_enum = getattr(unreal.ELootMode, mode.upper(), unreal.ELootMode.GROUP_LOOT)

        success = unreal.PartyLibrary.set_loot_mode(party_id, leader_id, mode_enum)

        return {"success": success}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_party_settings(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get party settings"""
    try:
        party_id = command.get("party_id", "")

        if not party_id:
            return {"success": False, "error": "party_id required"}

        settings = unreal.PartyLibrary.get_party_settings(party_id)

        return {
            "success": True,
            "settings": {
                "loot_mode": str(settings.loot_mode),
                "loot_threshold": settings.loot_threshold,
                "allow_invites": settings.b_allow_member_invites
            }
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# READY CHECK
# ============================================

def handle_start_ready_check(command: Dict[str, Any]) -> Dict[str, Any]:
    """Start ready check"""
    try:
        party_id = command.get("party_id", "")
        initiator_id = command.get("initiator_id", "")
        duration = command.get("duration", 30.0)

        if not party_id or not initiator_id:
            return {"success": False, "error": "party_id and initiator_id required"}

        started = unreal.PartyLibrary.start_ready_check(party_id, initiator_id, duration)

        return {"success": started}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_respond_ready_check(command: Dict[str, Any]) -> Dict[str, Any]:
    """Respond to ready check"""
    try:
        party_id = command.get("party_id", "")
        player_id = command.get("player_id", "")
        is_ready = command.get("is_ready", True)

        if not party_id or not player_id:
            return {"success": False, "error": "party_id and player_id required"}

        responded = unreal.PartyLibrary.respond_to_ready_check(party_id, player_id, is_ready)

        return {"success": responded}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_ready_check_status(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get ready check status"""
    try:
        party_id = command.get("party_id", "")

        if not party_id:
            return {"success": False, "error": "party_id required"}

        status = unreal.PartyLibrary.get_ready_check_status(party_id)

        responses = []
        for player_id, ready in status.responses.items():
            responses.append({"player_id": player_id, "ready": ready})

        return {
            "success": True,
            "active": status.b_is_active,
            "time_remaining": status.time_remaining,
            "responses": responses
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# LOOT MANAGEMENT
# ============================================

def handle_start_loot_roll(command: Dict[str, Any]) -> Dict[str, Any]:
    """Start loot roll"""
    try:
        party_id = command.get("party_id", "")
        item_id = command.get("item_id", "")
        item_name = command.get("item_name", "Item")
        item_quality = command.get("item_quality", 2)
        duration = command.get("duration", 30.0)

        if not party_id or not item_id:
            return {"success": False, "error": "party_id and item_id required"}

        loot_roll = unreal.PartyLibrary.start_loot_roll(
            party_id, item_id, item_name, item_quality, duration
        )

        return {
            "success": True,
            "roll_id": loot_roll.roll_id,
            "item_id": loot_roll.item_id,
            "time_remaining": loot_roll.time_remaining
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_roll_for_loot(command: Dict[str, Any]) -> Dict[str, Any]:
    """Roll for loot (Need/Greed)"""
    try:
        party_id = command.get("party_id", "")
        item_id = command.get("item_id", "")
        player_id = command.get("player_id", "")
        need = command.get("need", False)

        if not all([party_id, item_id, player_id]):
            return {"success": False, "error": "party_id, item_id, player_id required"}

        rolled = unreal.PartyLibrary.roll_for_loot(party_id, item_id, player_id, need)

        return {"success": rolled}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_pass_on_loot(command: Dict[str, Any]) -> Dict[str, Any]:
    """Pass on loot"""
    try:
        party_id = command.get("party_id", "")
        item_id = command.get("item_id", "")
        player_id = command.get("player_id", "")

        if not all([party_id, item_id, player_id]):
            return {"success": False, "error": "party_id, item_id, player_id required"}

        passed = unreal.PartyLibrary.pass_on_loot(party_id, item_id, player_id)

        return {"success": passed}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_loot_winner(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get loot roll winner"""
    try:
        party_id = command.get("party_id", "")
        item_id = command.get("item_id", "")

        if not party_id or not item_id:
            return {"success": False, "error": "party_id and item_id required"}

        winner_id = unreal.PartyLibrary.get_loot_roll_winner(party_id, item_id)

        return {"success": True, "winner_id": winner_id}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# XP & REWARDS
# ============================================

def handle_calculate_xp_share(command: Dict[str, Any]) -> Dict[str, Any]:
    """Calculate XP share for party"""
    try:
        party_id = command.get("party_id", "")
        base_xp = command.get("base_xp", 100)
        enemy_level = command.get("enemy_level", 1)

        if not party_id:
            return {"success": False, "error": "party_id required"}

        result = unreal.PartyLibrary.calculate_xp_share(party_id, base_xp, enemy_level)

        shares = []
        for player_id, xp in result.xp_shares.items():
            shares.append({"player_id": player_id, "xp": xp})

        return {
            "success": True,
            "total_xp": result.total_xp,
            "bonus_xp": result.bonus_xp,
            "shares": shares
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_synergy_bonus(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get party synergy bonus"""
    try:
        party_id = command.get("party_id", "")

        if not party_id:
            return {"success": False, "error": "party_id required"}

        bonus = unreal.PartyLibrary.get_party_synergy_bonus(party_id)

        return {"success": True, "synergy_bonus": bonus}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# PARTY BUFFS
# ============================================

def handle_apply_party_buff(command: Dict[str, Any]) -> Dict[str, Any]:
    """Apply party buff"""
    try:
        party_id = command.get("party_id", "")
        source_id = command.get("source_id", "")
        buff_id = command.get("buff_id", "")
        buff_name = command.get("buff_name", "Buff")
        duration = command.get("duration", 60.0)
        effect_value = command.get("effect_value", 10.0)

        if not all([party_id, source_id, buff_id]):
            return {"success": False, "error": "party_id, source_id, buff_id required"}

        buff = unreal.PartyBuff()
        buff.buff_id = buff_id
        buff.buff_name = buff_name
        buff.duration = duration
        buff.remaining_duration = duration
        buff.effect_value = effect_value
        buff.source_player_id = source_id

        applied = unreal.PartyLibrary.apply_party_buff(party_id, source_id, buff)

        return {"success": applied}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_active_buffs(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get active party buffs"""
    try:
        party_id = command.get("party_id", "")

        if not party_id:
            return {"success": False, "error": "party_id required"}

        buffs = unreal.PartyLibrary.get_active_party_buffs(party_id)

        buff_list = []
        for b in buffs:
            buff_list.append({
                "buff_id": b.buff_id,
                "name": b.buff_name,
                "remaining": b.remaining_duration,
                "effect_value": b.effect_value
            })

        return {"success": True, "buffs": buff_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# TARGET MARKERS
# ============================================

def handle_set_target_mark(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set target mark"""
    try:
        party_id = command.get("party_id", "")
        setter_id = command.get("setter_id", "")
        mark_index = command.get("mark_index", 0)
        target_id = command.get("target_id", "")

        if not all([party_id, setter_id, target_id]):
            return {"success": False, "error": "party_id, setter_id, target_id required"}

        success = unreal.PartyLibrary.set_target_mark(party_id, setter_id, mark_index, target_id)

        return {"success": success}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_target_marks(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all target marks"""
    try:
        party_id = command.get("party_id", "")

        if not party_id:
            return {"success": False, "error": "party_id required"}

        marks = unreal.PartyLibrary.get_target_marks(party_id)

        mark_list = []
        for index, target_id in marks.items():
            mark_list.append({"index": index, "target_id": target_id})

        return {"success": True, "marks": mark_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# DISPLAY HELPERS
# ============================================

def handle_get_role_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get role display info"""
    try:
        role = command.get("role", "DAMAGE")

        role_enum = getattr(unreal.EPartyRole, role.upper(), unreal.EPartyRole.DAMAGE)

        name = unreal.PartyLibrary.get_role_display_name(role_enum)
        icon = unreal.PartyLibrary.get_role_icon_path(role_enum)
        color = unreal.PartyLibrary.get_role_color(role_enum)

        return {
            "success": True,
            "name": str(name),
            "icon": icon,
            "color": {"r": color.r, "g": color.g, "b": color.b}
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SAVE/LOAD
# ============================================

def handle_save_party(command: Dict[str, Any]) -> Dict[str, Any]:
    """Save party to JSON"""
    try:
        party_id = command.get("party_id", "")

        if not party_id:
            return {"success": False, "error": "party_id required"}

        json_str = unreal.PartyLibrary.save_party_to_json(party_id)

        return {"success": True, "json": json_str}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_load_party(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load party from JSON"""
    try:
        json_str = command.get("json", "")

        if not json_str:
            return {"success": False, "error": "json required"}

        loaded, party_id = unreal.PartyLibrary.load_party_from_json(json_str)

        return {"success": loaded, "party_id": party_id}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

PARTY_COMMANDS = {
    # Management
    "party_create": handle_create_party,
    "party_disband": handle_disband_party,
    "party_get": handle_get_party,
    "party_get_player": handle_get_player_party,
    "party_is_in": handle_is_in_party,
    "party_is_leader": handle_is_party_leader,
    "party_convert_raid": handle_convert_to_raid,

    # Members
    "party_invite": handle_invite_player,
    "party_accept": handle_accept_invitation,
    "party_decline": handle_decline_invitation,
    "party_leave": handle_leave_party,
    "party_kick": handle_kick_member,
    "party_get_members": handle_get_all_members,

    # Roles
    "party_set_role": handle_set_member_role,
    "party_transfer_leader": handle_transfer_leadership,
    "party_set_loot_master": handle_set_loot_master,

    # Settings
    "party_set_loot_mode": handle_set_loot_mode,
    "party_get_settings": handle_get_party_settings,

    # Ready Check
    "party_ready_check": handle_start_ready_check,
    "party_ready_respond": handle_respond_ready_check,
    "party_ready_status": handle_get_ready_check_status,

    # Loot
    "party_loot_roll": handle_start_loot_roll,
    "party_roll": handle_roll_for_loot,
    "party_pass": handle_pass_on_loot,
    "party_loot_winner": handle_get_loot_winner,

    # Rewards
    "party_xp_share": handle_calculate_xp_share,
    "party_synergy": handle_get_synergy_bonus,

    # Buffs
    "party_apply_buff": handle_apply_party_buff,
    "party_get_buffs": handle_get_active_buffs,

    # Markers
    "party_set_mark": handle_set_target_mark,
    "party_get_marks": handle_get_target_marks,

    # Display
    "party_role_info": handle_get_role_info,

    # Save/Load
    "party_save": handle_save_party,
    "party_load": handle_load_party,
}
