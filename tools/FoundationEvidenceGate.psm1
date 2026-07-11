Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'SpikeEvidenceValidation.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force

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
}

Export-ModuleMember -Function Assert-NinhoFoundationEvidence
