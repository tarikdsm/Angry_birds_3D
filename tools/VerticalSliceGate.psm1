Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'TestedInputIdentity.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'ToolchainIntegrity.psm1') -Force
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

function Get-NinhoCompactDownsampleSelectExpression {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][int[]]$Mapping,
        [Parameter(Mandatory)][ValidateRange(1,[int]::MaxValue)][int]$SourceFrameCount
    )

    if ($Mapping.Count -ne 300) {
        throw 'compact downsample mapping must contain exactly 300 frames'
    }
    if ($SourceFrameCount -lt 300) {
        throw 'compact downsample source must contain at least 300 frames'
    }
    if ($Mapping[0] -ne 0 -or $Mapping[299] -ne $SourceFrameCount - 1) {
        throw 'compact downsample mapping must preserve source endpoints'
    }
    for ($index = 0; $index -lt $Mapping.Count; ++$index) {
        if ($Mapping[$index] -lt 0 -or $Mapping[$index] -ge $SourceFrameCount) {
            throw 'compact downsample mapping frame is outside the source range'
        }
        if ($index -gt 0 -and $Mapping[$index] -le $Mapping[$index - 1]) {
            throw 'compact downsample mapping must be strictly increasing'
        }
    }

    $lastSourceFrame = $SourceFrameCount - 1
    $removed = [Collections.Generic.List[int]]::new()
    $added = [Collections.Generic.List[int]]::new()
    for ($index = 0; $index -lt 300; ++$index) {
        $baseline = [int][Math]::Round(
            [double]$index * [double]$lastSourceFrame / 299.0,
            [MidpointRounding]::AwayFromZero)
        if ($Mapping[$index] -ne $baseline) {
            $removed.Add($baseline)
            $added.Add($Mapping[$index])
        }
    }
    if ($removed.Count -gt 8) {
        throw 'compact downsample mapping has more than 8 sparse overrides'
    }

    $baselineExpression =
        "eq(n\,round(round(n*299/$lastSourceFrame)*$lastSourceFrame/299))"
    if ($removed.Count -eq 0) { return $baselineExpression }
    $removedExpression = @($removed | ForEach-Object { "eq(n\,$_)" }) -join '+'
    $addedExpression = @($added | ForEach-Object { "eq(n\,$_)" }) -join '+'
    return "$baselineExpression*not($removedExpression)+$addedExpression"
}

function Get-NinhoVirelaAbilityProof {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Text)

    $pattern = '(?m)^NINHO_CAPTURE_EVENT frame=(\d+) tick=(\d+) kind=(\S+) profile=(\S+) affected=(\d+) damage=([0-9.]+) camera=(\([^)]+\)) exposure=([0-9.]+)\s*$'
    $proofs = @([regex]::Matches($Text, $pattern) | Where-Object {
        ($_.Groups[3].Value -ceq 'ability_started' -and $_.Groups[4].Value -ceq 'vortex') -or
        ($_.Groups[3].Value -ceq 'ability_pulse' -and $_.Groups[4].Value -ceq 'virela_pulse')
    })
    if ($proofs.Count -eq 0) { throw 'capture has no Virela ability/VFX event' }

    $proof = $proofs[0]
    return [pscustomobject]@{
        frame = [int]$proof.Groups[1].Value
        tick = [int64]$proof.Groups[2].Value
        phase = 'flight_ability'
        outcome = 'none'
        event = "$($proof.Groups[3].Value):$($proof.Groups[4].Value)"
        camera = $proof.Groups[7].Value
        exposure = [double]$proof.Groups[8].Value
    }
}

function Stop-NinhoProcessTree {
    param([Parameter(Mandatory)][int]$ProcessId)
    foreach ($child in @(Get-CimInstance Win32_Process -Filter "ParentProcessId = $ProcessId" -ErrorAction SilentlyContinue)) {
        Stop-NinhoProcessTree -ProcessId ([int]$child.ProcessId)
    }
    $process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if ($null -eq $process) { return }
    try {
        Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue
        if (-not $process.HasExited) {
            $process.WaitForExit(3000) | Out-Null
        }
    } finally {
        $process.Dispose()
    }
}

