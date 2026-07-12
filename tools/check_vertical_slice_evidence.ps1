[CmdletBinding()]
param([Parameter(Mandatory)][string]$Root)

$ErrorActionPreference = 'Stop'
$Root = [IO.Path]::GetFullPath($Root)
Import-Module (Join-Path $PSScriptRoot 'VerticalSliceGate.psm1') -Force

foreach ($configuration in 'Debug','Release') {
    $preset = $configuration.ToLowerInvariant()
    $evidencePath = Join-Path $Root "docs\gameplay\evidence\vertical-slice-$preset.json"
    if (-not (Test-Path -LiteralPath $evidencePath -PathType Leaf)) {
        throw "vertical slice evidence missing: $evidencePath"
    }
    $document = Get-Content -Raw -LiteralPath $evidencePath | ConvertFrom-Json
    if ([string]$document.commit -notmatch '^[0-9a-f]{40}$') {
        throw "vertical slice evidence commit invalid: $evidencePath"
    }
    Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $Root `
        -ExpectedConfiguration $configuration -ExpectedCommit ([string]$document.commit) | Out-Null
}

Write-Output 'VERTICAL_SLICE_EVIDENCE_CHECK_OK'
