$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

$runner = [IO.File]::ReadAllText((Join-Path $root 'tools\test.ps1'))
$contractCall = "tests\bootstrap-test-integration-contract.ps1"
$smokeCall = "tests\bootstrap-smoke.ps1"
$behaviorCall = "tests\bootstrap-behavior.ps1"
$contractIndex = $runner.IndexOf($contractCall, [StringComparison]::Ordinal)
$smokeIndex = $runner.IndexOf($smokeCall, [StringComparison]::Ordinal)
$behaviorIndex = $runner.IndexOf($behaviorCall, [StringComparison]::Ordinal)
$firstToolchainConsumer = $runner.IndexOf("tests\test_generate_foundation_report.py", [StringComparison]::Ordinal)

Assert-True ($contractIndex -ge 0) 'Official gate must execute the bootstrap integration contract'
Assert-True ($smokeIndex -gt $contractIndex) 'Official gate must execute bootstrap-smoke.ps1'
Assert-True ($behaviorIndex -gt $smokeIndex) 'Official gate must execute bootstrap-behavior.ps1 after the lock smoke'
Assert-True ($firstToolchainConsumer -gt $behaviorIndex) `
    'Bootstrap tests must run before the rest of the official gate'

$smoke = [IO.File]::ReadAllText((Join-Path $root 'tools\tests\bootstrap-smoke.ps1'))
$behavior = [IO.File]::ReadAllText((Join-Path $root 'tools\tests\bootstrap-behavior.ps1'))
Assert-True ($smoke -match '&\s+\$bootstrap\s+-ValidateLock\s+-Json') `
    'Bootstrap smoke must remain a lock-only validation'
Assert-True ($smoke -notmatch '-Install(?:Portable|VisualStudio|ExportTemplates)') `
    'Bootstrap smoke must not install tools'
Assert-True ($behavior -match '\$fixtureBootstrap\s+-InstallPortable\s+-Json') `
    'Bootstrap behavior test must invoke only its isolated bootstrap fixture'
Assert-True ($behavior -match "GetTempPath\(\)") `
    'Bootstrap behavior fixtures must remain under the system temporary directory'
Assert-True ($behavior -notmatch 'Invoke-WebRequest') `
    'Bootstrap behavior test must not perform direct downloads'

Write-Output 'bootstrap test integration contract: PASS'
