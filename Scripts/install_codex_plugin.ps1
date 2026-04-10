[CmdletBinding()]
param(
    [string]$CodexHome,
    [string]$PluginName = "generative-ai-support-blueprint",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

function Normalize-PluginName {
    param([Parameter(Mandatory = $true)][string]$Name)

    $normalized = $Name.ToLowerInvariant() -replace "[^a-z0-9]+", "-"
    $normalized = $normalized -replace "-{2,}", "-"
    $normalized = $normalized.Trim("-")

    if ([string]::IsNullOrWhiteSpace($normalized)) {
        throw "PluginName must contain at least one letter or digit."
    }

    if ($normalized.Length -gt 64) {
        throw "Normalized plugin name '$normalized' is longer than 64 characters."
    }

    return $normalized
}

function Ensure-Directory {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Write-Utf8File {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Content
    )

    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Content, $utf8NoBom)
}

function Get-RelativePathNormalized {
    param(
        [Parameter(Mandatory = $true)][string]$FromDirectory,
        [Parameter(Mandatory = $true)][string]$ToPath
    )

    $trimChars = [char[]]@('\', '/')
    $fromFull = [System.IO.Path]::GetFullPath($FromDirectory)
    $toFull = [System.IO.Path]::GetFullPath($ToPath)

    $fromUri = [System.Uri]::new(($fromFull.TrimEnd($trimChars) + [System.IO.Path]::DirectorySeparatorChar))
    $toUri = [System.Uri]::new(($toFull.TrimEnd($trimChars) + [System.IO.Path]::DirectorySeparatorChar))
    return $fromUri.MakeRelativeUri($toUri).ToString().TrimEnd("/")
}

function Load-JsonObject {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json)
}

function Save-JsonObject {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Object
    )

    $json = $Object | ConvertTo-Json -Depth 32
    Write-Utf8File -Path $Path -Content ($json + "`n")
}

$pluginRepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$distributionRoot = Join-Path $pluginRepoRoot "codex"
$mcpServerPath = Join-Path $pluginRepoRoot "Content\Python\mcp_server.py"

if (-not (Test-Path -LiteralPath $distributionRoot)) {
    throw "Missing Codex distribution directory: $distributionRoot"
}

if (-not (Test-Path -LiteralPath $mcpServerPath)) {
    throw "Missing MCP server entry point: $mcpServerPath"
}

if ([string]::IsNullOrWhiteSpace($CodexHome)) {
    if (-not [string]::IsNullOrWhiteSpace($env:CODEX_HOME)) {
        $CodexHome = $env:CODEX_HOME
    }
    else {
        $CodexHome = Join-Path $env:USERPROFILE ".codex"
    }
}

$CodexHome = [System.IO.Path]::GetFullPath($CodexHome)
$normalizedPluginName = Normalize-PluginName -Name $PluginName
$pluginInstallParent = Join-Path $CodexHome "plugins"
$pluginInstallPath = Join-Path $pluginInstallParent $normalizedPluginName
$marketplaceDir = Join-Path $env:USERPROFILE ".agents\plugins"
$marketplacePath = Join-Path $marketplaceDir "marketplace.json"

Ensure-Directory -Path $pluginInstallParent
Ensure-Directory -Path $marketplaceDir

if (Test-Path -LiteralPath $pluginInstallPath) {
    if (-not $Force) {
        Write-Host "Codex plugin already installed at: $pluginInstallPath"
    }
    else {
        $resolvedInstallPath = [System.IO.Path]::GetFullPath($pluginInstallPath)
        $resolvedParent = [System.IO.Path]::GetFullPath($pluginInstallParent)
        if (-not $resolvedInstallPath.StartsWith($resolvedParent, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to remove install path outside CodexHome/plugins: $resolvedInstallPath"
        }

        Remove-Item -LiteralPath $pluginInstallPath -Recurse -Force
    }
}

if (-not (Test-Path -LiteralPath $pluginInstallPath)) {
    Ensure-Directory -Path $pluginInstallPath

    Get-ChildItem -LiteralPath $distributionRoot -Force | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $pluginInstallPath -Recurse -Force
    }
}

