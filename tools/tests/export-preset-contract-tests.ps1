param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
)

$ErrorActionPreference = 'Stop'

function Assert-True {
    param([bool]$Condition, [string]$Message)

    if (-not $Condition) {
        throw $Message
    }
}

$presetPath = Join-Path $Root 'game\export_presets.cfg'
$packageScriptPath = Join-Path $Root 'tools\package_windows.ps1'
$readmePath = Join-Path $Root 'README.md'

$preset = [IO.File]::ReadAllText($presetPath)
$packageScript = [IO.File]::ReadAllText($packageScriptPath)
$readme = [IO.File]::ReadAllText($readmePath)

Assert-True ($preset.Contains(
        'export_path="../artifacts/export/windows/NinhoOrbital.exe"')) `
    'The editor export must use the clearly non-package manual export directory'
Assert-True ($readme.Contains('artifacts/package/windows-release')) `
    'README must identify the validated package output directory'
Assert-True ($packageScript -match
    '\$exportExecutable\s*=\s*Join-Path\s+\$stagingRoot\s+''NinhoOrbital\.exe''') `
    'The package script must export to its validated staging directory'
$explicitExportInvocation = @'
'--export-release', '"Windows Desktop"', ('"' + $exportExecutable + '"')
'@
Assert-True ($packageScript.Contains($explicitExportInvocation.Trim())) `
    'The package script must pass an explicit CLI output path after the preset name'

Assert-True ($preset.Contains('application/modify_resources=false')) `
    'Windows PE resource modification must remain explicitly disabled'

$inertResourceOptions = @(
    'application/icon=',
    'application/console_wrapper_icon=',
    'application/icon_interpolation=',
    'application/file_version=',
    'application/product_version=',
    'application/company_name=',
    'application/product_name=',
    'application/file_description=',
    'application/copyright=',
    'application/trademarks='
)
foreach ($option in $inertResourceOptions) {
    Assert-True (-not $preset.Contains($option)) `
        "Inactive PE resource option must be absent while modify_resources=false: $option"
}

Write-Host 'Export preset contract tests passed.'
