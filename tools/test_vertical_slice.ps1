[CmdletBinding(DefaultParameterSetName = 'Legacy')]
param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [switch]$IncludeUpstream,
    [switch]$IncludeVisualGate,
    [Parameter(ParameterSetName = 'CaptureOnly')]
    [switch]$CaptureOnly,
    [Parameter(ParameterSetName = 'UseExistingCapture')]
    [switch]$UseExistingCapture
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$preset = $Configuration.ToLowerInvariant()
Import-Module (Join-Path $PSScriptRoot 'VerticalSliceGate.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'ToolchainIntegrity.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'VerticalSliceCaptureWorkflow.psm1') -Force
$ffmpeg = Resolve-NinhoPinnedToolchainExecutable `
    -Root $root -ToolName 'ffmpeg' `
    -ExecutableProperty 'exe' -HashProperty 'exe_sha256' `
    -Name 'FFmpeg'
Resolve-NinhoPinnedToolchainExecutable `
    -Root $root -ToolName 'ffmpeg' `
    -ExecutableProperty 'ffprobe_exe' -HashProperty 'ffprobe_exe_sha256' `
    -Name 'FFprobe' | Out-Null
$env:PATH = "$(Split-Path -Parent $ffmpeg);$env:PATH"

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
& (Join-Path $PSScriptRoot 'tests\vertical-slice-capture-workflow-tests.ps1') -Root $root | Out-Null
Invoke-Checked { python (Join-Path $PSScriptRoot 'tests\test_art_contracts.py') } 'art contract tests'
& (Join-Path $PSScriptRoot 'tests\art-pipeline-tests.ps1')
Invoke-Checked { python (Join-Path $PSScriptRoot 'tests\test_audio_contracts.py') } 'audio contract tests'
& (Join-Path $PSScriptRoot 'tests\audio-pipeline-tests.ps1')
if ($Configuration -ceq 'Release') {
    & (Join-Path $PSScriptRoot 'tests\package-windows-tests.ps1')
}

& (Join-Path $PSScriptRoot 'tests\vertical-slice-finding64-smoke.ps1') -Root $root
& (Join-Path $PSScriptRoot 'tests\vertical-slice-input-feedback-smoke.ps1') -Root $root

$visualGateRequested = $IncludeVisualGate -or $CaptureOnly -or $UseExistingCapture
if (-not $visualGateRequested) {
    Write-Output "VERTICAL_SLICE_NON_VISUAL_GATE_OK configuration=$Configuration"
    exit 0
}

