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
$fixtureSource = Join-Path $gateRoot 'native\kernel\fixture.cpp'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $fixtureSource) | Out-Null
[System.IO.File]::WriteAllText($fixtureSource, "int foundation_fixture = 1;`n")
$requiredFoundationInputs = @(
    'CMakeLists.txt',
    'CMakePresets.json',
    'tools/bootstrap.ps1',
    'tools/box3d-v0.1.0-compile-sources.txt',
    'tools/build.ps1',
    'tools/FoundationEvidenceGate.psm1',
    'tools/generate_foundation_report.py',
    'tools/GodotSmokeRegistry.psm1',
    'tools/GodotSpikeGate.psm1',
    'tools/Invoke-Native.ps1',
    'tools/run_spike.ps1',
    'tools/SafePath.psm1',
    'tools/SpikeEvidenceValidation.psm1',
    'tools/SpikeReportGate.psm1',
    'tools/test.ps1',
    'tools/TestedInputIdentity.psm1',
    'tools/ToolchainIntegrity.psm1',
    'tools/toolchain.lock.json',
    'tools/UpstreamBox3DGate.psm1',
    'tools/VerticalSliceGate.psm1'
)
$cmakeInputs = @(
    'cmake/Dependencies.cmake',
    'cmake/RequireFoundationFlags.cmake'
)
$fixtureContents = @{
    'CMakeLists.txt' = "cmake_minimum_required(VERSION 3.22)`n"
    'CMakePresets.json' = "{}`n"
    'cmake/Dependencies.cmake' = @"
FetchContent_Declare(box3d
  GIT_TAG 8441b4a06d6d09dcfb0b0f704df4d847d1437b92)
target_compile_options(box3d PRIVATE /fp:precise)
"@
    'cmake/RequireFoundationFlags.cmake' = "set(NINHO_FOUNDATION_FLAGS precise)`n"
    'tools/toolchain.lock.json' = "{`"cmake`":{`"version`":`"4.3.3`"}}`n"
}
foreach ($relative in @($requiredFoundationInputs + $cmakeInputs)) {
    $path = Join-Path $gateRoot $relative
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $path) | Out-Null
    $content = if ($fixtureContents.ContainsKey($relative)) {
        $fixtureContents[$relative]
    } else {
        "foundation fixture: $relative`n"
    }
    [System.IO.File]::WriteAllText($path, $content)
}
& git -c "safe.directory=$gateRoot" -C $gateRoot config user.email fixture@example.invalid
& git -c "safe.directory=$gateRoot" -C $gateRoot config user.name 'Foundation Fixture'
$trackedFixtureInputs = @(
    'native/kernel/fixture.cpp',
    'docs/physics/evidence/foundation-report-debug.json',
    'docs/physics/evidence/foundation-report-release.json'
) + $requiredFoundationInputs + $cmakeInputs
& git -c "safe.directory=$gateRoot" -C $gateRoot add -- $trackedFixtureInputs
if ($LASTEXITCODE -ne 0) { throw 'failed to create tracked evidence fixture' }
& git -c "safe.directory=$gateRoot" -C $gateRoot commit --quiet -m fixture
if ($LASTEXITCODE -ne 0) { throw 'failed to commit foundation source fixture' }
$sourceRevision = (& git -c "safe.directory=$gateRoot" -C $gateRoot rev-parse HEAD).Trim()

$expectedFoundationInputs = [string[]]@(
    $requiredFoundationInputs + $cmakeInputs + 'native/kernel/fixture.cpp'
)
[Array]::Sort($expectedFoundationInputs, [StringComparer]::Ordinal)
$actualFoundationInputs = [string[]]@(
    Get-NinhoFoundationTestedInputPaths -Root $gateRoot
)
if ($actualFoundationInputs.Count -ne $expectedFoundationInputs.Count) {
    throw "Foundation tested input count mismatch: expected $($expectedFoundationInputs.Count), got $($actualFoundationInputs.Count)"
}
for ($index = 0; $index -lt $expectedFoundationInputs.Count; ++$index) {
    if ($actualFoundationInputs[$index] -cne $expectedFoundationInputs[$index]) {
        throw "Foundation tested input mismatch at ${index}: expected $($expectedFoundationInputs[$index]), got $($actualFoundationInputs[$index])"
    }
}

