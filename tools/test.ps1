[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$preset = $Configuration.ToLowerInvariant()
$artifactDirectory = Join-Path $root 'artifacts\physics'
$godot = Join-Path $root '.tools\godot\Godot_v4.5.1-stable_win64.exe'
$godotImportCache = Join-Path $root 'game\.godot'
New-Item -ItemType Directory -Force -Path $artifactDirectory | Out-Null
Import-Module (Join-Path $PSScriptRoot 'GodotSpikeGate.psm1') -Force

# Validate the shipped integration contract before creating the deterministic
# runtime cache used by CI and by a developer before opening the editor.
& (Join-Path $PSScriptRoot 'tests\godot-spike-gate-tests.ps1') -Root $root
$descriptorPath = Join-Path $root 'game\bin\ninho_physics.gdextension'
$descriptor = Read-NinhoGDExtensionDescriptor -Path $descriptorPath

if (Test-Path -LiteralPath $godotImportCache) {
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

if (-not (Test-Path -LiteralPath $godot -PathType Leaf)) {
    [Console]::Error.WriteLine("Pinned Godot executable not found: $godot")
    exit 1
}

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
        [string]$ExpectedMoviePath = ''
    )

    $stdout = Join-Path $artifactDirectory "$Name-$preset.stdout.log"
    $stderr = Join-Path $artifactDirectory "$Name-$preset.stderr.log"
    if ($ExpectedMoviePath) {
        $expectedMovieFullPath = [System.IO.Path]::GetFullPath($ExpectedMoviePath)
        $artifactRoot = [System.IO.Path]::GetFullPath($artifactDirectory).TrimEnd('\', '/') +
            [System.IO.Path]::DirectorySeparatorChar
        if (-not $expectedMovieFullPath.StartsWith(
                $artifactRoot,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to manage movie outside the artifact directory: $expectedMovieFullPath"
        }
        if (Test-Path -LiteralPath $expectedMovieFullPath -PathType Leaf) {
            Remove-Item -LiteralPath $expectedMovieFullPath -Force
        }
    }
    $process = Start-Process `
        -FilePath $godot `
        -ArgumentList $GodotArguments `
        -WorkingDirectory $root `
        -WindowStyle Hidden `
        -Wait `
        -PassThru `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr

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
    if ($ExpectedMoviePath -and
            (-not (Test-Path -LiteralPath $expectedMovieFullPath -PathType Leaf) -or
            (Get-Item -LiteralPath $expectedMovieFullPath).Length -le 0)) {
        [Console]::Error.WriteLine("Godot smoke '$Name' did not create a non-empty movie")
        return 1
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
    -ExpectedMoviePath $mobileMoviePath `
    -GodotArguments @(
    '--path', 'game',
    '--write-movie', "../artifacts/physics/godot-scene-$preset.avi",
    '--quit-after', '15'
)
if ($godotExitCode -ne 0) {
    exit $godotExitCode
}

$compatibilityMoviePath = Join-Path $artifactDirectory "godot-scene-gl-$preset.avi"
$godotExitCode = Invoke-GodotSmoke `
    -Name 'godot-scene-gl' `
    -RequiredLogText 'Compatibility' `
    -ExpectedMoviePath $compatibilityMoviePath `
    -GodotArguments @(
    '--rendering-method', 'gl_compatibility',
    '--path', 'game',
    '--write-movie', "../artifacts/physics/godot-scene-gl-$preset.avi",
    '--quit-after', '15'
)
if ($godotExitCode -ne 0) {
    exit $godotExitCode
}

exit 0
