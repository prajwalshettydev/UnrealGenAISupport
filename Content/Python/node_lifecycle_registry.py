"""Node Lifecycle Registry — P2-T4

Defines per-node-type lifecycle requirements for Blueprint graph editing.
Used by:
  - tools/patch.py: _compute_patch_risk_score(), preflight validation
  - mcp_server.py: high-layer structural tools

Fields per entry:
  needs_a1          list[str]   Properties that MUST be set before AllocateDefaultPins (A1 phase)
  risk              str         "low" | "medium" | "high"
  status            str         "supported" | "restricted" | "specialist_tool_required"
  reconstruct_after list[str]   Operations that require ReconstructNode() afterward
  static_schema_preflight bool  Whether static pin schema is reliable for preflight
  note              str         Free-text explanation
"""

NODE_LIFECYCLE: dict = {
    # -----------------------------------------------------------------------
    # Low-risk, fully supported
    # -----------------------------------------------------------------------
    "K2Node_CustomEvent": {
        "needs_a1": ["function_name"],
        "risk": "low",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": True,
        "note": "Set CustomFunctionName before AllocateDefaultPins",
    },
    "K2Node_InputKey": {
        "needs_a1": ["key_name"],
        "risk": "low",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": True,
        "note": "Set InputKey before AllocateDefaultPins",
    },
    "K2Node_InputAction": {
        "needs_a1": ["action_name"],
        "risk": "low",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": True,
        "note": "Set InputActionName before AllocateDefaultPins",
    },
    "K2Node_BreakStruct": {
        "needs_a1": ["struct_type"],
        "risk": "low",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": False,
        "note": "Pin count determined by struct fields; set StructType in A1",
    },
    "K2Node_MakeStruct": {
        "needs_a1": ["struct_type"],
        "risk": "low",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": False,
        "note": "Pin count determined by struct fields; set StructType in A1",
    },
    "K2Node_IfThenElse": {
        "needs_a1": [],
        "risk": "low",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": True,
        "note": "Branch node — pins: Condition(bool), then(exec), else(exec)",
    },
    "K2Node_ExecutionSequence": {
        "needs_a1": [],
        "risk": "low",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": True,
        "note": "Sequence node — dynamic output pins but stable for 2-3 outputs",
    },

    # -----------------------------------------------------------------------
    # Medium-risk, supported with caveats
    # -----------------------------------------------------------------------
    "K2Node_SwitchEnum": {
        "needs_a1": ["enum_type"],
        "risk": "medium",
        "status": "supported",
        "reconstruct_after": ["add_switch_case"],
        "static_schema_preflight": False,
        "note": "Enum type must be set in A1; ReconstructNode after add_switch_case",
    },
    "K2Node_SwitchString": {
        "needs_a1": [],
        "risk": "medium",
        "status": "supported",
        "reconstruct_after": ["add_switch_case"],
        "static_schema_preflight": False,
        "note": "ReconstructNode required after add_switch_case to materialize exec pin",
    },
    "K2Node_SwitchInteger": {
        "needs_a1": [],
        "risk": "medium",
        "status": "supported",
        "reconstruct_after": ["add_switch_case"],
        "static_schema_preflight": False,
        "note": "ReconstructNode required after add_switch_case",
    },
    "K2Node_CallFunction": {
        "needs_a1": ["function_name"],
        "risk": "medium",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": True,
        "note": "function_name must resolve via ResolveCallableFunction; wrong name = wildcard pins",
    },
    "K2Node_VariableGet": {
        "needs_a1": ["variable_name"],
        "risk": "low",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": True,
        "note": "Variable must exist on blueprint before creating this node",
    },
    "K2Node_VariableSet": {
        "needs_a1": ["variable_name"],
        "risk": "low",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": True,
        "note": "Variable must exist on blueprint before creating this node",
    },
    "K2Node_ForEachLoop": {
        "needs_a1": [],
        "risk": "medium",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": True,
        "note": "Loop body nodes require fname: prefix for reference; instance_id traversal limited",
    },
    "ForEachLoop": {
        "needs_a1": [],
        "risk": "medium",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": False,
        "note": "Alias for K2Node_MacroInstance referencing StandardMacros:ForEachLoop. "
                "Use node_type='ForEachLoop' in patches — alias resolver in tools/patch.py "
                "injects macro_graph automatically (caller need not provide it). "
                "Pin types are wildcard; static preflight not possible.",
    },

    # -----------------------------------------------------------------------
    # High-risk, restricted or requires specialist tool
    # -----------------------------------------------------------------------
    "K2Node_MacroInstance": {
        "needs_a1": ["macro_graph"],
        "risk": "high",
        "status": "restricted",
        "reconstruct_after": [],
        "static_schema_preflight": False,
        "note": "Pin count/types determined at instantiation; static schema unreliable for preflight",
    },
    "K2Node_ComponentBoundEvent": {
        "needs_a1": [],
        "risk": "high",
        "status": "specialist_tool_required",
        "reconstruct_after": [],
        "static_schema_preflight": False,
        "note": "Requires component instance binding; use bind_component_event tool",
    },
    "K2Node_Timeline": {
        "needs_a1": [],
        "risk": "high",
        "status": "restricted",
        "reconstruct_after": [],
        "static_schema_preflight": False,
        "note": "Timeline track configuration not supported via patch; use UE Editor manually",
    },
}


def get_node_lifecycle(node_type: str) -> dict:
    """Return lifecycle info for a node type, or a safe default for unknown types."""
    # Strip package prefix if present (e.g. "/Script/BlueprintGraph.K2Node_IfThenElse")
    short = node_type.rsplit(".", 1)[-1] if "." in node_type else node_type
    return NODE_LIFECYCLE.get(short, {
        "needs_a1": [],
        "risk": "low",
        "status": "supported",
        "reconstruct_after": [],
        "static_schema_preflight": True,
        "note": "Unknown node type — treated as low-risk supported",
    })


def is_restricted(node_type: str) -> bool:
    info = get_node_lifecycle(node_type)
    return info.get("status") in ("restricted", "specialist_tool_required")


def risk_score_for_node(node_type: str) -> int:
    """Return extra risk points for this node type (for patch_risk_score)."""
    info = get_node_lifecycle(node_type)
    return {"low": 0, "medium": 1, "high": 3}.get(info.get("risk", "low"), 0)
