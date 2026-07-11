[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [switch]$IncludeUpstream
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$preset = $Configuration.ToLowerInvariant()
$artifactDirectory = Join-Path $root 'artifacts\physics'
$foundationReport = Join-Path $root 'docs\physics\box3d-spike-report.md'
$godot = Join-Path $root '.tools\godot\Godot_v4.5.1-stable_win64.exe'
$godotImportCache = Join-Path $root 'game\.godot'
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force
Assert-NinhoNoReparseAncestors -Path $artifactDirectory -AllowedRoot $root | Out-Null
New-Item -ItemType Directory -Force -Path $artifactDirectory | Out-Null
Import-Module (Join-Path $PSScriptRoot 'GodotSpikeGate.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'UpstreamBox3DGate.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'FoundationEvidenceGate.psm1') -Force
$foundationReportTests = Join-Path $PSScriptRoot `
    'tests\test_generate_foundation_report.py'
& python $foundationReportTests
$foundationReportTestExitCode = $LASTEXITCODE
if ($foundationReportTestExitCode -ne 0) {
    exit $foundationReportTestExitCode
}
& (Join-Path $PSScriptRoot 'tests\spike-report-gate-tests.ps1') -Root $root
& (Join-Path $PSScriptRoot 'tests\upstream-box3d-gate-tests.ps1') -Root $root
& (Join-Path $PSScriptRoot 'tests\foundation-evidence-gate-tests.ps1') -Root $root

Assert-NinhoNoReparseAncestors -Path $foundationReport -AllowedRoot $root | Out-Null
if (-not (Test-Path -LiteralPath $foundationReport -PathType Leaf)) {
    [Console]::Error.WriteLine('box3d-spike-report.md missing')
    exit 1
}

$reportText = [System.IO.File]::ReadAllText($foundationReport)
$requiredReportHeadings = @(
    'Versions',
    'Capability Matrix',
    'Scenario Metrics',
    'Godot Smoke',
    'Known Limits',
    'Recommendation'
)
foreach ($heading in $requiredReportHeadings) {
    if ($reportText -notmatch "(?m)^## $([regex]::Escape($heading))\s*$") {
        [Console]::Error.WriteLine("box3d-spike-report.md missing heading: $heading")
        exit 1
    }
}

$recommendationMatch = [regex]::Match(
    $reportText,
    '(?m)^Recommendation:\s*(prosseguir|prosseguir_com_limites|bloquear)\s*$')
if (-not $recommendationMatch.Success) {
    [Console]::Error.WriteLine('box3d-spike-report.md has invalid recommendation')
    exit 1
}
Assert-NinhoFoundationEvidence -Root $root -ReportPath $foundationReport

# Validate the shipped integration contract before creating the deterministic
# runtime cache used by CI and by a developer before opening the editor.
& (Join-Path $PSScriptRoot 'tests\godot-spike-gate-tests.ps1') -Root $root
$descriptorPath = Join-Path $root 'game\bin\ninho_physics.gdextension'
Assert-NinhoNoReparseAncestors -Path $descriptorPath -AllowedRoot $root | Out-Null
$descriptor = Read-NinhoGDExtensionDescriptor -Path $descriptorPath

Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force
Assert-NinhoNoReparseAncestors -Path $godotImportCache -AllowedRoot $root | Out-Null
if (Test-Path -LiteralPath $godotImportCache) {
    Assert-NinhoNoReparseAncestors `
        -Path $godotImportCache `
        -AllowedRoot $root | Out-Null
    $resolvedCache = (Resolve-Path -LiteralPath $godotImportCache).Path
    $expectedCache = [System.IO.Path]::GetFullPath($godotImportCache)
    if (-not [string]::Equals(
            $resolvedCache,
            $expectedCache,
            [System.StringComparison]::OrdinalIgnoreCase)) {
        [Console]::Error.WriteLine("Refusing to remove unexpected Godot cache: $resolvedCache")
        exit 1
    }
    Remove-Item -LiteralPath $resolvedCache -Recurse -Force
}

$manifest = New-NinhoTestGDExtensionManifest `
    -Descriptor $descriptor `
    -Configuration $Configuration `
    -CacheDirectory $godotImportCache

& (Join-Path $PSScriptRoot 'build.ps1') `
    -Configuration $Configuration `
    -WithGodot
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$gameDirectory = Join-Path $root 'game'
$selectedLibraryRelative = $manifest.SelectedLibrary.Substring('res://'.Length).Replace('/', '\')
$selectedLibraryPath = Join-Path $gameDirectory $selectedLibraryRelative
Assert-NinhoNoReparseAncestors -Path $selectedLibraryPath -AllowedRoot $root | Out-Null
if (-not (Test-Path -LiteralPath $selectedLibraryPath -PathType Leaf)) {
    [Console]::Error.WriteLine(
        "Validated $Configuration GDExtension library was not built: $selectedLibraryPath")
    exit 1
}

$ctestCommand = "ctest --preset $preset --output-on-failure"
& (Join-Path $PSScriptRoot 'Invoke-Native.ps1') -Command $ctestCommand
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if ($IncludeUpstream) {
    $expectedBox3DCommit = '8441b4a06d6d09dcfb0b0f704df4d847d1437b92'
    $fetchContentRoot = Join-Path $root '.fetchcontent-cache'
    Assert-NinhoNoReparseAncestors -Path $fetchContentRoot -AllowedRoot $root | Out-Null
    $box3DSource = $null
    $candidateCommits = [System.Collections.Generic.List[string]]::new()
    foreach ($candidate in @(Get-ChildItem -LiteralPath $fetchContentRoot -Directory |
            Where-Object Name -Like 'box3d-src*')) {
        Assert-NinhoNoReparseAncestors -Path $candidate.FullName -AllowedRoot $root | Out-Null
        $candidatePath = $candidate.FullName.Replace('\', '/')
        $commit = & git -c "safe.directory=$candidatePath" -C $candidate.FullName `
            rev-parse HEAD 2>$null
        if ($LASTEXITCODE -ne 0) {
            continue
        }
        $commit = "$commit".Trim().ToLowerInvariant()
        $candidateCommits.Add("$($candidate.FullName)=$commit")
        if ($commit -eq $expectedBox3DCommit) {
            if ($null -ne $box3DSource) {
                [Console]::Error.WriteLine('Multiple exact Box3D source checkouts found')
                exit 1
            }
            $box3DSource = $candidate.FullName
        }
    }
    if ($null -eq $box3DSource) {
        [Console]::Error.WriteLine(
            "Exact Box3D source commit not found: $expectedBox3DCommit; candidates: " +
            ($candidateCommits -join '; '))
        exit 1
    }

    $box3DSafePath = $box3DSource.Replace('\', '/')
    Assert-NinhoNoReparseAncestors -Path $box3DSource -AllowedRoot $root | Out-Null
    $box3DChanges = & git -c "safe.directory=$box3DSafePath" -C $box3DSource `
        status --porcelain
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    if (@($box3DChanges).Count -ne 0) {
        [Console]::Error.WriteLine('Pinned Box3D source checkout is modified')
        exit 1
    }

    $upstreamRoot = [System.IO.Path]::GetFullPath(
        (Join-Path $root 'build\upstream-box3d'))
    $upstreamBuild = [System.IO.Path]::GetFullPath(
        (Join-Path $upstreamRoot $preset))
    Assert-NinhoNoReparseAncestors -Path $upstreamRoot -AllowedRoot $root | Out-Null
    Assert-NinhoNoReparseAncestors -Path $upstreamBuild -AllowedRoot $root | Out-Null
    Reset-NinhoUpstreamBuildDirectory `
        -Path $upstreamBuild `
        -AllowedRoot $upstreamRoot
    $ninja = Join-Path $root '.tools\ninja\ninja.exe'
    $upstreamTest = Join-Path $upstreamBuild 'bin\test.exe'
    $runtime = if ($Configuration -eq 'Debug') {
        'MultiThreadedDebug'
    } else {
        'MultiThreaded'
    }
    $upstreamCommand = @(
        'ctest --build-and-test',
        "'$($box3DSource.Replace('\', '/'))'",
        "'$($upstreamBuild.Replace('\', '/'))'",
        '--build-generator Ninja',
        "--build-makeprogram '$($ninja.Replace('\', '/'))'",
        "--build-config $Configuration",
        '--build-options',
        "'-DCMAKE_BUILD_TYPE=$Configuration'",
        "'-DCMAKE_C_COMPILER=cl'",
        "'-DCMAKE_CXX_COMPILER=cl'",
        "'-DCMAKE_SYSTEM_VERSION=10.0.26100.0'",
        "'-DCMAKE_MSVC_RUNTIME_LIBRARY=$runtime'",
        "'-DBUILD_SHARED_LIBS=OFF'",
        "'-DBOX3D_SAMPLES=OFF'",
        "'-DBOX3D_BENCHMARKS=OFF'",
        "'-DBOX3D_DOCS=OFF'",
        "'-DBOX3D_UNIT_TESTS=ON'",
        "'-DBOX3D_BUILD_SHADERS=OFF'",
        '--build-target test',
        "--test-command '$($upstreamTest.Replace('\', '/'))'"
    ) -join ' '
    $upstreamLog = Join-Path $artifactDirectory "upstream-box3d-$preset.log"
    $upstreamOutput = & (Join-Path $PSScriptRoot 'Invoke-Native.ps1') `
        -Command $upstreamCommand 2>&1
    $upstreamExitCode = $LASTEXITCODE
    Assert-NinhoNoReparseAncestors `
        -Path $upstreamLog `
        -AllowedRoot $artifactDirectory | Out-Null
    $upstreamOutput | Tee-Object -FilePath $upstreamLog
    if ($upstreamExitCode -ne 0) {
        exit $upstreamExitCode
    }

    $upstreamText = $upstreamOutput -join "`n"
    $upstreamPassed = [regex]::Matches($upstreamText, '(?m)^test passed:').Count
    if ($upstreamPassed -ne 20 -or $upstreamText -match '(?m)^test failed:') {
        [Console]::Error.WriteLine(
            "Unexpected Box3D upstream test result: $upstreamPassed/20 passed")
        exit 1
    }

    $cachePath = Join-Path $upstreamBuild 'CMakeCache.txt'
    Assert-NinhoNoReparseAncestors -Path $cachePath -AllowedRoot $root | Out-Null
    $cacheText = [System.IO.File]::ReadAllText($cachePath)
    $expectedCacheEntries = @(
        "CMAKE_BUILD_TYPE:STRING=$Configuration",
        'CMAKE_GENERATOR:INTERNAL=Ninja',
        'CMAKE_SYSTEM_VERSION:UNINITIALIZED=10.0.26100.0',
        'BUILD_SHARED_LIBS:UNINITIALIZED=OFF',
        'BOX3D_SAMPLES:BOOL=OFF',
        'BOX3D_BENCHMARKS:BOOL=OFF',
        'BOX3D_DOCS:BOOL=OFF',
        'BOX3D_UNIT_TESTS:BOOL=ON',
        'BOX3D_BUILD_SHADERS:BOOL=OFF',
        "CMAKE_MSVC_RUNTIME_LIBRARY:UNINITIALIZED=$runtime"
    )
    foreach ($entry in $expectedCacheEntries) {
        if (-not $cacheText.Contains($entry)) {
            [Console]::Error.WriteLine("Box3D upstream cache mismatch: $entry")
            exit 1
        }
    }

        Assert-NinhoUpstreamCompileDatabase `
            -Path (Join-Path $upstreamBuild 'compile_commands.json') `
            -Configuration $Configuration `
            -ManifestPath (Join-Path $root 'tools\box3d-v0.1.0-compile-sources.txt') `
            -AllowedRoot $root
    [Console]::Out.WriteLine(
        "Upstream Box3D $expectedBox3DCommit ($Configuration): 20/20 passed")
}

Assert-NinhoNoReparseAncestors -Path $godot -AllowedRoot $root | Out-Null
if (-not (Test-Path -LiteralPath $godot -PathType Leaf)) {
    [Console]::Error.WriteLine("Pinned Godot executable not found: $godot")
    exit 1
}
$ffprobeCommand = Get-Command 'ffprobe.exe' -ErrorAction SilentlyContinue
if ($null -eq $ffprobeCommand) {
    [Console]::Error.WriteLine('ffprobe.exe is required to validate Godot movie captures')
    exit 1
}
$ffprobe = $ffprobeCommand.Source
$visualCompletionMarker = 'NINHO_VISUAL_CAPTURE_COMPLETE frame=300'

function Assert-GodotLogIsClean {
    param(
        [Parameter(Mandatory)] [string]$StandardOutput,
        [Parameter(Mandatory)] [string]$StandardError
    )

    $standardOutputText = [System.IO.File]::ReadAllText($StandardOutput)
    $standardErrorText = [System.IO.File]::ReadAllText($StandardError)
    [Console]::Out.Write($standardOutputText)
    if ($standardErrorText.Length -gt 0) {
        [Console]::Error.Write($standardErrorText)
    }
    $log = $standardOutputText + "`n" + $standardErrorText
    $forbiddenLogPatterns = @(
        'ERROR:',
        'SCRIPT ERROR:',
        'WARNING:',
        'Cannot open dynamic library',
        "Can't open dynamic library",
        'Failed to load extension',
        'Could not load extension',
        'No GDExtension library found'
    )
    foreach ($pattern in $forbiddenLogPatterns) {
        if ($log -match [regex]::Escape($pattern)) {
            [Console]::Error.WriteLine("Godot log contains forbidden text: $pattern")
            return $false
        }
    }
    return $true
}

function Invoke-GodotSmoke {
    param(
        [Parameter(Mandatory)] [string]$Name,
        [Parameter(Mandatory)] [string[]]$GodotArguments,
        [string]$RequiredLogText = '',
        [string]$RequiredCompletionMarker = '',
        [string]$ExpectedMoviePath = ''
    )

    $stdout = Join-Path $artifactDirectory "$Name-$preset.stdout.log"
    $stderr = Join-Path $artifactDirectory "$Name-$preset.stderr.log"
    Assert-NinhoNoReparseAncestors -Path $stdout -AllowedRoot $artifactDirectory | Out-Null
    Assert-NinhoNoReparseAncestors -Path $stderr -AllowedRoot $artifactDirectory | Out-Null
    $movieCaptureStartedUtc = $null
    if ($ExpectedMoviePath) {
        $expectedMovieFullPath = [System.IO.Path]::GetFullPath($ExpectedMoviePath)
        $artifactRoot = [System.IO.Path]::GetFullPath($artifactDirectory).TrimEnd('\', '/') +
            [System.IO.Path]::DirectorySeparatorChar
        if (-not $expectedMovieFullPath.StartsWith(
                $artifactRoot,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to manage movie outside the artifact directory: $expectedMovieFullPath"
        }
        Assert-NinhoNoReparseAncestors `
            -Path $expectedMovieFullPath `
            -AllowedRoot $artifactDirectory | Out-Null
        if (Test-Path -LiteralPath $expectedMovieFullPath -PathType Leaf) {
            Remove-Item -LiteralPath $expectedMovieFullPath -Force
        }
        $movieCaptureStartedUtc = [DateTime]::UtcNow
    }
    Assert-NinhoNoReparseAncestors -Path $godot -AllowedRoot $root | Out-Null
    $process = Start-Process `
        -FilePath $godot `
        -ArgumentList $GodotArguments `
        -WorkingDirectory $root `
        -WindowStyle Hidden `
        -Wait `
        -PassThru `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr

    # Godot is an external process and may have replaced either redirected
    # output path while it was running. Revalidate immediately before reads.
    Assert-NinhoNoReparseAncestors -Path $stdout -AllowedRoot $artifactDirectory | Out-Null
    Assert-NinhoNoReparseAncestors -Path $stderr -AllowedRoot $artifactDirectory | Out-Null
    if ($process.ExitCode -ne 0) {
        [Console]::Out.Write([System.IO.File]::ReadAllText($stdout))
        [Console]::Error.Write([System.IO.File]::ReadAllText($stderr))
        return [int]$process.ExitCode
    }
    if (-not (Assert-GodotLogIsClean $stdout $stderr)) {
        [Console]::Error.WriteLine("Godot smoke '$Name' emitted a forbidden diagnostic")
        return 1
    }
    if ($RequiredLogText) {
        $log = (Get-Content -Raw -LiteralPath $stdout) + "`n" +
            (Get-Content -Raw -LiteralPath $stderr)
        if (-not $log.Contains($RequiredLogText)) {
            [Console]::Error.WriteLine(
                "Godot smoke '$Name' did not initialize the required renderer: $RequiredLogText")
            return 1
        }
    }
    if ($RequiredCompletionMarker) {
        $log = (Get-Content -Raw -LiteralPath $stdout) + "`n" +
            (Get-Content -Raw -LiteralPath $stderr)
        $markerCount = [regex]::Matches(
            $log,
            [regex]::Escape($RequiredCompletionMarker)).Count
        if ($markerCount -ne 1) {
            [Console]::Error.WriteLine(
                "Godot smoke '$Name' expected one frame completion marker, got $markerCount")
            return 1
        }
    }
    if ($ExpectedMoviePath) {
        try {
            $metadata = Assert-NinhoGodotMovieCapture `
                -Path $expectedMovieFullPath `
                -AllowedRoot $artifactDirectory `
                -StartedUtc $movieCaptureStartedUtc `
                -FfprobePath $ffprobe
            [Console]::Out.WriteLine(
                "Godot movie validated: $($metadata.Codec) $($metadata.Width)x$($metadata.Height), " +
                "$($metadata.FrameCount) frames, $($metadata.DurationSeconds) seconds")
        } catch {
            [Console]::Error.WriteLine($_.Exception.Message)
            return 1
        }
    }
    return 0
}

$godotExitCode = Invoke-GodotSmoke -Name 'godot-smoke' -GodotArguments @(
    '--headless',
    '--path', 'game',
    '--script', 'res://scripts/physics_spike_smoke.gd'
)
if ($godotExitCode -ne 0) {
    exit $godotExitCode
}

$mobileMoviePath = Join-Path $artifactDirectory "godot-scene-$preset.avi"
$godotExitCode = Invoke-GodotSmoke `
    -Name 'godot-scene' `
    -RequiredLogText 'Forward Mobile' `
    -RequiredCompletionMarker $visualCompletionMarker `
    -ExpectedMoviePath $mobileMoviePath `
    -GodotArguments @(
    '--path', 'game',
    '--write-movie', "../artifacts/physics/godot-scene-$preset.avi",
    '--fixed-fps', '60',
    '--', '--ninho-capture-300'
)
if ($godotExitCode -ne 0) {
    exit $godotExitCode
}

$compatibilityMoviePath = Join-Path $artifactDirectory "godot-scene-gl-$preset.avi"
$godotExitCode = Invoke-GodotSmoke `
    -Name 'godot-scene-gl' `
    -RequiredLogText 'Compatibility' `
    -RequiredCompletionMarker $visualCompletionMarker `
    -ExpectedMoviePath $compatibilityMoviePath `
    -GodotArguments @(
    '--rendering-method', 'gl_compatibility',
    '--path', 'game',
    '--write-movie', "../artifacts/physics/godot-scene-gl-$preset.avi",
    '--fixed-fps', '60',
    '--', '--ninho-capture-300'
)
if ($godotExitCode -ne 0) {
    exit $godotExitCode
}

exit 0
