[CmdletBinding()]
param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $Root 'tools\GodotSmokeRegistry.psm1') -Force

function Assert-True {
    param([bool]$Condition, [string]$Message)

    if (-not $Condition) {
        throw $Message
    }
}

$package = [IO.File]::ReadAllText((Join-Path $Root 'tools\package_windows.ps1'))
$project = [IO.File]::ReadAllText((Join-Path $Root 'game\project.godot'))
$registry = @(Get-NinhoGodotSmokeRegistry)
$verticalSliceSmoke = @($registry | Where-Object Name -ceq 'vertical-slice-smoke')

Assert-True ($project -match '(?m)^common/physics_ticks_per_second=60\r?$') `
    'The packaged cadence contract requires the authoritative 60 Hz physics setting'
Assert-True ($verticalSliceSmoke.Count -eq 1 -and
        [int]$verticalSliceSmoke[0].FixedFps -eq 60) `
    'The development vertical-slice smoke must retain deterministic fixed-FPS execution'

$launchStart = $package.IndexOf(
    '$launchProcess = Start-Process', [StringComparison]::Ordinal)
$launchEnd = $package.IndexOf(
    ') -PassThru -WindowStyle Hidden', $launchStart, [StringComparison]::Ordinal)
Assert-True ($launchStart -ge 0 -and $launchEnd -gt $launchStart) `
    'Could not locate the packaged launch argument block'
$launchBlock = $package.Substring($launchStart, $launchEnd - $launchStart)
Assert-True ($launchBlock.Contains('res://tests/vertical_slice_smoke.gd')) `
    'The packaged launch must execute the vertical-slice smoke'
Assert-True (-not $launchBlock.Contains('--fixed-fps')) `
    'The packaged launch must exercise normal engine cadence, not fixed-FPS test mode'
Assert-True ($package.Contains(
        'Exercise the packaged runtime at normal engine cadence.')) `
    'The deliberate package/development cadence split must be documented in code'
Assert-True ($package.Contains('$launchProcess.WaitForExit(120000)')) `
    'Normal-cadence packaged validation must retain its bounded timeout'
Assert-True ($package -match
    "\$launchLog -match '\(\?im\)\^\\s\*\(SCRIPT ERROR\|ERROR\|WARNING\):'") `
    'The packaged launch must reject warnings and errors from normal-cadence shutdown'

Write-Output 'package-launch-cadence-contract-tests: PASS'
