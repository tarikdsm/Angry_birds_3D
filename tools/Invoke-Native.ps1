[CmdletBinding(DefaultParameterSetName = 'Command')]
param(
    [Parameter(Mandatory, ParameterSetName = 'Command')]
    [string]$Command,

    [Parameter(Mandatory, ParameterSetName = 'Operation')]
    [scriptblock]$Operation,

    [Parameter(ParameterSetName = 'Operation')]
    [object[]]$ArgumentList = @()
)
$ErrorActionPreference = 'Stop'
$bootstrap = Join-Path $PSScriptRoot 'bootstrap.ps1'
$check = & $bootstrap -CheckOnly -Json | ConvertFrom-Json
if (-not $check.ok) { throw ($check.errors -join '; ') }
$vs = $check.visual_studio.path
if (-not $vs) { throw 'Locked Visual Studio 18.7.3 was not found; run tools/bootstrap.ps1 -InstallVisualStudio' }
$cmake = [string]$check.cmake.path
$ninja = [string]$check.ninja.path
$godot = [string]$check.godot.path
$devCmd = Join-Path $vs 'Common7\Tools\VsDevCmd.bat'
$environment = & cmd.exe /s /c "`"$devCmd`" -no_logo -arch=x64 -host_arch=x64 -vcvars_ver=14.44 -winsdk=10.0.26100.0 && set"
$devCmdExitCode = $LASTEXITCODE
if ($devCmdExitCode -ne 0) {
    [Console]::Error.WriteLine("VsDevCmd failed with exit code ${devCmdExitCode}: $devCmd")
    exit $devCmdExitCode
}
foreach ($lockedTool in @(
        @{ Name = 'CMake'; Path = $cmake },
        @{ Name = 'Ninja'; Path = $ninja },
        @{ Name = 'Godot'; Path = $godot }
    )) {
    if ([string]::IsNullOrWhiteSpace($lockedTool.Path) -or
            -not [IO.Path]::IsPathRooted($lockedTool.Path) -or
            -not (Test-Path -LiteralPath $lockedTool.Path -PathType Leaf)) {
        throw "Bootstrap returned an invalid locked $($lockedTool.Name) path: $($lockedTool.Path)"
    }
}
foreach ($line in $environment) {
    $split = $line.IndexOf('=')
    if ($split -gt 0) { Set-Item -Path "Env:$($line.Substring(0,$split))" -Value $line.Substring($split+1) }
}
$cmakeBin = Split-Path -Parent $cmake
$ninjaBin = Split-Path -Parent $ninja
$env:PATH = "$cmakeBin;$ninjaBin;$env:PATH"
$env:NINHO_GODOT_EXECUTABLE = $godot
if ($PSCmdlet.ParameterSetName -ceq 'Operation') {
    & $Operation @ArgumentList
} else {
    & powershell -NoProfile -ExecutionPolicy Bypass -Command $Command
}
$operationExitCode = $LASTEXITCODE
if ($operationExitCode -ne 0) { exit $operationExitCode }
