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

$nativeTarget = if ($Configuration -eq 'Debug') { 'template_debug' } else { 'template_release' }
$extensionPath = Join-Path $godotImportCache 'ninho_physics.test.gdextension'
$extensionListPath = Join-Path $godotImportCache 'extension_list.cfg'
New-Item -ItemType Directory -Force -Path $godotImportCache | Out-Null
$utf8WithoutBom = [System.Text.UTF8Encoding]::new($false)
$extensionDescriptor = @"
[configuration]
entry_symbol = "ninho_physics_library_init"
compatibility_minimum = "4.5"
reloadable = false

[libraries]
windows.debug.x86_64 = "res://bin/ninho_physics.windows.$nativeTarget.x86_64.dll"
"@
[System.IO.File]::WriteAllText($extensionPath, $extensionDescriptor, $utf8WithoutBom)
[System.IO.File]::WriteAllText(
    $extensionListPath,
    "res://.godot/ninho_physics.test.gdextension`n",
    $utf8WithoutBom)

& (Join-Path $PSScriptRoot 'build.ps1') `
    -Configuration $Configuration `
    -WithGodot
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
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

    Get-Content -LiteralPath $StandardOutput
    if ((Get-Item -LiteralPath $StandardError).Length -gt 0) {
        [Console]::Error.Write((Get-Content -Raw -LiteralPath $StandardError))
    }
    $log = (Get-Content -Raw -LiteralPath $StandardOutput) + "`n" +
        (Get-Content -Raw -LiteralPath $StandardError)
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

$stdout = Join-Path $artifactDirectory "godot-smoke-$preset.stdout.log"
$stderr = Join-Path $artifactDirectory "godot-smoke-$preset.stderr.log"
$arguments = @(
    '--headless',
    '--path', 'game',
    '--script', 'res://scripts/physics_spike_smoke.gd'
)
$process = Start-Process `
    -FilePath $godot `
    -ArgumentList $arguments `
    -WorkingDirectory $root `
    -WindowStyle Hidden `
    -Wait `
    -PassThru `
    -RedirectStandardOutput $stdout `
    -RedirectStandardError $stderr

if ($process.ExitCode -ne 0) {
    Get-Content -LiteralPath $stdout
    [Console]::Error.Write((Get-Content -Raw -LiteralPath $stderr))
    exit $process.ExitCode
}
if (-not (Assert-GodotLogIsClean $stdout $stderr)) {
    exit 1
}

exit 0
