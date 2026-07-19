$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

$relativePaths = @(
    'tools\VerticalSliceGate.psm1',
    'tools\check_vertical_slice_evidence.ps1',
    'tools\test_vertical_slice.ps1',
    'tools\tests\vertical-slice-gate-tests.ps1'
)
foreach ($relativePath in $relativePaths) {
    $source = [IO.File]::ReadAllText((Join-Path $root $relativePath))
    Assert-True ($source -notmatch 'ExpectedCommit|evidence is stale') `
        "Vertical-slice evidence API retains tautological commit checking: $relativePath"
}

$gate = [IO.File]::ReadAllText((Join-Path $root 'tools\VerticalSliceGate.psm1'))
$identity = [IO.File]::ReadAllText((Join-Path $root 'tools\TestedInputIdentity.psm1'))
Assert-True ($gate.Contains('-ExpectedSourceRevision ([string]$doc.commit)')) `
    'Evidence gate must preserve commit/source_revision coherence'
Assert-True ($identity.Contains('merge-base --is-ancestor $sourceRevision HEAD')) `
    'Evidence identity must preserve Git ancestry validation'
Assert-True ($identity.Contains('tested inputs aggregate SHA-256 mismatch')) `
    'Evidence identity must preserve content fingerprint validation'

Write-Output 'vertical slice evidence API contract: PASS'
