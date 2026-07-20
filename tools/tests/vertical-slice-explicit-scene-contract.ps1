[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$captureScriptPath = (Resolve-Path -LiteralPath (
        Join-Path $root 'tools\capture_vertical_slice.ps1')).Path
$projectPath = (Resolve-Path -LiteralPath (Join-Path $root 'game\project.godot')).Path
$godotPath = Join-Path $root '.tools\godot\Godot_v4.5.1-stable_win64.exe'
$expectedScene = 'res://scenes/vertical_slice.tscn'
$sceneFailure = 'Legacy capture must pass exactly one executable explicit res:// scene argument'
$argvFailure = 'Legacy capture recorder observed invalid executable argv'
$recorderFailure = 'Legacy capture recorder did not remain isolated'
$reachabilityFailure = 'Legacy capture top-level must reach Invoke-CaptureRun and the timed process interceptor'
$reachabilitySentinel = 'NINHO_TOP_LEVEL_REACHABILITY_INTERCEPT:'

Import-Module (Join-Path $root 'tools\VerticalSliceGate.psm1') -Force
Import-Module (Join-Path $root 'tools\SafePath.psm1') -Force

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Test-ExactSequence {
    param([string[]]$Actual, [string[]]$Expected)

    if ($Actual.Count -ne $Expected.Count) { return $false }
    for ($index = 0; $index -lt $Expected.Count; ++$index) {
        if ($Actual[$index] -cne $Expected[$index]) { return $false }
    }
    return $true
}

function Get-CaptureFunctionContract {
    param([Parameter(Mandatory)][string]$ScriptPath)

    $tokens = $null
    $parseErrors = $null
    $ast = [Management.Automation.Language.Parser]::ParseFile(
        $ScriptPath, [ref]$tokens, [ref]$parseErrors)
    Assert-True ($parseErrors.Count -eq 0) `
        "Legacy capture AST parse failed: $(@($parseErrors.Message) -join '; ')"

    $functions = @($ast.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.FunctionDefinitionAst] -and
                    $candidate.Name -ceq 'Invoke-CaptureRun'
            }, $true))
    Assert-True ($functions.Count -eq 1) `
        'Legacy capture must define exactly one Invoke-CaptureRun function'
    $function = $functions[0]

    $callSites = @($ast.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.CommandAst] -and
                    $candidate.GetCommandName() -ceq 'Invoke-CaptureRun'
            }, $true) | Sort-Object { $_.Extent.StartOffset })
    Assert-True ($callSites.Count -eq 2) `
        'Legacy capture must have exactly two canonical Invoke-CaptureRun call sites'
    foreach ($callSite in $callSites) {
        Assert-True ($callSite.Extent.StartOffset -lt $function.Extent.StartOffset -or
                $callSite.Extent.EndOffset -gt $function.Extent.EndOffset) `
            'Legacy capture call sites must invoke the real top-level function'
    }
    $expectedCallSites = @(
        'Invoke-CaptureRun -Name "capture-$slug" -Renderer $renderer -Scale 100 -Movie $rawMovie',
        'Invoke-CaptureRun -Name "performance-$slug-$scale" -Renderer $renderer -Scale $scale -Metrics $metrics')
    for ($index = 0; $index -lt $callSites.Count; ++$index) {
        Assert-True ($callSites[$index].Extent.Text -ceq $expectedCallSites[$index]) `
            'Legacy capture canonical Invoke-CaptureRun call sites changed'
    }

    return [pscustomobject]@{
        Ast = $ast
        Function = $function
        FunctionText = $function.Extent.Text
        ScriptText = [IO.File]::ReadAllText($ScriptPath)
        CallSites = $callSites
    }
}

function Replace-UniqueFunctionText {
    param(
        [Parameter(Mandatory)]$FunctionContract,
        [Parameter(Mandatory)][string]$Search,
        [AllowEmptyString()][string]$Replacement
    )

    $matches = @([regex]::Matches(
            $FunctionContract.FunctionText, [regex]::Escape($Search)))
    Assert-True ($matches.Count -eq 1) `
        "Legacy capture fixture source must occur once in the real function: $Search"
    $match = $matches[0]
    $mutatedFunction = $FunctionContract.FunctionText.Substring(0, $match.Index) +
        $Replacement +
        $FunctionContract.FunctionText.Substring($match.Index + $match.Length)
    return $FunctionContract.ScriptText.Substring(
        0, $FunctionContract.Function.Extent.StartOffset) + $mutatedFunction +
        $FunctionContract.ScriptText.Substring(
            $FunctionContract.Function.Extent.EndOffset)
}

