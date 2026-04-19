import json
from typing import Any, Dict

import unreal

from utils import logging as log


def handle_start_fab_search(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        query = command.get("query", "")
        max_results = int(command.get("max_results", 10))
        timeout_seconds = float(command.get("timeout_seconds", 60.0))

        response_str = unreal.GenFabUtils.start_search_free_fab_assets(query, max_results, timeout_seconds)
        response = json.loads(response_str)
        log.log_result("start_fab_search", response.get("success", False), response.get("status") or response.get("error"))
        return response
    except Exception as e:
        log.log_error(f"Error starting Fab search: {str(e)}", include_traceback=True)
        return {"success": False, "status": "error", "error": str(e)}


def handle_start_fab_add_to_project(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        listing_id_or_url = command.get("listing_id_or_url", "")
        timeout_seconds = float(command.get("timeout_seconds", 180.0))

        response_str = unreal.GenFabUtils.start_add_free_fab_asset_to_project(listing_id_or_url, timeout_seconds)
        response = json.loads(response_str)
        log.log_result("start_fab_add_to_project", response.get("success", False), response.get("status") or response.get("error"))
        return response
    except Exception as e:
        log.log_error(f"Error starting Fab import: {str(e)}", include_traceback=True)
        return {"success": False, "status": "error", "error": str(e)}


def handle_get_fab_operation_status(command: Dict[str, Any]) -> Dict[str, Any]:
    try:
        operation_id = command.get("operation_id", "")
        response_str = unreal.GenFabUtils.get_fab_operation_status(operation_id)
        return json.loads(response_str)
    except Exception as e:
        log.log_error(f"Error fetching Fab operation status: {str(e)}", include_traceback=True)
        return {"success": False, "status": "error", "error": str(e)}
