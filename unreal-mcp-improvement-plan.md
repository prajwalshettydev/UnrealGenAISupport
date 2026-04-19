# Unreal MCP Stability And Animation Workflow Improvement Plan

## Document Info
- Version: `v1.0`
- Date: `2026-04-18`
- Scope: Current `GenerativeAISupport` Unreal MCP plugin, Python socket server, Blueprint/AnimBlueprint/BlendSpace/Enhanced Input workflows
- Goal: Address architecture mismatches, missing restart session restoration, unsafe asset writes, limited AnimBlueprint support, and weak diagnostics

## Background
The current MCP already supports basic asset creation, ordinary Blueprint graph edits, Python execution, plugin enable/disable, and editor restart. The main issue is not command availability. The issue is that many commands do not execute a full Unreal Editor transaction.

This plan is grounded in the current implementation:
- Transport and dispatch: [unreal_socket_server.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/unreal_socket_server.py>)
- MCP entrypoint: [mcp_server.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/mcp_server.py>)
- Editor context and restart: [plugin_commands.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/handlers/plugin_commands.py>)
- Blueprint graph editing: [GenBlueprintNodeCreator.cpp](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Source/GenerativeAISupportEditor/Private/MCP/GenBlueprintNodeCreator.cpp>)
- Blueprint connections: [GenBlueprintUtils.cpp](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Source/GenerativeAISupportEditor/Private/MCP/GenBlueprintUtils.cpp>)
- Legacy input binding path: [basic_commands.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/handlers/basic_commands.py>)
- Python escape hatch: [python_commands.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/handlers/python_commands.py>)
- Restart launcher: [restart_editor_launcher.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/restart_editor_launcher.py>)

## Current Problems
- No preflight checks for module architecture, target architecture, plugin state, or input system.
- Restart only exits and relaunches the editor. It does not preserve or restore open asset editors or working context.
- High-risk asset writes do not use a shared transaction layer. BlendSpace and AnimBlueprint changes can leave internal caches out of sync.
- Blueprint graph lookup is too narrow. It mostly assumes `UbergraphPages`, `FunctionGraphs`, and `MacroGraphs`, which is insufficient for AnimBlueprint state machines.
- Input commands still prefer legacy `InputSettings`, which does not align with UE5 template projects using Enhanced Input.
- Error reporting is shallow. It often stops at Python exceptions instead of exposing structured diagnostics, changed assets, rollback state, or Unreal log deltas.

## Goals
- Add reliable preflight checks and capability negotiation.
- Upgrade from property-level edits to Unreal Editor transaction-level operations.
- Add first-class APIs for BlendSpace, AnimBlueprint, and Enhanced Input.
- Make editor restart session-aware so open working windows can be restored.
- Improve observability so failures can be tied to save, compile, cache sync, or architecture causes.

## Design Principles
- Python handles protocol, argument validation, and orchestration.
- C++ Editor utilities handle actual Unreal asset mutation.
- High-risk assets use semantic APIs instead of relying on arbitrary Python execution.
- All write operations require validation, reporting, and rollback where appropriate.
- Read support comes before write support for any graph or asset domain.

## Target Architecture
- `mcp_server.py`: MCP registration and protocol adaptation only.
- `unreal_socket_server.py`: request dispatch, job status, timeout handling, cancellation, progress.
- `handlers/`: request normalization and command routing.
- C++ MCP utilities: asset transactions, graph read/write, input utilities, animation utilities, session restore.
- `Saved/MCP`: session snapshots, mutation reports, rollback metadata, and diagnostic artifacts.

## Delivery Phases

| Phase | Name | Goal | Output | Exit Criteria |
|---|---|---|---|---|
| P0 | Stability And Diagnostics | Detect root causes before execution | Capability negotiation, project preflight, richer editor context | Architecture and plugin issues are reported before restart loops |
| P1 | Safe Write Layer | Standardize asset mutation flow | Transaction utilities, post-save verification, rollback | High-risk writes fail cleanly without leaving partially broken assets |
| P1.5 | Restart Session Restore | Make restart resumable | Session snapshot, asset editor restoration | Open assets are restored after restart |
| P2 | Input System Alignment | Match UE5 defaults | Enhanced Input APIs | Input mapping works directly in template projects |
| P3 | BlendSpace Support | Stabilize animation sample editing | Read APIs and safe write APIs | BlendSpace changes save and reload correctly |
| P4 | AnimBlueprint Read Support | Understand structures before mutating | State machine and graph introspection | MCP can accurately describe existing AnimBlueprints |
| P5 | AnimBlueprint Write Support | Enable semantic animation graph editing | State machine and transition APIs | Locomotion-oriented AnimBlueprint edits are supported safely |
| P6 | Observability | Make failures diagnosable | Error codes, mutation reports, log deltas | Failures include enough information to locate the cause |
| P7 | Testing And Validation | Add regression protection | Test project and end-to-end scripts | Core workflows pass on template projects and Mac arm64 |

