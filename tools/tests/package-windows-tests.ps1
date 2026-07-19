$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Import-Module (Join-Path $root 'tools\SafePath.psm1') -Force
Import-Module (Join-Path $root 'tools\TestedInputIdentity.psm1') -Force

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Invoke-ExpectFailure {
    param([scriptblock]$Action, [string]$Pattern, [string]$Message)
    try {
        & $Action
    } catch {
        if ($_.Exception.Message -match $Pattern) { return }
        throw "$Message (unexpected error: $($_.Exception.Message))"
    }
    throw "$Message (operation unexpectedly succeeded)"
}

$lockPath = Join-Path $root 'tools\toolchain.lock.json'
$lock = Get-Content -Raw -LiteralPath $lockPath | ConvertFrom-Json
$templates = $lock.godot_export_templates
Assert-True ($null -ne $templates) 'Godot export templates lock is missing'
Assert-True ($templates.version -ceq '4.5.1-stable') 'Godot export templates version is not pinned'
Assert-True ($templates.url -ceq 'https://github.com/godotengine/godot/releases/download/4.5.1-stable/Godot_v4.5.1-stable_export_templates.tpz') 'Godot export templates URL mismatch'
Assert-True ($templates.sha256 -ceq '1998af37f1387684e2c211cdb483daf492fc64dc6b12096bddcdca25b6910c86') 'Godot export templates SHA-256 mismatch'
Assert-True ($templates.install_directory -ceq '4.5.1.stable') 'Godot export templates install directory mismatch'
Assert-True ($templates.copyright_url -ceq 'https://raw.githubusercontent.com/godotengine/godot/4.5.1-stable/COPYRIGHT.txt') 'Godot copyright URL mismatch'
Assert-True ($templates.copyright_sha256 -ceq '2039020f520ebd55592070ede2ef38dfbc28a6550140008da004143872789e5d') 'Godot copyright SHA-256 mismatch'

$bootstrapText = [IO.File]::ReadAllText((Join-Path $root 'tools\bootstrap.ps1'))
Assert-True ($bootstrapText -match 'godot_export_templates') 'Bootstrap does not install pinned export templates'
Assert-True ($bootstrapText -match 'Assert-NinhoNoReparseAncestors') 'Bootstrap export-template path is not guarded'
Assert-True ($bootstrapText -match 'Get-FileHash') 'Bootstrap does not verify the archive before extraction'
Assert-True ($bootstrapText -match "'_sc_'") 'Bootstrap does not enable Godot self-contained mode'
Assert-True ($bootstrapText -match 'editor_data\\export_templates') 'Bootstrap does not use Godot portable editor data'

$presetPath = Join-Path $root 'game\export_presets.cfg'
Assert-True (Test-Path -LiteralPath $presetPath -PathType Leaf) 'Windows export preset is missing'
$presetText = [IO.File]::ReadAllText($presetPath)
Assert-True ($presetText -match 'name="Windows Desktop"') 'Windows Desktop export preset is missing'
Assert-True ($presetText -match 'platform="Windows Desktop"') 'Windows Desktop platform mismatch'
Assert-True ($presetText -match 'binary_format/embed_pck=false') 'Windows package must keep a separate PCK'
Assert-True ($presetText -match 'binary_format/architecture="x86_64"') 'Windows package must target x86_64'

$licensePath = Join-Path $root 'third_party\godot-export-templates.LICENSE.txt'
Assert-True (Test-Path -LiteralPath $licensePath -PathType Leaf) 'Godot export templates license is missing'
$templateLicenseText = [IO.File]::ReadAllText($licensePath) -replace "`r`n", "`n"
$godotLicenseText = [IO.File]::ReadAllText((Join-Path $root 'third_party\godot.LICENSE.txt')) -replace "`r`n", "`n"
Assert-True ($templateLicenseText -ceq $godotLicenseText) 'Godot export templates license must be the exact pinned Godot MIT text'
$copyrightPath = Join-Path $root 'third_party\godot.COPYRIGHT.txt'
Assert-True (Test-Path -LiteralPath $copyrightPath -PathType Leaf) 'Godot upstream copyright inventory is missing'
$copyrightContent = Get-NinhoCanonicalTestedInputContent `
    -Path $copyrightPath -RelativePath 'third_party/godot.COPYRIGHT.txt'
Assert-True ($copyrightContent.mode -ceq 'text_utf8_lf') `
    'Godot upstream copyright inventory must be UTF-8 text'