function Get-NinhoIdentifiedProcessIds {
    param([Parameter(Mandatory)][string]$IdentityToken)

    return @(Get-CimInstance Win32_Process -ErrorAction Stop | Where-Object {
        $null -ne $_.CommandLine -and
        $_.CommandLine.IndexOf($IdentityToken, [StringComparison]::Ordinal) -ge 0
    } | ForEach-Object { [int]$_.ProcessId })
}

function Stop-NinhoIdentifiedProcessTrees {
    param([Parameter(Mandatory)][string]$IdentityToken)

    foreach ($identifiedPid in @(Get-NinhoIdentifiedProcessIds -IdentityToken $IdentityToken)) {
        Stop-NinhoProcessTree -ProcessId $identifiedPid
    }
}

function Add-NinhoTextWithRetry {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Text,
        [ValidateRange(1,[int]::MaxValue)][int]$RetryTimeoutMs = 3000
    )

    $watch = [Diagnostics.Stopwatch]::StartNew()
    try {
        do {
            try {
                [IO.File]::AppendAllText(
                    $Path, $Text, [Text.UTF8Encoding]::new($false))
                return
            } catch {
                $cause = if ($null -ne $_.Exception.InnerException) {
                    $_.Exception.InnerException
                } else {
                    $_.Exception
                }
                if ($cause -isnot [IO.IOException] -and
                        $cause -isnot [UnauthorizedAccessException]) {
                    throw
                }
                $remainingMs = $RetryTimeoutMs - [int]$watch.ElapsedMilliseconds
                if ($remainingMs -le 0) { throw }
                Start-Sleep -Milliseconds ([Math]::Min(50, $remainingMs))
            }
        } while ($true)
    } finally {
        $watch.Stop()
    }
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
        [string]$FatalMarker = 'NINHO_CAPTURE_FATAL reason=timeout',
        [string]$ProcessIdentityToken = ''
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
    $processDisposed = $false
    $watch = [Diagnostics.Stopwatch]::StartNew()
    try {
        $timedOut = -not $process.WaitForExit($TimeoutMs)
        if (-not $timedOut) {
            $process.WaitForExit()
        }
        if (-not $timedOut -and
                -not [string]::IsNullOrWhiteSpace($ProcessIdentityToken)) {
            do {
                $identifiedPids = @(
                    Get-NinhoIdentifiedProcessIds -IdentityToken $ProcessIdentityToken)
                if ($identifiedPids.Count -eq 0) { break }
                $remainingMs = $TimeoutMs - [int]$watch.ElapsedMilliseconds
                if ($remainingMs -le 0) {
                    $timedOut = $true
                    break
                }
                Start-Sleep -Milliseconds ([Math]::Min(50, $remainingMs))
            } while ($true)
        }
        if ($timedOut) {
            Stop-NinhoProcessTree -ProcessId $process.Id
            if (-not [string]::IsNullOrWhiteSpace($ProcessIdentityToken)) {
                Stop-NinhoIdentifiedProcessTrees -IdentityToken $ProcessIdentityToken
            }
            if (-not $process.HasExited) { $process.WaitForExit(3000) | Out-Null }
            $process.Dispose()
            $processDisposed = $true
            $timeoutMessage = "process timed out after $TimeoutMs ms: $FilePath"
            $markerFailure = ''
            try {
                Add-NinhoTextWithRetry -Path $StderrPath `
                    -Text "$FatalMarker timeout_ms=$TimeoutMs`n"
            } catch {
                $markerFailure = $_.Exception.Message
            }
            if (-not [string]::IsNullOrWhiteSpace($markerFailure)) {
                throw "$timeoutMessage; failed to persist timeout marker after bounded retry: $markerFailure"
            }
            throw $timeoutMessage
        }
        return [pscustomobject]@{ ExitCode=[int]$process.ExitCode; ProcessId=[int]$process.Id }
    } finally {
        $watch.Stop()
        if (-not $processDisposed) { $process.Dispose() }
    }
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

