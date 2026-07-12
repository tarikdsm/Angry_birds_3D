[CmdletBinding()]
param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [string]$OutputDirectory,
    [string]$GoldenDirectory
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$preset = $Configuration.ToLowerInvariant()
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $root "artifacts\vertical-slice\$preset" }
if (-not $GoldenDirectory) { $GoldenDirectory = Join-Path $root "docs\art\goldens\vertical-slice\$preset" }
Import-Module (Join-Path $PSScriptRoot 'VerticalSliceGate.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force
foreach ($path in $OutputDirectory,$GoldenDirectory) {
    Assert-NinhoNoReparseAncestors -Path $path -AllowedRoot $root | Out-Null
    New-Item -ItemType Directory -Force -Path $path | Out-Null
}
$godot = Join-Path $root '.tools\godot\Godot_v4.5.1-stable_win64.exe'
if (-not (Test-Path -LiteralPath $godot -PathType Leaf)) { throw "pinned Godot missing: $godot" }
$ffprobe = (Get-Command ffprobe.exe -ErrorAction Stop).Source
$ffmpeg = (Get-Command ffmpeg.exe -ErrorAction Stop).Source
$artifacts = [Collections.Generic.List[object]]::new()

function Get-RelativePath([string]$Base, [string]$Target) {
    $basePath = [IO.Path]::GetFullPath($Base).TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
    $targetPath = [IO.Path]::GetFullPath($Target)
    $baseUri = [Uri]$basePath
    $targetUri = [Uri]$targetPath
    return [Uri]::UnescapeDataString($baseUri.MakeRelativeUri($targetUri).ToString())
}

function Add-Artifact([string]$Path) {
    Assert-NinhoNoReparseAncestors -Path $Path -AllowedRoot $root | Out-Null
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "capture artifact missing: $Path" }
    $relative = (Get-RelativePath $root $Path).Replace('\','/')
    $script:artifacts.Add([ordered]@{
        path = $relative
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant()
    })
}

function Assert-CleanLog([string]$Stdout, [string]$Stderr, [string]$Marker = '') {
    $text = [IO.File]::ReadAllText($Stdout) + "`n" + [IO.File]::ReadAllText($Stderr)
    foreach ($forbidden in 'ERROR:','SCRIPT ERROR:','WARNING:') {
        if ($text.Contains($forbidden)) { throw "Godot log contains forbidden diagnostic: $forbidden" }
    }
    if ($Marker -and ([regex]::Matches($text, [regex]::Escape($Marker))).Count -ne 1) {
        throw "Godot log missing unique marker: $Marker"
    }
    return $text
}

function Invoke-CaptureRun {
    param(
        [string]$Name,
        [string]$Renderer,
        [int]$Scale,
        [string]$Movie = '',
        [string]$Metrics = ''
    )
    $isVulkan = $Renderer -ceq 'Vulkan'
    $maximum = if ($isVulkan) { 2 } else { 1 }
    $result = Invoke-NinhoLimitedRetry -Name $Name -MaximumAttempts $maximum -Operation {
        param($attempt)
        $stdout = Join-Path $OutputDirectory "$Name-attempt$attempt.stdout.log"
        $stderr = Join-Path $OutputDirectory "$Name-attempt$attempt.stderr.log"
        $arguments = [Collections.Generic.List[string]]::new()
        if ($Renderer -ceq 'OpenGL') { $arguments.AddRange([string[]]@('--rendering-method','gl_compatibility')) }
        $arguments.AddRange([string[]]@('--path','game','--resolution','1920x1080'))
        if ($Movie) {
            $movieRelative = (Get-RelativePath (Join-Path $root 'game') $Movie).Replace('\','/')
            $arguments.AddRange([string[]]@('--write-movie',$movieRelative,'--fixed-fps','60'))
        }
        $arguments.Add('res://scenes/vertical_slice.tscn')
        $arguments.Add('--')
        $arguments.Add('--vertical-slice-capture')
        $arguments.Add("--ui-scale=$Scale")
        if ($Movie) { $arguments.Add('--capture-normal-terminal') }
        if ($Metrics) {
            $metricsRelative = (Get-RelativePath (Join-Path $root 'game') $Metrics).Replace('\','/')
            $arguments.Add("--vertical-slice-metrics=res://$metricsRelative")
        }
        $timeoutMs = if ($Movie) { 600000 } else { 120000 }
        $process = Invoke-NinhoTimedProcess -FilePath $godot -ArgumentList @($arguments) `
            -TimeoutMs $timeoutMs -StdoutPath $stdout -StderrPath $stderr `
            -WorkingDirectory $root -FatalMarker "NINHO_CAPTURE_FATAL name=$Name reason=timeout"
        $exitCode = [int]$process.ExitCode
        if ($exitCode -ne 0 -and $isVulkan -and $attempt -lt $maximum -and
                $exitCode -notin -1073741819,-1073740791) {
            throw "$Name non-transient failure is not retryable: $exitCode"
        }
        return [pscustomobject]@{ ExitCode=$exitCode; Stdout=$stdout; Stderr=$stderr }
    }
    $requiredMarker = if ($Movie) {
        'VERTICAL_SLICE_SOURCE_CAPTURE_COMPLETE'
    } else {
        'VERTICAL_SLICE_CAPTURE_COMPLETE frame=300'
    }
    $null = Assert-CleanLog $result.Stdout $result.Stderr $requiredMarker
    Add-Artifact $result.Stdout
    Add-Artifact $result.Stderr
    return $result
}

$rendererEvidence = [Collections.Generic.List[object]]::new()
$vulkanCaptureLog = ''
$vulkanRawMovie = ''
$vulkanSelectedFrames = @()
foreach ($renderer in 'Vulkan','OpenGL') {
    $slug = if ($renderer -ceq 'Vulkan') { 'vulkan' } else { 'opengl' }
    $rawMovie = Join-Path $OutputDirectory "vertical-slice-$slug-source.avi"
    $movie = Join-Path $OutputDirectory "vertical-slice-$slug.avi"
    foreach ($target in $rawMovie,$movie) {
        if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Force }
    }
    $capture = Invoke-CaptureRun -Name "capture-$slug" -Renderer $renderer -Scale 100 -Movie $rawMovie
    if ($renderer -ceq 'Vulkan') { $vulkanCaptureLog = $capture.Stdout }
    $rawProbe = & $ffprobe -v error -select_streams v:0 `
        -show_entries stream=width,height,nb_frames -of json $rawMovie | ConvertFrom-Json
    if ($LASTEXITCODE -ne 0 -or @($rawProbe.streams).Count -ne 1) { throw "ffprobe failed: $rawMovie" }
    $rawStream = $rawProbe.streams[0]
    $rawFrameCount = [int]$rawStream.nb_frames
    if ([int]$rawStream.width -ne 1920 -or [int]$rawStream.height -ne 1080 -or
            $rawFrameCount -lt 300 -or $rawFrameCount -gt 6000) {
        throw "source movie contract mismatch: $renderer $($rawStream.width)x$($rawStream.height) $rawFrameCount frames"
    }
    $selectedFrames = [Collections.Generic.List[int]]::new()
    for ($index = 0; $index -lt 300; ++$index) {
        $selectedFrames.Add([int][Math]::Round(
            [double]$index * [double]($rawFrameCount - 1) / 299.0,
            [MidpointRounding]::AwayFromZero))
    }
    if (@($selectedFrames | Select-Object -Unique).Count -ne 300 -or
            $selectedFrames[0] -ne 0 -or $selectedFrames[299] -ne $rawFrameCount - 1) {
        throw "deterministic downsample mapping is invalid: $renderer"
    }
    if ($renderer -ceq 'Vulkan') {
        $impactPattern = '(?m)^NINHO_CAPTURE_EVENT frame=(\d+) tick=(\d+) kind=\S+ profile=vulnerable affected=200 damage=([0-9.]+)'
        $impactMatches = @([regex]::Matches([IO.File]::ReadAllText($capture.Stdout), $impactPattern) |
            Where-Object { [double]$_.Groups[3].Value -gt 0 })
        if ($impactMatches.Count -eq 0) { throw 'capture has no causal vulnerable Anchor impact frame' }
        $requiredImpactFrame = [int]$impactMatches[0].Groups[1].Value + 1
        $selectedFrames = Set-NinhoRequiredDownsampleFrame `
            -Mapping ([int[]]@($selectedFrames)) -RequiredSourceFrame $requiredImpactFrame
        $flightAbilityState = [regex]::Match(
            [IO.File]::ReadAllText($capture.Stdout),
            '(?m)^NINHO_CAPTURE_STATE frame=(\d+) tick=\d+ phase=flight_ability outcome=none ')
        if (-not $flightAbilityState.Success) { throw 'capture has no flight ability transition frame' }
        $requiredFlightAbilityFrame = [int]$flightAbilityState.Groups[1].Value + 5
        $selectedFrames = Set-NinhoRequiredDownsampleFrame `
            -Mapping ([int[]]@($selectedFrames)) -RequiredSourceFrame $requiredFlightAbilityFrame
        $vulkanRawMovie = $rawMovie
        $vulkanSelectedFrames = @($selectedFrames)
    }
    $selectTerms = @($selectedFrames | ForEach-Object { "eq(n\,$_)" }) -join '+'
    $videoFilter = "select='$selectTerms',setpts=N/(60*TB)"
    $downsampleStdout = Join-Path $OutputDirectory "downsample-$slug.stdout.log"
    $downsampleStderr = Join-Path $OutputDirectory "downsample-$slug.stderr.log"
    $downsample = Invoke-NinhoTimedProcess -FilePath $ffmpeg -ArgumentList @(
        '-v','error','-y','-i',$rawMovie,'-an','-vf',$videoFilter,
        '-c:v','mjpeg','-q:v','2',$movie
    ) -TimeoutMs 300000 -StdoutPath $downsampleStdout -StderrPath $downsampleStderr `
        -FatalMarker "NINHO_CAPTURE_FATAL name=downsample-$slug reason=timeout"
    if ($downsample.ExitCode -ne 0) { throw "failed to downsample source capture: $renderer" }
    $null = Assert-CleanLog $downsampleStdout $downsampleStderr
    Add-Artifact $downsampleStdout
    Add-Artifact $downsampleStderr
    $probe = & $ffprobe -v error -select_streams v:0 `
        -show_entries stream=width,height,nb_frames -of json $movie | ConvertFrom-Json
    if ($LASTEXITCODE -ne 0 -or @($probe.streams).Count -ne 1) { throw "ffprobe failed: $movie" }
    $stream = $probe.streams[0]
    if ([int]$stream.width -ne 1920 -or [int]$stream.height -ne 1080 -or [int]$stream.nb_frames -ne 300) {
        throw "downsampled movie contract mismatch: $renderer $($stream.width)x$($stream.height) $($stream.nb_frames) frames"
    }
    Add-Artifact $rawMovie
    Add-Artifact $movie
    $scales = [Collections.Generic.List[object]]::new()
    foreach ($scale in 100,150) {
        $metrics = Join-Path $OutputDirectory "metrics-$slug-$scale.json"
        if (Test-Path -LiteralPath $metrics) { Remove-Item -LiteralPath $metrics -Force }
        $null = Invoke-CaptureRun -Name "performance-$slug-$scale" -Renderer $renderer -Scale $scale -Metrics $metrics
        if (-not (Test-Path -LiteralPath $metrics -PathType Leaf)) { throw "runtime metrics missing: $metrics" }
        $document = Get-Content -Raw -LiteralPath $metrics | ConvertFrom-Json
        if ($document.schema -cne 'ninho.vertical-slice.runtime-metrics.v1' -or $document.frames_measured -lt 200) {
            throw "runtime metrics invalid: $metrics"
        }
        $scales.Add([ordered]@{
            ui_scale = $scale
            frame_p95_ms = $document.frame_p95_ms
            frame_p99_ms = $document.frame_p99_ms
            max_hitch_ms = $document.max_hitch_ms
            input_feedback_p95_ms = $document.input_feedback_p95_ms
            input_feedback_samples = [int]$document.input_feedback_samples
            input_feedback_markers = @($document.input_feedback_markers)
            physics_step_p95_ms = $document.physics_step_p95_ms
            frames_measured = [int]$document.frames_measured
            rupture_observed = [bool]$document.rupture_observed
            vfx_observed = [bool]$document.vfx_observed
            post_vfx_frames = [int]$document.post_vfx_frames
            metrics_path = (Get-RelativePath $root $metrics).Replace('\','/')
            metrics_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $metrics).Hash.ToLowerInvariant()
        })
        Add-Artifact $metrics
    }
    $rendererEvidence.Add([ordered]@{
        name = $renderer
        scales = @($scales)
        capture = [ordered]@{
            frames = 300; width = 1920; height = 1080
            path = (Get-RelativePath $root $movie).Replace('\','/')
            hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $movie).Hash.ToLowerInvariant()
            log_path = (Get-RelativePath $root $capture.Stdout).Replace('\','/')
            log_hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $capture.Stdout).Hash.ToLowerInvariant()
            retry_count = $capture.AttemptCount - 1
            source_path = (Get-RelativePath $root $rawMovie).Replace('\','/')
            source_hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $rawMovie).Hash.ToLowerInvariant()
            source_frames = $rawFrameCount
            downsample_source_frames = @($selectedFrames)
        }
    })
}

$simulationTests = Join-Path $root "build\$preset\native\simulation\ninho_simulation_tests.exe"
if (-not (Test-Path -LiteralPath $simulationTests -PathType Leaf)) { throw "simulation tests missing: $simulationTests" }
$routeLog = Join-Path $OutputDirectory 'route-determinism.log'
$routeRawLog = Join-Path $OutputDirectory 'route-determinism-raw.log'
$normalized = [Collections.Generic.List[string]]::new()
$output = @(& $simulationTests --filter 'vertical slice determinism' 2>&1)
if ($LASTEXITCODE -ne 0) { throw "route determinism run failed: $($output -join ' ')" }
[IO.File]::WriteAllLines($routeRawLog, @($output), [Text.UTF8Encoding]::new($false))
Add-Artifact $routeRawLog
$traceText = $output -join "`n"
$traces = @(
    [regex]::Match($traceText, '(?m)^\[TRACE\] canonical_playthrough_v3 (\d+) (\d+) (\d+)$'),
    [regex]::Match($traceText, '(?m)^\[TRACE\] canonical_playthrough_v3_repeat (\d+) (\d+) (\d+)$')
)
if (@($traces | Where-Object Success).Count -ne 2) { throw 'route determinism repeat traces missing' }
function Get-HexPayloadBytes([string]$Hex) {
    if ($Hex.Length -eq 0 -or ($Hex.Length % 2) -ne 0 -or $Hex -notmatch '^[0-9a-f]+$') {
        throw 'ordered event payload is not canonical lowercase hex'
    }
    $bytes = New-Object byte[] ($Hex.Length / 2)
    for ($index = 0; $index -lt $bytes.Length; ++$index) {
        $bytes[$index] = [Convert]::ToByte($Hex.Substring($index * 2, 2), 16)
    }
    return $bytes
}
for ($traceIndex = 0; $traceIndex -lt $traces.Count; ++$traceIndex) {
    $trace = $traces[$traceIndex]
    $runName = if ($traceIndex -eq 0) { 'baseline' } else { 'repeat' }
    $routes = @(
        @('virela_win','Victory',$trace.Groups[1].Value),
        @('structural_win','Victory',$trace.Groups[2].Value),
        @('no_ability_loss','Defeat',$trace.Groups[3].Value)
    )
    foreach ($route in $routes) {
        $payloadMatch = [regex]::Match(
            $traceText,
            "(?m)^\[TRACE\] ordered_events_v1 $([regex]::Escape($route[0])) $runName ([0-9a-f]+)$")
        if (-not $payloadMatch.Success) {
            throw "ordered event payload missing: $($route[0])/$runName"
        }
        $bytes = Get-HexPayloadBytes $payloadMatch.Groups[1].Value
        $algorithm = [Security.Cryptography.SHA256]::Create()
        try {
            $sha = -join ($algorithm.ComputeHash($bytes) | ForEach-Object { $_.ToString('x2') })
        } finally {
            $algorithm.Dispose()
        }
        $normalized.Add("NINHO_ROUTE_HASH name=$($route[0]) outcome=$($route[1]) canonical_state_v1=$($route[2]) events_sha256=$sha")
    }
}
[IO.File]::WriteAllLines($routeLog, $normalized, [Text.UTF8Encoding]::new($false))
Add-Artifact $routeLog
$routeEvidence = @(Get-NinhoRouteEvidenceFromLog -Text ([IO.File]::ReadAllText($routeLog)))

$statePattern = '(?m)^NINHO_CAPTURE_STATE frame=(\d+) tick=(\d+) phase=(\S+) outcome=(\S+) camera=(\([^)]+\)) exposure=([0-9.]+)\s*$'
$states = @([regex]::Matches([IO.File]::ReadAllText($vulkanCaptureLog), $statePattern))
function Find-State([string]$Phase, [string]$Outcome = '') {
    $found = @($states | Where-Object {
        $_.Groups[3].Value -ceq $Phase -and (-not $Outcome -or $_.Groups[4].Value -ceq $Outcome)
    } | Select-Object -First 1)
    if ($found.Count -ne 1) { throw "capture state missing: phase=$Phase outcome=$Outcome" }
    $match = $found[0]
    return [pscustomobject]@{
        frame=[int]$match.Groups[1].Value; tick=[int64]$match.Groups[2].Value
        phase=$match.Groups[3].Value; outcome=$match.Groups[4].Value
        camera=$match.Groups[5].Value; exposure=[double]$match.Groups[6].Value
        event='phase_transition'
    }
}
$overviewState = Find-State 'inspection'
$aimState = Find-State 'aim'
$virelaState = Find-State 'flight_ability'
$resultState = Find-State 'result' 'victory'
$eventPattern = '(?m)^NINHO_CAPTURE_EVENT frame=(\d+) tick=(\d+) kind=(\S+) profile=(\S+) affected=(\d+) damage=([0-9.]+) camera=(\([^)]+\)) exposure=([0-9.]+)\s*$'
$vulnerableMatch = @([regex]::Matches([IO.File]::ReadAllText($vulkanCaptureLog), $eventPattern) |
    Where-Object { $_.Groups[4].Value -ceq 'vulnerable' -and $_.Groups[5].Value -ceq '200' -and [double]$_.Groups[6].Value -gt 0 } |
    Select-Object -First 1)
if ($vulnerableMatch.Count -ne 1) { throw 'capture has no positive vulnerable Anchor damage event' }
$impactState = [pscustomobject]@{
    frame=[int]$vulnerableMatch[0].Groups[1].Value; tick=[int64]$vulnerableMatch[0].Groups[2].Value
    phase='resolution'; outcome='none'; camera=$vulnerableMatch[0].Groups[7].Value
    exposure=[double]$vulnerableMatch[0].Groups[8].Value
    event="damage_applied:vulnerable:anchor:$($vulnerableMatch[0].Groups[6].Value)"
}
$goldenPlan = [ordered]@{
    overview=@($overviewState,0)
    aim=@($aimState,30)
    virela=@($virelaState,5)
    vulnerable_impact=@($impactState,1)
    result=@($resultState,10)
}
$vulkanRenderer = @($rendererEvidence | Where-Object name -CEQ 'Vulkan')[0]
$goldenMetadata = [Collections.Generic.List[object]]::new()
foreach ($entry in $goldenPlan.GetEnumerator()) {
    $state = $entry.Value[0]
    $frame = Get-NinhoCausalFrameIndex `
        -Mapping $vulkanSelectedFrames `
        -SourceTransitionFrame ([int]$state.frame) `
        -PostTransitionFrames ([int]$entry.Value[1])
    $sourceFrame = [int]$vulkanSelectedFrames[$frame]
    $target = Join-Path $GoldenDirectory "$($entry.Key).png"
    $goldenStdout = Join-Path $OutputDirectory "golden-$($entry.Key).stdout.log"
    $goldenStderr = Join-Path $OutputDirectory "golden-$($entry.Key).stderr.log"
    $goldenProcess = Invoke-NinhoTimedProcess -FilePath $ffmpeg -ArgumentList @(
        '-v','error','-y','-i',$vulkanRawMovie,'-vf',"select=eq(n\,$sourceFrame)",
        '-frames:v','1',$target
    ) -TimeoutMs 120000 -StdoutPath $goldenStdout -StderrPath $goldenStderr `
        -FatalMarker "NINHO_CAPTURE_FATAL name=golden-$($entry.Key) reason=timeout"
    if ($goldenProcess.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $target -PathType Leaf)) {
        throw "failed to extract golden: $($entry.Key)"
    }
    $null = Assert-CleanLog $goldenStdout $goldenStderr
    Add-Artifact $goldenStdout
    Add-Artifact $goldenStderr
    Add-Artifact $target
    $goldenMetadata.Add([ordered]@{
        name=$entry.Key
        frame=$frame
        source_frame=$sourceFrame
        source_transition_frame=[int]$state.frame
        tick=[int64]$state.tick
        phase=$state.phase
        outcome=$state.outcome
        event=$state.event
        camera=$state.camera
        exposure=[double]$state.exposure
        renderer='Vulkan'
        path=(Get-RelativePath $root $target).Replace('\','/')
        sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $target).Hash.ToLowerInvariant()
    })
}

$manifest = [ordered]@{
    schema = 'ninho.vertical-slice.capture.v1'
    configuration = $Configuration
    renderers = @($rendererEvidence)
    routes = $routeEvidence
    route_raw_log = [ordered]@{
        path=(Get-RelativePath $root $routeRawLog).Replace('\','/')
        sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $routeRawLog).Hash.ToLowerInvariant()
    }
    goldens = @($goldenPlan.Keys)
    golden_metadata = @($goldenMetadata)
    source_artifacts = @($artifacts)
}
$manifestPath = Join-Path $OutputDirectory 'capture-manifest.json'
$manifest | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $manifestPath -Encoding utf8
Write-Output $manifestPath