function New-CaptureFixture {
    param(
        [Parameter(Mandatory)]$FunctionContract,
        [Parameter(Mandatory)][string]$Directory,
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Search,
        [AllowEmptyString()][string]$Replacement
    )

    $path = Join-Path $Directory $Name
    $text = Replace-UniqueFunctionText -FunctionContract $FunctionContract `
        -Search $Search -Replacement $Replacement
    [IO.File]::WriteAllText($path, $text, [Text.UTF8Encoding]::new($false))
    return $path
}

function Disable-CaptureCallSites {
    param([Parameter(Mandatory)]$FunctionContract)

    $text = $FunctionContract.ScriptText
    foreach ($callSite in @($FunctionContract.CallSites |
            Sort-Object { $_.Extent.StartOffset } -Descending)) {
        $replacement = "if (`$false) { $($callSite.Extent.Text) }"
        $text = $text.Substring(0, $callSite.Extent.StartOffset) +
            $replacement + $text.Substring($callSite.Extent.EndOffset)
    }
    return $text
}

function Add-TopLevelReachabilityInterceptor {
    param(
        [Parameter(Mandatory)]$FunctionContract,
        [Parameter(Mandatory)][string]$ScriptText
    )

    $functionStart = $FunctionContract.Function.Extent.StartOffset
    $functionEnd = $FunctionContract.Function.Extent.EndOffset
    Assert-True ($ScriptText.Substring(
                $functionStart, $functionEnd - $functionStart) -ceq
            $FunctionContract.FunctionText) `
        'Reachability instrumentation must preserve the exact real function Extent'
    $interceptor = @'

function Invoke-NinhoTimedProcess {
    param(
        [string]$FilePath,
        [string[]]$ArgumentList,
        [int]$TimeoutMs,
        [string]$StdoutPath,
        [string]$StderrPath,
        [string]$WorkingDirectory,
        [string]$FatalMarker
    )
    $payload = [ordered]@{
        name = $Name
        call_stack = @(Get-PSCallStack | ForEach-Object { $_.FunctionName })
        file_path = $FilePath
        argument_list = [string[]]@($ArgumentList)
        timeout_ms = $TimeoutMs
        stdout_path = $StdoutPath
        stderr_path = $StderrPath
        working_directory = $WorkingDirectory
        fatal_marker = $FatalMarker
        script_root = $PSScriptRoot
    }
    throw ('__REACHABILITY_SENTINEL__' +
        ($payload | ConvertTo-Json -Depth 5 -Compress))
}
'@.Replace('__REACHABILITY_SENTINEL__', $reachabilitySentinel)
    return $ScriptText.Substring(0, $functionEnd) + $interceptor +
        $ScriptText.Substring($functionEnd)
}

function Invoke-TopLevelReachabilityProbe {
    param(
        [Parameter(Mandatory)][string]$ProbeScriptPath,
        [Parameter(Mandatory)][string]$OutputDirectory,
        [Parameter(Mandatory)][string]$GoldenDirectory
    )

    $harness = @'
param($ProbeScriptPath, $OutputDirectory, $GoldenDirectory)
$ErrorActionPreference = 'Stop'
$breakpointsBefore = @(Get-PSBreakpoint).Count
$completed = $false
$failure = ''
try {
    $null = & $ProbeScriptPath -Configuration Debug `
        -OutputDirectory $OutputDirectory -GoldenDirectory $GoldenDirectory
    $completed = $true
} catch {
    $failure = $_.Exception.Message
}
[pscustomobject]@{
    Completed = $completed
    Failure = $failure
    BreakpointsBefore = $breakpointsBefore
    BreakpointsAfter = @(Get-PSBreakpoint).Count
}
'@
    $powerShell = [PowerShell]::Create()
    try {
        $null = $powerShell.AddScript($harness).
            AddParameter('ProbeScriptPath', $ProbeScriptPath).
            AddParameter('OutputDirectory', $OutputDirectory).
            AddParameter('GoldenDirectory', $GoldenDirectory)
        $output = @($powerShell.Invoke())
        $errors = @($powerShell.Streams.Error)
        # A caught terminating error sets HadErrors; the exact sentinel is validated below.
        Assert-True ($errors.Count -eq 0 -and $output.Count -eq 1) `
            ($reachabilityFailure + ': isolated top-level runspace failed ' +
                "had_errors=$($powerShell.HadErrors) outputs=$($output.Count) " +
                "errors=[$(@($errors | ForEach-Object { $_.ToString() }) -join '; ')]")
        return $output[0]
    } finally {
        $powerShell.Dispose()
    }
}

function Get-TopLevelReachabilityContract {
    param(
        [Parameter(Mandatory)]$FunctionContract,
        [Parameter(Mandatory)][string]$ScriptText,
        [Parameter(Mandatory)][string]$ProbeScriptPath,
        [Parameter(Mandatory)][string]$OutputDirectory,
        [Parameter(Mandatory)][string]$GoldenDirectory
    )

    $instrumentedText = Add-TopLevelReachabilityInterceptor `
        -FunctionContract $FunctionContract -ScriptText $ScriptText
    [IO.File]::WriteAllText(
        $ProbeScriptPath, $instrumentedText, [Text.UTF8Encoding]::new($false))
    $probe = Invoke-TopLevelReachabilityProbe `
        -ProbeScriptPath $ProbeScriptPath `
        -OutputDirectory $OutputDirectory `
        -GoldenDirectory $GoldenDirectory
    if ($probe.Completed -or
            -not $probe.Failure.StartsWith(
                $reachabilitySentinel, [StringComparison]::Ordinal) -or
            $probe.BreakpointsBefore -ne 0 -or $probe.BreakpointsAfter -ne 0) {
        throw $reachabilityFailure
    }

    $payloadText = $probe.Failure.Substring($reachabilitySentinel.Length)
    try {
        $payload = $payloadText | ConvertFrom-Json
    } catch {
        throw $reachabilityFailure
    }
    $callStack = @($payload.call_stack | ForEach-Object { [string]$_ })
    if ($callStack -cnotcontains 'Invoke-CaptureRun' -or
            $callStack -cnotcontains 'Invoke-NinhoLimitedRetry') {
        throw $reachabilityFailure
    }

    $expectedMovie = Join-Path $OutputDirectory 'vertical-slice-vulkan-source.avi'
    $expectedCase = [pscustomobject]@{
        Renderer = 'Vulkan'
        Scale = 100
        Movie = $expectedMovie
        Metrics = ''
        MovieEnabled = $true
        MetricsEnabled = $false
    }
    $expectedArguments = @(Get-ExpectedCaptureArguments `
            -Case $expectedCase -RecorderRoot $root)
    if ($payload.name -cne 'capture-vulkan' -or
            $payload.file_path -cne $godotPath -or
            -not (Test-ExactSequence -Actual @($payload.argument_list) `
                -Expected $expectedArguments) -or
            $payload.timeout_ms -ne 600000 -or
            $payload.working_directory -cne $root -or
            $payload.fatal_marker -cne
                'NINHO_CAPTURE_FATAL name=capture-vulkan reason=timeout' -or
            $payload.script_root -cne (Join-Path $root 'tools')) {
        throw $reachabilityFailure
    }
    return $payload
}

function Get-RecorderRelativePath {
    param(
        [Parameter(Mandatory)][string]$Base,
        [Parameter(Mandatory)][string]$Target
    )

    $basePath = [IO.Path]::GetFullPath($Base).TrimEnd('\', '/') +
        [IO.Path]::DirectorySeparatorChar
    $targetPath = [IO.Path]::GetFullPath($Target)
    return [Uri]::UnescapeDataString(
        ([Uri]$basePath).MakeRelativeUri([Uri]$targetPath).ToString())
}

function New-RecorderCases {
    param([Parameter(Mandatory)][string]$RecorderOutput)

    $cases = [Collections.Generic.List[object]]::new()
    foreach ($renderer in 'OpenGL', 'Vulkan') {
        foreach ($movieEnabled in $false, $true) {
            foreach ($metricsEnabled in $false, $true) {
                foreach ($scale in 100, 150) {
                    $slug = $renderer.ToLowerInvariant()
                    $caseId = "$slug-s$scale-m$([int]$movieEnabled)-x$([int]$metricsEnabled)"
                    $movie = if ($movieEnabled) {
                        Join-Path $RecorderOutput "$caseId.avi"
                    } else { '' }
                    $metrics = if ($metricsEnabled) {
                        Join-Path $RecorderOutput "$caseId.json"
                    } else { '' }
                    $cases.Add([pscustomobject]@{
                            Id = $caseId
                            Name = "recorder-$caseId"
                            Renderer = $renderer
                            Scale = $scale
                            Movie = $movie
                            Metrics = $metrics
                            MovieEnabled = $movieEnabled
                            MetricsEnabled = $metricsEnabled
                        })
                }
            }
        }
    }
    return @($cases)
}

function Invoke-CaptureRecorder {
    param(
        [Parameter(Mandatory)][string]$FunctionText,
        [Parameter(Mandatory)][object[]]$Cases,
        [Parameter(Mandatory)][string]$RecorderRoot,
        [Parameter(Mandatory)][string]$RecorderOutput
    )

    $harness = @'
param($FunctionText, $Cases, $RecorderRoot, $RecorderOutput)
$ErrorActionPreference = 'Stop'
$root = $RecorderRoot
$OutputDirectory = $RecorderOutput
$godot = Join-Path $RecorderOutput '__recorder_no_launch__.exe'
$script:records = [Collections.Generic.List[object]]::new()
$script:retryCalls = [Collections.Generic.List[object]]::new()
$script:markers = [Collections.Generic.List[object]]::new()
$script:artifacts = [Collections.Generic.List[object]]::new()
$script:currentCaseId = ''
$script:currentAttempt = 0
$script:currentMaximum = 0
$script:interceptedLaunchCount = 0
$script:externalLaunchCount = 0

function Get-RelativePath([string]$Base, [string]$Target) {
    $basePath = [IO.Path]::GetFullPath($Base).TrimEnd('\', '/') +
        [IO.Path]::DirectorySeparatorChar
    $targetPath = [IO.Path]::GetFullPath($Target)
    return [Uri]::UnescapeDataString(
        ([Uri]$basePath).MakeRelativeUri([Uri]$targetPath).ToString())
}

function Invoke-NinhoLimitedRetry {
    param(
        [string]$Name,
        [int]$MaximumAttempts,
        [scriptblock]$Operation
    )
    $script:currentMaximum = $MaximumAttempts
    $script:retryCalls.Add([pscustomobject]@{
            CaseId = $script:currentCaseId
            Name = $Name
            MaximumAttempts = $MaximumAttempts
        }) | Out-Null
    $result = $null
    foreach ($attempt in 1..$MaximumAttempts) {
        $script:currentAttempt = $attempt
        $result = & $Operation $attempt
    }
    return $result
}

function Invoke-NinhoTimedProcess {
    param(
        [string]$FilePath,
        [string[]]$ArgumentList,
        [int]$TimeoutMs,
        [string]$StdoutPath,
        [string]$StderrPath,
        [string]$WorkingDirectory,
        [string]$FatalMarker
    )
    ++$script:interceptedLaunchCount
    $script:records.Add([pscustomobject]@{
            CaseId = $script:currentCaseId
            Attempt = $script:currentAttempt
            MaximumAttempts = $script:currentMaximum
            FilePath = $FilePath
            ArgumentList = [string[]]@($ArgumentList)
            TimeoutMs = $TimeoutMs
            StdoutPath = $StdoutPath
            StderrPath = $StderrPath
            WorkingDirectory = $WorkingDirectory
            FatalMarker = $FatalMarker
        }) | Out-Null
    return [pscustomobject]@{
        ExitCode = 0
        Stdout = $StdoutPath
        Stderr = $StderrPath
    }
}

function Assert-CleanLog {
    param([string]$Stdout, [string]$Stderr, [string]$Marker = '')
    $script:markers.Add([pscustomobject]@{
            CaseId = $script:currentCaseId
            Stdout = $Stdout
            Stderr = $Stderr
            Marker = $Marker
        }) | Out-Null
    return ''
}

function Add-Artifact {
    param([string]$Path)
    $script:artifacts.Add([pscustomobject]@{
            CaseId = $script:currentCaseId
            Path = $Path
        }) | Out-Null
}

function Start-Process {
    param([Parameter(ValueFromRemainingArguments = $true)]$Ignored)
    ++$script:externalLaunchCount
    throw 'Recorder blocked a real Start-Process call'
}

Invoke-Expression -Command $FunctionText
foreach ($case in @($Cases)) {
    $script:currentCaseId = $case.Id
    $null = Invoke-CaptureRun -Name $case.Name -Renderer $case.Renderer `
        -Scale $case.Scale -Movie $case.Movie -Metrics $case.Metrics
}

[pscustomobject]@{
    Records = @($script:records)
    RetryCalls = @($script:retryCalls)
    Markers = @($script:markers)
    Artifacts = @($script:artifacts)
    InterceptedLaunchCount = $script:interceptedLaunchCount
    ExternalLaunchCount = $script:externalLaunchCount
    GodotSentinel = $godot
}
'@

    $powerShell = [PowerShell]::Create()
    try {
        $null = $powerShell.AddScript($harness).
            AddParameter('FunctionText', $FunctionText).
            AddParameter('Cases', [object[]]$Cases).
            AddParameter('RecorderRoot', $RecorderRoot).
            AddParameter('RecorderOutput', $RecorderOutput)
        $output = @($powerShell.Invoke())
        $errors = @($powerShell.Streams.Error)
        Assert-True (-not $powerShell.HadErrors -and $errors.Count -eq 0) `
            "$recorderFailure`: $(@($errors | ForEach-Object { $_.ToString() }) -join '; ')"
        Assert-True ($output.Count -eq 1) `
            "$recorderFailure`: expected one recorder result, found $($output.Count)"
        return $output[0]
    } finally {
        $powerShell.Dispose()
    }
}

