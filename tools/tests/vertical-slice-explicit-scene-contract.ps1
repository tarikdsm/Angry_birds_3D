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

function Get-NearestScriptBlockAst {
    param([Parameter(Mandatory)]$Node)

    for ($current = $Node.Parent; $null -ne $current; $current = $current.Parent) {
        if ($current -is [Management.Automation.Language.ScriptBlockAst]) {
            return $current
        }
    }
    throw 'Legacy capture AST node is not inside a script block'
}

function Get-NearestIfStatementAst {
    param(
        [Parameter(Mandatory)]$Node,
        [Parameter(Mandatory)]$StopAt
    )

    for ($current = $Node.Parent; $null -ne $current -and $current -ne $StopAt;
            $current = $current.Parent) {
        if ($current -is [Management.Automation.Language.IfStatementAst]) {
            return $current
        }
    }
    return $null
}

function Assert-UnconditionalInBlock {
    param(
        [Parameter(Mandatory)]$Node,
        [Parameter(Mandatory)]$Block,
        [Parameter(Mandatory)][string]$Label
    )

    for ($current = $Node.Parent; $null -ne $current -and $current -ne $Block;
            $current = $current.Parent) {
        if ($current -is [Management.Automation.Language.IfStatementAst] -or
                $current -is [Management.Automation.Language.LoopStatementAst] -or
                $current -is [Management.Automation.Language.TryStatementAst] -or
                $current -is [Management.Automation.Language.ScriptBlockAst] -or
                $current -is [Management.Automation.Language.FunctionDefinitionAst]) {
            throw "Legacy capture $Label must execute unconditionally in the argv block"
        }
    }
    Assert-True ($current -eq $Block) `
        "Legacy capture $Label is outside the argv block"
}

function Get-StringConstants {
    param([Parameter(Mandatory)]$Node)

    return @($Node.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.StringConstantExpressionAst]
            }, $true) | ForEach-Object { [string]$_.Value })
}

function Get-LiteralStringArray {
    param(
        [Parameter(Mandatory)]$Call,
        [Parameter(Mandatory)][string]$Label
    )

    Assert-True ($Call.Arguments.Count -eq 1) `
        "Legacy capture $Label AddRange must have one argument"
    $argument = $Call.Arguments[0]
    Assert-True ($argument -is [Management.Automation.Language.ConvertExpressionAst] -and
            $argument.StaticType -eq [string[]] -and
            $argument.Child -is [Management.Automation.Language.ArrayExpressionAst]) `
        "Legacy capture $Label must be a literal string[] AddRange"
    $variables = @($argument.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.VariableExpressionAst] -or
                    $candidate -is [Management.Automation.Language.ExpandableStringExpressionAst]
            }, $true))
    Assert-True ($variables.Count -eq 0) `
        "Legacy capture $Label must not contain dynamic argv values"
    $arrayLiterals = @($argument.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.ArrayLiteralAst]
            }, $true))
    Assert-True ($arrayLiterals.Count -eq 1) `
        "Legacy capture $Label must contain one literal argv array"
    $values = @()
    foreach ($element in $arrayLiterals[0].Elements) {
        Assert-True ($element -is [Management.Automation.Language.StringConstantExpressionAst]) `
            "Legacy capture $Label contains a non-literal argv value"
        $values += [string]$element.Value
    }
    return [string[]]$values
}

