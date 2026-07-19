[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$PublishCanonicalEvidence
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = Join-Path $PSScriptRoot 'build.ps1'
Import-Module (Join-Path $PSScriptRoot 'SpikeReportGate.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'FoundationEvidenceGate.psm1') -Force
& $build -Configuration $Configuration
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$preset = $Configuration.ToLowerInvariant()
$artifactDirectory = Join-Path $root 'artifacts\physics'
Assert-NinhoNoReparseAncestors -Path $artifactDirectory -AllowedRoot $root | Out-Null
New-Item -ItemType Directory -Force -Path $artifactDirectory | Out-Null
$report = Join-Path $artifactDirectory "box3d-spike-$preset.json"
$executable = Join-Path $root "build\$preset\native\spike\ninho_physics_spike.exe"
$startedUtc = Start-NinhoSpikeReportCapture `
    -Path $report `
    -AllowedRoot $artifactDirectory
[Console]::Out.WriteLine(
    "Spike report execution started UTC: $($startedUtc.ToString('O'))")

& $executable --all --repeat 2 --json $report
$spikeExitCode = $LASTEXITCODE
if ($spikeExitCode -ne 0) {
    exit $spikeExitCode
}

$document = $null
try {
    $document = Read-NinhoFreshSpikeReport `
        -Path $report `
        -StartedUtc $startedUtc `
        -AllowedRoot $artifactDirectory `
        -ExpectedBuildType $Configuration
} catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}
if (@($document.violations).Count -ne 0) {
    [Console]::Error.WriteLine('successful spike report contains normative violations')
    exit 1
}
if ($document.recommendation -ne 'prosseguir_com_limites') {
    [Console]::Error.WriteLine(
        "unexpected recommendation: $($document.recommendation)")
    exit 1
}
if ($document.budget_qualification.status -ne 'deferred' -or
    $document.budget_qualification.warning -ne
        'private_commit_budget_unqualified') {
    [Console]::Error.WriteLine('private commit budget must remain deferred')
    exit 1
}
if (-not (@($document.warnings).code -contains
        'private_commit_budget_unqualified')) {
    [Console]::Error.WriteLine('missing permanent private budget warning')
    exit 1
}

$document = Add-NinhoFoundationEvidenceIdentity -Root $root -Document $document
[System.IO.File]::WriteAllText(
    $report,
    ($document | ConvertTo-Json -Depth 100),
    [System.Text.UTF8Encoding]::new($false))
if ($PublishCanonicalEvidence) {
    Publish-NinhoFoundationEvidence -Root $root `
        -ArtifactDirectory $artifactDirectory
}

exit 0
