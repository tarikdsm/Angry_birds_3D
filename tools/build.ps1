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
$godot = if ($WithGodot) { '-DNINHO_BUILD_GDEXTENSION=ON' } else { '-DNINHO_BUILD_GDEXTENSION=OFF' }
$production = if ($ProductionPackage) {
    ' -DBUILD_TESTING=OFF -DNINHO_ENABLE_TEST_FACADES=OFF'
} else {
    ' -DBUILD_TESTING=ON -DNINHO_ENABLE_TEST_FACADES=ON'
}
$command = "cmake --preset $preset $godot$production; if (`$LASTEXITCODE) { exit `$LASTEXITCODE }; cmake --build --preset $preset"
& (Join-Path $PSScriptRoot 'Invoke-Native.ps1') -Command $command
exit $LASTEXITCODE
