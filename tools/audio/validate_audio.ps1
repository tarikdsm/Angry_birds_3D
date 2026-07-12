[CmdletBinding()]
param(
    [string]$OutputRoot,
    [string]$AllowedRoot
)

$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $OutputRoot) { $OutputRoot = $projectRoot }
$generator = Join-Path $PSScriptRoot 'generate_audio.py'
$arguments = @($generator, 'validate', '--output-root', $OutputRoot)
if ($AllowedRoot) { $arguments += @('--allowed-root', $AllowedRoot) }
& python @arguments
exit $LASTEXITCODE
