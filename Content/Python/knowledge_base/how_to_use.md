# Note: This guide is not for humans, LLMs can refer it to understand how to use the Unreal Engine MCP Plugin.

# Unreal Engine MCP Plugin Guide

This plugin lets an LLM (like you, Claude) boss around Unreal Engine with MCP. Automates Blueprint wizard-ry, node hookups, all that jazz. Check this s to not f it up.

## Key Notes

- **Pin Connections**: For inbuilt Events like BeginPlay Use "then" for execution pin, not "OutputDelegate" (delegates). Verify pin names in JSON.
- **Node Types**: Use `add_node_to_blueprint` with "EventBeginPlay", "Multiply_FloatFloat", or basics like "Branch", "Sequence". Unrecognized types return suggestions.
- **Node Spacing**: Set `node_position` in JSON (e.g., [0, 0], [400, 0])—maintain 400x, 300y gaps to prevent overlap.
- **Inputs**: Use `add_input_binding` to set up the binding (e.g., "Jump", "SpaceBar"), then `add_node_to_blueprint` with "K2Node_InputAction" and `"action_name": "Jump"` in `node_properties`. Ensure `action_name` matches.
- **Colliders**: Add via `add_component_with_events` (e.g., "MyBox", "BoxComponent")—returns `"begin_overlap_guid"` (BeginOverlap node) and `"end_overlap_guid"` (EndOverlap node).
- **Materials**: Use `edit_component_property` with property_name as "Material", "SetMaterial", or "BaseMaterial" and value as a material path (e.g., "'/Game/Materials/M_MyMaterial'") to set on mesh components (slot 0 default)."



*(Additional quirks will be added as discovered.)*