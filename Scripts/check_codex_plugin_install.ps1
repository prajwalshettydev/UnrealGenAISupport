[CmdletBinding()]
param(
    [string]$CodexHome,
    [string]$PluginName = "generative-ai-support-blueprint"
)

$ErrorActionPreference = "Stop"

function Normalize-PluginName {
    param([Parameter(Mandatory = $true)][string]$Name)

    $normalized = $Name.ToLowerInvariant() -replace "[^a-z0-9]+", "-"
    $normalized = $normalized -replace "-{2,}", "-"
    return $normalized.Trim("-")
}

function Add-Result {
    param(
        [Parameter(Mandatory = $true)][string]$Check,
        [Parameter(Mandatory = $true)][string]$Status,
        [Parameter(Mandatory = $true)][string]$Detail
    )

    $script:results.Add([pscustomobject]@{
            Check  = $Check
            Status = $Status
            Detail = $Detail
        })
}

function Load-JsonObject {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json)
}

function Test-TcpPort {
    param(
        [string]$TargetHost,
        [int]$Port,
        [int]$TimeoutMs = 1500
    )

    $client = New-Object System.Net.Sockets.TcpClient
    try {
        $async = $client.BeginConnect($TargetHost, $Port, $null, $null)
        if (-not $async.AsyncWaitHandle.WaitOne($TimeoutMs, $false)) {
            return $false
        }

        $client.EndConnect($async)
        return $true
    }
    catch {
        return $false
    }
    finally {
        $client.Close()
    }
}

$script:results = New-Object System.Collections.Generic.List[object]

$pluginRepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$normalizedPluginName = Normalize-PluginName -Name $PluginName

if ([string]::IsNullOrWhiteSpace($CodexHome)) {
    if (-not [string]::IsNullOrWhiteSpace($env:CODEX_HOME)) {
        $CodexHome = $env:CODEX_HOME
    }
    else {
        $CodexHome = Join-Path $env:USERPROFILE ".codex"
    }
}

$CodexHome = [System.IO.Path]::GetFullPath($CodexHome)
$pluginInstallPath = Join-Path (Join-Path $CodexHome "plugins") $normalizedPluginName
$pluginJsonPath = Join-Path $pluginInstallPath ".codex-plugin\plugin.json"
$mcpConfigPath = Join-Path $pluginInstallPath ".mcp.json"
$installDocPath = Join-Path $pluginInstallPath "docs\codex-plugin-install.md"
$marketplacePath = Join-Path $env:USERPROFILE ".agents\plugins\marketplace.json"
$sourceMcpServerPath = Join-Path $pluginRepoRoot "Content\Python\mcp_server.py"
$expectedSourceRoot = $pluginRepoRoot -replace "\\", "/"

if (Test-Path -LiteralPath $pluginInstallPath) {
    Add-Result -Check "Plugin Directory" -Status "PASS" -Detail "Installed plugin directory exists at '$pluginInstallPath'."
}
else {
    Add-Result -Check "Plugin Directory" -Status "FAIL" -Detail "Missing installed plugin directory '$pluginInstallPath'."
}

if (Test-Path -LiteralPath $pluginJsonPath) {
    try {
        $pluginJson = Load-JsonObject -Path $pluginJsonPath
        if ($pluginJson.name -eq $normalizedPluginName) {
            Add-Result -Check "plugin.json" -Status "PASS" -Detail "plugin.json parsed and name matches '$normalizedPluginName'."
        }
        else {
            Add-Result -Check "plugin.json" -Status "FAIL" -Detail "plugin.json parsed but name is '$($pluginJson.name)' instead of '$normalizedPluginName'."
        }
    }
    catch {
        Add-Result -Check "plugin.json" -Status "FAIL" -Detail "plugin.json exists but could not be parsed: $($_.Exception.Message)"
    }
}
else {
    Add-Result -Check "plugin.json" -Status "FAIL" -Detail "Missing '$pluginJsonPath'."
}

