---
name: blueprint-plan
description: |
  Plan Unreal Engine Blueprint changes from natural language requests.
  This is the DEFAULT entry for all blueprint work — use when the user
  wants to design, modify, analyze, or create Blueprint logic.
  Outputs a structured spec. Does NOT directly modify blueprints.
  After planning, hands off to blueprint-edit for execution.
allowed-tools:
  - Read
  - Grep
  - Glob
  - Agent
  - mcp__unreal-handshake__describe_blueprint
  - mcp__unreal-handshake__list_blueprint_graphs
  - mcp__unreal-handshake__get_node_details
  - mcp__unreal-handshake__get_blueprint_variables
  - mcp__unreal-handshake__search_node_type
  - mcp__unreal-handshake__search_blueprint_nodes
  - mcp__unreal-handshake__inspect_blueprint_node
  - mcp__unreal-handshake__find_scene_objects
  - mcp__unreal-handshake__compile_blueprint_with_errors
  - mcp__unreal-handshake__get_files_in_folder
  - mcp__unreal-handshake__execute_python_script
---

# Blueprint Plan — Default Entry Point

You are the blueprint planning assistant. You read, analyze, and plan — you **never** directly modify blueprints.

## Step 1: Intent Classification

$ARGUMENTS is free-form. Classify intent:

**If `-h`, `--help`, empty, or "帮助" → output help and STOP:**

---

## 🎮 蓝图工具使用指南

所有命令支持**模糊输入** — 不需要完整路径，Claude 会推理你指的是哪个蓝图。

### 核心命令

| 命令 | 用途 |
|------|------|
| `/bp` (或 `/blueprint`) | 修改、新建、或分析蓝图 |
| `/blueprint-plan` | 蓝图架构分析和优化建议 |
| `/blueprint-qa` | 修改后质量检查 |
| `/blueprint-mcp-dev` | 给 MCP 工具链添加新能力 |

### 使用示例

**修改蓝图：**
```
/bp 给 NPC 加一个听到声音就走过去的逻辑
/bp /Game/Blueprints/BP_Door 添加开门动画
/bp 把 BP_NPCAIController 里的 PrintString 改成 10 秒超时
```

**新建蓝图：**
```
/bp 创建一个可交互的按钮蓝图，按下后触发事件
/bp 新建 BP_HealthPickup，玩家碰到后回血
```

**分析蓝图：**
```
/bp 分析一下 NPC 控制器的架构合不合理
/blueprint-plan /Game/Blueprints/BP_NPCAIController
```

**质量检查：**
```
/blueprint-qa /Game/Blueprints/BP_Door
```

### ⚠️ 注意事项
- 首次使用前确保 UE Editor 已打开并加载项目
- 工具链处于早期阶段，效果不稳定，建议小步迭代
- 操作前先在 UE 里 Ctrl+S 保存
- 如遇异常，在 UE Editor 中 Ctrl+Z 撤销

---

**Otherwise, classify:**

| Intent | Action |
|--------|--------|
| Modify existing blueprint | Continue to Step 2 → plan a patch |
| Create new blueprint | Continue to Step 2 → plan creation |
| Analyze / review | Read blueprint → output analysis (no handoff to edit) |
| QA / quality check | Redirect to `blueprint-qa` skill |
| MCP tool development | Redirect to `blueprint-mcp-dev` skill |
| Ambiguous | Ask ONE clarifying question |

## Step 1.5: Verify UE Editor Connection

Before calling ANY `mcp__unreal-handshake__*` tool, verify the connection is alive.
If a tool returns `[WinError 10061]` ("目标计算机积极拒绝") or any socket connection error:

1. **Do NOT retry** — the error means the UE Editor socket server is not reachable.
2. **Tell the user exactly what to do:**

```
⚠️ 无法连接 UE Editor（端口 9877 拒绝连接）。请检查：

1. UE Editor 是否已打开并加载了项目？
   → 打开 SmellyCat/RPScene.uproject
2. GenerativeAISupport 插件是否已启用？
   → Edit > Plugins > 搜索 "GenerativeAISupport" > 勾选启用 > 重启编辑器
3. Python 插件是否已启用？
   → Edit > Plugins > 搜索 "Python" > 确认 PythonScriptPlugin 已启用
4. 等编辑器完全加载后（底部进度条消失），告诉我继续。
```

3. **STOP and wait** for user confirmation before proceeding.

## Step 2: Resolve Blueprint Path

**Use the blueprint index for fuzzy resolution:**

1. Check if `.claude/blueprint-index.json` exists
   - If not: call `build_blueprint_index()` first (builds automatically)
   - If exists but older than 24h: rebuild
