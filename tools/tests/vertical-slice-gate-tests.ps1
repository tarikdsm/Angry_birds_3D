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

function Set-NinhoFixtureArtReviewBinding {
    param(
        [Parameter(Mandatory)]$Evidence,
        [Parameter(Mandatory)]$CaptureManifest,
        [Parameter(Mandatory)][string]$ReviewsPath
    )

    $fixtureReviews = Get-Content -Raw -LiteralPath $ReviewsPath | ConvertFrom-Json
    $artReview = @($fixtureReviews.reviews | Where-Object role -CEQ 'art')
    if ($artReview.Count -ne 1) { throw 'fixture art review is not unique' }
    $binding = @($artReview[0].artifact_bindings | Where-Object {
        $_.capture_configuration -ceq [string]$Evidence.configuration
    })
    if ($binding.Count -ne 1) { throw 'fixture art review binding is not unique' }
    $binding[0].capture_manifest_sha256 = [string]$Evidence.capture_manifest.sha256
    $binding[0].goldens = @($CaptureManifest.golden_metadata | ForEach-Object {
        [pscustomobject]@{name=$_.name;sha256=$_.sha256}
    })
    $fixtureReviews | ConvertTo-Json -Depth 10 |
        Set-Content -LiteralPath $ReviewsPath -Encoding utf8
    $Evidence.reviews_manifest.sha256 =
        (Get-FileHash -Algorithm SHA256 -LiteralPath $ReviewsPath).Hash.ToLowerInvariant()
}

$routeLog = @'
NINHO_ROUTE_HASH name=virela_win outcome=Victory canonical_state_v2=101 events_sha256=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
NINHO_ROUTE_HASH name=virela_win outcome=Victory canonical_state_v2=101 events_sha256=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
NINHO_ROUTE_HASH name=structural_win outcome=Victory canonical_state_v2=202 events_sha256=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
NINHO_ROUTE_HASH name=structural_win outcome=Victory canonical_state_v2=202 events_sha256=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
NINHO_ROUTE_HASH name=no_ability_loss outcome=Defeat canonical_state_v2=303 events_sha256=cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
NINHO_ROUTE_HASH name=no_ability_loss outcome=Defeat canonical_state_v2=303 events_sha256=cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
'@
$parsedRoutes = @(Get-NinhoRouteEvidenceFromLog -Text $routeLog)
if ($parsedRoutes.Count -ne 3 -or $parsedRoutes[0].hashes.Count -ne 2) {
    throw 'route evidence parser did not produce three repeated hashes'
}
$mutatedRouteLog = ([regex]'canonical_state_v2=101').Replace(
    $routeLog, 'canonical_state_v2=999', 1)
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

$compactSourceFrameCount = 601
$compactUniformMapping = [int[]]@(for ($index = 0; $index -lt 300; ++$index) {
    [int][Math]::Round(
        [double]$index * [double]($compactSourceFrameCount - 1) / 299.0,
        [MidpointRounding]::AwayFromZero)
})
$compactUniformExpression = Get-NinhoCompactDownsampleSelectExpression `
    -Mapping $compactUniformMapping -SourceFrameCount $compactSourceFrameCount
if ($compactUniformExpression -cne 'eq(n\,round(round(n*299/600)*600/299))') {
    throw "uniform compact downsample expression drifted: $compactUniformExpression"
}
$compactInjectedMapping = Set-NinhoRequiredDownsampleFrame `
    -Mapping $compactUniformMapping -RequiredSourceFrame 101
