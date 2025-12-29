"""
Editor Commands Handler - Extended Unreal API Features
Created: 2025-12-29
"""

from typing import Dict, Any
import unreal
import json

# ============================================================================
# LEVEL MANAGEMENT
# ============================================================================

def handle_load_level(command: Dict[str, Any]) -> Dict[str, Any]:
    """Load a level/map in the editor"""
    try:
        level_path = command.get("level_path")
        if not level_path:
            return {"success": False, "error": "level_path required"}

        success = unreal.EditorLevelLibrary.load_level(level_path)
        return {"success": success, "message": f"Loaded level: {level_path}"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_save_current_level(command: Dict[str, Any]) -> Dict[str, Any]:
    """Save the current level"""
    try:
        success = unreal.EditorLevelLibrary.save_current_level()
        return {"success": success, "message": "Current level saved"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_save_all_dirty_levels(command: Dict[str, Any]) -> Dict[str, Any]:
    """Save all modified levels"""
    try:
        success = unreal.EditorLevelLibrary.save_all_dirty_levels()
        return {"success": success, "message": "All dirty levels saved"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_start_pie(command: Dict[str, Any]) -> Dict[str, Any]:
    """Start Play-In-Editor (simulate)"""
    try:
        unreal.EditorLevelLibrary.editor_play_simulate()
        return {"success": True, "message": "PIE started"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_stop_pie(command: Dict[str, Any]) -> Dict[str, Any]:
    """Stop Play-In-Editor"""
    try:
        unreal.EditorLevelLibrary.editor_end_play()
        return {"success": True, "message": "PIE stopped"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_pilot_actor(command: Dict[str, Any]) -> Dict[str, Any]:
    """Move editor camera to pilot an actor"""
    try:
        actor_name = command.get("actor_name")
        if not actor_name:
            return {"success": False, "error": "actor_name required"}

        actors = unreal.EditorLevelLibrary.get_all_level_actors()
        for actor in actors:
            if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                unreal.EditorLevelLibrary.pilot_level_actor(actor)
                return {"success": True, "message": f"Piloting actor: {actor_name}"}

        return {"success": False, "error": f"Actor not found: {actor_name}"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_set_level_visibility(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set visibility of a streaming level"""
    try:
        level_name = command.get("level_name")
        visible = command.get("visible", True)

        # Get all streaming levels
        world = unreal.EditorLevelLibrary.get_editor_world()
        if world:
            levels = world.get_streaming_levels()
            for level in levels:
                if level_name in level.get_world_asset_package_name():
                    level.set_should_be_visible(visible)
                    return {"success": True, "message": f"Level {level_name} visibility: {visible}"}

        return {"success": False, "error": f"Level not found: {level_name}"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================================================
# ASSET MANAGEMENT
# ============================================================================

def handle_delete_asset(command: Dict[str, Any]) -> Dict[str, Any]:
    """Delete an asset"""
    try:
        asset_path = command.get("asset_path")
        if not asset_path:
            return {"success": False, "error": "asset_path required"}

        success = unreal.EditorAssetLibrary.delete_asset(asset_path)
        return {"success": success, "message": f"Deleted asset: {asset_path}"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_duplicate_asset(command: Dict[str, Any]) -> Dict[str, Any]:
    """Duplicate an asset"""
    try:
        source_path = command.get("source_path")
        dest_path = command.get("dest_path")

        if not source_path or not dest_path:
            return {"success": False, "error": "source_path and dest_path required"}

        result = unreal.EditorAssetLibrary.duplicate_asset(source_path, dest_path)
        if result:
            return {"success": True, "new_asset": dest_path}
        return {"success": False, "error": "Failed to duplicate asset"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_rename_asset(command: Dict[str, Any]) -> Dict[str, Any]:
    """Rename/move an asset"""
    try:
        source_path = command.get("source_path")
        dest_path = command.get("dest_path")

        if not source_path or not dest_path:
            return {"success": False, "error": "source_path and dest_path required"}

        success = unreal.EditorAssetLibrary.rename_asset(source_path, dest_path)
        return {"success": success, "new_path": dest_path}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_save_asset(command: Dict[str, Any]) -> Dict[str, Any]:
    """Save a specific asset"""
    try:
        asset_path = command.get("asset_path")
        if not asset_path:
            return {"success": False, "error": "asset_path required"}

        success = unreal.EditorAssetLibrary.save_asset(asset_path)
        return {"success": success, "message": f"Saved asset: {asset_path}"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_does_asset_exist(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if an asset exists"""
    try:
        asset_path = command.get("asset_path")
        if not asset_path:
            return {"success": False, "error": "asset_path required"}

        exists = unreal.EditorAssetLibrary.does_asset_exist(asset_path)
        return {"success": True, "exists": exists, "asset_path": asset_path}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_sync_browser(command: Dict[str, Any]) -> Dict[str, Any]:
    """Sync Content Browser to asset(s)"""
    try:
        asset_paths = command.get("asset_paths", [])
        if isinstance(asset_paths, str):
            asset_paths = [asset_paths]

        assets = [unreal.EditorAssetLibrary.load_asset(p) for p in asset_paths if p]
        assets = [a for a in assets if a]

        if assets:
            unreal.EditorAssetLibrary.sync_browser_to_objects(assets)
            return {"success": True, "message": f"Synced browser to {len(assets)} asset(s)"}
        return {"success": False, "error": "No valid assets found"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================================================
# ACTOR SELECTION
# ============================================================================

def handle_select_actors(command: Dict[str, Any]) -> Dict[str, Any]:
    """Select actors in the level"""
    try:
        actor_names = command.get("actor_names", [])
        if isinstance(actor_names, str):
            actor_names = [actor_names]

        all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
        to_select = []

        for actor in all_actors:
            if actor.get_actor_label() in actor_names or actor.get_name() in actor_names:
                to_select.append(actor)

        if to_select:
            unreal.EditorLevelLibrary.set_selected_level_actors(to_select)
            return {"success": True, "selected_count": len(to_select)}
        return {"success": False, "error": "No matching actors found"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_selected_actors(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get currently selected actors"""
    try:
        selected = unreal.EditorLevelLibrary.get_selected_level_actors()
        actors = []
        for actor in selected:
            actors.append({
                "name": actor.get_name(),
                "label": actor.get_actor_label(),
                "class": actor.get_class().get_name(),
                "location": [actor.get_actor_location().x, actor.get_actor_location().y, actor.get_actor_location().z]
            })
        return {"success": True, "actors": actors, "count": len(actors)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_clear_selection(command: Dict[str, Any]) -> Dict[str, Any]:
    """Clear actor selection"""
    try:
        unreal.EditorLevelLibrary.select_nothing()
        return {"success": True, "message": "Selection cleared"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================================================
# MATERIAL EDITING
# ============================================================================

def handle_create_material_expression(command: Dict[str, Any]) -> Dict[str, Any]:
    """Add an expression node to a material"""
    try:
        material_path = command.get("material_path")
        expression_class = command.get("expression_class")  # e.g., "MaterialExpressionTextureSample"
        pos_x = command.get("pos_x", 0)
        pos_y = command.get("pos_y", 0)

        if not material_path or not expression_class:
            return {"success": False, "error": "material_path and expression_class required"}

        material = unreal.EditorAssetLibrary.load_asset(material_path)
        if not material:
            return {"success": False, "error": f"Material not found: {material_path}"}

        # Get expression class
        expr_class = getattr(unreal, expression_class, None)
        if not expr_class:
            return {"success": False, "error": f"Expression class not found: {expression_class}"}

        # Create expression
        mel = unreal.MaterialEditingLibrary
        expression = mel.create_material_expression(material, expr_class, pos_x, pos_y)

        if expression:
            return {"success": True, "message": f"Created {expression_class}", "expression_name": expression.get_name()}
        return {"success": False, "error": "Failed to create expression"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_connect_material_expressions(command: Dict[str, Any]) -> Dict[str, Any]:
    """Connect two material expressions"""
    try:
        material_path = command.get("material_path")
        from_expression = command.get("from_expression")
        from_output = command.get("from_output", 0)
        to_expression = command.get("to_expression")
        to_input = command.get("to_input", 0)

        if not all([material_path, from_expression, to_expression]):
            return {"success": False, "error": "material_path, from_expression, to_expression required"}

        material = unreal.EditorAssetLibrary.load_asset(material_path)
        if not material:
            return {"success": False, "error": f"Material not found: {material_path}"}

        # Find expressions by name
        mel = unreal.MaterialEditingLibrary
        expressions = mel.get_material_expressions(material)

        from_expr = None
        to_expr = None
        for expr in expressions:
            if expr.get_name() == from_expression:
                from_expr = expr
            if expr.get_name() == to_expression:
                to_expr = expr

        if not from_expr or not to_expr:
            return {"success": False, "error": "Expression(s) not found"}

        success = mel.connect_material_expressions(from_expr, from_output, to_expr, to_input)
        return {"success": success, "message": "Expressions connected"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================================================
# STATIC MESH OPERATIONS
# ============================================================================

def handle_set_mesh_collision(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set collision complexity on a static mesh"""
    try:
        mesh_path = command.get("mesh_path")
        collision_type = command.get("collision_type", "SimpleAndComplex")  # Simple, Complex, SimpleAndComplex

        if not mesh_path:
            return {"success": False, "error": "mesh_path required"}

        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        if not mesh or not isinstance(mesh, unreal.StaticMesh):
            return {"success": False, "error": f"Static mesh not found: {mesh_path}"}

        # Map collision type
        collision_map = {
            "Simple": unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX,
            "Complex": unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,
            "SimpleAndComplex": unreal.CollisionTraceFlag.CTF_USE_DEFAULT
        }

        if collision_type in collision_map:
            body_setup = mesh.get_editor_property("body_setup")
            if body_setup:
                body_setup.set_editor_property("collision_trace_flag", collision_map[collision_type])
                return {"success": True, "message": f"Set collision to {collision_type}"}

        return {"success": False, "error": "Failed to set collision"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_generate_lods(command: Dict[str, Any]) -> Dict[str, Any]:
    """Generate LODs for a static mesh"""
    try:
        mesh_path = command.get("mesh_path")
        num_lods = command.get("num_lods", 4)

        if not mesh_path:
            return {"success": False, "error": "mesh_path required"}

        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        if not mesh or not isinstance(mesh, unreal.StaticMesh):
            return {"success": False, "error": f"Static mesh not found: {mesh_path}"}

        # Use StaticMeshEditorSubsystem if available
        if hasattr(unreal, "StaticMeshEditorSubsystem"):
            subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
            if subsystem:
                # Generate LODs with reduction settings
                options = unreal.EditorScriptingMeshReductionOptions()
                options.reduction_percentage = 0.5
                subsystem.set_lod_from_static_mesh(mesh, 1, mesh, 0, options)
                return {"success": True, "message": f"Generated LODs for {mesh_path}"}

        return {"success": False, "error": "StaticMeshEditorSubsystem not available"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_enable_nanite(command: Dict[str, Any]) -> Dict[str, Any]:
    """Enable/disable Nanite on a static mesh"""
    try:
        mesh_path = command.get("mesh_path")
        enable = command.get("enable", True)

        if not mesh_path:
            return {"success": False, "error": "mesh_path required"}

        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        if not mesh or not isinstance(mesh, unreal.StaticMesh):
            return {"success": False, "error": f"Static mesh not found: {mesh_path}"}

        # Set Nanite settings
        nanite_settings = mesh.get_editor_property("nanite_settings")
        if nanite_settings:
            nanite_settings.set_editor_property("enabled", enable)
            mesh.set_editor_property("nanite_settings", nanite_settings)
            unreal.EditorAssetLibrary.save_asset(mesh_path)
            return {"success": True, "message": f"Nanite {'enabled' if enable else 'disabled'} for {mesh_path}"}

        return {"success": False, "error": "Could not access Nanite settings"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================================================
# DATA TABLES
# ============================================================================

def handle_get_data_table_rows(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get all row names from a data table"""
    try:
        table_path = command.get("table_path")
        if not table_path:
            return {"success": False, "error": "table_path required"}

        table = unreal.EditorAssetLibrary.load_asset(table_path)
        if not table:
            return {"success": False, "error": f"Data table not found: {table_path}"}

        row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
        return {"success": True, "rows": [str(name) for name in row_names], "count": len(row_names)}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_data_table_row_exists(command: Dict[str, Any]) -> Dict[str, Any]:
    """Check if a row exists in a data table"""
    try:
        table_path = command.get("table_path")
        row_name = command.get("row_name")

        if not table_path or not row_name:
            return {"success": False, "error": "table_path and row_name required"}

        table = unreal.EditorAssetLibrary.load_asset(table_path)
        if not table:
            return {"success": False, "error": f"Data table not found: {table_path}"}

        exists = unreal.DataTableFunctionLibrary.does_data_table_row_exist(table, row_name)
        return {"success": True, "exists": exists, "row_name": row_name}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================================================
# VIEWPORT CONTROL
# ============================================================================

def handle_focus_viewport(command: Dict[str, Any]) -> Dict[str, Any]:
    """Focus viewport on selected actors or location"""
    try:
        location = command.get("location")  # [x, y, z]
        actor_name = command.get("actor_name")

        if actor_name:
            actors = unreal.EditorLevelLibrary.get_all_level_actors()
            for actor in actors:
                if actor.get_actor_label() == actor_name or actor.get_name() == actor_name:
                    unreal.EditorLevelLibrary.set_selected_level_actors([actor])
                    # Focus on selection
                    unreal.EditorLevelLibrary.pilot_level_actor(actor)
                    return {"success": True, "message": f"Focused on {actor_name}"}

        return {"success": False, "error": "Actor not found or no location specified"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_set_game_view(command: Dict[str, Any]) -> Dict[str, Any]:
    """Toggle game view mode in editor"""
    try:
        enabled = command.get("enabled", True)
        unreal.EditorLevelLibrary.editor_set_game_view(enabled)
        return {"success": True, "message": f"Game view {'enabled' if enabled else 'disabled'}"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================================================
# BLUEPRINT UTILITIES
# ============================================================================

def handle_reparent_blueprint(command: Dict[str, Any]) -> Dict[str, Any]:
    """Change parent class of a blueprint"""
    try:
        blueprint_path = command.get("blueprint_path")
        new_parent_class = command.get("new_parent_class")

        if not blueprint_path or not new_parent_class:
            return {"success": False, "error": "blueprint_path and new_parent_class required"}

        bp = unreal.EditorAssetLibrary.load_asset(blueprint_path)
        if not bp:
            return {"success": False, "error": f"Blueprint not found: {blueprint_path}"}

        # Get parent class
        parent = getattr(unreal, new_parent_class, None)
        if not parent:
            parent = unreal.EditorAssetLibrary.load_asset(new_parent_class)

        if parent:
            success = unreal.BlueprintEditorLibrary.reparent_blueprint(bp, parent)
            return {"success": success, "message": f"Reparented to {new_parent_class}"}

        return {"success": False, "error": f"Parent class not found: {new_parent_class}"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================================================
# SOURCE CONTROL (if available)
# ============================================================================

def handle_checkout_asset(command: Dict[str, Any]) -> Dict[str, Any]:
    """Checkout asset from source control"""
    try:
        asset_path = command.get("asset_path")
        if not asset_path:
            return {"success": False, "error": "asset_path required"}

        if hasattr(unreal, "SourceControlHelpers"):
            success = unreal.SourceControlHelpers.checkout_file(asset_path)
            return {"success": success, "message": f"Checked out: {asset_path}"}

        return {"success": False, "error": "Source control not available"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_mark_for_add(command: Dict[str, Any]) -> Dict[str, Any]:
    """Mark asset for add in source control"""
    try:
        asset_path = command.get("asset_path")
        if not asset_path:
            return {"success": False, "error": "asset_path required"}

        if hasattr(unreal, "SourceControlHelpers"):
            success = unreal.SourceControlHelpers.mark_file_for_add(asset_path)
            return {"success": success, "message": f"Marked for add: {asset_path}"}

        return {"success": False, "error": "Source control not available"}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================================================
# COMMAND REGISTRY
# ============================================================================

EDITOR_COMMANDS = {
    # Level Management
    "load_level": handle_load_level,
    "save_current_level": handle_save_current_level,
    "save_all_dirty_levels": handle_save_all_dirty_levels,
    "start_pie": handle_start_pie,
    "stop_pie": handle_stop_pie,
    "pilot_actor": handle_pilot_actor,
    "set_level_visibility": handle_set_level_visibility,

    # Asset Management
    "delete_asset": handle_delete_asset,
    "duplicate_asset": handle_duplicate_asset,
    "rename_asset": handle_rename_asset,
    "save_asset": handle_save_asset,
    "does_asset_exist": handle_does_asset_exist,
    "sync_browser": handle_sync_browser,

    # Actor Selection
    "select_actors": handle_select_actors,
    "get_selected_actors": handle_get_selected_actors,
    "clear_selection": handle_clear_selection,

    # Material Editing
    "create_material_expression": handle_create_material_expression,
    "connect_material_expressions": handle_connect_material_expressions,

    # Static Mesh
    "set_mesh_collision": handle_set_mesh_collision,
    "generate_lods": handle_generate_lods,
    "enable_nanite": handle_enable_nanite,

    # Data Tables
    "get_data_table_rows": handle_get_data_table_rows,
    "data_table_row_exists": handle_data_table_row_exists,

    # Viewport
    "focus_viewport": handle_focus_viewport,
    "set_game_view": handle_set_game_view,

    # Blueprint
    "reparent_blueprint": handle_reparent_blueprint,

    # Source Control
    "checkout_asset": handle_checkout_asset,
    "mark_for_add": handle_mark_for_add,
}

def get_editor_commands():
    """Return all available editor commands"""
    return EDITOR_COMMANDS
