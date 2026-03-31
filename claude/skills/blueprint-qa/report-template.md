# QA Report Template

```markdown
## QA Report: <BP_Name>

### Structural (X/5 pass)
- [PASS] S1: Compiles successfully
- [FAIL] S2: Orphan impure node "<title>" (GUID: ...) — not in any exec/data edge
- [PASS] S3: All exec chains reach terminal
- [PASS] S4: All required pins connected or defaulted
- [PASS] S5: Variables properly typed

### Functional (X/4 pass)
- [PASS] F1: All spec events present
- [FAIL] F2: Spec action "SendStimulus" not found in graph
- [PASS] F3: Data flow matches
- [PASS] F4: No unspecced side effects
(or: "No spec found — functional checks skipped.")

### Runtime (X/3 pass)
- [SKIP] PIE not running

### Summary
Overall: X/12 checks passed, Y skipped
Critical issues:
  1. <issue description>

Suggested fixes:
  1. <fix description>
     ```json
     {"remove_nodes": ["<orphan_guid>"]}
     ```
```
