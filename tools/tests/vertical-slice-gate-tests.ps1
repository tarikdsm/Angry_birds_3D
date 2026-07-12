[CmdletBinding()]
param([Parameter(Mandatory)][string]$Root)

$ErrorActionPreference = 'Stop'
$module = Join-Path $Root 'tools\VerticalSliceGate.psm1'
if (-not (Test-Path -LiteralPath $module -PathType Leaf)) {
    throw 'VerticalSliceGate.psm1 missing (expected RED before implementation)'
}
Import-Module $module -Force

function Assert-Throws([scriptblock]$Operation, [string]$Expected) {
    try { & $Operation } catch {
        if ($_.Exception.Message -notlike "*$Expected*") {
            throw "Unexpected failure: $($_.Exception.Message)"
        }
        return
    }
    throw "Expected failure containing: $Expected"
}

$routeLog = @'
NINHO_ROUTE_HASH name=virela_win outcome=Victory canonical_state_v1=101 events_sha256=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
NINHO_ROUTE_HASH name=virela_win outcome=Victory canonical_state_v1=101 events_sha256=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
NINHO_ROUTE_HASH name=structural_win outcome=Victory canonical_state_v1=202 events_sha256=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
NINHO_ROUTE_HASH name=structural_win outcome=Victory canonical_state_v1=202 events_sha256=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
NINHO_ROUTE_HASH name=no_ability_loss outcome=Defeat canonical_state_v1=303 events_sha256=cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
NINHO_ROUTE_HASH name=no_ability_loss outcome=Defeat canonical_state_v1=303 events_sha256=cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
'@
$parsedRoutes = @(Get-NinhoRouteEvidenceFromLog -Text $routeLog)
if ($parsedRoutes.Count -ne 3 -or $parsedRoutes[0].hashes.Count -ne 2) {
    throw 'route evidence parser did not produce three repeated hashes'
}
$mutatedRouteLog = ([regex]'canonical_state_v1=101').Replace(
    $routeLog, 'canonical_state_v1=999', 1)
Assert-Throws { Get-NinhoRouteEvidenceFromLog -Text $mutatedRouteLog } 'repeat hash'

$causalMapping = [int[]]@(0,13,27,40,53,66,80,93)
$causalIndex = Get-NinhoCausalFrameIndex -Mapping $causalMapping -SourceTransitionFrame 55 -PostTransitionFrames 3
if ($causalIndex -ne 5 -or $causalMapping[$causalIndex] -ne 66) {
    throw 'causal golden selection did not choose the first frame after the requested transition tail'
}
Assert-Throws {
    Get-NinhoCausalFrameIndex -Mapping ([int[]]@(0,13,13)) -SourceTransitionFrame 1
} 'strictly increasing'
Assert-Throws {
    Get-NinhoCausalFrameIndex -Mapping ([int[]]@(0,13,27)) -SourceTransitionFrame 27 -PostTransitionFrames 1
} 'causal tail unavailable'
$injectedMapping = Set-NinhoRequiredDownsampleFrame `
    -Mapping ([int[]]@(0,13,27,40,53)) -RequiredSourceFrame 28
if (($injectedMapping -join ',') -cne '0,13,28,40,53') {
    throw 'required causal golden frame was not injected deterministically'
}
Assert-Throws {
    Set-NinhoRequiredDownsampleFrame -Mapping ([int[]]@(0,13,27)) -RequiredSourceFrame 28
} 'cannot be injected'

$causalInputMarkers = @(
    [ordered]@{sample_index=0;command_id='set_aim_center';theta_degrees=0.0;phase_degrees=0.0;speed=10.5;observed_tick=4;preview_hash='101'},
    [ordered]@{sample_index=1;command_id='set_aim_right';theta_degrees=2.0;phase_degrees=0.0;speed=8.0;observed_tick=5;preview_hash='202'},
    [ordered]@{sample_index=2;command_id='set_aim_left';theta_degrees=-2.0;phase_degrees=0.0;speed=8.0;observed_tick=6;preview_hash='303'}
)
Assert-NinhoInputFeedbackMarkers -Markers $causalInputMarkers -Context 'fixture'
Assert-Throws {
    Assert-NinhoInputFeedbackMarkers -Markers @($causalInputMarkers[0],$causalInputMarkers[1]) -Context 'missing fixture'
} 'exactly three causal markers'
$duplicateInputMarkers = @($causalInputMarkers | ForEach-Object { [ordered]@{} + $_ })
$duplicateInputMarkers[2].preview_hash = '202'
Assert-Throws {
    Assert-NinhoInputFeedbackMarkers -Markers $duplicateInputMarkers -Context 'duplicate fixture'
} 'duplicated causal command or preview hash'

$timeoutFixture = Join-Path $Root 'artifacts\timed-process-fixture'
New-Item -ItemType Directory -Force -Path $timeoutFixture | Out-Null
try {
    $timeoutStdout = Join-Path $timeoutFixture 'stdout.log'
    $timeoutStderr = Join-Path $timeoutFixture 'stderr.log'
    Assert-Throws {
        Invoke-NinhoTimedProcess -FilePath 'powershell.exe' -ArgumentList @(
            '-NoProfile','-Command','Start-Sleep -Seconds 5'
        ) -TimeoutMs 200 -StdoutPath $timeoutStdout -StderrPath $timeoutStderr `
            -FatalMarker 'NINHO_CAPTURE_FATAL name=fake-hang reason=timeout'
    } 'timed out'
    if (-not ([IO.File]::ReadAllText($timeoutStderr)).Contains('NINHO_CAPTURE_FATAL name=fake-hang reason=timeout')) {
        throw 'timed process timeout did not persist its fatal marker'
    }
} finally {
    if (Test-Path -LiteralPath $timeoutFixture) { Remove-Item -LiteralPath $timeoutFixture -Recurse -Force }
}

