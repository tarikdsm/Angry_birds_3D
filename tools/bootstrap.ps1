[CmdletBinding(DefaultParameterSetName='Check')]
param(
    [Parameter(ParameterSetName='Validate')][switch]$ValidateLock,
    [Parameter(ParameterSetName='Check')][switch]$CheckOnly,
    [Parameter(ParameterSetName='Portable')][switch]$InstallPortable,
    [Parameter(ParameterSetName='Templates')][switch]$InstallExportTemplates,
    [Parameter(ParameterSetName='VisualStudio')][switch]$InstallVisualStudio,
    [switch]$Json
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$tools = Join-Path $root '.tools'
$downloads = Join-Path $tools 'downloads'
$lockPath = Join-Path $PSScriptRoot 'toolchain.lock.json'
$errors = [System.Collections.Generic.List[string]]::new()
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force
Assert-NinhoNoReparseAncestors -Path $lockPath -AllowedRoot $PSScriptRoot | Out-Null
$lock = Get-Content -Raw -LiteralPath $lockPath | ConvertFrom-Json

function Test-HexSha([string]$value) { return $value -match '^[0-9a-f]{64}$' }
foreach ($name in 'cmake','ninja','godot','blender','ffmpeg','visual_studio') {
    $entry = $lock.$name
    if (-not $entry.version) { $errors.Add("$name.version missing") }
    if ($entry.url -notmatch '^https://') { $errors.Add("$name.url must use https") }
    if (-not (Test-HexSha $entry.sha256)) { $errors.Add("$name.sha256 invalid") }
    if ($name -ne 'visual_studio' -and -not (Test-HexSha $entry.exe_sha256)) { $errors.Add("$name.exe_sha256 invalid") }
}
$ffmpegLock = $lock.ffmpeg
if (-not $ffmpegLock.ffprobe_exe) { $errors.Add('ffmpeg.ffprobe_exe missing') }
if (-not (Test-HexSha $ffmpegLock.ffprobe_exe_sha256)) {
    $errors.Add('ffmpeg.ffprobe_exe_sha256 invalid')
}
$templateLock = $lock.godot_export_templates
if (-not $templateLock.version) { $errors.Add('godot_export_templates.version missing') }
if ($templateLock.url -notmatch '^https://') { $errors.Add('godot_export_templates.url must use https') }
if (-not (Test-HexSha $templateLock.sha256)) { $errors.Add('godot_export_templates.sha256 invalid') }
if ($templateLock.install_directory -notmatch '^4\.5\.1\.stable$') {
    $errors.Add('godot_export_templates.install_directory invalid')
}

function Get-LockedArchive([string]$name) {
    $entry = $lock.$name
    Assert-NinhoNoReparseAncestors -Path $downloads -AllowedRoot $root | Out-Null
    New-Item -ItemType Directory -Force $downloads | Out-Null
    $extension = [IO.Path]::GetExtension(([Uri]$entry.url).AbsolutePath)
    $target = Join-Path $downloads "$name$extension"
    Assert-NinhoNoReparseAncestors -Path $target -AllowedRoot $downloads | Out-Null
    if (-not (Test-Path -LiteralPath $target)) {
        Invoke-WebRequest -UseBasicParsing -Uri $entry.url -OutFile $target
    }
    # The downloader may have replaced the output or an ancestor.
    Assert-NinhoNoReparseAncestors -Path $target -AllowedRoot $downloads | Out-Null
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $target).Hash.ToLowerInvariant()
    if ($actual -ne $entry.sha256) {
        Remove-Item -LiteralPath $target -Force
        throw "$name checksum mismatch: $actual"
    }
    return $target
}

function Install-Portable([string]$name) {
    $entry = $lock.$name
    $destination = Join-Path $tools $name
    $exe = Join-Path $destination $entry.exe
    Assert-NinhoNoReparseAncestors -Path $destination -AllowedRoot $root | Out-Null
    $requiredExecutables = @($exe)
    if ($entry.PSObject.Properties.Name -ccontains 'ffprobe_exe') {
        $requiredExecutables += Join-Path $destination $entry.ffprobe_exe
    }
    $requiresInstall = @($requiredExecutables | Where-Object {
            -not (Test-Path -LiteralPath $_ -PathType Leaf)
        }).Count -gt 0
    if ($requiresInstall) {
        $archive = Get-LockedArchive $name
        New-Item -ItemType Directory -Force $destination | Out-Null
        Assert-NinhoNoReparseAncestors -Path $archive -AllowedRoot $downloads | Out-Null
        Expand-Archive -LiteralPath $archive -DestinationPath $destination -Force
        Assert-NinhoNoReparseAncestors -Path $destination -AllowedRoot $root | Out-Null
        Assert-NinhoNoReparseAncestors -Path $exe -AllowedRoot $root | Out-Null
    }
    foreach ($requiredExecutable in $requiredExecutables) {
        Assert-NinhoNoReparseAncestors -Path $requiredExecutable -AllowedRoot $root | Out-Null
    }
    return $exe
}