if (Test-Path -LiteralPath $mcpConfigPath) {
    try {
        $mcpConfig = Load-JsonObject -Path $mcpConfigPath
        $server = $mcpConfig.mcpServers."unreal-handshake"
        if ($null -eq $server) {
            Add-Result -Check ".mcp.json" -Status "FAIL" -Detail "Installed .mcp.json exists but unreal-handshake is missing."
        }
        else {
            $argsJoined = (@($server.args) -join " ")
            if ($argsJoined -like "*$expectedSourceRoot*") {
                Add-Result -Check ".mcp.json" -Status "PASS" -Detail "Installed .mcp.json points at the current repository path."
            }
            else {
                Add-Result -Check ".mcp.json" -Status "FAIL" -Detail "Installed .mcp.json does not contain the expected source root '$expectedSourceRoot'."
            }
        }
    }
    catch {
        Add-Result -Check ".mcp.json" -Status "FAIL" -Detail ".mcp.json exists but could not be parsed: $($_.Exception.Message)"
    }
}
else {
    Add-Result -Check ".mcp.json" -Status "FAIL" -Detail "Missing '$mcpConfigPath'."
}

if (Test-Path -LiteralPath $marketplacePath) {
    try {
        $marketplace = Load-JsonObject -Path $marketplacePath
        $entry = @($marketplace.plugins) | Where-Object { $_.name -eq $normalizedPluginName } | Select-Object -First 1
        if ($null -eq $entry) {
            Add-Result -Check "Marketplace Entry" -Status "FAIL" -Detail "marketplace.json exists but has no entry for '$normalizedPluginName'."
        }
        else {
            $marketplaceDir = Split-Path -Path $marketplacePath -Parent
            $resolvedSourcePath = [System.IO.Path]::GetFullPath((Join-Path $marketplaceDir $entry.source.path))
            if ($resolvedSourcePath -eq [System.IO.Path]::GetFullPath($pluginInstallPath)) {
                Add-Result -Check "Marketplace Entry" -Status "PASS" -Detail "marketplace.json points '$normalizedPluginName' at the installed plugin path."
            }
            else {
                Add-Result -Check "Marketplace Entry" -Status "FAIL" -Detail "marketplace.json entry resolves to '$resolvedSourcePath' instead of '$pluginInstallPath'."
            }
        }
    }
    catch {
        Add-Result -Check "Marketplace Entry" -Status "FAIL" -Detail "marketplace.json exists but could not be parsed: $($_.Exception.Message)"
    }
}
else {
    Add-Result -Check "Marketplace Entry" -Status "FAIL" -Detail "Missing '$marketplacePath'."
}

if (Test-Path -LiteralPath $installDocPath) {
    $installDoc = Get-Content -LiteralPath $installDocPath -Raw
    if ($installDoc -match "Start a fresh Codex session") {
        Add-Result -Check "Install Doc" -Status "PASS" -Detail "Install doc includes the fresh-session requirement."
    }
    else {
        Add-Result -Check "Install Doc" -Status "FAIL" -Detail "Install doc exists but does not include the fresh-session requirement."
    }
}
else {
    Add-Result -Check "Install Doc" -Status "FAIL" -Detail "Missing '$installDocPath'."
}

if (Test-Path -LiteralPath $sourceMcpServerPath) {
    Add-Result -Check "Source MCP Server" -Status "PASS" -Detail "Found '$sourceMcpServerPath'."
}
else {
    Add-Result -Check "Source MCP Server" -Status "FAIL" -Detail "Missing '$sourceMcpServerPath'."
}

if (Test-TcpPort -TargetHost "127.0.0.1" -Port 9877) {
    Add-Result -Check "UE Socket Port" -Status "PASS" -Detail "127.0.0.1:9877 is reachable."
}
else {
    Add-Result -Check "UE Socket Port" -Status "WARN" -Detail "127.0.0.1:9877 is not reachable. Installation is still valid, but UE may not be running."
}

$results | Format-Table -AutoSize

$hasFail = $results.Status -contains "FAIL"
$hasWarn = $results.Status -contains "WARN"

if ($hasFail) {
    exit 1
}

if ($hasWarn) {
    exit 2
}

exit 0
