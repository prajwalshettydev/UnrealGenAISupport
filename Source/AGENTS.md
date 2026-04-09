<!-- Parent: ../AGENTS.md -->
<!-- Generated: 2026-04-08 | Updated: 2026-04-08 -->

# Source

## Purpose
C++ source modules for the GenerativeAISupport plugin. Contains runtime LLM API integrations and editor-only Blueprint tooling.

## Subdirectories

| Directory | Purpose |
|-----------|---------|
| `GenerativeAISupport/` | Runtime module: LLM API clients (Claude, OpenAI, DeepSeek, XAI) (see `GenerativeAISupport/AGENTS.md`) |
| `GenerativeAISupportEditor/` | Editor module: MCP Blueprint node creator, editor commands, settings (see `GenerativeAISupportEditor/AGENTS.md`) |

## For AI Agents

### Working In This Directory
- Runtime module loads at `Default` phase; Editor module loads at `PostEngineInit`
- Never add Editor-only APIs (Slate, Kismet2) to the runtime module
- GenBlueprintNodeCreator.cpp is a hot path (221x) — profile before modifying

### Dependencies

#### External
- Unreal Engine 5.7
- UHT (Unreal Header Tool) for reflection

<!-- MANUAL: -->
