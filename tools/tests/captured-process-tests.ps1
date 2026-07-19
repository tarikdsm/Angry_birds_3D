$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Import-Module (Join-Path $root 'tools\SafePath.psm1') -Force
Import-Module (Join-Path $root 'tools\CapturedProcess.psm1') -Force

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

$packageText = [IO.File]::ReadAllText((Join-Path $root 'tools\package_windows.ps1'))
Assert-True ($packageText -match "CapturedProcess\.psm1") `
    'Windows package script does not import the captured-process module'
Assert-True ($packageText -match "(?s)Write-Verbose 'Building the Release GDExtension'.+" +
    "Invoke-NinhoCapturedProcess.+-LogPrefix 'package-build'.+" +
    "-FailureLabel 'Release GDExtension build'.+" +
    "Write-Verbose 'Release GDExtension build completed'") `
    'Windows package build does not use captured process diagnostics'
Assert-True ($packageText -notmatch '\$buildProcess\s*=\s*Start-Process') `
    'Windows package build still bypasses captured process diagnostics'
$testRunnerText = [IO.File]::ReadAllText((Join-Path $root 'tools\test.ps1'))
Assert-True ($testRunnerText -match 'tests\\captured-process-tests\.ps1') `
    'Native test runner does not execute captured-process diagnostics tests'

$artifactsRoot = Join-Path $root 'artifacts'
New-Item -ItemType Directory -Force -Path $artifactsRoot | Out-Null
$fixtureRoot = Join-Path $artifactsRoot ('captured-process-tests-' + [Guid]::NewGuid().ToString('N'))
Assert-NinhoNoReparseAncestors -Path $fixtureRoot -AllowedRoot $artifactsRoot | Out-Null

try {
    New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
    $fakeCommand = Join-Path $fixtureRoot 'fake-build.ps1'
    [IO.File]::WriteAllText($fakeCommand, @'
[Console]::Out.WriteLine('FAKE_BUILD_STDOUT_49')
[Console]::Error.WriteLine('FAKE_BUILD_STDERR_49')
exit 37
'@, [Text.UTF8Encoding]::new($false))

    $diagnostic = $null
    try {
        Invoke-NinhoCapturedProcess `
            -FilePath 'powershell' `
            -ArgumentList @(
                '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                ('"' + $fakeCommand + '"')
            ) `
            -LogDirectory $fixtureRoot `
            -AllowedRoot $artifactsRoot `
            -LogPrefix 'package-build' `
            -FailureLabel 'Release GDExtension build'
    } catch {
        $diagnostic = $_.Exception.Message
    }

    Assert-True ($null -ne $diagnostic) 'Captured fake build unexpectedly succeeded'
    Assert-True ($diagnostic -match 'exit code 37') 'Captured process did not preserve exit code 37'
    Assert-True ($diagnostic -match '(?s)stdout:\s*FAKE_BUILD_STDOUT_49') `
        'Captured process diagnostic omitted stdout'
    Assert-True ($diagnostic -match '(?s)stderr:\s*FAKE_BUILD_STDERR_49') `
        'Captured process diagnostic omitted stderr'
    Assert-True (@(Get-ChildItem -LiteralPath $fixtureRoot -Filter 'package-build-*.log').Count -eq 0) `
        'Captured process left temporary logs behind after failure'

    Write-Output 'Captured process failure diagnostics: PASS'
}
finally {
    if (Test-Path -LiteralPath $fixtureRoot) {
        Assert-NinhoNoReparseAncestors -Path $fixtureRoot -AllowedRoot $artifactsRoot | Out-Null
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}