$requiredBuildScript = Join-Path $gateRoot 'tools\build.ps1'
$requiredBuildScriptText = [System.IO.File]::ReadAllText($requiredBuildScript)
try {
    Remove-Item -LiteralPath $requiredBuildScript -Force
    Assert-Throws {
        Get-NinhoFoundationTestedInputPaths -Root $gateRoot | Out-Null
    } 'required foundation tested input is missing: tools/build.ps1'
} finally {
    [System.IO.File]::WriteAllText($requiredBuildScript, $requiredBuildScriptText)
}

function Set-EvidenceIdentity {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)]$Identity,
        [string]$Revision = $sourceRevision
    )
    $document = [System.IO.File]::ReadAllText($Path) | ConvertFrom-Json
    $document | Add-Member -NotePropertyName source_revision -NotePropertyValue $Revision -Force
    $document | Add-Member -NotePropertyName tested_inputs_schema `
        -NotePropertyValue 'ninho.tested-inputs.v2' -Force
    $document | Add-Member -NotePropertyName tested_inputs_sha256 `
        -NotePropertyValue $Identity.sha256 -Force
    $document | Add-Member -NotePropertyName tested_inputs `
        -NotePropertyValue @($Identity.files) -Force
    [System.IO.File]::WriteAllText(
        $Path,
        ($document | ConvertTo-Json -Depth 100),
        [System.Text.UTF8Encoding]::new($false))
}

function Reset-EvidenceDocuments {
    $identity = Get-NinhoFoundationTestedInputs -Root $gateRoot
    foreach ($name in 'Debug', 'Release') {
        Copy-Item -LiteralPath (Join-Path $repositoryRoot "docs\physics\evidence\foundation-report-$($name.ToLowerInvariant()).json") `
            -Destination $documents[$name].Path -Force
        Set-EvidenceIdentity -Path $documents[$name].Path -Identity $identity
        $documents[$name].Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $documents[$name].Path).Hash
    }
    return $identity
}

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
    Reset-EvidenceDocuments | Out-Null
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

function Assert-TestedInputMutationInvalidatesEvidence {
    param(
        [Parameter(Mandatory)][string]$RelativePath,
        [Parameter(Mandatory)][string]$From,
        [Parameter(Mandatory)][string]$To
    )
    Reset-EvidenceDocuments | Out-Null
    Write-EvidenceReport
    $path = Join-Path $gateRoot $RelativePath
    $original = [System.IO.File]::ReadAllText($path)
    $mutated = $original.Replace($From, $To)
    if ($mutated -ceq $original) {
        throw "Foundation mutation fixture token is missing: $RelativePath / $From"
    }
    try {
        [System.IO.File]::WriteAllText($path, $mutated)
        Assert-Throws {
            Assert-NinhoFoundationEvidence -Root $gateRoot -ReportPath $reportPath
        } 'tested inputs aggregate SHA-256 mismatch'
    } finally {
        [System.IO.File]::WriteAllText($path, $original)
    }
}

$validIdentity = Reset-EvidenceDocuments
Write-EvidenceReport
Assert-NinhoFoundationEvidence -Root $gateRoot -ReportPath $reportPath

$fixtureSourceText = [System.IO.File]::ReadAllText($fixtureSource)
[System.IO.File]::AppendAllText($fixtureSource, "int stale_mutation = 2;`n")
Assert-Throws {
    Assert-NinhoFoundationEvidence -Root $gateRoot -ReportPath $reportPath
} 'tested inputs aggregate SHA-256 mismatch'
[System.IO.File]::WriteAllText($fixtureSource, $fixtureSourceText)

