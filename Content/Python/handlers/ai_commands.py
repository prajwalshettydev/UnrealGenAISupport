"""
AI/NPC Chat Commands
Verbindet Unreal NPCs mit dem AI Server
"""

import json
import socket
from typing import Dict, Any
from utils import logging as log

# AI Server Konfiguration - TCP Port (nicht WebSocket)
AI_SERVER_HOST = "192.168.178.73"
AI_SERVER_PORT = 9879  # Separater TCP Port für Unreal


def send_to_ai_server(message: Dict[str, Any], timeout: float = 30.0) -> Dict[str, Any]:
    """
    Sendet eine Nachricht an den AI Server über einfachen TCP Socket
    """
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(timeout)
            s.connect((AI_SERVER_HOST, AI_SERVER_PORT))

            # JSON senden
            s.sendall(json.dumps(message).encode('utf-8'))

            # Antwort empfangen
            data = b''
            while True:
                chunk = s.recv(8192)
                if not chunk:
                    break
                data += chunk
                # Versuche JSON zu parsen
                try:
                    return json.loads(data.decode('utf-8'))
                except json.JSONDecodeError:
                    continue

            if data:
                return json.loads(data.decode('utf-8'))
            return {"success": False, "error": "Keine Antwort vom Server"}

    except socket.timeout:
        log.log_error("AI Server Timeout")
        return {"success": False, "error": "Timeout - AI Server antwortet nicht"}
    except ConnectionRefusedError:
        log.log_error("AI Server nicht erreichbar")
        return {"success": False, "error": "AI Server nicht gestartet (Port 9879)"}
    except Exception as e:
        log.log_error(f"AI Server Fehler: {e}")
        return {"success": False, "error": str(e)}


def handle_npc_chat(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Lässt einen NPC mit dem AI Server chatten

    Args:
        command: Dictionary mit:
            - npc_id: Eindeutige ID des NPCs
            - personality: Persönlichkeitstyp (default, merchant, guard, wise_penguin)
            - message: Spieler-Nachricht an den NPC

    Returns:
        Dictionary mit:
            - success: True/False
            - response: NPC Antwort
    """
    try:
        npc_id = command.get("npc_id", "unknown_npc")
        personality = command.get("personality", "default")
        player_message = command.get("message", "")

        log.log_info(f"NPC Chat: [{npc_id}] Spieler sagt: {player_message}")

        # An AI Server senden
        ai_response = send_to_ai_server({
            "type": "chat",
            "npc_id": npc_id,
            "personality": personality,
            "message": player_message
        })

        if ai_response.get("type") == "response":
            npc_message = ai_response.get("message", "...")
            log.log_info(f"NPC Chat: [{npc_id}] NPC antwortet: {npc_message}")
            return {
                "success": True,
                "npc_id": npc_id,
                "response": npc_message
            }
        else:
            return {
                "success": False,
                "error": ai_response.get("error", "Unbekannter Fehler")
            }

    except Exception as e:
        log.log_error(f"NPC Chat Fehler: {e}")
        return {"success": False, "error": str(e)}


def handle_npc_reset(command: Dict[str, Any]) -> Dict[str, Any]:
    """
    Setzt die Konversation eines NPCs zurück
    """
    try:
        npc_id = command.get("npc_id")

        if not npc_id:
            return {"success": False, "error": "npc_id fehlt"}

        ai_response = send_to_ai_server({
            "type": "reset",
            "npc_id": npc_id
        })

        return {"success": True, "npc_id": npc_id}

    except Exception as e:
        return {"success": False, "error": str(e)}
