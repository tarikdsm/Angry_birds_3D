Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'SpikeEvidenceValidation.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'TestedInputIdentity.psm1') -Force

function Get-NinhoFoundationTestedInputPaths {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Root)
    $rootPath = [System.IO.Path]::GetFullPath($Root)
    [string[]]$requiredPaths = @(
        'CMakeLists.txt',
        'CMakePresets.json',
        'tools/bootstrap.ps1',
        'tools/box3d-v0.1.0-compile-sources.txt',
        'tools/build.ps1',
        'tools/FoundationEvidenceGate.psm1',
        'tools/generate_foundation_report.py',
        'tools/GodotSmokeRegistry.psm1',
        'tools/GodotSpikeGate.psm1',
        'tools/Invoke-Native.ps1',
        'tools/run_spike.ps1',
        'tools/SafePath.psm1',
        'tools/SpikeEvidenceValidation.psm1',
        'tools/SpikeReportGate.psm1',
        'tools/test.ps1',
        'tools/TestedInputIdentity.psm1',
        'tools/ToolchainIntegrity.psm1',
        'tools/toolchain.lock.json',
        'tools/UpstreamBox3DGate.psm1',
        'tools/VerticalSliceGate.psm1'
    )
    [string[]]$pathSpecs = @('native', 'cmake') + $requiredPaths
    $paths = @(& git -c "safe.directory=$rootPath" -C $rootPath `
        ls-files --cached --others --exclude-standard -- $pathSpecs)
    if ($LASTEXITCODE -ne 0) {
        throw 'unable to enumerate foundation tested inputs with git'
    }
    $requiredSet = [Collections.Generic.HashSet[string]]::new(
        $requiredPaths,
        [StringComparer]::Ordinal)
    $unique = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::Ordinal)
    foreach ($path in $paths) {
        $relative = ([string]$path).Replace('\','/')
        if ($relative -match '^(native|cmake)/.+' -or
                $requiredSet.Contains($relative)) {
            $null = $unique.Add($relative)
        }
    }
    foreach ($required in $requiredPaths) {
        $requiredPath = Join-Path $rootPath $required
        if (-not $unique.Contains($required) -or
                -not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            throw "required foundation tested input is missing: $required"
        }
    }
    [string[]]$filtered = @($unique)
    [Array]::Sort($filtered, [StringComparer]::Ordinal)
    if ($filtered.Count -eq 0) {
        throw 'foundation tested input set is empty'
    }
    return $filtered
}

function Get-NinhoFoundationTestedInputs {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Root)
    return Get-NinhoTestedInputIdentity -Root $Root `
        -RelativePaths @(Get-NinhoFoundationTestedInputPaths -Root $Root)
}

