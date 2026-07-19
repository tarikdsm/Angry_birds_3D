[CmdletBinding()]
param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $Root 'tools\SafePath.psm1') -Force
Import-Module (Join-Path $Root 'tools\TestedInputIdentity.psm1') -Force

function Assert-True {
    param([bool]$Condition, [string]$Message)

    if (-not $Condition) {
        throw $Message
    }
}

function Get-BytesSha256 {
    param([Parameter(Mandatory)][byte[]]$Bytes)

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        return -join ($algorithm.ComputeHash($Bytes) |
            ForEach-Object { $_.ToString('x2') })
    }
    finally {
        $algorithm.Dispose()
    }
}

Assert-True ($null -ne (Get-Command Assert-NinhoNoReparseAncestors `
            -ErrorAction SilentlyContinue)) `
    'Importing TestedInputIdentity must not remove SafePath from the caller scope'

$lock = [IO.File]::ReadAllText((Join-Path $Root 'tools\toolchain.lock.json')) |
    ConvertFrom-Json
$expectedHash = [string]$lock.godot_export_templates.copyright_sha256
$copyrightPath = Join-Path $Root 'third_party\godot.COPYRIGHT.txt'
$canonicalSource = Get-NinhoCanonicalTestedInputContent `
    -Path $copyrightPath -RelativePath 'third_party/godot.COPYRIGHT.txt'
Assert-True ($canonicalSource.mode -ceq 'text_utf8_lf') `
    'Godot copyright inventory must be canonicalizable UTF-8 text'
Assert-True ((Get-BytesSha256 -Bytes $canonicalSource.bytes) -ceq $expectedHash) `
    'Godot copyright canonical LF hash differs from the upstream pin'

$tempBase = [IO.Path]::GetTempPath().TrimEnd('\', '/')
$tempRoot = Join-Path $tempBase ('ninho-godot-copyright-eol-' + [Guid]::NewGuid().ToString('N'))
Assert-NinhoNoReparseAncestors -Path $tempRoot -AllowedRoot $tempBase | Out-Null

try {
    New-Item -ItemType Directory -Path $tempRoot | Out-Null
    $crlfPath = Join-Path $tempRoot 'godot.COPYRIGHT.txt'
    $canonicalText = [Text.UTF8Encoding]::new($false, $true).GetString(
        [byte[]]$canonicalSource.bytes)
    $crlfBytes = [Text.UTF8Encoding]::new($false).GetBytes(
        $canonicalText.Replace("`n", "`r`n"))
    [IO.File]::WriteAllBytes($crlfPath, $crlfBytes)

    Assert-True ($crlfBytes -contains 13) 'CRLF fixture does not contain carriage returns'
    Assert-True ((Get-BytesSha256 -Bytes $crlfBytes) -cne $expectedHash) `
        'CRLF fixture must demonstrate why a raw-byte hash is checkout-sensitive'
    $canonicalCrlf = Get-NinhoCanonicalTestedInputContent `
        -Path $crlfPath -RelativePath 'third_party/godot.COPYRIGHT.txt'
    Assert-True ($canonicalCrlf.mode -ceq 'text_utf8_lf') `
        'CRLF Godot copyright fixture must remain canonicalizable UTF-8 text'
    Assert-True ((Get-BytesSha256 -Bytes $canonicalCrlf.bytes) -ceq $expectedHash) `
        'CRLF Godot copyright content must match the canonical upstream pin'
    Assert-True ([Convert]::ToBase64String($canonicalCrlf.bytes) -ceq
        [Convert]::ToBase64String($canonicalSource.bytes)) `
        'LF and CRLF Godot copyright content must canonicalize to identical bytes'
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Assert-NinhoNoReparseAncestors -Path $tempRoot -AllowedRoot $tempBase | Out-Null
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}

$package = [IO.File]::ReadAllText((Join-Path $Root 'tools\package_windows.ps1'))
$packageTest = [IO.File]::ReadAllText(
    (Join-Path $Root 'tools\tests\package-windows-tests.ps1'))
foreach ($consumer in @($package, $packageTest)) {
    Assert-True ($consumer.Contains('Get-NinhoCanonicalTestedInputContent')) `
        'Package production and verification must reuse shared text canonicalization'
}

Write-Output 'godot-copyright-eol-contract-tests: PASS'
