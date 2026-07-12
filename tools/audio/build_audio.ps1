[CmdletBinding()]
param(
    [string]$OutputRoot,
    [string]$AllowedRoot,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $OutputRoot) { $OutputRoot = $projectRoot }
$generator = Join-Path $PSScriptRoot 'generate_audio.py'
$arguments = @($generator, 'build', '--output-root', $OutputRoot)
if ($AllowedRoot) { $arguments += @('--allowed-root', $AllowedRoot) }
if ($Clean) { $arguments += '--clean' }
& python @arguments
exit $LASTEXITCODE
