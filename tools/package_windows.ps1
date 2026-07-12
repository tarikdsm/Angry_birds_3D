[CmdletBinding()]
param(
    [ValidateSet('Release')][string]$Configuration = 'Release',
    [string]$OutputRoot,
    [switch]$VerifyOnly
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$artifactsRoot = Join-Path $root 'artifacts'
$gameRoot = Join-Path $root 'game'
$lockPath = Join-Path $PSScriptRoot 'toolchain.lock.json'
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force

if (-not ('Ninho.NativeProcessExit' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
namespace Ninho {
    public static class NativeProcessExit {
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool GetExitCodeProcess(IntPtr process, out uint exitCode);
    }
}
'@
}

New-Item -ItemType Directory -Force -Path $artifactsRoot | Out-Null
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $artifactsRoot 'package\windows-release'
} elseif (-not [IO.Path]::IsPathRooted($OutputRoot)) {
    $OutputRoot = Join-Path $root $OutputRoot
}
$OutputRoot = [IO.Path]::GetFullPath($OutputRoot)
Assert-NinhoNoReparseAncestors -Path $OutputRoot -AllowedRoot $artifactsRoot | Out-Null

function Write-CanonicalJson {
    param([Parameter(Mandatory)]$Value, [Parameter(Mandatory)][string]$Path)
    $json = $Value | ConvertTo-Json -Depth 12 -Compress
    [IO.File]::WriteAllText($Path, $json + "`n", [Text.UTF8Encoding]::new($false))
}

function Get-OrdinalSortedStrings {
    param([object[]]$Values)
    [string[]]$items = @($Values | ForEach-Object { [string]$_ })
    [Array]::Sort($items, [StringComparer]::Ordinal)
    return $items
}

function Get-PackageRole {
    param([string]$RelativePath)
    if ($RelativePath -ceq 'NinhoOrbital.exe' -or $RelativePath -ceq 'NinhoOrbital.console.exe') { return 'game_executable' }
    if ($RelativePath -ceq 'NinhoOrbital.pck') { return 'game_data' }
    if ($RelativePath -ceq 'package-content.json') { return 'content_inventory' }
    if ($RelativePath -ceq 'ninho_physics.windows.template_release.x86_64.dll') { return 'native_extension' }
    if ($RelativePath -ceq 'licenses/sbom.spdx.json') { return 'sbom' }
    if ($RelativePath -ceq 'licenses/THIRD_PARTY_NOTICES.md') { return 'notice' }
    if ($RelativePath.StartsWith('licenses/', [StringComparison]::Ordinal)) { return 'license' }
    return 'support'
}

function Test-ManifestRelativePath {
    param([string]$RelativePath)
    if ([string]::IsNullOrWhiteSpace($RelativePath) -or
            [IO.Path]::IsPathRooted($RelativePath) -or
            $RelativePath.Contains(':')) {
        return $false
    }
    foreach ($segment in $RelativePath.Replace('\', '/').Split('/')) {
        if ($segment -ceq '..' -or $segment -ceq '.' -or [string]::IsNullOrEmpty($segment)) {
            return $false
        }
    }
    return $true
}

function Assert-WindowsPackage {
    param([Parameter(Mandatory)][string]$PackageRoot)
    Assert-NinhoNoReparseAncestors -Path $PackageRoot -AllowedRoot $artifactsRoot | Out-Null
    if (-not (Test-Path -LiteralPath $PackageRoot -PathType Container)) {
        throw "Package directory does not exist: $PackageRoot"
    }
    $manifestPath = Join-Path $PackageRoot 'manifest.sha256.json'
    Assert-NinhoNoReparseAncestors -Path $manifestPath -AllowedRoot $PackageRoot | Out-Null
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw "Package manifest does not exist: $manifestPath"
    }
    $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    if ($manifest.schema_version -ne 1 -or $manifest.algorithm -cne 'SHA-256' -or
            $manifest.configuration -cne 'Release') {
        throw 'Package manifest header mismatch'
    }
    $lock = Get-Content -Raw -LiteralPath $lockPath | ConvertFrom-Json
    if ($manifest.export_templates.version -cne $lock.godot_export_templates.version -or
            $manifest.export_templates.archive_sha256 -cne $lock.godot_export_templates.sha256) {
        throw 'Package manifest export-template pin mismatch'
    }
    $releaseDllName = 'ninho_physics.windows.template_release.x86_64.dll'
    if ($manifest.runtime_extension.path -cne $releaseDllName -or
            $manifest.runtime_extension.source_path -cne "game/bin/$releaseDllName" -or
            [string]$manifest.runtime_extension.source_sha256 -notmatch '^[0-9a-f]{64}$') {
        throw 'Package manifest runtime-extension snapshot mismatch'
    }

    $manifestPaths = [Collections.Generic.List[string]]::new()
    foreach ($entry in @($manifest.files)) {
        $relative = [string]$entry.path
        if (-not (Test-ManifestRelativePath $relative)) {
            throw "Package manifest contains unsafe path: $relative"
        }
        $nativeRelative = $relative.Replace('/', [IO.Path]::DirectorySeparatorChar)
        $file = Join-Path $PackageRoot $nativeRelative
        Assert-NinhoNoReparseAncestors -Path $file -AllowedRoot $PackageRoot | Out-Null
        if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
            throw "Package manifest references missing file: $relative"
        }
        $actualSize = (Get-Item -LiteralPath $file).Length
        if ($actualSize -ne [long]$entry.size_bytes) {
            throw "Package size mismatch: $relative"
        }
        $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $file).Hash.ToLowerInvariant()
        if ($actualHash -cne [string]$entry.sha256) {
            throw "Package hash mismatch: $relative"
        }
        if ((Get-PackageRole $relative) -cne [string]$entry.role) {
            throw "Package role mismatch: $relative"
        }
        $manifestPaths.Add($relative)
    }
    $expectedOrder = @(Get-OrdinalSortedStrings @($manifestPaths))
    if (($manifestPaths -join "`n") -cne ($expectedOrder -join "`n")) {
        throw 'Package manifest is not canonically sorted'
    }

    $actualPaths = @(
        Get-ChildItem -LiteralPath $PackageRoot -Recurse -File | ForEach-Object {
            $relative = $_.FullName.Substring($PackageRoot.Length).TrimStart('\', '/').Replace('\', '/')
            if ($relative -cne 'manifest.sha256.json') { $relative }
        }
    )
    $actualPaths = @(Get-OrdinalSortedStrings $actualPaths)
    if (($actualPaths -join "`n") -cne ($expectedOrder -join "`n")) {
        throw 'Package file set differs from the canonical manifest'
    }
    $runtimeRows = @($manifest.files | Where-Object {
        $_.path -ceq $releaseDllName -and $_.role -ceq 'native_extension'
    })
    if ($runtimeRows.Count -ne 1 -or
            [string]$runtimeRows[0].sha256 -cne [string]$manifest.runtime_extension.source_sha256) {
        throw 'Package runtime DLL differs from the recorded post-build snapshot'
    }

    foreach ($required in @(
        'NinhoOrbital.exe',
        'NinhoOrbital.pck',
        'ninho_physics.windows.template_release.x86_64.dll',
        'package-content.json',
        'licenses/THIRD_PARTY_NOTICES.md',
        'licenses/sbom.spdx.json',
        'licenses/godot.COPYRIGHT.txt',
        'licenses/godot-export-templates.LICENSE.txt'
    )) {
        if (-not ($manifestPaths -ccontains $required)) { throw "Required package file missing: $required" }
    }
    if ($manifestPaths -ccontains 'ninho_physics.windows.template_debug.x86_64.dll') {
        throw 'Debug GDExtension is forbidden in the Release package'
    }
    Write-Output "Windows package verification: PASS ($($manifestPaths.Count) files)"
}