$compactInjectedExpression = Get-NinhoCompactDownsampleSelectExpression `
    -Mapping $compactInjectedMapping -SourceFrameCount $compactSourceFrameCount
if ($compactInjectedExpression -cne
        'eq(n\,round(round(n*299/600)*600/299))*not(eq(n\,100))+eq(n\,101)' -or
        [regex]::Matches($compactInjectedExpression, 'eq\(n').Count -ne 3 -or
        $compactInjectedExpression.Length -gt 128) {
    throw "sparse compact downsample override drifted: $compactInjectedExpression"
}
Assert-Throws {
    Get-NinhoCompactDownsampleSelectExpression `
        -Mapping ([int[]]@(0..298)) -SourceFrameCount $compactSourceFrameCount
} 'exactly 300 frames'
$duplicateCompactMapping = [int[]]$compactUniformMapping.Clone()
$duplicateCompactMapping[10] = $duplicateCompactMapping[9]
Assert-Throws {
    Get-NinhoCompactDownsampleSelectExpression `
        -Mapping $duplicateCompactMapping -SourceFrameCount $compactSourceFrameCount
} 'strictly increasing'
$badEndpointCompactMapping = [int[]]$compactUniformMapping.Clone()
$badEndpointCompactMapping[299] = 599
Assert-Throws {
    Get-NinhoCompactDownsampleSelectExpression `
        -Mapping $badEndpointCompactMapping -SourceFrameCount $compactSourceFrameCount
} 'endpoints'
$denseOverrideMapping = [int[]]$compactUniformMapping.Clone()
foreach ($index in 10..18) { $denseOverrideMapping[$index] += 1 }
Assert-Throws {
    Get-NinhoCompactDownsampleSelectExpression `
        -Mapping $denseOverrideMapping -SourceFrameCount $compactSourceFrameCount
} 'more than 8 sparse overrides'
$captureScriptText = [IO.File]::ReadAllText((Join-Path $Root 'tools\capture_vertical_slice.ps1'))
if (-not $captureScriptText.Contains('Get-NinhoCompactDownsampleSelectExpression') -or
        $captureScriptText.Contains('$selectTerms = @($selectedFrames | ForEach-Object')) {
    throw 'vertical slice capture must consume the compact fail-closed downsample expression'
}

$abilityProof = Get-NinhoVirelaAbilityProof -Text @'
NINHO_CAPTURE_EVENT frame=42 tick=77 kind=ability_started profile=vortex affected=0 damage=0.000000 camera=(1,2,3) exposure=1.25
NINHO_CAPTURE_EVENT frame=43 tick=78 kind=ability_pulse profile=virela_pulse affected=0 damage=0.000000 camera=(4,5,6) exposure=1.25
'@
if ($abilityProof.frame -ne 42 -or $abilityProof.tick -ne 77 -or
        $abilityProof.event -cne 'ability_started:vortex' -or
        $abilityProof.phase -cne 'flight_ability' -or $abilityProof.outcome -cne 'none' -or
        $abilityProof.camera -cne '(1,2,3)' -or [Math]::Abs($abilityProof.exposure - 1.25) -gt 0.000001) {
    throw 'Virela ability proof did not preserve the first causal ability/VFX event'
}
$pulseProof = Get-NinhoVirelaAbilityProof -Text @'
NINHO_CAPTURE_EVENT frame=84 tick=99 kind=ability_pulse profile=virela_pulse affected=0 damage=0.000000 camera=(7,8,9) exposure=1.0
'@
if ($pulseProof.event -cne 'ability_pulse:virela_pulse') {
    throw 'Virela ability proof did not accept the pulse VFX event'
}
Assert-Throws {
    Get-NinhoVirelaAbilityProof -Text 'NINHO_CAPTURE_EVENT frame=3 tick=3 kind=ability_started profile=rejected affected=0 damage=0.000000 camera=(0,0,0) exposure=1.0'
} 'no Virela ability/VFX event'

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

$reviewCaptureHash = 'a' * 64
$reviewGoldenMetadata = @(
    @{name='overview';sha256=('1' * 64)},
    @{name='aim';sha256=('2' * 64)},
    @{name='virela';sha256=('3' * 64)},
    @{name='vulnerable_impact';sha256=('4' * 64)},
    @{name='result';sha256=('5' * 64)}
)
$boundReviews = [ordered]@{
    schema='ninho.vertical-slice.reviews.v2';schema_version=2
    tested_inputs_schema='ninho.tested-inputs.v2';tested_inputs_sha256=('b' * 64)
    reviews=@('code','architecture','gameplay','art' | ForEach-Object {
        $review = [ordered]@{
            role=$_;reviewer_id="/root/$($_)_review";verdict='approved'
            critical=0;important=0;minor=0
        }
        if ($_ -ceq 'art') {
            $review.artifact_bindings = @([ordered]@{
                    schema='ninho.vertical-slice.art-review-binding.v1'
                    capture_manifest_sha256=$reviewCaptureHash
                    capture_configuration='Debug'
                    goldens=@($reviewGoldenMetadata | ForEach-Object {
                        [ordered]@{name=$_.name;sha256=$_.sha256}
                    })
                })
        }
        $review
    })
}
$boundReviews = $boundReviews | ConvertTo-Json -Depth 10 | ConvertFrom-Json
$assertBoundReviews = {
    param($Document)
    Assert-NinhoAgentReviews -Reviews $Document `
        -ExpectedTestedInputsSha256 ('b' * 64) `
        -ExpectedCaptureManifestSha256 $reviewCaptureHash `
        -ExpectedCaptureConfiguration Debug `
        -ExpectedGoldenMetadata $reviewGoldenMetadata
}
& $assertBoundReviews $boundReviews

$legacyReviews = $boundReviews | ConvertTo-Json -Depth 10 | ConvertFrom-Json
$legacyReviews.schema = 'ninho.vertical-slice.reviews.v1'
$legacyReviews.schema_version = 1
Assert-Throws { & $assertBoundReviews $legacyReviews } 'schema/tested-content mismatch'

$captureDriftReviews = $boundReviews | ConvertTo-Json -Depth 10 | ConvertFrom-Json
@($captureDriftReviews.reviews | Where-Object role -CEQ 'art')[0].artifact_bindings[0].capture_manifest_sha256 = 'c' * 64
Assert-Throws { & $assertBoundReviews $captureDriftReviews } 'capture manifest SHA-256 mismatch'

$configurationDriftReviews = $boundReviews | ConvertTo-Json -Depth 10 | ConvertFrom-Json
@($configurationDriftReviews.reviews | Where-Object role -CEQ 'art')[0].artifact_bindings[0].capture_configuration = 'Release'
Assert-Throws { & $assertBoundReviews $configurationDriftReviews } 'capture configuration mismatch'

$goldenDriftReviews = $boundReviews | ConvertTo-Json -Depth 10 | ConvertFrom-Json
@($goldenDriftReviews.reviews | Where-Object role -CEQ 'art')[0].artifact_bindings[0].goldens[2].sha256 = 'c' * 64
Assert-Throws { & $assertBoundReviews $goldenDriftReviews } 'golden binding mismatch: virela'

$missingGoldenReviews = $boundReviews | ConvertTo-Json -Depth 10 | ConvertFrom-Json
$artBinding = @($missingGoldenReviews.reviews | Where-Object role -CEQ 'art')[0].artifact_bindings[0]
$artBinding.goldens = @($artBinding.goldens | Where-Object name -CNE 'result')
Assert-Throws { & $assertBoundReviews $missingGoldenReviews } 'exactly five canonical goldens'

$duplicateConfigurationReviews = $boundReviews | ConvertTo-Json -Depth 10 | ConvertFrom-Json
$artReview = @($duplicateConfigurationReviews.reviews | Where-Object role -CEQ 'art')[0]
$artReview.artifact_bindings = @($artReview.artifact_bindings[0],$artReview.artifact_bindings[0])
Assert-Throws { & $assertBoundReviews $duplicateConfigurationReviews } 'exactly one binding for capture configuration'

$cleanRoomScope = @('names','logos','silhouettes','sounds','ui','layouts','promotional_material')
$cleanRoomReview = [pscustomobject]@{
    schema='ninho.vertical-slice.clean-room-review.v1';schema_version=1
    result='approved_no_confusing_similarity';reviewed_utc='2026-07-19T12:00:00Z'
    reviewer=[pscustomobject]@{type='agent';id='/root/clean_room_review'}
    scope=$cleanRoomScope
    manifests=[pscustomobject]@{
        assets=[pscustomobject]@{
            path='tools/art/vertical_slice_asset_manifest.json';sha256=('a' * 64)
        }
        audio=[pscustomobject]@{
            path='tools/audio/audio_manifest.json';sha256=('b' * 64)
        }
    }
    legal_limit='not_legal_advice'
}
$assertCleanRoomReview = {
    param($Document)
    Assert-NinhoCleanRoomReview -Review $Document `
        -ExpectedAssetManifestSha256 ('a' * 64) `
        -ExpectedAudioManifestSha256 ('b' * 64)
}
& $assertCleanRoomReview $cleanRoomReview

$pendingCleanRoomReview = $cleanRoomReview | ConvertTo-Json -Depth 10 | ConvertFrom-Json
$pendingCleanRoomReview.result = 'pending'
Assert-Throws { & $assertCleanRoomReview $pendingCleanRoomReview } 'pending or not approved'

$anonymousCleanRoomReview = $cleanRoomReview | ConvertTo-Json -Depth 10 | ConvertFrom-Json
$anonymousCleanRoomReview.reviewer.id = ''
Assert-Throws { & $assertCleanRoomReview $anonymousCleanRoomReview } 'reviewer attribution is missing'

$scopeDriftCleanRoomReview = $cleanRoomReview | ConvertTo-Json -Depth 10 | ConvertFrom-Json
$scopeDriftCleanRoomReview.scope = @($scopeDriftCleanRoomReview.scope | Where-Object { $_ -cne 'sounds' })
Assert-Throws { & $assertCleanRoomReview $scopeDriftCleanRoomReview } 'scope mismatch'

$assetDriftCleanRoomReview = $cleanRoomReview | ConvertTo-Json -Depth 10 | ConvertFrom-Json
$assetDriftCleanRoomReview.manifests.assets.sha256 = 'c' * 64
Assert-Throws { & $assertCleanRoomReview $assetDriftCleanRoomReview } 'asset manifest SHA-256 mismatch'

$audioDriftCleanRoomReview = $cleanRoomReview | ConvertTo-Json -Depth 10 | ConvertFrom-Json
$audioDriftCleanRoomReview.manifests.audio.sha256 = 'c' * 64
Assert-Throws { & $assertCleanRoomReview $audioDriftCleanRoomReview } 'audio manifest SHA-256 mismatch'

$timeoutFixture = Join-Path $Root (
    'artifacts\timed-process-fixture-' + [Guid]::NewGuid().ToString('N'))
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

    $runToken = 'ninho-process-tree-' + [Guid]::NewGuid().ToString('N')
    $childPidPath = Join-Path $timeoutFixture 'child.pid'
    $childScript = Join-Path $timeoutFixture 'child.ps1'
    $parentScript = Join-Path $timeoutFixture 'parent.ps1'
    [IO.File]::WriteAllText($childScript, @'
param([Parameter(Mandatory)][string]$RunToken)
Start-Sleep -Seconds 30
'@, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText($parentScript, @'
param(
    [Parameter(Mandatory)][string]$RunToken,
    [Parameter(Mandatory)][string]$ChildScript,
    [Parameter(Mandatory)][string]$ChildPidPath
)
$child = Start-Process -FilePath 'powershell.exe' -ArgumentList @(
    '-NoProfile', '-File', ('"' + $ChildScript + '"'), '-RunToken', $RunToken
) -PassThru -WindowStyle Hidden
[IO.File]::WriteAllText($ChildPidPath, [string]$child.Id)
Write-Output 'FAKE_SUCCESS_MARKER'
exit 0
'@, [Text.UTF8Encoding]::new($false))

    $treeStdout = Join-Path $timeoutFixture 'tree.stdout.log'
    $treeStderr = Join-Path $timeoutFixture 'tree.stderr.log'
    $watch = [Diagnostics.Stopwatch]::StartNew()
    Assert-Throws {
        Invoke-NinhoTimedProcess -FilePath 'powershell.exe' -ArgumentList @(
            '-NoProfile', '-File', $parentScript,
            '-RunToken', $runToken,
            '-ChildScript', $childScript,
            '-ChildPidPath', $childPidPath
        ) -TimeoutMs 750 -StdoutPath $treeStdout -StderrPath $treeStderr `
            -ProcessIdentityToken $runToken `
            -FatalMarker 'NINHO_CAPTURE_FATAL name=fake-tree reason=timeout'
    } 'timed out'
    $watch.Stop()
    if ($watch.ElapsedMilliseconds -lt 500) {
        throw 'timed process accepted an early success marker before the process tree exited'
    }
    if (-not ([IO.File]::ReadAllText($treeStdout)).Contains('FAKE_SUCCESS_MARKER')) {
        throw 'process-tree fixture did not emit its early success marker'
    }
    if (-not ([IO.File]::ReadAllText($treeStderr)).Contains(
            'NINHO_CAPTURE_FATAL name=fake-tree reason=timeout')) {
        throw 'process-tree timeout did not persist its fatal marker'
    }
    $childPid = [int][IO.File]::ReadAllText($childPidPath)
    Start-Sleep -Milliseconds 100
    if ($null -ne (Get-Process -Id $childPid -ErrorAction SilentlyContinue)) {
        throw "timed process left child PID $childPid alive after watchdog"
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
    $fixtureFfmpegRoot = Join-Path $sandbox '.tools\ffmpeg\fixture\bin'
    New-Item -ItemType Directory -Force -Path $fixtureFfmpegRoot | Out-Null
    $fixtureGoldenBytes = [Text.Encoding]::UTF8.GetBytes('ninho-fixture-derived-golden')
    $fixtureFfmpeg = Join-Path $fixtureFfmpegRoot 'ffmpeg.exe'
    $fixtureFfprobe = Join-Path $fixtureFfmpegRoot 'ffprobe.exe'
    $fixtureClassName = 'NinhoMediaFixture' + [Guid]::NewGuid().ToString('N')
    Add-Type -TypeDefinition @"
using System;
using System.IO;
using System.Text;

public static class $fixtureClassName
{
    public static int Main(string[] args)
    {
        string executable = Path.GetFileNameWithoutExtension(
            Environment.GetCommandLineArgs()[0]);
        if (string.Equals(executable, "ffprobe", StringComparison.OrdinalIgnoreCase))
        {
            Console.WriteLine("{\"streams\":[{\"width\":1920,\"height\":1080,\"nb_frames\":\"300\"}]}");
            return 0;
        }
        if (args.Length == 0)
        {
            return 2;
        }
        File.WriteAllBytes(
            args[args.Length - 1],
            Encoding.UTF8.GetBytes("ninho-fixture-derived-golden"));
        return 0;
    }
}
"@ -Language CSharp -OutputAssembly $fixtureFfmpeg -OutputType ConsoleApplication
    Copy-Item -LiteralPath $fixtureFfmpeg -Destination $fixtureFfprobe
    $fixtureFfmpegHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $fixtureFfmpeg).Hash.ToLowerInvariant()
    $fixtureFfprobeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $fixtureFfprobe).Hash.ToLowerInvariant()
    [IO.File]::WriteAllText(
        (Join-Path $sandbox 'tools\toolchain.lock.json'),
        (@{
            ffmpeg = @{
                exe = 'fixture/bin/ffmpeg.exe'
                exe_sha256 = $fixtureFfmpegHash
                ffprobe_exe = 'fixture/bin/ffprobe.exe'
                ffprobe_exe_sha256 = $fixtureFfprobeHash
            }
        } | ConvertTo-Json -Depth 3),
        [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $sandbox '.gitattributes'), '* text=auto')
    [IO.File]::WriteAllText((Join-Path $sandbox '.gitignore'), 'artifacts/')
    [IO.File]::WriteAllText((Join-Path $sandbox 'cmake\Dependencies.cmake'), 'include(FetchContent)')
    [IO.File]::WriteAllText((Join-Path $sandbox 'game\tested-input.txt'), 'fixture')
    [IO.File]::WriteAllText((Join-Path $sandbox 'tools\art\vertical_slice_asset_manifest.json'), '{"fixture":"art"}')
    [IO.File]::WriteAllText((Join-Path $sandbox 'tools\audio\audio_manifest.json'), '{"fixture":"audio"}')
    & git -C $sandbox init --quiet
    & git -c "safe.directory=$sandbox" -C $sandbox config user.email 'fixture@example.invalid'
    & git -c "safe.directory=$sandbox" -C $sandbox config user.name 'Fixture'
    & git -c "safe.directory=$sandbox" -C $sandbox add -- .gitattributes .gitignore cmake/Dependencies.cmake game/tested-input.txt tools/art/vertical_slice_asset_manifest.json tools/audio/audio_manifest.json tools/toolchain.lock.json
    & git -c "safe.directory=$sandbox" -C $sandbox commit --quiet -m fixture
    if ($LASTEXITCODE -ne 0) { throw 'failed to initialize evidence git fixture' }
    $artifact = Join-Path $sandbox 'capture.avi'
    [IO.File]::WriteAllBytes($artifact, [Text.Encoding]::UTF8.GetBytes('ninho-fixture-capture'))
    $artifactOriginal = [IO.File]::ReadAllBytes($artifact)
    $log = Join-Path $sandbox 'capture.log'
    [IO.File]::WriteAllText($log, @'
NINHO_CAPTURE_STATE frame=1 tick=1 phase=inspection outcome=none camera=(0,0,0) exposure=1.0
NINHO_CAPTURE_STATE frame=2 tick=2 phase=aim outcome=none camera=(0,0,0) exposure=1.0
NINHO_CAPTURE_STATE frame=3 tick=3 phase=flight_ability outcome=none camera=(0,0,0) exposure=1.0
NINHO_CAPTURE_EVENT frame=4 tick=4 kind=ability_started profile=vortex affected=0 damage=0.000000 camera=(0,0,0) exposure=1.0
NINHO_CAPTURE_EVENT frame=5 tick=5 kind=damage_applied profile=vulnerable affected=200 damage=25.000000 camera=(0,0,0) exposure=1.0
NINHO_CAPTURE_STATE frame=6 tick=6 phase=result outcome=victory camera=(0,0,0) exposure=1.0
capture complete
'@)
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $artifact).Hash.ToLowerInvariant()
    $logHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $log).Hash.ToLowerInvariant()
    $commit = (& git -c "safe.directory=$sandbox" -C $sandbox rev-parse HEAD).Trim()
    $testedInputs = Get-NinhoTestedInputs -Root $sandbox
    $assetProvenanceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (
        Join-Path $sandbox 'tools\art\vertical_slice_asset_manifest.json')).Hash.ToLowerInvariant()
    $audioProvenanceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (
        Join-Path $sandbox 'tools\audio\audio_manifest.json')).Hash.ToLowerInvariant()
    $cleanRoomReviewRelativePath = 'docs/gameplay/evidence/vertical-slice-clean-room-review.json'
    $cleanRoomReviewPath = Join-Path $sandbox $cleanRoomReviewRelativePath
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $cleanRoomReviewPath) | Out-Null
    [ordered]@{
        schema='ninho.vertical-slice.clean-room-review.v1';schema_version=1
        result='approved_no_confusing_similarity';reviewed_utc='2026-07-19T12:00:00Z'
        reviewer=[ordered]@{type='agent';id='/root/fixture_clean_room_review'}
        scope=$cleanRoomScope
        manifests=[ordered]@{
            assets=[ordered]@{
                path='tools/art/vertical_slice_asset_manifest.json';sha256=$assetProvenanceHash
            }
            audio=[ordered]@{
                path='tools/audio/audio_manifest.json';sha256=$audioProvenanceHash
            }
        }
        legal_limit='not_legal_advice'
    } | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $cleanRoomReviewPath -Encoding utf8
    $cleanRoomReviewHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $cleanRoomReviewPath).Hash.ToLowerInvariant()
    $routeRawPath = Join-Path $sandbox 'route-raw.log'
    $routeRawFixture = @'
[TRACE] canonical_playthrough_v4 11 22 33
[TRACE] canonical_playthrough_v4_repeat 11 22 33
[TRACE] ordered_events_v2 virela_win baseline 01
[TRACE] ordered_events_v2 virela_win repeat 01
[TRACE] ordered_events_v2 structural_win baseline 02
[TRACE] ordered_events_v2 structural_win repeat 02
[TRACE] ordered_events_v2 no_ability_loss baseline 03
[TRACE] ordered_events_v2 no_ability_loss repeat 03
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
        source_revision = $commit; tested_inputs_schema='ninho.tested-inputs.v2'
        tested_inputs_sha256=$testedInputs.sha256; tested_inputs=@($testedInputs.files)
        hardware = @{ cpu = 'fixture'; gpu = 'fixture'; ram_bytes = 1; os = 'fixture' }
        canonical_state_contract = 'canonical_state_v2'
        routes = @(
            @{ name='virela_win'; outcome='Victory'; hashes=@('11','11'); ordered_events_sha256=$eventHashes.virela_win },
            @{ name='structural_win'; outcome='Victory'; hashes=@('22','22'); ordered_events_sha256=$eventHashes.structural_win },
            @{ name='no_ability_loss'; outcome='Defeat'; hashes=@('33','33'); ordered_events_sha256=$eventHashes.no_ability_loss }
        )
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
            @{name='virela';frame=4;source_frame=4;source_transition_frame=4;tick=4;phase='flight_ability';outcome='none';event='ability_started:vortex';camera='(0,0,0)';exposure=1.0;renderer='Vulkan';path='docs/art/goldens/vertical-slice/debug/virela.png';sha256=('3'*64)},
            @{name='vulnerable_impact';frame=5;source_frame=5;source_transition_frame=5;tick=5;phase='resolution';outcome='none';event='damage_applied:vulnerable:anchor:25.000000';camera='(0,0,0)';exposure=1.0;renderer='Vulkan';path='docs/art/goldens/vertical-slice/debug/impact.png';sha256=('4'*64)},
            @{name='result';frame=6;source_frame=6;source_transition_frame=6;tick=6;phase='result';outcome='victory';event='phase_transition';camera='(0,0,0)';exposure=1.0;renderer='Vulkan';path='docs/art/goldens/vertical-slice/debug/result.png';sha256=('5'*64)}
        )
        rubric = @{ visual='approved'; gameplay='approved'; architecture='approved'; code='approved'; critical=0; important=0 }
        playtest = @{
            status='not_performed'; participants=0; substitute='none'; gate_status='pending'
            required_before='product_release'; legal_limit='not_legal_advice'
        }
        clean_room_review = @{
            path=$cleanRoomReviewRelativePath;sha256=$cleanRoomReviewHash
        }
        source_artifacts = @(
            @{ path='capture.avi'; sha256=$hash },
            @{ path='capture.log'; sha256=$logHash },
            @{ path='route-raw.log'; sha256=$routeRawHash }
        )
    }
    foreach ($metadata in $evidence.golden_metadata) {
        $goldenPath = Join-Path $sandbox $metadata.path
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $goldenPath) | Out-Null
        [IO.File]::WriteAllBytes($goldenPath, $fixtureGoldenBytes)
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
    $captureManifestPath = Join-Path $sandbox 'capture-manifest.json'
    $captureManifest = [ordered]@{
        schema='ninho.vertical-slice.capture.v1';configuration='Debug'
        routes=$evidence.routes;renderers=$evidence.renderers;goldens=$evidence.goldens
        golden_metadata=$evidence.golden_metadata;source_artifacts=$evidence.source_artifacts
        route_raw_log=@{path='route-raw.log';sha256=$routeRawHash}
    }
    $captureManifest | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $captureManifestPath -Encoding utf8
    $debugCaptureManifestHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    $evidence.capture_manifest = @{
        path='capture-manifest.json';sha256=$debugCaptureManifestHash
    }
    $reviewsPath = Join-Path $sandbox 'reviews.json'
    $reviews = [ordered]@{
        schema='ninho.vertical-slice.reviews.v2';schema_version=2
        tested_inputs_schema='ninho.tested-inputs.v2';tested_inputs_sha256=$testedInputs.sha256
        reviews=@('code','architecture','gameplay','art' | ForEach-Object {
            $review = [ordered]@{role=$_;reviewer_id="/root/task12_$($_)_review";verdict='approved';critical=0;important=0;minor=0}
            if ($_ -ceq 'art') {
                $review.artifact_bindings = @([ordered]@{
                        schema='ninho.vertical-slice.art-review-binding.v1'
                        capture_manifest_sha256=$debugCaptureManifestHash
                        capture_configuration='Debug'
                        goldens=@($evidence.golden_metadata | ForEach-Object {
                            [ordered]@{name=$_.name;sha256=$_.sha256}
                        })
                    })
            }
            $review
        })
    }
    $reviews | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reviewsPath -Encoding utf8
    $evidence.reviews_manifest = @{
        path='reviews.json';sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $reviewsPath).Hash.ToLowerInvariant()
    }
    $evidencePath = Join-Path $sandbox 'evidence.json'
    $evidence | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8

    Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug
    $validEvidenceText = [IO.File]::ReadAllText($evidencePath)
    $validCaptureManifestText = [IO.File]::ReadAllText($captureManifestPath)
    $validCaptureManifestBytes = [IO.File]::ReadAllBytes($captureManifestPath)
    $validReviewsBytes = [IO.File]::ReadAllBytes($reviewsPath)

    $publishedEvidencePath = Join-Path $sandbox 'published-evidence.json'
    [IO.File]::WriteAllText($publishedEvidencePath, $validEvidenceText)
    $publishedEvidenceHash = (
        Get-FileHash -Algorithm SHA256 -LiteralPath $publishedEvidencePath
    ).Hash.ToLowerInvariant()
    $invalidPublication = $validEvidenceText | ConvertFrom-Json
    $invalidPublication.renderers = @(
        $invalidPublication.renderers | Where-Object name -CEQ 'Vulkan')
    $publicationFilesBefore = @(
        Get-ChildItem -LiteralPath $sandbox -Force -File |
            Sort-Object Name | ForEach-Object Name)
    Assert-Throws {
        Publish-NinhoVerticalSliceEvidence -Document $invalidPublication `
            -EvidencePath $publishedEvidencePath -ArtifactRoot $sandbox `
            -ExpectedConfiguration Debug
    } 'renderer'
    $publishedEvidenceHashAfterFailure = (
        Get-FileHash -Algorithm SHA256 -LiteralPath $publishedEvidencePath
    ).Hash.ToLowerInvariant()
    if ($publishedEvidenceHashAfterFailure -cne $publishedEvidenceHash) {
        throw 'failed publication replaced the previous canonical evidence'
    }
    $publicationFilesAfterFailure = @(
        Get-ChildItem -LiteralPath $sandbox -Force -File |
            Sort-Object Name | ForEach-Object Name)
    if (($publicationFilesAfterFailure -join "`n") -cne
            ($publicationFilesBefore -join "`n")) {
        throw 'failed publication left a temporary evidence file'
    }

    $successfulPublication = $validEvidenceText | ConvertFrom-Json
    $successfulPublication.generated_utc = '2026-07-19T18:00:00Z'
    Publish-NinhoVerticalSliceEvidence -Document $successfulPublication `
        -EvidencePath $publishedEvidencePath -ArtifactRoot $sandbox `
        -ExpectedConfiguration Debug | Out-Null
    $publishedEvidence = Get-Content -Raw -LiteralPath $publishedEvidencePath |
        ConvertFrom-Json
    if ($publishedEvidence.generated_utc -cne $successfulPublication.generated_utc) {
        throw 'successful publication did not promote the validated evidence'
    }
    $publicationFilesAfterSuccess = @(
        Get-ChildItem -LiteralPath $sandbox -Force -File |
            Sort-Object Name | ForEach-Object Name)
    if (($publicationFilesAfterSuccess -join "`n") -cne
            ($publicationFilesBefore -join "`n")) {
        throw 'successful publication left a temporary evidence file'
    }

    $legacyAcceptanceEvidence = $validEvidenceText | ConvertFrom-Json
    $legacyAcceptanceEvidence | Add-Member -NotePropertyName acceptance -NotePropertyValue @{
        foundation=$false; slice_tests=$false; restart_20=$false; abi=$false
        scanner=$false; assets=$false; smokes=$false; box3d_only=$false
    }
    $legacyAcceptanceEvidence | ConvertTo-Json -Depth 30 |
        Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath `
        -ArtifactRoot $sandbox -ExpectedConfiguration Debug | Out-Null
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)
    $legacyPlaytestEvidence = $validEvidenceText | ConvertFrom-Json
    $legacyPlaytestEvidence.playtest = @{
        status='unavailable'; substitute='independent_agents'; legal_limit='not_legal_advice'
    }
    $legacyPlaytestEvidence | ConvertTo-Json -Depth 30 |
        Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath `
        -ArtifactRoot $sandbox -ExpectedConfiguration Debug | Out-Null
    $fabricatedHumanPlaytest = $validEvidenceText | ConvertFrom-Json
    $fabricatedHumanPlaytest.playtest = @{
        status='completed'; participants=5; substitute='none'; gate_status='passed'
        required_before='product_release'; legal_limit='not_legal_advice'
    }
    $fabricatedHumanPlaytest | ConvertTo-Json -Depth 30 |
        Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath `
            -ArtifactRoot $sandbox -ExpectedConfiguration Debug
    } 'human playtest record must remain pending'
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)
    $duplicateMarkerEvidence = $validEvidenceText | ConvertFrom-Json
    $duplicateMarkerEvidence.renderers[0].scales[0].input_feedback_markers[2].preview_hash = '202'
    $duplicateMarkerEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath `
            -ArtifactRoot $sandbox -ExpectedConfiguration Debug
    } 'duplicated causal command or preview hash'
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)
    foreach ($tickMutation in @(
            @{ name='regressive'; observed_tick=3 },
            @{ name='duplicated'; observed_tick=4 }
        )) {
        $tickMutationEvidence = $validEvidenceText | ConvertFrom-Json
        $tickMutationEvidence.renderers[0].scales[0].input_feedback_markers[1].observed_tick = `
            $tickMutation.observed_tick
        $tickMutationEvidence | ConvertTo-Json -Depth 30 |
            Set-Content -LiteralPath $evidencePath -Encoding utf8
        Assert-Throws {
            Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath `
                -ArtifactRoot $sandbox -ExpectedConfiguration Debug
        } 'observed_tick must be strictly increasing'
    }
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)
    $cmakeInputPath = Join-Path $sandbox 'cmake\Dependencies.cmake'
    $cmakeInputOriginal = [IO.File]::ReadAllText($cmakeInputPath)
    [IO.File]::WriteAllText($cmakeInputPath, 'include(FetchContent)`n# stale mutation')
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath `
            -ArtifactRoot $sandbox -ExpectedConfiguration Debug
    } 'tested inputs aggregate SHA-256 mismatch'
    [IO.File]::WriteAllText($cmakeInputPath, $cmakeInputOriginal)
    $claimMutation = $validEvidenceText | ConvertFrom-Json
    $claimMutation.renderers[0].scales[0].frame_p95_ms = 0.001
    $claimMutation | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug } 'differs from capture manifest'
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)

    $manualGoldenEvidence = $validEvidenceText | ConvertFrom-Json
    $manualGoldenManifest = $validCaptureManifestText | ConvertFrom-Json
    $manualGoldenPath = Join-Path $sandbox $manualGoldenEvidence.golden_metadata[0].path
    $manualGoldenOriginal = [IO.File]::ReadAllBytes($manualGoldenPath)
    [IO.File]::WriteAllBytes(
        $manualGoldenPath,
        [Text.Encoding]::UTF8.GetBytes('ninho-fixture-manually-substituted-golden'))
    $manualGoldenHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $manualGoldenPath).Hash.ToLowerInvariant()
    foreach ($document in $manualGoldenEvidence,$manualGoldenManifest) {
        $document.golden_metadata[0].sha256 = $manualGoldenHash
        @($document.source_artifacts | Where-Object path -CEQ $document.golden_metadata[0].path)[0].sha256 = $manualGoldenHash
    }
    $manualGoldenManifest | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $captureManifestPath -Encoding utf8
    $manualGoldenEvidence.capture_manifest.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    Set-NinhoFixtureArtReviewBinding -Evidence $manualGoldenEvidence `
        -CaptureManifest $manualGoldenManifest -ReviewsPath $reviewsPath
    $manualGoldenEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    $manualWrittenEvidence = Get-Content -Raw -LiteralPath $evidencePath | ConvertFrom-Json
    $manualActualCaptureHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    if ($manualWrittenEvidence.capture_manifest.sha256 -cne $manualActualCaptureHash) {
        throw "manual golden fixture capture hash drift: expected=$($manualWrittenEvidence.capture_manifest.sha256) actual=$manualActualCaptureHash"
    }
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug
    } 'differs from raw source frame'
    [IO.File]::WriteAllBytes($manualGoldenPath, $manualGoldenOriginal)
    [IO.File]::WriteAllBytes($captureManifestPath, $validCaptureManifestBytes)
    [IO.File]::WriteAllBytes($reviewsPath, $validReviewsBytes)
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
    Set-NinhoFixtureArtReviewBinding -Evidence $fabricatedStateEvidence `
        -CaptureManifest $fabricatedStateManifest -ReviewsPath $reviewsPath
    $fabricatedStateEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug
    } 'golden state metadata differs from verified Vulkan log: aim'
    [IO.File]::WriteAllBytes($captureManifestPath, $validCaptureManifestBytes)
    [IO.File]::WriteAllBytes($reviewsPath, $validReviewsBytes)
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)

    $fabricatedEventEvidence = $validEvidenceText | ConvertFrom-Json
    $fabricatedEventManifest = $validCaptureManifestText | ConvertFrom-Json
    foreach ($document in $fabricatedEventEvidence,$fabricatedEventManifest) {
        @($document.golden_metadata | Where-Object name -CEQ 'aim')[0].event = 'fabricated'
    }
    $fabricatedEventManifest | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $captureManifestPath -Encoding utf8
    $fabricatedEventEvidence.capture_manifest.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    Set-NinhoFixtureArtReviewBinding -Evidence $fabricatedEventEvidence `
        -CaptureManifest $fabricatedEventManifest -ReviewsPath $reviewsPath
    $fabricatedEventEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug
    } 'golden phase event contract mismatch: aim'
    [IO.File]::WriteAllBytes($captureManifestPath, $validCaptureManifestBytes)
    [IO.File]::WriteAllBytes($reviewsPath, $validReviewsBytes)
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)

    $legacyVirelaEvidence = $validEvidenceText | ConvertFrom-Json
    $legacyVirelaManifest = $validCaptureManifestText | ConvertFrom-Json
    foreach ($document in $legacyVirelaEvidence,$legacyVirelaManifest) {
        @($document.golden_metadata | Where-Object name -CEQ 'virela')[0].event = 'phase_transition'
    }
    $legacyVirelaManifest | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $captureManifestPath -Encoding utf8
    $legacyVirelaEvidence.capture_manifest.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    Set-NinhoFixtureArtReviewBinding -Evidence $legacyVirelaEvidence `
        -CaptureManifest $legacyVirelaManifest -ReviewsPath $reviewsPath
    $legacyVirelaEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug
    } 'Virela golden is not tied to an exact ability/VFX event frame'
    [IO.File]::WriteAllBytes($captureManifestPath, $validCaptureManifestBytes)
    [IO.File]::WriteAllBytes($reviewsPath, $validReviewsBytes)
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)

    $mismatchedVirelaEvidence = $validEvidenceText | ConvertFrom-Json
    $mismatchedVirelaManifest = $validCaptureManifestText | ConvertFrom-Json
    foreach ($document in $mismatchedVirelaEvidence,$mismatchedVirelaManifest) {
        @($document.golden_metadata | Where-Object name -CEQ 'virela')[0].event = 'ability_pulse:virela_pulse'
    }
    $mismatchedVirelaManifest | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $captureManifestPath -Encoding utf8
    $mismatchedVirelaEvidence.capture_manifest.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    Set-NinhoFixtureArtReviewBinding -Evidence $mismatchedVirelaEvidence `
        -CaptureManifest $mismatchedVirelaManifest -ReviewsPath $reviewsPath
    $mismatchedVirelaEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug
    } 'Virela ability metadata differs from verified Vulkan log'
    [IO.File]::WriteAllBytes($captureManifestPath, $validCaptureManifestBytes)
    [IO.File]::WriteAllBytes($reviewsPath, $validReviewsBytes)
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)

    $reviewsOriginalBytes = [IO.File]::ReadAllBytes($reviewsPath)
    $reviewsOriginal = [IO.File]::ReadAllText($reviewsPath)
    $reviewsMutation = $reviewsOriginal | ConvertFrom-Json
    $reviewsMutation.reviews[0].verdict = 'changes_required'
    $reviewsMutation | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reviewsPath -Encoding utf8
    $reviewEvidenceMutation = $validEvidenceText | ConvertFrom-Json
    $reviewEvidenceMutation.reviews_manifest.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $reviewsPath).Hash.ToLowerInvariant()
    $reviewEvidenceMutation | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug } 'blocking or missing agent review'
    [IO.File]::WriteAllBytes($reviewsPath, $reviewsOriginalBytes)
    [IO.File]::WriteAllText($evidencePath, $validEvidenceText)

    $packageRoot = Join-Path $sandbox 'package'
    New-Item -ItemType Directory -Force -Path $packageRoot | Out-Null
    foreach ($name in @(
        'NinhoOrbital.exe','NinhoOrbital.pck','ninho_physics.windows.template_release.x86_64.dll',
        'package-source-inventory.json','build-contract.json','licenses/THIRD_PARTY_NOTICES.md','licenses/sbom.spdx.json',
        'licenses/box3d.LICENSE.txt','licenses/godot.LICENSE.txt','licenses/godot.COPYRIGHT.txt',
        'licenses/godot-export-templates.LICENSE.txt','licenses/godot-cpp.LICENSE.txt',
        'licenses/nlohmann-json.LICENSE.txt')) {
        $directory = Split-Path -Parent (Join-Path $packageRoot $name)
        if ($directory) { New-Item -ItemType Directory -Force -Path $directory | Out-Null }
        [IO.File]::WriteAllText((Join-Path $packageRoot $name), $name)
    }
    $buildContractPath = Join-Path $packageRoot 'build-contract.json'
    [ordered]@{
        schema='ninho.native-build-contract.v1';configuration='Release';build_testing=$false
        test_facades=$false;NINHO_ENABLE_TEST_FACADES='absent'
    } | ConvertTo-Json -Compress | Set-Content -LiteralPath $buildContractPath -Encoding utf8
    $buildContractHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $buildContractPath).Hash.ToLowerInvariant()
    [ordered]@{
        schema='ninho.package-source-inventory.v1';schema_version=1
        note='Working-tree source inputs supplied to export; this does not enumerate NinhoOrbital.pck contents. PCK SHA-256 and packaged runtime smoke validate the resulting package.'
        native_build_contract=@{
            path='build-contract.json';sha256=$buildContractHash;build_testing=$false;test_facades=$false
        };resources=@()
    } | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $packageRoot 'package-source-inventory.json') -Encoding utf8
    $packageManifest = Join-Path $packageRoot 'manifest.sha256.json'
    [string[]]$packageNames = @(
        'NinhoOrbital.exe','NinhoOrbital.pck','ninho_physics.windows.template_release.x86_64.dll',
        'package-source-inventory.json','build-contract.json','licenses/THIRD_PARTY_NOTICES.md','licenses/sbom.spdx.json',
        'licenses/box3d.LICENSE.txt','licenses/godot.LICENSE.txt','licenses/godot.COPYRIGHT.txt',
        'licenses/godot-export-templates.LICENSE.txt','licenses/godot-cpp.LICENSE.txt',
        'licenses/nlohmann-json.LICENSE.txt')
    [Array]::Sort($packageNames, [StringComparer]::Ordinal)
    $packageRows = @($packageNames | ForEach-Object {
        $file = Join-Path $packageRoot $_
        [ordered]@{
            path=$_
            role=if ($_ -ceq 'package-source-inventory.json') { 'source_inventory' } else { 'fixture' }
            size_bytes=[int64](Get-Item $file).Length
            sha256=(Get-FileHash $file -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    })
    $packageDllHash = ($packageRows | Where-Object path -CEQ 'ninho_physics.windows.template_release.x86_64.dll').sha256
    [ordered]@{
        schema_version=1;algorithm='SHA-256';configuration='Release';files=$packageRows
        runtime_extension=@{path='ninho_physics.windows.template_release.x86_64.dll';source_path='game/bin/ninho_physics.windows.template_release.x86_64.dll';source_sha256=$packageDllHash}
        native_build_contract=@{path='build-contract.json';sha256=$buildContractHash;build_testing=$false;test_facades=$false}
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
    $releaseCaptureManifestHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $captureManifestPath).Hash.ToLowerInvariant()
    $releaseEvidence.capture_manifest.sha256 = $releaseCaptureManifestHash
    $releaseReviews = Get-Content -Raw -LiteralPath $reviewsPath | ConvertFrom-Json
    $releaseArtReview = @($releaseReviews.reviews | Where-Object role -CEQ 'art')[0]
    $releaseArtReview.artifact_bindings = @($releaseArtReview.artifact_bindings) + @([pscustomobject]@{
            schema='ninho.vertical-slice.art-review-binding.v1'
            capture_manifest_sha256=$releaseCaptureManifestHash
            capture_configuration='Release'
            goldens=@($releaseEvidence.golden_metadata | ForEach-Object {
                [pscustomobject]@{name=$_.name;sha256=$_.sha256}
            })
        })
    $releaseReviews | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $reviewsPath -Encoding utf8
    $releaseReviewsHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $reviewsPath).Hash.ToLowerInvariant()
    $releaseEvidence.reviews_manifest.sha256 = $releaseReviewsHash
    $evidence.reviews_manifest.sha256 = $releaseReviewsHash
    $releaseEvidence | Add-Member -NotePropertyName package -NotePropertyValue @{
        path='package'; manifest_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $packageManifest).Hash.ToLowerInvariant()
        launch_from_space_path='passed'
    }
    $releaseEvidence | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Release
    $validReleaseEvidenceText = [IO.File]::ReadAllText($evidencePath)
    $validPackageManifestText = [IO.File]::ReadAllText($packageManifest)
    $validPackageManifestBytes = [IO.File]::ReadAllBytes($packageManifest)
    $sourceInventoryPath = Join-Path $packageRoot 'package-source-inventory.json'
    $validSourceInventoryBytes = [IO.File]::ReadAllBytes($sourceInventoryPath)

    $legacySchemaInventory = Get-Content -Raw -LiteralPath $sourceInventoryPath | ConvertFrom-Json
    $legacySchemaInventory.schema = 'ninho.package-content.v1'
    $legacySchemaInventory | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $sourceInventoryPath -Encoding utf8
    $legacySchemaManifest = $validPackageManifestText | ConvertFrom-Json
    $legacySchemaRow = @($legacySchemaManifest.files | Where-Object path -CEQ 'package-source-inventory.json')[0]
    $legacySchemaRow.size_bytes = [int64](Get-Item -LiteralPath $sourceInventoryPath).Length
    $legacySchemaRow.sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $sourceInventoryPath).Hash.ToLowerInvariant()
    $legacySchemaManifest | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $packageManifest -Encoding utf8
    $legacySchemaEvidence = $validReleaseEvidenceText | ConvertFrom-Json
    $legacySchemaEvidence.package.manifest_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $packageManifest).Hash.ToLowerInvariant()
    $legacySchemaEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Release
    } 'source inventory contract/provenance mismatch'

    [IO.File]::WriteAllBytes($sourceInventoryPath, $validSourceInventoryBytes)
    $legacyRoleManifest = $validPackageManifestText | ConvertFrom-Json
    @($legacyRoleManifest.files | Where-Object path -CEQ 'package-source-inventory.json')[0].role = 'content_inventory'
    $legacyRoleManifest | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $packageManifest -Encoding utf8
    $legacyRoleEvidence = $validReleaseEvidenceText | ConvertFrom-Json
    $legacyRoleEvidence.package.manifest_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $packageManifest).Hash.ToLowerInvariant()
    $legacyRoleEvidence | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws {
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Release
    } 'source inventory contract/provenance mismatch'
    [IO.File]::WriteAllBytes($packageManifest, $validPackageManifestBytes)
    [IO.File]::WriteAllText($evidencePath, $validReleaseEvidenceText)

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
        Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Release
    } 'missing required distribution file'
    [IO.File]::WriteAllBytes($missingLicensePath, $missingLicenseBytes)
    [IO.File]::WriteAllBytes($packageManifest, $validPackageManifestBytes)
    [IO.File]::WriteAllText($evidencePath, $validReleaseEvidenceText)
    [IO.File]::AppendAllText((Join-Path $packageRoot 'NinhoOrbital.pck'), 'tamper')
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Release } 'package size mismatch'
    [IO.File]::WriteAllText((Join-Path $packageRoot 'NinhoOrbital.pck'), 'NinhoOrbital.pck')
    [IO.File]::AppendAllText($packageManifest, 'tamper')
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Release } 'package manifest SHA-256'
    $evidence | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8

    Remove-Item -LiteralPath $artifact
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug } 'missing'
    [IO.File]::WriteAllBytes($artifact, $artifactOriginal)

    $doc = Get-Content -Raw -LiteralPath $evidencePath | ConvertFrom-Json
    $doc.commit = 'ffffffffffffffffffffffffffffffffffffffff'
    $doc | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug } 'tested-content identity header mismatch'

    $doc.source_revision = $doc.commit
    $doc | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug } 'source revision is not an ancestor of the current HEAD'

    $evidence | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    $doc = Get-Content -Raw -LiteralPath $evidencePath | ConvertFrom-Json
    $doc.renderers = @($doc.renderers | Where-Object name -CEQ 'Vulkan')
    $doc | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug } 'renderer'

    $evidence | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    [IO.File]::WriteAllBytes($artifact, [byte[]](9,9,9))
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug } 'SHA-256'

    [IO.File]::WriteAllBytes($artifact, $artifactOriginal)
    [IO.File]::WriteAllText($log, "SCRIPT ERROR: contamination`n")
    $doc = Get-Content -Raw -LiteralPath $evidencePath | ConvertFrom-Json
    $newLogHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $log).Hash.ToLowerInvariant()
    $doc.source_artifacts[1].sha256 = $newLogHash
    foreach ($renderer in $doc.renderers) { $renderer.capture.log_hash = $newLogHash }
    $doc | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $evidencePath -Encoding utf8
    Assert-Throws { Assert-NinhoVerticalSliceEvidence -EvidencePath $evidencePath -ArtifactRoot $sandbox -ExpectedConfiguration Debug } 'forbidden diagnostic'
} finally {
    if (Test-Path -LiteralPath $sandbox) { Remove-Item -LiteralPath $sandbox -Recurse -Force }
}

Write-Output 'vertical slice gate tests: PASS'