$artifactDirectory = Join-Path $root "artifacts\vertical-slice\$preset"
$captureMode = switch ($PSCmdlet.ParameterSetName) {
    'CaptureOnly' { 'CaptureOnly' }
    'UseExistingCapture' { 'UseExistingCapture' }
    default { 'Legacy' }
}
$captureScript = Join-Path $PSScriptRoot 'capture_vertical_slice.ps1'
$captureOperation = {
    param([string]$OutputDirectory)
    & $captureScript `
        -Configuration $Configuration -OutputDirectory $OutputDirectory
}.GetNewClosure()
$captureParameters = @{
    Root = $root
    Configuration = $Configuration
    ArtifactDirectory = $artifactDirectory
    Mode = $captureMode
}
if ($captureMode -cne 'UseExistingCapture') {
    $captureParameters.CaptureOperation = $captureOperation
}
$captureResult = Invoke-NinhoVerticalSliceCaptureWorkflow @captureParameters
$captureManifestPath = $captureResult.ManifestPath
$capture = $captureResult.Manifest
$captureManifestSha256 = $captureResult.ManifestSha256
# The capture script imports shared modules with -Force from inside the capture
# callback. Restore them at the parent use site and qualify every post-capture
# call so a transient child scope cannot decide which implementation is used.
Import-Module (Join-Path $PSScriptRoot 'VerticalSliceGate.psm1') -Force -Scope Local
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force -Scope Local
$testedInputs = VerticalSliceGate\Get-NinhoTestedInputs -Root $root
if ($CaptureOnly) {
    Write-Output "VERTICAL_SLICE_CAPTURE_READY configuration=$Configuration manifest=$captureManifestPath manifest_sha256=$captureManifestSha256 tested_inputs_sha256=$($testedInputs.sha256)"
    exit 0
}

$reviewPath = Join-Path $root 'docs\gameplay\evidence\vertical-slice-reviews.json'
if (-not (Test-Path -LiteralPath $reviewPath -PathType Leaf)) {
    throw 'agent review evidence missing; code/architecture/gameplay/art reviews are blocking technical controls'
}
$reviews = Get-Content -Raw -LiteralPath $reviewPath | ConvertFrom-Json
if ($reviews.schema -cne 'ninho.vertical-slice.reviews.v2' -or
        $reviews.schema_version -ne 2 -or
        @($reviews.reviews).Count -ne 4) { throw 'agent review evidence schema/count mismatch' }
VerticalSliceGate\Assert-NinhoAgentReviews -Reviews $reviews `
    -ExpectedTestedInputsSha256 ([string]$testedInputs.sha256) `
    -ExpectedCaptureManifestSha256 $captureManifestSha256 `
    -ExpectedCaptureConfiguration $Configuration `
    -ExpectedGoldenMetadata @($capture.golden_metadata)
$critical = [int](@($reviews.reviews | Measure-Object critical -Sum).Sum)
$important = [int](@($reviews.reviews | Measure-Object important -Sum).Sum)
if ($critical -ne 0 -or $important -ne 0) { throw "blocking review findings remain: C=$critical I=$important" }

$cleanRoomReviewRelativePath = 'docs/gameplay/evidence/vertical-slice-clean-room-review.json'
$cleanRoomReviewPath = Join-Path $root $cleanRoomReviewRelativePath
if (-not (Test-Path -LiteralPath $cleanRoomReviewPath -PathType Leaf)) {
    throw 'separate clean-room review evidence is missing or pending recertification'
}
$assetProvenancePath = VerticalSliceGate\Assert-NinhoRelativeArtifactPath `
    'tools/art/vertical_slice_asset_manifest.json' $root
$audioProvenancePath = VerticalSliceGate\Assert-NinhoRelativeArtifactPath `
    'tools/audio/audio_manifest.json' $root
foreach ($provenancePath in $assetProvenancePath,$audioProvenancePath) {
    if (-not (Test-Path -LiteralPath $provenancePath -PathType Leaf)) {
        throw "clean-room provenance manifest missing: $provenancePath"
    }
}
$cleanRoomReview = Get-Content -Raw -LiteralPath $cleanRoomReviewPath | ConvertFrom-Json
VerticalSliceGate\Assert-NinhoCleanRoomReview -Review $cleanRoomReview `
    -ExpectedAssetManifestSha256 (
        (Get-FileHash -Algorithm SHA256 -LiteralPath $assetProvenancePath).Hash.ToLowerInvariant()) `
    -ExpectedAudioManifestSha256 (
        (Get-FileHash -Algorithm SHA256 -LiteralPath $audioProvenancePath).Hash.ToLowerInvariant())

$package = [ordered]@{ path='not_applicable_debug'; manifest_sha256=('0'*64); launch_from_space_path='not_applicable_debug' }
if ($Configuration -ceq 'Release') {
    $packageOutputRoot = Join-Path $root 'artifacts\package\windows-release'
    $packageOutput = @(& (Join-Path $PSScriptRoot 'package_windows.ps1') -Configuration Release)
    $packageManifest = VerticalSliceGate\Resolve-NinhoPackageManifestOutput `
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
    canonical_state_contract = 'canonical_state_v2'
    routes = @($capture.routes)
    physics = [ordered]@{ step_p95_ms=$physicsP95; limit_ms=8.0 }
    renderers = @($capture.renderers)
    goldens = @($capture.goldens)
    golden_metadata = @($capture.golden_metadata)
    rubric = [ordered]@{
        visual='approved'; gameplay='approved'; architecture='approved'; code='approved'
        critical=$critical; important=$important
    }
    playtest = [ordered]@{
        status='not_performed'; participants=0; substitute='none'; gate_status='pending'
        required_before='product_release'; legal_limit='not_legal_advice'
        human_targets='4/5 launch in 90 s; 4/5 finish in 6 min; 4/5 understand front armor; 3/5 discover debris; 5/5 distinguish materials'
    }
    clean_room_review = [ordered]@{
        path = $cleanRoomReviewRelativePath
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $cleanRoomReviewPath).Hash.ToLowerInvariant()
    }
    capture_manifest = [ordered]@{
        path = ([IO.Path]::GetFullPath($captureManifestPath)).Substring($root.Length + 1).Replace('\','/')
        sha256 = $captureManifestSha256
    }
    reviews_manifest = [ordered]@{
        path = 'docs/gameplay/evidence/vertical-slice-reviews.json'
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $reviewPath).Hash.ToLowerInvariant()
    }
    package = $package
    source_artifacts = @($capture.source_artifacts)
}
$evidenceDirectory = Join-Path $root 'docs\gameplay\evidence'
SafePath\Assert-NinhoNoReparseAncestors -Path $evidenceDirectory -AllowedRoot $root | Out-Null
New-Item -ItemType Directory -Force -Path $evidenceDirectory | Out-Null
$evidencePath = Join-Path $evidenceDirectory "vertical-slice-$preset.json"
VerticalSliceGate\Publish-NinhoVerticalSliceEvidence -Document $evidence `
    -EvidencePath $evidencePath -ArtifactRoot $root `
    -ExpectedConfiguration $Configuration | Out-Null

python (Join-Path $PSScriptRoot 'generate_vertical_slice_report.py') --root $root --write
if ($LASTEXITCODE -ne 0 -and $Configuration -ceq 'Debug') {
    Write-Output 'Debug evidence written; report waits for Release evidence.'
} elseif ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
Write-Output "VERTICAL_SLICE_GATE_OK configuration=$Configuration evidence=$evidencePath"
