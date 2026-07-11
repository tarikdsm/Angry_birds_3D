[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$Root
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $Root 'tools\SpikeReportGate.psm1') -Force

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        throw $Message
    }
}

function Assert-Throws {
    param([scriptblock]$Operation, [string]$ExpectedMessage)
    try {
        & $Operation
    } catch {
        if ($_.Exception.Message -notlike "*$ExpectedMessage*") {
            throw "Unexpected failure: $($_.Exception.Message)"
        }
        return
    }
    throw "Expected failure containing: $ExpectedMessage"
}

$sandbox = Join-Path $Root 'artifacts\physics\spike-report-gate-test'
New-Item -ItemType Directory -Force -Path $sandbox | Out-Null
$target = Join-Path $sandbox 'report.json'

Set-Content -LiteralPath $target -Value '{"stale":true}' -Encoding utf8
$started = Start-NinhoSpikeReportCapture -Path $target -AllowedRoot $sandbox
Assert-True (-not (Test-Path -LiteralPath $target)) 'capture did not remove stale report'
Assert-Throws {
    Read-NinhoFreshSpikeReport -Path $target -StartedUtc $started -ExpectedBuildType Debug
} 'did not create a report'

Set-Content -LiteralPath $target -Value '{"ok":true}' -Encoding utf8
(Get-Item -LiteralPath $target).LastWriteTimeUtc = $started.AddMinutes(-1)
Assert-Throws {
    Read-NinhoFreshSpikeReport -Path $target -StartedUtc $started -ExpectedBuildType Debug
} 'predates this execution'

Copy-Item -LiteralPath (Join-Path $Root 'docs\physics\evidence\foundation-report-debug.json') `
    -Destination $target -Force
(Get-Item -LiteralPath $target).LastWriteTimeUtc = [DateTime]::UtcNow
$document = Read-NinhoFreshSpikeReport `
    -Path $target -StartedUtc $started -ExpectedBuildType Debug
Assert-True ($document.schema -eq 'ninho.physics.scenario.v1') 'fresh report did not validate'

Set-Content -LiteralPath $target -Value '{"ok":true}' -Encoding utf8
Assert-Throws {
    Read-NinhoFreshSpikeReport -Path $target -StartedUtc $started -ExpectedBuildType Debug
} 'missing schema'

Assert-Throws {
    Start-NinhoSpikeReportCapture `
        -Path (Join-Path (Split-Path $sandbox -Parent) 'outside.json') `
        -AllowedRoot $sandbox
} 'outside the allowed root'

$junctionTestRoot = Join-Path ([System.IO.Path]::GetTempPath()) "ninho-spike-reparse-$PID"
$junctionAllowedRoot = Join-Path $junctionTestRoot 'artifacts'
$external = Join-Path $junctionTestRoot 'external'
$junction = Join-Path $junctionAllowedRoot 'physics'
New-Item -ItemType Directory -Force -Path $junctionAllowedRoot, $external | Out-Null
Set-Content -LiteralPath (Join-Path $external 'marker.txt') -Value 'keep' -Encoding ascii
Set-Content -LiteralPath (Join-Path $external 'report.json') -Value '{}' -Encoding ascii
New-Item -ItemType Junction -Path $junction -Target $external | Out-Null
Assert-Throws {
    Start-NinhoSpikeReportCapture `
        -Path (Join-Path $junction 'report.json') `
        -AllowedRoot $junctionAllowedRoot
} 'reparse point'
Assert-True (Test-Path -LiteralPath (Join-Path $external 'marker.txt')) `
    'external junction target was modified'
[System.IO.Directory]::Delete($junction)
Remove-Item -LiteralPath $junctionTestRoot -Recurse -Force

Remove-Item -LiteralPath $target -Force
Write-Output 'spike-report-gate-tests: PASS'
