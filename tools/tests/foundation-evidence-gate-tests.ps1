[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$Root
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $Root 'tools\FoundationEvidenceGate.psm1') -Force
Import-Module (Join-Path $Root 'tools\SafePath.psm1') -Force

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

$repositoryRoot = $Root
$artifactRoot = Join-Path $repositoryRoot 'artifacts\physics'
Assert-NinhoNoReparseAncestors -Path $artifactRoot -AllowedRoot $repositoryRoot | Out-Null
New-Item -ItemType Directory -Force -Path $artifactRoot | Out-Null
$sandbox = Join-Path $artifactRoot "foundation-evidence-gate-$([guid]::NewGuid().ToString('N'))"
Assert-NinhoNoReparseAncestors -Path $sandbox -AllowedRoot $artifactRoot | Out-Null
New-Item -ItemType Directory -Force -Path $sandbox | Out-Null
$gateRoot = $sandbox
$evidenceDirectory = Join-Path $gateRoot 'docs\physics\evidence'
Assert-NinhoNoReparseAncestors -Path $evidenceDirectory -AllowedRoot $gateRoot | Out-Null
New-Item -ItemType Directory -Force -Path $evidenceDirectory | Out-Null
$reportPath = Join-Path $gateRoot 'docs\physics\box3d-spike-report.md'

$documents = @{}
foreach ($configuration in 'Debug', 'Release') {
    $relative = "docs/physics/evidence/foundation-report-$($configuration.ToLowerInvariant()).json"
    $path = Join-Path $gateRoot $relative
    Copy-Item -LiteralPath (Join-Path $repositoryRoot $relative) `
        -Destination $path
    $documents[$configuration] = [ordered]@{
        Path = $path
        Relative = $relative
        Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash
    }
}
& git -C $gateRoot init --quiet
& git -c "safe.directory=$gateRoot" -C $gateRoot add -- docs/physics/evidence/foundation-report-debug.json `
    docs/physics/evidence/foundation-report-release.json
if ($LASTEXITCODE -ne 0) { throw 'failed to create tracked evidence fixture' }

function Write-EvidenceReport {
    param([switch]$AllowInvalidEvidence)

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & python (Join-Path $repositoryRoot 'tools\generate_foundation_report.py') `
            --root $gateRoot --write 2>&1 | Out-Null
        $generatorExitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    if ($generatorExitCode -eq 0) { return }
    if (-not $AllowInvalidEvidence) {
        throw 'failed to generate evidence report fixture'
    }
    $report = [System.IO.File]::ReadAllText($reportPath)
    foreach ($name in 'Debug', 'Release') {
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $documents[$name].Path).Hash
        $report = [regex]::Replace(
            $report,
            "(?m)^Evidence-$name-SHA256:\s*[0-9A-Fa-f]{64}\s*$",
            "Evidence-$name-SHA256: $hash")
    }
    [System.IO.File]::WriteAllText($reportPath, $report)
}

function Assert-SemanticMutation {
    param(
        [scriptblock]$Mutation,
        [string]$ExpectedMessage,
        [ValidateSet('Debug', 'Release')]
        [string]$Configuration = 'Debug'
    )
    foreach ($name in 'Debug', 'Release') {
        Copy-Item -LiteralPath (Join-Path $repositoryRoot "docs\physics\evidence\foundation-report-$($name.ToLowerInvariant()).json") `
            -Destination $documents[$name].Path -Force
        $documents[$name].Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $documents[$name].Path).Hash
    }
    Write-EvidenceReport
    $mutated = [System.IO.File]::ReadAllText($documents[$Configuration].Path) | ConvertFrom-Json
    & $Mutation $mutated
    $mutated | ConvertTo-Json -Depth 100 |
        Set-Content -LiteralPath $documents[$Configuration].Path -Encoding utf8
    $documents[$Configuration].Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $documents[$Configuration].Path).Hash
    Write-EvidenceReport -AllowInvalidEvidence
    Assert-Throws {
        Assert-NinhoFoundationEvidence -Root $gateRoot -ReportPath $reportPath
    } $ExpectedMessage
}

Write-EvidenceReport
Assert-NinhoFoundationEvidence -Root $gateRoot -ReportPath $reportPath

$validReport = [System.IO.File]::ReadAllText($reportPath)
[System.IO.File]::WriteAllText($reportPath, $validReport + "`nmanual markdown drift`n")
Assert-Throws {
    Assert-NinhoFoundationEvidence -Root $gateRoot -ReportPath $reportPath
} 'generated report is out of date'
[System.IO.File]::WriteAllText($reportPath, $validReport)
foreach ($mutation in @(
        @('Matrix-Debug-Hash: 17104053157009575930', 'Matrix-Debug-Hash: 1'),
        @('Matrix-Debug-Topology: 123/123/1;121/221', 'Matrix-Debug-Topology: 81/81/0;80/204'),
        @('Recommendation-Debug: prosseguir_com_limites', 'Recommendation-Debug: bloquear'))) {
    [System.IO.File]::WriteAllText($reportPath, $validReport.Replace($mutation[0], $mutation[1]))
    Assert-Throws {
        Assert-NinhoFoundationEvidence -Root $gateRoot -ReportPath $reportPath
    } 'Foundation report'
}
[System.IO.File]::WriteAllText($reportPath, $validReport)

