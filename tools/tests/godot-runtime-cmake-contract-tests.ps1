[CmdletBinding()]
param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
)

$ErrorActionPreference = 'Stop'
$cmakeCandidates = [Collections.Generic.List[string]]::new()
$cmakeCandidates.Add((Join-Path $Root '.tools\cmake\cmake-4.3.3-windows-x86_64\bin\cmake.exe'))
foreach ($cache in Get-ChildItem (Join-Path $Root 'build') -Recurse -Filter CMakeCache.txt `
        -ErrorAction SilentlyContinue) {
    $match = [regex]::Match(
        [IO.File]::ReadAllText($cache.FullName),
        '(?m)^CMAKE_COMMAND:INTERNAL=(.+)$')
    if ($match.Success) {
        $cmakeCandidates.Add($match.Groups[1].Value.Trim().Replace('/', '\'))
    }
}
$cmake = $cmakeCandidates | Where-Object {
    if (-not (Test-Path -LiteralPath $_ -PathType Leaf)) { return $false }
    $installationRoot = Split-Path (Split-Path $_)
    $shareRoot = Join-Path $installationRoot 'share'
    return $null -ne (Get-ChildItem $shareRoot -Recurse -Directory -Filter Modules `
        -ErrorAction SilentlyContinue | Select-Object -First 1)
} | Select-Object -First 1
if (-not $cmake) {
    throw 'A functional CMake installation is required for the runtime contract test'
}
$requirement = Join-Path $Root 'cmake\RequireGodotRuntimeTests.cmake'
$missingGodot = Join-Path $Root "artifacts\missing-godot-$([guid]::NewGuid().ToString('N')).exe"
$pinnedGodot = Join-Path $Root '.tools\godot\Godot_v4.5.1-stable_win64.exe'

function Invoke-GodotRuntimeRequirement {
    param(
        [Parameter(Mandatory)] [bool]$BuildExtension,
        [Parameter(Mandatory)] [bool]$BuildTesting,
        [Parameter(Mandatory)] [string]$GodotExecutable
    )

    $extensionValue = if ($BuildExtension) { 'ON' } else { 'OFF' }
    $testingValue = if ($BuildTesting) { 'ON' } else { 'OFF' }
    $arguments = @(
        "-DNINHO_BUILD_GDEXTENSION=$extensionValue",
        "-DBUILD_TESTING=$testingValue",
        "-DNINHO_GODOT_EXECUTABLE=$GodotExecutable",
        '-P',
        $requirement
    ) | ForEach-Object { '"' + $_.Replace('"', '\"') + '"' }
    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $cmake
    $startInfo.Arguments = $arguments -join ' '
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $process = [Diagnostics.Process]::Start($startInfo)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    [pscustomobject]@{
        ExitCode = $process.ExitCode
        Output = "$stdout`n$stderr"
    }
}

$missingRuntime = Invoke-GodotRuntimeRequirement `
    -BuildExtension $true -BuildTesting $true -GodotExecutable $missingGodot
if ($missingRuntime.ExitCode -eq 0 -or
        $missingRuntime.Output -notmatch 'requires the pinned\s+Godot\s+runtime') {
    throw "GDExtension tests must fail loudly without Godot: $($missingRuntime.Output)"
}

foreach ($bypass in @(
        @{ Extension = $false; Testing = $true; Name = 'kernel-only' },
        @{ Extension = $true; Testing = $false; Name = 'production extension' }
    )) {
    $result = Invoke-GodotRuntimeRequirement `
        -BuildExtension $bypass.Extension `
        -BuildTesting $bypass.Testing `
        -GodotExecutable $missingGodot
    if ($result.ExitCode -ne 0) {
        throw "$($bypass.Name) configuration must not require Godot: $($result.Output)"
    }
}

$availableRuntime = Invoke-GodotRuntimeRequirement `
    -BuildExtension $true -BuildTesting $true -GodotExecutable $pinnedGodot
if ($availableRuntime.ExitCode -ne 0) {
    throw "GDExtension test configuration rejected the pinned Godot runtime: $($availableRuntime.Output)"
}

$extensionCMake = [IO.File]::ReadAllText((Join-Path $Root 'native\extension\CMakeLists.txt'))
if ($extensionCMake.Contains('if(EXISTS "${_ninho_godot_executable}")')) {
    throw 'GDExtension runtime tests must not be silently skipped when Godot is absent'
}
foreach ($contract in @(
        'COMMAND "${NINHO_GODOT_EXECUTABLE}"',
        'NAME gdextension_project_import',
        'NAME gdextension_vertical_slice'
    )) {
    if (-not $extensionCMake.Contains($contract)) {
        throw "GDExtension CMake is missing runtime test contract: $contract"
    }
}

$releaseReproducibleLink = [regex]::Match(
    $extensionCMake,
    '(?s)target_link_options\(ninho_physics_extension\s+PRIVATE\s+(.+?)\)')
if (-not $releaseReproducibleLink.Success) {
    throw 'Release GDExtension must define target-local linker options'
}
$releaseLinkOptions = $releaseReproducibleLink.Groups[1].Value
foreach ($contract in @(
        '$<CXX_COMPILER_ID:MSVC>',
        '$<CONFIG:Release>',
        '/Brepro'
    )) {
    if (-not $releaseLinkOptions.Contains($contract)) {
        throw "Release GDExtension linker options are missing contract: $contract"
    }
}
if ([regex]::Matches($extensionCMake, '/Brepro').Count -ne 1) {
    throw '/Brepro must be scoped exactly once to the Release GDExtension target'
}
if ($extensionCMake -match 'CMAKE_(EXE|SHARED|MODULE)_LINKER_FLAGS') {
    throw 'GDExtension reproducibility must not mutate global linker flags'
}

Write-Output 'godot-runtime-cmake-contract-tests: PASS'
