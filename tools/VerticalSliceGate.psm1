Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force

function Assert-NinhoProperty {
    param([object]$Object, [string]$Name, [string]$Context)
    if ($null -eq $Object -or $Object.PSObject.Properties.Name -cnotcontains $Name) {
        throw "$Context missing $Name"
    }
    return $Object.$Name
}

function Assert-NinhoRelativeArtifactPath {
    param([string]$Path, [string]$ArtifactRoot)
    if ([IO.Path]::IsPathRooted($Path)) { throw "artifact path must be relative: $Path" }
    $root = [IO.Path]::GetFullPath($ArtifactRoot).TrimEnd('\','/')
    $full = [IO.Path]::GetFullPath((Join-Path $root $Path))
    if (-not $full.StartsWith($root + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "artifact path escapes root: $Path"
    }
    Assert-NinhoNoReparseAncestors -Path $full -AllowedRoot $root | Out-Null
    return $full
}

function Test-NinhoManifestRelativePath {
    param([string]$RelativePath)
    if ([string]::IsNullOrWhiteSpace($RelativePath) -or
            [IO.Path]::IsPathRooted($RelativePath) -or $RelativePath.Contains(':')) {
        return $false
    }
    foreach ($segment in $RelativePath.Replace('\','/').Split('/')) {
        if ($segment -ceq '..' -or $segment -ceq '.' -or [string]::IsNullOrEmpty($segment)) {
            return $false
        }
    }
    return $true
}

function Get-NinhoOrdinalSortedStrings {
    param([object[]]$Values)
    [string[]]$items = @($Values | ForEach-Object { [string]$_ })
    [Array]::Sort($items, [StringComparer]::Ordinal)
    return $items
}

function Get-NinhoCausalFrameIndex {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][int[]]$Mapping,
        [Parameter(Mandatory)][ValidateRange(0,[int]::MaxValue)][int]$SourceTransitionFrame,
        [ValidateRange(0,[int]::MaxValue)][int]$PostTransitionFrames = 0
    )
    if ($Mapping.Count -eq 0) { throw 'causal frame mapping is empty' }
    for ($index = 1; $index -lt $Mapping.Count; ++$index) {
        if ($Mapping[$index] -le $Mapping[$index - 1]) {
            throw 'causal frame mapping must be strictly increasing'
        }
    }
    $target = [Math]::Min([int64][int]::MaxValue,
        [int64]$SourceTransitionFrame + [int64]$PostTransitionFrames)
    for ($index = 0; $index -lt $Mapping.Count; ++$index) {
        if ([int64]$Mapping[$index] -ge $target) { return $index }
    }
    throw 'causal tail unavailable in downsample mapping'
}

function Set-NinhoRequiredDownsampleFrame {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][int[]]$Mapping,
        [Parameter(Mandatory)][ValidateRange(0,[int]::MaxValue)][int]$RequiredSourceFrame
    )
    if ($Mapping.Count -lt 3) { throw 'downsample mapping cannot accept a required internal frame' }
    for ($index = 1; $index -lt $Mapping.Count; ++$index) {
        if ($Mapping[$index] -le $Mapping[$index - 1]) {
            throw 'downsample mapping must be strictly increasing'
        }
    }
    if ($Mapping -contains $RequiredSourceFrame) { return [int[]]@($Mapping) }
    $bestIndex = -1
    $bestDistance = [int64]::MaxValue
    for ($index = 1; $index -lt $Mapping.Count - 1; ++$index) {
        if ($RequiredSourceFrame -le $Mapping[$index - 1] -or
                $RequiredSourceFrame -ge $Mapping[$index + 1]) { continue }
        $distance = [Math]::Abs([int64]$Mapping[$index] - [int64]$RequiredSourceFrame)
        if ($distance -lt $bestDistance) {
            $bestDistance = $distance
            $bestIndex = $index
        }
    }
    if ($bestIndex -lt 0) { throw 'required source frame cannot be injected into downsample mapping' }
    [int[]]$result = @($Mapping)
    $result[$bestIndex] = $RequiredSourceFrame
    return $result
}

function Stop-NinhoProcessTree {
    param([Parameter(Mandatory)][int]$ProcessId)
    foreach ($child in @(Get-CimInstance Win32_Process -Filter "ParentProcessId = $ProcessId" -ErrorAction SilentlyContinue)) {
        Stop-NinhoProcessTree -ProcessId ([int]$child.ProcessId)
    }
    Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue
}

function Invoke-NinhoTimedProcess {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string[]]$ArgumentList,
        [Parameter(Mandatory)][ValidateRange(1,[int]::MaxValue)][int]$TimeoutMs,
        [Parameter(Mandatory)][string]$StdoutPath,
        [Parameter(Mandatory)][string]$StderrPath,
        [string]$WorkingDirectory = '',
        [string]$FatalMarker = 'NINHO_CAPTURE_FATAL reason=timeout'
    )
    $encodedArguments = @($ArgumentList | ForEach-Object {
        $argument = [string]$_
        if ($argument -match '[\s"]') { '"' + $argument.Replace('"','\"') + '"' } else { $argument }
    })
    $parameters = @{
        FilePath=$FilePath; ArgumentList=$encodedArguments; PassThru=$true; WindowStyle='Hidden'
        RedirectStandardOutput=$StdoutPath; RedirectStandardError=$StderrPath
    }
    if (-not [string]::IsNullOrWhiteSpace($WorkingDirectory)) { $parameters.WorkingDirectory = $WorkingDirectory }
    $process = Start-Process @parameters
    try {
        if (-not $process.WaitForExit($TimeoutMs)) {
            Stop-NinhoProcessTree -ProcessId $process.Id
            $process.WaitForExit()
            [IO.File]::AppendAllText($StderrPath, "$FatalMarker timeout_ms=$TimeoutMs`n", [Text.UTF8Encoding]::new($false))
            throw "process timed out after $TimeoutMs ms: $FilePath"
        }
        $process.WaitForExit()
        return [pscustomobject]@{ ExitCode=[int]$process.ExitCode; ProcessId=[int]$process.Id }
    } finally { $process.Dispose() }
}

