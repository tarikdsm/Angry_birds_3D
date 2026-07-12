[CmdletBinding()]
param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot '..\SafePath.psm1') -Force
Import-Module (Join-Path $PSScriptRoot '..\GodotSpikeGate.psm1') -Force
if ($null -eq (Get-Command 'Assert-NinhoNoReparseAncestors' -ErrorAction SilentlyContinue)) {
    throw 'Importing GodotSpikeGate must not remove the caller SafePath commands'
}
Import-Module (Join-Path $PSScriptRoot '..\VerticalSliceGate.psm1') -Force
Import-Module (Join-Path $PSScriptRoot '..\SafePath.psm1') -Force

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

$fakeImportFixture = Join-Path $Root 'artifacts\fake-godot-import-timeout'
New-Item -ItemType Directory -Force -Path $fakeImportFixture | Out-Null
try {
    $fakeImportStdout = Join-Path $fakeImportFixture 'stdout.log'
    $fakeImportStderr = Join-Path $fakeImportFixture 'stderr.log'
    Assert-Throws -Pattern 'timed out' -Action {
        Invoke-NinhoTimedProcess -FilePath 'powershell.exe' -ArgumentList @(
            '-NoProfile', '-Command', 'Start-Sleep -Seconds 5'
        ) -TimeoutMs 200 -StdoutPath $fakeImportStdout -StderrPath $fakeImportStderr `
            -FatalMarker 'NINHO_GODOT_FATAL name=fake-import reason=timeout'
    }
    if (-not ([IO.File]::ReadAllText($fakeImportStderr)).Contains(
            'NINHO_GODOT_FATAL name=fake-import reason=timeout')) {
        throw 'fake hanging importer did not persist its fatal timeout marker'
    }
} finally {
    if (Test-Path -LiteralPath $fakeImportFixture) {
        Remove-Item -LiteralPath $fakeImportFixture -Recurse -Force
    }
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

$artifactRoot = Join-Path $Root 'artifacts\physics'
Assert-NinhoNoReparseAncestors -Path $artifactRoot -AllowedRoot $Root | Out-Null
New-Item -ItemType Directory -Force -Path $artifactRoot | Out-Null
$temporaryDirectory = Join-Path $artifactRoot "descriptor-contract-$([guid]::NewGuid().ToString('N'))"
Assert-NinhoNoReparseAncestors -Path $temporaryDirectory -AllowedRoot $artifactRoot | Out-Null
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
$viewText = [System.IO.File]::ReadAllText(
    (Join-Path $Root 'game\scripts\physics_spike_view.gd'))
foreach ($captureArgumentContract in @(
        'const MOVIE_CAPTURE_ARGUMENT := "--ninho-capture-300"',
        'const MOVIE_CAPTURE_INITIAL_FRAME_COUNT := 1',
        'OS.get_cmdline_user_args().has(MOVIE_CAPTURE_ARGUMENT)'
    )) {
    if (-not $viewText.Contains($captureArgumentContract)) {
        throw "Spike view is missing explicit capture argument contract: $captureArgumentContract"
    }
}
if (-not [regex]::IsMatch(
        $viewText,
        '(?m)^\s*if _movie_capture and frames == MOVIE_CAPTURE_FRAME_LIMIT - MOVIE_CAPTURE_INITIAL_FRAME_COUNT:$')) {
    throw 'Spike view must account for the Movie Maker initial frame before quitting'
}
if ($viewText.Contains('OS.get_cmdline_args().has("--write-movie")')) {
    throw 'Spike view must not infer capture mode from an engine-consumed argument'
}

$moviePath = Join-Path $temporaryDirectory 'capture.avi'
Assert-NinhoNoReparseAncestors -Path $moviePath -AllowedRoot $temporaryDirectory | Out-Null
[System.IO.File]::WriteAllBytes($moviePath, [byte[]](1, 2, 3, 4))
$movieTimestamp = [DateTime]::UtcNow
(Get-Item -LiteralPath $moviePath).LastWriteTimeUtc = $movieTimestamp

$fakeFfprobe = Join-Path $temporaryDirectory 'ffprobe.cmd'
function Set-FakeFfprobeResult {
    param(
        [Parameter(Mandatory)] [string]$Json,
        [int]$ExitCode = 0
    )

    $script = "@echo off`r`necho $Json`r`nexit /b $ExitCode`r`n"
    [System.IO.File]::WriteAllText(
        $fakeFfprobe,
        $script,
        [System.Text.Encoding]::ASCII)
}

$validProbeJson = '{"streams":[{"codec_name":"mjpeg","width":1280,"height":720,"duration":"5.000000","nb_read_frames":"300"}]}'
Set-FakeFfprobeResult -Json $validProbeJson
$movieMetadata = Assert-NinhoGodotMovieCapture `
    -Path $moviePath `
    -AllowedRoot $temporaryDirectory `
    -StartedUtc ($movieTimestamp.AddSeconds(-1)) `
    -FfprobePath $fakeFfprobe
if ($movieMetadata.Codec -cne 'mjpeg' -or
        $movieMetadata.Width -ne 1280 -or
        $movieMetadata.Height -ne 720 -or
        $movieMetadata.FrameCount -ne 300 -or
        $movieMetadata.DurationSeconds -ne 5.0) {
    throw 'Movie validation did not return the expected normalized metadata'
}

Assert-Throws -Pattern 'not fresh' -Action {
    Assert-NinhoGodotMovieCapture `
        -Path $moviePath `
        -AllowedRoot $temporaryDirectory `
        -StartedUtc ([DateTime]::UtcNow.AddMinutes(1)) `
        -FfprobePath $fakeFfprobe
}

foreach ($invalidProbe in @(
        @{
            Name = 'codec'
            Json = $validProbeJson.Replace('"mjpeg"', '"h264"')
            Pattern = 'codec'
        },
        @{
            Name = 'resolution'
            Json = $validProbeJson.Replace('"width":1280', '"width":1920')
            Pattern = 'resolution'
        },
        @{
            Name = 'frames'
            Json = $validProbeJson.Replace('"300"', '"299"')
            Pattern = 'frame count'
        },
        @{
            Name = 'duration'
            Json = $validProbeJson.Replace('"5.000000"', '"4.983333"')
            Pattern = 'duration'
        }
    )) {
    Set-FakeFfprobeResult -Json $invalidProbe.Json
    Assert-Throws -Pattern $invalidProbe.Pattern -Action {
        Assert-NinhoGodotMovieCapture `
            -Path $moviePath `
            -AllowedRoot $temporaryDirectory `
            -StartedUtc ($movieTimestamp.AddSeconds(-1)) `
            -FfprobePath $fakeFfprobe
    }
}
Set-FakeFfprobeResult -Json $validProbeJson -ExitCode 7
Assert-Throws -Pattern 'ffprobe failed' -Action {
    Assert-NinhoGodotMovieCapture `
        -Path $moviePath `
        -AllowedRoot $temporaryDirectory `
        -StartedUtc ($movieTimestamp.AddSeconds(-1)) `
        -FfprobePath $fakeFfprobe
}
$global:LASTEXITCODE = 0
Set-FakeFfprobeResult -Json $validProbeJson

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
        'Assert-NinhoGodotMovieCapture',
        'NINHO_VISUAL_CAPTURE_COMPLETE frame=300',
        '-RequiredCompletionMarker $visualCompletionMarker',
        "'--fixed-fps', '60'",
        "'--headless', '--path', 'game', '--import'",
        "'--script', 'res://tests/import_completeness_smoke.gd'",
        "Get-ChildItem -LiteralPath `$assetRoot -Recurse -File -Filter '*.import'",
        "Contains('valid=false')",
        'Godot import metadata is invalid',
        'Invoke-NinhoTimedProcess',
        '-TimeoutMs 180000',
        "'--', '--ninho-capture-300'",
        "'--path', 'game', '--editor', '--quit', 'res://scenes/physics_spike.tscn'",
        "'--script', 'res://tests/vertical_slice_smoke.gd'",
        "'--script', 'res://tests/forbid_godot_physics.gd'",
        "'res://scenes/vertical_slice.tscn'",
        "'--', '--vertical-slice-capture'"
    )) {
    if (-not $runnerText.Contains($runnerContract)) {
        throw "tools/test.ps1 is missing runner contract: $runnerContract"
    }
}
$importCommandIndex = $runnerText.IndexOf("'--headless', '--path', 'game', '--import'", [StringComparison]::Ordinal)
$importSmokeIndex = $runnerText.IndexOf("'--script', 'res://tests/import_completeness_smoke.gd'", [StringComparison]::Ordinal)
$verticalSmokeIndex = $runnerText.IndexOf("'--script', 'res://tests/vertical_slice_smoke.gd'", [StringComparison]::Ordinal)
if ($importCommandIndex -lt 0 -or $importSmokeIndex -le $importCommandIndex -or
        $verticalSmokeIndex -le $importSmokeIndex) {
    throw 'full import and import-completeness smoke must precede the vertical slice smoke'
}
if ($runnerText.Contains("'--quit-after'")) {
    throw 'tools/test.ps1 must let the visual scene finish frame 300 itself'
}
if ([regex]::Matches(
        $runnerText,
        [regex]::Escape('-RequiredCompletionMarker $visualCompletionMarker')).Count -ne 2) {
    throw 'Both Vulkan and OpenGL movie gates must require the frame-300 marker'
}
if ([regex]::Matches($runnerText, [regex]::Escape("'--fixed-fps', '60'")).Count -ne 6) {
    throw 'Logical and feedback smokes plus foundation and playable captures must use fixed 60 FPS'
}
if ([regex]::Matches(
        $runnerText,
        [regex]::Escape("'--', '--ninho-capture-300'")).Count -ne 2) {
    throw 'Both movie gates must activate the explicit GDScript capture argument'
}
if ([regex]::Matches(
        $runnerText,
        [regex]::Escape("'--', '--vertical-slice-capture'")).Count -ne 2) {
    throw 'Both renderers must capture the playable vertical slice explicitly'
}

