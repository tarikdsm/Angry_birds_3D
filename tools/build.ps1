[CmdletBinding()]
param(
    [ValidateSet('Debug','Release')][string]$Configuration='Debug',
    [switch]$WithGodot,
    [switch]$ProductionPackage
)
if ($ProductionPackage -and ($Configuration -cne 'Release' -or -not $WithGodot)) {
    throw 'ProductionPackage requires -Configuration Release -WithGodot'
}
$preset = $Configuration.ToLowerInvariant()
$buildOperation = {
    param(
        [Parameter(Mandatory)][string]$Preset,
        [Parameter(Mandatory)][bool]$EnableGodot,
        [Parameter(Mandatory)][bool]$BuildProductionPackage
    )

    $configureArguments = @('--preset', $Preset)
    if ($EnableGodot) {
        $configureArguments += @(
            '-DNINHO_BUILD_GDEXTENSION=ON',
            "-DNINHO_GODOT_EXECUTABLE=$env:NINHO_GODOT_EXECUTABLE"
        )
    } else {
        $configureArguments += '-DNINHO_BUILD_GDEXTENSION=OFF'
    }
    if ($BuildProductionPackage) {
        $configureArguments += @(
            '-DBUILD_TESTING=OFF',
            '-DNINHO_ENABLE_TEST_FACADES=OFF'
        )
    } else {
        $configureArguments += @(
            '-DBUILD_TESTING=ON',
            '-DNINHO_ENABLE_TEST_FACADES=ON'
        )
    }

    & cmake @configureArguments
    if ($LASTEXITCODE -ne 0) { return }
    & cmake --build --preset $Preset
}
& (Join-Path $PSScriptRoot 'Invoke-Native.ps1') -Operation $buildOperation -ArgumentList @(
    $preset,
    $WithGodot.IsPresent,
    $ProductionPackage.IsPresent
)
exit $LASTEXITCODE