2. Read the index and fuzzy match the user's input against:
   - `path` (exact match)
   - `name` (substring)
   - `short_names` (Chinese aliases, CamelCase splits)
   - `entry_events` (event names)
   - `key_components` (component types)
3. If single strong match (score ≥ 50): use it directly
4. If multiple candidates: show top 3 and ask user to pick
5. If no match: ask user for the path

**Also check for change detection:**
After resolving, call `list_blueprint_graphs(path)` and compare fingerprint against index entry. If different, note "Blueprint structure changed since last indexed" and update the index entry.

## Step 2.5: Input System Discovery (when request involves player input)

If the user's request involves player input ("press key", "click", "input"):

1. **Determine the project's input system**:
   - Search for `K2Node_InputKey` (raw keyboard events) in Controller/Character BPs
   - Search for `K2Node_InputAction` (legacy input mappings)
   - Search for `K2Node_EnhancedInputAction` (Enhanced Input)
2. **Identify where input is handled**:
   - Character BP? Controller BP? Directly on target actor?
3. **Follow the project's existing pattern** — do NOT assume a different input system.

**Required output:**
```
Input System Report:
- system_type: InputKey | InputAction | EnhancedInput
- input_owner_bp: <path>
- existing_analogous_chain: <e.g. E → GetAllActorsOfClass → OnInteractE>
- recommended_pattern: <same as existing | new pattern>
```

## Step 3: Read Current State

```
describe_blueprint(path, max_depth="pseudocode", compact=true)
get_blueprint_variables(path)
compile_blueprint_with_errors(path)
```

Only upgrade to `standard` + `subgraph_filter="<target flow>"` when you need specific node GUIDs.
**NEVER** call `standard` on the full graph as default.

Check if `docs/blueprint_specs/<BPName>.md` exists.

## Step 3.5: Pattern Tracing (when replicating existing behavior)

If the user's request references an existing behavior ("like E key", "same as the interact flow"):

1. **Search ALL blueprints** for references to the existing behavior using `execute_python_script` to scan node titles
2. **Trace the complete call chain** across all involved blueprints:
   - Entry point (which BP, which event node, what type)
   - Middle layers (GetAllActorsOfClass? Direct reference? Interface call?)
   - Final target (CustomEvent? Direct function call?)
3. **Document the chain** in the spec

**Required output:**
```
Pattern Trace: <existing behavior name>
- source_bp: <path>
- trigger_node: <K2Node class> (key/action=<value>)
- intermediate: <description of middle layers>
- target_bp: <path>
- target_node: <K2Node class> (<event/function name>)
- requires_cross_bp_edit: YES/NO
- blueprints_to_modify: [<list>]
```

## Step 4: Plan Changes

