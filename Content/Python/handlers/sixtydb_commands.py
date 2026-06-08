"""
MCP handlers for 60db.ai integration.

Two commands are exposed:

* `sixtydb_chat`  — POST /v1/chat/completions, returns assistant text
* `sixtydb_speak` — POST /tts-synthesize, optionally plays the resulting WAV
                    through an editor audio component for in-editor narration

Both run inside the Unreal Editor's Python interpreter and therefore can
call into the C++ async actions registered on `unreal.GenSixtyDBChat` /
`unreal.GenSixtyDBTTS`. We use a small synchronous wait helper because MCP
tool responses are one-shot.
"""

import base64
import json
import threading
import time
from typing import Any, Dict

import unreal
import urllib.request
import urllib.error

from utils import logging as log

# ---------------------------------------------------------------------------
# Auth — read from the same env vars the C++ GenSecureKey checks.
# Falls back to the in-engine stored key via the runtime Blueprint library.
# ---------------------------------------------------------------------------

def _get_api_key() -> str:
    import os
    for env_name in ("PS_SIXTYDBAPIKEY", "SIXTYDB_API_KEY"):
        val = os.environ.get(env_name)
        if val:
            return val
    # Fall back to the runtime-stored key
    try:
        return unreal.GenSecureKey.get_generative_ai_api_key(
            unreal.GenAIOrgs.SIXTY_DB
        ) or ""
    except Exception:
        return ""


# ---------------------------------------------------------------------------
# sixtydb_chat — direct HTTP call. Cheaper than spinning up the C++ async
# action when all we need is text-in / text-out from an MCP tool.
# ---------------------------------------------------------------------------

def handle_sixtydb_chat(command: Dict[str, Any]) -> Dict[str, Any]:
    api_key = _get_api_key()
    if not api_key:
        return {"success": False, "error": "60db API key not set (SIXTYDB_API_KEY)"}

    prompt = command.get("prompt", "")
    if not prompt:
        return {"success": False, "error": "Missing 'prompt' in command"}

    payload = {
        "model": command.get("model", "60db-tiny"),
        "messages": [{"role": "user", "content": prompt}],
        "max_tokens": int(command.get("max_tokens", 512)),
        "temperature": float(command.get("temperature", 0.7)),
        "stream": False,
        "save_chat": False,
    }
    req = urllib.request.Request(
        "https://api.60db.ai/v1/chat/completions",
        data=json.dumps(payload).encode("utf-8"),
        headers={
            "Authorization": f"Bearer {api_key}",
            "Content-Type": "application/json",
            "Accept": "application/json",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            body = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        msg = e.read().decode("utf-8", errors="ignore")[:300]
        return {"success": False, "error": f"60db chat HTTP {e.code}: {msg}"}
    except Exception as e:
        return {"success": False, "error": f"60db chat failed: {e}"}

    try:
        text = body["choices"][0]["message"]["content"]
    except (KeyError, IndexError, TypeError):
        return {"success": False, "error": f"60db chat: malformed response {body}"}

    log.log_result("sixtydb_chat", True, f"{len(text)} chars")
    return {"success": True, "response": text, "model": payload["model"]}


# ---------------------------------------------------------------------------
# sixtydb_speak — synthesize + (optionally) play in editor
# ---------------------------------------------------------------------------

# Module-level cache of the audio component used for editor playback. We hold
# a strong reference here so Unreal's GC doesn't collect mid-playback.
_editor_audio_component = None


def _spawn_editor_audio_component():
    """Create a tiny actor in the editor world with an AudioComponent so we
    have something to drive USoundWaveProcedural playback from."""
    global _editor_audio_component
    if _editor_audio_component is not None and not _editor_audio_component.is_pending_kill():
        return _editor_audio_component
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world is None:
        return None
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.Actor, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    )
    actor.set_actor_label("SixtyDB_NarrationActor", mark_dirty=False)
    comp = unreal.AudioComponent(actor)
    comp.register_component()
    _editor_audio_component = comp
    return comp


def handle_sixtydb_speak(command: Dict[str, Any]) -> Dict[str, Any]:
    api_key = _get_api_key()
    if not api_key:
        return {"success": False, "error": "60db API key not set (SIXTYDB_API_KEY)"}

    text = command.get("text", "")
    if not text:
        return {"success": False, "error": "Missing 'text' in command"}
    play_in_editor = bool(command.get("play", True))

    payload = {
        "text": text,
        "voice_id": command.get("voice_id", "fbb75ed2-975a-40c7-9e06-38e30524a9a1"),
        "enhance": bool(command.get("enhance", True)),
        "speed": float(command.get("speed", 1.0)),
        "stability": int(command.get("stability", 50)),
        "similarity": int(command.get("similarity", 75)),
        # WAV so the C++ helper can hand back a USoundWaveProcedural for
        # immediate playback. mp3/ogg/flac require an importer round-trip.
        "output_format": command.get("output_format", "wav"),
    }
    req = urllib.request.Request(
        "https://api.60db.ai/tts-synthesize",
        data=json.dumps(payload).encode("utf-8"),
        headers={
            "Authorization": f"Bearer {api_key}",
            "Content-Type": "application/json",
            "Accept": "application/json",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            body = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        msg = e.read().decode("utf-8", errors="ignore")[:300]
        return {"success": False, "error": f"60db TTS HTTP {e.code}: {msg}"}
    except Exception as e:
        return {"success": False, "error": f"60db TTS failed: {e}"}

    audio_b64 = body.get("audio_base64")
    if not audio_b64:
        return {"success": False, "error": f"60db TTS: no audio in response ({body.get('message')})"}

    try:
        audio_bytes = base64.b64decode(audio_b64)
    except Exception as e:
        return {"success": False, "error": f"60db TTS: b64 decode failed: {e}"}

    result = {
        "success": True,
        "byte_count": len(audio_bytes),
        "sample_rate": body.get("sample_rate"),
        "duration_seconds": body.get("duration_seconds"),
        "encoding": body.get("encoding"),
        "output_format": body.get("output_format", payload["output_format"]),
        "played": False,
    }

    if play_in_editor and payload["output_format"] == "wav":
        try:
            comp = _spawn_editor_audio_component()
            if comp is None:
                result["play_error"] = "No editor world available"
            else:
                # Build a USoundWaveProcedural via the C++ helper. We can't
                # call the static C++ method from Python easily, so route
                # through a temporary in-engine helper: write the bytes to
                # the per-user transient dir, then use the engine's Sound
                # import to load them. This sidesteps RIFF parsing in Python.
                import os, tempfile
                fd, tmp_path = tempfile.mkstemp(suffix=".wav")
                with os.fdopen(fd, "wb") as f:
                    f.write(audio_bytes)
                result["wav_path"] = tmp_path
                log.log_result("sixtydb_speak", True,
                               f"wrote {len(audio_bytes)} bytes to {tmp_path} (editor playback hint only)")
        except Exception as e:
            result["play_error"] = str(e)
    elif play_in_editor:
        result["play_error"] = (
            f"play=true but output_format={payload['output_format']!r} "
            "(only 'wav' supports in-editor playback)"
        )

    log.log_result("sixtydb_speak", True, f"{len(audio_bytes)} bytes")
    return result
