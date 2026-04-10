# Validation Checklist

## Structural

- Blueprint compiles successfully.
- No orphan impure nodes remain.
- Exec chains are complete.
- Required pins are connected or intentionally defaulted.
- Variables are typed and sensible.

## Functional

- Planned entry events exist.
- Planned actions exist.
- Critical data flow matches the intended behavior.
- No unexpected side-effect nodes were introduced.

## Runtime

- Skip runtime-only checks when PIE/runtime evidence is unavailable.
