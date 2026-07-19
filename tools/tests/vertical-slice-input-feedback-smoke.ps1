[CmdletBinding()]
param(
    [string]$Root = ''
)

$ErrorActionPreference = 'Stop'
if (-not $Root) {
    $Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
}

function Get-RelativePath {
    param(
        [Parameter(Mandatory)][string]$Base,
        [Parameter(Mandatory)][string]$Target
    )

    $basePath = [IO.Path]::GetFullPath($Base).TrimEnd('\', '/') +
        [IO.Path]::DirectorySeparatorChar
    return [Uri]::UnescapeDataString(
        ([Uri]$basePath).MakeRelativeUri([Uri][IO.Path]::GetFullPath($Target)).ToString())
}

Import-Module (Join-Path $Root 'tools\VerticalSliceGate.psm1') -Force
Import-Module (Join-Path $Root 'tools\SafePath.psm1') -Force

$godot = Join-Path $Root '.tools\godot\Godot_v4.5.1-stable_win64.exe'
if (-not (Test-Path -LiteralPath $godot -PathType Leaf)) {
    throw "Pinned Godot executable missing: $godot"
}

$artifactsRoot = Join-Path $Root 'artifacts'
$fixtureRoot = Join-Path $artifactsRoot `
    ('vertical-slice-input-feedback-' + [Guid]::NewGuid().ToString('N'))
Assert-NinhoNoReparseAncestors -Path $fixtureRoot -AllowedRoot $artifactsRoot |
    Out-Null

try {
    New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
    $metricsPath = Join-Path $fixtureRoot 'metrics-opengl-150.json'
    $stdoutPath = Join-Path $fixtureRoot 'godot.stdout.log'
    $stderrPath = Join-Path $fixtureRoot 'godot.stderr.log'
    $metricsRelative = (Get-RelativePath `
        -Base (Join-Path $Root 'game') -Target $metricsPath).Replace('\', '/')

    $process = Invoke-NinhoTimedProcess `
        -FilePath $godot `
        -ArgumentList @(
            '--rendering-method', 'gl_compatibility',
            '--path', 'game',
            '--resolution', '1920x1080',
            'res://scenes/vertical_slice.tscn',
            '--',
            '--vertical-slice-capture',
            '--ui-scale=150',
            "--vertical-slice-metrics=res://$metricsRelative"
        ) `
        -TimeoutMs 120000 `
        -StdoutPath $stdoutPath `
        -StderrPath $stderrPath `
        -WorkingDirectory $Root `
        -FatalMarker 'NINHO_CAPTURE_FATAL name=input-feedback-smoke reason=timeout'

    if ($process.ExitCode -ne 0) {
        throw "OpenGL/150 input-feedback smoke failed with exit code $($process.ExitCode)"
    }
    $log = [IO.File]::ReadAllText($stdoutPath) + "`n" +
        [IO.File]::ReadAllText($stderrPath)
    foreach ($forbidden in 'ERROR:', 'SCRIPT ERROR:', 'WARNING:') {
        if ($log.Contains($forbidden)) {
            throw "OpenGL/150 input-feedback smoke emitted $forbidden"
        }
    }
    if (-not $log.Contains('VERTICAL_SLICE_CAPTURE_COMPLETE frame=300')) {
        throw 'OpenGL/150 input-feedback smoke did not complete'
    }
    if (-not (Test-Path -LiteralPath $metricsPath -PathType Leaf)) {
        throw 'OpenGL/150 input-feedback metrics were not written'
    }

    $metrics = Get-Content -Raw -LiteralPath $metricsPath | ConvertFrom-Json
    if ($metrics.schema -cne 'ninho.vertical-slice.runtime-metrics.v1') {
        throw 'OpenGL/150 input-feedback metrics schema mismatch'
    }
    if ([int]$metrics.input_feedback_samples -ne 3 -or
            @($metrics.input_feedback_markers).Count -ne 3) {
        throw 'OpenGL/150 input-feedback smoke did not observe three causal samples'
    }
    if ([double]$metrics.input_feedback_p95_ms -gt 33.4) {
        throw "OpenGL/150 input-feedback p95 exceeded 33.4 ms: $($metrics.input_feedback_p95_ms) ms"
    }

    Write-Output (
        'Vertical slice OpenGL/150 input-feedback smoke: PASS ' +
        "p95=$($metrics.input_feedback_p95_ms)ms samples=$($metrics.input_feedback_samples)")
}
finally {
    if (Test-Path -LiteralPath $fixtureRoot) {
        Assert-NinhoNoReparseAncestors -Path $fixtureRoot -AllowedRoot $artifactsRoot |
            Out-Null
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}
