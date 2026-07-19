$ErrorActionPreference = 'Stop'

$toolsDirectory = Split-Path -Parent $PSScriptRoot
$root = (Resolve-Path (Join-Path $toolsDirectory '..')).Path

Import-Module (Join-Path $toolsDirectory 'ToolchainIntegrity.psm1') -Force -Scope Local
Import-Module (Join-Path $toolsDirectory 'SafePath.psm1') -Force -Scope Local

if ($null -eq (Get-Command Resolve-NinhoPinnedToolchainExecutable -ErrorAction SilentlyContinue)) {
    throw 'Resolve-NinhoPinnedToolchainExecutable was not imported into the parent script scope.'
}

& (Join-Path $PSScriptRoot 'toolchain-integrity-tests.ps1')

Import-Module (Join-Path $toolsDirectory 'ToolchainIntegrity.psm1') -Force -Scope Local
if ($null -eq (Get-Command Resolve-NinhoPinnedToolchainExecutable -ErrorAction SilentlyContinue)) {
    throw 'Resolve-NinhoPinnedToolchainExecutable was not restored at the parent use site.'
}

$resolved = ToolchainIntegrity\Resolve-NinhoPinnedToolchainExecutable `
    -Root $root `
    -ToolName 'ffmpeg' `
    -ExecutableProperty 'ffprobe_exe' `
    -HashProperty 'ffprobe_exe_sha256' `
    -Name 'FFprobe'

if ([string]::IsNullOrWhiteSpace($resolved)) {
    throw 'The parent scope resolver returned an empty executable path.'
}

Write-Output 'toolchain parent scope regression: PASS'
