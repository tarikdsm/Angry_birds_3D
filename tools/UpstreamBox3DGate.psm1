Set-StrictMode -Version Latest

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
    if (Test-Path -LiteralPath $target) {
        $item = Get-Item -Force -LiteralPath $target
        if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Refusing to remove an upstream reparse point: $target"
        }
        $resolved = (Resolve-Path -LiteralPath $target).Path.TrimEnd('\', '/')
        if (-not [string]::Equals(
                $resolved,
                $target,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Resolved upstream path differs from the verified target: $resolved"
        }
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
    $relevant = @($entries | Where-Object {
        $_.file -match '[\\/](src|shared|test)[\\/].+\.(c|cc|cpp|cxx)$'
    })
    if ($relevant.Count -eq 0) {
        throw 'Upstream compile database has no Box3D/test objects'
    }
    $libraryEntries = @($relevant | Where-Object { $_.file -match '[\\/]src[\\/]' })
    $testEntries = @($relevant | Where-Object { $_.file -match '[\\/]test[\\/]' })
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
        if ($command -notmatch
                "(?i)(?:^|\s)[/-]$expectedRuntime(?:\s|$)") {
            throw "Upstream object is missing /${expectedRuntime}: $($entry.file)"
        }
        if ($command -match '(?i)(?:^|\s)(?:[/-]fp:fast|-ffast-math)(?:\s|$)') {
            throw "Upstream object has forbidden floating-point flag: $($entry.file)"
        }
        if ($Configuration -eq 'Debug') {
            if ($command -match '(?i)(?:^|\s)[/-](?:O2|GL|DNDEBUG)(?:\s|$)') {
                throw "Debug upstream object has Release contamination: $($entry.file)"
            }
        } else {
            if ($command -notmatch '(?i)(?:^|\s)[/-]O2(?:\s|$)') {
                throw "Release upstream object is missing /O2: $($entry.file)"
            }
            if ($command -match '(?i)(?:^|\s)[/-](?:MTd|RTC1|Od|ZI)(?:\s|$)') {
                throw "Release upstream object has Debug contamination: $($entry.file)"
            }
        }
    }
}

Export-ModuleMember `
    -Function Reset-NinhoUpstreamBuildDirectory, Assert-NinhoUpstreamCompileDatabase