if ($VerifyOnly) {
    Assert-WindowsPackage -PackageRoot $OutputRoot
    return
}

Write-Verbose 'Building the Release GDExtension'
$quotedBuildScript = '"' + (Join-Path $PSScriptRoot 'build.ps1') + '"'
$buildProcess = Start-Process -FilePath 'powershell' -ArgumentList @(
    '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $quotedBuildScript,
    '-Configuration', 'Release', '-WithGodot'
) -Wait -PassThru -WindowStyle Hidden
if ($buildProcess.ExitCode -ne 0) {
    throw "Release GDExtension build failed with exit code $($buildProcess.ExitCode)"
}
Write-Verbose 'Release GDExtension build completed'
$releaseDllName = 'ninho_physics.windows.template_release.x86_64.dll'
$releaseDll = Join-Path $gameRoot "bin\$releaseDllName"
Assert-NinhoNoReparseAncestors -Path $releaseDll -AllowedRoot $root | Out-Null
if (-not (Test-Path -LiteralPath $releaseDll -PathType Leaf)) {
    throw "Release GDExtension is missing after build: $releaseDll"
}
$releaseDllSnapshotHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $releaseDll).Hash.ToLowerInvariant()

$bootstrapStdout = Join-Path $artifactsRoot ('package-bootstrap-' + [Guid]::NewGuid().ToString('N') + '.stdout.log')
$bootstrapStderr = Join-Path $artifactsRoot ('package-bootstrap-' + [Guid]::NewGuid().ToString('N') + '.stderr.log')
Assert-NinhoNoReparseAncestors -Path $bootstrapStdout -AllowedRoot $artifactsRoot | Out-Null
Assert-NinhoNoReparseAncestors -Path $bootstrapStderr -AllowedRoot $artifactsRoot | Out-Null
Write-Verbose 'Installing the pinned Godot export templates'
$quotedBootstrapScript = '"' + (Join-Path $PSScriptRoot 'bootstrap.ps1') + '"'
$bootstrapProcess = Start-Process -FilePath 'powershell' -ArgumentList @(
    '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $quotedBootstrapScript,
    '-InstallExportTemplates', '-Json'
) -Wait -PassThru -WindowStyle Hidden -RedirectStandardOutput $bootstrapStdout -RedirectStandardError $bootstrapStderr
try {
    if ($bootstrapProcess.ExitCode -ne 0) {
        $diagnostic = ((Get-Content -Raw -LiteralPath $bootstrapStdout -ErrorAction SilentlyContinue) +
            (Get-Content -Raw -LiteralPath $bootstrapStderr -ErrorAction SilentlyContinue)).Trim()
        throw "Export-template bootstrap failed with exit code $($bootstrapProcess.ExitCode): $diagnostic"
    }
    Write-Verbose 'Pinned Godot export templates installed'
}
finally {
    foreach ($log in @($bootstrapStdout, $bootstrapStderr)) {
        if (Test-Path -LiteralPath $log) {
            Assert-NinhoNoReparseAncestors -Path $log -AllowedRoot $artifactsRoot | Out-Null
            Remove-Item -LiteralPath $log -Force
        }
    }
}

