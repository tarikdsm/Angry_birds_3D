Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'SpikeEvidenceValidation.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force

function Start-NinhoSpikeReportCapture {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$Path,
        [Parameter(Mandatory)] [string]$AllowedRoot
    )

    $target = [System.IO.Path]::GetFullPath($Path)
    $root = [System.IO.Path]::GetFullPath($AllowedRoot).TrimEnd('\', '/') +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $target.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Spike report target is outside the allowed root: $target"
    }
    if (Test-Path -LiteralPath $target -PathType Container) {
        throw "Spike report target is a directory: $target"
    }
    if (Test-Path -LiteralPath $target -PathType Leaf) {
        Assert-NinhoNoReparseAncestors -Path $target -AllowedRoot $AllowedRoot | Out-Null
        Remove-Item -LiteralPath $target -Force
    }
    return [DateTime]::UtcNow
}

function Read-NinhoFreshSpikeReport {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$Path,
        [Parameter(Mandatory)] [DateTime]$StartedUtc,
        [Parameter(Mandatory)]
        [ValidateSet('Debug', 'Release')]
        [string]$ExpectedBuildType
    )

    $target = [System.IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $target -PathType Leaf)) {
        throw "Successful spike executable did not create a report: $target"
    }
    $item = Get-Item -LiteralPath $target
    if ($item.Length -le 0) {
        throw "Successful spike executable created an empty report: $target"
    }
    if ($item.LastWriteTimeUtc -lt $StartedUtc.ToUniversalTime()) {
        throw "Spike report predates this execution: $target"
    }
    try {
        $document = [System.IO.File]::ReadAllText($target) | ConvertFrom-Json
        Assert-NinhoSpikeEvidenceDocument `
            -Document $document `
            -ExpectedBuildType $ExpectedBuildType
        return $document
    } catch {
        throw "Spike report is not valid JSON: $target; $($_.Exception.Message)"
    }
}

Export-ModuleMember -Function Start-NinhoSpikeReportCapture, Read-NinhoFreshSpikeReport
