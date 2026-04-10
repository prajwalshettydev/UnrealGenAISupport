# Codex Plugin Install

This package is installed from the GenerativeAISupport repository checkout at:

`__PLUGIN_SOURCE_ROOT__`

## Install

Run from the plugin repository root:

```powershell
powershell -ExecutionPolicy Bypass -File Scripts/install_codex_plugin.ps1
```

## After Install

1. Start a fresh Codex session.
2. Confirm the plugin is visible in Codex.
3. Use one of these entry points:
   - `$bp`
   - `$blueprint-plan`
   - `$blueprint-edit`
   - `$blueprint-qa`
   - `$blueprint-mcp-dev`

## Read-Path Smoke Test

Run these in a fresh session after UE is open:

1. `get_runtime_status`
2. `handshake_test`
3. `describe_blueprint("/Game/Blueprints/BP_NPCAIController")`

## Notes

- The installed Codex plugin points back to the repository checkout above; do not move or delete that checkout without reinstalling.
- Re-run the install script with `-Force` after updating the repository copy of `codex/`.
