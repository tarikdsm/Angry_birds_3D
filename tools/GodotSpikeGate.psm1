Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1')

function Read-NinhoGDExtensionDescriptor {
    [CmdletBinding()]
    param([Parameter(Mandatory)] [string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "GDExtension descriptor does not exist: $Path"
    }

    $values = [System.Collections.Generic.Dictionary[string, object]]::new(
        [System.StringComparer]::Ordinal)
    $section = $null
    $lineNumber = 0
    foreach ($rawLine in [System.IO.File]::ReadAllLines((Resolve-Path -LiteralPath $Path).Path)) {
        $lineNumber += 1
        $line = $rawLine.Trim()
        if ($line.Length -eq 0 -or $line.StartsWith(';') -or $line.StartsWith('#')) {
            continue
        }
        if ($line -cmatch '^\[([a-z_]+)\]$') {
            $section = $Matches[1]
            if ($section -cne 'configuration' -and $section -cne 'libraries') {
                throw "Unexpected descriptor section [$section] at line $lineNumber"
            }
            continue
        }
        if (-not $section) {
            throw "Descriptor value outside a section at line $lineNumber"
        }
        if ($line -cnotmatch '^([a-z0-9_.]+)\s*=\s*(.+)$') {
            throw "Malformed descriptor line $lineNumber`: $line"
        }

        $key = "$section/$($Matches[1])"
        if ($values.ContainsKey($key)) {
            throw "Duplicate descriptor key: $key"
        }
        $rawValue = $Matches[2].Trim()
        if ($rawValue -cmatch '^"([^"]*)"$') {
            $value = $Matches[1]
        } elseif ($rawValue -ceq 'true' -or $rawValue -ceq 'false') {
            $value = $rawValue -ceq 'true'
        } else {
            throw "Unsupported descriptor value for $key"
        }
        $values[$key] = $value
    }

    $expected = [System.Collections.Generic.Dictionary[string, object]]::new(
        [System.StringComparer]::Ordinal)
    $expected.Add('configuration/entry_symbol', 'ninho_physics_library_init')
    $expected.Add('configuration/compatibility_minimum', '4.5')
    $expected.Add('configuration/reloadable', $true)
    $expected.Add(
        'libraries/windows.debug.x86_64',
        'res://bin/ninho_physics.windows.template_debug.x86_64.dll')
    $expected.Add(
        'libraries/windows.release.x86_64',
        'res://bin/ninho_physics.windows.template_release.x86_64.dll')
    foreach ($key in $expected.Keys) {
        if (-not $values.ContainsKey($key)) {
            throw "GDExtension descriptor is missing $key"
        }
        if ($values[$key] -cne $expected[$key]) {
            throw "Invalid $key`: expected '$($expected[$key])', got '$($values[$key])'"
        }
    }
    foreach ($key in $values.Keys) {
        if (-not $expected.ContainsKey($key)) {
            throw "Unexpected descriptor key: $key"
        }
    }

    [pscustomobject]@{
        EntrySymbol = $values['configuration/entry_symbol']
        CompatibilityMinimum = $values['configuration/compatibility_minimum']
        Reloadable = $values['configuration/reloadable']
        DebugLibrary = $values['libraries/windows.debug.x86_64']
        ReleaseLibrary = $values['libraries/windows.release.x86_64']
    }
}

function New-NinhoTestGDExtensionManifest {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [pscustomobject]$Descriptor,
        [Parameter(Mandatory)] [ValidateSet('Debug', 'Release')] [string]$Configuration,
        [Parameter(Mandatory)] [string]$CacheDirectory
    )

    $selectedLibrary = if ($Configuration -eq 'Debug') {
        $Descriptor.DebugLibrary
    } else {
        $Descriptor.ReleaseLibrary
    }
    if (-not $selectedLibrary.StartsWith('res://bin/', [System.StringComparison]::Ordinal)) {
        throw "Selected GDExtension library must be below res://bin/: $selectedLibrary"
    }

    New-Item -ItemType Directory -Force -Path $CacheDirectory | Out-Null
    $extensionListPath = Join-Path $CacheDirectory 'extension_list.cfg'
    $utf8WithoutBom = [System.Text.UTF8Encoding]::new($false)
    if ($Configuration -eq 'Debug') {
        [System.IO.File]::WriteAllText(
            $extensionListPath,
            "res://bin/ninho_physics.gdextension`n",
            $utf8WithoutBom)
        return [pscustomobject]@{
            SelectedLibrary = $selectedLibrary
            ExtensionPath = $null
            ExtensionListPath = $extensionListPath
        }
    }

    $extensionPath = Join-Path $CacheDirectory 'ninho_physics.test.gdextension'
    $extensionText = @"
[configuration]
entry_symbol = "$($Descriptor.EntrySymbol)"
compatibility_minimum = "$($Descriptor.CompatibilityMinimum)"
reloadable = false

[libraries]
windows.debug.x86_64 = "$selectedLibrary"
"@
    [System.IO.File]::WriteAllText($extensionPath, $extensionText, $utf8WithoutBom)
    [System.IO.File]::WriteAllText(
        $extensionListPath,
        "res://.godot/ninho_physics.test.gdextension`n",
        $utf8WithoutBom)

    [pscustomobject]@{
        SelectedLibrary = $selectedLibrary
        ExtensionPath = $extensionPath
        ExtensionListPath = $extensionListPath
    }
}