Write-EvidenceReport
[System.IO.File]::WriteAllText(
    $reportPath,
    ([System.IO.File]::ReadAllText($reportPath)).Replace(
        'Evidence-Debug-Path: docs/physics/evidence/foundation-report-debug.json',
        'Evidence-Debug-Path: artifacts/physics/not-canonical.json'))
Assert-Throws {
    Assert-NinhoFoundationEvidence -Root $gateRoot -ReportPath $reportPath
} 'must be exactly docs/physics/evidence/foundation-report-debug.json'
Write-EvidenceReport
& git -c "safe.directory=$gateRoot" -C $gateRoot rm --cached --quiet -- $documents.Debug.Relative
Assert-Throws {
    Assert-NinhoFoundationEvidence -Root $gateRoot -ReportPath $reportPath
} 'is not tracked by git'
& git -c "safe.directory=$gateRoot" -C $gateRoot add -- $documents.Debug.Relative
if ($LASTEXITCODE -ne 0) { throw 'failed to restore tracked evidence fixture' }

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
        ($row.values | Where-Object name -CEQ 'box3d_allocator_final_bytes').value = 1
    }
}
Assert-SemanticMutation -ExpectedMessage 'value or unit mismatch' -Mutation {
    param($document)
    foreach ($matrix in @($document.matrix, ($document.scenarios | Where-Object name -CEQ 'capability_matrix').matrix)) {
        $row = $matrix | Where-Object capability -CEQ 'shape_cast_overlap'
        ($row.values | Where-Object name -CEQ 'cast_distance').unit = 'cm'
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
Assert-SemanticMutation -ExpectedMessage 'memory observation missing' -Mutation {
    param($document)
    $observation = ($document.scenarios | Where-Object name -CEQ 'stress').repeat_observations[0]
    $observation.PSObject.Properties.Remove('memory')
}
Assert-SemanticMutation -ExpectedMessage 'requires exactly ten warmup and measured samples' -Mutation {
    param($document)
    $private = ($document.scenarios | Where-Object name -CEQ 'stress').repeat_observations[0].memory.private_commit
    $private.warmup_samples = @($private.warmup_samples | Select-Object -First 9)
}
Assert-SemanticMutation -ExpectedMessage 'derived summary mismatch' -Mutation {
    param($document)
    $private = ($document.scenarios | Where-Object name -CEQ 'stress').repeat_observations[0].memory.private_commit
    $private.baseline_median_bytes += 1
}
Assert-SemanticMutation -ExpectedMessage 'derived summary mismatch' -Mutation {
    param($document)
    $private = ($document.scenarios | Where-Object name -CEQ 'stress').repeat_observations[0].memory.private_commit
    $private.stable = -not $private.stable
}
Assert-SemanticMutation -ExpectedMessage 'derived summary mismatch' -Mutation {
    param($document)
    $memory = ($document.scenarios | Where-Object name -CEQ 'stress').repeat_observations[0].memory
    $memory.assessment_status = 'pass'
}
Assert-SemanticMutation -ExpectedMessage 'derived summary mismatch' -Mutation {
    param($document)
    $working = ($document.scenarios | Where-Object name -CEQ 'stress').repeat_observations[0].memory.working_set
    $working.instant_growth_ratio += 0.1
}
Assert-SemanticMutation -ExpectedMessage 'derived summary mismatch' -Mutation {
    param($document)
    $private = ($document.scenarios | Where-Object name -CEQ 'stress').repeat_observations[0].memory.private_commit
    $private.instant_growth_ratio += 0.1
}
Assert-SemanticMutation -ExpectedMessage 'stress metric/memory mismatch' -Mutation {
    param($document)
    $stress = $document.scenarios | Where-Object name -CEQ 'stress'
    ($stress.metrics | Where-Object name -CEQ 'private_commit_growth_ratio').value = 999
}
Assert-SemanticMutation -ExpectedMessage 'derived summary mismatch' -Mutation {
    param($document)
    $stress = $document.scenarios | Where-Object name -CEQ 'stress'
    $stress.repeat_observations[0].memory.private_commit.peak_bytes += 1
    $stress.memory.private_commit.peak_bytes += 1
}
Assert-SemanticMutation -ExpectedMessage 'memory gate contract mismatch' -Mutation {
    param($document)
    ($document.scenarios | Where-Object name -CEQ 'stress').repeat_observations[0].memory.gate_applied = $true
}
Assert-SemanticMutation -ExpectedMessage 'budget is not explicitly deferred' -Mutation {
    param($document)
    $document.budget_qualification.target_growth_ratio = 999
}
Assert-SemanticMutation -ExpectedMessage 'global warning contract mismatch' -Mutation {
    param($document)
    $document.warnings = @($document.warnings[0], $document.warnings[0])
}
Assert-SemanticMutation -ExpectedMessage 'stress warning contract mismatch' -Mutation {
    param($document)
    ($document.scenarios | Where-Object name -CEQ 'stress').warnings = @()
}
Assert-SemanticMutation -ExpectedMessage 'Stress allocator samples must be ten exact zeros' -Mutation {
    param($document)
    $allocator = ($document.scenarios | Where-Object name -CEQ 'stress').repeat_observations[0].box3d_allocator
    $allocator.warmup_post_teardown[0] = 1
}
Assert-SemanticMutation -ExpectedMessage 'Stress allocator samples must be ten exact zeros' -Mutation {
    param($document)
    ($document.scenarios | Where-Object name -CEQ 'stress').box3d_allocator.measured_post_teardown[0] = 1
}
Assert-SemanticMutation -ExpectedMessage 'unexpected allocator samples for process' -Mutation {
    param($document)
    $document.process_box3d_allocator.warmup_post_teardown = @(1)
}
Assert-SemanticMutation -ExpectedMessage 'CRT applicability mismatch' -Mutation {
    param($document)
    ($document.scenarios | Where-Object name -CEQ 'stress').repeat_observations[0].crt.applicable = $false
}
Assert-SemanticMutation -Configuration Release -ExpectedMessage 'CRT applicability mismatch' -Mutation {
    param($document)
    ($document.scenarios | Where-Object name -CEQ 'stress').repeat_observations[0].crt.applicable = $true
}
Assert-SemanticMutation -ExpectedMessage 'scenario execution identity mismatch' -Mutation {
    param($document)
    ($document.scenarios | Where-Object name -CEQ 'radial_fall').ticks = 601
}
Assert-SemanticMutation -ExpectedMessage 'scenario execution identity mismatch' -Mutation {
    param($document)
    ($document.scenarios | Where-Object name -CEQ 'radial_fall').seed = 2
}
Assert-SemanticMutation -ExpectedMessage 'step_ms ordering mismatch' -Mutation {
    param($document)
    ($document.scenarios | Where-Object name -CEQ 'radial_fall').step_ms.p50 = 999
}
Assert-SemanticMutation -ExpectedMessage 'invalid step_ms value' -Mutation {
    param($document)
    ($document.scenarios | Where-Object name -CEQ 'radial_fall').step_ms.min = 'NaN'
}
Assert-SemanticMutation -ExpectedMessage 'repeat topology mismatch' -Mutation {
    param($document)
    ($document.scenarios | Where-Object name -CEQ 'radial_fall').repeat_observations[1].peak_contact_count += 1
}
Assert-SemanticMutation -ExpectedMessage 'canonical scenario hash mismatch' -Mutation {
    param($document)
    $scenario = $document.scenarios | Where-Object name -CEQ 'capability_matrix'
    $scenario.final_hash += 1
    $scenario.hashes = @($scenario.final_hash, $scenario.final_hash)
    foreach ($observation in $scenario.repeat_observations) { $observation.hash = $scenario.final_hash }
}
Assert-SemanticMutation -ExpectedMessage 'canonical fixture hashes mismatch' -Mutation {
    param($document)
    foreach ($matrix in @($document.matrix, ($document.scenarios | Where-Object name -CEQ 'capability_matrix').matrix)) {
        ($matrix | Where-Object capability -CEQ 'shape_cast_overlap').fixture_hashes[0] += 1
    }
}
Assert-SemanticMutation -ExpectedMessage 'capability matrix failure' -Mutation {
    param($document)
    foreach ($matrix in @($document.matrix, ($document.scenarios | Where-Object name -CEQ 'capability_matrix').matrix)) {
        $row = $matrix | Where-Object capability -CEQ 'shape_cast_overlap'
        $row.status = 'fallback'
        $row.fallback = 'unexpected'
    }
}

Add-Content -LiteralPath $documents.Debug.Path -Value ' ' -Encoding ascii
Assert-Throws {
    Assert-NinhoFoundationEvidence -Root $gateRoot -ReportPath $reportPath
} 'SHA-256 mismatch'

Assert-NinhoNoReparseAncestors -Path $sandbox -AllowedRoot $artifactRoot | Out-Null
Remove-Item -LiteralPath $sandbox -Recurse -Force
Write-Output 'foundation-evidence-gate-tests: PASS'
