$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Import-Module (Join-Path $root 'tools\SafePath.psm1') -Force
Import-Module (Join-Path $root 'tools\ToolchainIntegrity.psm1') -Force

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Get-ExpectedFailure {
    param([scriptblock]$Action, [string]$Message)
    try { & $Action }
    catch { return $_.Exception.Message }
    throw "$Message (operation unexpectedly succeeded)"
}

$packageText = [IO.File]::ReadAllText((Join-Path $root 'tools\package_windows.ps1'))
Assert-True ($packageText -match 'ToolchainIntegrity\.psm1') `
    'Windows package script does not import toolchain integrity checks'
$safePathImport = $packageText.IndexOf("Import-Module (Join-Path `$PSScriptRoot 'SafePath.psm1')", [StringComparison]::Ordinal)
$integrityImport = $packageText.IndexOf("Import-Module (Join-Path `$PSScriptRoot 'ToolchainIntegrity.psm1')", [StringComparison]::Ordinal)
Assert-True ($safePathImport -gt $integrityImport) `
    'Windows package must import SafePath after modules with nested SafePath imports'
Assert-True ($packageText -match '(?s)Assert-NinhoPinnedExecutable\s+`?\s*' +
    '-Path \$godot\s+`?\s*-ExpectedSha256 \$lock\.godot\.exe_sha256\s+`?\s*' +
    '-Name ''Godot''\s+`?\s*-AllowedRoot \$root\s*' +
    '\$exportProcess = Start-Process -FilePath \$godot') `
    'Windows package must verify the lock-pinned Godot immediately before export'
$testRunnerText = [IO.File]::ReadAllText((Join-Path $root 'tools\test.ps1'))
Assert-True ($testRunnerText -match 'tests\\toolchain-integrity-tests\.ps1') `
    'Native test runner does not execute toolchain integrity tests'
Assert-True ($testRunnerText -match 'tests\\toolchain-parent-scope-regression\.ps1') `
    'Native test runner does not execute the toolchain parent-scope regression'
$justInTimeImportPattern = @'
(?ms)Import-Module \(Join-Path \$PSScriptRoot 'ToolchainIntegrity\.psm1'\) -Force -Scope Local\s*
\$ffprobe = ToolchainIntegrity\\Resolve-NinhoPinnedToolchainExecutable\s+`
'@
Assert-True ($testRunnerText -match $justInTimeImportPattern) `
    'Native test runner must reimport ToolchainIntegrity locally at the FFprobe use site'

$artifactsRoot = Join-Path $root 'artifacts'
New-Item -ItemType Directory -Force -Path $artifactsRoot | Out-Null
$fixtureRoot = Join-Path $artifactsRoot ('toolchain-integrity-tests-' + [Guid]::NewGuid().ToString('N'))
Assert-NinhoNoReparseAncestors -Path $fixtureRoot -AllowedRoot $artifactsRoot | Out-Null

try {
    New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
    $fixtureExecutable = Join-Path $fixtureRoot 'godot-fixture.exe'
    [IO.File]::WriteAllBytes($fixtureExecutable, [Text.Encoding]::UTF8.GetBytes('known-good-godot'))
    $expectedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $fixtureExecutable).Hash.ToLowerInvariant()

    Assert-NinhoPinnedExecutable `
        -Path $fixtureExecutable `
        -ExpectedSha256 $expectedHash `
        -Name 'Godot' `
        -AllowedRoot $artifactsRoot

    [IO.File]::WriteAllBytes($fixtureExecutable, [Text.Encoding]::UTF8.GetBytes('tampered-godot'))
    $tamperedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $fixtureExecutable).Hash.ToLowerInvariant()
    $mismatch = Get-ExpectedFailure -Message 'Tampered executable was accepted' -Action {
        Assert-NinhoPinnedExecutable `
            -Path $fixtureExecutable `
            -ExpectedSha256 $expectedHash `
            -Name 'Godot' `
            -AllowedRoot $artifactsRoot
    }
    Assert-True ($mismatch -match 'Godot executable checksum mismatch') `
        'Tampered executable diagnostic omitted the tool name'
    Assert-True ($mismatch.Contains("expected=$expectedHash")) `
        'Tampered executable diagnostic omitted the expected hash'
    Assert-True ($mismatch.Contains("actual=$tamperedHash")) `
        'Tampered executable diagnostic omitted the actual hash'

    $invalidPin = Get-ExpectedFailure -Message 'Invalid SHA-256 pin was accepted' -Action {
        Assert-NinhoPinnedExecutable `
            -Path $fixtureExecutable `
            -ExpectedSha256 'not-a-sha256' `
            -Name 'Godot' `
            -AllowedRoot $artifactsRoot
    }
    Assert-True ($invalidPin -match 'pinned SHA-256 is invalid') `
        'Invalid pin diagnostic is missing'

    $missingPath = Join-Path $fixtureRoot 'missing.exe'
    $missing = Get-ExpectedFailure -Message 'Missing executable was accepted' -Action {
        Assert-NinhoPinnedExecutable `
            -Path $missingPath `
            -ExpectedSha256 $expectedHash `
            -Name 'Godot' `
            -AllowedRoot $artifactsRoot
    }
    Assert-True ($missing -match 'pinned executable is missing') `
        'Missing executable diagnostic is missing'

    $outside = Get-ExpectedFailure -Message 'Executable outside AllowedRoot was accepted' -Action {
        Assert-NinhoPinnedExecutable `
            -Path (Join-Path $root 'tools\toolchain.lock.json') `
            -ExpectedSha256 $expectedHash `
            -Name 'Godot' `
            -AllowedRoot $fixtureRoot
    }
    Assert-True ($outside -match 'outside the allowed root') `
        'Pinned executable did not enforce SafePath containment'

    $fixtureTools = Join-Path $fixtureRoot 'tools'
    $fixtureFfmpegRoot = Join-Path $fixtureRoot '.tools\ffmpeg\ffmpeg-fixture\bin'
    New-Item -ItemType Directory -Force -Path $fixtureTools, $fixtureFfmpegRoot | Out-Null
    $fixtureFfprobe = Join-Path $fixtureFfmpegRoot 'ffprobe.exe'
    [IO.File]::WriteAllBytes($fixtureFfprobe, [Text.Encoding]::UTF8.GetBytes('known-good-ffprobe'))
    $fixtureFfprobeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $fixtureFfprobe).Hash.ToLowerInvariant()
    [IO.File]::WriteAllText(
        (Join-Path $fixtureTools 'toolchain.lock.json'),
        (@{
            ffmpeg = @{
                ffprobe_exe = 'ffmpeg-fixture/bin/ffprobe.exe'
                ffprobe_exe_sha256 = $fixtureFfprobeHash
            }
        } | ConvertTo-Json -Depth 3),
        [Text.UTF8Encoding]::new($false))

    $resolvedFfprobe = Resolve-NinhoPinnedToolchainExecutable `
        -Root $fixtureRoot `
        -ToolName 'ffmpeg' `
        -ExecutableProperty 'ffprobe_exe' `
        -HashProperty 'ffprobe_exe_sha256' `
        -Name 'FFprobe'
    Assert-True ($resolvedFfprobe -ceq $fixtureFfprobe) `
        'Pinned toolchain resolver did not return the lock-owned FFprobe path'

    Write-Output 'Pinned toolchain executable integrity: PASS'
}
finally {
    if (Test-Path -LiteralPath $fixtureRoot) {
        Assert-NinhoNoReparseAncestors -Path $fixtureRoot -AllowedRoot $artifactsRoot | Out-Null
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}
