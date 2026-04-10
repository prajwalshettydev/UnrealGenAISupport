[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$pluginRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$cwd = [System.IO.Path]::GetFullPath((Get-Location).Path)
$mcpConfigPath = Join-Path $pluginRoot ".mcp.json"
$mcpServerPath = Join-Path $pluginRoot "Content\Python\mcp_server.py"
$claudeConfigPath = Join-Path $env:USERPROFILE "AppData\Local\Claude\claude_desktop_config.json"
$hostName = "127.0.0.1"
$port = 9877

$results = New-Object System.Collections.Generic.List[object]

function Add-Result {
    param(
        [string]$Check,
        [string]$Status,
        [string]$Detail
    )

    $results.Add([pscustomobject]@{
            Check  = $Check
            Status = $Status
            Detail = $Detail
        })
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

function Read-JsonFile {
    param([string]$Path)

    Get-Content -Path $Path -Raw | ConvertFrom-Json
}

if ($cwd -eq $pluginRoot) {
    Add-Result -Check "Workspace Root" -Status "PASS" -Detail "Current working directory is the plugin root."
}
else {
    Add-Result -Check "Workspace Root" -Status "FAIL" -Detail "Current working directory is '$cwd'. Open or switch to '$pluginRoot'."
}

if (Test-Path -LiteralPath $mcpConfigPath) {
    try {
        $mcpConfig = Read-JsonFile -Path $mcpConfigPath
        if ($null -ne $mcpConfig.mcpServers."unreal-handshake") {
            Add-Result -Check "Plugin .mcp.json" -Status "PASS" -Detail "Found plugin-level unreal-handshake configuration."
        }
        else {
            Add-Result -Check "Plugin .mcp.json" -Status "FAIL" -Detail "File exists but unreal-handshake is missing."
        }
    }
    catch {
        Add-Result -Check "Plugin .mcp.json" -Status "FAIL" -Detail "File exists but could not be parsed as JSON: $($_.Exception.Message)"
    }
}
else {
    Add-Result -Check "Plugin .mcp.json" -Status "FAIL" -Detail "Missing '$mcpConfigPath'."
}

if (Test-Path -LiteralPath $mcpServerPath) {
    Add-Result -Check "mcp_server.py" -Status "PASS" -Detail "Found '$mcpServerPath'."
}
else {
    Add-Result -Check "mcp_server.py" -Status "FAIL" -Detail "Missing '$mcpServerPath'."
}

if (Test-Path -LiteralPath $claudeConfigPath) {
    try {
        $claudeConfig = Read-JsonFile -Path $claudeConfigPath
        if ($null -ne $claudeConfig.mcpServers."unreal-handshake") {
            Add-Result -Check "Claude Desktop Config" -Status "PASS" -Detail "Claude Desktop config contains unreal-handshake."
        }
        else {
            Add-Result -Check "Claude Desktop Config" -Status "WARN" -Detail "Claude Desktop config exists but unreal-handshake is missing."
        }
    }
    catch {
        Add-Result -Check "Claude Desktop Config" -Status "WARN" -Detail "Claude Desktop config exists but could not be parsed: $($_.Exception.Message)"
    }
}
else {
    Add-Result -Check "Claude Desktop Config" -Status "WARN" -Detail "Claude Desktop config not found at '$claudeConfigPath'."
}

if (Test-TcpPort -TargetHost $hostName -Port $port) {
    Add-Result -Check "UE Socket Port" -Status "PASS" -Detail "$hostName`:$port is reachable."
}
else {
    Add-Result -Check "UE Socket Port" -Status "WARN" -Detail "$hostName`:$port is not reachable. Unreal Editor or the UE-side socket server may not be running."
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
