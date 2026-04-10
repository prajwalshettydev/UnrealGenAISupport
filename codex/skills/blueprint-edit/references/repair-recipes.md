# Repair Recipes

## Preflight Failure

- Read the reported issues carefully.
- Re-read only the affected nodes or subgraph.
- Rebuild the patch with corrected refs or pin names.

## Compile Failure

- Read the compile errors first.
- Inspect the flagged node or its immediate neighbors.
- Fix the smallest possible issue and re-apply.

## Runtime Disconnect

- Stop editing immediately.
- Ask the user to reopen UE, confirm the plugin is enabled, and retry in a fresh turn.
