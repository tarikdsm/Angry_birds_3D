[CmdletBinding()]
param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot '..\GodotSpikeGate.psm1') -Force

function Assert-Throws {
    param(
        [Parameter(Mandatory)] [scriptblock]$Action,
        [Parameter(Mandatory)] [string]$Pattern
    )

    try {
        & $Action
    } catch {
        if ($_.Exception.Message -notmatch $Pattern) {
            throw "Expected error matching '$Pattern', got: $($_.Exception.Message)"
        }
        return
    }
    throw "Expected action to throw an error matching '$Pattern'"
}

$descriptorPath = Join-Path $Root 'game\bin\ninho_physics.gdextension'
$descriptor = Read-NinhoGDExtensionDescriptor -Path $descriptorPath
if ($descriptor.DebugLibrary -ne 'res://bin/ninho_physics.windows.template_debug.x86_64.dll') {
    throw "Unexpected Debug library: $($descriptor.DebugLibrary)"
}
if ($descriptor.ReleaseLibrary -ne 'res://bin/ninho_physics.windows.template_release.x86_64.dll') {
    throw "Unexpected Release library: $($descriptor.ReleaseLibrary)"
}

Assert-Throws -Pattern 'does not exist' -Action {
    Read-NinhoGDExtensionDescriptor -Path (Join-Path $Root 'game\bin\missing.gdextension')
}

$temporaryDirectory = Join-Path $Root 'artifacts\physics\descriptor-contract-tests'
New-Item -ItemType Directory -Force -Path $temporaryDirectory | Out-Null
$corruptDescriptor = Join-Path $temporaryDirectory 'corrupt.gdextension'
$validText = [System.IO.File]::ReadAllText($descriptorPath)
[System.IO.File]::WriteAllText(
    $corruptDescriptor,
    $validText.Replace('entry_symbol = "ninho_physics_library_init"', 'entry_symbol = "wrong_entry"'))
Assert-Throws -Pattern 'entry_symbol' -Action {
    Read-NinhoGDExtensionDescriptor -Path $corruptDescriptor
}

[System.IO.File]::WriteAllText(
    $corruptDescriptor,
    $validText.Replace('windows.release.x86_64', 'windows.release.arm64'))
Assert-Throws -Pattern 'windows.release.x86_64' -Action {
    Read-NinhoGDExtensionDescriptor -Path $corruptDescriptor
}

foreach ($caseMutation in @(
        @{ Name = 'section'; From = '[configuration]'; To = '[Configuration]' },
        @{ Name = 'key'; From = 'entry_symbol'; To = 'ENTRY_SYMBOL' },
        @{ Name = 'boolean'; From = 'reloadable = true'; To = 'reloadable = TRUE' },
        @{
            Name = 'path'
            From = 'res://bin/ninho_physics.windows.template_debug.x86_64.dll'
            To = 'Res://bin/ninho_physics.windows.template_debug.x86_64.dll'
        }
    )) {
    [System.IO.File]::WriteAllText(
        $corruptDescriptor,
        $validText.Replace($caseMutation.From, $caseMutation.To))
    Assert-Throws -Pattern 'descriptor|Descriptor|Unsupported|Unexpected|Invalid' -Action {
        Read-NinhoGDExtensionDescriptor -Path $corruptDescriptor
    }
}

Assert-NinhoSpikeViewContract -Path (Join-Path $Root 'game\scripts\physics_spike_view.gd')

$manifestDirectory = Join-Path $temporaryDirectory 'release-manifest'
$manifest = New-NinhoTestGDExtensionManifest `
    -Descriptor $descriptor `
    -Configuration Release `
    -CacheDirectory $manifestDirectory
$manifestText = [System.IO.File]::ReadAllText($manifest.ExtensionPath)
if (-not $manifestText.Contains($descriptor.ReleaseLibrary)) {
    throw 'Release manifest was not derived from the validated Release library'
}
if ($manifestText.Contains($descriptor.DebugLibrary)) {
    throw 'Release manifest unexpectedly references the Debug library'
}

$debugManifestDirectory = Join-Path $temporaryDirectory 'debug-manifest'
$debugManifest = New-NinhoTestGDExtensionManifest `
    -Descriptor $descriptor `
    -Configuration Debug `
    -CacheDirectory $debugManifestDirectory
$debugExtensionList = [System.IO.File]::ReadAllText($debugManifest.ExtensionListPath)
if ($debugExtensionList -cne "res://bin/ninho_physics.gdextension`n") {
    throw 'Debug must register the distributed GDExtension descriptor directly'
}
if ($debugManifest.ExtensionPath) {
    throw 'Debug must not create a synthetic GDExtension descriptor'
}

$sceneText = [System.IO.File]::ReadAllText(
    (Join-Path $Root 'game\scenes\physics_spike.tscn'))
if (-not $sceneText.StartsWith('[gd_scene load_steps=13 format=3')) {
    throw 'physics_spike.tscn must declare load_steps=13'
}

$projectText = [System.IO.File]::ReadAllText((Join-Path $Root 'game\project.godot'))
if (-not $projectText.Contains('rendering_device/fallback_to_opengl3=true')) {
    throw 'project.godot must explicitly enable the OpenGL 3 fallback'
}
if ($projectText.Contains('rendering/rendering_device/fallback_to_opengl3=true')) {
    throw 'project.godot contains a duplicated rendering section prefix'
}

$smokeText = [System.IO.File]::ReadAllText(
    (Join-Path $Root 'game\scripts\physics_spike_smoke.gd'))
if (-not [regex]::IsMatch(
        $smokeText,
        'ProjectSettings\.get_setting\(\s*"rendering/rendering_device/fallback_to_opengl3"')) {
    throw 'Godot smoke must read back the effective OpenGL fallback project setting'
}

$runnerText = [System.IO.File]::ReadAllText((Join-Path $Root 'tools\test.ps1'))
foreach ($runnerContract in @(
        'return [int]$process.ExitCode',
        'exit $godotExitCode',
        'Remove-Item -LiteralPath $expectedMovieFullPath -Force',
        'did not create a non-empty movie'
    )) {
    if (-not $runnerText.Contains($runnerContract)) {
        throw "tools/test.ps1 is missing runner contract: $runnerContract"
    }
}

Write-Output 'godot-spike-gate-tests: PASS'
