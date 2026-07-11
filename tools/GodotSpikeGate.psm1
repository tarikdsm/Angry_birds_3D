Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Read-NinhoGDExtensionDescriptor {
    [CmdletBinding()]
    param([Parameter(Mandatory)] [string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "GDExtension descriptor does not exist: $Path"
    }

    $values = [System.Collections.Generic.Dictionary[string, object]]::new(
        [System.StringComparer]::Ordinal)
    $section = $null
    $lineNumber = 0
    foreach ($rawLine in [System.IO.File]::ReadAllLines((Resolve-Path -LiteralPath $Path).Path)) {
        $lineNumber += 1
        $line = $rawLine.Trim()
        if ($line.Length -eq 0 -or $line.StartsWith(';') -or $line.StartsWith('#')) {
            continue
        }
        if ($line -cmatch '^\[([a-z_]+)\]$') {
            $section = $Matches[1]
            if ($section -cne 'configuration' -and $section -cne 'libraries') {
                throw "Unexpected descriptor section [$section] at line $lineNumber"
            }
            continue
        }
        if (-not $section) {
            throw "Descriptor value outside a section at line $lineNumber"
        }
        if ($line -cnotmatch '^([a-z0-9_.]+)\s*=\s*(.+)$') {
            throw "Malformed descriptor line $lineNumber`: $line"
        }

        $key = "$section/$($Matches[1])"
        if ($values.ContainsKey($key)) {
            throw "Duplicate descriptor key: $key"
        }
        $rawValue = $Matches[2].Trim()
        if ($rawValue -cmatch '^"([^"]*)"$') {
            $value = $Matches[1]
        } elseif ($rawValue -ceq 'true' -or $rawValue -ceq 'false') {
            $value = $rawValue -ceq 'true'
        } else {
            throw "Unsupported descriptor value for $key"
        }
        $values[$key] = $value
    }

    $expected = [System.Collections.Generic.Dictionary[string, object]]::new(
        [System.StringComparer]::Ordinal)
    $expected.Add('configuration/entry_symbol', 'ninho_physics_library_init')
    $expected.Add('configuration/compatibility_minimum', '4.5')
    $expected.Add('configuration/reloadable', $true)
    $expected.Add(
        'libraries/windows.debug.x86_64',
        'res://bin/ninho_physics.windows.template_debug.x86_64.dll')
    $expected.Add(
        'libraries/windows.release.x86_64',
        'res://bin/ninho_physics.windows.template_release.x86_64.dll')
    foreach ($key in $expected.Keys) {
        if (-not $values.ContainsKey($key)) {
            throw "GDExtension descriptor is missing $key"
        }
        if ($values[$key] -cne $expected[$key]) {
            throw "Invalid $key`: expected '$($expected[$key])', got '$($values[$key])'"
        }
    }
    foreach ($key in $values.Keys) {
        if (-not $expected.ContainsKey($key)) {
            throw "Unexpected descriptor key: $key"
        }
    }

    [pscustomobject]@{
        EntrySymbol = $values['configuration/entry_symbol']
        CompatibilityMinimum = $values['configuration/compatibility_minimum']
        Reloadable = $values['configuration/reloadable']
        DebugLibrary = $values['libraries/windows.debug.x86_64']
        ReleaseLibrary = $values['libraries/windows.release.x86_64']
    }
}

function New-NinhoTestGDExtensionManifest {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [pscustomobject]$Descriptor,
        [Parameter(Mandatory)] [ValidateSet('Debug', 'Release')] [string]$Configuration,
        [Parameter(Mandatory)] [string]$CacheDirectory
    )

    $selectedLibrary = if ($Configuration -eq 'Debug') {
        $Descriptor.DebugLibrary
    } else {
        $Descriptor.ReleaseLibrary
    }
    if (-not $selectedLibrary.StartsWith('res://bin/', [System.StringComparison]::Ordinal)) {
        throw "Selected GDExtension library must be below res://bin/: $selectedLibrary"
    }

    New-Item -ItemType Directory -Force -Path $CacheDirectory | Out-Null
    $extensionListPath = Join-Path $CacheDirectory 'extension_list.cfg'
    $utf8WithoutBom = [System.Text.UTF8Encoding]::new($false)
    if ($Configuration -eq 'Debug') {
        [System.IO.File]::WriteAllText(
            $extensionListPath,
            "res://bin/ninho_physics.gdextension`n",
            $utf8WithoutBom)
        return [pscustomobject]@{
            SelectedLibrary = $selectedLibrary
            ExtensionPath = $null
            ExtensionListPath = $extensionListPath
        }
    }

    $extensionPath = Join-Path $CacheDirectory 'ninho_physics.test.gdextension'
    $extensionText = @"
[configuration]
entry_symbol = "$($Descriptor.EntrySymbol)"
compatibility_minimum = "$($Descriptor.CompatibilityMinimum)"
reloadable = false

[libraries]
windows.debug.x86_64 = "$selectedLibrary"
"@
    [System.IO.File]::WriteAllText($extensionPath, $extensionText, $utf8WithoutBom)
    [System.IO.File]::WriteAllText(
        $extensionListPath,
        "res://.godot/ninho_physics.test.gdextension`n",
        $utf8WithoutBom)

    [pscustomobject]@{
        SelectedLibrary = $selectedLibrary
        ExtensionPath = $extensionPath
        ExtensionListPath = $extensionListPath
    }
}

function Assert-NinhoSpikeViewContract {
    [CmdletBinding()]
    param([Parameter(Mandatory)] [string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Spike view script does not exist: $Path"
    }
    $source = [System.IO.File]::ReadAllText((Resolve-Path -LiteralPath $Path).Path)
    $requiredPatterns = [ordered]@{
        'expected body count' = '(?m)^const EXPECTED_BODY_COUNT := 122$'
        'expected visual count' = '(?m)^const EXPECTED_VISUAL_BODY_COUNT := 121$'
        'snapshot timeout' = '(?m)^const FIRST_SNAPSHOT_FRAME_LIMIT := 10$'
        'invalid handle guard' = '(?m)^\s*if handle == 0:$'
        'body snapshot assertion' = '(?m)^\s*if states\.size\(\) != EXPECTED_BODY_COUNT:$'
        'visual handle assertion' = '(?m)^\s*if alive_visual_count != EXPECTED_VISUAL_BODY_COUNT:$'
    }
    foreach ($name in $requiredPatterns.Keys) {
        if (-not [regex]::IsMatch($source, $requiredPatterns[$name])) {
            throw "Spike view contract is missing ${name}: $($requiredPatterns[$name])"
        }
    }
    if ([regex]::Matches(
            $source,
            '(?m)^\s*if not _register_visual_handle\(').Count -ne 2) {
        throw 'Spike view must fail-fast on all 120 boxes and the projectile spawn paths'
    }
    if ([regex]::Matches($source, 'physics\.get_body_states\(\)').Count -ne 1) {
        throw 'Spike view must fetch exactly one batched Box3D snapshot per physics frame'
    }
}

Export-ModuleMember -Function @(
    'Read-NinhoGDExtensionDescriptor',
    'New-NinhoTestGDExtensionManifest',
    'Assert-NinhoSpikeViewContract'
)
