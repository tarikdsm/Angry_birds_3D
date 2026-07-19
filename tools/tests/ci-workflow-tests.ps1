$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$workflowPath = Join-Path $root '.github\workflows\official-gate.yml'

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

Assert-True (Test-Path -LiteralPath $workflowPath -PathType Leaf) `
    'Official GitHub Actions workflow is missing'
$workflow = [IO.File]::ReadAllText($workflowPath) -replace "`r`n", "`n"

Assert-True ($workflow -match '(?m)^  push:\n    branches:\n      - main$') `
    'Workflow must run on pushes to main'
Assert-True ($workflow -match '(?m)^  pull_request:$') `
    'Workflow must run on pull requests'
Assert-True ($workflow -notmatch '(?m)^\s*pull_request_target:') `
    'Workflow must not run untrusted code through pull_request_target'
Assert-True ($workflow -match '(?m)^permissions:\n  contents: read$') `
    'Workflow permissions must be read-only'
Assert-True ($workflow -notmatch '(?m)^\s*[\w-]+: write\s*$') `
    'Workflow must not grant write permissions'
Assert-True ($workflow -notmatch '\$\{\{\s*secrets\.') `
    'Workflow must not consume repository secrets'

Assert-True ($workflow -match '(?m)^    runs-on: windows-2025-vs2026$') `
    'Workflow must use the locked Visual Studio 2026 runner image'
Assert-True ($workflow -match '(?m)^    timeout-minutes: 60$') `
    'Workflow must bound gate execution time'
Assert-True ($workflow -match '(?m)^  cancel-in-progress: true$') `
    'Workflow must cancel superseded runs'
$checkoutSha = 'de0fac2e4500dabe0009e67214ff5f5447ce83dd'
$cacheSha = '27d5ce7f107fe9357f9df03efb73ab90386fccae'
$externalActionUses = [regex]::Matches($workflow, '(?m)^\s*uses:\s+(?<reference>\S+)')
Assert-True ($externalActionUses.Count -gt 0) `
    'Workflow must declare at least one external action'
foreach ($actionUse in $externalActionUses) {
    $reference = $actionUse.Groups['reference'].Value
    if ($reference -match '^[^./][^@\s]*/[^@\s]+@') {
        Assert-True ($reference -match '^[^@\s]+@[0-9a-f]{40}$') `
            "External action must be pinned to a full commit SHA, found: $reference"
    }
}
Assert-True ($workflow -match "(?m)^\s+uses: actions/checkout@${checkoutSha} # v6\.0\.2$") `
    'Workflow must pin checkout v6.0.2 to its audited commit SHA'
Assert-True ($workflow -match "(?s)uses: actions/checkout@${checkoutSha} # v6\.0\.2.*?fetch-depth: 0.*?persist-credentials: false") `
    'Checkout must preserve history without retaining credentials'
Assert-True ($workflow -match "(?m)^\s+uses: actions/cache@${cacheSha} # v5\.0\.5$") `
    'Workflow must pin cache v5.0.5 to its audited commit SHA'
Assert-True ($workflow -match "(?s)uses: actions/cache@${cacheSha} # v5\.0\.5.*?path: \|\n\s+\.tools\n\s+\.fetchcontent-cache") `
    'Workflow cache must cover portable tools and FetchContent sources'
Assert-True ($workflow -match "hashFiles\('tools/toolchain.lock.json', 'cmake/Dependencies.cmake'\)") `
    'Workflow cache key must track both dependency locks'

$bootstrapIndex = $workflow.IndexOf('.\tools\bootstrap.ps1 -InstallPortable', [StringComparison]::Ordinal)
$gateIndex = $workflow.IndexOf('.\tools\test.ps1 -Configuration Debug', [StringComparison]::Ordinal)
Assert-True ($bootstrapIndex -ge 0) 'Workflow must install the pinned portable toolchain'
Assert-True ($gateIndex -gt $bootstrapIndex) 'Workflow must execute the official gate after bootstrap'
Assert-True ($workflow -notmatch '\s-IncludeUpstream(?:\s|$)') `
    'Default CI must not add the expensive optional upstream gate'
$officialGate = [IO.File]::ReadAllText((Join-Path $root 'tools\test.ps1'))
Assert-True ($officialGate.Contains("tests\ci-workflow-tests.ps1")) `
    'Official gate must enforce its workflow contract test'

$lock = [IO.File]::ReadAllText((Join-Path $root 'tools\toolchain.lock.json')) | ConvertFrom-Json
Assert-True ($null -ne $lock.ffmpeg) `
    'Portable toolchain lock must provision FFmpeg for the official gate'
foreach ($property in 'version','url','sha256','exe','exe_sha256','ffprobe_exe','ffprobe_exe_sha256') {
    Assert-True ($lock.ffmpeg.PSObject.Properties.Name -ccontains $property) `
        "FFmpeg lock is missing $property"
}
$bootstrap = [IO.File]::ReadAllText((Join-Path $root 'tools\bootstrap.ps1'))
Assert-True ($bootstrap.Contains("Install-Portable 'ffmpeg'")) `
    'InstallPortable must install the lock-pinned FFmpeg archive'
Assert-True ($bootstrap.Contains('ffprobe_exe_sha256')) `
    'Bootstrap must validate the secondary ffprobe executable pin'
Assert-True ($bootstrap.Contains('$ffprobeVersionOutput = @(& $ffprobe -version 2>&1)')) `
    'Bootstrap must consume complete FFprobe version output without closing its pipe early'
Assert-True ($bootstrap.Contains("if (`$ffprobeVersionExitCode -ne 0)")) `
    'Bootstrap must reject a failed FFprobe version process'
Assert-True ($officialGate.Contains("-ToolName 'ffmpeg'")) `
    'Official gate must resolve its media probe from the portable lock'
Assert-True (-not $officialGate.Contains("Get-Command 'ffprobe.exe'")) `
    'Official gate must not depend on runner-provided ffprobe'

Write-Output 'ci workflow contract: PASS'