function Add-NinhoFoundationEvidenceIdentity {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)]$Document
    )
    $rootPath = [System.IO.Path]::GetFullPath($Root)
    $sourceRevision = [string](& git -c "safe.directory=$rootPath" `
        -C $rootPath rev-parse HEAD 2>$null)
    if ($LASTEXITCODE -ne 0 -or $sourceRevision -notmatch '^[0-9a-f]{40}$') {
        throw 'unable to resolve foundation source revision'
    }
    $identity = Get-NinhoFoundationTestedInputs -Root $rootPath
    $Document | Add-Member -NotePropertyName source_revision `
        -NotePropertyValue $sourceRevision -Force
    $Document | Add-Member -NotePropertyName tested_inputs_schema `
        -NotePropertyValue 'ninho.tested-inputs.v2' -Force
    $Document | Add-Member -NotePropertyName tested_inputs_sha256 `
        -NotePropertyValue $identity.sha256 -Force
    $Document | Add-Member -NotePropertyName tested_inputs `
        -NotePropertyValue @($identity.files) -Force
    return $Document
}

function Publish-NinhoFoundationEvidence {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][string]$ArtifactDirectory
    )
    $rootPath = [System.IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    $artifactRoot = [System.IO.Path]::GetFullPath($ArtifactDirectory).TrimEnd('\','/')
    if (-not $artifactRoot.StartsWith(
            $rootPath + [System.IO.Path]::DirectorySeparatorChar,
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "foundation artifact directory escapes repository: $artifactRoot"
    }
    Assert-NinhoNoReparseAncestors -Path $artifactRoot -AllowedRoot $rootPath | Out-Null
    $testedInputs = Get-NinhoFoundationTestedInputs -Root $rootPath
    $entries = [Collections.Generic.List[object]]::new()
    foreach ($configuration in 'Debug', 'Release') {
        $preset = $configuration.ToLowerInvariant()
        $source = Join-Path $artifactRoot "box3d-spike-$preset.json"
        Assert-NinhoNoReparseAncestors -Path $source -AllowedRoot $rootPath | Out-Null
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
            throw "foundation artifact missing for publication: $source"
        }
        try {
            $raw = [System.IO.File]::ReadAllBytes($source)
            $document = [System.IO.File]::ReadAllText($source) | ConvertFrom-Json
        } catch {
            throw "foundation artifact is not valid JSON for ${configuration}: $($_.Exception.Message)"
        }
        Assert-NinhoSpikeEvidenceDocument -Document $document `
            -ExpectedBuildType $configuration
        Assert-NinhoTestedInputIdentity -Root $rootPath -Document $document `
            -ExpectedIdentity $testedInputs
        $relative = "docs/physics/evidence/foundation-report-$preset.json"
        $destination = Join-Path $rootPath $relative
        Assert-NinhoNoReparseAncestors -Path $destination -AllowedRoot $rootPath | Out-Null
        & git -c "safe.directory=$rootPath" -C $rootPath `
            ls-files --error-unmatch -- $relative 2>$null | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "canonical foundation evidence is not tracked by git: $relative"
        }
        $entries.Add([pscustomobject]@{
            Configuration=$configuration
            Document=$document
            Raw=$raw
            Destination=$destination
        })
    }
    if ($entries[0].Document.source_revision -cne $entries[1].Document.source_revision -or
            $entries[0].Document.tested_inputs_sha256 -cne
                $entries[1].Document.tested_inputs_sha256) {
        throw 'Debug and Release foundation evidence source identities diverge'
    }

    $reportPath = Join-Path $rootPath 'docs\physics\box3d-spike-report.md'
    Assert-NinhoNoReparseAncestors -Path $reportPath -AllowedRoot $rootPath | Out-Null
    $backups = @($entries | ForEach-Object {
        [pscustomobject]@{
            Path=$_.Destination
            Bytes=[System.IO.File]::ReadAllBytes($_.Destination)
        }
    })
    $reportExisted = Test-Path -LiteralPath $reportPath -PathType Leaf
    $reportBackup = if ($reportExisted) {
        [System.IO.File]::ReadAllBytes($reportPath)
    } else { $null }
    try {
        foreach ($entry in $entries) {
            [System.IO.File]::WriteAllBytes($entry.Destination, $entry.Raw)
        }
        $generator = Join-Path $PSScriptRoot 'generate_foundation_report.py'
        $python = Get-Command python -CommandType Application -ErrorAction Stop |
            Select-Object -First 1
        $generatorOutput = @(& $python.Source $generator --root $rootPath --write 2>&1)
        if ($LASTEXITCODE -ne 0) {
            throw "failed to regenerate canonical foundation report: $($generatorOutput -join ' ')"
        }
        Assert-NinhoFoundationEvidence -Root $rootPath -ReportPath $reportPath
    } catch {
        foreach ($backup in $backups) {
            [System.IO.File]::WriteAllBytes($backup.Path, $backup.Bytes)
        }
        if ($reportExisted) {
            [System.IO.File]::WriteAllBytes($reportPath, $reportBackup)
        } elseif (Test-Path -LiteralPath $reportPath -PathType Leaf) {
            Remove-Item -LiteralPath $reportPath -Force
        }
        throw
    }
}

function Get-NinhoSingleReportToken {
    param(
        [Parameter(Mandatory)] [string]$Report,
        [Parameter(Mandatory)] [string]$Name,
        [Parameter(Mandatory)] [string]$ValuePattern
    )

    $matches = [regex]::Matches(
        $Report,
        "(?m)^$([regex]::Escape($Name)):\s*($ValuePattern)\s*$")
    if ($matches.Count -ne 1) {
        throw "Foundation report must contain exactly one $Name token"
    }
    return $matches[0].Groups[1].Value
}

