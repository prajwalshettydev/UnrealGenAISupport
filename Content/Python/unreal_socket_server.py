import socket
import json
import unreal
import threading
import time
from typing import Dict, Any, Tuple, List, Optional

# Import handlers
from handlers import basic_commands, actor_commands, blueprint_commands, python_commands
from handlers import ui_commands, ai_commands, editor_commands, mapbuilding_commands
from handlers import input_commands, terrain_commands, layer_commands, asset_registry_commands
from handlers import kismet_utilities_commands, material_commands, skeletal_mesh_commands
from handlers import npc_dialog_commands
# NEW: GenUtils Handlers
from handlers import sequencer_commands, datatable_commands, behavior_tree_commands
from handlers import dialogue_utils_commands, quest_commands
from utils import logging as log

# Global queues and state
command_queue = []
response_dict = {}


class CommandDispatcher:
    """
    Dispatches commands to appropriate handlers based on command type
    """
    def __init__(self):
        # Register command handlers
        self.handlers = {
            "handshake": self._handle_handshake,

            # Basic object commands
            "spawn": basic_commands.handle_spawn,
            "create_material": basic_commands.handle_create_material,
            "modify_object": actor_commands.handle_modify_object,
            "take_screenshot": basic_commands.handle_take_screenshot,

            # Blueprint commands
            "create_blueprint": blueprint_commands.handle_create_blueprint,
            "add_component": blueprint_commands.handle_add_component,
            "add_variable": blueprint_commands.handle_add_variable,
            "add_function": blueprint_commands.handle_add_function,
            "add_node": blueprint_commands.handle_add_node,
            "connect_nodes": blueprint_commands.handle_connect_nodes,
            "compile_blueprint": blueprint_commands.handle_compile_blueprint,
            "spawn_blueprint": blueprint_commands.handle_spawn_blueprint,
            "delete_node": blueprint_commands.handle_delete_node,
            
            # Getters
            "get_node_guid": blueprint_commands.handle_get_node_guid,
            "get_all_nodes": blueprint_commands.handle_get_all_nodes,
            "get_node_suggestions": blueprint_commands.handle_get_node_suggestions,
            
            
            # Bulk commands
            "add_nodes_bulk": blueprint_commands.handle_add_nodes_bulk,
            "connect_nodes_bulk": blueprint_commands.handle_connect_nodes_bulk,
            
            # Python and console
            "execute_python": python_commands.handle_execute_python,
            "execute_unreal_command": python_commands.handle_execute_unreal_command,
            
            # New
            "edit_component_property": actor_commands.handle_edit_component_property,
            "add_component_with_events": actor_commands.handle_add_component_with_events,
            
            # Scene
            "get_all_scene_objects": basic_commands.handle_get_all_scene_objects,
            "create_project_folder": basic_commands.handle_create_project_folder,
            "get_files_in_folder": basic_commands.handle_get_files_in_folder,
            
            # Input
            "add_input_binding": basic_commands.handle_add_input_binding,

            # --- NEW UI COMMANDS ---
            "add_widget_to_user_widget": ui_commands.handle_add_widget_to_user_widget,
            "edit_widget_property": ui_commands.handle_edit_widget_property,

            # --- NPC AI COMMANDS ---
            "npc_chat": ai_commands.handle_npc_chat,
            "npc_reset": ai_commands.handle_npc_reset,

            # --- EXTENDED EDITOR COMMANDS ---
            # Level Management
            "load_level": editor_commands.handle_load_level,
            "save_current_level": editor_commands.handle_save_current_level,
            "save_all_dirty_levels": editor_commands.handle_save_all_dirty_levels,
            "start_pie": editor_commands.handle_start_pie,
            "stop_pie": editor_commands.handle_stop_pie,
            "pilot_actor": editor_commands.handle_pilot_actor,
            "set_level_visibility": editor_commands.handle_set_level_visibility,

            # Asset Management
            "delete_asset": editor_commands.handle_delete_asset,
            "duplicate_asset": editor_commands.handle_duplicate_asset,
            "rename_asset": editor_commands.handle_rename_asset,
            "save_asset": editor_commands.handle_save_asset,
            "does_asset_exist": editor_commands.handle_does_asset_exist,
            "sync_browser": editor_commands.handle_sync_browser,

            # Actor Selection
            "select_actors": editor_commands.handle_select_actors,
            "get_selected_actors": editor_commands.handle_get_selected_actors,
            "clear_selection": editor_commands.handle_clear_selection,

            # Material Editing
            "create_material_expression": editor_commands.handle_create_material_expression,
            "connect_material_expressions": editor_commands.handle_connect_material_expressions,

            # Static Mesh
            "set_mesh_collision": editor_commands.handle_set_mesh_collision,
            "generate_lods": editor_commands.handle_generate_lods,
            "enable_nanite": editor_commands.handle_enable_nanite,

            # Data Tables
            "get_data_table_rows": editor_commands.handle_get_data_table_rows,
            "data_table_row_exists": editor_commands.handle_data_table_row_exists,

            # Viewport
            "focus_viewport": editor_commands.handle_focus_viewport,
            "set_game_view": editor_commands.handle_set_game_view,

            # Blueprint
            "reparent_blueprint": editor_commands.handle_reparent_blueprint,

            # Source Control
            "checkout_asset": editor_commands.handle_checkout_asset,
            "mark_for_add": editor_commands.handle_mark_for_add,

            # --- MAPBUILDING COMMANDS ---
            # Landscape
            "create_landscape": mapbuilding_commands.handle_create_landscape,
            "import_landscape_heightmap": mapbuilding_commands.handle_import_landscape_heightmap,
            "export_landscape_heightmap": mapbuilding_commands.handle_export_landscape_heightmap,
            "get_landscape_info": mapbuilding_commands.handle_get_landscape_info,

            # Foliage
            "spawn_foliage_type": mapbuilding_commands.handle_spawn_foliage_type,
            "add_foliage_to_level": mapbuilding_commands.handle_add_foliage_to_level,
            "spawn_procedural_foliage": mapbuilding_commands.handle_spawn_procedural_foliage,
            "resimulate_foliage": mapbuilding_commands.handle_resimulate_foliage,

            # Navigation
            "rebuild_navigation": mapbuilding_commands.handle_rebuild_navigation,
            "get_navmesh_info": mapbuilding_commands.handle_get_navmesh_info,
            "add_nav_modifier_volume": mapbuilding_commands.handle_add_nav_modifier_volume,
            "find_path": mapbuilding_commands.handle_find_path,

            # Lighting
            "build_lighting": mapbuilding_commands.handle_build_lighting,
            "add_light": mapbuilding_commands.handle_add_light,

            # NPC / AI
            "spawn_ai_actor": mapbuilding_commands.handle_spawn_ai_actor,
            "spawn_npc": mapbuilding_commands.handle_spawn_npc,
            "set_ai_blackboard_value": mapbuilding_commands.handle_set_ai_blackboard_value,
            "run_behavior_tree": mapbuilding_commands.handle_run_behavior_tree,
            "move_ai_to": mapbuilding_commands.handle_move_ai_to,
            "get_all_npcs": mapbuilding_commands.handle_get_all_npcs,

            # Generic Actor Spawning
            "spawn_actor_at": mapbuilding_commands.handle_spawn_actor_at,
            "delete_actor": mapbuilding_commands.handle_delete_actor,
            "duplicate_actor": mapbuilding_commands.handle_duplicate_actor,

            # --- INPUT & BLUEPRINT INSPECTION COMMANDS ---
            "get_input_action_key": input_commands.handle_get_input_action_key,
            "get_blueprint_events": input_commands.handle_get_blueprint_events,
            "has_input_action_binding": input_commands.handle_has_input_action_binding,
            "list_all_input_actions": input_commands.handle_list_all_input_actions,

            # --- TERRAIN SCULPTING COMMANDS ---
            "sculpt_raise": terrain_commands.handle_sculpt_raise,
            "sculpt_lower": terrain_commands.handle_sculpt_lower,
            "sculpt_flatten": terrain_commands.handle_sculpt_flatten,
            "sculpt_smooth": terrain_commands.handle_sculpt_smooth,
            "generate_heightmap": terrain_commands.handle_generate_heightmap,
            "generate_mountain_range": terrain_commands.handle_generate_mountain_range,
            "generate_valley": terrain_commands.handle_generate_valley,
            "import_heightmap_to_landscape": terrain_commands.handle_import_heightmap_to_landscape,
            "export_heightmap_from_landscape": terrain_commands.handle_export_heightmap_from_landscape,
            "paint_layer": terrain_commands.handle_paint_layer,
            "auto_paint_by_slope": terrain_commands.handle_auto_paint_by_slope,
            "auto_paint_by_height": terrain_commands.handle_auto_paint_by_height,
            "apply_erosion": terrain_commands.handle_apply_erosion,
            "apply_noise": terrain_commands.handle_apply_noise,
            "create_terrain_preset": terrain_commands.handle_create_terrain_preset,

            # --- LAYER COMMANDS ---
            "create_layer": layer_commands.handle_create_layer,
            "delete_layer": layer_commands.handle_delete_layer,
            "rename_layer": layer_commands.handle_rename_layer,
            "add_actor_to_layer": layer_commands.handle_add_actor_to_layer,
            "remove_actor_from_layer": layer_commands.handle_remove_actor_from_layer,
            "get_actors_from_layer": layer_commands.handle_get_actors_from_layer,
            "set_layer_visibility": layer_commands.handle_set_layer_visibility,
            "get_all_layers": layer_commands.handle_get_all_layers,
            "add_actors_to_layer_by_pattern": layer_commands.handle_add_actors_to_layer_by_pattern,

            # --- ASSET REGISTRY COMMANDS ---
            "find_assets_by_class": asset_registry_commands.handle_find_assets_by_class,
            "find_assets_by_path": asset_registry_commands.handle_find_assets_by_path,
            "get_asset_dependencies": asset_registry_commands.handle_get_asset_dependencies,
            "get_asset_referencers": asset_registry_commands.handle_get_asset_referencers,
            "search_assets": asset_registry_commands.handle_search_assets,
            "find_blueprints_of_type": asset_registry_commands.handle_find_blueprints_of_type,

            # --- KISMET UTILITIES COMMANDS ---
            "create_blueprint_from_actor": kismet_utilities_commands.handle_create_blueprint_from_actor,
            "is_actor_valid_for_level_script": kismet_utilities_commands.handle_is_actor_valid_for_level_script,
            "compile_blueprint_by_path": kismet_utilities_commands.handle_compile_blueprint_by_path,

            # --- MATERIAL COMMANDS ---
            "set_material_scalar_param": material_commands.handle_set_material_scalar_param,
            "set_material_vector_param": material_commands.handle_set_material_vector_param,
            "set_material_texture_param": material_commands.handle_set_material_texture_param,
            "get_material_info": material_commands.handle_get_material_info,
            "recompile_material": material_commands.handle_recompile_material,
            "set_actor_material": material_commands.handle_set_actor_material,
            "create_material_instance": material_commands.handle_create_material_instance,

            # --- SKELETAL MESH COMMANDS ---
            "get_skeleton_bones": skeletal_mesh_commands.handle_get_skeleton_bones,
            "get_animation_sequences": skeletal_mesh_commands.handle_get_animation_sequences,
            "set_actor_animation": skeletal_mesh_commands.handle_set_actor_animation,
            "play_animation": skeletal_mesh_commands.handle_play_animation,
            "stop_animation": skeletal_mesh_commands.handle_stop_animation,
            "get_actor_skeleton_info": skeletal_mesh_commands.handle_get_actor_skeleton_info,
            "set_animation_blueprint": skeletal_mesh_commands.handle_set_animation_blueprint,

            # --- NPC DIALOG COMMANDS ---
            "configure_npc_dialog": npc_dialog_commands.handle_configure_npc_dialog,
            "start_npc_dialog": npc_dialog_commands.handle_start_npc_dialog,
            "end_npc_dialog": npc_dialog_commands.handle_end_npc_dialog,
            "send_npc_message": npc_dialog_commands.handle_send_npc_message,
            "set_npc_mood": npc_dialog_commands.handle_set_npc_mood,
            "modify_npc_reputation": npc_dialog_commands.handle_modify_npc_reputation,
            "get_npc_dialog_state": npc_dialog_commands.handle_get_npc_dialog_state,
            "add_dialog_choice": npc_dialog_commands.handle_add_dialog_choice,
            "clear_dialog_choices": npc_dialog_commands.handle_clear_dialog_choices,
            "is_npc_server_running": npc_dialog_commands.handle_is_npc_server_running,
            "spawn_npc_on_ground": npc_dialog_commands.handle_spawn_npc_on_ground,
            "find_ground_position": npc_dialog_commands.handle_find_ground_position,
            "adjust_actor_to_ground": npc_dialog_commands.handle_adjust_actor_to_ground,
            "reset_npc_conversation": npc_dialog_commands.handle_reset_npc_conversation,

            # ============================================
            # GENUTILS: SEQUENCER COMMANDS
            # ============================================
            "create_sequence": sequencer_commands.handle_create_sequence,
            "open_sequence": sequencer_commands.handle_open_sequence,
            "list_sequences": sequencer_commands.handle_list_sequences,
            "get_sequence_info": sequencer_commands.handle_get_sequence_info,
            "delete_sequence": sequencer_commands.handle_delete_sequence,
            "add_actor_to_sequence": sequencer_commands.handle_add_actor_to_sequence,
            "add_camera_cut_track": sequencer_commands.handle_add_camera_cut_track,
            "add_transform_track": sequencer_commands.handle_add_transform_track,
            "add_animation_track": sequencer_commands.handle_add_animation_track,
            "add_audio_track": sequencer_commands.handle_add_audio_track,
            "add_event_track": sequencer_commands.handle_add_event_track,
            "list_tracks": sequencer_commands.handle_list_tracks,
            "add_transform_key": sequencer_commands.handle_add_transform_key,
            "add_animation_section": sequencer_commands.handle_add_animation_section,
            "add_audio_section": sequencer_commands.handle_add_audio_section,
            "spawn_sequence_camera": sequencer_commands.handle_spawn_sequence_camera,
            "add_camera_cut": sequencer_commands.handle_add_camera_cut,
            "add_camera_shake": sequencer_commands.handle_add_camera_shake,
            "play_sequence": sequencer_commands.handle_play_sequence,
            "stop_sequence": sequencer_commands.handle_stop_sequence,
            "set_playback_position": sequencer_commands.handle_set_playback_position,
            "set_playback_range": sequencer_commands.handle_set_playback_range,
            "render_sequence": sequencer_commands.handle_render_sequence,
            "export_sequence_to_fbx": sequencer_commands.handle_export_sequence_to_fbx,
            "add_subsequence": sequencer_commands.handle_add_subsequence,
            "list_subsequences": sequencer_commands.handle_list_subsequences,

            # ============================================
            # GENUTILS: DATATABLE COMMANDS
            # ============================================
            "list_data_tables": datatable_commands.handle_list_data_tables,
            "get_data_table_info": datatable_commands.handle_get_data_table_info,
            "get_all_rows": datatable_commands.handle_get_all_rows,
            "get_row": datatable_commands.handle_get_row,
            "search_rows": datatable_commands.handle_search_rows,
            "add_datatable_row": datatable_commands.handle_add_datatable_row,
            "update_datatable_row": datatable_commands.handle_update_datatable_row,
            "delete_datatable_row": datatable_commands.handle_delete_datatable_row,
            "rename_datatable_row": datatable_commands.handle_rename_datatable_row,
            "create_data_table": datatable_commands.handle_create_data_table,
            "duplicate_data_table": datatable_commands.handle_duplicate_data_table,
            "delete_data_table": datatable_commands.handle_delete_data_table,
            "export_datatable_to_json": datatable_commands.handle_export_datatable_to_json,
            "export_datatable_to_csv": datatable_commands.handle_export_datatable_to_csv,
            "import_datatable_from_json": datatable_commands.handle_import_datatable_from_json,
            "import_datatable_from_csv": datatable_commands.handle_import_datatable_from_csv,
            "list_row_structs": datatable_commands.handle_list_row_structs,
            "get_struct_properties": datatable_commands.handle_get_struct_properties,
            "list_curve_tables": datatable_commands.handle_list_curve_tables,
            "get_curve_data": datatable_commands.handle_get_curve_data,
            "set_curve_data": datatable_commands.handle_set_curve_data,
            "list_composite_tables": datatable_commands.handle_list_composite_tables,
            "get_parent_tables": datatable_commands.handle_get_parent_tables,

            # ============================================
            # GENUTILS: BEHAVIOR TREE / AI COMMANDS
            # ============================================
            "list_ai_controllers": behavior_tree_commands.handle_list_ai_controllers,
            "get_ai_controller_info": behavior_tree_commands.handle_get_ai_controller_info,
            "set_ai_controller": behavior_tree_commands.handle_set_ai_controller,
            "list_behavior_trees": behavior_tree_commands.handle_list_behavior_trees,
            "get_behavior_tree_info": behavior_tree_commands.handle_get_behavior_tree_info,
            "set_behavior_tree": behavior_tree_commands.handle_set_behavior_tree,
            "run_bt": behavior_tree_commands.handle_run_bt,
            "stop_bt": behavior_tree_commands.handle_stop_bt,
            "list_blackboards": behavior_tree_commands.handle_list_blackboards,
            "get_blackboard_keys": behavior_tree_commands.handle_get_blackboard_keys,
            "get_bb_value": behavior_tree_commands.handle_get_bb_value,
            "set_bb_value": behavior_tree_commands.handle_set_bb_value,
            "is_location_navigable": behavior_tree_commands.handle_is_location_navigable,
            "find_nav_path": behavior_tree_commands.handle_find_nav_path,
            "get_random_navigable_point": behavior_tree_commands.handle_get_random_navigable_point,
            "project_point_to_navigation": behavior_tree_commands.handle_project_point_to_navigation,
            "ai_move_to_location": behavior_tree_commands.handle_ai_move_to_location,
            "ai_move_to_actor": behavior_tree_commands.handle_ai_move_to_actor,
            "stop_ai_movement": behavior_tree_commands.handle_stop_ai_movement,
            "get_movement_status": behavior_tree_commands.handle_get_movement_status,
            "get_perception_info": behavior_tree_commands.handle_get_perception_info,
            "get_perceived_actors": behavior_tree_commands.handle_get_perceived_actors,
            "configure_sight_sense": behavior_tree_commands.handle_configure_sight_sense,
            "configure_hearing_sense": behavior_tree_commands.handle_configure_hearing_sense,
            "list_eqs_queries": behavior_tree_commands.handle_list_eqs_queries,
            "run_eqs_query": behavior_tree_commands.handle_run_eqs_query,
            "list_smart_objects": behavior_tree_commands.handle_list_smart_objects,
            "find_smart_objects": behavior_tree_commands.handle_find_smart_objects,
            "get_avoidance_info": behavior_tree_commands.handle_get_avoidance_info,
            "set_avoidance_settings": behavior_tree_commands.handle_set_avoidance_settings,

            # ============================================
            # GENUTILS: DIALOGUE UTILS COMMANDS
            # ============================================
            "gu_list_dialog_actors": dialogue_utils_commands.handle_list_dialog_actors,
            "gu_get_dialog_config": dialogue_utils_commands.handle_get_dialog_config,
            "gu_set_dialog_config": dialogue_utils_commands.handle_set_dialog_config,
            "gu_add_dialog_component": dialogue_utils_commands.handle_add_dialog_component,
            "gu_remove_dialog_component": dialogue_utils_commands.handle_remove_dialog_component,
            "gu_set_greeting": dialogue_utils_commands.handle_set_greeting,
            "gu_set_farewell": dialogue_utils_commands.handle_set_farewell,
            "gu_get_quick_options": dialogue_utils_commands.handle_get_quick_options,
            "gu_add_quick_option": dialogue_utils_commands.handle_add_quick_option,
            "gu_clear_quick_options": dialogue_utils_commands.handle_clear_quick_options,
            "gu_set_trade_config": dialogue_utils_commands.handle_set_trade_config,
            "gu_set_mood": dialogue_utils_commands.handle_set_npc_mood_genutils,
            "gu_set_reputation": dialogue_utils_commands.handle_set_npc_reputation,
            "gu_set_portrait": dialogue_utils_commands.handle_set_npc_portrait,
            "gu_set_faction": dialogue_utils_commands.handle_set_npc_faction,
            "gu_start_dialog": dialogue_utils_commands.handle_start_dialog_genutils,
            "gu_end_dialog": dialogue_utils_commands.handle_end_dialog_genutils,
            "gu_send_message": dialogue_utils_commands.handle_send_message_genutils,
            "gu_export_all_dialog_configs": dialogue_utils_commands.handle_export_all_dialog_configs,
            "gu_list_moods": dialogue_utils_commands.handle_list_moods,
            "gu_list_choice_types": dialogue_utils_commands.handle_list_choice_types,

            # ============================================
            # GENUTILS: QUEST COMMANDS
            # ============================================
            # Creation
            "quest_create": quest_commands.handle_create_quest,
            "quest_create_from_json": quest_commands.handle_create_quest_from_json,
            "quest_create_batch": quest_commands.handle_create_quests_from_json,
            "quest_batch_create": quest_commands.handle_batch_create_quests,

            # Objectives
            "quest_add_objective": quest_commands.handle_add_objective,
            "quest_make_kill_objective": quest_commands.handle_make_kill_objective,
            "quest_make_collect_objective": quest_commands.handle_make_collect_objective,
            "quest_make_talk_objective": quest_commands.handle_make_talk_objective,
            "quest_make_explore_objective": quest_commands.handle_make_explore_objective,
            "quest_make_deliver_objective": quest_commands.handle_make_deliver_objective,

            # Chains
            "quest_create_chain": quest_commands.handle_create_quest_chain,
            "quest_link_as_chain": quest_commands.handle_link_quests_as_chain,

            # Export/Import
            "quest_export_to_json": quest_commands.handle_export_quest_to_json,
            "quest_save_to_file": quest_commands.handle_save_quests_to_file,
            "quest_load_from_file": quest_commands.handle_load_quests_from_file,

            # Validation
            "quest_validate": quest_commands.handle_validate_quest,
            "quest_validate_chain": quest_commands.handle_validate_quest_chain,

            # Templates
            "quest_get_templates": quest_commands.handle_get_quest_templates,
            "quest_create_from_template": quest_commands.handle_create_quest_from_template,

            # Utilities
            "quest_generate_id": quest_commands.handle_generate_quest_id,
            "quest_duplicate": quest_commands.handle_duplicate_quest,
            "quest_scale_rewards": quest_commands.handle_scale_rewards,
            "quest_scale_objective_counts": quest_commands.handle_scale_objective_counts,
        }

    def dispatch(self, command: Dict[str, Any]) -> Dict[str, Any]:
        """Dispatch command to appropriate handler"""
        command_type = command.get("type")
        if command_type not in self.handlers:
            return {"success": False, "error": f"Unknown command type: {command_type}"}

        try:
            handler = self.handlers[command_type]
            return handler(command)
        except Exception as e:
            log.log_error(f"Error processing command: {str(e)}")
            return {"success": False, "error": str(e)}

    def _handle_handshake(self, command: Dict[str, Any]) -> Dict[str, Any]:
        """Built-in handler for handshake command"""
        message = command.get("message", "")
        log.log_info(f"Handshake received: {message}")
        
        # Get Unreal Engine version
        engine_version = unreal.SystemLibrary.get_engine_version()
        
        # Add connection and session information
        connection_info = {
            "status": "Connected",
            "engine_version": engine_version,
            "timestamp": time.time(),
            "session_id": f"UE-{int(time.time())}"
        }
        
        return {
            "success": True, 
            "message": f"Received: {message}",
            "connection_info": connection_info
        }


