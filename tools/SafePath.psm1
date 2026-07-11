Set-StrictMode -Version Latest

function Assert-NinhoNoReparseAncestors {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$Path,
        [Parameter(Mandatory)] [string]$AllowedRoot
    )

    if (-not [System.IO.Path]::IsPathRooted($Path) -or
            -not [System.IO.Path]::IsPathRooted($AllowedRoot)) {
        throw 'Target and allowed-root paths must be absolute'
    }
    $target = [System.IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    $root = [System.IO.Path]::GetFullPath($AllowedRoot).TrimEnd('\', '/')
    $rootPrefix = $root + [System.IO.Path]::DirectorySeparatorChar
    if ($target -eq $root -or -not $target.StartsWith(
            $rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Target is outside the allowed root: $target"
    }

    $relative = $target.Substring($rootPrefix.Length)
    $segments = $relative -split '[\\/]'
    $current = $root
    foreach ($segment in @('') + $segments) {
        if ($segment) {
            $current = Join-Path $current $segment
        }
        if (-not (Test-Path -LiteralPath $current)) {
            break
        }
        $item = Get-Item -Force -LiteralPath $current
        if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Refusing destructive operation through reparse point: $current"
        }
        $resolved = (Resolve-Path -LiteralPath $current).Path.TrimEnd('\', '/')
        $expected = [System.IO.Path]::GetFullPath($current).TrimEnd('\', '/')
        if (-not [string]::Equals(
                $resolved, $expected,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Resolved path differs from verified ancestor: $resolved"
        }
    }
    return $target
}

Export-ModuleMember -Function Assert-NinhoNoReparseAncestors
