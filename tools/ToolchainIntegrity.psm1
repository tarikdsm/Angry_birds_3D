Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1')

function Assert-NinhoPinnedExecutable {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$ExpectedSha256,
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$AllowedRoot
    )

    Assert-NinhoNoReparseAncestors -Path $Path -AllowedRoot $AllowedRoot | Out-Null
    if ($ExpectedSha256 -notmatch '^[0-9a-f]{64}$') {
        throw "$Name pinned SHA-256 is invalid: $ExpectedSha256"
    }
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Name pinned executable is missing: $Path"
    }
    $actualSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant()
    if ($actualSha256 -cne $ExpectedSha256) {
        throw "$Name executable checksum mismatch: expected=$ExpectedSha256 actual=$actualSha256"
    }
}

function Resolve-NinhoPinnedToolchainExecutable {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][string]$ToolName,
        [string]$ExecutableProperty = 'exe',
        [string]$HashProperty = 'exe_sha256',
        [Parameter(Mandatory)][string]$Name
    )

    $resolvedRoot = [IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    if ($ToolName -notmatch '^[a-z0-9_-]+$' -or
            $ExecutableProperty -notmatch '^[a-z0-9_]+$' -or
            $HashProperty -notmatch '^[a-z0-9_]+$') {
        throw "$Name toolchain lock selector is invalid"
    }
    $lockPath = Join-Path $resolvedRoot 'tools\toolchain.lock.json'
    Assert-NinhoNoReparseAncestors -Path $lockPath -AllowedRoot $resolvedRoot | Out-Null
    if (-not (Test-Path -LiteralPath $lockPath -PathType Leaf)) {
        throw "$Name toolchain lock is missing: $lockPath"
    }
    $lock = [IO.File]::ReadAllText($lockPath) | ConvertFrom-Json
    $toolProperty = $lock.PSObject.Properties[$ToolName]
    if ($null -eq $toolProperty) { throw "$Name toolchain lock entry is missing: $ToolName" }
    $entry = $toolProperty.Value
    $pathProperty = $entry.PSObject.Properties[$ExecutableProperty]
    $hashPropertyValue = $entry.PSObject.Properties[$HashProperty]
    if ($null -eq $pathProperty -or [string]::IsNullOrWhiteSpace([string]$pathProperty.Value)) {
        throw "$Name toolchain executable path is missing: $ToolName.$ExecutableProperty"
    }
    if ($null -eq $hashPropertyValue) {
        throw "$Name toolchain executable hash is missing: $ToolName.$HashProperty"
    }
    $toolRoot = Join-Path $resolvedRoot ".tools\$ToolName"
    $executable = [IO.Path]::GetFullPath((Join-Path $toolRoot ([string]$pathProperty.Value)))
    Assert-NinhoPinnedExecutable `
        -Path $executable `
        -ExpectedSha256 ([string]$hashPropertyValue.Value) `
        -Name $Name `
        -AllowedRoot $toolRoot
    return $executable
}

Export-ModuleMember -Function Assert-NinhoPinnedExecutable,Resolve-NinhoPinnedToolchainExecutable
