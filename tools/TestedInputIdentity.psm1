Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1')

function Get-NinhoCanonicalTestedInputContent {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$RelativePath
    )
    [byte[]]$bytes = [IO.File]::ReadAllBytes($Path)
    $binaryExtension = $RelativePath -match '(?i)\.(blend|glb|png|wav|dll|exe|pck|avi|jpg|jpeg|webp|ttf|otf)$'
    if ($binaryExtension -or $bytes -contains 0) {
        return [pscustomobject]@{ mode='binary'; bytes=$bytes }
    }
    $strictUtf8 = [Text.UTF8Encoding]::new($false, $true)
    try { $text = $strictUtf8.GetString($bytes) }
    catch { return [pscustomobject]@{ mode='binary'; bytes=$bytes } }
    $canonicalText = $text.Replace("`r`n", "`n").Replace("`r", "`n")
    return [pscustomobject]@{
        mode = 'text_utf8_lf'
        bytes = [Text.UTF8Encoding]::new($false).GetBytes($canonicalText)
    }
}

function Resolve-NinhoTestedInputPath {
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][string]$RelativePath
    )
    $relative = $RelativePath.Replace('\','/')
    if ([string]::IsNullOrWhiteSpace($relative) -or
            [IO.Path]::IsPathRooted($relative) -or $relative.Contains(':')) {
        throw "tested input path must be repository-relative: $RelativePath"
    }
    foreach ($segment in $relative.Split('/')) {
        if ($segment -ceq '..' -or $segment -ceq '.' -or [string]::IsNullOrEmpty($segment)) {
            throw "tested input path is not canonical: $RelativePath"
        }
    }
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    $path = [IO.Path]::GetFullPath((Join-Path $rootPath $relative))
    if (-not $path.StartsWith(
            $rootPath + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "tested input escapes repository: $RelativePath"
    }
    Assert-NinhoNoReparseAncestors -Path $path -AllowedRoot $rootPath | Out-Null
    return $path
}

function Get-NinhoTestedInputIdentity {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][object[]]$RelativePaths
    )
    [string[]]$paths = @($RelativePaths | ForEach-Object {
        ([string]$_).Replace('\','/')
    })
    [Array]::Sort($paths, [StringComparer]::Ordinal)
    $rows = [Collections.Generic.List[object]]::new()
    $aggregate = [Text.StringBuilder]::new()
    $previous = $null
    $fileAlgorithm = [Security.Cryptography.SHA256]::Create()
    try {
    foreach ($relative in $paths) {
        if ($null -ne $previous -and $relative -ceq $previous) {
            throw "tested input path is duplicated: $relative"
        }
        $previous = $relative
        $path = Resolve-NinhoTestedInputPath -Root $Root -RelativePath $relative
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "tested input missing: $relative"
        }
        $content = Get-NinhoCanonicalTestedInputContent -Path $path -RelativePath $relative
        $size = [int64]$content.bytes.Length
        # BitConverter formats the digest in one native call. Piping the 32
        # bytes through ForEach-Object instead cost more than the hashing
        # itself once this set grew past a hundred files, and this identity is
        # recomputed by every foundation and vertical slice gate.
        $hash = [BitConverter]::ToString(
            $fileAlgorithm.ComputeHash($content.bytes)).Replace('-','').ToLowerInvariant()
        $rows.Add([ordered]@{
            path=$relative; mode=$content.mode; size_bytes=$size; sha256=$hash
        })
        $null = $aggregate.Append($relative).Append("`0").Append($content.mode).Append("`0")
        $null = $aggregate.Append($size).Append("`0").Append($hash).Append("`n")
    }
    } finally { $fileAlgorithm.Dispose() }
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $fingerprint = [BitConverter]::ToString($algorithm.ComputeHash(
            [Text.Encoding]::UTF8.GetBytes($aggregate.ToString()))).Replace(
            '-','').ToLowerInvariant()
    } finally { $algorithm.Dispose() }
    return [pscustomobject]@{ files=@($rows); sha256=$fingerprint }
}

function Get-NinhoIdentityProperty {
    param([Parameter(Mandatory)]$Document, [Parameter(Mandatory)][string]$Name)
    if ($Document.PSObject.Properties.Name -cnotcontains $Name) {
        throw 'tested-content identity header mismatch'
    }
    return $Document.$Name
}

function Assert-NinhoTestedInputIdentity {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)]$Document,
        [Parameter(Mandatory)]$ExpectedIdentity,
        [string]$ExpectedSourceRevision = ''
    )
    $sourceRevision = [string](Get-NinhoIdentityProperty -Document $Document -Name source_revision)
    $schema = [string](Get-NinhoIdentityProperty -Document $Document -Name tested_inputs_schema)
    $aggregateHash = [string](Get-NinhoIdentityProperty -Document $Document -Name tested_inputs_sha256)
    if ($sourceRevision -notmatch '^[0-9a-f]{40}$' -or
            $schema -cne 'ninho.tested-inputs.v2' -or
            $aggregateHash -notmatch '^[0-9a-f]{64}$' -or
            (-not [string]::IsNullOrEmpty($ExpectedSourceRevision) -and
                $sourceRevision -cne $ExpectedSourceRevision)) {
        throw 'tested-content identity header mismatch'
    }
    $rootPath = [IO.Path]::GetFullPath($Root)
    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & git -c "safe.directory=$rootPath" -C $rootPath `
            merge-base --is-ancestor $sourceRevision HEAD 2>$null
        $gitExitCode = $LASTEXITCODE
    } finally { $ErrorActionPreference = $previousErrorActionPreference }
    if ($gitExitCode -ne 0) {
        throw 'source revision is not an ancestor of the current HEAD'
    }
    if ($ExpectedIdentity.sha256 -cne $aggregateHash) {
        throw 'tested inputs aggregate SHA-256 mismatch'
    }
    $declaredInputs = @((Get-NinhoIdentityProperty -Document $Document -Name tested_inputs))
    if ($declaredInputs.Count -ne $ExpectedIdentity.files.Count) {
        throw 'tested input file set mismatch'
    }
    for ($index = 0; $index -lt $declaredInputs.Count; ++$index) {
        $declared = $declaredInputs[$index]
        $actual = $ExpectedIdentity.files[$index]
        if ($declared.path -cne $actual.path -or $declared.mode -cne $actual.mode -or
                [int64]$declared.size_bytes -ne $actual.size_bytes -or
                $declared.sha256 -cne $actual.sha256) {
            throw "tested input mismatch at index $index"
        }
    }
}

Export-ModuleMember -Function Get-NinhoCanonicalTestedInputContent,Get-NinhoTestedInputIdentity,Assert-NinhoTestedInputIdentity