$controllerPath = Join-Path $Root 'game\scripts\game\vertical_slice_controller.gd'
if (-not (Test-Path -LiteralPath $controllerPath -PathType Leaf)) {
    throw 'vertical slice controller is missing'
}
$controllerText = [System.IO.File]::ReadAllText($controllerPath)
foreach ($inputContract in @(
        'PERFORMANCE_INPUT_SAMPLES',
        '"command_id": "set_aim_center"',
        '"command_id": "set_aim_left"',
        '"command_id": "set_aim_right"',
        '_preview_matches_input_sample',
        '_input_feedback_markers')) {
    if (-not $controllerText.Contains($inputContract)) {
        throw "vertical slice controller is missing causal input contract: $inputContract"
    }
}
if ([regex]::Matches($controllerText,
        [regex]::Escape('set_aim_degrees(0.0, 0.0, 10.5)')).Count -ne 1 -or
        [regex]::Matches($controllerText,
        [regex]::Escape('set_aim_degrees(-2.0, 0.0, 8.0)')).Count -ne 1 -or
        [regex]::Matches($controllerText,
        [regex]::Escape('set_aim_degrees(2.0, 0.0, 8.0)')).Count -ne 1) {
    throw 'performance input samples must issue one distinct center, left and right aim command'
}
if ([regex]::Matches($controllerText, 'session\.consume_frame\(\)').Count -ne 1) {
    throw 'vertical slice controller must call consume_frame exactly once in source'
}
if (-not [regex]::IsMatch(
        $controllerText,
        '(?s)func _physics_process\([^)]*\).*?session\.consume_frame\(\)')) {
    throw 'vertical slice controller must consume the frame from _physics_process'
}

