[CmdletBinding()]
param([string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $Root 'tools\VerticalSliceGate.psm1') -Force
Import-Module (Join-Path $Root 'tools\TestedInputIdentity.psm1') -Force

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
	'docs/gameplay/vertical-slice-content-schema.md',
	'docs/superpowers/specs/2026-07-11-vertical-slice-design.md',
    'docs/superpowers/specs/2026-07-11-vertical-slice-balance-correction-design.md',
    'docs/superpowers/plans/2026-07-11-vertical-slice-implementation.md',
    'docs/superpowers/plans/2026-07-11-vertical-slice-balance-correction.md'
)
$inputs = @(Get-NinhoTestedInputPaths -Root $Root)
$sliceIdentity = Get-NinhoTestedInputs -Root $Root
$sharedIdentity = Get-NinhoTestedInputIdentity -Root $Root -RelativePaths $inputs
Assert-True ($sliceIdentity.sha256 -ceq $sharedIdentity.sha256) `
    'vertical slice wrapper diverges from the shared tested-input identity'
foreach ($document in $normativeDocuments) {
    Assert-True ($inputs -ccontains $document) "normative fingerprint omits $document"
}

$canonicalizationSandbox = Join-Path ([IO.Path]::GetTempPath()) `
    ('ninho-canonical-inputs-' + [Guid]::NewGuid().ToString('N'))
$lineEndingPath = Join-Path $canonicalizationSandbox 'line-ending-contract.txt'
$binaryPath = Join-Path $canonicalizationSandbox 'binary-contract.bin'
try {
    New-Item -ItemType Directory -Force -Path $canonicalizationSandbox | Out-Null
    [IO.File]::WriteAllBytes($lineEndingPath, [Text.UTF8Encoding]::new($false).GetBytes("alpha`nbeta`n"))
    $lf = Get-NinhoCanonicalTestedInputContent -Path $lineEndingPath -RelativePath 'tools/tests/.line-ending-contract.tmp'
    [IO.File]::WriteAllBytes($lineEndingPath, [Text.UTF8Encoding]::new($false).GetBytes("alpha`r`nbeta`r`n"))
    $crlf = Get-NinhoCanonicalTestedInputContent -Path $lineEndingPath -RelativePath 'tools/tests/.line-ending-contract.tmp'
    Assert-True ($lf.mode -ceq 'text_utf8_lf' -and $crlf.mode -ceq 'text_utf8_lf') `
        'UTF-8 text must use canonical LF mode'
    Assert-True ([Convert]::ToBase64String($lf.bytes) -ceq [Convert]::ToBase64String($crlf.bytes)) `
        'LF and CRLF text must have identical canonical bytes'
    [IO.File]::WriteAllBytes($binaryPath, [byte[]](0,13,10,255))
    $binaryA = Get-NinhoCanonicalTestedInputContent -Path $binaryPath -RelativePath 'art/.binary-contract.tmp'
    [IO.File]::WriteAllBytes($binaryPath, [byte[]](0,10,255))
    $binaryB = Get-NinhoCanonicalTestedInputContent -Path $binaryPath -RelativePath 'art/.binary-contract.tmp'
    Assert-True ($binaryA.mode -ceq 'binary' -and $binaryB.mode -ceq 'binary') `
        'binary inputs must remain byte-preserving'
    Assert-True ([Convert]::ToBase64String($binaryA.bytes) -cne [Convert]::ToBase64String($binaryB.bytes)) `
        'binary inputs must remain sensitive to CRLF byte changes'
} finally {
    if (Test-Path -LiteralPath $canonicalizationSandbox) {
        Remove-Item -LiteralPath $canonicalizationSandbox -Recurse -Force
    }
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

$reviewGoldenMetadata = @(
    [pscustomobject]@{name='overview';sha256=('1' * 64)},
    [pscustomobject]@{name='aim';sha256=('2' * 64)},
    [pscustomobject]@{name='virela';sha256=('3' * 64)},
    [pscustomobject]@{name='vulnerable_impact';sha256=('4' * 64)},
    [pscustomobject]@{name='result';sha256=('5' * 64)}
)
$reviews = [pscustomobject]@{
    schema = 'ninho.vertical-slice.reviews.v2'
    schema_version = 2
    tested_inputs_schema = 'ninho.tested-inputs.v2'
    tested_inputs_sha256 = ('a' * 64)
    reviews = @(
        [pscustomobject]@{ role='code'; reviewer_id='/root/task12_code_review'; verdict='approved'; critical=0; important=0 },
        [pscustomobject]@{ role='architecture'; reviewer_id='/root/task12_architecture_review'; verdict='approved'; critical=0; important=0 },
        [pscustomobject]@{ role='gameplay'; reviewer_id='/root/task12_gameplay_review'; verdict='approved'; critical=0; important=0 },
        [pscustomobject]@{
            role='art'; reviewer_id='/root/task12_art_review'; verdict='approved'; critical=0; important=0
            artifact_bindings=@([pscustomobject]@{
                    schema='ninho.vertical-slice.art-review-binding.v1'
                    capture_manifest_sha256=('b' * 64);capture_configuration='Debug'
                    goldens=$reviewGoldenMetadata
                })
        }
    )
}
$reviewValidationParameters = @{
    Reviews=$reviews
    ExpectedTestedInputsSha256=('a' * 64)
    ExpectedCaptureManifestSha256=('b' * 64)
    ExpectedCaptureConfiguration='Debug'
    ExpectedGoldenMetadata=$reviewGoldenMetadata
}
Assert-NinhoAgentReviews @reviewValidationParameters

$reviews.reviews[3].reviewer_id = $reviews.reviews[0].reviewer_id
Assert-NinhoAgentReviews @reviewValidationParameters
$reviews.reviews[3].reviewer_id = ''
Assert-Throws {
    Assert-NinhoAgentReviews @reviewValidationParameters
} 'agent review attribution label is missing'

$pendingPlaytest = [pscustomobject]@{
    status='not_performed'; participants=0; substitute='none'; gate_status='pending'
    required_before='product_release'; legal_limit='not_legal_advice'
}
$pendingResult = Assert-NinhoHumanPlaytestPending -Playtest $pendingPlaytest
Assert-True ($pendingResult.status -ceq 'pending' -and
    $pendingResult.satisfies_human_playtest -ceq $false -and
    $pendingResult.legacy_record -ceq $false) `
    'current pending playtest was treated as satisfied'
$legacyResult = Assert-NinhoHumanPlaytestPending -Playtest ([pscustomobject]@{
    status='unavailable'; substitute='independent_agents'; legal_limit='not_legal_advice'
})
Assert-True ($legacyResult.status -ceq 'pending' -and
    $legacyResult.satisfies_human_playtest -ceq $false -and
    $legacyResult.legacy_record -ceq $true) `
    'legacy agent substitute was treated as a satisfied human playtest'

Write-Output 'vertical slice identity contracts: PASS'
