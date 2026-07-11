[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$Root
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $Root 'tools\FoundationEvidenceGate.psm1') -Force

function Assert-Throws {
    param([scriptblock]$Operation, [string]$ExpectedMessage)
    try { & $Operation } catch {
        if ($_.Exception.Message -notlike "*$ExpectedMessage*") {
            throw "Unexpected failure: $($_.Exception.Message)"
        }
        return
    }
    throw "Expected failure containing: $ExpectedMessage"
}

$sandbox = Join-Path $Root 'artifacts\physics\foundation-evidence-gate-test'
New-Item -ItemType Directory -Force -Path $sandbox | Out-Null
$rootPrefix = [System.IO.Path]::GetFullPath($Root).TrimEnd('\', '/') + '\'
$relativeSandbox = [System.IO.Path]::GetFullPath($sandbox).Substring(
    $rootPrefix.Length).Replace('\', '/')
$reportPath = Join-Path $sandbox 'report.md'

$documents = @{}
foreach ($configuration in 'Debug', 'Release') {
    $path = Join-Path $sandbox "foundation-report-$($configuration.ToLowerInvariant()).json"
    $document = [ordered]@{
        build_type = $configuration
        recommendation = 'prosseguir_com_limites'
        violations = @()
        scenarios = @(
            foreach ($name in @(
                    'radial_fall',
                    'projectile_pile',
                    'radial_pile',
                    'mass_ratio',
                    'stress',
                    'capability_matrix')) {
                [ordered]@{
                    name = $name
                    hashes = @(11, 11)
                    peak_body_count = if ($name -eq 'radial_fall') { 2 } else { 1 }
                    peak_shape_count = if ($name -eq 'radial_fall') { 2 } else { 1 }
                    peak_joint_count = 0
                }
            }
        )
    }
    $document | ConvertTo-Json -Depth 6 |
        Set-Content -LiteralPath $path -Encoding utf8
    $documents[$configuration] = [ordered]@{
        Path = $path
        Relative = "$relativeSandbox/$([System.IO.Path]::GetFileName($path))"
        Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash
    }
}

@"
# Audit report
Evidence-Debug-Path: $($documents.Debug.Relative)
Evidence-Debug-SHA256: $($documents.Debug.Hash)
Evidence-Release-Path: $($documents.Release.Relative)
Evidence-Release-SHA256: $($documents.Release.Hash)
"@ | Set-Content -LiteralPath $reportPath -Encoding utf8

Assert-NinhoFoundationEvidence -Root $Root -ReportPath $reportPath
Add-Content -LiteralPath $documents.Debug.Path -Value ' ' -Encoding ascii
Assert-Throws {
    Assert-NinhoFoundationEvidence -Root $Root -ReportPath $reportPath
} 'SHA-256 mismatch'

Remove-Item -LiteralPath $sandbox -Recurse -Force
Write-Output 'foundation-evidence-gate-tests: PASS'
