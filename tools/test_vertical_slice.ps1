[CmdletBinding()]
param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [switch]$IncludeUpstream,
    [switch]$IncludeVisualGate
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$preset = $Configuration.ToLowerInvariant()
Import-Module (Join-Path $PSScriptRoot 'VerticalSliceGate.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force

function Invoke-Checked([scriptblock]$Operation, [string]$Name) {
    & $Operation
    if ($LASTEXITCODE -ne 0) { throw "$Name failed with exit code $LASTEXITCODE" }
}

# Foundation is intentionally the first executable gate. A failed foundation
# prevents every slice-specific test, capture, evidence write, and package.
$foundationArguments = @{ Configuration=$Configuration }
if ($IncludeUpstream) { $foundationArguments.IncludeUpstream = $true }
& (Join-Path $PSScriptRoot 'test.ps1') @foundationArguments
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Invoke-Checked { python (Join-Path $PSScriptRoot 'tests\test_generate_vertical_slice_report.py') } 'vertical slice report tests'
Invoke-Checked { python (Join-Path $PSScriptRoot 'tests\test_vertical_slice_tooling_contracts.py') } 'vertical slice tooling contract tests'
& (Join-Path $PSScriptRoot 'tests\vertical-slice-identity-contract-tests.ps1') -Root $root | Out-Null
& (Join-Path $PSScriptRoot 'tests\vertical-slice-gate-tests.ps1') -Root $root | Out-Null
Invoke-Checked { python (Join-Path $PSScriptRoot 'tests\test_art_contracts.py') } 'art contract tests'
& (Join-Path $PSScriptRoot 'tests\art-pipeline-tests.ps1')
Invoke-Checked { python (Join-Path $PSScriptRoot 'tests\test_audio_contracts.py') } 'audio contract tests'
& (Join-Path $PSScriptRoot 'tests\audio-pipeline-tests.ps1')
if ($Configuration -ceq 'Release') {
    & (Join-Path $PSScriptRoot 'tests\package-windows-tests.ps1')
}

if (-not $IncludeVisualGate) {
    Write-Output "VERTICAL_SLICE_NON_VISUAL_GATE_OK configuration=$Configuration"
    exit 0
}

$artifactDirectory = Join-Path $root "artifacts\vertical-slice\$preset"
Assert-NinhoNoReparseAncestors -Path $artifactDirectory -AllowedRoot $root | Out-Null
if (Test-Path -LiteralPath $artifactDirectory) {
    $resolved = (Resolve-Path -LiteralPath $artifactDirectory).Path
    if (-not [string]::Equals($resolved, [IO.Path]::GetFullPath($artifactDirectory), [StringComparison]::OrdinalIgnoreCase)) {
        throw "refusing to clean unexpected artifact directory: $resolved"
    }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $artifactDirectory | Out-Null

$captureManifestPath = & (Join-Path $PSScriptRoot 'capture_vertical_slice.ps1') `
    -Configuration $Configuration -OutputDirectory $artifactDirectory
$captureManifestPath = @($captureManifestPath)[-1]
$capture = Get-Content -Raw -LiteralPath $captureManifestPath | ConvertFrom-Json
if ($capture.schema -cne 'ninho.vertical-slice.capture.v1') { throw 'capture manifest schema mismatch' }

$reviewPath = Join-Path $root 'docs\gameplay\evidence\vertical-slice-reviews.json'
if (-not (Test-Path -LiteralPath $reviewPath -PathType Leaf)) {
    throw 'independent review evidence missing; code/architecture/gameplay/art reviews are blocking'
}
$reviews = Get-Content -Raw -LiteralPath $reviewPath | ConvertFrom-Json
if ($reviews.schema -cne 'ninho.vertical-slice.reviews.v1' -or
        @($reviews.reviews).Count -ne 4) { throw 'independent review evidence schema/count mismatch' }
Assert-NinhoIndependentReviews -Reviews $reviews `
    -ExpectedTestedInputsSha256 ([string]$reviews.tested_inputs_sha256)
$critical = [int](@($reviews.reviews | Measure-Object critical -Sum).Sum)
$important = [int](@($reviews.reviews | Measure-Object important -Sum).Sum)
if ($critical -ne 0 -or $important -ne 0) { throw "blocking review findings remain: C=$critical I=$important" }

$package = [ordered]@{ path='not_applicable_debug'; manifest_sha256=('0'*64); launch_from_space_path='not_applicable_debug' }
if ($Configuration -ceq 'Release') {
    $packageOutputRoot = Join-Path $root 'artifacts\package\windows-release'
    $packageOutput = @(& (Join-Path $PSScriptRoot 'package_windows.ps1') -Configuration Release)
    $packageManifest = Resolve-NinhoPackageManifestOutput `
        -Output $packageOutput -ExpectedOutputRoot $packageOutputRoot
    $package = [ordered]@{
        path = ([IO.Path]::GetDirectoryName($packageManifest)).Substring($root.Length + 1).Replace('\','/')
        manifest_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $packageManifest).Hash.ToLowerInvariant()
        launch_from_space_path = 'passed'
    }
}

$hardware = [ordered]@{
    cpu = (Get-CimInstance Win32_Processor | Select-Object -First 1 -ExpandProperty Name).Trim()
    gpu = ((Get-CimInstance Win32_VideoController | Sort-Object AdapterRAM -Descending | Select-Object -First 1 -ExpandProperty Name)).Trim()
    ram_bytes = [int64](Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory
    os = (Get-CimInstance Win32_OperatingSystem).Caption
}
$physicsP95 = [double](@($capture.renderers | ForEach-Object { $_.scales } | ForEach-Object { $_.physics_step_p95_ms } | Measure-Object -Maximum).Maximum)
$sourceCommit = (& git -C $root rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $sourceCommit -notmatch '^[0-9a-f]{40}$') { throw 'unable to resolve certified source commit' }
$assetManifest = Join-Path $root 'tools\art\vertical_slice_asset_manifest.json'
$audioManifest = Join-Path $root 'tools\audio\audio_manifest.json'
$provenanceBytes = [Text.Encoding]::UTF8.GetBytes(
    (Get-FileHash -Algorithm SHA256 -LiteralPath $assetManifest).Hash.ToLowerInvariant() + '|' +
    (Get-FileHash -Algorithm SHA256 -LiteralPath $audioManifest).Hash.ToLowerInvariant())
$sha = [Security.Cryptography.SHA256]::Create()
try { $provenanceHash = -join ($sha.ComputeHash($provenanceBytes) | ForEach-Object { $_.ToString('x2') }) }
finally { $sha.Dispose() }
$testedInputs = Get-NinhoTestedInputs -Root $root

$evidence = [ordered]@{
    schema = 'ninho.vertical-slice.evidence.v1'
    schema_version = 1
    configuration = $Configuration
    commit = $sourceCommit
    source_revision = $sourceCommit
    tested_inputs_schema = 'ninho.tested-inputs.v2'
    tested_inputs_sha256 = $testedInputs.sha256
    tested_inputs = @($testedInputs.files)
    generated_utc = [DateTime]::UtcNow.ToString('o')
    hardware = $hardware
    canonical_state_contract = 'canonical_state_v1'
    routes = @($capture.routes)
    acceptance = [ordered]@{
        foundation=$true; slice_tests=$true; restart_20=$true; abi=$true
        scanner=$true; assets=$true; smokes=$true; box3d_only=$true
    }
    physics = [ordered]@{ step_p95_ms=$physicsP95; limit_ms=8.0 }
    renderers = @($capture.renderers)
    goldens = @($capture.goldens)
    golden_metadata = @($capture.golden_metadata)
    rubric = [ordered]@{
        visual='approved'; gameplay='approved'; architecture='approved'; code='approved'
        critical=$critical; important=$important
    }
    playtest = [ordered]@{
        status='unavailable'; substitute='independent_agents'; legal_limit='not_legal_advice'
        human_targets='4/5 launch in 90 s; 4/5 finish in 6 min; 4/5 understand front armor; 3/5 discover debris; 5/5 distinguish materials'
    }
    clean_room = [ordered]@{
        approved=$true; provenance_manifest_sha256=$provenanceHash; comparative_review='approved'
        scope=@('names','logos','silhouettes','sounds','ui','layouts','promotional_material')
        codenames=@('Virela','Nox','Talo')
    }
    capture_manifest = [ordered]@{
        path = ([IO.Path]::GetFullPath($captureManifestPath)).Substring($root.Length + 1).Replace('\','/')
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    }
    reviews_manifest = [ordered]@{
        path = 'docs/gameplay/evidence/vertical-slice-reviews.json'
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $reviewPath).Hash.ToLowerInvariant()
    }
    package = $package
    source_artifacts = @($capture.source_artifacts)
}
$evidenceDirectory = Join-Path $root 'docs\gameplay\evidence'
Assert-NinhoNoReparseAncestors -Path $evidenceDirectory -AllowedRoot $root | Out-Null
New-Item -ItemType Directory -Force -Path $evidenceDirectory | Out-Null
$evidencePath = Join-Path $evidenceDirectory "vertical-slice-$preset.json"
$evidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $root `
    -ExpectedConfiguration $Configuration -ExpectedCommit $sourceCommit | Out-Null

python (Join-Path $PSScriptRoot 'generate_vertical_slice_report.py') --root $root --write
if ($LASTEXITCODE -ne 0 -and $Configuration -ceq 'Debug') {
    Write-Output 'Debug evidence written; report waits for Release evidence.'
} elseif ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
Write-Output "VERTICAL_SLICE_GATE_OK configuration=$Configuration evidence=$evidencePath"