function Get-CaptureArgumentContract {
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

    $processCommands = @($function.Body.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.CommandAst] -and
                    $candidate.GetCommandName() -ceq 'Invoke-NinhoTimedProcess'
            }, $true))
    Assert-True ($processCommands.Count -eq 1) `
        'Legacy capture must invoke exactly one timed process per capture run'
    $processCommand = $processCommands[0]
    $argumentListAst = $null
    for ($index = 0; $index -lt $processCommand.CommandElements.Count; ++$index) {
        $element = $processCommand.CommandElements[$index]
        if ($element -is [Management.Automation.Language.CommandParameterAst] -and
                $element.ParameterName -ceq 'ArgumentList') {
            Assert-True ($null -eq $argumentListAst -and
                    $index + 1 -lt $processCommand.CommandElements.Count) `
                'Legacy capture process has an invalid -ArgumentList parameter'
            $argumentListAst = $processCommand.CommandElements[$index + 1]
        }
    }
    Assert-True ($argumentListAst -is [Management.Automation.Language.ArrayExpressionAst]) `
        'Legacy capture process must pass @($arguments) as -ArgumentList'
    $argumentStatements = @($argumentListAst.SubExpression.Statements)
    Assert-True ($argumentStatements.Count -eq 1 -and
            $argumentStatements[0].PipelineElements.Count -eq 1 -and
            $argumentStatements[0].PipelineElements[0] -is
                [Management.Automation.Language.CommandExpressionAst] -and
            $argumentStatements[0].PipelineElements[0].Expression -is
                [Management.Automation.Language.VariableExpressionAst] -and
            $argumentStatements[0].PipelineElements[0].Expression.VariablePath.UserPath -ceq
                'arguments') `
        'Legacy capture process must pass only @($arguments) as -ArgumentList'
    $argumentVariables = @($argumentListAst.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.VariableExpressionAst]
            }, $true))
    Assert-True ($argumentVariables.Count -eq 1 -and
            $argumentVariables[0].VariablePath.UserPath -ceq 'arguments') `
        'Legacy capture process must consume the validated $arguments list'

    $argumentBlock = Get-NearestScriptBlockAst -Node $processCommand
    $initializers = @($argumentBlock.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.AssignmentStatementAst] -and
                    $candidate.Left -is [Management.Automation.Language.VariableExpressionAst] -and
                    $candidate.Left.VariablePath.UserPath -ceq 'arguments'
            }, $true))
    Assert-True ($initializers.Count -eq 1 -and
            $initializers[0].Operator -eq 'Equals' -and
            $initializers[0].Right.Extent.Text -ceq
                '[Collections.Generic.List[string]]::new()') `
        'Legacy capture must initialize one concrete string argv list'

    $argumentCalls = @($argumentBlock.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.InvokeMemberExpressionAst] -and
                    $candidate.Expression -is [Management.Automation.Language.VariableExpressionAst] -and
                    $candidate.Expression.VariablePath.UserPath -ceq 'arguments'
            }, $true))
    $functionArgumentCalls = @($function.Body.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.InvokeMemberExpressionAst] -and
                    $candidate.Expression -is [Management.Automation.Language.VariableExpressionAst] -and
                    $candidate.Expression.VariablePath.UserPath -ceq 'arguments'
            }, $true))
    Assert-True ($argumentCalls.Count -eq $functionArgumentCalls.Count) `
        'Legacy capture has $arguments calls outside the process argv block'

    $literalAddCalls = @($argumentCalls | Where-Object {
            $_.Member.Value -ceq 'Add' -and $_.Arguments.Count -eq 1 -and
                $_.Arguments[0] -is
                    [Management.Automation.Language.StringConstantExpressionAst]
        })
    $sceneCalls = @($literalAddCalls | Where-Object {
            [string]$_.Arguments[0].Value -match '^res://.+\.tscn$'
        })
    if ($sceneCalls.Count -ne 1) { throw $sceneFailure }
    $sceneCall = $sceneCalls[0]
    Assert-True ($sceneCall.Arguments[0].StringConstantType -eq
            [Management.Automation.Language.StringConstantType]::SingleQuoted) `
        'Legacy capture executable scene must be a single-quoted literal'
    $scene = [string]$sceneCall.Arguments[0].Value
    Assert-True ($scene -ceq $expectedScene) `
        "Legacy capture executable scene changed: $scene"

    $rangeCalls = @($argumentCalls | Where-Object { $_.Member.Value -ceq 'AddRange' })
    $pathCalls = @($rangeCalls | Where-Object {
            (Get-StringConstants -Node $_.Arguments[0]) -ccontains '--path'
        })
    Assert-True ($pathCalls.Count -eq 1) `
        'Legacy capture must have one executable --path AddRange call'
    $pathArguments = @(Get-LiteralStringArray -Call $pathCalls[0] -Label 'project path')
    Assert-True (Test-ExactSequence -Actual $pathArguments -Expected @(
                '--path', 'game', '--resolution', '1920x1080')) `
        'Legacy capture project path argv changed'

    $rendererCalls = @($rangeCalls | Where-Object {
            (Get-StringConstants -Node $_.Arguments[0]) -ccontains '--rendering-method'
        })
    Assert-True ($rendererCalls.Count -eq 1) `
        'Legacy capture must have one executable renderer AddRange call'
    $rendererArguments = @(
        Get-LiteralStringArray -Call $rendererCalls[0] -Label 'OpenGL renderer')
    Assert-True (Test-ExactSequence -Actual $rendererArguments -Expected @(
                '--rendering-method', 'gl_compatibility')) `
        'Legacy capture OpenGL renderer argv changed'
    $rendererIf = Get-NearestIfStatementAst `
        -Node $rendererCalls[0] -StopAt $argumentBlock
    Assert-True ($null -ne $rendererIf -and $rendererIf.Clauses.Count -eq 1 -and
            $rendererIf.Clauses[0].Item1.Extent.Text -ceq "`$Renderer -ceq 'OpenGL'") `
        'Legacy capture renderer argv is not guarded by the OpenGL branch'

    $delimiterCalls = @($literalAddCalls | Where-Object {
            [string]$_.Arguments[0].Value -ceq '--'
        })
    $captureFlagCalls = @($literalAddCalls | Where-Object {
            [string]$_.Arguments[0].Value -ceq '--vertical-slice-capture'
        })
    Assert-True ($delimiterCalls.Count -eq 1) `
        'Legacy capture must add exactly one executable user-argument delimiter'
    Assert-True ($captureFlagCalls.Count -eq 1) `
        'Legacy capture must add exactly one executable capture flag'

    $scaleCalls = @($argumentCalls | Where-Object {
            $_.Member.Value -ceq 'Add' -and $_.Arguments.Count -eq 1 -and
                $_.Arguments[0] -is
                    [Management.Automation.Language.ExpandableStringExpressionAst] -and
                $_.Arguments[0].Value -ceq '--ui-scale=$Scale'
        })
    Assert-True ($scaleCalls.Count -eq 1) `
        'Legacy capture must add exactly one executable UI scale argument'
    $scaleVariables = @($scaleCalls[0].Arguments[0].FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.VariableExpressionAst]
            }, $true))
    Assert-True ($scaleVariables.Count -eq 1 -and
            $scaleVariables[0].VariablePath.UserPath -ceq 'Scale') `
        'Legacy capture UI scale argument must derive only from $Scale'

    foreach ($required in @(
            @($initializers[0], 'argv initialization'),
            @($pathCalls[0], 'project path'),
            @($sceneCall, 'explicit scene'),
            @($delimiterCalls[0], 'user-argument delimiter'),
            @($captureFlagCalls[0], 'capture flag'),
            @($scaleCalls[0], 'UI scale'),
            @($processCommand, 'timed process invocation')
        )) {
        Assert-UnconditionalInBlock -Node $required[0] `
            -Block $argumentBlock -Label $required[1]
    }

    $orderedNodes = @(
        $initializers[0], $rendererCalls[0], $pathCalls[0], $sceneCall,
        $delimiterCalls[0], $captureFlagCalls[0], $scaleCalls[0], $processCommand)
    for ($index = 1; $index -lt $orderedNodes.Count; ++$index) {
        Assert-True ($orderedNodes[$index - 1].Extent.StartOffset -lt
                $orderedNodes[$index].Extent.StartOffset) `
            'Legacy capture executable argv statements changed relative order'
    }

    $scaleArgument = $scaleCalls[0].Arguments[0].Value.Replace('$Scale', '100')
    $derivedArguments = @(
        $rendererArguments
        $pathArguments
        $scene
        [string]$delimiterCalls[0].Arguments[0].Value
        [string]$captureFlagCalls[0].Arguments[0].Value
        [string]$scaleArgument
    )
    return [pscustomobject]@{
        Ast = $ast
        Scene = $scene
        SceneCall = $sceneCall
        ArgumentList = [string[]]$derivedArguments
    }
}

function Replace-AstExtent {
    param(
        [Parameter(Mandatory)][string]$Text,
        [Parameter(Mandatory)]$Node,
        [AllowEmptyString()][string]$Replacement
    )

    return $Text.Substring(0, $Node.Extent.StartOffset) + $Replacement +
        $Text.Substring($Node.Extent.EndOffset)
}

function Assert-SceneContractFailure {
    param(
        [Parameter(Mandatory)][string]$ScriptPath,
        [Parameter(Mandatory)][string]$Label
    )

    $failure = ''
    try {
        $null = Get-CaptureArgumentContract -ScriptPath $ScriptPath
    } catch {
        $failure = $_.Exception.Message
    }
    Assert-True ($failure -ceq $sceneFailure) `
        "$Label did not cause the exact scene-contract RED: $failure"
}

