[CmdletBinding()]
param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $Root 'tools\SafePath.psm1') -Force

function Assert-True {
    param([bool]$Condition, [string]$Message)

    if (-not $Condition) {
        throw $Message
    }
}

$invokeNativePath = Join-Path $Root 'tools\Invoke-Native.ps1'
$invokeNative = [IO.File]::ReadAllText($invokeNativePath)
$build = [IO.File]::ReadAllText((Join-Path $Root 'tools\build.ps1'))
$cmake = [IO.File]::ReadAllText((Join-Path $Root 'CMakeLists.txt'))

foreach ($pathContract in '$check.cmake.path', '$check.ninja.path', '$check.godot.path') {
    Assert-True ($invokeNative.Contains($pathContract)) `
        "Invoke-Native must consume bootstrap lock output: $pathContract"
}
foreach ($duplicate in @(
        '.tools\cmake\cmake-4.3.3-windows-x86_64\bin\cmake.exe',
        '.tools\ninja\ninja.exe'
    )) {
    Assert-True (-not $invokeNative.Contains($duplicate)) `
        "Invoke-Native duplicates a lock-owned path: $duplicate"
}
Assert-True ($build.Contains('-Operation $buildOperation -ArgumentList')) `
    'build.ps1 must use the structured Invoke-Native operation contract'
Assert-True ($build.Contains('"-DNINHO_GODOT_EXECUTABLE=$env:NINHO_GODOT_EXECUTABLE"')) `
    'build.ps1 must pass the lock-derived Godot path as one CMake argument'
Assert-True (-not $build.Contains('-Command $command')) `
    'build.ps1 must not serialize the build operation into nested PowerShell source'
Assert-True (-not $cmake.Contains('Godot_v4.5.1-stable_win64.exe')) `
    'CMake must not duplicate the Godot executable name owned by the lock'

$tempBase = [IO.Path]::GetTempPath().TrimEnd('\', '/')
$tempRoot = Join-Path $tempBase ('ninho-lock-path-contract-' + [Guid]::NewGuid().ToString('N'))
Assert-NinhoNoReparseAncestors -Path $tempRoot -AllowedRoot $tempBase | Out-Null

try {
    $fixtureTools = Join-Path $tempRoot 'tools'
    $visualStudio = Join-Path $tempRoot 'visual studio'
    $devCmdDirectory = Join-Path $visualStudio 'Common7\Tools'
    $cmakeExecutable = Join-Path $tempRoot 'from lock\cmake bin\cmake-from-lock.exe'
    $ninjaExecutable = Join-Path $tempRoot 'from lock\ninja bin\ninja-from-lock.exe'
    $godotExecutable = Join-Path $tempRoot 'from lock\godot bin\Godot From Lock.exe'
    foreach ($directory in @(
            $fixtureTools,
            $devCmdDirectory,
            (Split-Path $cmakeExecutable),
            (Split-Path $ninjaExecutable),
            (Split-Path $godotExecutable)
        )) {
        New-Item -ItemType Directory -Force -Path $directory | Out-Null
    }
    foreach ($executable in @($cmakeExecutable, $ninjaExecutable, $godotExecutable)) {
        New-Item -ItemType File -Force -Path $executable | Out-Null
    }

    Copy-Item -LiteralPath $invokeNativePath -Destination $fixtureTools
    Set-Content -LiteralPath (Join-Path $devCmdDirectory 'VsDevCmd.bat') `
        -Value '@exit /b 0' -Encoding ASCII

    $fixtureResult = [ordered]@{
        ok = $true
        errors = @()
        cmake = @{ path = $cmakeExecutable }
        ninja = @{ path = $ninjaExecutable }
        godot = @{ path = $godotExecutable }
        visual_studio = @{ path = $visualStudio }
    } | ConvertTo-Json -Depth 4
    $fakeBootstrap = @"
[CmdletBinding()]
param([switch]`$CheckOnly, [switch]`$Json)
@'
$fixtureResult
'@
"@
    Set-Content -LiteralPath (Join-Path $fixtureTools 'bootstrap.ps1') `
        -Value $fakeBootstrap -Encoding UTF8

    $probePath = Join-Path $tempRoot 'probe.ps1'
    $resultPath = Join-Path $tempRoot 'environment.txt'
    Set-Content -LiteralPath $probePath -Encoding UTF8 -Value @'
param([Parameter(Mandatory)][string]$OutputPath)
[IO.File]::WriteAllLines($OutputPath, @($env:PATH, $env:NINHO_GODOT_EXECUTABLE))
'@
    $escapedProbe = $probePath.Replace("'", "''")
    $escapedResult = $resultPath.Replace("'", "''")
    $command = "& '$escapedProbe' -OutputPath '$escapedResult'"
    & powershell -NoProfile -ExecutionPolicy Bypass -File `
        (Join-Path $fixtureTools 'Invoke-Native.ps1') -Command $command
    Assert-True ($LASTEXITCODE -eq 0) 'Invoke-Native fixture failed'

    $environment = [IO.File]::ReadAllLines($resultPath)
    $pathEntries = $environment[0].Split(';')
    Assert-True ($pathEntries[0] -ceq (Split-Path $cmakeExecutable)) `
        'Lock-derived CMake directory must lead PATH'
    Assert-True ($pathEntries[1] -ceq (Split-Path $ninjaExecutable)) `
        'Lock-derived Ninja directory must be second in PATH'
    Assert-True ($environment[1] -ceq $godotExecutable) `
        'Lock-derived Godot executable must be exposed to the CMake command'

    $argumentResultPath = Join-Path $tempRoot 'cmake-arguments.txt'
    $argumentProbe = {
        param([Parameter(Mandatory)][string]$OutputPath)

        $configureArguments = @(
            '--preset',
            'debug',
            '-DNINHO_BUILD_GDEXTENSION=ON',
            "-DNINHO_GODOT_EXECUTABLE=$env:NINHO_GODOT_EXECUTABLE"
        )
        [IO.File]::WriteAllLines($OutputPath, $configureArguments)
    }
    & (Join-Path $fixtureTools 'Invoke-Native.ps1') `
        -Operation $argumentProbe -ArgumentList @($argumentResultPath)

    $configureArguments = [IO.File]::ReadAllLines($argumentResultPath)
    Assert-True ($configureArguments.Count -eq 4) `
        'Structured operation must preserve the CMake argument count'
    Assert-True ($configureArguments[3] -ceq "-DNINHO_GODOT_EXECUTABLE=$godotExecutable") `
        'Structured operation must expand the absolute Godot path as one CMake argument'
    Assert-True (-not $configureArguments[3].Contains('$env:NINHO_GODOT_EXECUTABLE')) `
        'Structured operation must not pass the environment expression literally'
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Assert-NinhoNoReparseAncestors -Path $tempRoot -AllowedRoot $tempBase | Out-Null
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}

Write-Output 'toolchain-lock-path-contract-tests: PASS'
