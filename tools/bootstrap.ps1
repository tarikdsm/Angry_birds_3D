[CmdletBinding(DefaultParameterSetName='Check')]
param(
    [Parameter(ParameterSetName='Validate')][switch]$ValidateLock,
    [Parameter(ParameterSetName='Check')][switch]$CheckOnly,
    [Parameter(ParameterSetName='Portable')][switch]$InstallPortable,
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
foreach ($name in 'cmake','ninja','godot','visual_studio') {
    $entry = $lock.$name
    if (-not $entry.version) { $errors.Add("$name.version missing") }
    if ($entry.url -notmatch '^https://') { $errors.Add("$name.url must use https") }
    if (-not (Test-HexSha $entry.sha256)) { $errors.Add("$name.sha256 invalid") }
    if ($name -ne 'visual_studio' -and -not (Test-HexSha $entry.exe_sha256)) { $errors.Add("$name.exe_sha256 invalid") }
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
    if (-not (Test-Path -LiteralPath $exe)) {
        $archive = Get-LockedArchive $name
        New-Item -ItemType Directory -Force $destination | Out-Null
        Assert-NinhoNoReparseAncestors -Path $archive -AllowedRoot $downloads | Out-Null
        Expand-Archive -LiteralPath $archive -DestinationPath $destination -Force
        Assert-NinhoNoReparseAncestors -Path $destination -AllowedRoot $root | Out-Null
        Assert-NinhoNoReparseAncestors -Path $exe -AllowedRoot $root | Out-Null
    }
    Assert-NinhoNoReparseAncestors -Path $exe -AllowedRoot $root | Out-Null
    return $exe
}

$cmake = Join-Path $tools ('cmake\' + $lock.cmake.exe)
$ninja = Join-Path $tools ('ninja\' + $lock.ninja.exe)
$godot = Join-Path $tools ('godot\' + $lock.godot.exe)
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
    foreach ($pair in @(@('cmake',$cmake),@('ninja',$ninja),@('godot',$godot))) {
        Assert-NinhoNoReparseAncestors -Path $pair[1] -AllowedRoot $root | Out-Null
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
    python = @{ minimum_version=$lock.python.minimum_version; detected_version=$pythonVersion }
    visual_studio = @{ version=$lock.visual_studio.version; path=$vsInstall }
    errors = @($errors)
}
if ($Json) { $result | ConvertTo-Json -Depth 5 } else { $result }
if (-not $result.ok) { exit 1 }