function Assert-NinhoAgentReviews {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Reviews,
        [Parameter(Mandatory)][string]$ExpectedTestedInputsSha256,
        [Parameter(Mandatory)][ValidatePattern('^[0-9a-f]{64}$')]
        [string]$ExpectedCaptureManifestSha256,
        [Parameter(Mandatory)][ValidateSet('Debug','Release')]
        [string]$ExpectedCaptureConfiguration,
        [Parameter(Mandatory)][object[]]$ExpectedGoldenMetadata
    )
    if ($Reviews.schema -cne 'ninho.vertical-slice.reviews.v2' -or
            $Reviews.schema_version -ne 2 -or
            $Reviews.tested_inputs_schema -cne 'ninho.tested-inputs.v2' -or
            $Reviews.tested_inputs_sha256 -cne $ExpectedTestedInputsSha256) {
        throw 'reviews manifest schema/tested-content mismatch'
    }
    foreach ($role in 'code','architecture','gameplay','art') {
        $review = @($Reviews.reviews | Where-Object role -CEQ $role)
        if ($review.Count -ne 1 -or $review[0].verdict -cne 'approved' -or
                $review[0].critical -ne 0 -or $review[0].important -ne 0) {
            throw "blocking or missing agent review: $role"
        }
        $reviewerId = [string]$review[0].reviewer_id
        if ([string]::IsNullOrWhiteSpace($reviewerId)) {
            throw "agent review attribution label is missing for role: $role"
        }
    }
    if (@($Reviews.reviews).Count -ne 4) {
        throw 'reviews manifest must contain exactly four canonical roles'
    }

    $artReview = @($Reviews.reviews | Where-Object role -CEQ 'art')[0]
    $bindings = @(Assert-NinhoProperty $artReview 'artifact_bindings' 'art review')
    $seenConfigurations = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($candidate in $bindings) {
        $candidateConfiguration = [string](
            Assert-NinhoProperty $candidate 'capture_configuration' 'art review artifact binding')
        if ($candidate.schema -cne 'ninho.vertical-slice.art-review-binding.v1' -or
                $candidateConfiguration -notin @('Debug','Release')) {
            throw 'art review artifact binding schema/configuration mismatch'
        }
        if (-not $seenConfigurations.Add($candidateConfiguration)) {
            throw "art review must contain exactly one binding for capture configuration: $candidateConfiguration"
        }
    }
    $matchingBindings = @($bindings | Where-Object capture_configuration -CEQ $ExpectedCaptureConfiguration)
    if ($matchingBindings.Count -ne 1) {
        throw "art review capture configuration mismatch: expected $ExpectedCaptureConfiguration"
    }
    $binding = $matchingBindings[0]
    if ([string]$binding.capture_manifest_sha256 -notmatch '^[0-9a-f]{64}$' -or
            $binding.capture_manifest_sha256 -cne $ExpectedCaptureManifestSha256) {
        throw 'art review capture manifest SHA-256 mismatch'
    }

    $canonicalGoldenNames = @('overview','aim','virela','vulnerable_impact','result')
    $declaredGoldens = @(Assert-NinhoProperty $binding 'goldens' 'art review artifact binding')
    if ($declaredGoldens.Count -ne $canonicalGoldenNames.Count) {
        throw 'art review artifact binding must contain exactly five canonical goldens'
    }
    foreach ($goldenName in $canonicalGoldenNames) {
        $expectedGolden = @($ExpectedGoldenMetadata | Where-Object name -CEQ $goldenName)
        $declaredGolden = @($declaredGoldens | Where-Object name -CEQ $goldenName)
        if ($expectedGolden.Count -ne 1 -or
                [string]$expectedGolden[0].sha256 -notmatch '^[0-9a-f]{64}$') {
            throw "expected golden metadata is invalid: $goldenName"
        }
        if ($declaredGolden.Count -ne 1 -or
                [string]$declaredGolden[0].sha256 -notmatch '^[0-9a-f]{64}$' -or
                $declaredGolden[0].sha256 -cne $expectedGolden[0].sha256) {
            throw "art review golden binding mismatch: $goldenName"
        }
    }
}

