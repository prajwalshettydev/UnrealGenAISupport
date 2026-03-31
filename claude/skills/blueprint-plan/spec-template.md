# Blueprint Change Spec Template

Use this format when outputting a planned change.

```markdown
## Blueprint Change Spec: <BP_Name>

### Target
- Blueprint: <path>
- Graph: <EventGraph or function name>
- Current state: <node count> nodes, <flow count> event flows

### Prerequisite Checks
- [ ] Blueprint compiles: <yes/no>
- [ ] Spec file exists: <path or "none">
- [ ] Relevant event flow identified: <flow name>

### Changes (in execution order)
1. <action>: <details>
   - node_type: <type>
   - position: [x, y]
   - pin_values: {key: value}
2. <action>: <details>
...

### Connections
- <source_node>.then → <target_node>.execute
- <source_node>.<pin> → <target_node>.<pin>

### Expected Result
<1-2 sentences: what the blueprint should do after these changes>

### Risks
- <any potential issues or side effects>
```
