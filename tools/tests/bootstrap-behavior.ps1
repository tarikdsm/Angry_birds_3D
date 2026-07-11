$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Import-Module (Join-Path $root 'tools\SafePath.psm1') -Force
$tempBase = [IO.Path]::GetTempPath().TrimEnd('\', '/')
$tempRoot = Join-Path $tempBase ('ninho-toolchain-tests-' + [Guid]::NewGuid().ToString('N'))
Assert-NinhoNoReparseAncestors -Path $tempRoot -AllowedRoot $tempBase | Out-Null

try {
    $fixtureTools = Join-Path $tempRoot 'tools'
    $visualStudio = Join-Path $tempRoot 'visual-studio'
    $devCmdDirectory = Join-Path $visualStudio 'Common7\Tools'
    New-Item -ItemType Directory -Force -Path $fixtureTools, $devCmdDirectory | Out-Null

    $fixtureInvoke = Join-Path $fixtureTools 'Invoke-Native.ps1'
    Copy-Item -LiteralPath (Join-Path $root 'tools\Invoke-Native.ps1') -Destination $fixtureInvoke

    $escapedVisualStudio = $visualStudio.Replace("'", "''")
    $fakeBootstrap = @"
[CmdletBinding()]
param([switch]`$CheckOnly, [switch]`$Json)
`$result = [ordered]@{
    ok = `$true
    errors = @()
    visual_studio = @{ path = '$escapedVisualStudio' }
}
if (`$Json) { `$result | ConvertTo-Json -Depth 3 } else { `$result }
"@
    Set-Content -LiteralPath (Join-Path $fixtureTools 'bootstrap.ps1') -Value $fakeBootstrap -Encoding ASCII
    Set-Content -LiteralPath (Join-Path $devCmdDirectory 'VsDevCmd.bat') -Value '@exit /b 23' -Encoding ASCII

    $sentinel = Join-Path $tempRoot 'command-ran.txt'
    $escapedSentinel = $sentinel.Replace("'", "''")
    $command = "Set-Content -LiteralPath '$escapedSentinel' -Value ran"
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $fixtureInvoke -Command $command 2>&1
    $exitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorActionPreference

    if (Test-Path -LiteralPath $sentinel) {
        throw 'Invoke-Native ran the command after VsDevCmd failed'
    }
    if ($exitCode -ne 23) {
        throw "Invoke-Native returned $exitCode instead of VsDevCmd exit code 23"
    }
    if (($output -join "`n") -notmatch 'VsDevCmd failed with exit code 23') {
        throw 'Invoke-Native did not report the VsDevCmd exit code'
    }

    Write-Output 'Invoke-Native VsDevCmd failure: PASS'
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Assert-NinhoNoReparseAncestors -Path $tempRoot -AllowedRoot $tempBase | Out-Null
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}

$tempRoot = Join-Path $tempBase ('ninho-toolchain-tests-' + [Guid]::NewGuid().ToString('N'))
Assert-NinhoNoReparseAncestors -Path $tempRoot -AllowedRoot $tempBase | Out-Null

try {
    $fixtureTools = Join-Path $tempRoot 'tools'
    $downloads = Join-Path $tempRoot '.tools\downloads'
    New-Item -ItemType Directory -Force -Path $fixtureTools, $downloads | Out-Null
    Copy-Item -LiteralPath (Join-Path $root 'tools\bootstrap.ps1') -Destination $fixtureTools
    Copy-Item -LiteralPath (Join-Path $root 'tools\toolchain.lock.json') -Destination $fixtureTools
    Copy-Item -LiteralPath (Join-Path $root 'tools\SafePath.psm1') -Destination $fixtureTools

    $corruptedArchive = Join-Path $downloads 'cmake.zip'
    Set-Content -LiteralPath $corruptedArchive -Value 'corrupted archive' -Encoding ASCII

    $fixtureBootstrap = Join-Path $fixtureTools 'bootstrap.ps1'
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $fixtureBootstrap -InstallPortable -Json 2>&1
    $exitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorActionPreference

    if ($exitCode -eq 0) {
        throw 'Bootstrap accepted a corrupted cached archive'
    }
    if (($output -join "`n") -notmatch 'cmake checksum mismatch') {
        throw 'Bootstrap did not report the corrupted CMake archive'
    }
    if (Test-Path -LiteralPath $corruptedArchive) {
        throw 'Bootstrap left the corrupted cached archive on disk'
    }

    Write-Output 'Bootstrap corrupted cache cleanup: PASS'
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Assert-NinhoNoReparseAncestors -Path $tempRoot -AllowedRoot $tempBase | Out-Null
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}
