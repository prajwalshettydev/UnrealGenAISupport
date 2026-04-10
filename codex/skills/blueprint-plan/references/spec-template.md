# Blueprint Change Spec Template

Use this structure for implementation-ready plans:

```markdown
## Blueprint Change Spec: <Blueprint Name>

### Target
- Blueprint: <asset path>
- Graph: <EventGraph or function graph>
- Current state: <brief summary>

### Planned Changes
1. <ordered action>
2. <ordered action>

### Capability Matrix
| Node or Tool | Purpose | Confirmed By | Risk |
|---|---|---|---|

### Risks
- <typed pins / unsupported nodes / cross-BP dependencies>

### Expected Result
<what the Blueprint should do after the edit>
```

Prefer behavior-level language over raw patch JSON unless the user explicitly wants the patch shape during planning.
