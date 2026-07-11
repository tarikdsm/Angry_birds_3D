[CmdletBinding()]
param(
    [ValidateSet('Debug','Release')][string]$Configuration='Debug',
    [switch]$WithGodot
)
$preset = $Configuration.ToLowerInvariant()
$godot = if ($WithGodot) { '-DNINHO_BUILD_GDEXTENSION=ON' } else { '-DNINHO_BUILD_GDEXTENSION=OFF' }
$command = "cmake --preset $preset $godot; if (`$LASTEXITCODE) { exit `$LASTEXITCODE }; cmake --build --preset $preset"
& (Join-Path $PSScriptRoot 'Invoke-Native.ps1') -Command $command
exit $LASTEXITCODE