Set-EvidenceIdentity -Path $documents.Debug.Path -Identity $validIdentity `
    -Revision ('f' * 40)
Write-EvidenceReport
Assert-Throws {
    Assert-NinhoFoundationEvidence -Root $gateRoot -ReportPath $reportPath
} 'source revision is not an ancestor of the current HEAD'
$validIdentity = Reset-EvidenceDocuments

[System.IO.File]::AppendAllText($fixtureSource, "int local_artifact = 3;`n")
$localIdentity = Get-NinhoFoundationTestedInputs -Root $gateRoot
foreach ($name in 'Debug', 'Release') {
    Set-EvidenceIdentity -Path $documents[$name].Path -Identity $localIdentity
}
Write-EvidenceReport
Assert-NinhoFoundationEvidence -Root $gateRoot -ReportPath $reportPath
$publicationArtifacts = Join-Path $gateRoot 'artifacts\physics'
New-Item -ItemType Directory -Force -Path $publicationArtifacts | Out-Null
foreach ($name in 'Debug', 'Release') {
    Copy-Item -LiteralPath $documents[$name].Path -Destination (Join-Path `
        $publicationArtifacts "box3d-spike-$($name.ToLowerInvariant()).json") -Force
}
[System.IO.File]::WriteAllText($fixtureSource, $fixtureSourceText)

Assert-TestedInputMutationInvalidatesEvidence `
    -RelativePath 'cmake/Dependencies.cmake' `
    -From '8441b4a06d6d09dcfb0b0f704df4d847d1437b92' `
    -To '9441b4a06d6d09dcfb0b0f704df4d847d1437b92'
Assert-TestedInputMutationInvalidatesEvidence `
    -RelativePath 'cmake/Dependencies.cmake' `
    -From '/fp:precise' `
    -To '/fp:fast'
Assert-TestedInputMutationInvalidatesEvidence `
    -RelativePath 'tools/toolchain.lock.json' `
    -From '4.3.3' `
    -To '4.3.4'
Assert-TestedInputMutationInvalidatesEvidence `
    -RelativePath 'tools/build.ps1' `
    -From 'tools/build.ps1' `
    -To 'tools/build-v2.ps1'
Assert-TestedInputMutationInvalidatesEvidence `
    -RelativePath 'tools/run_spike.ps1' `
    -From 'tools/run_spike.ps1' `
    -To 'tools/run-spike-v2.ps1'
Assert-TestedInputMutationInvalidatesEvidence `
    -RelativePath 'tools/generate_foundation_report.py' `
    -From 'tools/generate_foundation_report.py' `
    -To 'tools/generate-foundation-report-v2.py'
Assert-TestedInputMutationInvalidatesEvidence `
    -RelativePath 'tools/SpikeEvidenceValidation.psm1' `
    -From 'tools/SpikeEvidenceValidation.psm1' `
    -To 'tools/SpikeEvidenceValidationV2.psm1'
Assert-TestedInputMutationInvalidatesEvidence `
    -RelativePath 'tools/GodotSpikeGate.psm1' `
    -From 'tools/GodotSpikeGate.psm1' `
    -To 'tools/GodotSpikeGateV2.psm1'
Assert-TestedInputMutationInvalidatesEvidence `
    -RelativePath 'tools/VerticalSliceGate.psm1' `
    -From 'tools/VerticalSliceGate.psm1' `
    -To 'tools/VerticalSliceGateV2.psm1'
Assert-TestedInputMutationInvalidatesEvidence `
    -RelativePath 'tools/GodotSmokeRegistry.psm1' `
    -From 'tools/GodotSmokeRegistry.psm1' `
    -To 'tools/GodotSmokeRegistryV2.psm1'
Assert-TestedInputMutationInvalidatesEvidence `
    -RelativePath 'tools/ToolchainIntegrity.psm1' `
    -From 'tools/ToolchainIntegrity.psm1' `
    -To 'tools/ToolchainIntegrityV2.psm1'
$validIdentity = Reset-EvidenceDocuments
Write-EvidenceReport
Assert-Throws {
    Publish-NinhoFoundationEvidence -Root $gateRoot `
        -ArtifactDirectory $publicationArtifacts
} 'tested inputs aggregate SHA-256 mismatch'
foreach ($name in 'Debug', 'Release') {
    Copy-Item -LiteralPath $documents[$name].Path -Destination (Join-Path `
        $publicationArtifacts "box3d-spike-$($name.ToLowerInvariant()).json") -Force
}
Publish-NinhoFoundationEvidence -Root $gateRoot `
    -ArtifactDirectory $publicationArtifacts
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
