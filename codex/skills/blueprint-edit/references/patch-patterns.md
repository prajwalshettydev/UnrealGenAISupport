# Patch Patterns

## Local Surgical Edit

Use when one graph needs a small change:

1. Read scoped pseudocode
2. Inspect the 1-3 relevant nodes
3. Preflight the patch
4. Apply
5. Compile and diff

## Declarative Create Flow

Use when building a new Blueprint or a larger new graph:

1. Build a spec
2. Apply with `apply_blueprint_spec`
3. Compile
4. Save
5. Run QA

## Cross-BP Replication

Use only with an explicit plan:

1. Trace the source pattern first
2. Confirm all target Blueprints
3. Apply one Blueprint at a time with compile checkpoints
4. Run QA after the final graph lands