function Install-GodotExportTemplates {
    $entry = $lock.godot_export_templates
    $archive = Get-LockedArchive 'godot_export_templates'
    $godotRoot = Join-Path $tools 'godot'
    $selfContainedMarker = Join-Path $godotRoot '_sc_'
    Assert-NinhoNoReparseAncestors -Path $selfContainedMarker -AllowedRoot $root | Out-Null
    if (-not (Test-Path -LiteralPath $godotRoot -PathType Container)) {
        throw 'godot_export_templates requires the pinned portable Godot installation'
    }
    if (-not (Test-Path -LiteralPath $selfContainedMarker)) {
        [IO.File]::WriteAllText($selfContainedMarker, '', [Text.UTF8Encoding]::new($false))
    }
    $templatesRoot = Join-Path $godotRoot 'editor_data\export_templates'
    $destination = Join-Path $templatesRoot $entry.install_directory
    $temporary = Join-Path $templatesRoot ('.install-' + [Guid]::NewGuid().ToString('N'))
    Assert-NinhoNoReparseAncestors -Path $templatesRoot -AllowedRoot $root | Out-Null
    New-Item -ItemType Directory -Force -Path $templatesRoot | Out-Null
    Assert-NinhoNoReparseAncestors -Path $temporary -AllowedRoot $templatesRoot | Out-Null
    New-Item -ItemType Directory -Path $temporary | Out-Null

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archiveHandle = $null
    try {
        # Get-LockedArchive verifies the complete TPZ before any archive entry is trusted.
        Assert-NinhoNoReparseAncestors -Path $archive -AllowedRoot $downloads | Out-Null
        $archiveHandle = [IO.Compression.ZipFile]::OpenRead($archive)
        foreach ($zipEntry in $archiveHandle.Entries) {
            $entryPath = $zipEntry.FullName.Replace('\', '/')
            if (-not $entryPath.StartsWith('templates/', [StringComparison]::Ordinal) -or
                    $entryPath.Contains('../') -or $entryPath.Contains(':') -or
                    [IO.Path]::IsPathRooted($entryPath)) {
                throw "godot_export_templates unsafe archive entry: $entryPath"
            }
            $relative = $entryPath.Substring('templates/'.Length)
            if ([string]::IsNullOrEmpty($relative)) { continue }
            $mode = (($zipEntry.ExternalAttributes -shr 16) -band 0xF000)
            if ($mode -eq 0xA000) {
                throw "godot_export_templates symbolic link entry is forbidden: $entryPath"
            }
            $target = Join-Path $temporary $relative
            Assert-NinhoNoReparseAncestors -Path $target -AllowedRoot $temporary | Out-Null
            if ($entryPath.EndsWith('/', [StringComparison]::Ordinal)) {
                New-Item -ItemType Directory -Force -Path $target | Out-Null
                continue
            }
            $parent = Split-Path -Parent $target
            New-Item -ItemType Directory -Force -Path $parent | Out-Null
            [IO.Compression.ZipFileExtensions]::ExtractToFile($zipEntry, $target, $true)
        }
        $archiveHandle.Dispose()
        $archiveHandle = $null

        foreach ($required in 'windows_release_x86_64.exe','windows_debug_x86_64.exe') {
            $requiredPath = Join-Path $temporary $required
            Assert-NinhoNoReparseAncestors -Path $requiredPath -AllowedRoot $temporary | Out-Null
            if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf) -or
                    (Get-Item -LiteralPath $requiredPath).Length -eq 0) {
                throw "godot_export_templates missing required file: $required"
            }
        }

        if (Test-Path -LiteralPath $destination) {
            Assert-NinhoNoReparseAncestors -Path $destination -AllowedRoot $templatesRoot | Out-Null
            Remove-Item -LiteralPath $destination -Recurse -Force
        }
        Move-Item -LiteralPath $temporary -Destination $destination
        Assert-NinhoNoReparseAncestors -Path $destination -AllowedRoot $templatesRoot | Out-Null
        return $destination
    }
    finally {
        if ($archiveHandle) { $archiveHandle.Dispose() }
        if (Test-Path -LiteralPath $temporary) {
            Assert-NinhoNoReparseAncestors -Path $temporary -AllowedRoot $templatesRoot | Out-Null
            Remove-Item -LiteralPath $temporary -Recurse -Force
        }
    }
}

