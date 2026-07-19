Set-StrictMode -Version Latest

Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1')

function Get-NinhoRequiredCaptureProperty {
    param([object]$Object, [string]$Name, [string]$Context)

    if ($null -eq $Object -or $Object.PSObject.Properties.Name -cnotcontains $Name) {
        throw "$Context missing $Name"
    }
    return $Object.$Name
}

function Test-NinhoSafeCaptureRelativePath {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or
            [IO.Path]::IsPathRooted($Path) -or $Path.Contains(':')) {
        return $false
    }
    foreach ($segment in $Path.Replace('\','/').Split('/')) {
        if ([string]::IsNullOrEmpty($segment) -or $segment -in @('.','..')) {
            return $false
        }
    }
    return $true
}

function Resolve-NinhoCaptureRelativePath {
    param([string]$Root, [string]$RelativePath)

    if (-not (Test-NinhoSafeCaptureRelativePath $RelativePath)) {
        throw "capture manifest contains unsafe artifact path: $RelativePath"
    }
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    $fullPath = [IO.Path]::GetFullPath((Join-Path $rootPath $RelativePath))
    if (-not $fullPath.StartsWith(
            $rootPath + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "capture manifest contains unsafe artifact path: $RelativePath"
    }
    Assert-NinhoNoReparseAncestors -Path $fullPath -AllowedRoot $rootPath |
        Out-Null
    return $fullPath
}

function Assert-NinhoVerticalSliceCaptureManifest {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][ValidateSet('Debug','Release')]
        [string]$Configuration,
        [Parameter(Mandatory)][string]$ArtifactDirectory,
        [Parameter(Mandatory)][string]$ManifestPath
    )

    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    $artifactPath = [IO.Path]::GetFullPath($ArtifactDirectory).TrimEnd('\','/')
    $expectedManifestPath = [IO.Path]::GetFullPath(
        (Join-Path $artifactPath 'capture-manifest.json'))
    $actualManifestPath = [IO.Path]::GetFullPath($ManifestPath)
    if (-not [string]::Equals(
            $actualManifestPath, $expectedManifestPath,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "capture manifest must use canonical path: $expectedManifestPath"
    }
    Assert-NinhoNoReparseAncestors -Path $actualManifestPath -AllowedRoot $rootPath |
        Out-Null
    if (-not (Test-Path -LiteralPath $actualManifestPath -PathType Leaf)) {
        throw "capture manifest missing: $actualManifestPath"
    }

    $manifest = Get-Content -Raw -LiteralPath $actualManifestPath | ConvertFrom-Json
    if ($manifest.schema -cne 'ninho.vertical-slice.capture.v1') {
        throw 'capture manifest schema mismatch'
    }
    if ($manifest.configuration -cne $Configuration) {
        throw "capture manifest configuration mismatch: expected $Configuration"
    }

    $sourceArtifacts = @(
        Get-NinhoRequiredCaptureProperty $manifest 'source_artifacts' 'capture manifest')
    if ($sourceArtifacts.Count -eq 0) {
        throw 'capture manifest source_artifacts is empty'
    }
    $sourceHashByPath = [Collections.Generic.Dictionary[string,string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    foreach ($artifact in $sourceArtifacts) {
        $relativePath = [string](
            Get-NinhoRequiredCaptureProperty $artifact 'path' 'capture artifact')
        $declaredHash = [string](
            Get-NinhoRequiredCaptureProperty $artifact 'sha256' 'capture artifact')
        if (-not (Test-NinhoSafeCaptureRelativePath $relativePath)) {
            throw "capture manifest contains unsafe artifact path: $relativePath"
        }
        if ($declaredHash -cnotmatch '^[0-9a-f]{64}$') {
            throw "capture artifact SHA-256 is invalid: $relativePath"
        }
        if ($sourceHashByPath.ContainsKey($relativePath)) {
            throw "capture manifest duplicates artifact path: $relativePath"
        }
        $fullPath = Resolve-NinhoCaptureRelativePath $rootPath $relativePath
        if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
            throw "capture artifact missing: $relativePath"
        }
        $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $fullPath).Hash.ToLowerInvariant()
        if ($actualHash -cne $declaredHash) {
            throw "capture artifact SHA-256 mismatch: $relativePath"
        }
        $sourceHashByPath.Add($relativePath, $declaredHash)
    }

    $canonicalGoldenNames = @(
        'overview','aim','virela','vulnerable_impact','result')
    $goldens = @(
        Get-NinhoRequiredCaptureProperty $manifest 'goldens' 'capture manifest')
    $goldenMetadata = @(
        Get-NinhoRequiredCaptureProperty $manifest 'golden_metadata' 'capture manifest')
    if ($goldens.Count -ne $canonicalGoldenNames.Count -or
            $goldenMetadata.Count -ne $canonicalGoldenNames.Count) {
        throw 'capture manifest must contain exactly five canonical goldens'
    }
    foreach ($goldenName in $canonicalGoldenNames) {
        if (@($goldens | Where-Object { [string]$_ -ceq $goldenName }).Count -ne 1) {
            throw "capture manifest golden set mismatch: $goldenName"
        }
        $metadata = @($goldenMetadata | Where-Object name -CEQ $goldenName)
        if ($metadata.Count -ne 1) {
            throw "capture manifest golden metadata mismatch: $goldenName"
        }
        $relativePath = [string](
            Get-NinhoRequiredCaptureProperty $metadata[0] 'path' 'golden metadata')
        $declaredHash = [string](
            Get-NinhoRequiredCaptureProperty $metadata[0] 'sha256' 'golden metadata')
        if (-not (Test-NinhoSafeCaptureRelativePath $relativePath) -or
                $declaredHash -cnotmatch '^[0-9a-f]{64}$' -or
                -not $sourceHashByPath.ContainsKey($relativePath) -or
                $sourceHashByPath[$relativePath] -cne $declaredHash) {
            throw "capture manifest golden/source binding mismatch: $goldenName"
        }
    }

    return [pscustomobject]@{
        ManifestPath = $actualManifestPath
        Manifest = $manifest
        ManifestSha256 = (
            Get-FileHash -Algorithm SHA256 -LiteralPath $actualManifestPath
        ).Hash.ToLowerInvariant()
    }
}

function Invoke-NinhoVerticalSliceCaptureWorkflow {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][ValidateSet('Debug','Release')]
        [string]$Configuration,
        [Parameter(Mandatory)][string]$ArtifactDirectory,
        [Parameter(Mandatory)][ValidateSet('Legacy','CaptureOnly','UseExistingCapture')]
        [string]$Mode,
        [scriptblock]$CaptureOperation
    )

    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    $preset = $Configuration.ToLowerInvariant()
    $expectedArtifactPath = [IO.Path]::GetFullPath(
        (Join-Path $rootPath "artifacts\vertical-slice\$preset")).TrimEnd('\','/')
    $artifactPath = [IO.Path]::GetFullPath($ArtifactDirectory).TrimEnd('\','/')
    if (-not [string]::Equals(
            $artifactPath, $expectedArtifactPath,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "vertical slice capture must use canonical artifact directory: $expectedArtifactPath"
    }
    Assert-NinhoNoReparseAncestors -Path $artifactPath -AllowedRoot $rootPath |
        Out-Null

    $manifestPath = Join-Path $artifactPath 'capture-manifest.json'
    if ($Mode -cne 'UseExistingCapture') {
        if ($null -eq $CaptureOperation) {
            throw "$Mode requires a capture operation"
        }
        if (Test-Path -LiteralPath $artifactPath) {
            $resolvedArtifactPath = (Resolve-Path -LiteralPath $artifactPath).Path
            if (-not [string]::Equals(
                    $resolvedArtifactPath, $artifactPath,
                    [StringComparison]::OrdinalIgnoreCase)) {
                throw "refusing to clean unexpected artifact directory: $resolvedArtifactPath"
            }
            Assert-NinhoNoReparseAncestors `
                -Path $resolvedArtifactPath -AllowedRoot $rootPath | Out-Null
            Remove-Item -LiteralPath $resolvedArtifactPath -Recurse -Force
        }
        New-Item -ItemType Directory -Force -Path $artifactPath | Out-Null
        $captureOutput = @(& $CaptureOperation $artifactPath)
        if ($captureOutput.Count -eq 0 -or
                [string]::IsNullOrWhiteSpace([string]$captureOutput[-1])) {
            throw 'capture operation returned no manifest path'
        }
        $reportedManifestPath = [IO.Path]::GetFullPath([string]$captureOutput[-1])
        if (-not [string]::Equals(
                $reportedManifestPath, [IO.Path]::GetFullPath($manifestPath),
                [StringComparison]::OrdinalIgnoreCase)) {
            throw "capture operation returned non-canonical manifest path: $reportedManifestPath"
        }
    }

    return Assert-NinhoVerticalSliceCaptureManifest `
        -Root $rootPath -Configuration $Configuration `
        -ArtifactDirectory $artifactPath -ManifestPath $manifestPath
}

Export-ModuleMember -Function `
    Assert-NinhoVerticalSliceCaptureManifest,Invoke-NinhoVerticalSliceCaptureWorkflow