$copyrightAlgorithm = [Security.Cryptography.SHA256]::Create()
try {
    $copyrightHash = -join ($copyrightAlgorithm.ComputeHash($copyrightContent.bytes) |
        ForEach-Object { $_.ToString('x2') })
} finally { $copyrightAlgorithm.Dispose() }
Assert-True ($copyrightHash -ceq $templates.copyright_sha256) `
    'Godot upstream copyright inventory canonical LF hash mismatch'

$noticeText = [IO.File]::ReadAllText((Join-Path $root 'THIRD_PARTY_NOTICES.md'))
Assert-True ($noticeText -match 'Godot Export Templates 4\.5\.1-stable') 'NOTICE omits Godot export templates'
Assert-True ($noticeText -match [regex]::Escape($templates.sha256)) 'NOTICE omits the export-template archive hash'
$thirdPartyReadme = [IO.File]::ReadAllText((Join-Path $root 'third_party\README.md'))
Assert-True ($thirdPartyReadme -match 'Godot Export Templates') 'Third-party README omits Godot export templates'

$sbom = Get-Content -Raw -LiteralPath (Join-Path $root 'third_party\sbom.spdx.json') | ConvertFrom-Json
$templatePackage = @($sbom.packages | Where-Object SPDXID -ceq 'SPDXRef-Package-Godot-Export-Templates')
Assert-True ($templatePackage.Count -eq 1) 'SPDX must contain exactly one Godot export templates package'
Assert-True ($templatePackage[0].checksums[0].checksumValue -ceq $templates.sha256) 'SPDX export-template checksum mismatch'
$generatedFrom = @($sbom.relationships | Where-Object {
    $_.spdxElementId -ceq 'SPDXRef-Package-Ninho-Orbital' -and
    $_.relationshipType -ceq 'GENERATED_FROM' -and
    $_.relatedSpdxElement -ceq 'SPDXRef-Package-Godot-Export-Templates'
})
Assert-True ($generatedFrom.Count -eq 1) 'SPDX omits the export-template GENERATED_FROM relationship'

$packageScript = Join-Path $root 'tools\package_windows.ps1'
Assert-True (Test-Path -LiteralPath $packageScript -PathType Leaf) 'Windows package script is missing'
$packageText = [IO.File]::ReadAllText($packageScript)
Assert-True ($packageText -match 'Assert-NinhoNoReparseAncestors') 'Package output is not SafePath-guarded'
Assert-True ($packageText -match '--export-release') 'Package script does not call Godot release export'
Assert-True ($packageText -match '(?s)Start-Process -FilePath \$godot.+\$exportExitCode = \$exportProcess\.ExitCode') 'Package export must capture the pinned Godot process exit code explicitly'
Assert-True ($packageText -match 'GetExitCodeProcess') 'Packaged GUI launch must query a reliable native process exit code'
Assert-True ($packageText -match '(?s)\$launchHandle = \$launchProcess\.Handle.+WaitForExit.+GetExitCodeProcess\(\$launchHandle') 'Packaged launch must retain its process handle before waiting'
Assert-True ($packageText -notmatch '\$packagedBin') 'Package script must not create an unused duplicate GDExtension directory'
Assert-True ($packageText -match '\$releaseDllSnapshotHash') 'Package script must retain the post-build Release DLL hash snapshot'
Assert-True ($packageText -match 'vertical_slice_smoke\.gd') 'Package script does not launch the packaged minimal flow'
Assert-True (@([regex]::Matches($packageText, '\(SCRIPT ERROR\|ERROR\|WARNING\)')).Count -ge 2) 'Package export and launch must reject warnings'
$launchSuccessIndex = $packageText.IndexOf('Windows package launch: PASS', [StringComparison]::Ordinal)
$manifestMarkerIndex = $packageText.IndexOf('NINHO_PACKAGE_MANIFEST', [StringComparison]::Ordinal)
Assert-True ($launchSuccessIndex -ge 0 -and $manifestMarkerIndex -gt $launchSuccessIndex) `
    'Package manifest marker must be emitted only after successful packaged launch'

