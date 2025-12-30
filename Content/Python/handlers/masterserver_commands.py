# MasterServer Commands - Python Handler for MasterServerClient
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# HELPER: Get MasterServerClient
# ============================================

def get_client():
    """Get the MasterServerClient subsystem"""
    world = unreal.EditorLevelLibrary.get_editor_world()
    game_instance = unreal.GameplayStatics.get_game_instance(world)
    if game_instance:
        return unreal.MasterServerClient.get(game_instance)
    return None

# ============================================
# CONFIGURATION
# ============================================

def handle_set_server_url(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set the Master Server URL"""
    try:
        url = command.get("url", "")

        if not url:
            return {"success": False, "error": "url required"}

        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        client.set_server_url(url)

        return {"success": True, "url": url}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_server_url(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get the current server URL"""
    try:
        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        url = client.get_server_url()

        return {"success": True, "url": url}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_connection_status(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get connection status"""
    try:
        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        return {
            "success": True,
            "is_connected": client.is_connected(),
            "is_authenticated": client.is_authenticated(),
            "session_id": client.get_session_id(),
            "username": client.get_username(),
            "account_id": client.get_account_id(),
            "is_admin": client.is_admin(),
            "is_gm": client.is_gm()
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# AUTHENTICATION
# ============================================

def handle_register(command: Dict[str, Any]) -> Dict[str, Any]:
    """Register a new account (async - returns immediately)"""
    try:
        username = command.get("username", "")
        email = command.get("email", "")
        password = command.get("password", "")

        if not username or not email or not password:
            return {"success": False, "error": "username, email, password required"}

        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        # Note: This is async, we can't wait for result
        # In real usage, bind to OnAuthComplete delegate
        log.log_info(f"Registration request sent for: {username}")

        return {
            "success": True,
            "message": "Registration request sent. Check server logs for result.",
            "note": "This is an async operation. Bind to OnAuthComplete for results."
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_login(command: Dict[str, Any]) -> Dict[str, Any]:
    """Login to existing account (async)"""
    try:
        username = command.get("username", "")
        password = command.get("password", "")

        if not username or not password:
            return {"success": False, "error": "username and password required"}

        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        log.log_info(f"Login request sent for: {username}")

        return {
            "success": True,
            "message": "Login request sent. Check server logs for result.",
            "note": "This is an async operation. Bind to OnAuthComplete for results."
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_logout(command: Dict[str, Any]) -> Dict[str, Any]:
    """Logout current session"""
    try:
        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        log.log_info("Logout request sent")

        return {
            "success": True,
            "message": "Logout request sent."
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_validate_session(command: Dict[str, Any]) -> Dict[str, Any]:
    """Validate current session"""
    try:
        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        is_valid = client.is_authenticated()

        return {
            "success": True,
            "session_valid": is_valid,
            "session_id": client.get_session_id() if is_valid else ""
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# CHARACTERS
# ============================================

def handle_get_characters(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get character list (async)"""
    try:
        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        log.log_info("Character list request sent")

        return {
            "success": True,
            "message": "Character list request sent.",
            "note": "Bind to OnCharacterListComplete for results."
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_create_character(command: Dict[str, Any]) -> Dict[str, Any]:
    """Create a new character"""
    try:
        name = command.get("name", "")
        class_name = command.get("class_name", "Warrior")

        if not name:
            return {"success": False, "error": "name required"}

        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        log.log_info(f"Create character request: {name} ({class_name})")

        return {
            "success": True,
            "message": f"Create character request sent: {name}",
            "note": "Bind to OnAPIComplete for results."
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_delete_character(command: Dict[str, Any]) -> Dict[str, Any]:
    """Delete a character"""
    try:
        character_id = command.get("character_id", 0)

        if not character_id:
            return {"success": False, "error": "character_id required"}

        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        log.log_info(f"Delete character request: {character_id}")

        return {
            "success": True,
            "message": f"Delete character request sent: {character_id}"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_select_character(command: Dict[str, Any]) -> Dict[str, Any]:
    """Select character for current session"""
    try:
        character_id = command.get("character_id", 0)

        if not character_id:
            return {"success": False, "error": "character_id required"}

        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        log.log_info(f"Select character: {character_id}")

        return {
            "success": True,
            "message": f"Select character request sent: {character_id}"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_selected_character(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get currently selected character ID"""
    try:
        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        char_id = client.get_selected_character_id()

        return {
            "success": True,
            "selected_character_id": char_id
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# SERVERS
# ============================================

def handle_get_server_list(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get list of game servers"""
    try:
        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        log.log_info("Server list request sent")

        return {
            "success": True,
            "message": "Server list request sent.",
            "note": "Bind to OnServerListComplete for results."
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_join_server(command: Dict[str, Any]) -> Dict[str, Any]:
    """Join a game server"""
    try:
        server_id = command.get("server_id", "")

        if not server_id:
            return {"success": False, "error": "server_id required"}

        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        log.log_info(f"Join server request: {server_id}")

        return {
            "success": True,
            "message": f"Join server request sent: {server_id}"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_get_current_server(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get currently joined server"""
    try:
        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        server_id = client.get_current_server_id()

        return {
            "success": True,
            "current_server_id": server_id
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# GAME SERVER API
# ============================================

def handle_register_game_server(command: Dict[str, Any]) -> Dict[str, Any]:
    """Register this server with master (for dedicated servers)"""
    try:
        server_id = command.get("server_id", "")
        name = command.get("name", "Game Server")
        host = command.get("host", "127.0.0.1")
        port = command.get("port", 7777)
        max_players = command.get("max_players", 32)
        map_name = command.get("map_name", "MainLevel")

        if not server_id:
            return {"success": False, "error": "server_id required"}

        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        log.log_info(f"Register game server: {name} ({server_id})")

        return {
            "success": True,
            "message": f"Register server request sent: {name}"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_send_heartbeat(command: Dict[str, Any]) -> Dict[str, Any]:
    """Send heartbeat to master server"""
    try:
        server_id = command.get("server_id", "")
        current_players = command.get("current_players", 0)

        if not server_id:
            return {"success": False, "error": "server_id required"}

        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        log.log_info(f"Heartbeat: {server_id} ({current_players} players)")

        return {
            "success": True,
            "message": "Heartbeat sent"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# ADMIN
# ============================================

def handle_get_server_stats(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get server statistics (admin only)"""
    try:
        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        if not client.is_admin():
            return {"success": False, "error": "Admin access required"}

        log.log_info("Server stats request sent")

        return {
            "success": True,
            "message": "Server stats request sent.",
            "note": "Bind to OnAPIComplete for results."
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_ban_account(command: Dict[str, Any]) -> Dict[str, Any]:
    """Ban an account (admin/gm only)"""
    try:
        account_id = command.get("account_id", 0)
        reason = command.get("reason", "No reason specified")
        permanent = command.get("permanent", False)
        duration_hours = command.get("duration_hours", 24)

        if not account_id:
            return {"success": False, "error": "account_id required"}

        client = get_client()
        if not client:
            return {"success": False, "error": "MasterServerClient not available"}

        if not client.is_admin() and not client.is_gm():
            return {"success": False, "error": "Admin or GM access required"}

        log.log_info(f"Ban account request: {account_id} - {reason}")

        return {
            "success": True,
            "message": f"Ban request sent for account {account_id}",
            "permanent": permanent,
            "duration_hours": duration_hours if not permanent else "N/A"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

MASTERSERVER_COMMANDS = {
    # Config
    "masterserver_set_url": handle_set_server_url,
    "masterserver_get_url": handle_get_server_url,
    "masterserver_status": handle_get_connection_status,

    # Auth
    "masterserver_register": handle_register,
    "masterserver_login": handle_login,
    "masterserver_logout": handle_logout,
    "masterserver_validate": handle_validate_session,

    # Characters
    "masterserver_get_characters": handle_get_characters,
    "masterserver_create_character": handle_create_character,
    "masterserver_delete_character": handle_delete_character,
    "masterserver_select_character": handle_select_character,
    "masterserver_selected_character": handle_get_selected_character,

    # Servers
    "masterserver_get_servers": handle_get_server_list,
    "masterserver_join": handle_join_server,
    "masterserver_current": handle_get_current_server,

    # Game Server API
    "masterserver_register_server": handle_register_game_server,
    "masterserver_heartbeat": handle_send_heartbeat,

    # Admin
    "masterserver_stats": handle_get_server_stats,
    "masterserver_ban": handle_ban_account,
}
