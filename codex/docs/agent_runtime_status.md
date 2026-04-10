# Agent Runtime Status

Use `get_runtime_status()` before a Blueprint session whenever connectivity is in doubt.

Expected healthy signals:

- `listener_alive: true`
- `protocol_version_active` present
- `last_transport_error` empty

Common interpretations:

| Signal | Meaning | Next step |
|---|---|---|
| `TRANSPORT_DISCONNECTED` | UE Editor or socket listener is unreachable | Open the UE project, enable the plugin, and retry in a fresh Codex turn |
| `TRANSPORT_TIMEOUT` | UE is busy or blocked on the game thread | Wait briefly, then retry one read call |
| `queue_length: -1` | Queue telemetry is unavailable | Continue if other reads succeed |

Recommended read-first sequence:

1. `get_runtime_status`
2. `handshake_test`
3. `describe_blueprint("/Game/Blueprints/BP_NPCAIController")`

Treat runtime status as diagnostic guidance, not as the only source of truth. If `describe_blueprint` returns real graph data, the read path is usable even if queue telemetry is limited.
