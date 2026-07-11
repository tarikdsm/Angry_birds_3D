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
if ($json.visual_studio.version -ne '18.7.3') { throw 'Visual Studio lock mismatch' }
if ($json.python.minimum_version -ne '3.11.0') { throw 'Python requirement mismatch' }
Write-Output 'bootstrap lock: PASS'
