Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force

function Reset-NinhoUpstreamBuildDirectory {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$Path,
        [Parameter(Mandatory)] [string]$AllowedRoot
    )

    if (-not [System.IO.Path]::IsPathRooted($Path) -or
            -not [System.IO.Path]::IsPathRooted($AllowedRoot)) {
        throw 'Upstream build and allowed-root paths must be absolute'
    }
    $target = [System.IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    $root = [System.IO.Path]::GetFullPath($AllowedRoot).TrimEnd('\', '/')
    $rootPrefix = $root + [System.IO.Path]::DirectorySeparatorChar
    if ($target -eq $root -or
            -not $target.StartsWith(
                $rootPrefix,
                [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Upstream build path is outside the allowed root: $target"
    }
    Assert-NinhoNoReparseAncestors -Path $target -AllowedRoot $root | Out-Null
    if (Test-Path -LiteralPath $target) {
        Remove-Item -LiteralPath $target -Recurse -Force
    }
    if (Test-Path -LiteralPath $target) {
        throw "Upstream build directory survived clean reset: $target"
    }
}

function Assert-NinhoUpstreamCompileDatabase {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$Path,
        [Parameter(Mandatory)]
        [ValidateSet('Debug', 'Release')]
        [string]$Configuration
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Upstream compile database missing: $Path"
    }
    $parsedDatabase = [System.IO.File]::ReadAllText($Path) | ConvertFrom-Json
    $entries = @($parsedDatabase)
    if ($entries.Count -eq 0) {
        throw 'Upstream compile database has no Box3D/test objects'
    }
    foreach ($entry in $entries) {
        if ($entry.PSObject.Properties.Name -notcontains 'file' -or
                $entry.file -notmatch '(?i)[\\/]box3d(?:-src)?[\\/](src|shared|test)[\\/].+\.(c|cc|cpp|cxx)$') {
            throw "Upstream compile database contains an unexpected source entry: $($entry.file)"
        }
    }
    $relevant = $entries
    $libraryEntries = @($entries | Where-Object { $_.file -match '[\\/]src[\\/]' })
    $testEntries = @($entries | Where-Object { $_.file -match '[\\/]test[\\/]' })
    if ($libraryEntries.Count -eq 0 -or $testEntries.Count -eq 0) {
        throw 'Upstream compile database does not cover both Box3D and test objects'
    }

    $expectedRuntime = if ($Configuration -eq 'Debug') { 'MTd' } else { 'MT' }
    foreach ($entry in $relevant) {
        $commandProperty = $entry.PSObject.Properties['command']
        $argumentsProperty = $entry.PSObject.Properties['arguments']
        $command = if ($null -ne $commandProperty) {
            [string]$commandProperty.Value
        } elseif ($null -ne $argumentsProperty) {
            @($argumentsProperty.Value) -join ' '
        } else {
            throw "Upstream compile entry has no command or arguments: $($entry.file)"
        }
        if ($command -notmatch '(?i)(?:^|[\\/])cl\.exe(?:\s|$)') {
            throw "Upstream object was not compiled with MSVC cl.exe: $($entry.file)"
        }
        if ($command -match '(?i)(?:^|\s)[/-]MDd?(?:\s|$)') {
            throw "Upstream object has forbidden dynamic CRT: $($entry.file)"
        }
        $runtimeFlags = [regex]::Matches(
            $command, '(?i)(?:^|\s)[/-](MTd|MT|MDd|MD)(?=\s|$)')
        if ($runtimeFlags.Count -ne 1 -or
                $runtimeFlags[0].Groups[1].Value -ine $expectedRuntime) {
            throw "Upstream object must have exactly one /${expectedRuntime} CRT flag: $($entry.file)"
        }
        if ($command -match '(?i)(?:^|\s)(?:[/-]fp:fast|-ffast-math)(?:\s|$)') {
            throw "Upstream object has forbidden floating-point flag: $($entry.file)"
        }
        if ($Configuration -eq 'Debug') {
            $optimizationFlags = [regex]::Matches(
                $command, '(?i)(?:^|\s)[/-](Od|O1|O2|Ox)(?=\s|$)')
            if ($optimizationFlags.Count -ne 1 -or
                    $optimizationFlags[0].Groups[1].Value -ine 'Od') {
                throw "Debug upstream object has conflicting optimization flags: $($entry.file)"
            }
            if ($command -match '(?i)(?:^|\s)[/-](?:GL|DNDEBUG)(?:\s|$)') {
                throw "Debug upstream object has Release contamination: $($entry.file)"
            }
        } else {
            $optimizationFlags = [regex]::Matches(
                $command, '(?i)(?:^|\s)[/-](Od|O1|O2|Ox)(?=\s|$)')
            if ($optimizationFlags.Count -ne 1 -or
                    $optimizationFlags[0].Groups[1].Value -ine 'O2') {
                throw "Release upstream object has conflicting optimization flags: $($entry.file)"
            }
            if ($command -notmatch '(?i)(?:^|\s)[/-]DNDEBUG(?:\s|$)') {
                throw "Release upstream object is missing /DNDEBUG: $($entry.file)"
            }
            if ($command -match '(?i)(?:^|\s)[/-](?:MTd|RTC\w*|Od|ZI)(?:\s|$)') {
                throw "Release upstream object has Debug contamination: $($entry.file)"
            }
        }
    }
}

Export-ModuleMember `
    -Function Reset-NinhoUpstreamBuildDirectory, Assert-NinhoUpstreamCompileDatabase
