# Unreal Engine Generative AI Support Plugin

[![Discord](https://img.shields.io/badge/Discord-7289DA?style=for-the-badge&logo=discord&logoColor=white)](https://discord.com/invite/KBWmkCKv5U)
[![License: MIT](https://img.shields.io/badge/License-MIT-007EC7?style=for-the-badge)](LICENSE)

## Anwendungsbeispiele:

#### MCP Beispiel:
Claude spawnt Szenenobjekte, steuert Transformationen und Materialien, generiert Blueprints, Funktionen, Variablen, fügt Komponenten hinzu, führt Python-Skripte aus etc.

<img src="Docs/UnrealMcpDemo.gif" width="480"/>

#### API Beispiel:
Ein Projekt namens "Become Human", in dem NPCs OpenAI-Agenten-Instanzen sind. Erstellt mit diesem Plugin.
![Become Human](Docs/BHDemoGif.gif)

---

# Über das Plugin

Jeden Monat werden hunderte neue KI-Modelle von verschiedenen Organisationen veröffentlicht, was es schwer macht, mit den neuesten Entwicklungen Schritt zu halten.

Das **"Unreal Engine Generative AI Support Plugin"** ermöglicht es dir, dich auf die Spielentwicklung zu konzentrieren, ohne dir über die LLM/GenAI-Integrationsschicht Gedanken machen zu müssen.

<p align="center"><img src="Docs/Repo Card - Updated.png" width="512"/></p>

Dieses Projekt zielt darauf ab, ein Long-Term-Support (LTS) Plugin für verschiedene hochmoderne LLM/GenAI-Modelle zu entwickeln und eine Community darum aufzubauen.

**Aktuell unterstützt:**
- OpenAI GPT-4o, GPT-4.5, o3-mini
- Anthropic Claude Sonnet 4, Claude Opus 4
- DeepSeek R1, DeepSeek-V3
- XAI Grok 3
- Unreal Engine 5.1 oder höher

---

## Inhaltsverzeichnis

- [API-Schlüssel einrichten](#api-schlüssel-einrichten)
- [MCP einrichten](#mcp-einrichten)
- [Plugin zum Projekt hinzufügen](#plugin-zum-projekt-hinzufügen)
- [Verwendung](#verwendung)
  - [OpenAI API](#openai-api)
  - [Anthropic Claude API](#anthropic-claude-api)
  - [DeepSeek API](#deepseek-api)
  - [XAI Grok API](#xai-grok-api)
- [Editor-Utilities (GenUtils)](#editor-utilities-genutils)
- [Model Control Protocol (MCP)](#model-control-protocol-mcp)
- [Bekannte Probleme](#bekannte-probleme)
- [Beitragen](#beitragen)

---

## API-Schlüssel einrichten

> **Hinweis:** Für MCP-Features in der Claude-App ist kein API-Schlüssel erforderlich. Der Anthropic-Schlüssel wird nur für die Claude-API benötigt.

### Für den Editor:

Setze die Umgebungsvariable `PS_<ORGNAME>` auf deinen API-Schlüssel.

#### Windows:
```cmd
setx PS_OPENAIAPIKEY "dein-api-schlüssel"
setx PS_ANTHROPICAPIKEY "dein-api-schlüssel"
setx PS_DEEPSEEKAPIKEY "dein-api-schlüssel"
```

#### Linux/MacOS:
```bash
echo "export PS_OPENAIAPIKEY='dein-api-schlüssel'" >> ~/.zshrc
source ~/.zshrc
```

**Wichtig:** Starte den Editor und verbundene IDEs nach dem Setzen der Umgebungsvariablen neu!

### Für gepackte Builds:

> ⚠️ **Sicherheitshinweis:** Das Speichern von API-Schlüsseln in gepackten Builds ist ein Sicherheitsrisiko. Requests sollten immer über deinen eigenen Backend-Server geleitet werden.

Für Test-Builds kannst du `GenSecureKey::SetGenAIApiKeyRuntime` in C++ oder Blueprints aufrufen.

---

## MCP einrichten

> **Hinweis:** Wenn dein Projekt nur die LLM-APIs und nicht MCP verwendet, kannst du diesen Abschnitt überspringen.

> ⚠️ **Warnung:** Die MCP-Funktion erlaubt der Claude Desktop App, dein Unreal Engine Projekt direkt zu steuern. Verwende sie nur in einer kontrollierten Umgebung und sichere dein Projekt vorher!

### 1. Client installieren:
- [Claude Desktop App](https://claude.anthropic.com/)
- [Cursor IDE](https://www.cursor.com/)

### 2. MCP-Konfiguration einrichten:

#### Für Claude Desktop App:
Bearbeite `claude_desktop_config.json`:
```json
{
    "mcpServers": {
      "unreal-handshake": {
        "command": "python",
        "args": ["<projekt-pfad>/Plugins/GenerativeAISupport/Content/Python/mcp_server.py"],
        "env": {
          "UNREAL_HOST": "localhost",
          "UNREAL_PORT": "9877"
        }
      }
    }
}
```

#### Für Cursor IDE:
Erstelle `.cursor/mcp.json` im Projektverzeichnis mit dem gleichen Inhalt.

### 3. MCP CLI installieren:
```bash
pip install mcp[cli]
```

### 4. Python Plugin in Unreal aktivieren:
`Edit -> Plugins -> Python Editor Script Plugin`

### 5. [Optional] Autostart aktivieren:
In den Plugin-Einstellungen kann der MCP-Server beim Editor-Start automatisch gestartet werden.

---

## Plugin zum Projekt hinzufügen

### Mit Git:

```cmd
git submodule add https://github.com/prajwalshettydev/UnrealGenAISupport Plugins/GenerativeAISupport
```

Danach:
1. Projektdateien neu generieren (Rechtsklick auf .uproject → Generate Visual Studio project files)
2. Plugin im Editor aktivieren (Edit → Plugins)
3. Für C++ Projekte in Build.cs hinzufügen:
```cpp
PrivateDependencyModuleNames.AddRange(new string[] { "GenerativeAISupport" });
```

### Updates holen:
```cmd
cd Plugins/GenerativeAISupport
git pull origin main
```

---

## Verwendung

### OpenAI API

#### Chat (C++):
```cpp
FGenChatSettings ChatSettings;
ChatSettings.Model = TEXT("gpt-4o-mini");
ChatSettings.MaxTokens = 500;
ChatSettings.Messages.Add(FGenChatMessage{ TEXT("system"), TEXT("Du bist ein hilfreicher Assistent.") });

FOnChatCompletionResponse OnComplete = FOnChatCompletionResponse::CreateLambda(
    [](const FString& Response, const FString& ErrorMessage, bool bSuccess)
{
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Antwort: %s"), *Response);
    }
});

UGenOAIChat::SendChatRequest(ChatSettings, OnComplete);
```

### Anthropic Claude API

#### Chat (C++):
```cpp
FGenClaudeChatSettings ChatSettings;
ChatSettings.Model = EClaudeModels::Claude_3_7_Sonnet;
ChatSettings.MaxTokens = 4096;
ChatSettings.Messages.Add(FGenChatMessage{TEXT("system"), TEXT("Du bist ein hilfreicher Assistent.")});
ChatSettings.Messages.Add(FGenChatMessage{TEXT("user"), TEXT("Was ist die Hauptstadt von Deutschland?")});

UGenClaudeChat::SendChatRequest(
    ChatSettings,
    FOnClaudeChatCompletionResponse::CreateLambda(
        [](const FString& Response, const FString& ErrorMessage, bool bSuccess)
        {
            if (bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("Claude Antwort: %s"), *Response);
            }
        })
);
```

### DeepSeek API

> ⚠️ **Wichtig:** Bei Verwendung des R1 Reasoning-Modells sollten die HTTP-Timeouts erhöht werden:
> ```ini
> [HTTP]
> HttpConnectionTimeout=180
> HttpReceiveTimeout=180
> ```

#### Reasoning (C++):
```cpp
FGenDSeekChatSettings ReasoningSettings;
ReasoningSettings.Model = EDeepSeekModels::Reasoner;
ReasoningSettings.MaxTokens = 100;
ReasoningSettings.Messages.Add(FGenChatMessage{TEXT("system"), TEXT("Du bist ein hilfreicher Assistent.")});
ReasoningSettings.Messages.Add(FGenChatMessage{TEXT("user"), TEXT("9.11 und 9.8, welche Zahl ist größer?")});

UGenDSeekChat::SendChatRequest(ReasoningSettings, OnComplete);
```

---

## Editor-Utilities (GenUtils)

Das Plugin enthält umfangreiche Editor-Utility-Klassen für die Steuerung über Python/MCP.

### GenWidgetUtils - Widget/UI Manipulation
```python
import unreal
gwu = unreal.GenWidgetUtils()

# Widgets auflisten
gwu.list_widgets("/Game/UI/MyWidget")

# Properties lesen/setzen
gwu.get_widget_property("/Game/UI/MyWidget", "Button1", "Visibility")
gwu.edit_widget_property("/Game/UI/MyWidget", "Button1", "Visibility", "Visible")

# Widget-Editor sicher öffnen
gwu.safe_open_widget_editor("/Game/UI/MyWidget")
```

### GenActorUtils - Actor Manipulation
```python
import unreal
gau = unreal.GenActorUtils()

# Actors spawnen
gau.spawn_basic_shape("Cube", unreal.Vector(0,0,0), unreal.Rotator(0,0,0), unreal.Vector(1,1,1), "MyCube")
gau.spawn_static_mesh_actor("/Game/Meshes/Tree", unreal.Vector(100,0,0), unreal.Rotator(0,0,0), unreal.Vector(1,1,1), "MyTree")

# Actors verwalten
gau.list_all_actors("")  # Alle Actors
gau.get_actor_properties("MyCube")
gau.set_actor_position("MyCube", unreal.Vector(500,0,0))
gau.delete_actor("OldActor")

# Level speichern
gau.save_current_level()
```

### GenWorldUtils - World/Level Manipulation
```python
import unreal
gwu = unreal.GenWorldUtils()

# Beleuchtung
gwu.spawn_point_light(unreal.Vector(0,0,500), 5000, unreal.LinearColor(1,1,1,1), "MainLight")
gwu.spawn_directional_light(unreal.Rotator(-45,0,0), 10, "SunLight")

# Volumes
gwu.spawn_trigger_box(unreal.Vector(0,0,0), unreal.Vector(200,200,100), "MyTrigger")

# Kamera
gwu.set_editor_camera(unreal.Vector(0,0,1000), unreal.Rotator(-90,0,0))
gwu.focus_on_actor("MyActor")

# Utilities
gwu.take_screenshot("/Game/Screenshots/shot1")
gwu.build_lighting()
gwu.play_in_editor()
```

### GenAnimationUtils - Animation Control
```python
import unreal
gau = unreal.GenAnimationUtils()

# Animationen abspielen
gau.play_animation("MyCharacter", "/Game/Animations/Walk", True)
gau.play_montage("MyCharacter", "/Game/Montages/Attack", 1.0)

# AI Movement
gau.move_to_location("MyNPC", unreal.Vector(1000,0,0), 50)
gau.look_at_actor("MyNPC", "Player", 5)
gau.face_player("MyNPC", True)

# Ragdoll
gau.enable_ragdoll("MyCharacter")
gau.apply_ragdoll_impulse("MyCharacter", unreal.Vector(0,0,5000), "spine_01")
```

### GenDialogueUtils - NPC Dialog System ⭐ NEU
```python
import unreal
gdu = unreal.GenDialogueUtils()

# Dialog-Actors finden
gdu.list_dialog_actors()

# Dialog konfigurieren
gdu.set_dialog_config("MyNPC", "npc_001", "Ältester Bob", "Dorfältester")
gdu.set_greeting("MyNPC", "Willkommen, Reisender!")
gdu.set_farewell("MyNPC", "Mögen die Götter dich beschützen!")

# Quick Options
gdu.add_quick_option("MyNPC", "Handeln", "open_trade", 1)
gdu.add_quick_option("MyNPC", "Quests", "show_quests", 2)

# Handel & Stimmung
gdu.set_trade_config("MyNPC", True, "shop_001")
gdu.set_mood("MyNPC", "Happy")
gdu.set_reputation("MyNPC", 500)

# Runtime (nur PIE)
gdu.start_dialog("MyNPC")
gdu.send_message("MyNPC", "Hallo!")
gdu.end_dialog("MyNPC")

# Export
gdu.export_all_dialog_configs()
```

### GenSequencerUtils - Level Sequences ⭐ NEU
```python
import unreal
gsu = unreal.GenSequencerUtils()

# Sequenzen erstellen
gsu.create_sequence("/Game/Cinematics/Intro", 30.0)
gsu.open_sequence("/Game/Cinematics/Intro")

# Actors hinzufügen
gsu.add_actor_to_sequence("/Game/Cinematics/Intro", "MainCharacter")
gsu.add_transform_track("/Game/Cinematics/Intro", "MainCharacter")

# Keyframes setzen
gsu.add_transform_key("/Game/Cinematics/Intro", "MainCharacter", 0,
    unreal.Vector(0,0,0), unreal.Rotator(0,0,0), unreal.Vector(1,1,1))
gsu.add_transform_key("/Game/Cinematics/Intro", "MainCharacter", 60,
    unreal.Vector(500,0,0), unreal.Rotator(0,45,0), unreal.Vector(1,1,1))

# Kamera
gsu.spawn_sequence_camera("/Game/Cinematics/Intro", "Camera1",
    unreal.Vector(-500,0,200), unreal.Rotator(-10,0,0))
gsu.add_camera_cut_track("/Game/Cinematics/Intro")
gsu.add_camera_cut("/Game/Cinematics/Intro", "Camera1", 0)

# Audio
gsu.add_audio_track("/Game/Cinematics/Intro")
gsu.add_audio_section("/Game/Cinematics/Intro", "/Game/Audio/Music", 0)

# Tracks auflisten
gsu.list_tracks("/Game/Cinematics/Intro")
```

### GenDataTableUtils - DataTable Verwaltung ⭐ NEU
```python
import unreal
gdtu = unreal.GenDataTableUtils()

# DataTables finden
gdtu.list_data_tables("/Game/Data")
gdtu.get_data_table_info("/Game/Data/Items")

# Rows lesen
gdtu.get_all_rows("/Game/Data/Items")
gdtu.get_row("/Game/Data/Items", "Sword_01")
gdtu.search_rows("/Game/Data/Items", "Name", "Schwert")

# Rows bearbeiten
gdtu.add_row("/Game/Data/Items", "NewSword", '{"Name": "Magisches Schwert", "Damage": 50}')
gdtu.update_row("/Game/Data/Items", "Sword_01", '{"Damage": 75}')
gdtu.delete_row("/Game/Data/Items", "OldItem")
gdtu.rename_row("/Game/Data/Items", "Sword_01", "LegendarySword")

# Export/Import
gdtu.export_to_json("/Game/Data/Items", "C:/Export/items.json")
gdtu.export_to_csv("/Game/Data/Items", "C:/Export/items.csv")
gdtu.import_from_csv("/Game/Data/Items", "C:/Import/items.csv", True)

# Struct-Info
gdtu.list_row_structs("Item")
gdtu.get_struct_properties("/Script/MyGame.FItemRow")
```

### GenAIUtils - KI/Behavior Tree Steuerung ⭐ NEU
```python
import unreal
gaiu = unreal.GenAIUtils()

# AI Controller
gaiu.list_ai_controllers()
gaiu.get_ai_controller_info("MyNPC")

# Behavior Trees
gaiu.list_behavior_trees("/Game/AI")
gaiu.get_behavior_tree_info("/Game/AI/BT_Patrol")
gaiu.run_behavior_tree("MyNPC", "/Game/AI/BT_Patrol")  # PIE
gaiu.stop_behavior_tree("MyNPC")

# Blackboard
gaiu.list_blackboards("/Game/AI")
gaiu.get_blackboard_keys("/Game/AI/BB_Default")
gaiu.get_blackboard_value("MyNPC", "TargetActor")  # PIE
gaiu.set_blackboard_value("MyNPC", "PatrolIndex", "2")

# Navigation
gaiu.is_location_navigable(unreal.Vector(0,0,0))
gaiu.find_path(unreal.Vector(0,0,0), unreal.Vector(1000,1000,0))
gaiu.get_random_navigable_point(unreal.Vector(0,0,0), 500)

# AI Movement (PIE)
gaiu.move_to_location("MyNPC", unreal.Vector(1000,0,0), 50)
gaiu.move_to_actor("MyNPC", "Player", 100)
gaiu.stop_movement("MyNPC")
gaiu.get_movement_status("MyNPC")

# Perception
gaiu.get_perception_info("MyNPC")
gaiu.get_perceived_actors("MyNPC")  # PIE

# EQS
gaiu.list_eqs_queries("/Game/AI")

# Avoidance
gaiu.get_avoidance_info("MyNPC")
gaiu.set_avoidance_settings("MyNPC", True, 0.5)
```

### GenQuestUtils - Quest System
```python
import unreal
gqu = unreal.GenQuestUtils()

# Quest-Daten laden
gqu.load_quests_from_json("/Game/Data/quests.json")
gqu.get_quest_info("quest_001")
gqu.list_all_quests()
```

---

## Model Control Protocol (MCP)

### Funktionen:

#### Blueprints Auto-Generierung:
- ✅ Neue Blueprints erstellen
- ✅ Funktionen und Variablen hinzufügen
- 🛠️ Nodes und Verbindungen (in Arbeit)

#### Level/Szenen-Kontrolle:
- ✅ Objekte und Formen spawnen
- ✅ Transformationen (Position, Rotation, Skalierung)
- ✅ Materialien und Farben ändern

#### Steuerung:
- ✅ Python-Skripte ausführen
- ✅ Konsolen-Befehle ausführen

#### Projektdateien:
- ✅ Dateien/Ordner erstellen und bearbeiten

### Legende:
- ✅ Fertig
- 🛠️ In Arbeit
- 🚧 Geplant
- ❌ Nicht unterstützt

---

## Bekannte Probleme

- Nodes verbinden sich manchmal nicht korrekt mit MCP
- Kein Undo/Redo Support für MCP
- Kein Streaming-Support für DeepSeek Reasoning
- Keine komplexe Material-Generierung
- Blueprint-Kompilierung ohne richtige Fehlerbehandlung
- Fenster docken nicht richtig an

---

## Beitragen

### Entwicklungsumgebung einrichten:

1. Unreal Python-Paket installieren:
```bash
pip install unreal
```

2. MCP CLI installieren:
```bash
pip install mcp[cli]
```

---

## Referenzen

- [OpenAI API Dokumentation](https://platform.openai.com/docs/api-reference)
- [Anthropic API Dokumentation](https://docs.anthropic.com/en/docs/about-claude/models)
- [DeepSeek API Dokumentation](https://api-docs.deepseek.com/)
- [XAI API Dokumentation](https://docs.x.ai/api)
- [MCP Dokumentation](https://modelcontextprotocol.io/)

---

## Dieses Projekt unterstützen

Wenn du UnrealGenAISupport hilfreich findest, erwäge eine Unterstützung des Projekts!

---

*Letzte Aktualisierung: 2025-12-30*
*Deutsche Übersetzung & Erweiterungen von Claude*
