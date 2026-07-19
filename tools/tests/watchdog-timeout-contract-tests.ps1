[CmdletBinding()]
param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path,

    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug'
)

$ErrorActionPreference = 'Stop'

function Assert-True {
    param([bool]$Condition, [string]$Message)

    if (-not $Condition) {
        throw $Message
    }
}

function Get-TestProperty {
    param(
        [Parameter(Mandatory)]$Test,
        [Parameter(Mandatory)][string]$Name
    )

    return $Test.properties |
        Where-Object { $_.name -ceq $Name } |
        Select-Object -ExpandProperty value -First 1
}

$bootstrap = & (Join-Path $Root 'tools\bootstrap.ps1') -CheckOnly -Json |
    ConvertFrom-Json
Assert-True ([bool]$bootstrap.ok) 'Pinned toolchain must be available'
$ctest = Join-Path (Split-Path -Parent ([string]$bootstrap.cmake.path)) 'ctest.exe'
Assert-True (Test-Path -LiteralPath $ctest -PathType Leaf) `
    "Pinned CTest executable missing: $ctest"

$preset = $Configuration.ToLowerInvariant()
$buildDirectory = Join-Path $Root "build\$preset"
$rawInventory = & $ctest --test-dir $buildDirectory --show-only=json-v1
Assert-True ($LASTEXITCODE -eq 0) `
    "CTest inventory failed for $Configuration"
$inventory = $rawInventory -join "`n" | ConvertFrom-Json

$stress = $inventory.tests | Where-Object { $_.name -ceq 'stress_protocol' }
$spikeJsonSmoke = $inventory.tests |
    Where-Object { $_.name -ceq 'spike_json_smoke' }
$scenarioWatchdog = $inventory.tests |
    Where-Object { $_.name -ceq 'scenario_watchdog' }
Assert-True ($null -ne $stress) 'stress_protocol must be registered'
Assert-True ($null -ne $spikeJsonSmoke) 'spike_json_smoke must be registered'
Assert-True ($null -ne $scenarioWatchdog) 'scenario_watchdog must be registered'

$expectedStressTimeout = if ($Configuration -ceq 'Debug') { 185.0 } else { 65.0 }
Assert-True ((Get-TestProperty -Test $stress -Name 'TIMEOUT') -eq $expectedStressTimeout) `
    "stress_protocol $Configuration timeout must be $expectedStressTimeout seconds"
Assert-True ((Get-TestProperty -Test $scenarioWatchdog -Name 'TIMEOUT') -eq 65.0) `
    'scenario_watchdog must retain the short 65 second CTest bound'

$scenarioWatchdogGraceSeconds = 5.0
$smokeStressRepeatCount = 2.0
$baselineSmokeTimeoutSeconds = 150.0
$stressWatchdogSeconds = $expectedStressTimeout - $scenarioWatchdogGraceSeconds
$standardWatchdogSeconds = 65.0 - $scenarioWatchdogGraceSeconds
$expectedSmokeTimeout = $baselineSmokeTimeoutSeconds +
    $smokeStressRepeatCount * ($stressWatchdogSeconds - $standardWatchdogSeconds)
Assert-True ((Get-TestProperty -Test $spikeJsonSmoke -Name 'TIMEOUT') -eq $expectedSmokeTimeout) `
    "spike_json_smoke $Configuration timeout must be $expectedSmokeTimeout seconds"

foreach ($negativeName in @(
        'runner_rejects_no_match',
        'runner_rejects_missing_filter_value',
        'runner_rejects_empty_filter_value',
        'runner_rejects_unknown_option',
        'runner_rejects_extra_arguments'
    )) {
    $negative = $inventory.tests | Where-Object { $_.name -ceq $negativeName }
    Assert-True ($null -ne $negative) "$negativeName must be registered"
    Assert-True ([bool](Get-TestProperty -Test $negative -Name 'WILL_FAIL')) `
        "$negativeName must retain WILL_FAIL semantics"
}

Write-Output "watchdog-timeout-contract-tests ($Configuration): PASS"