$retryResult = Invoke-NinhoLimitedRetry -Name 'Vulkan fixture' -MaximumAttempts 2 -Operation {
    param($attempt)
    return @{ ExitCode = $(if ($attempt -eq 1) { -1073741819 } else { 0 }); Value = $attempt }
}
if ($retryResult.AttemptCount -ne 2 -or $retryResult.Value -ne 2) { throw 'limited retry did not preserve success result' }
Assert-Throws {
    Invoke-NinhoLimitedRetry -Name 'persistent Vulkan fixture' -MaximumAttempts 2 -Operation {
        param($attempt)
        return @{ ExitCode = -1073741819; Value = $attempt }
    }
} 'failed after 2 attempts'

$packageOutputFixture = Join-Path $Root 'artifacts\package-output-contract'
New-Item -ItemType Directory -Force -Path $packageOutputFixture | Out-Null
try {
    $packageManifestFixture = Join-Path $packageOutputFixture 'manifest.sha256.json'
    [IO.File]::WriteAllText($packageManifestFixture, '{"schema_version":1}')
    $validPackageMarker = 'NINHO_PACKAGE_MANIFEST ' +
        (@{ path=$packageManifestFixture } | ConvertTo-Json -Compress)
    $selectedPackageManifest = Resolve-NinhoPackageManifestOutput `
        -Output @('Windows package launch: PASS', $validPackageMarker) `
        -ExpectedOutputRoot $packageOutputFixture
    if ($selectedPackageManifest -cne [IO.Path]::GetFullPath($packageManifestFixture)) {
        throw 'package output parser did not select the canonical manifest'
    }
    Assert-Throws {
        Resolve-NinhoPackageManifestOutput -Output @('Windows package launch: PASS') `
            -ExpectedOutputRoot $packageOutputFixture
    } 'exactly one'
    Assert-Throws {
        Resolve-NinhoPackageManifestOutput -Output @($validPackageMarker,$validPackageMarker) `
            -ExpectedOutputRoot $packageOutputFixture
    } 'exactly one'
    $escapeMarker = 'NINHO_PACKAGE_MANIFEST ' +
        (@{ path=(Join-Path $Root 'README.md') } | ConvertTo-Json -Compress)
    Assert-Throws {
        Resolve-NinhoPackageManifestOutput -Output @($escapeMarker) `
            -ExpectedOutputRoot $packageOutputFixture
    } 'outside expected output root'
} finally {
    if (Test-Path -LiteralPath $packageOutputFixture) {
        Remove-Item -LiteralPath $packageOutputFixture -Recurse -Force
    }
}

$sandbox = Join-Path $Root "artifacts\vertical-slice-gate-test-$([guid]::NewGuid().ToString('N'))"
New-Item -ItemType Directory -Force -Path $sandbox | Out-Null
try {
    New-Item -ItemType Directory -Force -Path (Join-Path $sandbox 'game') | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $sandbox 'cmake') | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $sandbox 'tools\art') | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $sandbox 'tools\audio') | Out-Null
    [IO.File]::WriteAllText((Join-Path $sandbox '.gitattributes'), '* text=auto')
    [IO.File]::WriteAllText((Join-Path $sandbox '.gitignore'), 'artifacts/')
    [IO.File]::WriteAllText((Join-Path $sandbox 'cmake\Dependencies.cmake'), 'include(FetchContent)')
    [IO.File]::WriteAllText((Join-Path $sandbox 'game\tested-input.txt'), 'fixture')
    [IO.File]::WriteAllText((Join-Path $sandbox 'tools\art\vertical_slice_asset_manifest.json'), '{"fixture":"art"}')
    [IO.File]::WriteAllText((Join-Path $sandbox 'tools\audio\audio_manifest.json'), '{"fixture":"audio"}')
    & git -C $sandbox init --quiet
    & git -c "safe.directory=$sandbox" -C $sandbox config user.email 'fixture@example.invalid'
    & git -c "safe.directory=$sandbox" -C $sandbox config user.name 'Fixture'
    & git -c "safe.directory=$sandbox" -C $sandbox add -- .gitattributes .gitignore cmake/Dependencies.cmake game/tested-input.txt tools/art/vertical_slice_asset_manifest.json tools/audio/audio_manifest.json
    & git -c "safe.directory=$sandbox" -C $sandbox commit --quiet -m fixture
    if ($LASTEXITCODE -ne 0) { throw 'failed to initialize evidence git fixture' }
    $artifact = Join-Path $sandbox 'capture.avi'
    & ffmpeg.exe -v error -f lavfi -i 'color=c=black:s=1920x1080:r=60' `
        -frames:v 300 -c:v mjpeg -q:v 31 -y $artifact
    if ($LASTEXITCODE -ne 0) { throw 'failed to create capture fixture' }
    $artifactOriginal = [IO.File]::ReadAllBytes($artifact)
    $log = Join-Path $sandbox 'capture.log'
    [IO.File]::WriteAllText($log, @'
NINHO_CAPTURE_STATE frame=1 tick=1 phase=inspection outcome=none camera=(0,0,0) exposure=1.0
NINHO_CAPTURE_STATE frame=2 tick=2 phase=aim outcome=none camera=(0,0,0) exposure=1.0
NINHO_CAPTURE_STATE frame=3 tick=3 phase=flight_ability outcome=none camera=(0,0,0) exposure=1.0
NINHO_CAPTURE_EVENT frame=4 tick=4 kind=damage_applied profile=vulnerable affected=200 damage=25.000000 camera=(0,0,0) exposure=1.0
NINHO_CAPTURE_STATE frame=5 tick=5 phase=result outcome=victory camera=(0,0,0) exposure=1.0
capture complete
'@)
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $artifact).Hash.ToLowerInvariant()
    $logHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $log).Hash.ToLowerInvariant()
    $commit = (& git -c "safe.directory=$sandbox" -C $sandbox rev-parse HEAD).Trim()
    $testedInputs = Get-NinhoTestedInputs -Root $sandbox
    $provenancePayload =
        (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $sandbox 'tools\art\vertical_slice_asset_manifest.json')).Hash.ToLowerInvariant() + '|' +
        (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $sandbox 'tools\audio\audio_manifest.json')).Hash.ToLowerInvariant()
    $provenanceAlgorithm = [Security.Cryptography.SHA256]::Create()
    try { $provenanceHash = -join ($provenanceAlgorithm.ComputeHash([Text.Encoding]::UTF8.GetBytes($provenancePayload)) | ForEach-Object { $_.ToString('x2') }) }
    finally { $provenanceAlgorithm.Dispose() }
    $routeRawPath = Join-Path $sandbox 'route-raw.log'
    $routeRawFixture = @'