Assert-True ((Split-Path -Parent $captureScriptPath) -ceq (Join-Path $root 'tools')) `
    'Capture contract resolved outside the canonical tools directory'
Assert-True ($projectPath -ceq (Join-Path $root 'game\project.godot')) `
    'Capture contract resolved a non-canonical project file'
Assert-True (Test-Path -LiteralPath $godotPath -PathType Leaf) `
    "Pinned Godot executable missing: $godotPath"

$contract = Get-CaptureArgumentContract -ScriptPath $captureScriptPath
$captureText = [IO.File]::ReadAllText($captureScriptPath)
$sceneCallText = $contract.SceneCall.Extent.Text

$temporaryParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$temporaryRoot = Join-Path $temporaryParent `
    ('ninho-legacy-scene-contract-' + [Guid]::NewGuid().ToString('N'))
Assert-NinhoNoReparseAncestors -Path $temporaryRoot -AllowedRoot $temporaryParent |
    Out-Null
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
try {
    $removedPath = Join-Path $temporaryRoot 'capture-without-scene.ps1'
    $removedText = Replace-AstExtent -Text $captureText `
        -Node $contract.SceneCall -Replacement ''
    [IO.File]::WriteAllText($removedPath, $removedText, [Text.UTF8Encoding]::new($false))
    Assert-SceneContractFailure -ScriptPath $removedPath -Label 'Removing only the scene AST node'

    $commentedPath = Join-Path $temporaryRoot 'capture-commented-scene.ps1'
    $commentedText = Replace-AstExtent -Text $captureText -Node $contract.SceneCall `
        -Replacement ("# " + $sceneCallText)
    [IO.File]::WriteAllText(
        $commentedPath, $commentedText, [Text.UTF8Encoding]::new($false))
    Assert-SceneContractFailure -ScriptPath $commentedPath `
        -Label 'Replacing the scene AST node with a comment'

    $stringPath = Join-Path $temporaryRoot 'capture-string-scene.ps1'
    $looseString = "'" + $sceneCallText.Replace("'", "''") + "'"
    $stringText = Replace-AstExtent -Text $captureText `
        -Node $contract.SceneCall -Replacement $looseString
    [IO.File]::WriteAllText($stringPath, $stringText, [Text.UTF8Encoding]::new($false))
    Assert-SceneContractFailure -ScriptPath $stringPath `
        -Label 'Replacing the scene AST node with a loose string'

    $duplicatePath = Join-Path $temporaryRoot 'capture-duplicate-scene.ps1'
    $duplicateText = Replace-AstExtent -Text $captureText -Node $contract.SceneCall `
        -Replacement ($sceneCallText + "`r`n        " + $sceneCallText)
    [IO.File]::WriteAllText(
        $duplicatePath, $duplicateText, [Text.UTF8Encoding]::new($false))
    Assert-SceneContractFailure -ScriptPath $duplicatePath `
        -Label 'Duplicating the executable scene AST node'

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
        "mutated_main=$mutatedMainScene ast_red=removed,comment,string,duplicate " +
        "runtime_marker=$completionMarker argv=$($contract.ArgumentList -join '|')")
} finally {
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
