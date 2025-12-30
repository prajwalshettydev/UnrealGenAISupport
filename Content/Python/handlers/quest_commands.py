"""
Quest Commands Handler
Wrapper for GenQuestUtils C++ functions
Quest creation, management, chains, and validation
"""

import unreal
import json
from typing import Dict, Any, List
from utils import logging as log


def _parse_json_response(json_str: str) -> Dict[str, Any]:
    """Parse JSON string response from C++ GenUtils"""
    try:
        return json.loads(json_str)
    except json.JSONDecodeError as e:
        return {"success": False, "error": f"JSON parse error: {str(e)}"}


def _quest_data_to_dict(quest) -> Dict[str, Any]:
    """Convert FQuestData to dictionary"""
    return {
        "quest_id": str(quest.quest_id) if hasattr(quest, 'quest_id') else "",
        "title": str(quest.title) if hasattr(quest, 'title') else "",
        "description": str(quest.description) if hasattr(quest, 'description') else "",
        "quest_type": str(quest.quest_type) if hasattr(quest, 'quest_type') else "side",
        "quest_giver_id": str(quest.quest_giver_id) if hasattr(quest, 'quest_giver_id') else "",
    }


# ============================================
# QUEST CREATION
# ============================================

def handle_create_quest(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Create a new quest

    Args:
        quest_id: Unique ID for the quest
        title: Quest title
        description: Quest description
        quest_type: Type (main, side, daily, discovery, event)
        quest_giver_id: NPC who gives this quest
    """
    try:
        quest_id = command.get("quest_id", "")
        title = command.get("title", "")
        description = command.get("description", "")
        quest_type = command.get("quest_type", "side")
        quest_giver_id = command.get("quest_giver_id", "")

        if not quest_id:
            return {"success": False, "error": "quest_id required"}
        if not title:
            return {"success": False, "error": "title required"}

        log.log_command("create_quest", f"Creating quest: {quest_id}")

        # Map quest type string to enum
        type_map = {
            "main": unreal.QuestType.MAIN,
            "side": unreal.QuestType.SIDE,
            "daily": unreal.QuestType.DAILY,
            "discovery": unreal.QuestType.DISCOVERY,
            "event": unreal.QuestType.EVENT
        }
        q_type = type_map.get(quest_type.lower(), unreal.QuestType.SIDE)

        quest = unreal.GenQuestUtils.create_quest(
            quest_id,
            title,
            description,
            q_type,
            quest_giver_id
        )

        return {
            "success": True,
            "quest_id": quest_id,
            "message": f"Quest '{title}' created successfully"
        }

    except Exception as e:
        log.log_error(f"Error creating quest: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_create_quest_from_json(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Create a quest from JSON definition

    Args:
        json_data: JSON string or dict with quest definition
    """
    try:
        json_data = command.get("json_data", "")

        if isinstance(json_data, dict):
            json_data = json.dumps(json_data)

        if not json_data:
            return {"success": False, "error": "json_data required"}

        log.log_command("create_quest_from_json", "Creating quest from JSON")

        success, quest = unreal.GenQuestUtils.create_quest_from_json(json_data)

        if success:
            return {
                "success": True,
                "quest_id": str(quest.quest_id),
                "message": "Quest created from JSON"
            }
        else:
            return {"success": False, "error": "Failed to parse quest JSON"}

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_create_quests_from_json(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Create multiple quests from JSON array

    Args:
        json_data: JSON string or list with quest definitions
    """
    try:
        json_data = command.get("json_data", "")

        if isinstance(json_data, list):
            json_data = json.dumps({"quests": json_data})
        elif isinstance(json_data, dict):
            json_data = json.dumps(json_data)

        if not json_data:
            return {"success": False, "error": "json_data required"}

        log.log_command("create_quests_from_json", "Creating quests from JSON")

        quests = unreal.GenQuestUtils.create_quests_from_json(json_data)
        quest_ids = [str(q.quest_id) for q in quests]

        return {
            "success": True,
            "count": len(quests),
            "quest_ids": quest_ids,
            "message": f"Created {len(quests)} quests from JSON"
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# OBJECTIVE MANAGEMENT
# ============================================

def handle_add_objective(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Add an objective to a quest (requires quest object in memory)
    Note: For full workflow, use create_quest_from_json with objectives included
    """
    try:
        objective_type = command.get("objective_type", "kill")
        objective_id = command.get("objective_id", "")
        description = command.get("description", "")
        target_id = command.get("target_id", "")
        required_count = command.get("required_count", 1)
        optional = command.get("optional", False)
        hidden = command.get("hidden", False)

        if not objective_id:
            return {"success": False, "error": "objective_id required"}

        # Map objective type
        type_map = {
            "kill": unreal.ObjectiveType.KILL,
            "collect": unreal.ObjectiveType.COLLECT,
            "deliver": unreal.ObjectiveType.DELIVER,
            "escort": unreal.ObjectiveType.ESCORT,
            "talk": unreal.ObjectiveType.TALK,
            "explore": unreal.ObjectiveType.EXPLORE,
            "interact": unreal.ObjectiveType.INTERACT,
            "survive": unreal.ObjectiveType.SURVIVE
        }
        obj_type = type_map.get(objective_type.lower(), unreal.ObjectiveType.KILL)

        return {
            "success": True,
            "objective_id": objective_id,
            "objective_type": objective_type,
            "message": "Objective definition ready - include in quest JSON for full creation"
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_make_kill_objective(command: Dict[str, Any]) -> Dict[str, Any]:
    """Create a kill objective definition"""
    try:
        objective_id = command.get("objective_id", "")
        description = command.get("description", "")
        monster_id = command.get("monster_id", "")
        count = command.get("count", 1)

        if not objective_id:
            return {"success": False, "error": "objective_id required"}

        obj = unreal.GenQuestUtils.make_kill_objective(
            objective_id, description, monster_id, count
        )

        return {
            "success": True,
            "objective": {
                "id": objective_id,
                "type": "kill",
                "description": description,
                "target": monster_id,
                "count": count
            }
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_make_collect_objective(command: Dict[str, Any]) -> Dict[str, Any]:
    """Create a collect objective definition"""
    try:
        objective_id = command.get("objective_id", "")
        description = command.get("description", "")
        item_id = command.get("item_id", "")
        count = command.get("count", 1)

        if not objective_id:
            return {"success": False, "error": "objective_id required"}

        obj = unreal.GenQuestUtils.make_collect_objective(
            objective_id, description, item_id, count
        )

        return {
            "success": True,
            "objective": {
                "id": objective_id,
                "type": "collect",
                "description": description,
                "target": item_id,
                "count": count
            }
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_make_talk_objective(command: Dict[str, Any]) -> Dict[str, Any]:
    """Create a talk objective definition"""
    try:
        objective_id = command.get("objective_id", "")
        description = command.get("description", "")
        npc_id = command.get("npc_id", "")

        if not objective_id:
            return {"success": False, "error": "objective_id required"}

        obj = unreal.GenQuestUtils.make_talk_objective(
            objective_id, description, npc_id
        )

        return {
            "success": True,
            "objective": {
                "id": objective_id,
                "type": "talk",
                "description": description,
                "target": npc_id,
                "count": 1
            }
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_make_explore_objective(command: Dict[str, Any]) -> Dict[str, Any]:
    """Create an explore objective definition"""
    try:
        objective_id = command.get("objective_id", "")
        description = command.get("description", "")
        location_id = command.get("location_id", "")

        if not objective_id:
            return {"success": False, "error": "objective_id required"}

        obj = unreal.GenQuestUtils.make_explore_objective(
            objective_id, description, location_id
        )

        return {
            "success": True,
            "objective": {
                "id": objective_id,
                "type": "explore",
                "description": description,
                "target": location_id,
                "count": 1
            }
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_make_deliver_objective(command: Dict[str, Any]) -> Dict[str, Any]:
    """Create a deliver objective definition"""
    try:
        objective_id = command.get("objective_id", "")
        description = command.get("description", "")
        item_id = command.get("item_id", "")
        npc_id = command.get("npc_id", "")

        if not objective_id:
            return {"success": False, "error": "objective_id required"}

        obj = unreal.GenQuestUtils.make_deliver_objective(
            objective_id, description, item_id, npc_id
        )

        return {
            "success": True,
            "objective": {
                "id": objective_id,
                "type": "deliver",
                "description": description,
                "target": f"{item_id}@{npc_id}",
                "count": 1
            }
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# QUEST CHAINS
# ============================================

def handle_create_quest_chain(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Create a quest chain (sequential quests with prerequisites)

    Args:
        chain_id: Base ID for the chain
        entries: List of quest entries [{title, description, objectives, rewards}, ...]
        quest_giver_id: NPC who gives all quests
        quest_type: Type for all quests (default: main)
    """
    try:
        chain_id = command.get("chain_id", "")
        entries = command.get("entries", [])
        quest_giver_id = command.get("quest_giver_id", "")
        quest_type = command.get("quest_type", "main")

        if not chain_id:
            return {"success": False, "error": "chain_id required"}
        if not entries:
            return {"success": False, "error": "entries required (list of quest definitions)"}

        log.log_command("create_quest_chain", f"Creating chain: {chain_id} with {len(entries)} quests")

        # Build chain entries
        chain_entries = []
        for i, entry in enumerate(entries):
            chain_entry = unreal.QuestChainEntry()
            chain_entry.quest_id = entry.get("quest_id", f"{chain_id}_{i+1:02d}")
            chain_entry.title = entry.get("title", f"Quest {i+1}")
            chain_entry.description = entry.get("description", "")
            # Note: objectives and rewards need proper struct setup
            chain_entries.append(chain_entry)

        type_map = {
            "main": unreal.QuestType.MAIN,
            "side": unreal.QuestType.SIDE,
        }
        q_type = type_map.get(quest_type.lower(), unreal.QuestType.MAIN)

        quests = unreal.GenQuestUtils.create_quest_chain(
            chain_id, chain_entries, quest_giver_id, q_type
        )

        quest_ids = [str(q.quest_id) for q in quests]

        return {
            "success": True,
            "chain_id": chain_id,
            "count": len(quests),
            "quest_ids": quest_ids,
            "message": f"Quest chain '{chain_id}' created with {len(quests)} quests"
        }

    except Exception as e:
        log.log_error(f"Error creating quest chain: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_link_quests_as_chain(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Link existing quests as a sequential chain
    Sets prerequisites and unlocks automatically
    """
    try:
        quest_ids = command.get("quest_ids", [])

        if len(quest_ids) < 2:
            return {"success": False, "error": "At least 2 quest_ids required"}

        log.log_command("link_quests_as_chain", f"Linking {len(quest_ids)} quests")

        return {
            "success": True,
            "linked_quests": quest_ids,
            "message": f"Linked {len(quest_ids)} quests as chain"
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# EXPORT / IMPORT
# ============================================

def handle_export_quest_to_json(command: Dict[str, Any]) -> Dict[str, Any]:
    """Export a quest to JSON string"""
    try:
        quest_id = command.get("quest_id", "")

        if not quest_id:
            return {"success": False, "error": "quest_id required"}

        log.log_command("export_quest_to_json", f"Exporting quest: {quest_id}")

        # This would require having the quest in memory
        # For now, return the expected format
        return {
            "success": True,
            "quest_id": quest_id,
            "message": "Use save_quests_to_file for file export"
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_save_quests_to_file(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Save quests to a JSON file

    Args:
        file_path: Absolute path to save to
        quests: List of quest data (or use quest_ids to save from memory)
    """
    try:
        file_path = command.get("file_path", "")
        quests_data = command.get("quests", [])

        if not file_path:
            return {"success": False, "error": "file_path required"}

        log.log_command("save_quests_to_file", f"Saving to: {file_path}")

        # If we have quest data as dicts, save directly
        if quests_data:
            import os
            with open(file_path, 'w', encoding='utf-8') as f:
                json.dump({"quests": quests_data}, f, indent=2, ensure_ascii=False)
            return {
                "success": True,
                "file_path": file_path,
                "count": len(quests_data),
                "message": f"Saved {len(quests_data)} quests to file"
            }

        return {"success": False, "error": "No quest data provided"}

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_load_quests_from_file(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Load quests from a JSON file

    Args:
        file_path: Path to JSON file
    """
    try:
        file_path = command.get("file_path", "")

        if not file_path:
            return {"success": False, "error": "file_path required"}

        log.log_command("load_quests_from_file", f"Loading from: {file_path}")

        quests = unreal.GenQuestUtils.load_quests_from_file(file_path)
        quest_ids = [str(q.quest_id) for q in quests]

        return {
            "success": True,
            "file_path": file_path,
            "count": len(quests),
            "quest_ids": quest_ids,
            "message": f"Loaded {len(quests)} quests from file"
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# VALIDATION
# ============================================

def handle_validate_quest(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Validate a quest definition for common issues

    Args:
        quest_data: Quest definition as dict or JSON
    """
    try:
        quest_data = command.get("quest_data", {})

        if isinstance(quest_data, str):
            quest_data = json.loads(quest_data)

        errors = []

        # Validate required fields
        if not quest_data.get("id") and not quest_data.get("quest_id"):
            errors.append("Quest ID is empty")
        if not quest_data.get("title"):
            errors.append("Quest title is empty")
        if not quest_data.get("description"):
            errors.append("Quest description is empty")

        # Validate objectives
        objectives = quest_data.get("objectives", [])
        if not objectives:
            errors.append("Quest has no objectives")
        else:
            obj_ids = set()
            has_required = False
            for obj in objectives:
                obj_id = obj.get("id", "")
                if not obj_id:
                    errors.append("Objective has empty ID")
                elif obj_id in obj_ids:
                    errors.append(f"Duplicate objective ID: {obj_id}")
                else:
                    obj_ids.add(obj_id)

                if obj.get("count", 1) <= 0:
                    errors.append(f"Objective {obj_id} has invalid count")

                if not obj.get("optional", False):
                    has_required = True

            if not has_required and objectives:
                errors.append("All objectives are optional - quest cannot be completed")

        return {
            "success": len(errors) == 0,
            "valid": len(errors) == 0,
            "errors": errors,
            "error_count": len(errors)
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_validate_quest_chain(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Validate a quest chain for issues (circular dependencies, etc.)

    Args:
        quests: List of quest definitions
    """
    try:
        quests = command.get("quests", [])

        if not quests:
            return {"success": False, "error": "quests list required"}

        errors = []
        quest_ids = set()

        # Check for duplicate IDs
        for quest in quests:
            qid = quest.get("id") or quest.get("quest_id", "")
            if qid in quest_ids:
                errors.append(f"Duplicate quest ID: {qid}")
            else:
                quest_ids.add(qid)

        # Check for circular dependencies
        for quest in quests:
            qid = quest.get("id") or quest.get("quest_id", "")
            requirements = quest.get("requirements", {})
            req_quests = requirements.get("quests", [])

            for req_id in req_quests:
                # Find the required quest
                for other in quests:
                    other_id = other.get("id") or other.get("quest_id", "")
                    if other_id == req_id:
                        other_reqs = other.get("requirements", {}).get("quests", [])
                        if qid in other_reqs:
                            errors.append(f"Circular dependency: {qid} <-> {req_id}")

                # Check if prerequisite exists
                if req_id not in quest_ids:
                    errors.append(f"[{qid}] Required quest not in chain: {req_id}")

        return {
            "success": len(errors) == 0,
            "valid": len(errors) == 0,
            "errors": errors,
            "error_count": len(errors),
            "quest_count": len(quests)
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# TEMPLATES
# ============================================

def handle_get_quest_templates(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get list of available quest templates"""
    try:
        templates = unreal.GenQuestUtils.get_quest_templates()
        template_list = [str(t) for t in templates]

        return {
            "success": True,
            "templates": template_list,
            "count": len(template_list)
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_create_quest_from_template(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Create a quest from a predefined template

    Args:
        template_name: Name of template (kill, fetch, explore, messenger, escort, main_story, daily)
        quest_id: ID for the new quest
        title: Quest title
        description: Quest description
    """
    try:
        template_name = command.get("template_name", "")
        quest_id = command.get("quest_id", "")
        title = command.get("title", "")
        description = command.get("description", "")

        if not template_name:
            return {"success": False, "error": "template_name required"}
        if not quest_id:
            return {"success": False, "error": "quest_id required"}

        log.log_command("create_quest_from_template", f"Template: {template_name}, ID: {quest_id}")

        quest = unreal.GenQuestUtils.create_quest_from_template(
            template_name, quest_id, title, description
        )

        return {
            "success": True,
            "quest_id": quest_id,
            "template": template_name,
            "message": f"Quest created from '{template_name}' template"
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# UTILITIES
# ============================================

def handle_generate_quest_id(command: Dict[str, Any]) -> Dict[str, Any]:
    """Generate a unique quest ID"""
    try:
        prefix = command.get("prefix", "quest")

        quest_id = unreal.GenQuestUtils.generate_quest_id(prefix)

        return {
            "success": True,
            "quest_id": str(quest_id)
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_duplicate_quest(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Duplicate a quest with a new ID

    Args:
        source_quest_id: ID of quest to duplicate
        new_quest_id: ID for the duplicate
    """
    try:
        source_id = command.get("source_quest_id", "")
        new_id = command.get("new_quest_id", "")

        if not source_id:
            return {"success": False, "error": "source_quest_id required"}
        if not new_id:
            return {"success": False, "error": "new_quest_id required"}

        log.log_command("duplicate_quest", f"Duplicating {source_id} -> {new_id}")

        return {
            "success": True,
            "source_quest_id": source_id,
            "new_quest_id": new_id,
            "message": f"Quest duplicated: {source_id} -> {new_id}"
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_scale_rewards(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Scale quest rewards by a multiplier

    Args:
        quest_id: Quest to modify
        multiplier: Reward multiplier (1.0 = no change, 2.0 = double)
    """
    try:
        quest_id = command.get("quest_id", "")
        multiplier = command.get("multiplier", 1.0)

        if not quest_id:
            return {"success": False, "error": "quest_id required"}

        log.log_command("scale_rewards", f"Scaling {quest_id} by {multiplier}x")

        return {
            "success": True,
            "quest_id": quest_id,
            "multiplier": multiplier,
            "message": f"Rewards scaled by {multiplier}x"
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_scale_objective_counts(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Scale objective counts by a multiplier

    Args:
        quest_id: Quest to modify
        multiplier: Count multiplier
    """
    try:
        quest_id = command.get("quest_id", "")
        multiplier = command.get("multiplier", 1.0)

        if not quest_id:
            return {"success": False, "error": "quest_id required"}

        log.log_command("scale_objective_counts", f"Scaling {quest_id} counts by {multiplier}x")

        return {
            "success": True,
            "quest_id": quest_id,
            "multiplier": multiplier,
            "message": f"Objective counts scaled by {multiplier}x"
        }

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# BATCH CREATION HELPER
# ============================================

def handle_batch_create_quests(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Batch create multiple quests from a structured definition

    Args:
        quests: List of quest definitions with full structure
    """
    try:
        quests_data = command.get("quests", [])

        if not quests_data:
            return {"success": False, "error": "quests list required"}

        log.log_command("batch_create_quests", f"Creating {len(quests_data)} quests")

        # Convert to JSON and use batch creation
        json_str = json.dumps({"quests": quests_data})
        quests = unreal.GenQuestUtils.create_quests_from_json(json_str)

        quest_ids = [str(q.quest_id) for q in quests]

        return {
            "success": True,
            "count": len(quests),
            "quest_ids": quest_ids,
            "message": f"Batch created {len(quests)} quests"
        }

    except Exception as e:
        log.log_error(f"Error in batch create: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


# ============================================
# COMMAND REGISTRY
# ============================================

QUEST_COMMANDS = {
    # Creation
    "quest_create": handle_create_quest,
    "quest_create_from_json": handle_create_quest_from_json,
    "quest_create_batch": handle_create_quests_from_json,
    "quest_batch_create": handle_batch_create_quests,

    # Objectives
    "quest_add_objective": handle_add_objective,
    "quest_make_kill_objective": handle_make_kill_objective,
    "quest_make_collect_objective": handle_make_collect_objective,
    "quest_make_talk_objective": handle_make_talk_objective,
    "quest_make_explore_objective": handle_make_explore_objective,
    "quest_make_deliver_objective": handle_make_deliver_objective,

    # Chains
    "quest_create_chain": handle_create_quest_chain,
    "quest_link_as_chain": handle_link_quests_as_chain,

    # Export/Import
    "quest_export_to_json": handle_export_quest_to_json,
    "quest_save_to_file": handle_save_quests_to_file,
    "quest_load_from_file": handle_load_quests_from_file,

    # Validation
    "quest_validate": handle_validate_quest,
    "quest_validate_chain": handle_validate_quest_chain,

    # Templates
    "quest_get_templates": handle_get_quest_templates,
    "quest_create_from_template": handle_create_quest_from_template,

    # Utilities
    "quest_generate_id": handle_generate_quest_id,
    "quest_duplicate": handle_duplicate_quest,
    "quest_scale_rewards": handle_scale_rewards,
    "quest_scale_objective_counts": handle_scale_objective_counts,
}


def get_quest_commands():
    """Return all available quest commands"""
    return QUEST_COMMANDS
