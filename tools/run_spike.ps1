[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = Join-Path $PSScriptRoot 'build.ps1'
& $build -Configuration $Configuration
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$preset = $Configuration.ToLowerInvariant()
$artifactDirectory = Join-Path $root 'artifacts\physics'
New-Item -ItemType Directory -Force -Path $artifactDirectory | Out-Null
$report = Join-Path $artifactDirectory "box3d-spike-$preset.json"
$executable = Join-Path $root "build\$preset\native\spike\ninho_physics_spike.exe"

& $executable --all --repeat 2 --json $report
$spikeExitCode = $LASTEXITCODE

if (Test-Path -LiteralPath $report) {
    & python -m json.tool $report | Out-Null
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    $document = Get-Content -Raw -LiteralPath $report | ConvertFrom-Json
    if ($spikeExitCode -eq 0) {
        if (@($document.violations).Count -ne 0) {
            [Console]::Error.WriteLine('successful spike report contains normative violations')
            exit 1
        }
        if ($document.recommendation -ne 'prosseguir_com_limites') {
            [Console]::Error.WriteLine(
                "unexpected recommendation: $($document.recommendation)")
            exit 1
        }
    }
}

exit $spikeExitCode
