"""
Asset Registry Commands Handler
Nutzt AssetRegistryHelpers fuer Asset-Discovery

Commands:
- find_assets_by_class: Assets einer Klasse finden
- find_assets_by_path: Assets in Pfad finden
- get_asset_dependencies: Asset Dependencies abfragen
- get_asset_referencers: Wer referenziert Asset?
- search_assets: Textsuche in Asset-Namen
"""

import unreal
from typing import Dict, Any, List
from utils import logging as log


def get_asset_registry():
    """Get the Asset Registry"""
    return unreal.AssetRegistryHelpers.get_asset_registry()


def handle_find_assets_by_class(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Find all assets of a specific class

    Args:
        command: Dictionary with:
            - class_name: Name of the class (e.g., "Blueprint", "StaticMesh", "Material")
            - path: Optional path to search in (default: /Game)
            - recursive: Search subdirectories (default: True)
    """
    try:
        class_name = command.get("class_name")
        search_path = command.get("path", "/Game")
        recursive = command.get("recursive", True)

        if not class_name:
            return {"success": False, "error": "class_name required"}

        log.log_command("find_assets_by_class", f"Finding {class_name} assets in {search_path}")

        registry = get_asset_registry()

        # Create filter
        filter = unreal.ARFilter()
        filter.class_names = [class_name]
        filter.package_paths = [search_path]
        filter.recursive_paths = recursive

        assets = registry.get_assets(filter)

        asset_list = []
        for asset in assets:
            asset_list.append({
                "name": str(asset.asset_name),
                "path": str(asset.package_name),
                "class": str(asset.asset_class)
            })

        log.log_result("find_assets_by_class", True, f"Found {len(asset_list)} assets")
        return {
            "success": True,
            "class_name": class_name,
            "assets": asset_list,
            "count": len(asset_list)
        }

    except Exception as e:
        log.log_error(f"Error finding assets: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_find_assets_by_path(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Find all assets in a specific path

    Args:
        command: Dictionary with:
            - path: Path to search (e.g., "/Game/Characters/NPCs")
            - recursive: Search subdirectories (default: True)
    """
    try:
        search_path = command.get("path")
        recursive = command.get("recursive", True)

        if not search_path:
            return {"success": False, "error": "path required"}

        log.log_command("find_assets_by_path", f"Finding assets in {search_path}")

        registry = get_asset_registry()

        filter = unreal.ARFilter()
        filter.package_paths = [search_path]
        filter.recursive_paths = recursive

        assets = registry.get_assets(filter)

        asset_list = []
        for asset in assets:
            asset_list.append({
                "name": str(asset.asset_name),
                "path": str(asset.package_name),
                "class": str(asset.asset_class)
            })

        log.log_result("find_assets_by_path", True, f"Found {len(asset_list)} assets")
        return {
            "success": True,
            "path": search_path,
            "assets": asset_list,
            "count": len(asset_list)
        }

    except Exception as e:
        log.log_error(f"Error finding assets: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_asset_dependencies(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Get all dependencies of an asset

    Args:
        command: Dictionary with:
            - asset_path: Full path to the asset
    """
    try:
        asset_path = command.get("asset_path")

        if not asset_path:
            return {"success": False, "error": "asset_path required"}

        log.log_command("get_asset_dependencies", f"Getting dependencies for {asset_path}")

        registry = get_asset_registry()
        dependencies = registry.get_dependencies(asset_path)

        dep_list = [str(dep) for dep in dependencies]

        log.log_result("get_asset_dependencies", True, f"Found {len(dep_list)} dependencies")
        return {
            "success": True,
            "asset_path": asset_path,
            "dependencies": dep_list,
            "count": len(dep_list)
        }

    except Exception as e:
        log.log_error(f"Error getting dependencies: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_asset_referencers(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Get all assets that reference this asset

    Args:
        command: Dictionary with:
            - asset_path: Full path to the asset
    """
    try:
        asset_path = command.get("asset_path")

        if not asset_path:
            return {"success": False, "error": "asset_path required"}

        log.log_command("get_asset_referencers", f"Getting referencers for {asset_path}")

        registry = get_asset_registry()
        referencers = registry.get_referencers(asset_path)

        ref_list = [str(ref) for ref in referencers]

        log.log_result("get_asset_referencers", True, f"Found {len(ref_list)} referencers")
        return {
            "success": True,
            "asset_path": asset_path,
            "referencers": ref_list,
            "count": len(ref_list)
        }

    except Exception as e:
        log.log_error(f"Error getting referencers: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_search_assets(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Search assets by name pattern

    Args:
        command: Dictionary with:
            - search_text: Text to search for in asset names
            - path: Optional path to limit search (default: /Game)
            - class_name: Optional class filter
    """
    try:
        search_text = command.get("search_text")
        search_path = command.get("path", "/Game")
        class_name = command.get("class_name", None)

        if not search_text:
            return {"success": False, "error": "search_text required"}

        log.log_command("search_assets", f"Searching for '{search_text}' in {search_path}")

        registry = get_asset_registry()

        filter = unreal.ARFilter()
        filter.package_paths = [search_path]
        filter.recursive_paths = True

        if class_name:
            filter.class_names = [class_name]

        assets = registry.get_assets(filter)

        # Filter by search text
        matching_assets = []
        search_lower = search_text.lower()

        for asset in assets:
            asset_name = str(asset.asset_name).lower()
            if search_lower in asset_name:
                matching_assets.append({
                    "name": str(asset.asset_name),
                    "path": str(asset.package_name),
                    "class": str(asset.asset_class)
                })

        log.log_result("search_assets", True, f"Found {len(matching_assets)} matching assets")
        return {
            "success": True,
            "search_text": search_text,
            "assets": matching_assets[:100],  # Limit to 100 results
            "count": len(matching_assets),
            "truncated": len(matching_assets) > 100
        }

    except Exception as e:
        log.log_error(f"Error searching assets: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_find_blueprints_of_type(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Find all Blueprints of a specific parent class

    Args:
        command: Dictionary with:
            - parent_class: Parent class name (e.g., "Character", "Actor", "Pawn")
            - path: Optional search path
    """
    try:
        parent_class = command.get("parent_class")
        search_path = command.get("path", "/Game")

        if not parent_class:
            return {"success": False, "error": "parent_class required"}

        log.log_command("find_blueprints_of_type", f"Finding Blueprints inheriting from {parent_class}")

        registry = get_asset_registry()

        # Get all Blueprints first
        filter = unreal.ARFilter()
        filter.class_names = ["Blueprint"]
        filter.package_paths = [search_path]
        filter.recursive_paths = True

        assets = registry.get_assets(filter)

        matching_bps = []

        for asset in assets:
            try:
                bp = unreal.EditorAssetLibrary.load_asset(str(asset.package_name))
                if bp:
                    generated_class = bp.generated_class()
                    if generated_class:
                        parent = generated_class.get_super_class()
                        if parent and parent_class.lower() in str(parent.get_name()).lower():
                            matching_bps.append({
                                "name": str(asset.asset_name),
                                "path": str(asset.package_name),
                                "parent": str(parent.get_name())
                            })
            except:
                pass  # Skip if can't load

        log.log_result("find_blueprints_of_type", True, f"Found {len(matching_bps)} Blueprints")
        return {
            "success": True,
            "parent_class": parent_class,
            "blueprints": matching_bps,
            "count": len(matching_bps)
        }

    except Exception as e:
        log.log_error(f"Error finding blueprints: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


# ============================================================================
# COMMAND REGISTRY
# ============================================================================

ASSET_REGISTRY_COMMANDS = {
    "find_assets_by_class": handle_find_assets_by_class,
    "find_assets_by_path": handle_find_assets_by_path,
    "get_asset_dependencies": handle_get_asset_dependencies,
    "get_asset_referencers": handle_get_asset_referencers,
    "search_assets": handle_search_assets,
    "find_blueprints_of_type": handle_find_blueprints_of_type,
}


def get_asset_registry_commands():
    """Return all available asset registry commands"""
    return ASSET_REGISTRY_COMMANDS
