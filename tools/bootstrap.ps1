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
$lock = Get-Content -Raw -LiteralPath $lockPath | ConvertFrom-Json
$errors = [System.Collections.Generic.List[string]]::new()

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
    New-Item -ItemType Directory -Force $downloads | Out-Null
    $extension = [IO.Path]::GetExtension(([Uri]$entry.url).AbsolutePath)
    $target = Join-Path $downloads "$name$extension"
    if (-not (Test-Path -LiteralPath $target)) {
        Invoke-WebRequest -UseBasicParsing -Uri $entry.url -OutFile $target
    }
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $target).Hash.ToLowerInvariant()
    if ($actual -ne $entry.sha256) { throw "$name checksum mismatch: $actual" }
    return $target
}

function Install-Portable([string]$name) {
    $entry = $lock.$name
    $destination = Join-Path $tools $name
    $exe = Join-Path $destination $entry.exe
    if (-not (Test-Path -LiteralPath $exe)) {
        $archive = Get-LockedArchive $name
        New-Item -ItemType Directory -Force $destination | Out-Null
        Expand-Archive -LiteralPath $archive -DestinationPath $destination -Force
    }
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
        if (-not (Test-Path -LiteralPath $pair[1])) { $errors.Add("$($pair[0]) missing: $($pair[1])") }
    }
    if (Test-Path $cmake) { if ((& $cmake --version | Select-Object -First 1) -notmatch '4\.3\.3') { $errors.Add('CMake executable version mismatch') } }
    if (Test-Path $ninja) { if ((& $ninja --version) -ne '1.13.2') { $errors.Add('Ninja executable version mismatch') } }
    if (Test-Path $godot) { if ((& $godot --version) -notmatch '^4\.5\.1\.stable') { $errors.Add('Godot executable version mismatch') } }
    foreach ($pair in @(@('cmake',$cmake),@('ninja',$ninja),@('godot',$godot))) {
        if (Test-Path $pair[1]) {
            $actualExe=(Get-FileHash -Algorithm SHA256 -LiteralPath $pair[1]).Hash.ToLowerInvariant()
            if ($actualExe -ne $lock.($pair[0]).exe_sha256) { $errors.Add("$($pair[0]) executable checksum mismatch") }
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
