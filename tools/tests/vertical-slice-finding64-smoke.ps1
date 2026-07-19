[CmdletBinding()]
param(
    [string]$Root = ''
)

$ErrorActionPreference = 'Stop'
if (-not $Root) {
    $Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
}

Import-Module (Join-Path $Root 'tools\VerticalSliceGate.psm1') -Force
Import-Module (Join-Path $Root 'tools\SafePath.psm1') -Force

$godot = Join-Path $Root '.tools\godot\Godot_v4.5.1-stable_win64.exe'
if (-not (Test-Path -LiteralPath $godot -PathType Leaf)) {
    throw "Pinned Godot executable missing: $godot"
}

$artifactsRoot = Join-Path $Root 'artifacts'
$runToken = 'ninho-finding64-' + [Guid]::NewGuid().ToString('N')
$fixtureRoot = Join-Path $artifactsRoot $runToken
Assert-NinhoNoReparseAncestors -Path $fixtureRoot -AllowedRoot $artifactsRoot |
    Out-Null

try {
    New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
    $stdoutPath = Join-Path $fixtureRoot 'godot.stdout.log'
    $stderrPath = Join-Path $fixtureRoot 'godot.stderr.log'

    $process = Invoke-NinhoTimedProcess `
        -FilePath $godot `
        -ArgumentList @(
            '--headless',
            '--path', 'game',
            '--fixed-fps', '60',
            '--script', 'res://tests/vertical_slice_smoke.gd',
            '--', '--finding64-only', "--finding64-run-token=$runToken"
        ) `
        -TimeoutMs 120000 `
        -StdoutPath $stdoutPath `
        -StderrPath $stderrPath `
        -WorkingDirectory $Root `
        -ProcessIdentityToken $runToken `
        -FatalMarker 'NINHO_GODOT_FATAL name=finding64-smoke reason=timeout'

    if ($process.ExitCode -ne 0) {
        throw "Finding 64 smoke failed with exit code $($process.ExitCode)"
    }
    $log = [IO.File]::ReadAllText($stdoutPath) + "`n" +
        [IO.File]::ReadAllText($stderrPath)
    foreach ($forbidden in 'ERROR:', 'SCRIPT ERROR:', 'WARNING:') {
        if ($log.Contains($forbidden)) {
            throw "Finding 64 smoke emitted $forbidden"
        }
    }
    $marker = 'AIM_COALESCING_SMOKE_OK'
    $markerCount = [regex]::Matches($log, [regex]::Escape($marker)).Count
    if ($markerCount -ne 1) {
        throw "Finding 64 smoke expected one completion marker, got $markerCount"
    }

    Write-Output 'Vertical slice finding 64 smoke: PASS'
}
finally {
    if (Test-Path -LiteralPath $fixtureRoot) {
        Assert-NinhoNoReparseAncestors -Path $fixtureRoot -AllowedRoot $artifactsRoot |
            Out-Null
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}
