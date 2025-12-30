# Combat Commands - Python Handler for CombatLibrary
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# DAMAGE CALCULATION
# ============================================

def handle_calculate_damage(command: Dict[str, Any]) -> Dict[str, Any]:
    """Calculate damage from attack"""
    try:
        # Build damage input
        damage_input = unreal.DamageInput()
        damage_input.base_damage = command.get("base_damage", 100.0)
        damage_input.damage_type = getattr(unreal.EDamageType, command.get("damage_type", "PHYSICAL").upper(), unreal.EDamageType.PHYSICAL)
        damage_input.attack_power = command.get("attack_power", 100.0)
        damage_input.crit_chance = command.get("crit_chance", 0.1)
        damage_input.crit_damage = command.get("crit_damage", 1.5)
        damage_input.armor_penetration = command.get("armor_penetration", 0.0)
        damage_input.attacker_level = command.get("attacker_level", 1)

        # Build defender stats
        defender_stats = unreal.CombatStats()
        defender_stats.armor = command.get("defender_armor", 50.0)
        defender_stats.magic_defense = command.get("defender_magic_defense", 50.0)
        defender_stats.dodge_chance = command.get("defender_dodge_chance", 0.05)
        defender_stats.block_chance = command.get("defender_block_chance", 0.0)

        defender_hp = command.get("defender_hp", 1000.0)
        defender_max_hp = command.get("defender_max_hp", 1000.0)

        result = unreal.CombatLibrary.calculate_damage(damage_input, defender_stats, defender_hp, defender_max_hp)

        return {
            "success": True,
            "final_damage": result.final_damage,
            "raw_damage": result.raw_damage,
            "mitigated_damage": result.mitigated_damage,
            "is_crit": result.is_crit,
            "combat_result": str(result.combat_result),
            "overkill": result.overkill
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_calculate_physical_damage(command: Dict[str, Any]) -> Dict[str, Any]:
    """Calculate physical damage"""
    try:
        base_damage = command.get("base_damage", 100.0)
        attack_power = command.get("attack_power", 100.0)
        target_armor = command.get("target_armor", 50.0)
        armor_pen = command.get("armor_penetration", 0.0)
        skill_coef = command.get("skill_coefficient", 1.0)

        damage = unreal.CombatLibrary.calculate_physical_damage(
            base_damage, attack_power, target_armor, armor_pen, skill_coef
        )

        return {"success": True, "damage": damage}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_calculate_magical_damage(command: Dict[str, Any]) -> Dict[str, Any]:
    """Calculate magical damage"""
    try:
        base_damage = command.get("base_damage", 100.0)
        spell_power = command.get("spell_power", 100.0)
        magic_defense = command.get("magic_defense", 50.0)
        magic_pen = command.get("magic_penetration", 0.0)
        spell_coef = command.get("spell_coefficient", 1.0)

        damage = unreal.CombatLibrary.calculate_magical_damage(
            base_damage, spell_power, magic_defense, magic_pen, spell_coef
        )

        return {"success": True, "damage": damage}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# HEALING
# ============================================

def handle_calculate_healing(command: Dict[str, Any]) -> Dict[str, Any]:
    """Calculate healing amount"""
    try:
        healing_input = unreal.HealingInput()
        healing_input.base_healing = command.get("base_healing", 100.0)
        healing_input.spell_power = command.get("spell_power", 100.0)
        healing_input.healing_power_bonus = command.get("healing_bonus", 0.0)
        healing_input.crit_chance = command.get("crit_chance", 0.1)
        healing_input.crit_bonus = command.get("crit_bonus", 0.5)

        target_hp = command.get("target_hp", 500.0)
        target_max_hp = command.get("target_max_hp", 1000.0)
        healing_received = command.get("healing_received_bonus", 0.0)

        result = unreal.CombatLibrary.calculate_healing(
            healing_input, target_hp, target_max_hp, healing_received
        )

        return {
            "success": True,
            "final_healing": result.final_healing,
            "effective_healing": result.effective_healing,
            "overheal": result.overheal,
            "is_crit": result.is_crit
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# STATUS EFFECTS
# ============================================

def handle_apply_status_effect(command: Dict[str, Any]) -> Dict[str, Any]:
    """Apply status effect to target"""
    try:
        target_id = command.get("target_id", "")
        effect_type = command.get("effect_type", "BURNING")
        duration = command.get("duration", 10.0)
        power = command.get("power", 50.0)
        stacks = command.get("stacks", 1)

        if not target_id:
            return {"success": False, "error": "target_id required"}

        effect = unreal.StatusEffect()
        effect.effect_type = getattr(unreal.EStatusEffectType, effect_type.upper(), unreal.EStatusEffectType.BURNING)
        effect.duration = duration
        effect.power = power
        effect.stacks = stacks

        success = unreal.CombatLibrary.apply_status_effect(target_id, effect)

        return {"success": success, "target_id": target_id, "effect_type": effect_type}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_remove_status_effect(command: Dict[str, Any]) -> Dict[str, Any]:
    """Remove status effect from target"""
    try:
        target_id = command.get("target_id", "")
        effect_type = command.get("effect_type", "")

        if not target_id or not effect_type:
            return {"success": False, "error": "target_id and effect_type required"}

        effect_enum = getattr(unreal.EStatusEffectType, effect_type.upper(), None)
        if effect_enum is None:
            return {"success": False, "error": f"Unknown effect type: {effect_type}"}

        success = unreal.CombatLibrary.remove_status_effect(target_id, effect_enum)

        return {"success": success}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_active_effects(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get active status effects on target"""
    try:
        target_id = command.get("target_id", "")

        if not target_id:
            return {"success": False, "error": "target_id required"}

        effects = unreal.CombatLibrary.get_active_effects(target_id)

        effect_list = []
        for e in effects:
            effect_list.append({
                "type": str(e.effect_type),
                "duration": e.remaining_duration,
                "power": e.power,
                "stacks": e.stacks
            })

        return {"success": True, "effects": effect_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_has_status_effect(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if target has specific status effect"""
    try:
        target_id = command.get("target_id", "")
        effect_type = command.get("effect_type", "")

        if not target_id or not effect_type:
            return {"success": False, "error": "target_id and effect_type required"}

        effect_enum = getattr(unreal.EStatusEffectType, effect_type.upper(), None)
        if effect_enum is None:
            return {"success": False, "error": f"Unknown effect type: {effect_type}"}

        has_effect = unreal.CombatLibrary.has_status_effect(target_id, effect_enum)

        return {"success": True, "has_effect": has_effect}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# THREAT & AGGRO
# ============================================

def handle_add_threat(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add threat to NPC's aggro table"""
    try:
        npc_id = command.get("npc_id", "")
        player_id = command.get("player_id", "")
        player_name = command.get("player_name", "Player")
        threat_amount = command.get("threat_amount", 100.0)

        if not npc_id or not player_id:
            return {"success": False, "error": "npc_id and player_id required"}

        unreal.CombatLibrary.add_threat(npc_id, player_id, player_name, threat_amount)

        return {"success": True, "npc_id": npc_id, "threat_added": threat_amount}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_aggro_table(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get NPC's aggro table"""
    try:
        npc_id = command.get("npc_id", "")

        if not npc_id:
            return {"success": False, "error": "npc_id required"}

        aggro = unreal.CombatLibrary.get_aggro_table(npc_id)
        current_target = unreal.CombatLibrary.get_current_target(npc_id)

        entries = []
        for entry in aggro.entries:
            entries.append({
                "player_id": entry.player_id,
                "player_name": entry.player_name,
                "threat": entry.threat
            })

        return {
            "success": True,
            "npc_id": npc_id,
            "current_target": current_target,
            "aggro_table": entries
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_clear_aggro(command: Dict[str, Any]) -> Dict[str, Any]:
    """Clear NPC's aggro table"""
    try:
        npc_id = command.get("npc_id", "")

        if not npc_id:
            return {"success": False, "error": "npc_id required"}

        unreal.CombatLibrary.clear_aggro(npc_id)

        return {"success": True, "npc_id": npc_id}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMBAT LOG
# ============================================

def handle_get_combat_log(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get combat log entries"""
    try:
        max_entries = command.get("max_entries", 100)

        log_entries = unreal.CombatLibrary.get_combat_log(max_entries)

        entries = []
        for entry in log_entries:
            entries.append({
                "timestamp": entry.timestamp,
                "source_name": entry.source_name,
                "target_name": entry.target_name,
                "ability": entry.ability_name,
                "damage": entry.damage,
                "is_crit": entry.is_crit
            })

        return {"success": True, "entries": entries}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_dps_summary(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get DPS summary from combat log"""
    try:
        time_window = command.get("time_window", 5.0)

        dps_map = unreal.CombatLibrary.get_dps_summary(time_window)

        dps_list = []
        for player_id, dps in dps_map.items():
            dps_list.append({"player_id": player_id, "dps": dps})

        return {"success": True, "time_window": time_window, "dps": dps_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_clear_combat_log(command: Dict[str, Any]) -> Dict[str, Any]:
    """Clear combat log"""
    try:
        unreal.CombatLibrary.clear_combat_log()

        return {"success": True}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# STAT CALCULATIONS
# ============================================

def handle_calculate_stats(command: Dict[str, Any]) -> Dict[str, Any]:
    """Calculate derived stats from base stats"""
    try:
        strength = command.get("strength", 10.0)
        intelligence = command.get("intelligence", 10.0)
        constitution = command.get("constitution", 10.0)
        wisdom = command.get("wisdom", 10.0)
        dexterity = command.get("dexterity", 10.0)
        level = command.get("level", 1)

        attack_power = unreal.CombatLibrary.calculate_attack_power(strength, level)
        spell_power = unreal.CombatLibrary.calculate_spell_power(intelligence, level)
        max_health = unreal.CombatLibrary.calculate_max_health(constitution, level)
        max_mana = unreal.CombatLibrary.calculate_max_mana(wisdom, level)
        crit_chance = unreal.CombatLibrary.calculate_crit_chance(dexterity, level)
        dodge_chance = unreal.CombatLibrary.calculate_dodge_chance(dexterity, level)

        return {
            "success": True,
            "attack_power": attack_power,
            "spell_power": spell_power,
            "max_health": max_health,
            "max_mana": max_mana,
            "crit_chance": crit_chance,
            "dodge_chance": dodge_chance
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

COMBAT_COMMANDS = {
    # Damage
    "combat_calculate_damage": handle_calculate_damage,
    "combat_calculate_physical": handle_calculate_physical_damage,
    "combat_calculate_magical": handle_calculate_magical_damage,

    # Healing
    "combat_calculate_healing": handle_calculate_healing,

    # Status Effects
    "combat_apply_effect": handle_apply_status_effect,
    "combat_remove_effect": handle_remove_status_effect,
    "combat_get_effects": handle_get_active_effects,
    "combat_has_effect": handle_has_status_effect,

    # Threat/Aggro
    "combat_add_threat": handle_add_threat,
    "combat_get_aggro": handle_get_aggro_table,
    "combat_clear_aggro": handle_clear_aggro,

    # Combat Log
    "combat_get_log": handle_get_combat_log,
    "combat_get_dps": handle_get_dps_summary,
    "combat_clear_log": handle_clear_combat_log,

    # Stats
    "combat_calculate_stats": handle_calculate_stats,
}
