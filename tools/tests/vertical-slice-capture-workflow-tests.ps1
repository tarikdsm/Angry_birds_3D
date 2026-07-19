[CmdletBinding()]
param([Parameter(Mandatory)][string]$Root)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $Root 'tools\VerticalSliceGate.psm1') -Force -Scope Local
Import-Module (Join-Path $Root 'tools\SafePath.psm1') -Force

function Assert-True {
    param([bool]$Condition, [string]$Message)

    if (-not $Condition) { throw $Message }
}

function Assert-Throws {
    param([scriptblock]$Operation, [string]$Expected)

    try { & $Operation } catch {
        if ($_.Exception.Message -notlike "*$Expected*") {
            throw "Unexpected failure: $($_.Exception.Message)"
        }
        return
    }
    throw "Expected failure containing: $Expected"
}

$module = Join-Path $Root 'tools\VerticalSliceCaptureWorkflow.psm1'
if (-not (Test-Path -LiteralPath $module -PathType Leaf)) {
    throw 'VerticalSliceCaptureWorkflow.psm1 missing (expected RED before implementation)'
}
Import-Module $module -Force

$tempBase = [IO.Path]::GetTempPath().TrimEnd('\', '/')
$fixtureRoot = Join-Path $tempBase ('ninho-capture-workflow-' + [Guid]::NewGuid().ToString('N'))
Assert-NinhoNoReparseAncestors -Path $fixtureRoot -AllowedRoot $tempBase | Out-Null

try {
    $artifactDirectory = Join-Path $fixtureRoot 'artifacts\vertical-slice\debug'
    $goldenDirectory = Join-Path $fixtureRoot 'docs\art\goldens\vertical-slice\debug'
    New-Item -ItemType Directory -Force -Path $artifactDirectory,$goldenDirectory | Out-Null

    $writeManifest = {
        param([string]$OutputDirectory, [string]$Configuration = 'Debug')

        New-Item -ItemType Directory -Force -Path $OutputDirectory,$goldenDirectory | Out-Null
        $captureLog = Join-Path $OutputDirectory 'capture.log'
        Set-Content -LiteralPath $captureLog -Value 'capture-ok' -Encoding UTF8
        $sourceArtifacts = [Collections.Generic.List[object]]::new()
        $relativeCaptureLog = $captureLog.Substring($fixtureRoot.Length + 1).Replace('\','/')
        $sourceArtifacts.Add([ordered]@{
            path=$relativeCaptureLog
            sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $captureLog).Hash.ToLowerInvariant()
        })

        $goldenMetadata = [Collections.Generic.List[object]]::new()
        foreach ($name in 'overview','aim','virela','vulnerable_impact','result') {
            $goldenPath = Join-Path $goldenDirectory "$name.png"
            Set-Content -LiteralPath $goldenPath -Value "golden-$name" -Encoding UTF8
            $relativeGolden = $goldenPath.Substring($fixtureRoot.Length + 1).Replace('\','/')
            $goldenHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $goldenPath).Hash.ToLowerInvariant()
            $sourceArtifacts.Add([ordered]@{path=$relativeGolden;sha256=$goldenHash})
            $goldenMetadata.Add([ordered]@{name=$name;path=$relativeGolden;sha256=$goldenHash})
        }

        $manifest = [ordered]@{
            schema='ninho.vertical-slice.capture.v1'
            configuration=$Configuration
            goldens=@('overview','aim','virela','vulnerable_impact','result')
            golden_metadata=@($goldenMetadata)
            source_artifacts=@($sourceArtifacts)
        }
        $manifestPath = Join-Path $OutputDirectory 'capture-manifest.json'
        $manifest | ConvertTo-Json -Depth 10 |
            Set-Content -LiteralPath $manifestPath -Encoding UTF8
        return $manifestPath
    }.GetNewClosure()

    $captureState = [pscustomobject]@{ Calls = 0 }
    $captureOperation = {
        param([string]$OutputDirectory)
        Import-Module (Join-Path $Root 'tools\VerticalSliceGate.psm1') -Force
        Import-Module (Join-Path $Root 'tools\ToolchainIntegrity.psm1') -Force
        Import-Module (Join-Path $Root 'tools\SafePath.psm1') -Force
        $captureState.Calls += 1
        & $writeManifest $OutputDirectory Debug
    }.GetNewClosure()

    $sentinel = Join-Path $artifactDirectory 'remove-me.txt'
    Set-Content -LiteralPath $sentinel -Value 'stale' -Encoding UTF8
    $captured = Invoke-NinhoVerticalSliceCaptureWorkflow `
        -Root $fixtureRoot -Configuration Debug `
        -ArtifactDirectory $artifactDirectory -Mode CaptureOnly `
        -CaptureOperation $captureOperation
    Assert-True ($captureState.Calls -eq 1) 'CaptureOnly must invoke capture exactly once'
    Assert-True (-not (Test-Path -LiteralPath $sentinel)) `
        'CaptureOnly must clean the canonical capture directory'
    Assert-True ($captured.Manifest.configuration -ceq 'Debug') `
        'CaptureOnly returned the wrong configuration'
    Assert-True ($captured.ManifestSha256 -match '^[0-9a-f]{64}$') `
        'CaptureOnly returned an invalid manifest SHA-256'

    $parentCommandWasRemoved = $null -eq (
        Get-Command Get-NinhoTestedInputs -ErrorAction SilentlyContinue)
    Import-Module (Join-Path $Root 'tools\VerticalSliceGate.psm1') -Force -Scope Local
    Import-Module (Join-Path $Root 'tools\SafePath.psm1') -Force -Scope Local
    $connectedIdentity = VerticalSliceGate\Get-NinhoTestedInputs -Root $Root
    Assert-True ($connectedIdentity.sha256 -match '^[0-9a-f]{64}$') `
        'JIT module-qualified tested-input identity failed after capture contamination'
    if ($parentCommandWasRemoved) {
        Write-Verbose 'connected capture fixture reproduced parent-scope module contamination'
    }

    $runnerText = Get-Content -Raw -LiteralPath (Join-Path $Root 'tools\test_vertical_slice.ps1')
    Assert-True ($runnerText -match '(?ms)Invoke-NinhoVerticalSliceCaptureWorkflow.*?Import-Module \(Join-Path \$PSScriptRoot ''VerticalSliceGate\.psm1''\) -Force -Scope Local.*?\$testedInputs = VerticalSliceGate\\Get-NinhoTestedInputs') `
        'vertical slice runner must restore and qualify VerticalSliceGate after capture'
    Assert-True ($runnerText -match 'vertical-slice-capture-workflow-tests\.ps1') `
        'vertical slice runner does not execute the connected capture workflow regression'

    $preserved = Join-Path $artifactDirectory 'preserve-me.txt'
    Set-Content -LiteralPath $preserved -Value 'reviewed capture' -Encoding UTF8
    $existing = Invoke-NinhoVerticalSliceCaptureWorkflow `
        -Root $fixtureRoot -Configuration Debug `
        -ArtifactDirectory $artifactDirectory -Mode UseExistingCapture `
        -CaptureOperation { throw 'UseExistingCapture invoked capture' }
    Assert-True (Test-Path -LiteralPath $preserved -PathType Leaf) `
        'UseExistingCapture must preserve existing capture files'
    Assert-True ($captureState.Calls -eq 1) `
        'UseExistingCapture must not increment capture calls'
    Assert-True ($existing.ManifestSha256 -ceq $captured.ManifestSha256) `
        'UseExistingCapture must preserve manifest identity'

    $legacySentinel = Join-Path $artifactDirectory 'legacy-remove-me.txt'
    Set-Content -LiteralPath $legacySentinel -Value 'stale' -Encoding UTF8
    $null = Invoke-NinhoVerticalSliceCaptureWorkflow `
        -Root $fixtureRoot -Configuration Debug `
        -ArtifactDirectory $artifactDirectory -Mode Legacy `
        -CaptureOperation $captureOperation
    Assert-True ($captureState.Calls -eq 2) 'Legacy must invoke capture exactly once per run'
    Assert-True (-not (Test-Path -LiteralPath $legacySentinel)) `
        'Legacy must clean the canonical capture directory'

    Assert-Throws {
        Invoke-NinhoVerticalSliceCaptureWorkflow `
            -Root $fixtureRoot -Configuration Debug `
            -ArtifactDirectory (Join-Path $fixtureRoot 'artifacts\other') `
            -Mode UseExistingCapture
    } 'canonical artifact directory'

    $manifestPath = & $writeManifest $artifactDirectory Release
    Assert-Throws {
        Invoke-NinhoVerticalSliceCaptureWorkflow `
            -Root $fixtureRoot -Configuration Debug `
            -ArtifactDirectory $artifactDirectory -Mode UseExistingCapture
    } 'configuration mismatch'

    $manifestPath = & $writeManifest $artifactDirectory Debug
    $document = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    $document.schema = 'ninho.vertical-slice.capture.v0'
    $document | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    Assert-Throws {
        Invoke-NinhoVerticalSliceCaptureWorkflow `
            -Root $fixtureRoot -Configuration Debug `
            -ArtifactDirectory $artifactDirectory -Mode UseExistingCapture
    } 'schema mismatch'

    $manifestPath = & $writeManifest $artifactDirectory Debug
    $document = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    $document.source_artifacts = @()
    $document | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    Assert-Throws {
        Invoke-NinhoVerticalSliceCaptureWorkflow `
            -Root $fixtureRoot -Configuration Debug `
            -ArtifactDirectory $artifactDirectory -Mode UseExistingCapture
    } 'source_artifacts is empty'

    $manifestPath = & $writeManifest $artifactDirectory Debug
    $document = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    $document.source_artifacts[0].sha256 = '0' * 64
    $document | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    Assert-Throws {
        Invoke-NinhoVerticalSliceCaptureWorkflow `
            -Root $fixtureRoot -Configuration Debug `
            -ArtifactDirectory $artifactDirectory -Mode UseExistingCapture
    } 'SHA-256 mismatch'

    $manifestPath = & $writeManifest $artifactDirectory Debug
    $document = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    Remove-Item -LiteralPath (Join-Path $fixtureRoot $document.source_artifacts[0].path)
    Assert-Throws {
        Invoke-NinhoVerticalSliceCaptureWorkflow `
            -Root $fixtureRoot -Configuration Debug `
            -ArtifactDirectory $artifactDirectory -Mode UseExistingCapture
    } 'artifact missing'

    $manifestPath = & $writeManifest $artifactDirectory Debug
    $document = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    $document.source_artifacts[0].path = '../escape.log'
    $document | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    Assert-Throws {
        Invoke-NinhoVerticalSliceCaptureWorkflow `
            -Root $fixtureRoot -Configuration Debug `
            -ArtifactDirectory $artifactDirectory -Mode UseExistingCapture
    } 'unsafe artifact path'

    $manifestPath = & $writeManifest $artifactDirectory Debug
    $document = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    $document.golden_metadata = @($document.golden_metadata | Where-Object name -CNE 'result')
    $document | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    Assert-Throws {
        Invoke-NinhoVerticalSliceCaptureWorkflow `
            -Root $fixtureRoot -Configuration Debug `
            -ArtifactDirectory $artifactDirectory -Mode UseExistingCapture
    } 'five canonical goldens'

    $manifestPath = & $writeManifest $artifactDirectory Debug
    $document = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    $document.golden_metadata[0].sha256 = '0' * 64
    $document | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    Assert-Throws {
        Invoke-NinhoVerticalSliceCaptureWorkflow `
            -Root $fixtureRoot -Configuration Debug `
            -ArtifactDirectory $artifactDirectory -Mode UseExistingCapture
    } 'golden/source binding mismatch'

    $manifestPath = & $writeManifest $artifactDirectory Debug
    Remove-Item -LiteralPath $manifestPath
    Assert-Throws {
        Invoke-NinhoVerticalSliceCaptureWorkflow `
            -Root $fixtureRoot -Configuration Debug `
            -ArtifactDirectory $artifactDirectory -Mode UseExistingCapture
    } 'manifest missing'
}
finally {
    if (Test-Path -LiteralPath $fixtureRoot) {
        Import-Module (Join-Path $Root 'tools\SafePath.psm1') -Force -Scope Local
        SafePath\Assert-NinhoNoReparseAncestors `
            -Path $fixtureRoot -AllowedRoot $tempBase | Out-Null
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}

Write-Output 'vertical-slice-capture-workflow-tests: PASS'