function Get-ExpectedCaptureArguments {
    param(
        [Parameter(Mandatory)]$Case,
        [Parameter(Mandatory)][string]$RecorderRoot
    )

    $expected = [Collections.Generic.List[string]]::new()
    if ($Case.Renderer -ceq 'OpenGL') {
        $expected.AddRange([string[]]@('--rendering-method', 'gl_compatibility'))
    }
    $expected.AddRange([string[]]@('--path', 'game', '--resolution', '1920x1080'))
    if ($Case.MovieEnabled) {
        $movieRelative = (Get-RecorderRelativePath `
                -Base (Join-Path $RecorderRoot 'game') -Target $Case.Movie).
            Replace('\', '/')
        $expected.AddRange([string[]]@(
                '--write-movie', $movieRelative, '--fixed-fps', '60'))
    }
    $expected.Add($expectedScene)
    $expected.Add('--')
    $expected.Add('--vertical-slice-capture')
    $expected.Add("--ui-scale=$($Case.Scale)")
    if ($Case.MovieEnabled) {
        $expected.Add('--capture-normal-terminal')
    }
    if ($Case.MetricsEnabled) {
        $metricsRelative = (Get-RecorderRelativePath `
                -Base (Join-Path $RecorderRoot 'game') -Target $Case.Metrics).
            Replace('\', '/')
        $expected.Add("--vertical-slice-metrics=res://$metricsRelative")
    }
    return [string[]]@($expected)
}

function Assert-RecordedCaptureContract {
    param(
        [Parameter(Mandatory)]$Recorder,
        [Parameter(Mandatory)][object[]]$Cases,
        [Parameter(Mandatory)][string]$RecorderRoot,
        [Parameter(Mandatory)][string]$RecorderOutput
    )

    $records = @($Recorder.Records)
    $retryCalls = @($Recorder.RetryCalls)
    $markers = @($Recorder.Markers)
    $artifacts = @($Recorder.Artifacts)
    $expectedSentinel = Join-Path $RecorderOutput '__recorder_no_launch__.exe'
    Assert-True ($Recorder.GodotSentinel -ceq $expectedSentinel -and
            -not (Test-Path -LiteralPath $expectedSentinel) -and
            -not (Test-Path -LiteralPath $RecorderOutput)) `
        "$recorderFailure`: recorder created or resolved a real external target"
    $expectedRecordCount = @($Cases | ForEach-Object {
            if ($_.Renderer -ceq 'Vulkan') { 2 } else { 1 }
        } | Measure-Object -Sum).Sum
    Assert-True ($Recorder.ExternalLaunchCount -eq 0 -and
            $Recorder.InterceptedLaunchCount -eq $records.Count -and
            $records.Count -eq $expectedRecordCount) $recorderFailure
    Assert-True ($retryCalls.Count -eq $Cases.Count -and
            $markers.Count -eq $Cases.Count -and
            $artifacts.Count -eq 2 * $Cases.Count) `
        'Legacy capture recorder did not observe the complete real function lifecycle'

    foreach ($case in $Cases) {
        $maximumAttempts = if ($case.Renderer -ceq 'Vulkan') { 2 } else { 1 }
        $caseRetry = @($retryCalls | Where-Object { $_.CaseId -ceq $case.Id })
        Assert-True ($caseRetry.Count -eq 1 -and
                $caseRetry[0].Name -ceq $case.Name -and
                $caseRetry[0].MaximumAttempts -eq $maximumAttempts) `
            'Legacy capture retry branch changed under the recorder'

        $caseRecords = @($records | Where-Object { $_.CaseId -ceq $case.Id } |
            Sort-Object Attempt)
        Assert-True ($caseRecords.Count -eq $maximumAttempts) `
            'Legacy capture recorder observed an invalid attempt count'
        $expectedArguments = @(Get-ExpectedCaptureArguments `
                -Case $case -RecorderRoot $RecorderRoot)
        foreach ($record in $caseRecords) {
            $actualArguments = @($record.ArgumentList)
            $sceneArguments = @($actualArguments | Where-Object {
                    $_ -match '^res://.+\.tscn$'
                })
            if ($sceneArguments.Count -ne 1 -or
                    $sceneArguments[0] -cne $expectedScene) {
                throw $sceneFailure
            }
            if (-not (Test-ExactSequence -Actual $actualArguments `
                        -Expected $expectedArguments)) {
                throw $argvFailure
            }
            $expectedStdout = Join-Path $RecorderOutput `
                "$($case.Name)-attempt$($record.Attempt).stdout.log"
            $expectedStderr = Join-Path $RecorderOutput `
                "$($case.Name)-attempt$($record.Attempt).stderr.log"
            Assert-True ($record.MaximumAttempts -eq $maximumAttempts -and
                    $record.FilePath -ceq $Recorder.GodotSentinel -and
                    $record.TimeoutMs -eq $(if ($case.MovieEnabled) { 600000 } else { 120000 }) -and
                    $record.StdoutPath -ceq $expectedStdout -and
                    $record.StderrPath -ceq $expectedStderr -and
                    $record.WorkingDirectory -ceq $RecorderRoot -and
                    $record.FatalMarker -ceq
                        "NINHO_CAPTURE_FATAL name=$($case.Name) reason=timeout") `
                'Legacy capture timed-process contract changed under the recorder'
        }

        $caseMarkers = @($markers | Where-Object { $_.CaseId -ceq $case.Id })
        $expectedMarker = if ($case.MovieEnabled) {
            'VERTICAL_SLICE_SOURCE_CAPTURE_COMPLETE'
        } else { 'VERTICAL_SLICE_CAPTURE_COMPLETE frame=300' }
        Assert-True ($caseMarkers.Count -eq 1 -and
                $caseMarkers[0].Marker -ceq $expectedMarker) `
            'Legacy capture completion-marker branch changed under the recorder'

        $caseArtifacts = @($artifacts | Where-Object { $_.CaseId -ceq $case.Id })
        $lastAttempt = $maximumAttempts
        Assert-True ($caseArtifacts.Count -eq 2 -and
                $caseArtifacts[0].Path -ceq (Join-Path $RecorderOutput `
                    "$($case.Name)-attempt$lastAttempt.stdout.log") -and
                $caseArtifacts[1].Path -ceq (Join-Path $RecorderOutput `
                    "$($case.Name)-attempt$lastAttempt.stderr.log")) `
            'Legacy capture artifact lifecycle changed under the recorder'
    }
}

function Get-ValidatedCaptureContract {
    param(
        [Parameter(Mandatory)][string]$ScriptPath,
        [Parameter(Mandatory)][string]$RecorderOutput
    )

    $functionContract = Get-CaptureFunctionContract -ScriptPath $ScriptPath
    $cases = @(New-RecorderCases -RecorderOutput $RecorderOutput)
    $recorder = Invoke-CaptureRecorder `
        -FunctionText $functionContract.FunctionText `
        -Cases $cases `
        -RecorderRoot $root `
        -RecorderOutput $RecorderOutput
    Assert-RecordedCaptureContract -Recorder $recorder -Cases $cases `
        -RecorderRoot $root -RecorderOutput $RecorderOutput

    $proofCase = @($cases | Where-Object {
            $_.Renderer -ceq 'OpenGL' -and $_.Scale -eq 100 -and
                -not $_.MovieEnabled -and -not $_.MetricsEnabled
        })
    Assert-True ($proofCase.Count -eq 1) `
        'Legacy capture recorder lacks the canonical Godot proof case'
    $proofRecord = @($recorder.Records | Where-Object {
            $_.CaseId -ceq $proofCase[0].Id -and $_.Attempt -eq 1
        })
    Assert-True ($proofRecord.Count -eq 1) `
        'Legacy capture recorder lacks one effective argv for the Godot proof'

    return [pscustomobject]@{
        FunctionContract = $functionContract
        Scene = $expectedScene
        ArgumentList = [string[]]@($proofRecord[0].ArgumentList)
        MatrixCount = $cases.Count
        RecordCount = @($recorder.Records).Count
        ExternalLaunchCount = $recorder.ExternalLaunchCount
        OutputWriteCount = 0
    }
}

function Assert-CaptureContractFailure {
    param(
        [Parameter(Mandatory)][string]$ScriptPath,
        [Parameter(Mandatory)][string]$RecorderOutput,
        [Parameter(Mandatory)][string]$ExpectedFailure,
        [Parameter(Mandatory)][string]$Label
    )

    $failure = ''
    try {
        $null = Get-ValidatedCaptureContract `
            -ScriptPath $ScriptPath -RecorderOutput $RecorderOutput
    } catch {
        $failure = $_.Exception.Message
    }
    Assert-True ($failure -ceq $ExpectedFailure) `
        "$Label did not cause the exact recorder RED: $failure"
}

Assert-True ((Split-Path -Parent $captureScriptPath) -ceq (Join-Path $root 'tools')) `
    'Capture contract resolved outside the canonical tools directory'
Assert-True ($projectPath -ceq (Join-Path $root 'game\project.godot')) `
    'Capture contract resolved a non-canonical project file'
Assert-True (Test-Path -LiteralPath $godotPath -PathType Leaf) `
    "Pinned Godot executable missing: $godotPath"

$functionContract = Get-CaptureFunctionContract -ScriptPath $captureScriptPath
$sceneStatement = "`$arguments.Add('$expectedScene')"
$temporaryParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$temporaryRoot = Join-Path $temporaryParent `
    ('ninho-legacy-scene-contract-' + [Guid]::NewGuid().ToString('N'))
$reachabilityId = [Guid]::NewGuid().ToString('N')
$toolsDirectory = Join-Path $root 'tools'
$probeScriptPath = Join-Path $toolsDirectory `
    "capture_vertical_slice.reachability-$reachabilityId.ps1"
$artifactsDirectory = Join-Path $root 'artifacts'
$reachabilityRoot = Join-Path $artifactsDirectory `
    "vertical-slice-reachability-$reachabilityId"
Assert-NinhoNoReparseAncestors -Path $temporaryRoot -AllowedRoot $temporaryParent |
    Out-Null
Assert-NinhoNoReparseAncestors `
    -Path $probeScriptPath -AllowedRoot $toolsDirectory | Out-Null
Assert-NinhoNoReparseAncestors `
    -Path $reachabilityRoot -AllowedRoot $artifactsDirectory | Out-Null
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
try {
    $recorderOutput = Join-Path $temporaryRoot 'recorder-output'
    $contract = Get-ValidatedCaptureContract `
        -ScriptPath $captureScriptPath -RecorderOutput $recorderOutput
    $canonicalReachabilityOutput = Join-Path $reachabilityRoot 'canonical\output'
    $canonicalReachabilityGolden = Join-Path $reachabilityRoot 'canonical\golden'
    $topLevelReachability = Get-TopLevelReachabilityContract `
        -FunctionContract $functionContract `
        -ScriptText $functionContract.ScriptText `
        -ProbeScriptPath $probeScriptPath `
        -OutputDirectory $canonicalReachabilityOutput `
        -GoldenDirectory $canonicalReachabilityGolden

    $fixtureDefinitions = @(
        [pscustomobject]@{
            Name = 'capture-without-scene.ps1'
            Label = 'Removing the executable scene statement'
            Replacement = ''
            Failure = $sceneFailure
        },
        [pscustomobject]@{
            Name = 'capture-commented-scene.ps1'
            Label = 'Replacing the scene statement with a comment'
            Replacement = "# $sceneStatement"
            Failure = $sceneFailure
        },
        [pscustomobject]@{
            Name = 'capture-string-scene.ps1'
            Label = 'Replacing the scene statement with a loose string'
            Replacement = "'" + $sceneStatement.Replace("'", "''") + "'"
            Failure = $sceneFailure
        },
        [pscustomobject]@{
            Name = 'capture-duplicate-scene.ps1'
            Label = 'Duplicating the executable scene statement'
            Replacement = $sceneStatement + "`r`n        " + $sceneStatement
            Failure = $sceneFailure
        },
        [pscustomobject]@{
            Name = 'capture-remove-scene.ps1'
            Label = 'Removing the scene through the effective list'
            Replacement = $sceneStatement + "`r`n        " +
                "`$arguments.Remove('$expectedScene')"
            Failure = $sceneFailure
        },
        [pscustomobject]@{
            Name = 'capture-indexed-assignment.ps1'
            Label = 'Mutating the effective list through an index'
            Replacement = $sceneStatement + "`r`n        " +
                "`$arguments[0] = 'tampered'"
            Failure = $argvFailure
        },
        [pscustomobject]@{
            Name = 'capture-provider-arguments.ps1'
            Label = 'Clearing the effective list through variable provider syntax'
            Replacement = $sceneStatement + "`r`n        " +
                '${variable:arguments}.Clear()'
            Failure = $sceneFailure
        },
        [pscustomobject]@{
            Name = 'capture-private-arguments.ps1'
            Label = 'Clearing the effective list through private scope syntax'
            Replacement = $sceneStatement + "`r`n        " +
                '${private:arguments}.Clear()'
            Failure = $sceneFailure
        },
        [pscustomobject]@{
            Name = 'capture-get-variable-arguments.ps1'
            Label = 'Clearing the effective list through Get-Variable'
            Replacement = $sceneStatement + "`r`n        " +
                '(Get-Variable arguments -ValueOnly).Clear()'
            Failure = $sceneFailure
        }
    )
    foreach ($fixture in $fixtureDefinitions) {
        $fixturePath = New-CaptureFixture `
            -FunctionContract $functionContract `
            -Directory $temporaryRoot `
            -Name $fixture.Name `
            -Search $sceneStatement `
            -Replacement $fixture.Replacement
        Assert-CaptureContractFailure `
            -ScriptPath $fixturePath `
            -RecorderOutput $recorderOutput `
            -ExpectedFailure $fixture.Failure `
            -Label $fixture.Label
    }

    $unreachableText = Disable-CaptureCallSites `
        -FunctionContract $functionContract
    $unreachableReachabilityOutput = Join-Path $reachabilityRoot 'unreachable\output'
    $unreachableReachabilityGolden = Join-Path $reachabilityRoot 'unreachable\golden'
    $reachabilityRed = ''
    try {
        $null = Get-TopLevelReachabilityContract `
            -FunctionContract $functionContract `
            -ScriptText $unreachableText `
            -ProbeScriptPath $probeScriptPath `
            -OutputDirectory $unreachableReachabilityOutput `
            -GoldenDirectory $unreachableReachabilityGolden
    } catch {
        $reachabilityRed = $_.Exception.Message
    }
    Assert-True ($reachabilityRed -ceq $reachabilityFailure) `
        "False-guarded call sites did not cause the exact reachability RED: $reachabilityRed"
    $reachabilityFiles = @(Get-ChildItem -LiteralPath $reachabilityRoot `
        -Recurse -File -ErrorAction SilentlyContinue)
    Assert-True ($reachabilityFiles.Count -eq 0) `
        'Top-level reachability proof performed an external file operation'

    $gameSource = Join-Path $root 'game'
    $reparseSources = @(Get-ChildItem -LiteralPath $gameSource -Recurse -Force |
        Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint })
    Assert-True ($reparseSources.Count -eq 0) `
        'Refusing to copy a canonical game tree containing reparse points'
    $temporaryGame = Join-Path $temporaryRoot 'game'
    Copy-Item -LiteralPath $gameSource -Destination $temporaryGame -Recurse -Force

    $temporaryProject = Join-Path $temporaryGame 'project.godot'
    $projectText = [IO.File]::ReadAllText($temporaryProject)
    $mainSceneMatches = @([regex]::Matches(
            $projectText, '(?m)^run/main_scene="([^"]+)"\r?$'))
    Assert-True ($mainSceneMatches.Count -eq 1) `
        'Temporary project must define exactly one run/main_scene'
    $originalMainScene = $mainSceneMatches[0].Groups[1].Value
    $mutatedMainScene = 'res://scenes/__legacy_contract_wrong_main__.tscn'
    $mutatedProjectText = [regex]::Replace(
        $projectText,
        '(?m)^run/main_scene="[^"]+"\r?$',
        "run/main_scene=`"$mutatedMainScene`"")
    Assert-True ($mutatedProjectText -cne $projectText) `
        'Temporary project fixture did not change run/main_scene'
    [IO.File]::WriteAllText(
        $temporaryProject, $mutatedProjectText, [Text.UTF8Encoding]::new($false))
    Assert-True (-not (Test-Path -LiteralPath (
                Join-Path $temporaryGame 'scenes\__legacy_contract_wrong_main__.tscn'))) `
        'Temporary wrong main scene unexpectedly exists'

    $stdoutPath = Join-Path $temporaryRoot 'godot.stdout.log'
    $stderrPath = Join-Path $temporaryRoot 'godot.stderr.log'
    $process = Invoke-NinhoTimedProcess `
        -FilePath $godotPath `
        -ArgumentList $contract.ArgumentList `
        -TimeoutMs 120000 `
        -StdoutPath $stdoutPath `
        -StderrPath $stderrPath `
        -WorkingDirectory $temporaryRoot `
        -FatalMarker 'NINHO_CAPTURE_FATAL name=legacy-explicit-scene-contract reason=timeout'
    Assert-True ($process.ExitCode -eq 0) `
        "Explicit-scene Godot proof failed with exit code $($process.ExitCode)"
    $runtimeLog = [IO.File]::ReadAllText($stdoutPath) + "`n" +
        [IO.File]::ReadAllText($stderrPath)
    foreach ($forbidden in 'ERROR:', 'SCRIPT ERROR:') {
        Assert-True (-not $runtimeLog.Contains($forbidden)) `
            "Explicit-scene Godot proof emitted $forbidden"
    }
    $completionMarker = 'VERTICAL_SLICE_CAPTURE_COMPLETE frame=300'
    $completionCount = [regex]::Matches(
        $runtimeLog, [regex]::Escape($completionMarker)).Count
    Assert-True ($completionCount -eq 1) `
        "Explicit-scene Godot proof must emit one completion marker, found $completionCount"

    Write-Output (
        'vertical slice explicit scene contract: PASS ' +
        "capture=$($contract.Scene) original_main=$originalMainScene " +
        "mutated_main=$mutatedMainScene " +
        'dynamic_red=removed,comment,string,duplicate,remove,indexed,provider,private,get-variable ' +
        "recorder_matrix=$($contract.MatrixCount) " +
        "recorded_attempts=$($contract.RecordCount) " +
        "recorder_external_launches=$($contract.ExternalLaunchCount) " +
        "recorder_output_writes=$($contract.OutputWriteCount) " +
        'top_level_red=false-guarded-call-sites ' +
        'top_level_reachability=Invoke-CaptureRun>Invoke-NinhoTimedProcess ' +
        "top_level_case=$($topLevelReachability.name) " +
        'top_level_external_launches=0 top_level_breakpoints=0 ' +
        "runtime_marker=$completionMarker argv=$($contract.ArgumentList -join '|')")
} finally {
    $resolvedProbeScript = [IO.Path]::GetFullPath($probeScriptPath)
    Assert-True ((Split-Path -Parent $resolvedProbeScript) -ceq $toolsDirectory -and
            [IO.Path]::GetFileName($resolvedProbeScript).StartsWith(
                'capture_vertical_slice.reachability-', [StringComparison]::Ordinal) -and
            [IO.Path]::GetExtension($resolvedProbeScript) -ceq '.ps1') `
        'Refusing to remove an unexpected top-level reachability probe'
    if (Test-Path -LiteralPath $resolvedProbeScript) {
        Remove-Item -LiteralPath $resolvedProbeScript -Force
    }
    $resolvedReachabilityRoot = [IO.Path]::GetFullPath($reachabilityRoot)
    Assert-True ((Split-Path -Parent $resolvedReachabilityRoot) -ceq
            $artifactsDirectory -and
            [IO.Path]::GetFileName($resolvedReachabilityRoot).StartsWith(
                'vertical-slice-reachability-', [StringComparison]::Ordinal)) `
        'Refusing to remove an unexpected reachability artifact root'
    if (Test-Path -LiteralPath $resolvedReachabilityRoot) {
        Assert-NinhoNoReparseAncestors `
            -Path $resolvedReachabilityRoot -AllowedRoot $artifactsDirectory |
            Out-Null
        Remove-Item -LiteralPath $resolvedReachabilityRoot -Recurse -Force
    }
    $resolvedTemporaryRoot = [IO.Path]::GetFullPath($temporaryRoot)
    $resolvedParent = [IO.Path]::GetFullPath((Split-Path -Parent $resolvedTemporaryRoot))
    Assert-True ($resolvedParent.TrimEnd('\', '/') -ceq
            $temporaryParent.TrimEnd('\', '/')) `
        'Refusing to remove temporary fixture outside the system temporary root'
    Assert-True ([IO.Path]::GetFileName($resolvedTemporaryRoot).StartsWith(
            'ninho-legacy-scene-contract-', [StringComparison]::Ordinal)) `
        'Refusing to remove an unexpected temporary fixture'
    if (Test-Path -LiteralPath $resolvedTemporaryRoot) {
        Assert-NinhoNoReparseAncestors `
            -Path $resolvedTemporaryRoot -AllowedRoot $temporaryParent | Out-Null
        Remove-Item -LiteralPath $resolvedTemporaryRoot -Recurse -Force
    }
}