# Create global dispatcher instance
dispatcher = CommandDispatcher()


def process_commands(delta_time=None):
    """Process commands on the main thread"""
    if not command_queue:
        return

    command_id, command = command_queue.pop(0)
    log.log_info(f"Processing command on main thread: {command}")

    try:
        response = dispatcher.dispatch(command)
        response_dict[command_id] = response
    except Exception as e:
        log.log_error(f"Error processing command: {str(e)}", include_traceback=True)
        response_dict[command_id] = {"success": False, "error": str(e)}


def receive_all_data(conn, buffer_size=4096):
    """
    Receive all data from socket until complete JSON is received
    
    Args:
        conn: Socket connection
        buffer_size: Initial buffer size for receiving data
        
    Returns:
        Decoded complete data
    """
    data = b""
    while True:
        try:
            # Receive chunk of data
            chunk = conn.recv(buffer_size)
            if not chunk:
                break
            
            data += chunk
            
            # Try to parse as JSON to check if we received complete data
            try:
                json.loads(data.decode('utf-8'))
                # If we get here, JSON is valid and complete
                return data.decode('utf-8')
            except json.JSONDecodeError as json_err:
                # Check if the error indicates an unterminated string or incomplete JSON
                if "Unterminated string" in str(json_err) or "Expecting" in str(json_err):
                    # Need more data, continue receiving
                    continue
                else:
                    # JSON is malformed in some other way, not just incomplete
                    log.log_error(f"Malformed JSON received: {str(json_err)}", include_traceback=True)
                    return None
            
        except socket.timeout:
            # Socket timeout, return what we have so far
            log.log_warning("Socket timeout while receiving data")
            return data.decode('utf-8')
        except Exception as e:
            log.log_error(f"Error receiving data: {str(e)}", include_traceback=True)
            return None
    
    return data.decode('utf-8')