$artifactsRoot = Join-Path $root 'artifacts'
New-Item -ItemType Directory -Force -Path $artifactsRoot | Out-Null
$testRoot = Join-Path $artifactsRoot ('package-tests-' + [Guid]::NewGuid().ToString('N'))
$packageRoot = Join-Path $testRoot 'Package With Spaces'
Assert-NinhoNoReparseAncestors -Path $testRoot -AllowedRoot $artifactsRoot | Out-Null

try {
    $packageOutput = @(& $packageScript -OutputRoot $packageRoot)

    $exe = Join-Path $packageRoot 'NinhoOrbital.exe'
    $pck = Join-Path $packageRoot 'NinhoOrbital.pck'
    $dllName = 'ninho_physics.windows.template_release.x86_64.dll'
    $dll = Join-Path $packageRoot $dllName
    $manifestPath = Join-Path $packageRoot 'manifest.sha256.json'
    $sourceInventoryPath = Join-Path $packageRoot 'package-source-inventory.json'
    foreach ($required in @($exe, $pck, $dll, $manifestPath, $sourceInventoryPath)) {
        Assert-True (Test-Path -LiteralPath $required -PathType Leaf) "Package file missing: $required"
        Assert-True ((Get-Item -LiteralPath $required).Length -gt 0) "Package file is empty: $required"
    }
    $manifestMarkers = @($packageOutput | ForEach-Object { [string]$_ } | Where-Object {
        $_.StartsWith('NINHO_PACKAGE_MANIFEST ', [StringComparison]::Ordinal)
    })
    Assert-True ($manifestMarkers.Count -eq 1) 'Clean package build must emit exactly one manifest marker'
    $manifestClaim = $manifestMarkers[0].Substring('NINHO_PACKAGE_MANIFEST '.Length) | ConvertFrom-Json
    Assert-True ([string]::Equals(
            [IO.Path]::GetFullPath([string]$manifestClaim.path),
            [IO.Path]::GetFullPath($manifestPath),
            [StringComparison]::OrdinalIgnoreCase)) `
        'Clean package marker did not reference the final manifest'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $packageRoot 'ninho_physics.windows.template_debug.x86_64.dll'))) 'Debug DLL leaked into release package'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $packageRoot "bin\$dllName"))) 'Unused duplicate release DLL leaked into package'

    foreach ($requiredLicense in @(
        'licenses\THIRD_PARTY_NOTICES.md',
        'licenses\sbom.spdx.json',
        'licenses\box3d.LICENSE.txt',
        'licenses\godot.LICENSE.txt',
        'licenses\godot.COPYRIGHT.txt',
        'licenses\godot-export-templates.LICENSE.txt',
        'licenses\godot-cpp.LICENSE.txt',
        'licenses\nlohmann-json.LICENSE.txt'
    )) {
        $packagedLicense = Join-Path $packageRoot $requiredLicense
        Assert-True (Test-Path -LiteralPath $packagedLicense -PathType Leaf) `
            "Package license missing: $requiredLicense"
        $packagedBytes = [IO.File]::ReadAllBytes($packagedLicense)
        $packagedCanonical = Get-NinhoCanonicalTestedInputContent `
            -Path $packagedLicense -RelativePath $requiredLicense.Replace('\','/')
        Assert-True ($packagedCanonical.mode -ceq 'text_utf8_lf') `
            "Package license is not UTF-8 text: $requiredLicense"
        Assert-True ([Convert]::ToBase64String($packagedBytes) -ceq
            [Convert]::ToBase64String($packagedCanonical.bytes)) `
            "Package license is not canonical LF text: $requiredLicense"
    }
    Assert-True ((Get-FileHash -Algorithm SHA256 -LiteralPath `
        (Join-Path $packageRoot 'licenses\godot.COPYRIGHT.txt')).Hash.ToLowerInvariant() -ceq
        $templates.copyright_sha256) `
        'Packaged Godot copyright inventory canonical hash mismatch'

    $sourceInventory = Get-Content -Raw -LiteralPath $sourceInventoryPath | ConvertFrom-Json
    Assert-True ($sourceInventory.schema -ceq 'ninho.package-source-inventory.v1') 'Package source inventory schema mismatch'
    Assert-True ($sourceInventory.schema_version -eq 1) 'Package source inventory schema version mismatch'
    Assert-True ($sourceInventory.note -match 'source inputs') 'Package source inventory must identify its source-input scope'
    Assert-True ($sourceInventory.note -match 'does not enumerate') 'Package source inventory must disclaim PCK enumeration'
    Assert-True ($sourceInventory.note -match 'PCK SHA-256') 'Package source inventory must identify the PCK integrity proof'
    Assert-True ($sourceInventory.note -match 'packaged runtime smoke') 'Package source inventory must identify the PCK runtime-load proof'
    Assert-True (@($sourceInventory.resources | Where-Object category -ceq 'data').Count -gt 0) 'Package source inventory does not contain gameplay data'
    Assert-True (@($sourceInventory.resources | Where-Object category -ceq 'asset').Count -gt 0) 'Package source inventory does not contain visual assets'
    Assert-True (@($sourceInventory.resources | Where-Object category -ceq 'audio').Count -gt 0) 'Package source inventory does not contain audio assets'

    $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    Assert-True ($manifest.schema_version -eq 1) 'Package manifest schema mismatch'
    Assert-True ($manifest.algorithm -ceq 'SHA-256') 'Package manifest algorithm mismatch'
    Assert-True ($manifest.configuration -ceq 'Release') 'Package manifest configuration mismatch'
    Assert-True ($manifest.export_templates.archive_sha256 -ceq $templates.sha256) 'Package manifest template pin mismatch'
    Assert-True ($manifest.runtime_extension.path -ceq $dllName) 'Package manifest runtime DLL path mismatch'
    Assert-True ($manifest.runtime_extension.source_path -ceq "game/bin/$dllName") 'Package manifest runtime DLL source mismatch'
    Assert-True ($manifest.runtime_extension.source_sha256 -ceq (Get-FileHash -Algorithm SHA256 -LiteralPath $dll).Hash.ToLowerInvariant()) 'Packaged DLL does not match the recorded post-build snapshot'
    $paths = @($manifest.files.path)
    $sourceInventoryRow = @($manifest.files | Where-Object path -CEQ 'package-source-inventory.json')
    Assert-True ($sourceInventoryRow.Count -eq 1) 'Package manifest must contain one source inventory row'
    Assert-True ($sourceInventoryRow[0].role -ceq 'source_inventory') 'Package source inventory role mismatch'
    Assert-True ($paths -cnotcontains 'package-content.json') 'Legacy misleading package-content.json leaked into package'
    [string[]]$sortedPaths = @($paths)
    [Array]::Sort($sortedPaths, [StringComparer]::Ordinal)
    Assert-True (($paths -join "`n") -ceq ($sortedPaths -join "`n")) 'Package manifest paths are not canonical and sorted'
    Assert-True (-not ($paths -contains 'manifest.sha256.json')) 'Manifest must not recursively hash itself'
    foreach ($entry in $manifest.files) {
        $file = Join-Path $packageRoot ([string]$entry.path)
        Assert-True (Test-Path -LiteralPath $file -PathType Leaf) "Manifest references missing file: $($entry.path)"
        Assert-True ((Get-FileHash -Algorithm SHA256 -LiteralPath $file).Hash.ToLowerInvariant() -ceq $entry.sha256) "Manifest hash mismatch: $($entry.path)"
        Assert-True ((Get-Item -LiteralPath $file).Length -eq $entry.size_bytes) "Manifest size mismatch: $($entry.path)"
    }

    & $packageScript -OutputRoot $packageRoot -VerifyOnly

    [IO.File]::AppendAllText($pck, 'tamper')
    Invoke-ExpectFailure { & $packageScript -OutputRoot $packageRoot -VerifyOnly } '(size|hash) mismatch' 'VerifyOnly accepted a tampered PCK'

    $outside = Join-Path (Split-Path $root -Parent) 'package-outside-artifacts'
    Invoke-ExpectFailure { & $packageScript -OutputRoot $outside -VerifyOnly } 'outside the allowed root' 'Package script accepted an output outside artifacts'
}
finally {
    if (Test-Path -LiteralPath $testRoot) {
        Assert-NinhoNoReparseAncestors -Path $testRoot -AllowedRoot $artifactsRoot | Out-Null
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}

Write-Output 'Windows package contracts: PASS'