function Assert-NinhoSpikeViewContract {
    [CmdletBinding()]
    param([Parameter(Mandatory)] [string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Spike view script does not exist: $Path"
    }
    $source = [System.IO.File]::ReadAllText((Resolve-Path -LiteralPath $Path).Path)
    $requiredPatterns = [ordered]@{
        'expected body count' = '(?m)^const EXPECTED_BODY_COUNT := 122$'
        'expected visual count' = '(?m)^const EXPECTED_VISUAL_BODY_COUNT := 121$'
        'snapshot timeout' = '(?m)^const FIRST_SNAPSHOT_FRAME_LIMIT := 10$'
        'movie frame limit' = '(?m)^const MOVIE_CAPTURE_FRAME_LIMIT := 300$'
        'movie capture argument' = '(?m)^const MOVIE_CAPTURE_ARGUMENT := "--ninho-capture-300"$'
        'movie initial frame count' = '(?m)^const MOVIE_CAPTURE_INITIAL_FRAME_COUNT := 1$'
        'movie completion marker' = '(?m)^const VISUAL_CAPTURE_COMPLETE_MARKER := "NINHO_VISUAL_CAPTURE_COMPLETE frame=300"$'
        'invalid handle guard' = '(?m)^\s*if handle == 0:$'
        'body snapshot assertion' = '(?m)^\s*if states\.size\(\) != EXPECTED_BODY_COUNT:$'
        'visual handle assertion' = '(?m)^\s*if alive_visual_count != EXPECTED_VISUAL_BODY_COUNT:$'
        'movie completion condition' = '(?m)^\s*if _movie_capture and frames == MOVIE_CAPTURE_FRAME_LIMIT - MOVIE_CAPTURE_INITIAL_FRAME_COUNT:$'
        'movie completion print' = '(?m)^\s*print\(VISUAL_CAPTURE_COMPLETE_MARKER\)$'
        'explicit movie mode' = '(?m)^\s*_movie_capture = OS\.get_cmdline_user_args\(\)\.has\(MOVIE_CAPTURE_ARGUMENT\)$'
    }
    foreach ($name in $requiredPatterns.Keys) {
        if (-not [regex]::IsMatch($source, $requiredPatterns[$name])) {
            throw "Spike view contract is missing ${name}: $($requiredPatterns[$name])"
        }
    }
    if ([regex]::Matches(
            $source,
            '(?m)^\s*if not _register_visual_handle\(').Count -ne 2) {
        throw 'Spike view must fail-fast on all 120 boxes and the projectile spawn paths'
    }
    if ([regex]::Matches($source, 'physics\.get_body_states\(\)').Count -ne 1) {
        throw 'Spike view must fetch exactly one batched Box3D snapshot per physics frame'
    }
    if ($source.Contains('OS.get_cmdline_args().has("--write-movie")')) {
        throw 'Spike view must not infer capture mode from an engine-consumed argument'
    }
    if ([regex]::Matches(
            $source,
            '(?m)^\s*print\(VISUAL_CAPTURE_COMPLETE_MARKER\)$').Count -ne 1) {
        throw 'Spike view must emit the frame-300 completion marker exactly once'
    }
    $completionSequence = @(
        'frames \+= 1',
        'if _movie_capture and frames == MOVIE_CAPTURE_FRAME_LIMIT - MOVIE_CAPTURE_INITIAL_FRAME_COUNT:',
        'print\(VISUAL_CAPTURE_COMPLETE_MARKER\)',
        'get_tree\(\)\.quit\(0\)'
    ) -join '\s+'
    if (-not [regex]::IsMatch($source, $completionSequence)) {
        throw 'Spike view must emit its marker immediately after frame 300 and then quit cleanly'
    }
}

function Assert-NinhoGodotMovieCapture {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$Path,
        [Parameter(Mandatory)] [string]$AllowedRoot,
        [Parameter(Mandatory)] [DateTime]$StartedUtc,
        [Parameter(Mandatory)] [string]$FfprobePath
    )

    $fullPath = Assert-NinhoNoReparseAncestors -Path $Path -AllowedRoot $AllowedRoot
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        throw "Godot movie does not exist: $fullPath"
    }
    $movie = Get-Item -LiteralPath $fullPath
    if ($movie.Length -le 0) {
        throw "Godot movie is empty: $fullPath"
    }
    if ($movie.LastWriteTimeUtc -lt $StartedUtc.ToUniversalTime()) {
        throw "Godot movie is not fresh: $fullPath"
    }
    if (-not (Test-Path -LiteralPath $FfprobePath -PathType Leaf)) {
        throw "ffprobe executable does not exist: $FfprobePath"
    }

    $probeOutput = & $FfprobePath `
        -v error `
        -count_frames `
        -select_streams 'v:0' `
        -show_entries 'stream=codec_name,width,height,nb_read_frames,duration' `
        -of json `
        $fullPath 2>&1
    $probeExitCode = $LASTEXITCODE
    if ($probeExitCode -ne 0) {
        throw "ffprobe failed with exit code ${probeExitCode}: $($probeOutput -join ' ')"
    }

    try {
        $probe = ($probeOutput -join "`n") | ConvertFrom-Json
    } catch {
        throw "ffprobe returned invalid JSON for ${fullPath}: $($_.Exception.Message)"
    }
    $streams = @($probe.streams)
    if ($streams.Count -ne 1) {
        throw "ffprobe expected one video stream, got $($streams.Count)"
    }
    $stream = $streams[0]
    if ("$($stream.codec_name)" -cne 'mjpeg') {
        throw "Godot movie codec mismatch: expected mjpeg, got '$($stream.codec_name)'"
    }
    $width = [int]$stream.width
    $height = [int]$stream.height
    if ($width -ne 1280 -or $height -ne 720) {
        throw "Godot movie resolution mismatch: expected 1280x720, got ${width}x${height}"
    }
    $frameCount = [int]$stream.nb_read_frames
    if ($frameCount -ne 300) {
        throw "Godot movie frame count mismatch: expected 300, got $frameCount"
    }
    try {
        $durationSeconds = [double]::Parse(
            "$($stream.duration)",
            [System.Globalization.CultureInfo]::InvariantCulture)
    } catch {
        throw "Godot movie duration is invalid: '$($stream.duration)'"
    }
    if ([Math]::Abs($durationSeconds - 5.0) -gt 0.000001) {
        throw "Godot movie duration mismatch: expected 5 seconds, got $durationSeconds"
    }

    [pscustomobject]@{
        Codec = 'mjpeg'
        Width = $width
        Height = $height
        FrameCount = $frameCount
        DurationSeconds = $durationSeconds
    }
}

Export-ModuleMember -Function @(
    'Read-NinhoGDExtensionDescriptor',
    'New-NinhoTestGDExtensionManifest',
    'Assert-NinhoSpikeViewContract',
    'Assert-NinhoGodotMovieCapture'
)
