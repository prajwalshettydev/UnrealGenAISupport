"""
NPC AI Server for Project Titan
Verwaltet NPC-Konversationen mit AI-gestützten Dialogen
Lädt Persönlichkeiten aus npc_personalities.json
"""

import json
import socket
import threading
import os
from typing import Dict, Any, Optional
from pathlib import Path

# Server Konfiguration
NPC_SERVER_HOST = "localhost"
NPC_SERVER_PORT = 9879

# Pfad zur Persönlichkeits-Datei (relativ zum Content-Ordner)
PERSONALITIES_PATH = None

# Gespeicherte Konversationen pro NPC
conversations: Dict[str, list] = {}

# Geladene Persönlichkeiten
personalities: Dict[str, Dict] = {}
default_personality: Dict = {}

# AI Provider Konfiguration (kann angepasst werden)
AI_PROVIDER = "openai"  # oder "anthropic", "local"
AI_MODEL = "gpt-4o-mini"  # oder "claude-3-haiku", etc.


def find_personalities_file() -> Optional[str]:
    """Sucht die npc_personalities.json Datei"""
    # Mögliche Pfade
    possible_paths = [
        # Relativ zum Script
        Path(__file__).parent.parent.parent.parent / "Content" / "Data" / "NPC" / "npc_personalities.json",
        # Absoluter Pfad für Project Titan
        Path("C:/Unreal_entwicklung/ProjectTitan/Content/Data/NPC/npc_personalities.json"),
        # Aktuelles Verzeichnis
        Path("npc_personalities.json"),
    ]

    for path in possible_paths:
        if path.exists():
            return str(path)

    return None


def load_personalities():
    """Lädt die NPC-Persönlichkeiten aus der JSON-Datei"""
    global personalities, default_personality, PERSONALITIES_PATH

    PERSONALITIES_PATH = find_personalities_file()

    if not PERSONALITIES_PATH:
        print("[NPC Server] WARNUNG: npc_personalities.json nicht gefunden, verwende Standard-Persönlichkeiten")
        default_personality = {
            "name": "NPC",
            "system_prompt": "Du bist ein freundlicher NPC in einem Fantasy-RPG.",
            "greeting": "Hallo, Abenteurer!"
        }
        return

    try:
        with open(PERSONALITIES_PATH, 'r', encoding='utf-8') as f:
            data = json.load(f)

        personalities = data.get("personalities", {})
        default_personality = data.get("default_personality", {})

        print(f"[NPC Server] {len(personalities)} Persönlichkeiten geladen von {PERSONALITIES_PATH}")
        for name in personalities.keys():
            print(f"  - {name}: {personalities[name].get('name', 'Unbekannt')}")

    except Exception as e:
        print(f"[NPC Server] Fehler beim Laden der Persönlichkeiten: {e}")
        default_personality = {
            "name": "NPC",
            "system_prompt": "Du bist ein freundlicher NPC.",
            "greeting": "Hallo!"
        }


def get_personality(personality_id: str) -> Dict:
    """Gibt die Persönlichkeit für eine ID zurück"""
    if personality_id in personalities:
        return personalities[personality_id]
    return default_personality


def build_system_prompt(personality: Dict) -> str:
    """Erstellt den System-Prompt für die AI"""
    base_prompt = personality.get("system_prompt", "Du bist ein freundlicher NPC.")
    name = personality.get("name", "NPC")
    traits = personality.get("traits", [])
    knowledge = personality.get("knowledge", [])
    mood = personality.get("mood", "neutral")
    speech_style = personality.get("speech_style", "normal")

    prompt = f"""Du bist {name}. {base_prompt}

Wichtige Regeln:
- Bleibe IMMER in deiner Rolle als {name}
- Antworte auf Deutsch
- Halte Antworten kurz (2-4 Sätze maximum)
- Du bist ein NPC in einem Fantasy-RPG Spiel
- Du weißt NICHTS über die echte Welt, nur über Valdoria
- Wenn jemand nach Dingen fragt die du nicht weißt, sage das ehrlich

Deine Eigenschaften: {', '.join(traits)}
Du kennst dich aus mit: {', '.join(knowledge)}
Aktuelle Stimmung: {mood}
Sprechstil: {speech_style}
"""
    return prompt