$lock = Get-Content -Raw -LiteralPath $lockPath | ConvertFrom-Json
$godot = Join-Path $root ('.tools\godot\' + $lock.godot.exe)
$templateDirectory = Join-Path $root ('.tools\godot\editor_data\export_templates\' + $lock.godot_export_templates.install_directory)
foreach ($requiredPath in @($godot, (Join-Path $templateDirectory 'windows_release_x86_64.exe'))) {
    Assert-NinhoNoReparseAncestors -Path $requiredPath -AllowedRoot $root | Out-Null
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "Pinned package input missing: $requiredPath"
    }
}
$stagingRoot = Join-Path $artifactsRoot ('.package-staging-' + [Guid]::NewGuid().ToString('N'))
$exportStdout = Join-Path $artifactsRoot ('package-export-' + [Guid]::NewGuid().ToString('N') + '.stdout.log')
$exportStderr = Join-Path $artifactsRoot ('package-export-' + [Guid]::NewGuid().ToString('N') + '.stderr.log')
$launchStdout = Join-Path $artifactsRoot ('package-launch-' + [Guid]::NewGuid().ToString('N') + '.stdout.log')
$launchStderr = Join-Path $artifactsRoot ('package-launch-' + [Guid]::NewGuid().ToString('N') + '.stderr.log')
Assert-NinhoNoReparseAncestors -Path $stagingRoot -AllowedRoot $artifactsRoot | Out-Null
Assert-NinhoNoReparseAncestors -Path $exportStdout -AllowedRoot $artifactsRoot | Out-Null
Assert-NinhoNoReparseAncestors -Path $exportStderr -AllowedRoot $artifactsRoot | Out-Null
Assert-NinhoNoReparseAncestors -Path $launchStdout -AllowedRoot $artifactsRoot | Out-Null
Assert-NinhoNoReparseAncestors -Path $launchStderr -AllowedRoot $artifactsRoot | Out-Null

try {
    New-Item -ItemType Directory -Path $stagingRoot | Out-Null
    $exportExecutable = Join-Path $stagingRoot 'NinhoOrbital.exe'
    $exportProcess = Start-Process -FilePath $godot -WorkingDirectory $gameRoot -ArgumentList @(
        '--headless', '--path', ('"' + $gameRoot + '"'),
        '--export-release', '"Windows Desktop"', ('"' + $exportExecutable + '"')
    ) -Wait -PassThru -WindowStyle Hidden -RedirectStandardOutput $exportStdout -RedirectStandardError $exportStderr
    $exportExitCode = $exportProcess.ExitCode
    $exportOutput = ((Get-Content -Raw -LiteralPath $exportStdout -ErrorAction SilentlyContinue) +
        "`n" + (Get-Content -Raw -LiteralPath $exportStderr -ErrorAction SilentlyContinue)).Trim()
    if ($exportExitCode -ne 0) {
        throw "Godot Windows export failed with exit code ${exportExitCode}: $exportOutput"
    }
    if (($exportOutput -join "`n") -match '(?im)^\s*(SCRIPT ERROR|ERROR|WARNING):') {
        throw "Godot Windows export emitted an error: $exportOutput"
    }

    foreach ($requiredExport in 'NinhoOrbital.exe','NinhoOrbital.pck') {
        $requiredExportPath = Join-Path $stagingRoot $requiredExport
        Assert-NinhoNoReparseAncestors -Path $requiredExportPath -AllowedRoot $stagingRoot | Out-Null
        if (-not (Test-Path -LiteralPath $requiredExportPath -PathType Leaf) -or
                (Get-Item -LiteralPath $requiredExportPath).Length -eq 0) {
            throw "Godot export omitted required output: $requiredExport"
        }
    }

    $packagedDll = Join-Path $stagingRoot $releaseDllName
    Assert-NinhoNoReparseAncestors -Path $packagedDll -AllowedRoot $stagingRoot | Out-Null
    if (Test-Path -LiteralPath $packagedDll) {
        if ((Get-FileHash -Algorithm SHA256 -LiteralPath $packagedDll).Hash.ToLowerInvariant() -cne
                $releaseDllSnapshotHash) {
            throw 'Godot exported a GDExtension DLL that differs from the post-build snapshot'
        }
    } else {
        throw 'Godot export omitted the runtime GDExtension DLL'
    }

    $licenseDirectory = Join-Path $stagingRoot 'licenses'
    Assert-NinhoNoReparseAncestors -Path $licenseDirectory -AllowedRoot $stagingRoot | Out-Null
    New-Item -ItemType Directory -Path $licenseDirectory | Out-Null
    $licenseSources = [ordered]@{
        'THIRD_PARTY_NOTICES.md' = (Join-Path $root 'THIRD_PARTY_NOTICES.md')
        'sbom.spdx.json' = (Join-Path $root 'third_party\sbom.spdx.json')
        'box3d.LICENSE.txt' = (Join-Path $root 'third_party\box3d.LICENSE.txt')
        'godot.LICENSE.txt' = (Join-Path $root 'third_party\godot.LICENSE.txt')
        'godot.COPYRIGHT.txt' = (Join-Path $root 'third_party\godot.COPYRIGHT.txt')
        'godot-export-templates.LICENSE.txt' = (Join-Path $root 'third_party\godot-export-templates.LICENSE.txt')
        'godot-cpp.LICENSE.txt' = (Join-Path $root 'third_party\godot-cpp.LICENSE.txt')
        'nlohmann-json.LICENSE.txt' = (Join-Path $root 'third_party\nlohmann-json.LICENSE.txt')
    }
    foreach ($name in $licenseSources.Keys) {
        $source = $licenseSources[$name]
        $target = Join-Path $licenseDirectory $name
        Assert-NinhoNoReparseAncestors -Path $source -AllowedRoot $root | Out-Null
        Assert-NinhoNoReparseAncestors -Path $target -AllowedRoot $stagingRoot | Out-Null
        Copy-Item -LiteralPath $source -Destination $target
    }

    $resourceRows = [Collections.Generic.List[object]]::new()
    foreach ($category in @(
        [ordered]@{ name = 'data'; root = (Join-Path $gameRoot 'data'); pattern = '*' },
        [ordered]@{ name = 'asset'; root = (Join-Path $gameRoot 'assets\vertical_slice'); pattern = '*' },
        [ordered]@{ name = 'audio'; root = (Join-Path $gameRoot 'assets\audio\generated'); pattern = '*.wav' }
    )) {
        Assert-NinhoNoReparseAncestors -Path $category.root -AllowedRoot $gameRoot | Out-Null
        foreach ($file in Get-ChildItem -LiteralPath $category.root -Recurse -File -Filter $category.pattern) {
            if ($file.Name.EndsWith('.import', [StringComparison]::OrdinalIgnoreCase) -or
                    $file.Name.EndsWith('.uid', [StringComparison]::OrdinalIgnoreCase)) { continue }
            Assert-NinhoNoReparseAncestors -Path $file.FullName -AllowedRoot $category.root | Out-Null
            $resourcePath = 'res://' + $file.FullName.Substring($gameRoot.Length).TrimStart('\', '/').Replace('\', '/')
            $resourceRows.Add([ordered]@{
                path = $resourcePath
                category = $category.name
                size_bytes = [long]$file.Length
                sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToLowerInvariant()
            })
        }
    }
    $resourceByPath = @{}
    foreach ($row in $resourceRows) { $resourceByPath[[string]$row.path] = $row }
    $resourcePaths = @(Get-OrdinalSortedStrings @($resourceByPath.Keys))
    $canonicalResources = @($resourcePaths | ForEach-Object { $resourceByPath[$_] })
    if (@($canonicalResources | Where-Object category -ceq 'data').Count -eq 0 -or
            @($canonicalResources | Where-Object category -ceq 'asset').Count -eq 0 -or
            @($canonicalResources | Where-Object category -ceq 'audio').Count -eq 0) {
        throw 'Package source inventory must contain data, visual assets, and audio'
    }
    Write-CanonicalJson -Value ([ordered]@{
        schema_version = 1
        note = 'These source resources are embedded in NinhoOrbital.pck; packaged launch validates their runtime load.'
        resources = $canonicalResources
    }) -Path (Join-Path $stagingRoot 'package-content.json')

    $fileByPath = @{}
    foreach ($file in Get-ChildItem -LiteralPath $stagingRoot -Recurse -File) {
        $relative = $file.FullName.Substring($stagingRoot.Length).TrimStart('\', '/').Replace('\', '/')
        if ($relative -ceq 'manifest.sha256.json') { continue }
        $fileByPath[$relative] = [ordered]@{
            path = $relative
            role = Get-PackageRole $relative
            size_bytes = [long]$file.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToLowerInvariant()
        }
    }
    $filePaths = @(Get-OrdinalSortedStrings @($fileByPath.Keys))
    $canonicalFiles = @($filePaths | ForEach-Object { $fileByPath[$_] })
    Write-CanonicalJson -Value ([ordered]@{
        schema_version = 1
        algorithm = 'SHA-256'
        configuration = 'Release'
        export_templates = [ordered]@{
            version = $lock.godot_export_templates.version
            url = $lock.godot_export_templates.url
            archive_sha256 = $lock.godot_export_templates.sha256
        }
        runtime_extension = [ordered]@{
            path = $releaseDllName
            source_path = "game/bin/$releaseDllName"
            source_sha256 = $releaseDllSnapshotHash
        }
        files = $canonicalFiles
    }) -Path (Join-Path $stagingRoot 'manifest.sha256.json')

    Assert-WindowsPackage -PackageRoot $stagingRoot
    if (Test-Path -LiteralPath $OutputRoot) {
        Assert-NinhoNoReparseAncestors -Path $OutputRoot -AllowedRoot $artifactsRoot | Out-Null
        Remove-Item -LiteralPath $OutputRoot -Recurse -Force
    }
    $outputParent = Split-Path -Parent $OutputRoot
    New-Item -ItemType Directory -Force -Path $outputParent | Out-Null
    Move-Item -LiteralPath $stagingRoot -Destination $OutputRoot
    Assert-WindowsPackage -PackageRoot $OutputRoot

    $launchExe = Join-Path $OutputRoot 'NinhoOrbital.exe'
    $launchProcess = Start-Process -FilePath $launchExe -WorkingDirectory $OutputRoot -ArgumentList @(
        '--headless', '--rendering-method', 'gl_compatibility',
        '--script', 'res://tests/vertical_slice_smoke.gd'
    ) -PassThru -WindowStyle Hidden -RedirectStandardOutput $launchStdout -RedirectStandardError $launchStderr
    $launchHandle = $launchProcess.Handle
    if ($null -eq $launchHandle -or $launchHandle -eq [IntPtr]::Zero) {
        throw 'Could not retain packaged process handle before waiting'
    }
    if (-not $launchProcess.WaitForExit(120000)) {
        $launchProcess.Kill()
        throw 'Packaged vertical slice smoke timed out after 120 seconds'
    }
    [uint32]$launchExitCode = 259
    if (-not [Ninho.NativeProcessExit]::GetExitCodeProcess($launchHandle, [ref]$launchExitCode)) {
        $nativeError = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw "Could not query packaged process exit code (Win32 $nativeError)"
    }
    if ($launchExitCode -eq 259) {
        throw 'Packaged process still reports STILL_ACTIVE after WaitForExit'
    }
    $stdout = Get-Content -Raw -LiteralPath $launchStdout -ErrorAction SilentlyContinue
    $stderr = Get-Content -Raw -LiteralPath $launchStderr -ErrorAction SilentlyContinue
    $launchLog = $stdout + "`n" + $stderr
    if ($launchExitCode -ne 0) {
        throw "Packaged vertical slice smoke failed with exit code ${launchExitCode}: $launchLog"
    }
    if ($launchLog -notmatch 'VERTICAL_SLICE_SMOKE_OK') {
        throw "Packaged vertical slice smoke marker is missing: $launchLog"
    }
    if ($launchLog -match '(?im)^\s*(SCRIPT ERROR|ERROR|WARNING):') {
        throw "Packaged vertical slice smoke emitted an error: $launchLog"
    }
    Write-Output "Windows package launch: PASS ($OutputRoot)"
}
finally {
    if ($null -ne $launchProcess) {
        $launchProcess.Dispose()
    }
    if (Test-Path -LiteralPath $stagingRoot) {
        Assert-NinhoNoReparseAncestors -Path $stagingRoot -AllowedRoot $artifactsRoot | Out-Null
        Remove-Item -LiteralPath $stagingRoot -Recurse -Force
    }
    foreach ($log in @($exportStdout, $exportStderr, $launchStdout, $launchStderr)) {
        if (Test-Path -LiteralPath $log) {
            Assert-NinhoNoReparseAncestors -Path $log -AllowedRoot $artifactsRoot | Out-Null
            Remove-Item -LiteralPath $log -Force
        }
    }
}

$packageManifestPath = Join-Path $OutputRoot 'manifest.sha256.json'
Assert-NinhoNoReparseAncestors -Path $packageManifestPath -AllowedRoot $OutputRoot | Out-Null
if (-not (Test-Path -LiteralPath $packageManifestPath -PathType Leaf)) {
    throw "Built package manifest is missing: $packageManifestPath"
}
Write-Output ('NINHO_PACKAGE_MANIFEST ' +
    (@{ path=[IO.Path]::GetFullPath($packageManifestPath) } | ConvertTo-Json -Compress))
