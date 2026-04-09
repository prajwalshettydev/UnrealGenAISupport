<!-- Parent: ../../AGENTS.md -->
<!-- Generated: 2026-04-08 | Updated: 2026-04-08 -->

# claude/skills (Blueprint Authoring Skills)

## Purpose
Claude Code skills for structured Blueprint authoring in the three-phase pattern: plan → edit → QA. These skills define the workflow AI agents follow when creating or modifying Blueprints via the MCP toolchain.

## Subdirectories
| Directory | Purpose |
|-----------|---------|
| `blueprint-plan/` | Strategic planning skill: generates spec JSON from intent; includes spec template and worked examples |
| `blueprint-edit/` | Execution skill: applies spec/patches via MCP tools; includes API reference and patch patterns |
| `blueprint-qa/` | QA verification skill: runs quality analysis and validates connections; includes report template and checklist |
| `blueprint-mcp-dev/` | Development skill: used when extending the MCP server itself (meta-level) |

## For AI Agents

### Working In This Directory
- Always invoke skills in order: plan → edit → qa.
- `blueprint-edit/mcp-api-reference.md` is the authoritative reference for all 18+ MCP tools.
- `blueprint-edit/safety-rules.md` defines which operations require dry-run validation first.
- `blueprint-plan/examples/` contains worked examples — read these before generating a new spec.
- Never apply a patch without running `preflight_blueprint_patch` (dry-run) first for destructive operations.

### Common Patterns
- Plan phase: `apply_blueprint_spec` with `dry_run=true` to validate before committing.
- Edit phase: `apply_blueprint_patch` in atomic transactions (`begin_transaction` / `end_transaction`).
- QA phase: `analyze_blueprint_quality` → check C1-C6 metrics → fix issues → re-verify.

<!-- MANUAL: -->
