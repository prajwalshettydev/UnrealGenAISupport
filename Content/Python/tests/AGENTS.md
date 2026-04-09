<!-- Parent: ../AGENTS.md -->
<!-- Generated: 2026-04-09 | Updated: 2026-04-09 -->

# tests

## Purpose
Python test suite for the MCP server internals. All tests run standalone without a live UE Editor connection (mock sockets and stubs used where UE APIs are needed).

## Key Files

| File | Description |
|------|-------------|
| `smoke_tests.py` | Fast smoke tests for transport constants, envelope, patch risk scoring, and strict_mode behavior (US-007) |
| `test_transport_protocol.py` | v1/v2 framing correctness, timeout handling, magic-byte detection, mock TCP server (US-002) |
| `test_patch_rollback.py` | Patch rollback and ghost-node cleanup after failed mutations |
| `test_p0_safety.py` | P0 safety checks: destructive-pattern blocking, RBW guard, HIGH_RISK_PATCH_BLOCKED |
| `test_bp_api_freshness.py` | Blueprint API freshness: cache fingerprint invalidation (sha256 node-guid hash) |

## For AI Agents

### Running Tests
```bash
# From Content/Python/ directory:
python tests/smoke_tests.py                  # no deps, fast
python tests/test_transport_protocol.py      # mock socket, no UE required
pytest tests/                                # full suite
```

### Working In This Directory
- All tests must run without a live UE Editor connection. Use mock sockets or stubs for `unreal.*` APIs.
- Test file names follow `test_<area>.py` convention; `smoke_tests.py` is the exception (legacy name).
- Add new tests here when adding features to `tools/`, `handlers/`, or `node_lifecycle_registry.py`.

### Common Patterns
- Mock TCP server pattern: see `_run_mock_server()` in `test_transport_protocol.py`.
- Report pattern: `_report(name, passed, detail)` — used in standalone scripts that print PASS/FAIL without pytest.
- `sys.path.insert(0, parent_dir)` at top of each file so tests resolve `tools.*`, `handlers.*` imports.

## Dependencies

### Internal
- `tools/transport.py` — transport constants and send function
- `tools/patch.py` — risk scoring, RBW guard
- `tools/envelope.py` — cache fingerprint logic
- `node_lifecycle_registry.py` — node risk/lifecycle data

<!-- MANUAL: -->