$pluginSourceRootForJson = $pluginRepoRoot -replace "\\", "/"

$mcpTemplatePath = Join-Path $pluginInstallPath ".mcp.json.template"
$mcpOutputPath = Join-Path $pluginInstallPath ".mcp.json"
if (-not (Test-Path -LiteralPath $mcpTemplatePath)) {
    throw "Installed plugin is missing .mcp.json.template at $mcpTemplatePath"
}

$mcpContent = Get-Content -LiteralPath $mcpTemplatePath -Raw
$mcpContent = $mcpContent.Replace("__PLUGIN_SOURCE_ROOT__", $pluginSourceRootForJson)
Write-Utf8File -Path $mcpOutputPath -Content ($mcpContent + "`n")

$renderTargets = @(
    (Join-Path $pluginInstallPath ".codex-plugin\plugin.json"),
    (Join-Path $pluginInstallPath "docs\codex-plugin-install.md")
)

foreach ($target in $renderTargets) {
    if (-not (Test-Path -LiteralPath $target)) {
        throw "Installed plugin is missing required render target: $target"
    }

    $content = Get-Content -LiteralPath $target -Raw
    $content = $content.Replace("__PLUGIN_NAME__", $normalizedPluginName)
    $content = $content.Replace("__PLUGIN_SOURCE_ROOT__", $pluginSourceRootForJson)
    Write-Utf8File -Path $target -Content $content
}

if (Test-Path -LiteralPath $marketplacePath) {
    $marketplace = Load-JsonObject -Path $marketplacePath
}
else {
    $marketplace = [ordered]@{
        name = "local-codex-plugins"
        interface = [ordered]@{
            displayName = "Local Codex Plugins"
        }
        plugins = @()
    }
}

if ($null -eq $marketplace.interface) {
    $marketplace | Add-Member -NotePropertyName interface -NotePropertyValue ([ordered]@{
            displayName = "Local Codex Plugins"
        }) -Force
}

if ($null -eq $marketplace.plugins) {
    $marketplace | Add-Member -NotePropertyName plugins -NotePropertyValue @() -Force
}

$relativePluginPath = Get-RelativePathNormalized -FromDirectory $marketplaceDir -ToPath $pluginInstallPath
$relativePluginPath = $relativePluginPath -replace "\\", "/"

$entry = [ordered]@{
    name = $normalizedPluginName
    source = [ordered]@{
        source = "local"
        path = $relativePluginPath
    }
    policy = [ordered]@{
        installation = "AVAILABLE"
        authentication = "ON_INSTALL"
    }
    category = "Developer Tools"
}

$updatedPlugins = @()
$existingReplaced = $false

foreach ($plugin in @($marketplace.plugins)) {
    if ($plugin.name -eq $normalizedPluginName) {
        $updatedPlugins += [pscustomobject]$entry
        $existingReplaced = $true
    }
    else {
        $updatedPlugins += $plugin
    }
}

if (-not $existingReplaced) {
    $updatedPlugins += [pscustomobject]$entry
}

$marketplace.plugins = @($updatedPlugins)
Save-JsonObject -Path $marketplacePath -Object $marketplace

Write-Host ""
Write-Host "Codex plugin installed."
Write-Host "Plugin name: $normalizedPluginName"
Write-Host "Install path: $pluginInstallPath"
Write-Host "Marketplace: $marketplacePath"
Write-Host "Source repo path: $pluginRepoRoot"
Write-Host ""
Write-Host "Next steps:"
Write-Host "1. Start a fresh Codex session."
Write-Host "2. Use `$bp or the explicit Blueprint skills."
Write-Host "3. Run Scripts/check_codex_plugin_install.ps1 to verify the install."
