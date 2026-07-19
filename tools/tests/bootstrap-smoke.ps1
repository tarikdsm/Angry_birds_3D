$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$bootstrap = Join-Path $root 'tools\bootstrap.ps1'
if (-not (Test-Path -LiteralPath $bootstrap)) {
    throw "bootstrap.ps1 is missing"
}
$json = & $bootstrap -ValidateLock -Json | ConvertFrom-Json
if (-not $json.ok) { throw ($json.errors -join '; ') }
if ($json.cmake.version -ne '4.3.3') { throw 'CMake lock mismatch' }
if ($json.ninja.version -ne '1.13.2') { throw 'Ninja lock mismatch' }
if ($json.godot.version -ne '4.5.1-stable') { throw 'Godot lock mismatch' }
if ($json.blender.executable_sha256 -ne 'a7d09b04df8f78d432bc45d32c08f25387a78c76d12d2a6f5de07d8e1066e8f8') {
    throw 'Blender portable executable lock mismatch'
}
if ($json.ffmpeg.version -ne '8.1.2') { throw 'FFmpeg lock mismatch' }
if (-not $json.ffprobe.path.EndsWith('ffprobe.exe', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'FFprobe lock path mismatch'
}
if ($json.visual_studio.version -ne '18.7.3') { throw 'Visual Studio lock mismatch' }
if ($json.python.minimum_version -ne '3.11.0') { throw 'Python requirement mismatch' }
Write-Output 'bootstrap lock: PASS'