function Get-NinhoTestedInputPaths {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Root)
    $rootPath = [IO.Path]::GetFullPath($Root)
    $paths = @(& git -c "safe.directory=$rootPath" -C $rootPath `
        ls-files --cached --others --exclude-standard)
    if ($LASTEXITCODE -ne 0) { throw 'unable to enumerate tested inputs with git' }
    $filtered = @($paths | ForEach-Object { ([string]$_).Replace('\','/') } | Where-Object {
		$_ -match '^(\.gitattributes$|\.gitignore$|art/|cmake/|game/|native/|tools/|third_party/|CMakeLists\.txt$|CMakePresets\.json$|THIRD_PARTY_NOTICES\.md$|README\.md$|docs/gameplay/vertical-slice-content-schema\.md$|docs/superpowers/specs/2026-07-11-vertical-slice(-balance-correction)?-design\.md$|docs/superpowers/plans/2026-07-11-vertical-slice-(implementation|balance-correction)\.md$)' -and
        $_ -notmatch '^(docs/gameplay/evidence/|docs/art/goldens/)' -and
        $_ -notmatch '^game/bin/.*\.(dll|pdb|ilk)$' -and
        $_ -notmatch '(^|/)\.godot/'
    })
    [Array]::Sort($filtered, [StringComparer]::Ordinal)
    return $filtered
}

function Assert-NinhoIndependentReviews {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Reviews,
        [Parameter(Mandatory)][string]$ExpectedTestedInputsSha256
    )
    if ($Reviews.schema -cne 'ninho.vertical-slice.reviews.v1' -or
            $Reviews.tested_inputs_sha256 -cne $ExpectedTestedInputsSha256) {
        throw 'reviews manifest schema/tested-content mismatch'
    }
    $reviewerIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($role in 'code','architecture','gameplay','art') {
        $review = @($Reviews.reviews | Where-Object role -CEQ $role)
        if ($review.Count -ne 1 -or $review[0].verdict -cne 'approved' -or
                $review[0].critical -ne 0 -or $review[0].important -ne 0) {
            throw "blocking or missing independent review: $role"
        }
        $reviewerId = [string]$review[0].reviewer_id
        if ($reviewerId -notmatch '^/root(?:/[a-z][a-z0-9_]*)+$') {
            throw "reviewer ID is not canonical for role ${role}: $reviewerId"
        }
        if (-not $reviewerIds.Add($reviewerId)) {
            throw "reviewer IDs must be unique across canonical roles: $reviewerId"
        }
    }
    if (@($Reviews.reviews).Count -ne 4) {
        throw 'reviews manifest must contain exactly four canonical roles'
    }
}

function Get-NinhoTestedInputs {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Root)
    $rows = [Collections.Generic.List[object]]::new()
    $aggregate = [Text.StringBuilder]::new()
    foreach ($relative in @(Get-NinhoTestedInputPaths -Root $Root)) {
        $path = Assert-NinhoRelativeArtifactPath $relative $Root
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "tested input missing: $relative" }
        $size = [int64](Get-Item -LiteralPath $path).Length
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
        $rows.Add([ordered]@{ path=$relative; size_bytes=$size; sha256=$hash })
        $null = $aggregate.Append($relative).Append("`0").Append($size).Append("`0").Append($hash).Append("`n")
    }
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $fingerprint = -join ($algorithm.ComputeHash([Text.Encoding]::UTF8.GetBytes($aggregate.ToString())) |
            ForEach-Object { $_.ToString('x2') })
    } finally { $algorithm.Dispose() }
    return [pscustomobject]@{ files=@($rows); sha256=$fingerprint }
}

function Assert-NinhoInputFeedbackMarkers {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][object[]]$Markers,
        [Parameter(Mandatory)][string]$Context
    )
    $items = @($Markers)
    if ($items.Count -ne 3) {
        throw "$Context must contain exactly three causal markers"
    }
    $expected = @(
        @{ command_id='set_aim_center'; theta_degrees=0.0; phase_degrees=0.0; speed=10.5 },
        @{ command_id='set_aim_right'; theta_degrees=2.0; phase_degrees=0.0; speed=8.0 },
        @{ command_id='set_aim_left'; theta_degrees=-2.0; phase_degrees=0.0; speed=8.0 }
    )
    $commands = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $hashes = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    for ($index = 0; $index -lt $items.Count; ++$index) {
        $marker = $items[$index]
        foreach ($property in 'sample_index','command_id','theta_degrees','phase_degrees','speed','observed_tick','preview_hash') {
            $hasProperty = if ($marker -is [Collections.IDictionary]) {
                $marker.Contains($property)
            } else {
                $marker.PSObject.Properties.Name -ccontains $property
            }
            if (-not $hasProperty) {
                throw "$Context causal marker $index missing $property"
            }
        }
        $previewHash = [string]$marker.preview_hash
        if ([int]$marker.sample_index -ne $index -or
                [string]$marker.command_id -cne [string]$expected[$index].command_id -or
                [double]$marker.theta_degrees -ne [double]$expected[$index].theta_degrees -or
                [double]$marker.phase_degrees -ne [double]$expected[$index].phase_degrees -or
                [double]$marker.speed -ne [double]$expected[$index].speed -or
                [int64]$marker.observed_tick -lt 0 -or
                $previewHash -notmatch '^-?[0-9]+$' -or $previewHash -ceq '0') {
            throw "$Context causal marker $index does not match its input command"
        }
        if (-not $commands.Add([string]$marker.command_id) -or -not $hashes.Add($previewHash)) {
            throw "$Context has duplicated causal command or preview hash"
        }
    }
}

