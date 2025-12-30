"""
DataTable Commands Handler
Wrapper for GenDataTableUtils C++ functions
DataTable CRUD, Import/Export, CurveTables
"""

import unreal
import json
from typing import Dict, Any
from utils import logging as log


def _parse_json_response(json_str: str) -> Dict[str, Any]:
    """Parse JSON string response from C++ GenUtils"""
    try:
        return json.loads(json_str)
    except json.JSONDecodeError as e:
        return {"success": False, "error": f"JSON parse error: {str(e)}"}


# ============================================
# DATATABLE DISCOVERY
# ============================================

def handle_list_data_tables(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    List all DataTables in a directory

    Args:
        command: Dictionary with:
            - directory_path: Directory to search (default /Game)
    """
    try:
        directory_path = command.get("directory_path", "/Game")
        log.log_command("list_data_tables", f"Searching in {directory_path}")

        result = unreal.GenDataTableUtils.list_data_tables(directory_path)
        return _parse_json_response(result)

    except Exception as e:
        log.log_error(f"Error listing DataTables: {str(e)}", include_traceback=True)
        return {"success": False, "error": str(e)}


def handle_get_data_table_info(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get DataTable structure info (row struct, columns)"""
    try:
        datatable_path = command.get("datatable_path")
        if not datatable_path:
            return {"success": False, "error": "datatable_path required"}

        result = unreal.GenDataTableUtils.get_data_table_info(datatable_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_all_rows(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all rows from a DataTable as JSON"""
    try:
        datatable_path = command.get("datatable_path")
        if not datatable_path:
            return {"success": False, "error": "datatable_path required"}

        result = unreal.GenDataTableUtils.get_all_rows(datatable_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_row(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get a specific row from a DataTable"""
    try:
        datatable_path = command.get("datatable_path")
        row_name = command.get("row_name")

        if not datatable_path or not row_name:
            return {"success": False, "error": "datatable_path and row_name required"}

        result = unreal.GenDataTableUtils.get_row(datatable_path, row_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_search_rows(command: Dict[str, Any]) -> Dict[str, Any]:
    """Search rows by column value"""
    try:
        datatable_path = command.get("datatable_path")
        column_name = command.get("column_name")
        search_value = command.get("search_value")

        if not datatable_path or not column_name or not search_value:
            return {"success": False, "error": "datatable_path, column_name and search_value required"}

        result = unreal.GenDataTableUtils.search_rows(
            datatable_path, column_name, search_value
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# ROW MANIPULATION
# ============================================

def handle_add_datatable_row(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Add a new row to a DataTable
    NOTE: This may be limited in UE5.5 due to API changes
    """
    try:
        datatable_path = command.get("datatable_path")
        row_name = command.get("row_name")
        row_data_json = command.get("row_data_json", "{}")

        if not datatable_path or not row_name:
            return {"success": False, "error": "datatable_path and row_name required"}

        result = unreal.GenDataTableUtils.add_row(datatable_path, row_name, row_data_json)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_update_datatable_row(command: Dict[str, Any]) -> Dict[str, Any]:
    """Update an existing row in a DataTable"""
    try:
        datatable_path = command.get("datatable_path")
        row_name = command.get("row_name")
        row_data_json = command.get("row_data_json", "{}")

        if not datatable_path or not row_name:
            return {"success": False, "error": "datatable_path and row_name required"}

        result = unreal.GenDataTableUtils.update_row(datatable_path, row_name, row_data_json)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_delete_datatable_row(command: Dict[str, Any]) -> Dict[str, Any]:
    """Delete a row from a DataTable"""
    try:
        datatable_path = command.get("datatable_path")
        row_name = command.get("row_name")

        if not datatable_path or not row_name:
            return {"success": False, "error": "datatable_path and row_name required"}

        result = unreal.GenDataTableUtils.delete_row(datatable_path, row_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_rename_datatable_row(command: Dict[str, Any]) -> Dict[str, Any]:
    """Rename a row in a DataTable"""
    try:
        datatable_path = command.get("datatable_path")
        old_row_name = command.get("old_row_name")
        new_row_name = command.get("new_row_name")

        if not datatable_path or not old_row_name or not new_row_name:
            return {"success": False, "error": "datatable_path, old_row_name and new_row_name required"}

        result = unreal.GenDataTableUtils.rename_row(
            datatable_path, old_row_name, new_row_name
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# DATATABLE CREATION
# ============================================

def handle_create_data_table(command: Dict[str, Any]) -> Dict[str, Any]:
    """Create a new DataTable asset"""
    try:
        datatable_path = command.get("datatable_path")
        row_struct_path = command.get("row_struct_path")

        if not datatable_path or not row_struct_path:
            return {"success": False, "error": "datatable_path and row_struct_path required"}

        log.log_command("create_data_table", f"Creating {datatable_path}")
        result = unreal.GenDataTableUtils.create_data_table(datatable_path, row_struct_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_duplicate_data_table(command: Dict[str, Any]) -> Dict[str, Any]:
    """Duplicate a DataTable"""
    try:
        source_path = command.get("source_path")
        dest_path = command.get("dest_path")

        if not source_path or not dest_path:
            return {"success": False, "error": "source_path and dest_path required"}

        result = unreal.GenDataTableUtils.duplicate_data_table(source_path, dest_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_delete_data_table(command: Dict[str, Any]) -> Dict[str, Any]:
    """Delete a DataTable asset"""
    try:
        datatable_path = command.get("datatable_path")
        if not datatable_path:
            return {"success": False, "error": "datatable_path required"}

        result = unreal.GenDataTableUtils.delete_data_table(datatable_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# IMPORT/EXPORT
# ============================================

def handle_export_datatable_to_json(command: Dict[str, Any]) -> Dict[str, Any]:
    """Export DataTable to JSON file"""
    try:
        datatable_path = command.get("datatable_path")
        output_file_path = command.get("output_file_path")

        if not datatable_path or not output_file_path:
            return {"success": False, "error": "datatable_path and output_file_path required"}

        result = unreal.GenDataTableUtils.export_to_json(datatable_path, output_file_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_export_datatable_to_csv(command: Dict[str, Any]) -> Dict[str, Any]:
    """Export DataTable to CSV file"""
    try:
        datatable_path = command.get("datatable_path")
        output_file_path = command.get("output_file_path")

        if not datatable_path or not output_file_path:
            return {"success": False, "error": "datatable_path and output_file_path required"}

        result = unreal.GenDataTableUtils.export_to_csv(datatable_path, output_file_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_import_datatable_from_json(command: Dict[str, Any]) -> Dict[str, Any]:
    """Import rows from JSON file"""
    try:
        datatable_path = command.get("datatable_path")
        input_file_path = command.get("input_file_path")
        clear_existing = command.get("clear_existing", False)

        if not datatable_path or not input_file_path:
            return {"success": False, "error": "datatable_path and input_file_path required"}

        result = unreal.GenDataTableUtils.import_from_json(
            datatable_path, input_file_path, clear_existing
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_import_datatable_from_csv(command: Dict[str, Any]) -> Dict[str, Any]:
    """Import rows from CSV file"""
    try:
        datatable_path = command.get("datatable_path")
        input_file_path = command.get("input_file_path")
        clear_existing = command.get("clear_existing", False)

        if not datatable_path or not input_file_path:
            return {"success": False, "error": "datatable_path and input_file_path required"}

        result = unreal.GenDataTableUtils.import_from_csv(
            datatable_path, input_file_path, clear_existing
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# ROW STRUCT UTILITIES
# ============================================

def handle_list_row_structs(command: Dict[str, Any]) -> Dict[str, Any]:
    """List available row structs"""
    try:
        filter_pattern = command.get("filter_pattern", "")
        result = unreal.GenDataTableUtils.list_row_structs(filter_pattern)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_struct_properties(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get properties of a row struct"""
    try:
        struct_path = command.get("struct_path")
        if not struct_path:
            return {"success": False, "error": "struct_path required"}

        result = unreal.GenDataTableUtils.get_struct_properties(struct_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# CURVE TABLES
# ============================================

def handle_list_curve_tables(command: Dict[str, Any]) -> Dict[str, Any]:
    """List all CurveTables in a directory"""
    try:
        directory_path = command.get("directory_path", "/Game")
        result = unreal.GenDataTableUtils.list_curve_tables(directory_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_curve_data(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get curve data from a CurveTable"""
    try:
        curve_table_path = command.get("curve_table_path")
        row_name = command.get("row_name")

        if not curve_table_path or not row_name:
            return {"success": False, "error": "curve_table_path and row_name required"}

        result = unreal.GenDataTableUtils.get_curve_data(curve_table_path, row_name)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_set_curve_data(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set curve data in a CurveTable"""
    try:
        curve_table_path = command.get("curve_table_path")
        row_name = command.get("row_name")
        points_json = command.get("points_json", "[]")

        if not curve_table_path or not row_name:
            return {"success": False, "error": "curve_table_path and row_name required"}

        result = unreal.GenDataTableUtils.set_curve_data(
            curve_table_path, row_name, points_json
        )
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# COMPOSITE TABLES
# ============================================

def handle_list_composite_tables(command: Dict[str, Any]) -> Dict[str, Any]:
    """List all CompositeDataTables"""
    try:
        directory_path = command.get("directory_path", "/Game")
        result = unreal.GenDataTableUtils.list_composite_tables(directory_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_get_parent_tables(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get parent tables of a CompositeDataTable"""
    try:
        composite_table_path = command.get("composite_table_path")
        if not composite_table_path:
            return {"success": False, "error": "composite_table_path required"}

        result = unreal.GenDataTableUtils.get_parent_tables(composite_table_path)
        return _parse_json_response(result)

    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================
# COMMAND REGISTRY
# ============================================

DATATABLE_COMMANDS = {
    # Discovery
    "list_data_tables": handle_list_data_tables,
    "get_data_table_info": handle_get_data_table_info,
    "get_all_rows": handle_get_all_rows,
    "get_row": handle_get_row,
    "search_rows": handle_search_rows,

    # Row Manipulation
    "add_datatable_row": handle_add_datatable_row,
    "update_datatable_row": handle_update_datatable_row,
    "delete_datatable_row": handle_delete_datatable_row,
    "rename_datatable_row": handle_rename_datatable_row,

    # DataTable Creation
    "create_data_table": handle_create_data_table,
    "duplicate_data_table": handle_duplicate_data_table,
    "delete_data_table": handle_delete_data_table,

    # Import/Export
    "export_datatable_to_json": handle_export_datatable_to_json,
    "export_datatable_to_csv": handle_export_datatable_to_csv,
    "import_datatable_from_json": handle_import_datatable_from_json,
    "import_datatable_from_csv": handle_import_datatable_from_csv,

    # Row Structs
    "list_row_structs": handle_list_row_structs,
    "get_struct_properties": handle_get_struct_properties,

    # Curve Tables
    "list_curve_tables": handle_list_curve_tables,
    "get_curve_data": handle_get_curve_data,
    "set_curve_data": handle_set_curve_data,

    # Composite Tables
    "list_composite_tables": handle_list_composite_tables,
    "get_parent_tables": handle_get_parent_tables,
}


def get_datatable_commands():
    """Return all available DataTable commands"""
    return DATATABLE_COMMANDS