function Assert-NinhoGeneratedFoundationReport {
    param(
        [Parameter(Mandatory)] [string]$Root,
        [Parameter(Mandatory)] [string]$ReportPath
    )

    $generator = Join-Path $PSScriptRoot 'generate_foundation_report.py'
    if (-not (Test-Path -LiteralPath $generator -PathType Leaf)) {
        throw "Foundation report generator is missing: $generator"
    }
    $python = Get-Command python -CommandType Application -ErrorAction Stop |
        Select-Object -First 1
    Assert-NinhoNoReparseAncestors -Path $generator -AllowedRoot $PSScriptRoot | Out-Null
    Assert-NinhoNoReparseAncestors -Path $ReportPath -AllowedRoot $Root | Out-Null
    foreach ($relative in @(
            'docs\physics\evidence\foundation-report-debug.json',
            'docs\physics\evidence\foundation-report-release.json')) {
        Assert-NinhoNoReparseAncestors `
            -Path (Join-Path $Root $relative) `
            -AllowedRoot $Root | Out-Null
    }
    $previousErrorActionPreference = $ErrorActionPreference
    $generatorOutput = @()
    $generatorExitCode = 1
    try {
        $ErrorActionPreference = 'Continue'
        $generatorOutput = @(& $python.Source $generator `
            --root $Root --check --report-path $ReportPath 2>&1)
        $generatorExitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    # Revalidate after the external generator as well: success must not hide
    # a path swap performed while the process was running.
    Assert-NinhoNoReparseAncestors -Path $ReportPath -AllowedRoot $Root | Out-Null
    foreach ($relative in @(
            'docs\physics\evidence\foundation-report-debug.json',
            'docs\physics\evidence\foundation-report-release.json')) {
        Assert-NinhoNoReparseAncestors `
            -Path (Join-Path $Root $relative) `
            -AllowedRoot $Root | Out-Null
    }
    if ($generatorExitCode -ne 0) {
        $detail = ($generatorOutput | ForEach-Object { [string]$_ }) -join ' '
        throw "Foundation generated report is out of date: $detail"
    }
}

function Assert-NinhoFoundationEvidence {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$Root,
        [Parameter(Mandatory)] [string]$ReportPath
    )

    $rootPath = [System.IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $rootPrefix = $rootPath + [System.IO.Path]::DirectorySeparatorChar
    $resolvedReportPath = [System.IO.Path]::GetFullPath($ReportPath)
    Assert-NinhoNoReparseAncestors -Path $resolvedReportPath -AllowedRoot $rootPath | Out-Null
    $report = [System.IO.File]::ReadAllText($resolvedReportPath)
    $expectedScenarios = @(
        'radial_fall',
        'projectile_pile',
        'radial_pile',
        'mass_ratio',
        'stress',
        'capability_matrix'
    )
    $testedInputs = Get-NinhoFoundationTestedInputs -Root $rootPath

    foreach ($configuration in 'Debug', 'Release') {
        $canonicalRelative = "docs/physics/evidence/foundation-report-$($configuration.ToLowerInvariant()).json"
        $relative = Get-NinhoSingleReportToken `
            -Report $report `
            -Name "Evidence-$configuration-Path" `
            -ValuePattern '\S+'
        if ($relative -cne $canonicalRelative) {
            throw "Foundation evidence path must be exactly $canonicalRelative"
        }
        $expectedHash = (Get-NinhoSingleReportToken `
            -Report $report `
            -Name "Evidence-$configuration-SHA256" `
            -ValuePattern '[0-9A-Fa-f]{64}').ToUpperInvariant()
        if ([System.IO.Path]::IsPathRooted($relative)) {
            throw "Foundation evidence path must be repository-relative: $relative"
        }
        $path = [System.IO.Path]::GetFullPath((Join-Path $rootPath $relative))
        if (-not $path.StartsWith(
                $rootPrefix,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Foundation evidence escapes the repository: $relative"
        }
        Assert-NinhoNoReparseAncestors -Path $path -AllowedRoot $rootPath | Out-Null
        $previousErrorActionPreference = $ErrorActionPreference
        $gitExitCode = 1
        try {
            $ErrorActionPreference = 'Continue'
            & git -c "safe.directory=$rootPath" -C $rootPath `
                ls-files --error-unmatch -- $relative 2>$null | Out-Null
            $gitExitCode = $LASTEXITCODE
        } finally {
            $ErrorActionPreference = $previousErrorActionPreference
        }
        if ($gitExitCode -ne 0) {
            throw "Foundation evidence is not tracked by git: $relative"
        }
        # git is external and can run hooks/configured helpers. Revalidate the
        # evidence path again immediately before hashing and reading it.
        Assert-NinhoNoReparseAncestors -Path $path -AllowedRoot $rootPath | Out-Null
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Foundation evidence file missing: $relative"
        }
        $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash
        if ($actualHash -cne $expectedHash) {
            throw "Foundation evidence SHA-256 mismatch for ${configuration}: expected $expectedHash, got $actualHash"
        }
        try {
            $document = [System.IO.File]::ReadAllText($path) | ConvertFrom-Json
        } catch {
            throw "Foundation evidence is not valid JSON for ${configuration}: $($_.Exception.Message)"
        }
        Assert-NinhoSpikeEvidenceDocument `
            -Document $document `
            -ExpectedBuildType $configuration
        Assert-NinhoTestedInputIdentity -Root $rootPath -Document $document `
            -ExpectedIdentity $testedInputs
        $matrixScenario = @($document.scenarios | Where-Object name -CEQ 'capability_matrix')
        if ($matrixScenario.Count -ne 1) {
            throw "Foundation evidence missing unique capability_matrix for $configuration"
        }
        $reportMatrixHash = Get-NinhoSingleReportToken `
            -Report $report `
            -Name "Matrix-$configuration-Hash" `
            -ValuePattern '\d+'
        $reportMatrixTopology = Get-NinhoSingleReportToken `
            -Report $report `
            -Name "Matrix-$configuration-Topology" `
            -ValuePattern '\d+/\d+/\d+;\d+/\d+'
        $reportRecommendation = Get-NinhoSingleReportToken `
            -Report $report `
            -Name "Recommendation-$configuration" `
            -ValuePattern 'prosseguir_com_limites'
        $expectedTopology = "$($matrixScenario[0].peak_body_count)/$($matrixScenario[0].peak_shape_count)/$($matrixScenario[0].peak_joint_count);$($matrixScenario[0].peak_awake_count)/$($matrixScenario[0].peak_contact_count)"
        if ($reportMatrixHash -cne [string]$matrixScenario[0].final_hash -or
                $reportMatrixTopology -cne $expectedTopology -or
                $reportRecommendation -cne [string]$document.recommendation) {
            throw "Foundation report machine-readable matrix contract mismatch for $configuration"
        }
        if ($document.build_type -cne $configuration) {
            throw "Foundation evidence build_type mismatch for $configuration"
        }
        if ($document.recommendation -ne 'prosseguir_com_limites' -or
                @($document.violations).Count -ne 0) {
            throw "Foundation evidence has a normative failure for $configuration"
        }
        $scenarios = @($document.scenarios)
        if ($scenarios.Count -ne $expectedScenarios.Count) {
            throw "Foundation evidence scenario count mismatch for $configuration"
        }
        foreach ($name in $expectedScenarios) {
            $scenario = @($scenarios | Where-Object name -CEQ $name)
            if ($scenario.Count -ne 1) {
                throw "Foundation evidence missing unique scenario $name for $configuration"
            }
            $scenario = $scenario[0]
            foreach ($property in 'peak_body_count', 'peak_shape_count', 'peak_joint_count') {
                if ($scenario.PSObject.Properties.Name -notcontains $property) {
                    throw "Foundation evidence missing $property for $configuration/$name"
                }
            }
            if (@($scenario.hashes).Count -ne 2 -or
                    $scenario.hashes[0] -ne $scenario.hashes[1]) {
                throw "Foundation evidence repeat hash mismatch for $configuration/$name"
            }
        }
        $radialFall = $scenarios | Where-Object name -CEQ 'radial_fall'
        if ($radialFall.peak_body_count -ne 2 -or
                $radialFall.peak_shape_count -ne 2 -or
                $radialFall.peak_joint_count -ne 0) {
            throw "Foundation radial_fall topology mismatch for $configuration"
        }
    }
    Assert-NinhoGeneratedFoundationReport -Root $rootPath -ReportPath $resolvedReportPath
}

Export-ModuleMember -Function Assert-NinhoFoundationEvidence,Get-NinhoFoundationTestedInputPaths,Get-NinhoFoundationTestedInputs,Add-NinhoFoundationEvidenceIdentity,Publish-NinhoFoundationEvidence
