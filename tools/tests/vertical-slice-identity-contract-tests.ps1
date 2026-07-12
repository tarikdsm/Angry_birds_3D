[CmdletBinding()]
param([string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $Root 'tools\VerticalSliceGate.psm1') -Force

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Throws {
    param([scriptblock]$Action, [string]$Pattern)
    try { & $Action }
    catch {
        if ($_.Exception.Message -match $Pattern) { return }
        throw "expected '$Pattern', got: $($_.Exception.Message)"
    }
    throw "expected failure matching '$Pattern'"
}

$normativeDocuments = @(
    'docs/superpowers/specs/2026-07-11-vertical-slice-design.md',
    'docs/superpowers/specs/2026-07-11-vertical-slice-balance-correction-design.md',
    'docs/superpowers/plans/2026-07-11-vertical-slice-implementation.md',
    'docs/superpowers/plans/2026-07-11-vertical-slice-balance-correction.md'
)
$inputs = @(Get-NinhoTestedInputPaths -Root $Root)
foreach ($document in $normativeDocuments) {
    Assert-True ($inputs -ccontains $document) "normative fingerprint omits $document"
}

$sandbox = Join-Path ([IO.Path]::GetTempPath()) ('ninho-identity-' + [Guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Force -Path $sandbox | Out-Null
    & git -C $sandbox init --quiet
    if ($LASTEXITCODE -ne 0) { throw 'failed to initialize identity sandbox' }
    foreach ($document in $normativeDocuments) {
        $path = Join-Path $sandbox $document
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $path) | Out-Null
        [IO.File]::WriteAllText($path, "normative $document`n", [Text.UTF8Encoding]::new($false))
    }
    & git -c core.autocrlf=false -C $sandbox add -- $normativeDocuments
    if ($LASTEXITCODE -ne 0) { throw 'failed to stage identity sandbox' }
    $before = Get-NinhoTestedInputs -Root $sandbox
    [IO.File]::AppendAllText((Join-Path $sandbox $normativeDocuments[0]), "stale mutation`n")
    $after = Get-NinhoTestedInputs -Root $sandbox
    Assert-True ($before.sha256 -cne $after.sha256) `
        'mutation of a normative specification did not stale the tested-input fingerprint'
} finally {
    if (Test-Path -LiteralPath $sandbox) { Remove-Item -LiteralPath $sandbox -Recurse -Force }
}

$reviews = [pscustomobject]@{
    schema = 'ninho.vertical-slice.reviews.v1'
    tested_inputs_sha256 = ('a' * 64)
    reviews = @(
        [pscustomobject]@{ role='code'; reviewer_id='/root/task12_code_review'; verdict='approved'; critical=0; important=0 },
        [pscustomobject]@{ role='architecture'; reviewer_id='/root/task12_architecture_review'; verdict='approved'; critical=0; important=0 },
        [pscustomobject]@{ role='gameplay'; reviewer_id='/root/task12_gameplay_review'; verdict='approved'; critical=0; important=0 },
        [pscustomobject]@{ role='art'; reviewer_id='/root/task12_art_review'; verdict='approved'; critical=0; important=0 }
    )
}
Assert-NinhoIndependentReviews -Reviews $reviews -ExpectedTestedInputsSha256 ('a' * 64)

$reviews.reviews[3].reviewer_id = $reviews.reviews[0].reviewer_id
Assert-Throws {
    Assert-NinhoIndependentReviews -Reviews $reviews -ExpectedTestedInputsSha256 ('a' * 64)
} 'reviewer ID.*unique'

Write-Output 'vertical slice identity contracts: PASS'
