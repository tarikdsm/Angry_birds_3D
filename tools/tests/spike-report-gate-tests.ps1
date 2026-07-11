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
    Read-NinhoFreshSpikeReport -Path $target -StartedUtc $started
} 'did not create a report'

Set-Content -LiteralPath $target -Value '{"ok":true}' -Encoding utf8
(Get-Item -LiteralPath $target).LastWriteTimeUtc = $started.AddMinutes(-1)
Assert-Throws {
    Read-NinhoFreshSpikeReport -Path $target -StartedUtc $started
} 'predates this execution'

Set-Content -LiteralPath $target -Value '{"ok":true}' -Encoding utf8
$document = Read-NinhoFreshSpikeReport -Path $target -StartedUtc $started
Assert-True ($document.ok -eq $true) 'fresh report did not parse'

Assert-Throws {
    Start-NinhoSpikeReportCapture `
        -Path (Join-Path (Split-Path $sandbox -Parent) 'outside.json') `
        -AllowedRoot $sandbox
} 'outside the allowed root'

Remove-Item -LiteralPath $target -Force
Write-Output 'spike-report-gate-tests: PASS'