def generate_ai_response(npc_id: str, personality_id: str, player_message: str) -> str:
    """
    Generiert eine AI-Antwort basierend auf der NPC-Persönlichkeit.
    Verwendet das konfigurierte AI-Backend.
    """
    personality = get_personality(personality_id)

    # Konversationshistorie abrufen oder erstellen
    if npc_id not in conversations:
        conversations[npc_id] = []
        # Erste Nachricht = Greeting
        greeting = personality.get("greeting", "Hallo, Abenteurer!")
        conversations[npc_id].append({"role": "assistant", "content": greeting})

    # System-Prompt erstellen
    system_prompt = build_system_prompt(personality)

    # Spieler-Nachricht zur Historie hinzufügen
    conversations[npc_id].append({"role": "user", "content": player_message})

    # Konversation auf letzte 10 Nachrichten begrenzen
    recent_history = conversations[npc_id][-10:]

    # AI-Antwort generieren
    try:
        response = call_ai_backend(system_prompt, recent_history)

        # Antwort zur Historie hinzufügen
        conversations[npc_id].append({"role": "assistant", "content": response})

        return response

    except Exception as e:
        print(f"[NPC Server] AI-Fehler: {e}")
        # Fallback-Antworten basierend auf Persönlichkeit
        return get_fallback_response(personality)


def call_ai_backend(system_prompt: str, messages: list) -> str:
    """
    Ruft das AI-Backend auf.
    Kann OpenAI, Anthropic oder ein lokales Modell sein.
    """
    # Versuche OpenAI
    try:
        import openai

        formatted_messages = [{"role": "system", "content": system_prompt}]
        formatted_messages.extend(messages)

        response = openai.chat.completions.create(
            model=AI_MODEL,
            messages=formatted_messages,
            max_tokens=150,
            temperature=0.8
        )

        return response.choices[0].message.content

    except ImportError:
        pass
    except Exception as e:
        print(f"[NPC Server] OpenAI-Fehler: {e}")

    # Versuche Anthropic
    try:
        import anthropic

        client = anthropic.Anthropic()

        response = client.messages.create(
            model="claude-3-haiku-20240307",
            max_tokens=150,
            system=system_prompt,
            messages=messages
        )

        return response.content[0].text

    except ImportError:
        pass
    except Exception as e:
        print(f"[NPC Server] Anthropic-Fehler: {e}")

    # Fallback: Einfache regel-basierte Antworten
    return generate_simple_response(messages[-1]["content"] if messages else "")


def generate_simple_response(player_message: str) -> str:
    """Generiert eine einfache Antwort ohne AI (Fallback)"""
    message_lower = player_message.lower()

    if any(word in message_lower for word in ["hallo", "hi", "grüß", "tag"]):
        return "Sei gegrüßt, Abenteurer!"
    elif any(word in message_lower for word in ["quest", "auftrag", "aufgabe"]):
        return "Sprich mit dem Gildenmeister in der Gildenhalle. Er hat immer Aufträge für tüchtige Abenteurer."
    elif any(word in message_lower for word in ["hilfe", "helfen"]):
        return "Ich helfe dir gerne. Was möchtest du wissen?"
    elif any(word in message_lower for word in ["tschüss", "bye", "gehst", "wiedersehen"]):
        return "Auf Wiedersehen! Mögen die Götter über dich wachen."
    elif any(word in message_lower for word in ["wo", "weg", "richtung", "finden"]):
        return "Die Taverne ist im Zentrum des Dorfes. Die Gildenhalle liegt nördlich davon."
    elif any(word in message_lower for word in ["gefahr", "monster", "wolf", "bandit"]):
        return "Sei vorsichtig dort draußen. Die Wälder sind nicht mehr sicher wie früher."
    elif any(word in message_lower for word in ["danke", "dank"]):
        return "Gern geschehen, Abenteurer!"
    else:
        return "Hmm, darüber weiß ich leider nicht viel. Vielleicht kann dir jemand anderes helfen."


