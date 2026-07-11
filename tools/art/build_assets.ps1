[CmdletBinding()]
param(
    [string]$OutputRoot,
    [switch]$Clean
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $OutputRoot) { $OutputRoot = $projectRoot }
$OutputRoot = [IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
Import-Module (Join-Path $projectRoot 'tools\SafePath.psm1') -Force

$bootstrap = & (Join-Path $projectRoot 'tools\bootstrap.ps1') -CheckOnly -Json | ConvertFrom-Json
if (-not $bootstrap.ok) { throw ($bootstrap.errors -join '; ') }
$blender = $bootstrap.blender.path
$expectedHash = $bootstrap.blender.executable_sha256

if ($Clean) {
    foreach ($relative in 'art\source\vertical_slice','game\assets\vertical_slice') {
        $target = [IO.Path]::GetFullPath((Join-Path $OutputRoot $relative))
        if (-not $target.StartsWith($OutputRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            throw "refusing to clean outside output root: $target"
        }
        if (Test-Path -LiteralPath $target) {
            Assert-NinhoNoReparseAncestors -Path $target -AllowedRoot $OutputRoot | Out-Null
            Remove-Item -LiteralPath $target -Recurse -Force
        }
    }
    $manifest = Join-Path $OutputRoot 'tools\art\vertical_slice_asset_manifest.json'
    if (Test-Path -LiteralPath $manifest) {
        Assert-NinhoNoReparseAncestors -Path $manifest -AllowedRoot $OutputRoot | Out-Null
        Remove-Item -LiteralPath $manifest -Force
    }
}

$script = Join-Path $projectRoot 'art\scripts\build_vertical_slice.py'
& $blender --background --factory-startup --disable-autoexec --python-exit-code 1 --python $script -- `
    --project-root $projectRoot `
    --output-root $OutputRoot `
    --blender-executable-sha256 $expectedHash
if ($LASTEXITCODE -ne 0) { throw "Blender asset build failed with exit code $LASTEXITCODE" }

& (Join-Path $PSScriptRoot 'validate_assets.ps1') -OutputRoot $OutputRoot
if ($LASTEXITCODE -ne 0) { throw "asset validation failed with exit code $LASTEXITCODE" }
