[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$Root
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $Root 'tools\UpstreamBox3DGate.psm1') -Force

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

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

$allowedRoot = [System.IO.Path]::GetFullPath(
    (Join-Path $Root 'build\upstream-box3d-gate-test'))
$configurationPath = Join-Path $allowedRoot 'debug'
New-Item -ItemType Directory -Force -Path $configurationPath | Out-Null
Set-Content -LiteralPath (Join-Path $configurationPath 'CMakeCache.txt') `
    -Value 'CONTAMINATED=ON' -Encoding ascii
Reset-NinhoUpstreamBuildDirectory `
    -Path $configurationPath `
    -AllowedRoot $allowedRoot
Assert-True (-not (Test-Path -LiteralPath $configurationPath)) `
    'contaminated upstream directory survived reset'

Assert-Throws {
    Reset-NinhoUpstreamBuildDirectory `
        -Path (Join-Path (Split-Path $allowedRoot -Parent) 'outside') `
        -AllowedRoot $allowedRoot
} 'outside the allowed root'

New-Item -ItemType Directory -Force -Path $configurationPath | Out-Null
$compileDatabase = Join-Path $configurationPath 'compile_commands.json'
$good = @(
    [ordered]@{
        directory = $configurationPath
        command = 'C:\tool\cl.exe /nologo /MTd /Zi /Od /fp:precise -c src\body.c'
        file = 'C:\source\box3d\src\body.c'
    },
    [ordered]@{
        directory = $configurationPath
        command = 'C:\tool\cl.exe /nologo /MTd /ZI /Od -c test\test_world.c'
        file = 'C:\source\box3d\test\test_world.c'
    }
)
$good | ConvertTo-Json | Set-Content -LiteralPath $compileDatabase -Encoding utf8
Assert-NinhoUpstreamCompileDatabase -Path $compileDatabase -Configuration Debug

$unknown = @($good | ForEach-Object { [ordered]@{ directory=$_.directory; command=$_.command; file=$_.file } })
$unknown += [ordered]@{
    directory = $configurationPath
    command = 'C:\tool\cl.exe /nologo /MTd /Od -c foreign.c'
    file = 'C:\source\foreign\foreign.c'
}
$unknown | ConvertTo-Json | Set-Content -LiteralPath $compileDatabase -Encoding utf8
Assert-Throws {
    Assert-NinhoUpstreamCompileDatabase -Path $compileDatabase -Configuration Debug
} 'unexpected source entry'

$badRuntime = @($good | ForEach-Object { [ordered]@{ directory=$_.directory; command=$_.command; file=$_.file } })
$badRuntime[1].command = $badRuntime[1].command.Replace('/MTd', '/MDd')
$badRuntime | ConvertTo-Json | Set-Content -LiteralPath $compileDatabase -Encoding utf8
Assert-Throws {
    Assert-NinhoUpstreamCompileDatabase -Path $compileDatabase -Configuration Debug
} 'forbidden dynamic CRT'

$badFloat = @($good | ForEach-Object { [ordered]@{ directory=$_.directory; command=$_.command; file=$_.file } })
$badFloat[0].command += ' /fp:fast'
$badFloat | ConvertTo-Json | Set-Content -LiteralPath $compileDatabase -Encoding utf8
Assert-Throws {
    Assert-NinhoUpstreamCompileDatabase -Path $compileDatabase -Configuration Debug
} 'forbidden floating-point flag'

$badDebugOptimization = @($good | ForEach-Object { [ordered]@{ directory=$_.directory; command=$_.command; file=$_.file } })
$badDebugOptimization[0].command += ' /O1'
$badDebugOptimization | ConvertTo-Json | Set-Content -LiteralPath $compileDatabase -Encoding utf8
Assert-Throws {
    Assert-NinhoUpstreamCompileDatabase -Path $compileDatabase -Configuration Debug
} 'conflicting optimization flags'

foreach ($conflictingCommand in @(
        ($good[0].command + ' /MT'),
        ('/MT ' + $good[0].command))) {
    $conflictingRuntime = @($good | ForEach-Object { [ordered]@{ directory=$_.directory; command=$_.command; file=$_.file } })
    $conflictingRuntime[0].command = $conflictingCommand
    $conflictingRuntime | ConvertTo-Json | Set-Content -LiteralPath $compileDatabase -Encoding utf8
    Assert-Throws {
        Assert-NinhoUpstreamCompileDatabase -Path $compileDatabase -Configuration Debug
    } 'exactly one /MTd CRT flag'
}

$release = @($good | ForEach-Object {
    [ordered]@{
        directory = $_.directory
        command = (((($_.command -replace '/MTd', '/MT') -replace '/Od', '/O2') `
            -replace '/Z[iI]', '') -replace '/RTC1', '') + ' /DNDEBUG'
        file = $_.file
    }
})
$release | ConvertTo-Json | Set-Content -LiteralPath $compileDatabase -Encoding utf8
Assert-NinhoUpstreamCompileDatabase -Path $compileDatabase -Configuration Release
$releaseBaseline = $release[0].command
foreach ($conflictingCommand in @(
        ($releaseBaseline + ' /O1'),
        ('/Ox ' + $releaseBaseline))) {
    $release[0].command = $conflictingCommand
    $release | ConvertTo-Json | Set-Content -LiteralPath $compileDatabase -Encoding utf8
    Assert-Throws {
        Assert-NinhoUpstreamCompileDatabase -Path $compileDatabase -Configuration Release
    } 'conflicting optimization flags'
}
$release[0].command = $releaseBaseline
$release[1].command += ' /RTC1'
$release | ConvertTo-Json | Set-Content -LiteralPath $compileDatabase -Encoding utf8
Assert-Throws {
    Assert-NinhoUpstreamCompileDatabase -Path $compileDatabase -Configuration Release
} 'Debug contamination'

$junctionTestRoot = Join-Path ([System.IO.Path]::GetTempPath()) "ninho-upstream-reparse-$PID"
$junctionAllowedRoot = Join-Path $junctionTestRoot 'build'
$external = Join-Path $junctionTestRoot 'external'
$junction = Join-Path $junctionAllowedRoot 'upstream-box3d'
New-Item -ItemType Directory -Force -Path $junctionAllowedRoot, $external | Out-Null
Set-Content -LiteralPath (Join-Path $external 'marker.txt') -Value 'keep' -Encoding ascii
New-Item -ItemType Directory -Force -Path (Join-Path $external 'debug') | Out-Null
Set-Content -LiteralPath (Join-Path $external 'debug\payload.txt') -Value 'keep' -Encoding ascii
New-Item -ItemType Junction -Path $junction -Target $external | Out-Null
Assert-Throws {
    Reset-NinhoUpstreamBuildDirectory `
        -Path (Join-Path $junction 'debug') `
        -AllowedRoot $junctionAllowedRoot
} 'reparse point'
Assert-True (Test-Path -LiteralPath (Join-Path $external 'marker.txt')) `
    'external junction target was modified'
[System.IO.Directory]::Delete($junction)
Remove-Item -LiteralPath $junctionTestRoot -Recurse -Force

Reset-NinhoUpstreamBuildDirectory `
    -Path $configurationPath `
    -AllowedRoot $allowedRoot
Remove-Item -LiteralPath $allowedRoot -Force
Write-Output 'upstream-box3d-gate-tests: PASS'