def get_fallback_response(personality: Dict) -> str:
    """Gibt eine Fallback-Antwort basierend auf Persönlichkeit"""
    name = personality.get("name", "NPC")
    traits = personality.get("traits", [])

    if "grumpy" in traits:
        return "Hmpf. Ich hab gerade keine Zeit für Geschwätz."
    elif "wise" in traits:
        return "Diese Frage bedarf tieferen Nachdenkens. Komm später wieder."
    elif "friendly" in traits or "warm" in traits:
        return "Entschuldige, ich war gerade in Gedanken. Was sagtest du?"
    elif "mysterious" in traits:
        return "Manche Antworten findet man nicht in Worten..."
    else:
        return "Hmm, ich verstehe nicht ganz was du meinst."


def reset_conversation(npc_id: str) -> bool:
    """Setzt die Konversation für einen NPC zurück"""
    if npc_id in conversations:
        del conversations[npc_id]
        print(f"[NPC Server] Konversation für {npc_id} zurückgesetzt")
        return True
    return False


def handle_client(client_socket: socket.socket, address):
    """Verarbeitet eine Client-Verbindung"""
    try:
        # Daten empfangen
        data = client_socket.recv(8192)
        if not data:
            return

        # JSON parsen
        try:
            request = json.loads(data.decode('utf-8'))
        except json.JSONDecodeError as e:
            response = {"type": "error", "error": f"Invalid JSON: {e}"}
            client_socket.send(json.dumps(response).encode('utf-8'))
            return

        request_type = request.get("type", "")

        if request_type == "chat":
            npc_id = request.get("npc_id", "unknown")
            personality = request.get("personality", "default")
            message = request.get("message", "")

            print(f"[NPC Server] Chat von {address}: [{personality}] {message[:50]}...")

            # AI-Antwort generieren
            npc_response = generate_ai_response(npc_id, personality, message)

            response = {
                "type": "response",
                "npc_id": npc_id,
                "message": npc_response
            }

        elif request_type == "reset":
            npc_id = request.get("npc_id", "")
            success = reset_conversation(npc_id)
            response = {
                "type": "reset_response",
                "npc_id": npc_id,
                "success": success
            }

        elif request_type == "greeting":
            personality_id = request.get("personality", "default")
            personality = get_personality(personality_id)
            greeting = personality.get("greeting", "Hallo!")
            response = {
                "type": "greeting_response",
                "message": greeting
            }

        elif request_type == "list_personalities":
            personality_list = {k: v.get("name", k) for k, v in personalities.items()}
            response = {
                "type": "personality_list",
                "personalities": personality_list
            }

        elif request_type == "reload":
            load_personalities()
            response = {
                "type": "reload_response",
                "success": True,
                "count": len(personalities)
            }

        else:
            response = {"type": "error", "error": f"Unknown request type: {request_type}"}

        # Antwort senden
        client_socket.send(json.dumps(response).encode('utf-8'))

    except Exception as e:
        print(f"[NPC Server] Fehler bei Client {address}: {e}")
        try:
            error_response = {"type": "error", "error": str(e)}
            client_socket.send(json.dumps(error_response).encode('utf-8'))
        except:
            pass

    finally:
        client_socket.close()


def start_server():
    """Startet den NPC AI Server"""
    # Persönlichkeiten laden
    load_personalities()

    # Server erstellen
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server.bind((NPC_SERVER_HOST, NPC_SERVER_PORT))
        server.listen(5)
        print(f"[NPC Server] Gestartet auf {NPC_SERVER_HOST}:{NPC_SERVER_PORT}")
        print(f"[NPC Server] Warte auf Verbindungen...")

        while True:
            client_socket, address = server.accept()
            print(f"[NPC Server] Verbindung von {address}")

            # Client in separatem Thread behandeln
            client_thread = threading.Thread(
                target=handle_client,
                args=(client_socket, address)
            )
            client_thread.daemon = True
            client_thread.start()

    except KeyboardInterrupt:
        print("\n[NPC Server] Beende...")
    except Exception as e:
        print(f"[NPC Server] Fehler: {e}")
    finally:
        server.close()


if __name__ == "__main__":
    print("=" * 50)
    print("  Project Titan - NPC AI Server")
    print("=" * 50)
    start_server()