## Detailed Development Checklist

### P0 Stability And Diagnostics
- [ ] Add `get_capabilities`.
  - Files: [unreal_socket_server.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/unreal_socket_server.py>), [mcp_server.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/mcp_server.py>)
  - Must return: `engine_version`, `platform`, `machine_architecture`, `input_system`, `supported_asset_types`, `supported_graph_types`, `unsafe_commands`.

- [ ] Add `preflight_project`.
  - Files: new `preflight_commands.py`, [plugin_commands.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/handlers/plugin_commands.py>)
  - Must check: `.uproject`, editor executable, project target architecture, binary module architecture, plugin state.
  - Must explicitly report `x64/arm64` mismatch with a remediation hint.

- [ ] Expand `get_editor_context`.
  - Files: [plugin_commands.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/handlers/plugin_commands.py>)
  - Add: `editor_binary_architecture`, `project_target_architecture`, `module_architectures`, `input_system`, `enabled_plugins`, `dirty_assets`.

- [ ] Replace the fixed queue/timeout flow with a job model.
  - Files: [unreal_socket_server.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/unreal_socket_server.py>)
  - Add: `job_id`, job states, progress, cancellation, per-command timeout policies.

### P1 Safe Write Layer
- [ ] Add `UGenAssetTransactionUtils`.
  - Files: new `GenAssetTransactionUtils.cpp/.h`
  - Standard flow: `Load -> Modify -> Apply -> PostEdit sync -> Save/Compile -> Reload -> Verify`.

- [ ] Add rollback support for high-risk asset types.
  - Files: `GenAssetTransactionUtils`, Python handler orchestration
  - Strategy: duplicate temporary asset, validate changes on the duplicate, then replace the primary asset only after success.

- [ ] Downgrade `execute_python` to an explicitly unsafe escape hatch.
  - Files: [python_commands.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/handlers/python_commands.py>)
  - Add: `read_only`, `dry_run`, `changed_assets`, `recent_logs`, `dirty_packages`.

### P1.5 Restart Session Restore
- [ ] Add `capture_editor_session`.
  - Files: new `session_commands.py`, new `GenEditorSessionUtils.cpp/.h`
  - Capture: `open_asset_paths`, `primary_asset_path`, `active_graph_path`, `selected_actors`, `current_map`.

- [ ] Capture open asset editors through `UAssetEditorSubsystem->GetAllEditedAssets()`.
  - Files: `GenEditorSessionUtils`
  - Save to `Saved/MCP/LastEditorSession.json`.

- [ ] Save the session snapshot automatically before restart.
  - Files: [plugin_commands.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/handlers/plugin_commands.py>)
  - Hook this into `request_editor_restart`.

- [ ] Restore the last editor session after relaunch.
  - Files: `init_unreal.py`, `session_commands.py`, `GenEditorSessionUtils`
  - First-stage restore targets: `Blueprint`, `AnimBlueprint`, `Animation Sequence`, `BlendSpace`, `Widget Blueprint`, `Material`.

- [ ] Restore the primary working asset to the foreground.
  - Files: `GenEditorSessionUtils`
  - The main asset should be focused after restoration.

- [ ] Add restore fault tolerance.
  - Files: `GenEditorSessionUtils`, Python handlers
  - Return `restored_assets` and `failed_assets`. One failure must not stop the rest.

- [ ] Add a restore policy setting.
  - Files: plugin settings, handlers
  - Modes: `none`, `assets_only`, `assets_and_tabs`
  - Default: `assets_only`

- [ ] Add second-stage tab/layout restoration.
  - Files: `GenEditorSessionUtils`
  - Restore common tabs such as Content Browser, World Outliner, Details, Output Log.
  - Defer this until asset restoration is already stable.

### P2 Input System Alignment
- [ ] Add `UGenEnhancedInputUtils` and `input_commands.py`.
  - Provide: `create_input_action`, `create_input_mapping_context`, `map_enhanced_input_action`, `list_input_mappings`.

- [ ] Keep legacy `add_input_binding`, but warn in Enhanced Input projects.
  - Files: [basic_commands.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/handlers/basic_commands.py>)

### P3 BlendSpace Support
- [ ] Add `get_blend_space_info`.
  - Files: new `animation_commands.py`, new `GenAnimationAssetUtils.cpp/.h`
  - Must return: axis settings, sample list, target skeleton, additive settings.