function Resolve-NinhoPackageManifestOutput {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][object[]]$Output,
        [Parameter(Mandatory)][string]$ExpectedOutputRoot
    )
    $prefix = 'NINHO_PACKAGE_MANIFEST '
    $markers = @($Output | ForEach-Object { [string]$_ } | Where-Object {
        $_.StartsWith($prefix, [StringComparison]::Ordinal)
    })
    if ($markers.Count -ne 1) {
        throw "package output must contain exactly one manifest marker; found $($markers.Count)"
    }
    try { $claim = $markers[0].Substring($prefix.Length) | ConvertFrom-Json }
    catch { throw "package manifest marker JSON is invalid: $($_.Exception.Message)" }
    if ($null -eq $claim -or $claim.PSObject.Properties.Name -cnotcontains 'path' -or
            [string]::IsNullOrWhiteSpace([string]$claim.path)) {
        throw 'package manifest marker path is missing'
    }
    $outputRoot = [IO.Path]::GetFullPath($ExpectedOutputRoot).TrimEnd('\','/')
    $manifestPath = [IO.Path]::GetFullPath([string]$claim.path)
    if (-not $manifestPath.StartsWith(
            $outputRoot + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "package manifest is outside expected output root: $manifestPath"
    }
    $expectedManifest = [IO.Path]::GetFullPath((Join-Path $outputRoot 'manifest.sha256.json'))
    if (-not [string]::Equals($manifestPath, $expectedManifest, [StringComparison]::OrdinalIgnoreCase)) {
        throw "package manifest path is not canonical: $manifestPath"
    }
    Assert-NinhoNoReparseAncestors -Path $manifestPath -AllowedRoot $outputRoot | Out-Null
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw "package manifest does not exist: $manifestPath"
    }
    return $manifestPath
}