function Assert-NinhoCleanRoomReview {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Review,
        [Parameter(Mandatory)][ValidatePattern('^[0-9a-f]{64}$')]
        [string]$ExpectedAssetManifestSha256,
        [Parameter(Mandatory)][ValidatePattern('^[0-9a-f]{64}$')]
        [string]$ExpectedAudioManifestSha256
    )

    if ($Review.schema -cne 'ninho.vertical-slice.clean-room-review.v1' -or
            $Review.schema_version -ne 1) {
        throw 'clean-room review schema/version mismatch'
    }
    if ($Review.result -cne 'approved_no_confusing_similarity') {
        throw 'clean-room review is pending or not approved'
    }

    $reviewer = Assert-NinhoProperty $Review 'reviewer' 'clean-room review'
    $reviewerType = [string](Assert-NinhoProperty $reviewer 'type' 'clean-room reviewer')
    $reviewerId = [string](Assert-NinhoProperty $reviewer 'id' 'clean-room reviewer')
    if ([string]::IsNullOrWhiteSpace($reviewerId)) {
        throw 'clean-room reviewer attribution is missing'
    }
    if ($reviewerType -notin @('agent','human') -or
            ($reviewerType -ceq 'agent' -and -not $reviewerId.StartsWith('/root', [StringComparison]::Ordinal)) -or
            ($reviewerType -ceq 'human' -and $reviewerId.StartsWith('/root', [StringComparison]::Ordinal))) {
        throw 'clean-room reviewer type/attribution mismatch'
    }

    $reviewedAt = [DateTimeOffset]::MinValue
    if (-not [DateTimeOffset]::TryParse(
            [string]$Review.reviewed_utc,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::RoundtripKind,
            [ref]$reviewedAt)) {
        throw 'clean-room reviewed_utc is invalid'
    }
    if ($Review.legal_limit -cne 'not_legal_advice') {
        throw 'clean-room legal limit is missing'
    }

    $canonicalScope = @('names','logos','silhouettes','sounds','ui','layouts','promotional_material')
    $declaredScope = @(Assert-NinhoProperty $Review 'scope' 'clean-room review')
    if ($declaredScope.Count -ne $canonicalScope.Count) {
        throw 'clean-room review scope mismatch'
    }
    foreach ($scopeItem in $canonicalScope) {
        if (@($declaredScope | Where-Object { $_ -ceq $scopeItem }).Count -ne 1) {
            throw 'clean-room review scope mismatch'
        }
    }

    $manifests = Assert-NinhoProperty $Review 'manifests' 'clean-room review'
    $expectedManifests = @(
        @('assets','tools/art/vertical_slice_asset_manifest.json',$ExpectedAssetManifestSha256),
        @('audio','tools/audio/audio_manifest.json',$ExpectedAudioManifestSha256)
    )
    foreach ($expected in $expectedManifests) {
        $entry = Assert-NinhoProperty $manifests $expected[0] 'clean-room review manifests'
        if ($entry.path -cne $expected[1] -or
                [string]$entry.sha256 -notmatch '^[0-9a-f]{64}$' -or
                $entry.sha256 -cne $expected[2]) {
            throw "clean-room $($expected[0].TrimEnd('s')) manifest SHA-256 mismatch"
        }
    }
}