$cmake = Join-Path $tools ('cmake\' + $lock.cmake.exe)
$ninja = Join-Path $tools ('ninja\' + $lock.ninja.exe)
$godot = Join-Path $tools ('godot\' + $lock.godot.exe)
$portableBlender = Join-Path $tools ('blender\' + $lock.blender.exe)
$ffmpeg = Join-Path $tools ('ffmpeg\' + $lock.ffmpeg.exe)
$ffprobe = Join-Path $tools ('ffmpeg\' + $lock.ffmpeg.ffprobe_exe)
$installedBlender = Join-Path $env:ProgramFiles 'Blender Foundation\Blender 5.1\blender.exe'
$blender = if (Test-Path -LiteralPath $portableBlender) { $portableBlender } else { $installedBlender }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsInstall = $null
function Get-LockedVisualStudio {
    if (-not (Test-Path -LiteralPath $vswhere)) { return $null }
    $requirements = @($lock.visual_studio.components)
    $items = & $vswhere -version '[18.7,18.8)' -products '*' -requires $requirements -format json | ConvertFrom-Json
    $match = $items | Where-Object {
        $_.catalog.productDisplayVersion -eq '18.7.3' -and $_.installationVersion -like '18.7.11925.98*'
    } | Select-Object -First 1
    return $match.installationPath
}
if (Test-Path -LiteralPath $vswhere) {
    $vsInstall = Get-LockedVisualStudio
}

if ($InstallPortable -and $errors.Count -eq 0) {
    $cmake = Install-Portable 'cmake'
    $ninja = Install-Portable 'ninja'
    $godot = Install-Portable 'godot'
    $blender = Install-Portable 'blender'
    $ffmpeg = Install-Portable 'ffmpeg'
    $ffprobe = Join-Path $tools ('ffmpeg\' + $lock.ffmpeg.ffprobe_exe)
}
$exportTemplates = Join-Path $tools ('godot\editor_data\export_templates\' + $lock.godot_export_templates.install_directory)
if (($InstallPortable -or $InstallExportTemplates) -and $errors.Count -eq 0) {
    $exportTemplates = Install-GodotExportTemplates
}
if ($InstallVisualStudio -and $errors.Count -eq 0) {
    if (-not $vsInstall) {
        $installer = Get-LockedArchive 'visual_studio'
        $args = @('--quiet','--wait','--norestart','--nocache')
        foreach ($component in $lock.visual_studio.components) { $args += @('--add', $component) }
        $process = Start-Process -FilePath $installer -ArgumentList $args -Wait -PassThru -WindowStyle Hidden
        if ($process.ExitCode -notin 0,3010) { throw "VS installer failed: $($process.ExitCode)" }
        $vsInstall = Get-LockedVisualStudio
    }
}

if ($CheckOnly -or $InstallPortable) {
    foreach ($pair in @(@('cmake',$cmake),@('ninja',$ninja),@('godot',$godot),@('blender',$blender),@('ffmpeg',$ffmpeg))) {
        if ($pair[0] -ne 'blender' -or $pair[1].StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) {
            Assert-NinhoNoReparseAncestors -Path $pair[1] -AllowedRoot $root | Out-Null
        }
        if (-not (Test-Path -LiteralPath $pair[1])) {
            $errors.Add("$($pair[0]) missing: $($pair[1])")
            continue
        }
        $actualExe=(Get-FileHash -Algorithm SHA256 -LiteralPath $pair[1]).Hash.ToLowerInvariant()
        if ($actualExe -ne $lock.($pair[0]).exe_sha256) {
            $errors.Add("$($pair[0]) executable checksum mismatch")
            continue
        }
        switch ($pair[0]) {
            'cmake' {
                if ((& $pair[1] --version | Select-Object -First 1) -notmatch '4\.3\.3') {
                    $errors.Add('CMake executable version mismatch')
                }
            }
            'ninja' {
                if ((& $pair[1] --version) -ne '1.13.2') {
                    $errors.Add('Ninja executable version mismatch')
                }
            }
            'godot' {
                if ((& $pair[1] --version) -notmatch '^4\.5\.1\.stable') {
                    $errors.Add('Godot executable version mismatch')
                }
            }
            'blender' {
                if ((& $pair[1] --version | Select-Object -First 1) -notmatch '^Blender 5\.1\.2$') {
                    $errors.Add('Blender executable version mismatch')
                }
            }
            'ffmpeg' {
                $ffmpegVersionOutput = @(& $pair[1] -version 2>&1)
                $ffmpegVersionExitCode = $LASTEXITCODE
                if ($ffmpegVersionExitCode -ne 0) {
                    $errors.Add("FFmpeg version check failed: $ffmpegVersionExitCode")
                } elseif ($ffmpegVersionOutput[0] -notmatch '^ffmpeg version 8\.1\.2-essentials_build-www\.gyan\.dev') {
                    $errors.Add('FFmpeg executable version mismatch')
                }
            }
        }
    }
    Assert-NinhoNoReparseAncestors -Path $ffprobe -AllowedRoot $root | Out-Null
    if (-not (Test-Path -LiteralPath $ffprobe -PathType Leaf)) {
        $errors.Add("ffprobe missing: $ffprobe")
    } else {
        $actualFfprobe = (Get-FileHash -Algorithm SHA256 -LiteralPath $ffprobe).Hash.ToLowerInvariant()
        if ($actualFfprobe -ne $lock.ffmpeg.ffprobe_exe_sha256) {
            $errors.Add('ffprobe executable checksum mismatch')
        } else {
            $ffprobeVersionOutput = @(& $ffprobe -version 2>&1)
            $ffprobeVersionExitCode = $LASTEXITCODE
            if ($ffprobeVersionExitCode -ne 0) {
                $errors.Add("FFprobe version check failed: $ffprobeVersionExitCode")
            } elseif ($ffprobeVersionOutput[0] -notmatch '^ffprobe version 8\.1\.2-essentials_build-www\.gyan\.dev') {
                $errors.Add('FFprobe executable version mismatch')
            }
        }
    }
    $pythonVersion = $null
    $pythonCommand = Get-Command $lock.python.command -ErrorAction SilentlyContinue
    if (-not $pythonCommand) {
        $errors.Add('Python 3.11+ missing')
    } else {
        try { $pythonVersion = & $pythonCommand.Source -c "import sys; print('.'.join(map(str,sys.version_info[:3])))" }
        catch { $errors.Add("Python detection failed: $($_.Exception.Message)") }
        if ($pythonVersion -and [version]$pythonVersion -lt [version]$lock.python.minimum_version) { $errors.Add("Python too old: $pythonVersion") }
    }
}
if ($CheckOnly -or $InstallVisualStudio) {
    if (-not $vsInstall) { $errors.Add('Visual Studio components missing') }
}

$result = [ordered]@{
    ok = ($errors.Count -eq 0)
    cmake = @{ version=$lock.cmake.version; path=$cmake }
    ninja = @{ version=$lock.ninja.version; path=$ninja }
    godot = @{ version=$lock.godot.version; path=$godot }
    godot_export_templates = @{
        version=$lock.godot_export_templates.version
        path=$exportTemplates
        archive_sha256=$lock.godot_export_templates.sha256
    }
    blender = @{ version=$lock.blender.version; path=$blender; executable_sha256=$lock.blender.exe_sha256 }
    ffmpeg = @{ version=$lock.ffmpeg.version; path=$ffmpeg; executable_sha256=$lock.ffmpeg.exe_sha256 }
    ffprobe = @{ version=$lock.ffmpeg.version; path=$ffprobe; executable_sha256=$lock.ffmpeg.ffprobe_exe_sha256 }
    python = @{ minimum_version=$lock.python.minimum_version; detected_version=$pythonVersion }
    visual_studio = @{ version=$lock.visual_studio.version; path=$vsInstall }
    errors = @($errors)
}
if ($Json) { $result | ConvertTo-Json -Depth 5 } else { $result }
if (-not $result.ok) { exit 1 }
