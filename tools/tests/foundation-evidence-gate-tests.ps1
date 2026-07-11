[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$Root
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $Root 'tools\FoundationEvidenceGate.psm1') -Force

function Assert-Throws {
    param([scriptblock]$Operation, [string]$ExpectedMessage)
    try { & $Operation } catch {
        if ($_.Exception.Message -notlike "*$ExpectedMessage*") {
            throw "Unexpected failure: $($_.Exception.Message)"
        }
        return
    }
    throw "Expected failure containing: $ExpectedMessage"
}

$sandbox = Join-Path $Root 'artifacts\physics\foundation-evidence-gate-test'
New-Item -ItemType Directory -Force -Path $sandbox | Out-Null
$rootPrefix = [System.IO.Path]::GetFullPath($Root).TrimEnd('\', '/') + '\'
$relativeSandbox = [System.IO.Path]::GetFullPath($sandbox).Substring(
    $rootPrefix.Length).Replace('\', '/')
$reportPath = Join-Path $sandbox 'report.md'

$documents = @{}
foreach ($configuration in 'Debug', 'Release') {
    $path = Join-Path $sandbox "foundation-report-$($configuration.ToLowerInvariant()).json"
    Copy-Item -LiteralPath (Join-Path $Root "docs\physics\evidence\foundation-report-$($configuration.ToLowerInvariant()).json") `
        -Destination $path
    $documents[$configuration] = [ordered]@{
        Path = $path
        Relative = "$relativeSandbox/$([System.IO.Path]::GetFileName($path))"
        Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash
    }
}

function Write-EvidenceReport {
@"
# Audit report
Evidence-Debug-Path: $($documents.Debug.Relative)
Evidence-Debug-SHA256: $($documents.Debug.Hash)
Evidence-Release-Path: $($documents.Release.Relative)
Evidence-Release-SHA256: $($documents.Release.Hash)
"@ | Set-Content -LiteralPath $reportPath -Encoding utf8
}

function Assert-SemanticMutation {
    param([scriptblock]$Mutation, [string]$ExpectedMessage)
    Copy-Item -LiteralPath (Join-Path $Root 'docs\physics\evidence\foundation-report-debug.json') `
        -Destination $documents.Debug.Path -Force
    $mutated = [System.IO.File]::ReadAllText($documents.Debug.Path) | ConvertFrom-Json
    & $Mutation $mutated
    $mutated | ConvertTo-Json -Depth 100 |
        Set-Content -LiteralPath $documents.Debug.Path -Encoding utf8
    $documents.Debug.Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $documents.Debug.Path).Hash
    Write-EvidenceReport
    Assert-Throws {
        Assert-NinhoFoundationEvidence -Root $Root -ReportPath $reportPath
    } $ExpectedMessage
}

Write-EvidenceReport
Assert-NinhoFoundationEvidence -Root $Root -ReportPath $reportPath

Assert-SemanticMutation -ExpectedMessage 'topology mismatch' -Mutation {
    param($document)
    ($document.scenarios | Where-Object name -CEQ 'radial_fall').peak_body_count = 999
}
Assert-SemanticMutation -ExpectedMessage 'outside limits' -Mutation {
    param($document)
    $scenario = $document.scenarios | Where-Object name -CEQ 'radial_fall'
    $scenario.surface_separation = 999
    ($scenario.metrics | Where-Object name -CEQ 'surface_separation').value = 999
}
Assert-SemanticMutation -ExpectedMessage 'shape query capability proof mismatch' -Mutation {
    param($document)
    foreach ($matrix in @($document.matrix, ($document.scenarios | Where-Object name -CEQ 'capability_matrix').matrix)) {
        $row = $matrix | Where-Object capability -CEQ 'shape_cast_overlap'
        ($row.values | Where-Object name -CEQ 'first_handle_matches_overlap').value = 0
    }
}
Assert-SemanticMutation -ExpectedMessage 'duplicate capability matrices diverge' -Mutation {
    param($document)
    ($document.matrix | Where-Object capability -CEQ 'shape_cast_overlap').detail = 'falsified'
}
Assert-SemanticMutation -ExpectedMessage 'declared limit mismatch' -Mutation {
    param($document)
    $scenario = $document.scenarios | Where-Object name -CEQ 'radial_fall'
    ($scenario.limits | Where-Object name -CEQ 'final_linear_speed').value = 999
}
Assert-SemanticMutation -ExpectedMessage 'projectile metric/property mismatch' -Mutation {
    param($document)
    $scenario = $document.scenarios | Where-Object name -CEQ 'projectile_pile'
    ($scenario.metrics | Where-Object name -CEQ 'primary_seed_passes').value = 0
}
Assert-SemanticMutation -ExpectedMessage 'contact capability proof mismatch' -Mutation {
    param($document)
    foreach ($matrix in @($document.matrix, ($document.scenarios | Where-Object name -CEQ 'capability_matrix').matrix)) {
        $row = $matrix | Where-Object capability -CEQ 'contact_hit_events'
        ($row.values | Where-Object name -CEQ 'normal_length').value = 0
    }
}
Assert-SemanticMutation -ExpectedMessage 'hull capability proof mismatch' -Mutation {
    param($document)
    foreach ($matrix in @($document.matrix, ($document.scenarios | Where-Object name -CEQ 'capability_matrix').matrix)) {
        $row = $matrix | Where-Object capability -CEQ 'hulls_compounds'
        ($row.values | Where-Object name -CEQ 'bounds_lower_y').value = 999
    }
}
Assert-SemanticMutation -ExpectedMessage 'hull capability proof mismatch' -Mutation {
    param($document)
    foreach ($matrix in @($document.matrix, ($document.scenarios | Where-Object name -CEQ 'capability_matrix').matrix)) {
        $row = $matrix | Where-Object capability -CEQ 'hulls_compounds'
        foreach ($name in 'expected_mass', 'mass', 'bounds_lower_x', 'bounds_lower_y', 'bounds_lower_z', 'bounds_upper_x', 'bounds_upper_y', 'bounds_upper_z') {
            ($row.values | Where-Object name -CEQ $name).value = 999
        }
        ($row.values | Where-Object name -CEQ 'bounds_tolerance').value = 10000
    }
}

Add-Content -LiteralPath $documents.Debug.Path -Value ' ' -Encoding ascii
Assert-Throws {
    Assert-NinhoFoundationEvidence -Root $Root -ReportPath $reportPath
} 'SHA-256 mismatch'

Remove-Item -LiteralPath $sandbox -Recurse -Force
Write-Output 'foundation-evidence-gate-tests: PASS'
