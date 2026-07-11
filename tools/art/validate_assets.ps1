[CmdletBinding()]
param([string]$OutputRoot)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $OutputRoot) { $OutputRoot = $projectRoot }
$OutputRoot = [IO.Path]::GetFullPath($OutputRoot)

$bootstrap = & (Join-Path $projectRoot 'tools\bootstrap.ps1') -CheckOnly -Json | ConvertFrom-Json
if (-not $bootstrap.ok) { throw ($bootstrap.errors -join '; ') }
$blender = $bootstrap.blender.path
$expectedHash = $bootstrap.blender.executable_sha256
$script = Join-Path $projectRoot 'art\scripts\build_vertical_slice.py'

& $blender --background --factory-startup --disable-autoexec --python-exit-code 1 --python $script -- `
    --project-root $projectRoot `
    --output-root $OutputRoot `
    --blender-executable-sha256 $expectedHash `
    --validate
if ($LASTEXITCODE -ne 0) { throw "Blender asset validation failed with exit code $LASTEXITCODE" }