- [ ] Add safe write APIs for BlendSpace.
  - Files: `GenAnimationAssetUtils`
  - Provide: `set_blend_space_axis`, `replace_blend_space_samples`, `set_blend_space_sample_animation`.
  - Internally call `ResampleData`, `ValidateSampleData`, and `PostEditChange`.

- [ ] Add post-save reload verification for BlendSpace.
  - Files: `GenAnimationAssetUtils`
  - Reload and compare sample count, coordinates, and asset references after save.

### P4 AnimBlueprint Read Support
- [ ] Replace narrow graph lookup with full graph enumeration.
  - Files: [GenBlueprintNodeCreator.cpp](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Source/GenerativeAISupportEditor/Private/MCP/GenBlueprintNodeCreator.cpp>)
  - Use `UBlueprint::GetAllGraphs()` instead of only scanning `UbergraphPages`, `FunctionGraphs`, and `MacroGraphs`.

- [ ] Add `get_anim_blueprint_structure`.
  - Files: new `GenAnimationBlueprintUtils.cpp/.h`, new `animation_commands.py`
  - Must expose: state machines, states, transitions, aliases, cached poses, slots, state asset bindings.

- [ ] Add graph inspection APIs.
  - Files: `GenAnimationBlueprintUtils`
  - Provide: `get_graph_nodes`, `get_graph_pins`, `resolve_graph_by_path`.

### P5 AnimBlueprint Write Support
- [ ] Add semantic state machine APIs.
  - Files: `GenAnimationBlueprintUtils`, `animation_commands.py`
  - Provide: `create_state_machine`, `create_state`, `create_transition`, `set_transition_rule`, `create_state_alias`, `set_alias_targets`.

- [ ] Add state content APIs.
  - Files: `GenAnimationBlueprintUtils`
  - Provide: `set_state_sequence_asset`, `set_state_blend_space_asset`, `set_cached_pose_node`, `set_default_slot_chain`, `set_apply_additive_chain`.

- [ ] Make semantic AnimBlueprint APIs the default path.
  - Files: `GenBlueprintUtils.cpp`, `GenBlueprintNodeCreator.cpp`, Python handlers
  - Raw node editing for AnimBlueprint should be `unsafe`, not the primary route.

### P6 Observability
- [ ] Return structured mutation reports for all write operations.
  - Fields: `changed_assets`, `compiled_assets`, `saved_assets`, `warnings`, `rollback_performed`.

- [ ] Attach Unreal log deltas automatically when save or compile fails.
  - Files: [python_commands.py](</Users/Shared/Epic Games/UE_5.4/Engine/Plugins/Marketplace/GenerativeAISupport/Content/Python/handlers/python_commands.py>), shared logging utilities

- [ ] Add unified error codes.
  - Minimum set: `ARCH_MISMATCH`, `GRAPH_NOT_SUPPORTED`, `ASSET_VALIDATION_FAILED`, `SAVE_FAILED`, `ROLLBACK_FAILED`, `LEGACY_INPUT_PATH`.

### P7 Testing And Validation
- [ ] Create a minimal regression project.
  - Cover: Actor Blueprint, Widget Blueprint, Enhanced Input, BlendSpace, AnimBlueprint.

- [ ] Add Mac arm64 regression coverage.
  - Validate project preflight and architecture mismatch detection.

- [ ] Add end-to-end automation for the Third Person template.
  - Cover: `IA_Crouch`, `IMC_Default`, `Locomotion`, `Crouched`, `Lean`, character Anim Class binding, compile, save, restart, restore, and reload verification.

## Priority Order
1. `P0` Stability And Diagnostics
2. `P1` Safe Write Layer
3. `P1.5` Restart Session Restore
4. `P2` Input System Alignment
5. `P3` BlendSpace Support
6. `P4` AnimBlueprint Read Support
7. `P5` AnimBlueprint Write Support
8. `P6` Observability
9. `P7` Testing And Validation

## Milestones
- Milestone 1: The MCP reports architecture, plugin, and input system issues before execution.
- Milestone 2: BlendSpace writes complete safely and survive save plus reload.
- Milestone 3: Restart restores previously open working assets automatically.
- Milestone 4: The MCP can reliably inspect existing AnimBlueprint structures.
- Milestone 5: The MCP can safely perform semantic locomotion edits in AnimBlueprints.

## Non-Goals
- Full arbitrary graph mutation for every editor graph type is not a first-phase goal.
- `execute_python` is not the primary implementation path.
- Full Slate layout fidelity is deferred until asset editor restoration is stable.

## Suggested Sprint Breakdown
- Sprint 1: `P0 + P1`
- Sprint 2: `P1.5 + P2 + P3`
- Sprint 3: `P4 + P5`
- Sprint 4: `P6 + P7`
