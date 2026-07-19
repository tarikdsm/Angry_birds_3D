$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$gate = [IO.File]::ReadAllText((Join-Path $root 'tools\VerticalSliceGate.psm1'))

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

Assert-True (-not $gate.Contains("artifacts\vertical-slice-golden-verification")) `
    'Evidence verification must not write derived frames under the artifact root'
Assert-True ($gate.Contains('[IO.Path]::GetTempPath()')) `
    'Evidence verification must derive frames under the system temporary root'
Assert-True ($gate.Contains("'ninho-vertical-slice-golden-verification-' + [Guid]::NewGuid().ToString('N')")) `
    'Evidence verification must use a unique directory for concurrent checks'
Assert-True ($gate.Contains('Assert-NinhoNoReparseAncestors -Path $goldenVerificationRoot -AllowedRoot $temporaryRoot')) `
    'Temporary verification paths must remain guarded by the safe-path contract'

foreach ($causalContract in @(
        "Resolve-NinhoPinnedToolchainExecutable -Root `$ArtifactRoot -ToolName 'ffmpeg'",
        "-ExecutableProperty 'ffprobe_exe' -HashProperty 'ffprobe_exe_sha256'",
        "-ExecutableProperty 'exe' -HashProperty 'exe_sha256'",
        'golden cannot be derived from raw source',
        'golden differs from raw source frame'
    )) {
    Assert-True ($gate.Contains($causalContract)) `
        "Evidence verification lost causal media contract: $causalContract"
}
$runner = [IO.File]::ReadAllText((Join-Path $root 'tools\test.ps1'))
Assert-True ($runner.Contains("tests\vertical-slice-verification-contract.ps1")) `
    'Official gate must enforce the verification workspace contract'

Write-Output 'vertical slice verification workspace contract: PASS'
