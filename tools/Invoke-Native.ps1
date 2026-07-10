[CmdletBinding()]
param([Parameter(Mandatory)][string]$Command)
$ErrorActionPreference = 'Stop'
$bootstrap = Join-Path $PSScriptRoot 'bootstrap.ps1'
$check = & $bootstrap -CheckOnly -Json | ConvertFrom-Json
if (-not $check.ok) { throw ($check.errors -join '; ') }
$vs = $check.visual_studio.path
if (-not $vs) { throw 'Locked Visual Studio 18.7.3 was not found; run tools/bootstrap.ps1 -InstallVisualStudio' }
$devCmd = Join-Path $vs 'Common7\Tools\VsDevCmd.bat'
$environment = & cmd.exe /s /c "`"$devCmd`" -no_logo -arch=x64 -host_arch=x64 -vcvars_ver=14.44 -winsdk=10.0.26100.0 && set"
foreach ($line in $environment) {
    $split = $line.IndexOf('=')
    if ($split -gt 0) { Set-Item -Path "Env:$($line.Substring(0,$split))" -Value $line.Substring($split+1) }
}
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$cmakeBin = Split-Path (Join-Path $root '.tools\cmake\cmake-4.3.3-windows-x86_64\bin\cmake.exe')
$ninjaBin = Split-Path (Join-Path $root '.tools\ninja\ninja.exe')
$env:PATH = "$cmakeBin;$ninjaBin;$env:PATH"
& powershell -NoProfile -ExecutionPolicy Bypass -Command $Command
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