$cameraText = [System.IO.File]::ReadAllText(
    (Join-Path $Root 'game\scripts\camera\orbital_camera.gd'))
foreach ($cameraContract in @(
        'const BASE_FOV := 48.0',
        'const MIN_DISTANCE := 14.0',
        'const MAX_DISTANCE := 24.0',
        'const MIN_INCLINATION := 15.0',
        'const MAX_INCLINATION := 70.0',
        'const LOOK_AHEAD_METERS := 2.0'
    )) {
    if (-not $cameraText.Contains($cameraContract)) {
        throw "orbital camera is missing contract: $cameraContract"
    }
}

$gateModuleText = [System.IO.File]::ReadAllText((Join-Path $Root 'tools\GodotSpikeGate.psm1'))
$movieGuardIndex = $gateModuleText.IndexOf(
    'Assert-NinhoNoReparseAncestors -Path $Path -AllowedRoot $AllowedRoot')
$movieReadIndex = $gateModuleText.IndexOf('Get-Item -LiteralPath $fullPath')
if ($movieGuardIndex -lt 0 -or $movieReadIndex -lt 0 -or $movieGuardIndex -gt $movieReadIndex) {
    throw 'Movie validation must apply the SafePath ancestor guard before reading the movie'
}

Assert-NinhoNoReparseAncestors -Path $temporaryDirectory -AllowedRoot $artifactRoot | Out-Null
Remove-Item -LiteralPath $temporaryDirectory -Recurse -Force
Write-Output 'godot-spike-gate-tests: PASS'
