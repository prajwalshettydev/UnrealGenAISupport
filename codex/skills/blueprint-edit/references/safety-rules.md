# Blueprint Edit Safety Rules

- Never edit without an approved plan or an unambiguous tiny change.
- Save in UE before MCP edits.
- Keep edits small and incremental.
- Take before and after snapshots and compare them.
- Stop after two failed auto-fix attempts.
- Treat class/object/soft reference pins as high risk until confirmed by live inspection.
- Prefer scoped graph reads over full-graph dumps.
- Do not use generic Python execution to bypass Blueprint edit guardrails.