function Get-NinhoTestedInputs {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Root)
    return Get-NinhoTestedInputIdentity -Root $Root `
        -RelativePaths @(Get-NinhoTestedInputPaths -Root $Root)
}

function Assert-NinhoHumanPlaytestPending {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Playtest)

    $properties = @($Playtest.PSObject.Properties.Name)
    $isCurrentPending =
        $properties -ccontains 'participants' -and [int]$Playtest.participants -eq 0 -and
        $Playtest.status -ceq 'not_performed' -and $Playtest.substitute -ceq 'none' -and
        $properties -ccontains 'gate_status' -and $Playtest.gate_status -ceq 'pending' -and
        $properties -ccontains 'required_before' -and $Playtest.required_before -ceq 'product_release'
    $isLegacyPending =
        $properties -cnotcontains 'participants' -and
        $properties -cnotcontains 'gate_status' -and
        $Playtest.status -ceq 'unavailable' -and
        $Playtest.substitute -ceq 'independent_agents'
    if (($Playtest.legal_limit -cne 'not_legal_advice') -or
            (-not $isCurrentPending -and -not $isLegacyPending)) {
        throw 'human playtest record must remain pending; agent reviews are not a substitute'
    }
    return [pscustomobject]@{
        status = 'pending'
        participants = 0
        satisfies_human_playtest = $false
        legacy_record = $isLegacyPending
    }
}

function Get-NinhoCanonicalTestedInputContent {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$RelativePath
    )
    return TestedInputIdentity\Get-NinhoCanonicalTestedInputContent `
        -Path $Path -RelativePath $RelativePath
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
        $observedTick = [int64]$marker.observed_tick
        if ([int]$marker.sample_index -ne $index -or
                [string]$marker.command_id -cne [string]$expected[$index].command_id -or
                [double]$marker.theta_degrees -ne [double]$expected[$index].theta_degrees -or
                [double]$marker.phase_degrees -ne [double]$expected[$index].phase_degrees -or
                [double]$marker.speed -ne [double]$expected[$index].speed -or
                $observedTick -lt 0 -or
                $previewHash -notmatch '^-?[0-9]+$' -or $previewHash -ceq '0') {
            throw "$Context causal marker $index does not match its input command"
        }
        if ($index -gt 0 -and $observedTick -le [int64]$items[$index - 1].observed_tick) {
            throw "$Context observed_tick must be strictly increasing at causal marker $index"
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
        [Parameter(Mandatory)][ValidateSet('Debug','Release')][string]$ExpectedConfiguration
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
    $rootForInputs = [IO.Path]::GetFullPath($ArtifactRoot)
    $actualInputs = Get-NinhoTestedInputs -Root $rootForInputs
    Assert-NinhoTestedInputIdentity -Root $rootForInputs -Document $doc `
        -ExpectedIdentity $actualInputs -ExpectedSourceRevision ([string]$doc.commit)
    if ($doc.canonical_state_contract -cne 'canonical_state_v2') {
        throw 'canonical_state_v2 contract missing'
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
    $virelaGolden = @($doc.golden_metadata | Where-Object name -CEQ 'virela')[0]
    if ($virelaGolden.phase -cne 'flight_ability' -or $virelaGolden.outcome -cne 'none' -or
            $virelaGolden.source_frame -ne $virelaGolden.source_transition_frame -or
            $virelaGolden.event -notin @('ability_started:vortex','ability_pulse:virela_pulse')) {
        throw 'Virela golden is not tied to an exact ability/VFX event frame'
    }
    if ($doc.rubric.critical -ne 0 -or $doc.rubric.important -ne 0) {
        throw 'agent review has blocking findings'
    }
    $null = Assert-NinhoHumanPlaytestPending -Playtest $doc.playtest
    $assetProvenancePath = Assert-NinhoRelativeArtifactPath 'tools/art/vertical_slice_asset_manifest.json' $ArtifactRoot
    $audioProvenancePath = Assert-NinhoRelativeArtifactPath 'tools/audio/audio_manifest.json' $ArtifactRoot
    foreach ($provenancePath in $assetProvenancePath,$audioProvenancePath) {
        if (-not (Test-Path -LiteralPath $provenancePath -PathType Leaf)) {
            throw "clean-room provenance manifest missing: $provenancePath"
        }
    }
    $assetProvenanceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $assetProvenancePath).Hash.ToLowerInvariant()
    $audioProvenanceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $audioProvenancePath).Hash.ToLowerInvariant()
    $cleanRoomReference = Assert-NinhoProperty $doc 'clean_room_review' 'vertical slice evidence'
    if ($cleanRoomReference.path -cne 'docs/gameplay/evidence/vertical-slice-clean-room-review.json' -or
            [string]$cleanRoomReference.sha256 -notmatch '^[0-9a-f]{64}$') {
        throw 'clean-room review reference is invalid'
    }
    $cleanRoomReviewPath = Assert-NinhoRelativeArtifactPath $cleanRoomReference.path $ArtifactRoot
    if (-not (Test-Path -LiteralPath $cleanRoomReviewPath -PathType Leaf)) {
        throw 'separate clean-room review evidence is missing'
    }
    $cleanRoomReviewHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $cleanRoomReviewPath).Hash.ToLowerInvariant()
    if ($cleanRoomReviewHash -cne [string]$cleanRoomReference.sha256) {
        throw 'clean-room review reference SHA-256 mismatch'
    }
    $cleanRoomReview = Get-Content -Raw -LiteralPath $cleanRoomReviewPath | ConvertFrom-Json
    Assert-NinhoCleanRoomReview -Review $cleanRoomReview `
        -ExpectedAssetManifestSha256 $assetProvenanceHash `
        -ExpectedAudioManifestSha256 $audioProvenanceHash
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
            'package-source-inventory.json','build-contract.json','licenses/THIRD_PARTY_NOTICES.md','licenses/sbom.spdx.json',
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
        $sourceInventoryRow = @($packageDocument.files | Where-Object path -CEQ 'package-source-inventory.json')
        $sourceInventoryDocument = Get-Content -Raw -LiteralPath (Join-Path $packageRoot 'package-source-inventory.json') | ConvertFrom-Json
        if ($buildContract.schema -cne 'ninho.native-build-contract.v1' -or
                $buildContract.configuration -cne 'Release' -or
                $buildContract.build_testing -cne $false -or
                $buildContract.test_facades -cne $false -or
                $buildContract.NINHO_ENABLE_TEST_FACADES -cne 'absent' -or
                $packageDocument.native_build_contract.path -cne 'build-contract.json' -or
                $packageDocument.native_build_contract.sha256 -cne $buildContractHash -or
                $packageDocument.native_build_contract.build_testing -cne $false -or
                $packageDocument.native_build_contract.test_facades -cne $false) {
            throw 'Release package native build provenance mismatch'
        }
        if ($sourceInventoryRow.Count -ne 1 -or
                $sourceInventoryRow[0].role -cne 'source_inventory' -or
                $sourceInventoryDocument.schema -cne 'ninho.package-source-inventory.v1' -or
                $sourceInventoryDocument.schema_version -ne 1 -or
                $sourceInventoryDocument.note -notmatch 'does not enumerate NinhoOrbital\.pck contents' -or
                $sourceInventoryDocument.note -notmatch 'PCK SHA-256' -or
                $sourceInventoryDocument.note -notmatch 'packaged runtime smoke' -or
                $sourceInventoryDocument.native_build_contract.path -cne 'build-contract.json' -or
                $sourceInventoryDocument.native_build_contract.sha256 -cne $buildContractHash -or
                $sourceInventoryDocument.native_build_contract.build_testing -cne $false -or
                $sourceInventoryDocument.native_build_contract.test_facades -cne $false) {
            throw 'Release package source inventory contract/provenance mismatch'
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
    $canonicalBaseline = [regex]::Match($routeRawText, '(?m)^\[TRACE\] canonical_playthrough_v4 (\d+) (\d+) (\d+)\r?$')
    $canonicalRepeat = [regex]::Match($routeRawText, '(?m)^\[TRACE\] canonical_playthrough_v4_repeat (\d+) (\d+) (\d+)\r?$')
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
                "(?m)^\[TRACE\] ordered_events_v2 $([regex]::Escape($routeName)) $runName ([0-9a-f]+)\r?`$")
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
        $ffprobe = Resolve-NinhoPinnedToolchainExecutable -Root $ArtifactRoot -ToolName 'ffmpeg' -ExecutableProperty 'ffprobe_exe' -HashProperty 'ffprobe_exe_sha256' -Name 'FFprobe'
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
    $ffmpeg = Resolve-NinhoPinnedToolchainExecutable -Root $ArtifactRoot -ToolName 'ffmpeg' -ExecutableProperty 'exe' -HashProperty 'exe_sha256' -Name 'FFmpeg'
    $vulkanSourcePath = Assert-NinhoRelativeArtifactPath $vulkan.capture.source_path $ArtifactRoot
    $temporaryRoot = [IO.Path]::GetTempPath().TrimEnd('\','/')
    $goldenVerificationRoot = Join-Path $temporaryRoot (
        'ninho-vertical-slice-golden-verification-' + [Guid]::NewGuid().ToString('N'))
    Assert-NinhoNoReparseAncestors `
        -Path $goldenVerificationRoot -AllowedRoot $temporaryRoot | Out-Null
    New-Item -ItemType Directory -Path $goldenVerificationRoot | Out-Null
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
        Assert-NinhoNoReparseAncestors -Path $goldenVerificationRoot -AllowedRoot $temporaryRoot | Out-Null
        Remove-Item -LiteralPath $goldenVerificationRoot -Recurse -Force
    }
    $reviewsPath = Assert-NinhoRelativeArtifactPath $doc.reviews_manifest.path $ArtifactRoot
    $reviews = Get-Content -Raw -LiteralPath $reviewsPath | ConvertFrom-Json
    Assert-NinhoAgentReviews -Reviews $reviews `
        -ExpectedTestedInputsSha256 ([string]$doc.tested_inputs_sha256) `
        -ExpectedCaptureManifestSha256 ([string]$doc.capture_manifest.sha256) `
        -ExpectedCaptureConfiguration $ExpectedConfiguration `
        -ExpectedGoldenMetadata @($captureManifest.golden_metadata)
    $vulkanLog = [IO.File]::ReadAllText((Assert-NinhoRelativeArtifactPath $vulkan.capture.log_path $ArtifactRoot))
    $stateProofPattern = '(?m)^NINHO_CAPTURE_STATE frame=(\d+) tick=(\d+) phase=(\S+) outcome=(\S+) camera=(\([^)]+\)) exposure=([0-9.]+)\s*$'
    $stateProofs = @([regex]::Matches($vulkanLog, $stateProofPattern))
    foreach ($stateGoldenName in 'overview','aim','result') {
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
    $virelaProof = Get-NinhoVirelaAbilityProof -Text $vulkanLog
    if ([int]$virelaGolden.source_transition_frame -ne [int]$virelaProof.frame -or
            [int64]$virelaGolden.tick -ne [int64]$virelaProof.tick -or
            $virelaGolden.phase -cne $virelaProof.phase -or
            $virelaGolden.outcome -cne $virelaProof.outcome -or
            $virelaGolden.event -cne $virelaProof.event -or
            $virelaGolden.camera -cne $virelaProof.camera -or
            [Math]::Abs([double]$virelaGolden.exposure - [double]$virelaProof.exposure) -gt 0.000001) {
        throw 'Virela ability metadata differs from verified Vulkan log'
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

function Publish-NinhoVerticalSliceEvidence {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Document,
        [Parameter(Mandatory)][string]$EvidencePath,
        [Parameter(Mandatory)][string]$ArtifactRoot,
        [Parameter(Mandatory)][ValidateSet('Debug','Release')]
        [string]$ExpectedConfiguration
    )

    $rootPath = [IO.Path]::GetFullPath($ArtifactRoot).TrimEnd('\','/')
    $canonicalPath = [IO.Path]::GetFullPath($EvidencePath)
    Assert-NinhoNoReparseAncestors -Path $canonicalPath -AllowedRoot $rootPath |
        Out-Null
    $evidenceDirectory = [IO.Path]::GetDirectoryName($canonicalPath)
    if (-not (Test-Path -LiteralPath $evidenceDirectory -PathType Container)) {
        throw "vertical slice evidence directory missing: $evidenceDirectory"
    }

    $publicationId = [Guid]::NewGuid().ToString('N')
    $temporaryName = '.{0}.{1}.tmp' -f (
        [IO.Path]::GetFileName($canonicalPath)),$publicationId
    $temporaryPath = Join-Path $evidenceDirectory $temporaryName
    $backupPath = Join-Path $evidenceDirectory ('.{0}.{1}.bak' -f (
        [IO.Path]::GetFileName($canonicalPath)),$publicationId)
    Assert-NinhoNoReparseAncestors -Path $temporaryPath -AllowedRoot $rootPath |
        Out-Null
    Assert-NinhoNoReparseAncestors -Path $backupPath -AllowedRoot $rootPath |
        Out-Null
    try {
        $Document | ConvertTo-Json -Depth 30 |
            Set-Content -LiteralPath $temporaryPath -Encoding utf8
        $validated = Assert-NinhoVerticalSliceEvidence `
            -EvidencePath $temporaryPath -ArtifactRoot $rootPath `
            -ExpectedConfiguration $ExpectedConfiguration

        if (Test-Path -LiteralPath $canonicalPath -PathType Leaf) {
            # File.Replace is an atomic same-volume promotion that keeps the old
            # canonical file in place until the validated temporary file wins.
            [IO.File]::Replace($temporaryPath, $canonicalPath, $backupPath)
        } else {
            [IO.File]::Move($temporaryPath, $canonicalPath)
        }
        return $validated
    } finally {
        if (Test-Path -LiteralPath $temporaryPath -PathType Leaf) {
            Remove-Item -LiteralPath $temporaryPath -Force
        }
        if (Test-Path -LiteralPath $backupPath -PathType Leaf) {
            Remove-Item -LiteralPath $backupPath -Force
        }
    }
}

function Get-NinhoRouteEvidenceFromLog {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Text)
    $pattern = '(?m)^NINHO_ROUTE_HASH name=(\S+) outcome=(Victory|Defeat) canonical_state_v2=(\d+) events_sha256=([0-9a-f]{64})\s*$'
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

Export-ModuleMember -Function Assert-NinhoVerticalSliceEvidence,Publish-NinhoVerticalSliceEvidence,Assert-NinhoRelativeArtifactPath,Get-NinhoRouteEvidenceFromLog,Invoke-NinhoLimitedRetry,Get-NinhoTestedInputPaths,Get-NinhoTestedInputs,Get-NinhoCausalFrameIndex,Set-NinhoRequiredDownsampleFrame,Get-NinhoCompactDownsampleSelectExpression,Get-NinhoVirelaAbilityProof,Invoke-NinhoTimedProcess,Resolve-NinhoPackageManifestOutput,Assert-NinhoInputFeedbackMarkers,Assert-NinhoAgentReviews,Assert-NinhoCleanRoomReview,Assert-NinhoHumanPlaytestPending,Get-NinhoCanonicalTestedInputContent