def socket_server_thread():
    """Socket server running in a separate thread"""
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('localhost', 9877))
    server_socket.listen(1)
    log.log_info("Unreal Engine socket server started on port 9877")

    command_counter = 0

    while True:
        try:
            conn, addr = server_socket.accept()
            # Set a timeout to prevent hanging
            conn.settimeout(5)  # 5-second timeout
            
            # Receive complete data, handling potential incomplete JSON
            data_str = receive_all_data(conn)
            
            if data_str:
                try:
                    command = json.loads(data_str)
                    log.log_info(f"Received command: {command}")

                    # For handshake, we can respond directly from the thread
                    if command.get("type") == "handshake":
                        response = dispatcher.dispatch(command)
                        conn.sendall(json.dumps(response).encode())
                    else:
                        # For other commands, queue them for main thread execution
                        command_id = command_counter
                        command_counter += 1
                        command_queue.append((command_id, command))

                        # Wait for the response with a timeout
                        timeout = 10  # seconds
                        start_time = time.time()
                        while command_id not in response_dict and time.time() - start_time < timeout:
                            time.sleep(0.1)

                        if command_id in response_dict:
                            response = response_dict.pop(command_id)
                            conn.sendall(json.dumps(response).encode())
                        else:
                            error_response = {"success": False, "error": "Command timed out"}
                            conn.sendall(json.dumps(error_response).encode())
                except json.JSONDecodeError as json_err:
                    log.log_error(f"Error parsing JSON: {str(json_err)}", include_traceback=True)
                    error_response = {"success": False, "error": f"Invalid JSON: {str(json_err)}"}
                    conn.sendall(json.dumps(error_response).encode())
            else:
                # No data or error receiving data
                error_response = {"success": False, "error": "No data received or error parsing data"}
                conn.sendall(json.dumps(error_response).encode())
                
            conn.close()
        except Exception as e:
            log.log_error(f"Error in socket server: {str(e)}", include_traceback=True)
            try:
                # Try to close the connection if it's still open
                conn.close()
            except:
                pass


# Register tick function to process commands on main thread
def register_command_processor():
    """Register the command processor with Unreal's tick system"""
    unreal.register_slate_post_tick_callback(process_commands)
    log.log_info("Command processor registered")


# Initialize the server
def initialize_server():
    """Initialize and start the socket server"""
    # Start the server thread
    thread = threading.Thread(target=socket_server_thread)
    thread.daemon = True
    thread.start()
    log.log_info("Socket server thread started")

    # Register the command processor on the main thread
    register_command_processor()

    log.log_info("Unreal Engine AI command server initialized successfully")
    log.log_info("Available commands:")
    log.log_info("  - Basic: handshake, spawn, create_material, modify_object")
    log.log_info("  - Blueprint: create_blueprint, add_component, add_variable, add_function, add_node, connect_nodes, compile_blueprint, spawn_blueprint, add_nodes_bulk, connect_nodes_bulk")

# Auto-start the server when this module is imported
initialize_server()