function Assert-NinhoVerticalSliceEvidence {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$EvidencePath,
        [Parameter(Mandatory)][string]$ArtifactRoot,
        [Parameter(Mandatory)][ValidateSet('Debug','Release')][string]$ExpectedConfiguration,
        [Parameter(Mandatory)][ValidatePattern('^[0-9a-f]{40}$')][string]$ExpectedCommit
    )

    if (-not (Test-Path -LiteralPath $EvidencePath -PathType Leaf)) {
        throw "vertical slice evidence missing: $EvidencePath"
    }
    try { $doc = Get-Content -Raw -LiteralPath $EvidencePath | ConvertFrom-Json }
    catch { throw "vertical slice evidence is invalid JSON: $($_.Exception.Message)" }
    if ($doc.schema -cne 'ninho.vertical-slice.evidence.v1' -or $doc.schema_version -ne 1) {
        throw 'vertical slice evidence schema/version mismatch'
    }
    if ($doc.configuration -cne $ExpectedConfiguration) {
        throw "vertical slice evidence configuration mismatch: $($doc.configuration)"
    }
    if ($doc.commit -cne $ExpectedCommit) {
        throw "vertical slice evidence is stale: expected commit $ExpectedCommit, got $($doc.commit)"
    }
    if ($doc.source_revision -cne $doc.commit -or
            $doc.tested_inputs_schema -cne 'ninho.tested-inputs.v1' -or
            $doc.tested_inputs_sha256 -notmatch '^[0-9a-f]{64}$') {
        throw 'tested-content identity header mismatch'
    }
    $rootForInputs = [IO.Path]::GetFullPath($ArtifactRoot)
    & git -c "safe.directory=$rootForInputs" -C $rootForInputs `
        merge-base --is-ancestor $doc.source_revision HEAD 2>$null
    if ($LASTEXITCODE -ne 0) { throw 'source revision is not an ancestor of the current HEAD' }
    $actualInputs = Get-NinhoTestedInputs -Root $rootForInputs
    if ($actualInputs.sha256 -cne [string]$doc.tested_inputs_sha256) {
        throw 'tested inputs aggregate SHA-256 mismatch'
    }
    $declaredInputs = @($doc.tested_inputs)
    if ($declaredInputs.Count -ne $actualInputs.files.Count) { throw 'tested input file set mismatch' }
    for ($inputIndex = 0; $inputIndex -lt $declaredInputs.Count; ++$inputIndex) {
        $declared = $declaredInputs[$inputIndex]
        $actual = $actualInputs.files[$inputIndex]
        if ($declared.path -cne $actual.path -or [int64]$declared.size_bytes -ne $actual.size_bytes -or
                $declared.sha256 -cne $actual.sha256) {
            throw "tested input mismatch at index $inputIndex"
        }
    }
    if ($doc.canonical_state_contract -cne 'canonical_state_v1') {
        throw 'canonical_state_v1 contract missing'
    }
    foreach ($name in 'cpu','gpu','ram_bytes','os') {
        $null = Assert-NinhoProperty $doc.hardware $name 'hardware'
    }
    $routes = @($doc.routes)
    $expectedRoutes = @{
        virela_win='Victory'; structural_win='Victory'; no_ability_loss='Defeat'
    }
    foreach ($name in $expectedRoutes.Keys) {
        $route = @($routes | Where-Object name -CEQ $name)
        if ($route.Count -ne 1 -or $route[0].outcome -cne $expectedRoutes[$name]) {
            throw "deterministic route mismatch: $name"
        }
        if (@($route[0].hashes).Count -lt 2 -or
                @($route[0].hashes | Select-Object -Unique).Count -ne 1) {
            throw "repeat hash mismatch: $name"
        }
        if ($route[0].ordered_events_sha256 -notmatch '^[0-9a-f]{64}$') {
            throw "ordered event hash missing: $name"
        }
    }
    foreach ($name in 'foundation','slice_tests','restart_20','abi','scanner','assets','smokes','box3d_only') {
        if ((Assert-NinhoProperty $doc.acceptance $name 'acceptance') -cne $true) {
            throw "acceptance gate failed: $name"
        }
    }
    if ([double]$doc.physics.step_p95_ms -gt 8.0 -or [double]$doc.physics.limit_ms -ne 8.0) {
        throw 'physics p95 exceeds 8 ms'
    }
    $renderers = @($doc.renderers)
    foreach ($rendererName in 'Vulkan','OpenGL') {
        $renderer = @($renderers | Where-Object name -CEQ $rendererName)
        if ($renderer.Count -ne 1) { throw "renderer evidence missing or duplicated: $rendererName" }
        foreach ($scale in 100,150) {
            $sample = @($renderer[0].scales | Where-Object ui_scale -EQ $scale)
            if ($sample.Count -ne 1) { throw "renderer $rendererName missing UI scale $scale" }
            if ([double]$sample[0].frame_p95_ms -gt 16.67 -or
                    [double]$sample[0].frame_p99_ms -gt 25.0 -or
                    [double]$sample[0].max_hitch_ms -gt 50.0 -or
                    [double]$sample[0].input_feedback_p95_ms -gt 33.4) {
                throw "renderer performance threshold failed: $rendererName/$scale"
            }
            if ([int]$sample[0].frames_measured -lt 300 -or
                    [int]$sample[0].input_feedback_samples -ne 3 -or
                    [int]$sample[0].post_vfx_frames -lt 30 -or
                    $sample[0].rupture_observed -cne $true -or
                    $sample[0].vfx_observed -cne $true) {
                throw "renderer performance evidence lacks rupture/VFX/input samples: $rendererName/$scale"
            }
            Assert-NinhoInputFeedbackMarkers -Markers @($sample[0].input_feedback_markers) `
                -Context "$rendererName/$scale"
        }
        if ($renderer[0].capture.frames -ne 300 -or
                $renderer[0].capture.width -ne 1920 -or
                $renderer[0].capture.height -ne 1080) {
            throw "renderer capture contract failed: $rendererName"
        }
    }
    foreach ($golden in 'overview','aim','virela','vulnerable_impact','result') {
        if (@($doc.goldens | Where-Object { $_ -ceq $golden }).Count -ne 1) {
            throw "golden missing: $golden"
        }
        $metadata = @($doc.golden_metadata | Where-Object name -CEQ $golden)
        if ($metadata.Count -ne 1 -or $metadata[0].frame -lt 0 -or $metadata[0].frame -ge 300 -or
                $metadata[0].source_frame -lt 0 -or
                $metadata[0].source_transition_frame -lt 0 -or
                $metadata[0].source_transition_frame -gt $metadata[0].source_frame -or
                $metadata[0].tick -lt 0 -or $metadata[0].renderer -cne 'Vulkan' -or
                $metadata[0].sha256 -notmatch '^[0-9a-f]{64}$' -or
                $metadata[0].exposure -le 0 -or -not $metadata[0].camera) {
            throw "golden metadata invalid: $golden"
        }
        $configurationDirectory = $ExpectedConfiguration.ToLowerInvariant()
        $expectedPrefix = "docs/art/goldens/vertical-slice/$configurationDirectory/"
        if (-not ([string]$metadata[0].path).StartsWith($expectedPrefix, [StringComparison]::Ordinal)) {
            throw "golden is not configuration-specific: $golden"
        }
    }
    $resultGolden = @($doc.golden_metadata | Where-Object name -CEQ 'result')[0]
    if ($resultGolden.phase -cne 'result' -or $resultGolden.outcome -cne 'victory') {
        throw 'result golden is not a terminal victory'
    }
    $impactGolden = @($doc.golden_metadata | Where-Object name -CEQ 'vulnerable_impact')[0]
    if ($impactGolden.event -notlike 'damage_applied:vulnerable:anchor:*') {
        throw 'vulnerable impact golden is not tied to a positive Anchor damage event'
    }
    if ($doc.rubric.critical -ne 0 -or $doc.rubric.important -ne 0) {
        throw 'independent review has blocking findings'
    }
    if ($doc.playtest.status -cne 'unavailable' -or
            $doc.playtest.substitute -cne 'independent_agents' -or
            $doc.playtest.legal_limit -cne 'not_legal_advice') {
        throw 'playtest availability/legal limit contract mismatch'
    }
    if ($doc.clean_room.approved -cne $true -or
            $doc.clean_room.comparative_review -cne 'approved') {
        throw 'clean-room review failed'
    }
    $assetProvenancePath = Assert-NinhoRelativeArtifactPath 'tools/art/vertical_slice_asset_manifest.json' $ArtifactRoot
    $audioProvenancePath = Assert-NinhoRelativeArtifactPath 'tools/audio/audio_manifest.json' $ArtifactRoot
    foreach ($provenancePath in $assetProvenancePath,$audioProvenancePath) {
        if (-not (Test-Path -LiteralPath $provenancePath -PathType Leaf)) {
            throw "clean-room provenance manifest missing: $provenancePath"
        }
    }
    $provenancePayload =
        (Get-FileHash -Algorithm SHA256 -LiteralPath $assetProvenancePath).Hash.ToLowerInvariant() + '|' +
        (Get-FileHash -Algorithm SHA256 -LiteralPath $audioProvenancePath).Hash.ToLowerInvariant()
    $provenanceAlgorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $actualProvenanceHash = -join ($provenanceAlgorithm.ComputeHash(
            [Text.Encoding]::UTF8.GetBytes($provenancePayload)) | ForEach-Object { $_.ToString('x2') })
    } finally { $provenanceAlgorithm.Dispose() }
    if ([string]$doc.clean_room.provenance_manifest_sha256 -cne $actualProvenanceHash) {
        throw 'clean-room provenance manifest SHA-256 mismatch'
    }
    if ($ExpectedConfiguration -ceq 'Release') {
        if ($doc.package.launch_from_space_path -cne 'passed' -or
                $doc.package.manifest_sha256 -notmatch '^[0-9a-f]{64}$') {
            throw 'Release package launch/hash evidence is invalid'
        }
        $packageRoot = Assert-NinhoRelativeArtifactPath $doc.package.path $ArtifactRoot
        if (-not (Test-Path -LiteralPath $packageRoot -PathType Container)) {
            throw "Release package directory missing: $($doc.package.path)"
        }
        $packageManifest = Join-Path $packageRoot 'manifest.sha256.json'
        Assert-NinhoNoReparseAncestors -Path $packageManifest -AllowedRoot $packageRoot | Out-Null
        if (-not (Test-Path -LiteralPath $packageManifest -PathType Leaf)) {
            throw 'Release package manifest missing'
        }
        $actualPackageHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $packageManifest).Hash.ToLowerInvariant()
        if ($actualPackageHash -cne [string]$doc.package.manifest_sha256) {
            throw "Release package manifest SHA-256 mismatch: $actualPackageHash"
        }
        $packageDocument = Get-Content -Raw -LiteralPath $packageManifest | ConvertFrom-Json
        if ($packageDocument.schema_version -ne 1 -or $packageDocument.algorithm -cne 'SHA-256' -or
                $packageDocument.configuration -cne 'Release') {
            throw 'Release package manifest header mismatch'
        }
        $packagePaths = @($packageDocument.files.path)
        foreach ($required in @(
            'NinhoOrbital.exe','NinhoOrbital.pck','ninho_physics.windows.template_release.x86_64.dll',
            'package-content.json','build-contract.json','licenses/THIRD_PARTY_NOTICES.md','licenses/sbom.spdx.json',
            'licenses/box3d.LICENSE.txt','licenses/godot.LICENSE.txt','licenses/godot.COPYRIGHT.txt',
            'licenses/godot-export-templates.LICENSE.txt','licenses/godot-cpp.LICENSE.txt',
            'licenses/nlohmann-json.LICENSE.txt')) {
            if ($packagePaths -cnotcontains $required) { throw "Release package manifest missing required distribution file: $required" }
        }
        $declaredPackagePaths = [Collections.Generic.List[string]]::new()
        foreach ($entry in @($packageDocument.files)) {
            $relative = [string]$entry.path
            if (-not (Test-NinhoManifestRelativePath $relative)) {
                throw "Release package manifest contains unsafe path: $relative"
            }
            if ($declaredPackagePaths.Contains($relative)) {
                throw "Release package manifest duplicates path: $relative"
            }
            $file = Join-Path $packageRoot $relative.Replace('/', [IO.Path]::DirectorySeparatorChar)
            Assert-NinhoNoReparseAncestors -Path $file -AllowedRoot $packageRoot | Out-Null
            if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
                throw "Release package manifest references missing file: $relative"
            }
            if ([int64](Get-Item -LiteralPath $file).Length -ne [int64]$entry.size_bytes) {
                throw "Release package size mismatch: $relative"
            }
            $fileHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $file).Hash.ToLowerInvariant()
            if ($fileHash -cne [string]$entry.sha256) {
                throw "Release package hash mismatch: $relative"
            }
            $declaredPackagePaths.Add($relative)
        }
        $sortedDeclaredPackagePaths = @(Get-NinhoOrdinalSortedStrings @($declaredPackagePaths))
        if (($declaredPackagePaths -join "`n") -cne ($sortedDeclaredPackagePaths -join "`n")) {
            throw 'Release package manifest is not canonically sorted'
        }
        $actualPackagePaths = @(
            Get-ChildItem -LiteralPath $packageRoot -Recurse -File | ForEach-Object {
                $relative = $_.FullName.Substring($packageRoot.Length).TrimStart('\','/').Replace('\','/')
                if ($relative -cne 'manifest.sha256.json') { $relative }
            }
        )
        $actualPackagePaths = @(Get-NinhoOrdinalSortedStrings $actualPackagePaths)
        if (($actualPackagePaths -join "`n") -cne ($sortedDeclaredPackagePaths -join "`n")) {
            throw 'Release package file set differs from the canonical manifest'
        }
        $releaseDllName = 'ninho_physics.windows.template_release.x86_64.dll'
        $runtimeRow = @($packageDocument.files | Where-Object path -CEQ $releaseDllName)
        if ($packageDocument.runtime_extension.path -cne $releaseDllName -or
                $packageDocument.runtime_extension.source_path -cne "game/bin/$releaseDllName" -or
                $runtimeRow.Count -ne 1 -or
                [string]$packageDocument.runtime_extension.source_sha256 -cne [string]$runtimeRow[0].sha256) {
            throw 'Release package runtime DLL snapshot mismatch'
        }
        $buildContractPath = Join-Path $packageRoot 'build-contract.json'
        $buildContract = Get-Content -Raw -LiteralPath $buildContractPath | ConvertFrom-Json
        $buildContractHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $buildContractPath).Hash.ToLowerInvariant()
        $contentDocument = Get-Content -Raw -LiteralPath (Join-Path $packageRoot 'package-content.json') | ConvertFrom-Json
        if ($buildContract.schema -cne 'ninho.native-build-contract.v1' -or
                $buildContract.configuration -cne 'Release' -or
                $buildContract.build_testing -cne $false -or
                $buildContract.test_facades -cne $false -or
                $buildContract.NINHO_ENABLE_TEST_FACADES -cne 'absent' -or
                $packageDocument.native_build_contract.path -cne 'build-contract.json' -or
                $packageDocument.native_build_contract.sha256 -cne $buildContractHash -or
                $packageDocument.native_build_contract.build_testing -cne $false -or
                $packageDocument.native_build_contract.test_facades -cne $false -or
                $contentDocument.native_build_contract.path -cne 'build-contract.json' -or
                $contentDocument.native_build_contract.sha256 -cne $buildContractHash -or
                $contentDocument.native_build_contract.build_testing -cne $false -or
                $contentDocument.native_build_contract.test_facades -cne $false) {
            throw 'Release package native build provenance mismatch'
        }
        if ($packagePaths -ccontains 'ninho_physics.windows.template_debug.x86_64.dll') {
            throw 'Release package contains the Debug runtime DLL'
        }
    }

    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $sourceHashByPath = @{}
    foreach ($artifact in @($doc.source_artifacts)) {
        if ($artifact.sha256 -notmatch '^[0-9a-f]{64}$') { throw "invalid SHA-256 for $($artifact.path)" }
        if (-not $seen.Add([string]$artifact.path)) { throw "duplicate source artifact: $($artifact.path)" }
        $path = Assert-NinhoRelativeArtifactPath $artifact.path $ArtifactRoot
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "source artifact missing: $($artifact.path)" }
        $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
        if ($actual -cne $artifact.sha256) {
            throw "source artifact SHA-256 mismatch for $($artifact.path): $actual"
        }
        $sourceHashByPath[[string]$artifact.path] = [string]$artifact.sha256
        if ([IO.Path]::GetExtension($path) -in '.log','.txt') {
            $log = [IO.File]::ReadAllText($path)
            foreach ($forbidden in 'ERROR:','SCRIPT ERROR:','WARNING:') {
                if ($log.Contains($forbidden)) { throw "source log contains forbidden diagnostic: $forbidden" }
            }
        }
    }
    if ($seen.Count -lt 2) { throw 'source artifact set is incomplete' }
    $vulkan = @($doc.renderers | Where-Object name -CEQ 'Vulkan')[0]
    foreach ($metadata in @($doc.golden_metadata)) {
        if (-not $sourceHashByPath.ContainsKey([string]$metadata.path) -or
                $sourceHashByPath[[string]$metadata.path] -cne [string]$metadata.sha256) {
            throw "golden metadata/source hash mismatch: $($metadata.name)"
        }
    }
    foreach ($referenceName in 'capture_manifest','reviews_manifest') {
        $reference = $doc.$referenceName
        if ($reference.sha256 -notmatch '^[0-9a-f]{64}$') { throw "$referenceName SHA-256 invalid" }
        $referencePath = Assert-NinhoRelativeArtifactPath $reference.path $ArtifactRoot
        if (-not (Test-Path -LiteralPath $referencePath -PathType Leaf)) { throw "$referenceName missing" }
        $referenceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $referencePath).Hash.ToLowerInvariant()
        if ($referenceHash -cne [string]$reference.sha256) { throw "$referenceName SHA-256 mismatch" }
    }
    $captureManifestPath = Assert-NinhoRelativeArtifactPath $doc.capture_manifest.path $ArtifactRoot
    $captureManifest = Get-Content -Raw -LiteralPath $captureManifestPath | ConvertFrom-Json
    if ($captureManifest.schema -cne 'ninho.vertical-slice.capture.v1' -or
            $captureManifest.configuration -cne $ExpectedConfiguration) {
        throw 'capture manifest schema/configuration mismatch'
    }
    foreach ($field in 'routes','renderers','goldens','golden_metadata','source_artifacts') {
        $declaredJson = $doc.$field | ConvertTo-Json -Depth 30 -Compress
        $manifestJson = $captureManifest.$field | ConvertTo-Json -Depth 30 -Compress
        if ($declaredJson -cne $manifestJson) { throw "evidence claim differs from capture manifest: $field" }
    }
    $routeRawPath = Assert-NinhoRelativeArtifactPath $captureManifest.route_raw_log.path $ArtifactRoot
    $routeRawHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $routeRawPath).Hash.ToLowerInvariant()
    if ($routeRawHash -cne [string]$captureManifest.route_raw_log.sha256 -or
            $sourceHashByPath[[string]$captureManifest.route_raw_log.path] -cne $routeRawHash) {
        throw 'raw route log hash/source mismatch'
    }
    $routeRawText = [IO.File]::ReadAllText($routeRawPath)
    $canonicalBaseline = [regex]::Match($routeRawText, '(?m)^\[TRACE\] canonical_playthrough_v3 (\d+) (\d+) (\d+)\r?$')
    $canonicalRepeat = [regex]::Match($routeRawText, '(?m)^\[TRACE\] canonical_playthrough_v3_repeat (\d+) (\d+) (\d+)\r?$')
    if (-not $canonicalBaseline.Success -or -not $canonicalRepeat.Success) {
        $routeRawLength = [int64](Get-Item -LiteralPath $routeRawPath).Length
        throw "raw canonical route traces missing: path=$routeRawPath bytes=$routeRawLength " +
            "sha256=$routeRawHash baseline=$($canonicalBaseline.Success) repeat=$($canonicalRepeat.Success)"
    }
    $routeIndex = 0
    foreach ($routeName in 'virela_win','structural_win','no_ability_loss') {
        $routeIndex += 1
        $route = @($doc.routes | Where-Object name -CEQ $routeName)[0]
        if ($route.hashes[0] -cne $canonicalBaseline.Groups[$routeIndex].Value -or
                $route.hashes[1] -cne $canonicalRepeat.Groups[$routeIndex].Value) {
            throw "route canonical hash differs from raw trace: $routeName"
        }
        $payloadHashes = [Collections.Generic.List[string]]::new()
        foreach ($runName in 'baseline','repeat') {
            $payload = [regex]::Match($routeRawText,
                "(?m)^\[TRACE\] ordered_events_v1 $([regex]::Escape($routeName)) $runName ([0-9a-f]+)\r?`$")
            if (-not $payload.Success -or ($payload.Groups[1].Value.Length % 2) -ne 0) {
                throw "raw ordered event payload missing: $routeName/$runName"
            }
            $hex = $payload.Groups[1].Value
            $bytes = New-Object byte[] ($hex.Length / 2)
            for ($byteIndex = 0; $byteIndex -lt $bytes.Length; ++$byteIndex) {
                $bytes[$byteIndex] = [Convert]::ToByte($hex.Substring($byteIndex * 2, 2), 16)
            }
            $algorithm = [Security.Cryptography.SHA256]::Create()
            try { $payloadHashes.Add(-join ($algorithm.ComputeHash($bytes) | ForEach-Object { $_.ToString('x2') })) }
            finally { $algorithm.Dispose() }
        }
        if ($payloadHashes[0] -cne $payloadHashes[1] -or
                $payloadHashes[0] -cne [string]$route.ordered_events_sha256) {
            throw "ordered event SHA-256 differs from raw payload: $routeName"
        }
    }
    foreach ($renderer in @($doc.renderers)) {
        foreach ($scale in @($renderer.scales)) {
            $metricsPath = Assert-NinhoRelativeArtifactPath $scale.metrics_path $ArtifactRoot
            if (-not (Test-Path -LiteralPath $metricsPath -PathType Leaf)) { throw "runtime metrics missing: $($scale.metrics_path)" }
            $metricsHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $metricsPath).Hash.ToLowerInvariant()
            if ($metricsHash -cne [string]$scale.metrics_sha256) { throw "runtime metrics SHA-256 mismatch: $($scale.metrics_path)" }
            $metrics = Get-Content -Raw -LiteralPath $metricsPath | ConvertFrom-Json
            if ($metrics.schema -cne 'ninho.vertical-slice.runtime-metrics.v1') { throw 'runtime metrics schema mismatch' }
            Assert-NinhoInputFeedbackMarkers -Markers @($metrics.input_feedback_markers) `
                -Context "runtime metrics $($renderer.name)/$($scale.ui_scale)"
            $claimedMarkers = @($scale.input_feedback_markers) | ConvertTo-Json -Depth 10 -Compress
            $metricMarkers = @($metrics.input_feedback_markers) | ConvertTo-Json -Depth 10 -Compress
            if ($claimedMarkers -cne $metricMarkers) {
                throw "runtime input marker claim mismatch: $($renderer.name)/$($scale.ui_scale)"
            }
            foreach ($pair in @(
                @('frame_p95_ms','frame_p95_ms'), @('frame_p99_ms','frame_p99_ms'),
                @('max_hitch_ms','max_hitch_ms'), @('input_feedback_p95_ms','input_feedback_p95_ms'),
                @('input_feedback_samples','input_feedback_samples'), @('physics_step_p95_ms','physics_step_p95_ms'),
                @('frames_measured','frames_measured'), @('rupture_observed','rupture_observed'),
                @('vfx_observed','vfx_observed'), @('post_vfx_frames','post_vfx_frames'))) {
                if ([string]$scale.($pair[0]) -cne [string]$metrics.($pair[1])) {
                    throw "runtime metric claim mismatch: $($renderer.name)/$($scale.ui_scale)/$($pair[0])"
                }
            }
        }
        $capturePath = Assert-NinhoRelativeArtifactPath $renderer.capture.path $ArtifactRoot
        if (-not $sourceHashByPath.ContainsKey([string]$renderer.capture.path) -or
                $sourceHashByPath[[string]$renderer.capture.path] -cne [string]$renderer.capture.hash) {
            throw "capture hash/artifact mismatch: $($renderer.name)"
        }
        $captureLogPath = Assert-NinhoRelativeArtifactPath $renderer.capture.log_path $ArtifactRoot
        if (-not $sourceHashByPath.ContainsKey([string]$renderer.capture.log_path) -or
                $sourceHashByPath[[string]$renderer.capture.log_path] -cne [string]$renderer.capture.log_hash) {
            throw "capture log hash/artifact mismatch: $($renderer.name)"
        }
        $ffprobe = (Get-Command ffprobe.exe -ErrorAction Stop).Source
        $probe = & $ffprobe -v error -select_streams v:0 `
            -show_entries stream=width,height,nb_frames -of json $capturePath | ConvertFrom-Json
        if ($LASTEXITCODE -ne 0 -or @($probe.streams).Count -ne 1 -or
                [int]$probe.streams[0].width -ne [int]$renderer.capture.width -or
                [int]$probe.streams[0].height -ne [int]$renderer.capture.height -or
                [int]$probe.streams[0].nb_frames -ne [int]$renderer.capture.frames) {
            throw "capture ffprobe claim mismatch: $($renderer.name)"
        }
        $sourcePath = Assert-NinhoRelativeArtifactPath $renderer.capture.source_path $ArtifactRoot
        if (-not $sourceHashByPath.ContainsKey([string]$renderer.capture.source_path) -or
                $sourceHashByPath[[string]$renderer.capture.source_path] -cne [string]$renderer.capture.source_hash) {
            throw "source capture hash/artifact mismatch: $($renderer.name)"
        }
        $sourceProbe = & $ffprobe -v error -select_streams v:0 `
            -show_entries stream=width,height,nb_frames -of json $sourcePath | ConvertFrom-Json
        if ($LASTEXITCODE -ne 0 -or @($sourceProbe.streams).Count -ne 1 -or
                [int]$sourceProbe.streams[0].width -ne [int]$renderer.capture.width -or
                [int]$sourceProbe.streams[0].height -ne [int]$renderer.capture.height -or
                [int]$sourceProbe.streams[0].nb_frames -ne [int]$renderer.capture.source_frames) {
            throw "source capture ffprobe claim mismatch: $($renderer.name)"
        }
        $mapping = @($renderer.capture.downsample_source_frames)
        if ($mapping.Count -ne 300 -or $mapping[0] -ne 0 -or
                $mapping[299] -ne [int]$renderer.capture.source_frames - 1) {
            throw "capture downsample mapping mismatch: $($renderer.name)"
        }
        for ($mappingIndex = 1; $mappingIndex -lt $mapping.Count; ++$mappingIndex) {
            if ([int]$mapping[$mappingIndex] -le [int]$mapping[$mappingIndex - 1]) {
                throw "capture downsample mapping is not strictly increasing: $($renderer.name)"
            }
        }
    }
    foreach ($metadata in @($doc.golden_metadata)) {
        if ([int]$vulkan.capture.downsample_source_frames[[int]$metadata.frame] -ne [int]$metadata.source_frame) {
            throw "golden/downsample source mapping mismatch: $($metadata.name)"
        }
    }
    $goldenVerificationRoot = Join-Path ([IO.Path]::GetFullPath($ArtifactRoot)) 'artifacts\vertical-slice-golden-verification'
    Assert-NinhoNoReparseAncestors -Path $goldenVerificationRoot -AllowedRoot $ArtifactRoot | Out-Null
    New-Item -ItemType Directory -Force -Path $goldenVerificationRoot | Out-Null
    $ffmpeg = (Get-Command ffmpeg.exe -ErrorAction Stop).Source
    $vulkanSourcePath = Assert-NinhoRelativeArtifactPath $vulkan.capture.source_path $ArtifactRoot
    try {
        foreach ($metadata in @($doc.golden_metadata)) {
            $token = [Guid]::NewGuid().ToString('N')
            $derivedGolden = Join-Path $goldenVerificationRoot "$token.png"
            $deriveStdout = Join-Path $goldenVerificationRoot "$token.stdout.log"
            $deriveStderr = Join-Path $goldenVerificationRoot "$token.stderr.log"
            $derived = Invoke-NinhoTimedProcess -FilePath $ffmpeg -ArgumentList @(
                '-v','error','-y','-i',$vulkanSourcePath,'-vf',"select=eq(n\,$([int]$metadata.source_frame))",
                '-frames:v','1',$derivedGolden
            ) -TimeoutMs 120000 -StdoutPath $deriveStdout -StderrPath $deriveStderr `
                -FatalMarker "NINHO_CAPTURE_FATAL name=verify-golden-$($metadata.name) reason=timeout"
            if ($derived.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $derivedGolden -PathType Leaf)) {
                throw "golden cannot be derived from raw source: $($metadata.name)"
            }
            $derivedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $derivedGolden).Hash.ToLowerInvariant()
            if ($derivedHash -cne [string]$metadata.sha256) {
                throw "golden differs from raw source frame: $($metadata.name)"
            }
        }
    } finally {
        Assert-NinhoNoReparseAncestors -Path $goldenVerificationRoot -AllowedRoot $ArtifactRoot | Out-Null
        Remove-Item -LiteralPath $goldenVerificationRoot -Recurse -Force
    }
    $reviewsPath = Assert-NinhoRelativeArtifactPath $doc.reviews_manifest.path $ArtifactRoot
    $reviews = Get-Content -Raw -LiteralPath $reviewsPath | ConvertFrom-Json
    Assert-NinhoIndependentReviews -Reviews $reviews `
        -ExpectedTestedInputsSha256 ([string]$doc.tested_inputs_sha256)
    $vulkanLog = [IO.File]::ReadAllText((Assert-NinhoRelativeArtifactPath $vulkan.capture.log_path $ArtifactRoot))
    $stateProofPattern = '(?m)^NINHO_CAPTURE_STATE frame=(\d+) tick=(\d+) phase=(\S+) outcome=(\S+) camera=(\([^)]+\)) exposure=([0-9.]+)\s*$'
    $stateProofs = @([regex]::Matches($vulkanLog, $stateProofPattern))
    foreach ($stateGoldenName in 'overview','aim','virela','result') {
        $metadata = @($doc.golden_metadata | Where-Object name -CEQ $stateGoldenName)[0]
        if ($metadata.event -cne 'phase_transition') {
            throw "golden phase event contract mismatch: $stateGoldenName"
        }
        $proof = @($stateProofs | Where-Object {
            [int]$_.Groups[1].Value -eq [int]$metadata.source_transition_frame
        })
        if ($proof.Count -ne 1 -or [int64]$proof[0].Groups[2].Value -ne [int64]$metadata.tick -or
                $proof[0].Groups[3].Value -cne [string]$metadata.phase -or
                $proof[0].Groups[4].Value -cne [string]$metadata.outcome -or
                $proof[0].Groups[5].Value -cne [string]$metadata.camera -or
                [Math]::Abs([double]$proof[0].Groups[6].Value - [double]$metadata.exposure) -gt 0.000001) {
            throw "golden state metadata differs from verified Vulkan log: $($metadata.name)"
        }
    }
    $impactProofPattern = '(?m)^NINHO_CAPTURE_EVENT frame=(\d+) tick=(\d+) kind=(\S+) profile=(\S+) affected=(\d+) damage=([0-9.]+) camera=(\([^)]+\)) exposure=([0-9.]+)\s*$'
    $impactProofs = @([regex]::Matches($vulkanLog, $impactProofPattern) | Where-Object {
        [int]$_.Groups[1].Value -eq [int]$impactGolden.source_transition_frame
    })
    $impactDamage = [double](([string]$impactGolden.event).Split(':')[-1])
    if ($impactProofs.Count -ne 1 -or
            [int64]$impactProofs[0].Groups[2].Value -ne [int64]$impactGolden.tick -or
            $impactProofs[0].Groups[3].Value -cne 'damage_applied' -or
            $impactProofs[0].Groups[4].Value -cne 'vulnerable' -or
            $impactProofs[0].Groups[5].Value -cne '200' -or
            [Math]::Abs([double]$impactProofs[0].Groups[6].Value - $impactDamage) -gt 0.000001 -or
            $impactProofs[0].Groups[7].Value -cne [string]$impactGolden.camera -or
            [Math]::Abs([double]$impactProofs[0].Groups[8].Value - [double]$impactGolden.exposure) -gt 0.000001) {
        throw 'vulnerable impact metadata differs from verified Vulkan log'
    }
    return $doc
}

function Get-NinhoRouteEvidenceFromLog {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Text)
    $pattern = '(?m)^NINHO_ROUTE_HASH name=(\S+) outcome=(Victory|Defeat) canonical_state_v1=(\d+) events_sha256=([0-9a-f]{64})\s*$'
    $matches = [regex]::Matches($Text, $pattern)
    $result = [Collections.Generic.List[object]]::new()
    foreach ($name in 'virela_win','structural_win','no_ability_loss') {
        $selected = @($matches | Where-Object { $_.Groups[1].Value -ceq $name })
        if ($selected.Count -ne 2) { throw "route evidence requires two runs: $name" }
        $hashes = @($selected | ForEach-Object { $_.Groups[3].Value })
        $events = @($selected | ForEach-Object { $_.Groups[4].Value })
        $outcomes = @($selected | ForEach-Object { $_.Groups[2].Value })
        if (@($hashes | Select-Object -Unique).Count -ne 1 -or
                @($events | Select-Object -Unique).Count -ne 1 -or
                @($outcomes | Select-Object -Unique).Count -ne 1) {
            throw "route repeat hash/order/outcome mismatch: $name"
        }
        $result.Add([ordered]@{
            name = $name
            outcome = $outcomes[0]
            hashes = $hashes
            ordered_events_sha256 = $events[0]
        })
    }
    return $result
}

function Invoke-NinhoLimitedRetry {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][ValidateRange(1,2)][int]$MaximumAttempts,
        [Parameter(Mandatory)][scriptblock]$Operation
    )
    $results = [Collections.Generic.List[object]]::new()
    for ($attempt = 1; $attempt -le $MaximumAttempts; ++$attempt) {
        $result = & $Operation $attempt
        $hasExitCode = $null -ne $result -and (
            ($result -is [Collections.IDictionary] -and $result.Contains('ExitCode')) -or
            $result.PSObject.Properties.Name -ccontains 'ExitCode')
        if (-not $hasExitCode) {
            throw "$Name operation returned no ExitCode"
        }
        $results.Add($result)
        if ([int]$result.ExitCode -eq 0) {
            $result | Add-Member -NotePropertyName AttemptCount -NotePropertyValue $attempt -Force
            $result | Add-Member -NotePropertyName Attempts -NotePropertyValue @($results) -Force
            return $result
        }
    }
    $codes = @($results | ForEach-Object { [string]$_.ExitCode }) -join ', '
    throw "$Name failed after $MaximumAttempts attempts (exit codes: $codes)"
}

Export-ModuleMember -Function Assert-NinhoVerticalSliceEvidence,Assert-NinhoRelativeArtifactPath,Get-NinhoRouteEvidenceFromLog,Invoke-NinhoLimitedRetry,Get-NinhoTestedInputPaths,Get-NinhoTestedInputs,Get-NinhoCausalFrameIndex,Set-NinhoRequiredDownsampleFrame,Invoke-NinhoTimedProcess,Resolve-NinhoPackageManifestOutput,Assert-NinhoInputFeedbackMarkers,Assert-NinhoIndependentReviews