[TRACE] canonical_playthrough_v3 11 22 33
[TRACE] canonical_playthrough_v3_repeat 11 22 33
[TRACE] ordered_events_v1 virela_win baseline 01
[TRACE] ordered_events_v1 virela_win repeat 01
[TRACE] ordered_events_v1 structural_win baseline 02
[TRACE] ordered_events_v1 structural_win repeat 02
[TRACE] ordered_events_v1 no_ability_loss baseline 03
[TRACE] ordered_events_v1 no_ability_loss repeat 03
'@
    # Production uses WriteAllLines on Windows, so certify CRLF explicitly.
    [IO.File]::WriteAllText($routeRawPath, ($routeRawFixture -replace "\r?\n", "`r`n"))
    $routeRawHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $routeRawPath).Hash.ToLowerInvariant()
    $eventHashes = @{}
    foreach ($pair in @(@('virela_win',1),@('structural_win',2),@('no_ability_loss',3))) {
        $algorithm = [Security.Cryptography.SHA256]::Create()
        try { $eventHashes[$pair[0]] = -join ($algorithm.ComputeHash([byte[]]@($pair[1])) | ForEach-Object { $_.ToString('x2') }) }
        finally { $algorithm.Dispose() }
    }
    $evidence = [ordered]@{
        schema = 'ninho.vertical-slice.evidence.v1'; schema_version = 1
        configuration = 'Debug'; commit = $commit; generated_utc = '2026-07-11T12:00:00Z'
        source_revision = $commit; tested_inputs_schema='ninho.tested-inputs.v1'
        tested_inputs_sha256=$testedInputs.sha256; tested_inputs=@($testedInputs.files)
        hardware = @{ cpu = 'fixture'; gpu = 'fixture'; ram_bytes = 1; os = 'fixture' }
        canonical_state_contract = 'canonical_state_v1'
        routes = @(
            @{ name='virela_win'; outcome='Victory'; hashes=@('11','11'); ordered_events_sha256=$eventHashes.virela_win },
            @{ name='structural_win'; outcome='Victory'; hashes=@('22','22'); ordered_events_sha256=$eventHashes.structural_win },
            @{ name='no_ability_loss'; outcome='Defeat'; hashes=@('33','33'); ordered_events_sha256=$eventHashes.no_ability_loss }
        )
        acceptance = @{ foundation=$true; slice_tests=$true; restart_20=$true; abi=$true; scanner=$true; assets=$true; smokes=$true; box3d_only=$true }
        physics = @{ step_p95_ms = 1.0; limit_ms = 8.0 }
        renderers = @(
            @{ name='Vulkan'; scales=@(
                @{ ui_scale=100; frame_p95_ms=10.0; frame_p99_ms=15.0; max_hitch_ms=20.0; input_feedback_p95_ms=16.7; input_feedback_samples=3; input_feedback_markers=$causalInputMarkers; frames_measured=300; rupture_observed=$true; vfx_observed=$true; post_vfx_frames=30 },
                @{ ui_scale=150; frame_p95_ms=10.0; frame_p99_ms=15.0; max_hitch_ms=20.0; input_feedback_p95_ms=16.7; input_feedback_samples=3; input_feedback_markers=$causalInputMarkers; frames_measured=300; rupture_observed=$true; vfx_observed=$true; post_vfx_frames=30 }
            ); capture=@{ frames=300; width=1920; height=1080; hash=$hash; path='capture.avi'; log_path='capture.log'; log_hash=$logHash; source_path='capture.avi'; source_hash=$hash; source_frames=300; downsample_source_frames=@(0..299) } },
            @{ name='OpenGL'; scales=@(
                @{ ui_scale=100; frame_p95_ms=10.0; frame_p99_ms=15.0; max_hitch_ms=20.0; input_feedback_p95_ms=16.7; input_feedback_samples=3; input_feedback_markers=$causalInputMarkers; frames_measured=300; rupture_observed=$true; vfx_observed=$true; post_vfx_frames=30 },
                @{ ui_scale=150; frame_p95_ms=10.0; frame_p99_ms=15.0; max_hitch_ms=20.0; input_feedback_p95_ms=16.7; input_feedback_samples=3; input_feedback_markers=$causalInputMarkers; frames_measured=300; rupture_observed=$true; vfx_observed=$true; post_vfx_frames=30 }
            ); capture=@{ frames=300; width=1920; height=1080; hash=$hash; path='capture.avi'; log_path='capture.log'; log_hash=$logHash; source_path='capture.avi'; source_hash=$hash; source_frames=300; downsample_source_frames=@(0..299) } }
        )
        goldens = @('overview','aim','virela','vulnerable_impact','result')
        golden_metadata = @(
            @{name='overview';frame=1;source_frame=1;source_transition_frame=1;tick=1;phase='inspection';outcome='none';event='phase_transition';camera='(0,0,0)';exposure=1.0;renderer='Vulkan';path='docs/art/goldens/vertical-slice/debug/overview.png';sha256=('1'*64)},
            @{name='aim';frame=2;source_frame=2;source_transition_frame=2;tick=2;phase='aim';outcome='none';event='phase_transition';camera='(0,0,0)';exposure=1.0;renderer='Vulkan';path='docs/art/goldens/vertical-slice/debug/aim.png';sha256=('2'*64)},
            @{name='virela';frame=3;source_frame=3;source_transition_frame=3;tick=3;phase='flight_ability';outcome='none';event='phase_transition';camera='(0,0,0)';exposure=1.0;renderer='Vulkan';path='docs/art/goldens/vertical-slice/debug/virela.png';sha256=('3'*64)},
            @{name='vulnerable_impact';frame=4;source_frame=4;source_transition_frame=4;tick=4;phase='resolution';outcome='none';event='damage_applied:vulnerable:anchor:25.000000';camera='(0,0,0)';exposure=1.0;renderer='Vulkan';path='docs/art/goldens/vertical-slice/debug/impact.png';sha256=('4'*64)},
            @{name='result';frame=5;source_frame=5;source_transition_frame=5;tick=5;phase='result';outcome='victory';event='phase_transition';camera='(0,0,0)';exposure=1.0;renderer='Vulkan';path='docs/art/goldens/vertical-slice/debug/result.png';sha256=('5'*64)}
        )
        rubric = @{ visual='approved'; gameplay='approved'; architecture='approved'; code='approved'; critical=0; important=0 }
        playtest = @{ status='unavailable'; substitute='independent_agents'; legal_limit='not_legal_advice' }
        clean_room = @{ approved=$true; provenance_manifest_sha256=$provenanceHash; comparative_review='approved' }
        source_artifacts = @(
            @{ path='capture.avi'; sha256=$hash },
            @{ path='capture.log'; sha256=$logHash },
            @{ path='route-raw.log'; sha256=$routeRawHash }
        )
    }
    foreach ($metadata in $evidence.golden_metadata) {
        $goldenPath = Join-Path $sandbox $metadata.path
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $goldenPath) | Out-Null
        & ffmpeg.exe -v error -y -i $artifact -vf "select=eq(n\,$($metadata.source_frame))" -frames:v 1 $goldenPath
        if ($LASTEXITCODE -ne 0) { throw "failed to derive golden fixture: $($metadata.name)" }
        $goldenHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $goldenPath).Hash.ToLowerInvariant()
        $metadata.sha256 = $goldenHash
        $evidence.source_artifacts += @(@{ path=$metadata.path; sha256=$goldenHash })
    }
    foreach ($renderer in $evidence.renderers) {
        foreach ($scale in $renderer.scales) {
            $metricsName = "metrics-$($renderer.name.ToLowerInvariant())-$($scale.ui_scale).json"
            $metricsPath = Join-Path $sandbox $metricsName
            $metrics = [ordered]@{
                schema='ninho.vertical-slice.runtime-metrics.v1'
                frame_p95_ms=$scale.frame_p95_ms; frame_p99_ms=$scale.frame_p99_ms
                max_hitch_ms=$scale.max_hitch_ms; input_feedback_p95_ms=$scale.input_feedback_p95_ms
                input_feedback_samples=$scale.input_feedback_samples
                input_feedback_markers=$scale.input_feedback_markers; physics_step_p95_ms=1.0
                frames_measured=$scale.frames_measured; rupture_observed=$true; vfx_observed=$true
                post_vfx_frames=$scale.post_vfx_frames
            }
            $metrics | ConvertTo-Json | Set-Content -LiteralPath $metricsPath -Encoding utf8
            $metricsHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $metricsPath).Hash.ToLowerInvariant()
            $scale.physics_step_p95_ms = 1.0
            $scale.metrics_path = $metricsName
            $scale.metrics_sha256 = $metricsHash
            $evidence.source_artifacts += @(@{path=$metricsName;sha256=$metricsHash})
        }
    }
    $reviewsPath = Join-Path $sandbox 'reviews.json'
    $reviews = [ordered]@{
        schema='ninho.vertical-slice.reviews.v1'; tested_inputs_sha256=$testedInputs.sha256
        reviews=@('code','architecture','gameplay','art' | ForEach-Object {
            [ordered]@{role=$_;verdict='approved';critical=0;important=0;minor=0}
        })
    }
    $reviews | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reviewsPath -Encoding utf8
    $evidence.reviews_manifest = @{
        path='reviews.json';sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $reviewsPath).Hash.ToLowerInvariant()
    }
    $captureManifestPath = Join-Path $sandbox 'capture-manifest.json'
    $captureManifest = [ordered]@{
        schema='ninho.vertical-slice.capture.v1';configuration='Debug'
        routes=$evidence.routes;renderers=$evidence.renderers;goldens=$evidence.goldens
        golden_metadata=$evidence.golden_metadata;source_artifacts=$evidence.source_artifacts
        route_raw_log=@{path='route-raw.log';sha256=$routeRawHash}
    }
    $captureManifest | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $captureManifestPath -Encoding utf8
    $evidence.capture_manifest = @{
        path='capture-manifest.json';sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    }
    $evidencePath = Join-Path $sandbox 'evidence.json'
    $evidence | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8

    Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit
    $validEvidenceText = [IO.File]::ReadAllText($evidencePath)
    $validCaptureManifestText = [IO.File]::ReadAllText($captureManifestPath)
    $validCaptureManifestBytes = [IO.File]::ReadAllBytes($captureManifestPath)
    $duplicateMarkerEvidence = $validEvidenceText | ConvertFrom-Json
    $duplicateMarkerEvidence.renderers[0].scales[0].input_feedback_markers[2].preview_hash = '202'
    $duplicateMarkerEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath `
            -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit
    } 'duplicated causal command or preview hash'
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)
    $cmakeInputPath = Join-Path $sandbox 'cmake\Dependencies.cmake'
    $cmakeInputOriginal = [IO.File]::ReadAllText($cmakeInputPath)
    [IO.File]::WriteAllText($cmakeInputPath, 'include(FetchContent)`n# stale mutation')
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath `
            -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit
    } 'tested inputs aggregate SHA-256 mismatch'
    [IO.File]::WriteAllText($cmakeInputPath, $cmakeInputOriginal)
    $claimMutation = $validEvidenceText | ConvertFrom-Json
    $claimMutation.renderers[0].scales[0].frame_p95_ms = 0.001
    $claimMutation | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit } 'differs from capture manifest'
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)

    $manualGoldenEvidence = $validEvidenceText | ConvertFrom-Json
    $manualGoldenManifest = $validCaptureManifestText | ConvertFrom-Json
    $manualGoldenPath = Join-Path $sandbox $manualGoldenEvidence.golden_metadata[0].path
    $manualGoldenOriginal = [IO.File]::ReadAllBytes($manualGoldenPath)
    & ffmpeg.exe -v error -f lavfi -i 'color=c=red:s=1920x1080' -frames:v 1 -y $manualGoldenPath
    if ($LASTEXITCODE -ne 0) { throw 'failed to create manually substituted golden fixture' }
    $manualGoldenHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $manualGoldenPath).Hash.ToLowerInvariant()
    foreach ($document in $manualGoldenEvidence,$manualGoldenManifest) {
        $document.golden_metadata[0].sha256 = $manualGoldenHash
        @($document.source_artifacts | Where-Object path -CEQ $document.golden_metadata[0].path)[0].sha256 = $manualGoldenHash
    }
    $manualGoldenManifest | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $captureManifestPath -Encoding utf8
    $manualGoldenEvidence.capture_manifest.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    $manualGoldenEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    $manualWrittenEvidence = Get-Content -Raw -LiteralPath $evidencePath | ConvertFrom-Json
    $manualActualCaptureHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    if ($manualWrittenEvidence.capture_manifest.sha256 -cne $manualActualCaptureHash) {
        throw "manual golden fixture capture hash drift: expected=$($manualWrittenEvidence.capture_manifest.sha256) actual=$manualActualCaptureHash"
    }
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit
    } 'differs from raw source frame'
    [IO.File]::WriteAllBytes($manualGoldenPath, $manualGoldenOriginal)
    [IO.File]::WriteAllBytes($captureManifestPath, $validCaptureManifestBytes)
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)

    $fabricatedStateEvidence = $validEvidenceText | ConvertFrom-Json
    $fabricatedStateManifest = $validCaptureManifestText | ConvertFrom-Json
    foreach ($document in $fabricatedStateEvidence,$fabricatedStateManifest) {
        $aimMetadata = @($document.golden_metadata | Where-Object name -CEQ 'aim')[0]
        $aimMetadata.tick = 999
        $aimMetadata.camera = '(9,9,9)'
    }
    $fabricatedStateManifest | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $captureManifestPath -Encoding utf8
    $fabricatedStateEvidence.capture_manifest.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    $fabricatedStateEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit
    } 'golden state metadata differs from verified Vulkan log: aim'
    [IO.File]::WriteAllBytes($captureManifestPath, $validCaptureManifestBytes)
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)

    $fabricatedEventEvidence = $validEvidenceText | ConvertFrom-Json
    $fabricatedEventManifest = $validCaptureManifestText | ConvertFrom-Json
    foreach ($document in $fabricatedEventEvidence,$fabricatedEventManifest) {
        @($document.golden_metadata | Where-Object name -CEQ 'aim')[0].event = 'fabricated'
    }
    $fabricatedEventManifest | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $captureManifestPath -Encoding utf8
    $fabricatedEventEvidence.capture_manifest.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    $fabricatedEventEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit
    } 'golden phase event contract mismatch: aim'
    [IO.File]::WriteAllBytes($captureManifestPath, $validCaptureManifestBytes)
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)

    $reviewsOriginalBytes = [IO.File]::ReadAllBytes($reviewsPath)
    $reviewsOriginal = [IO.File]::ReadAllText($reviewsPath)
    $reviewsMutation = $reviewsOriginal | ConvertFrom-Json
    $reviewsMutation.reviews[0].verdict = 'changes_required'
    $reviewsMutation | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reviewsPath -Encoding utf8
    $reviewEvidenceMutation = $validEvidenceText | ConvertFrom-Json
    $reviewEvidenceMutation.reviews_manifest.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $reviewsPath).Hash.ToLowerInvariant()
    $reviewEvidenceMutation | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit } 'blocking or missing independent review'
    [IO.File]::WriteAllBytes($reviewsPath, $reviewsOriginalBytes)
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)

    $packageRoot = Join-Path $sandbox 'package'
    New-Item -ItemType Directory -Force -Path $packageRoot | Out-Null
    foreach ($name in @(
        'NinhoOrbital.exe','NinhoOrbital.pck','ninho_physics.windows.template_release.x86_64.dll',
        'package-content.json','licenses/THIRD_PARTY_NOTICES.md','licenses/sbom.spdx.json',
        'licenses/box3d.LICENSE.txt','licenses/godot.LICENSE.txt','licenses/godot.COPYRIGHT.txt',
        'licenses/godot-export-templates.LICENSE.txt','licenses/godot-cpp.LICENSE.txt',
        'licenses/nlohmann-json.LICENSE.txt')) {
        $directory = Split-Path -Parent (Join-Path $packageRoot $name)
        if ($directory) { New-Item -ItemType Directory -Force -Path $directory | Out-Null }
        [IO.File]::WriteAllText((Join-Path $packageRoot $name), $name)
    }
    $packageManifest = Join-Path $packageRoot 'manifest.sha256.json'
    [string[]]$packageNames = @(
        'NinhoOrbital.exe','NinhoOrbital.pck','ninho_physics.windows.template_release.x86_64.dll',
        'package-content.json','licenses/THIRD_PARTY_NOTICES.md','licenses/sbom.spdx.json',
        'licenses/box3d.LICENSE.txt','licenses/godot.LICENSE.txt','licenses/godot.COPYRIGHT.txt',
        'licenses/godot-export-templates.LICENSE.txt','licenses/godot-cpp.LICENSE.txt',
        'licenses/nlohmann-json.LICENSE.txt')
    [Array]::Sort($packageNames, [StringComparer]::Ordinal)
    $packageRows = @($packageNames | ForEach-Object {
        $file = Join-Path $packageRoot $_
        [ordered]@{path=$_;size_bytes=[int64](Get-Item $file).Length;sha256=(Get-FileHash $file -Algorithm SHA256).Hash.ToLowerInvariant()}
    })
    $packageDllHash = ($packageRows | Where-Object path -CEQ 'ninho_physics.windows.template_release.x86_64.dll').sha256
    [ordered]@{
        schema_version=1;algorithm='SHA-256';configuration='Release';files=$packageRows
        runtime_extension=@{path='ninho_physics.windows.template_release.x86_64.dll';source_path='game/bin/ninho_physics.windows.template_release.x86_64.dll';source_sha256=$packageDllHash}
    } | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $packageManifest -Encoding utf8
    $releaseEvidence = Get-Content -Raw -LiteralPath $evidencePath | ConvertFrom-Json
    $releaseEvidence.configuration = 'Release'
    foreach ($metadata in $releaseEvidence.golden_metadata) {
        $debugPath = [string]$metadata.path
        $releasePath = $debugPath.Replace('/debug/', '/release/')
        $releaseGolden = Join-Path $sandbox $releasePath
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $releaseGolden) | Out-Null
        Copy-Item -LiteralPath (Join-Path $sandbox $debugPath) -Destination $releaseGolden
        $metadata.path = $releasePath
        $artifactRow = @($releaseEvidence.source_artifacts | Where-Object path -CEQ $debugPath)
        if ($artifactRow.Count -ne 1) { throw "release golden fixture artifact missing: $debugPath" }
        $artifactRow[0].path = $releasePath
    }
    $captureManifest.configuration = 'Release'
    $captureManifest.golden_metadata = $releaseEvidence.golden_metadata
    $captureManifest.source_artifacts = $releaseEvidence.source_artifacts
    $captureManifest | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $captureManifestPath -Encoding utf8
    $releaseEvidence.capture_manifest.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    $releaseEvidence | Add-Member -NotePropertyName package -NotePropertyValue @{
        path='package'; manifest_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $packageManifest).Hash.ToLowerInvariant()
        launch_from_space_path='passed'
    }
    $releaseEvidence | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Release -ExpectedCommit $commit
    $validReleaseEvidenceText = [IO.File]::ReadAllText($evidencePath)
    $validPackageManifestText = [IO.File]::ReadAllText($packageManifest)
    $validPackageManifestBytes = [IO.File]::ReadAllBytes($packageManifest)
    $missingLicenseRelative = 'licenses/godot.COPYRIGHT.txt'
    $missingLicensePath = Join-Path $packageRoot $missingLicenseRelative
    $missingLicenseBytes = [IO.File]::ReadAllBytes($missingLicensePath)
    Remove-Item -LiteralPath $missingLicensePath -Force
    $incompletePackage = $validPackageManifestText | ConvertFrom-Json
    $incompletePackage.files = @($incompletePackage.files | Where-Object path -CNE $missingLicenseRelative)
    $incompletePackage | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $packageManifest -Encoding utf8
    $incompleteEvidence = $validReleaseEvidenceText | ConvertFrom-Json
    $incompleteEvidence.package.manifest_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $packageManifest).Hash.ToLowerInvariant()
    $incompleteEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Release -ExpectedCommit $commit
    } 'missing required distribution file'
    [IO.File]::WriteAllBytes($missingLicensePath, $missingLicenseBytes)
    [IO.File]::WriteAllBytes($packageManifest, $validPackageManifestBytes)
    [IO.File]::WriteAllText($evidencePath, $validReleaseEvidenceText)
    [IO.File]::AppendAllText((Join-Path $packageRoot 'NinhoOrbital.pck'), 'tamper')
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Release -ExpectedCommit $commit } 'package size mismatch'
    [IO.File]::WriteAllText((Join-Path $packageRoot 'NinhoOrbital.pck'), 'NinhoOrbital.pck')
    [IO.File]::AppendAllText($packageManifest, 'tamper')
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Release -ExpectedCommit $commit } 'package manifest SHA-256'
    $evidence | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8

    Remove-Item -LiteralPath $artifact
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit } 'missing'
    [IO.File]::WriteAllBytes($artifact, $artifactOriginal)

    $doc = Get-Content -Raw -LiteralPath $evidencePath | ConvertFrom-Json
    $doc.commit = 'ffffffffffffffffffffffffffffffffffffffff'
    $doc | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit } 'stale'

    $evidence | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    $doc = Get-Content -Raw -LiteralPath $evidencePath | ConvertFrom-Json
    $doc.renderers = @($doc.renderers | Where-Object name -CEQ 'Vulkan')
    $doc | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit } 'renderer'

    $evidence | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    [IO.File]::WriteAllBytes($artifact, [byte[]](9,9,9))
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit } 'SHA-256'

    [IO.File]::WriteAllBytes($artifact, $artifactOriginal)
    [IO.File]::WriteAllText($log, "SCRIPT ERROR: contamination`n")
    $doc = Get-Content -Raw -LiteralPath $evidencePath | ConvertFrom-Json
    $newLogHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $log).Hash.ToLowerInvariant()
    $doc.source_artifacts[1].sha256 = $newLogHash
    foreach ($renderer in $doc.renderers) { $renderer.capture.log_hash = $newLogHash }
    $doc | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug -ExpectedCommit $commit } 'forbidden diagnostic'
} finally {
    if (Test-Path -LiteralPath $sandbox) { Remove-Item -LiteralPath $sandbox -Recurse -Force }
}

Write-Output 'vertical slice gate tests: PASS'