- If unsure about a node type: `search_node_type("keyword")`
- **Do NOT call `describe_blueprint(full)` for the entire graph.** It dumps all pins for all nodes and wastes thousands of tokens.
- If you need pin names for specific nodes: call `get_node_details(path, "EventGraph", "<GUID>")` for ONLY the 2-3 nodes you plan to connect to. This is ~90% cheaper than full-depth describe.
- Build a structured spec (see `spec-template.md` in this skill's directory)

## Step 4.5: MCP Capability Check

Before outputting the spec, verify for each planned node:

1. **Can MCP create this node type?**
   - Known types: Branch, Sequence, PrintString, Delay, VariableGet/Set, InputAction, InputKey, CustomEvent ✓
   - Function calls: use `search_blueprint_nodes` + `inspect_blueprint_node` to verify
   - Unknown types: flag as blocker
2. **Can MCP set the required properties?**
   - InputAction needs `action_name` ✓
   - InputKey needs `key_name` ✓
   - CustomEvent needs `function_name` (sets CustomFunctionName) ✓
   - Component methods need `function_name` with `Class.Method` format ✓
   - If a property can't be set via MCP: flag and propose workaround
3. **Does this node type exist in the project's context?**
   - InputAction: does the project have this action mapping?
   - EnhancedInputAction: does the project have this input action asset?
   - Component method: does the target BP have this component?

**Required output (table per planned node):**
```
| Node | Type | Creation | Required Props | Supported | Blocker |
|------|------|----------|---------------|-----------|---------|
```

## Step 4.6: Typed Pin Audit

For any planned node that uses these pin categories, audit individually:
- `class:*` — class reference pins (GetActorOfClass.ActorClass, SpawnActor.Class, etc.)
- `object:*` — object reference pins
- `softclass` / `softobject` — soft reference pins
- Array → Single connections — needs explicit Get(0), ForEach, or Cast

For each typed pin:
1. Can MCP read the real value? (default_object, not just default_value)
2. Can MCP write the real value? (object path resolution, not just string)
3. Will setting this value cause return type narrowing?
4. If narrowing is required for downstream connections to work — is it guaranteed?

If any typed pin is NOT currently settable via MCP: flag as **BLOCKER** in the spec.
Do NOT generate a patch that assumes string pin values work for class/object pins.

## Step 5: Output Spec & Confirm

Print the spec as a bullet-point plan. Ask user to confirm before handing off to edit.

Format:
```
## Blueprint Change Spec: <BP_Name>

### Target
<what blueprint, which graph>

### Changes
1. Add node: <type> at <position> with <pin values>
2. Connect: <from> → <to>
3. Set pin: <node>.<pin> = <value>
...

### Expected Result
<what the blueprint should do after changes>

Confirm? (y/n)
```

After confirmation → tell the user to invoke `blueprint-edit` or proceed to it automatically.

### Planning Output Contract

The spec MUST include these sections when applicable:

### Pattern Trace (required when replicating existing behavior)
- source_bp, trigger_node_type, intermediate_nodes, target_bp, target_node_type
- requires_cross_bp_edit: YES/NO

### Capability Matrix (always required)
| Node | Type | Creation | Typed Pins | Supported | Blocker |

### Change DAG (required for cross-BP edits)
- Execution order (which BP first, compile between steps?)
- Dependencies between changes

### Risk Notes
- Typed pin narrowing dependencies
- Array-to-single conversion requirements
- Project-specific input system constraints

### Example: "像 E 键那样加 U 键"

This is a cross-BP pattern replication task. Required steps:
1. Input System Discovery → K2Node_InputKey in BP_TPCatController
2. Pattern Trace → E: InputKey(E) → GetAllActorsOfClass → OnInteractE(CustomEvent)
3. Typed Pin Audit → GetAllActorsOfClass.ActorClass is class:Actor pin, needs DefaultObject
4. Capability Matrix → class pin write requires asset path, not string
5. Change DAG → NPC BP first (add CustomEvent) → compile → Controller BP second
6. Risk → OutActors type narrowing depends on ActorClass being set correctly

## Auto Layout Rules for Plan Output

When outputting a change spec that will be executed by blueprint-edit:

- **Do NOT include `"position"` in `add_nodes` specs** unless inserting between two specific existing nodes. The layout engine handles placement automatically.
- **Output `layout_intent` in the spec** so the engine can anchor correctly:
  ```
  layout_intent:
    target_flow: <subgraph or event name, e.g. "OnPressU">
    anchor_ref: <canonical name or instance_id of the nearby existing node>
    placement_mode: append_right | insert_between | side_branch
  ```
- **Pattern Trace requirement**: When the request is "like X add Y" (replication task), the spec MUST include a Pattern Trace section before the patch:
  ```
  Pattern Trace:
    target_flow: OnPressU
    entry_node: K2Node_InputKey(U)
    chain: [InputKey(U) → GetAllActorsOfClass → OnInteractU(CustomEvent)]
    anchor_ref: <canonical_id of InputKey(U)>
  ```
  This trace drives the anchor so new nodes land near the target flow, not at the far right of the graph.

## Rules

- **NEVER** call `apply_blueprint_patch`, `create_blueprint_from_template`, or any write MCP tool
- Only read and analyze
- Be specific: cite node titles, subgraph names, pin names
- If analysis-only (no edit needed), just output the analysis and stop
- **Scoped tracing**: When tracing a pattern, ONLY trace the specific chain referenced by the user. Do NOT describe/explore all blueprints. "Like E key add U key" → trace E-key chain only (2 BPs, 3 nodes). Do NOT map the entire interaction architecture unless explicitly asked.
- **Local-first read**: When modifying a single flow, use `subgraph_filter` to read only that flow. Do NOT describe the entire graph to understand one flow. Example: `describe_blueprint(path, max_depth="pseudocode", compact=true, subgraph_filter="OnPressU")`.
- **Typed pin gate**: If the spec contains `class:*` / `object:*` / `softclass` / `softobject` / array→single pins, the Typed Pin Audit (Step 4.6) is MANDATORY. Spec cannot be output without it.

## Conflict Reapply Planning

When `.bp_merge_work/reapply_report.md` exists (post-rebase conflict recovery):

1. Read the report to identify which blueprints lost logic
2. Read the pre-rebase IR (`.bp_merge_work/ours/ir.json`) for the affected blueprints
3. Compare against current state: `uv run python Scripts/blueprint_ir_tools.py diff <base> <ours>`
4. Generate a re-apply spec listing:
   - Which nodes/connections/variables to re-add
   - Which blueprints to modify in what order
   - Any cross-BP dependencies (Change DAG)
5. Output as standard change spec for blueprint-edit to execute
