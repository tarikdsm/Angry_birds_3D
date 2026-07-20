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
$usageFailure = 'Legacy capture $arguments usage must match the exhaustive executable argv contract'

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

function Test-ArgumentsVariableAst {
    param([Parameter(Mandatory)]$Node)

    if ($Node -isnot [Management.Automation.Language.VariableExpressionAst]) {
        return $false
    }
    return [string]$Node.VariablePath.UserPath -imatch '^(local:)?arguments$'
}

function Assert-ExactGuard {
    param(
        [Parameter(Mandatory)]$Node,
        [Parameter(Mandatory)]$Block,
        [Parameter(Mandatory)][string]$Condition,
        [Parameter(Mandatory)][string]$Label
    )

    $guards = @()
    $unexpectedControlFlow = $false
    for ($current = $Node.Parent; $null -ne $current -and $current -ne $Block;
            $current = $current.Parent) {
        if ($current -is [Management.Automation.Language.IfStatementAst]) {
            $guards += $current
        } elseif ($current -is [Management.Automation.Language.LoopStatementAst] -or
                $current -is [Management.Automation.Language.TryStatementAst] -or
                $current -is [Management.Automation.Language.ScriptBlockAst] -or
                $current -is [Management.Automation.Language.FunctionDefinitionAst]) {
            $unexpectedControlFlow = $true
        }
    }
    Assert-True ($current -eq $Block -and -not $unexpectedControlFlow -and
            $guards.Count -eq 1 -and $guards[0].Clauses.Count -eq 1 -and
            $null -eq $guards[0].ElseClause -and
            $guards[0].Clauses[0].Item1.Extent.Text -ceq $Condition) `
        "Legacy capture $Label guard must be exactly: $Condition"
    return $guards[0]
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
                    (Test-ArgumentsVariableAst -Node $candidate.Left)
            }, $true))
    Assert-True ($initializers.Count -eq 1 -and
            $initializers[0].Left.VariablePath.UserPath -ceq 'arguments' -and
            $initializers[0].Operator -eq 'Equals' -and
            $initializers[0].Right.Extent.Text -ceq
                '[Collections.Generic.List[string]]::new()') `
        'Legacy capture must initialize one concrete string argv list'

    $argumentCalls = @($argumentBlock.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.InvokeMemberExpressionAst] -and
                    (Test-ArgumentsVariableAst -Node $candidate.Expression)
            }, $true) | Sort-Object { $_.Extent.StartOffset })

    # Preserve the focused scene diagnostic before enforcing the complete argv-use set.
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

    $allArgumentReferences = @($ast.FindAll({
                param($candidate)
                Test-ArgumentsVariableAst -Node $candidate
            }, $true) | Sort-Object { $_.Extent.StartOffset })
    $blockArgumentReferences = @($argumentBlock.FindAll({
                param($candidate)
                Test-ArgumentsVariableAst -Node $candidate
            }, $true) | Sort-Object { $_.Extent.StartOffset })
    if ($argumentCalls.Count -ne 9 -or
            $allArgumentReferences.Count -ne 11 -or
            $blockArgumentReferences.Count -ne 11) {
        throw $usageFailure
    }
    for ($index = 0; $index -lt $allArgumentReferences.Count; ++$index) {
        if ($allArgumentReferences[$index].Extent.StartOffset -ne
                $blockArgumentReferences[$index].Extent.StartOffset) {
            throw $usageFailure
        }
    }

    $rendererCall = $argumentCalls[0]
    $pathCall = $argumentCalls[1]
    $movieCall = $argumentCalls[2]
    $orderedSceneCall = $argumentCalls[3]
    $delimiterCall = $argumentCalls[4]
    $captureFlagCall = $argumentCalls[5]
    $scaleCall = $argumentCalls[6]
    $normalTerminalCall = $argumentCalls[7]
    $metricsCall = $argumentCalls[8]

    $expectedMembers = @(
        'AddRange', 'AddRange', 'AddRange', 'Add', 'Add', 'Add', 'Add', 'Add', 'Add')
    for ($index = 0; $index -lt $argumentCalls.Count; ++$index) {
        if ($argumentCalls[$index].Expression.VariablePath.UserPath -cne 'arguments' -or
                $argumentCalls[$index].NullConditional -or
                $argumentCalls[$index].Member -isnot
                    [Management.Automation.Language.StringConstantExpressionAst] -or
                $argumentCalls[$index].Member.Value -cne $expectedMembers[$index]) {
            throw $usageFailure
        }
    }
    if ($orderedSceneCall.Extent.StartOffset -ne $sceneCall.Extent.StartOffset) {
        throw $usageFailure
    }

    $rendererArguments = @(
        Get-LiteralStringArray -Call $rendererCall -Label 'OpenGL renderer')
    Assert-True (Test-ExactSequence -Actual $rendererArguments -Expected @(
                '--rendering-method', 'gl_compatibility')) `
        'Legacy capture OpenGL renderer argv changed'
    $rendererGuard = Assert-ExactGuard -Node $rendererCall -Block $argumentBlock `
        -Condition "`$Renderer -ceq 'OpenGL'" -Label 'renderer argv'

    $pathArguments = @(Get-LiteralStringArray -Call $pathCall -Label 'project path')
    Assert-True (Test-ExactSequence -Actual $pathArguments -Expected @(
                '--path', 'game', '--resolution', '1920x1080')) `
        'Legacy capture project path argv changed'

    Assert-True ($movieCall.Arguments.Count -eq 1 -and
            $movieCall.Arguments[0].Extent.Text -ceq
                "[string[]]@('--write-movie',`$movieRelative,'--fixed-fps','60')") `
        'Legacy capture movie argv shape changed'
    $movieCallVariables = @($movieCall.Arguments[0].FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.VariableExpressionAst]
            }, $true))
    Assert-True ($movieCallVariables.Count -eq 1 -and
            $movieCallVariables[0].VariablePath.UserPath -ceq 'movieRelative') `
        'Legacy capture movie argv must derive only from $movieRelative'
    $movieGuard = Assert-ExactGuard -Node $movieCall -Block $argumentBlock `
        -Condition '$Movie' -Label 'movie argv'

    foreach ($literalContract in @(
            @($sceneCall, $expectedScene, 'explicit scene'),
            @($delimiterCall, '--', 'user-argument delimiter'),
            @($captureFlagCall, '--vertical-slice-capture', 'capture flag'),
            @($normalTerminalCall, '--capture-normal-terminal', 'normal-terminal flag')
        )) {
        $call = $literalContract[0]
        Assert-True ($call.Arguments.Count -eq 1 -and
                $call.Arguments[0] -is
                    [Management.Automation.Language.StringConstantExpressionAst] -and
                $call.Arguments[0].StringConstantType -eq
                    [Management.Automation.Language.StringConstantType]::SingleQuoted -and
                [string]$call.Arguments[0].Value -ceq $literalContract[1]) `
            "Legacy capture $($literalContract[2]) argv shape changed"
    }

    Assert-True ($scaleCall.Arguments.Count -eq 1 -and
            $scaleCall.Arguments[0] -is
                [Management.Automation.Language.ExpandableStringExpressionAst] -and
            $scaleCall.Arguments[0].Value -ceq '--ui-scale=$Scale') `
        'Legacy capture UI scale argv shape changed'
    $scaleVariables = @($scaleCall.Arguments[0].FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.VariableExpressionAst]
            }, $true))
    Assert-True ($scaleVariables.Count -eq 1 -and
            $scaleVariables[0].VariablePath.UserPath -ceq 'Scale') `
        'Legacy capture UI scale argument must derive only from $Scale'

    $normalTerminalGuard = Assert-ExactGuard -Node $normalTerminalCall `
        -Block $argumentBlock -Condition '$Movie' -Label 'normal-terminal argv'

    Assert-True ($metricsCall.Arguments.Count -eq 1 -and
            $metricsCall.Arguments[0] -is
                [Management.Automation.Language.ExpandableStringExpressionAst] -and
            $metricsCall.Arguments[0].Value -ceq
                '--vertical-slice-metrics=res://$metricsRelative') `
        'Legacy capture metrics argv shape changed'
    $metricsCallVariables = @($metricsCall.Arguments[0].FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.VariableExpressionAst]
            }, $true))
    Assert-True ($metricsCallVariables.Count -eq 1 -and
            $metricsCallVariables[0].VariablePath.UserPath -ceq 'metricsRelative') `
        'Legacy capture metrics argv must derive only from $metricsRelative'
    $metricsGuard = Assert-ExactGuard -Node $metricsCall -Block $argumentBlock `
        -Condition '$Metrics' -Label 'metrics argv'

    $movieAssignments = @($argumentBlock.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.AssignmentStatementAst] -and
                    $candidate.Left -is
                        [Management.Automation.Language.VariableExpressionAst] -and
                    $candidate.Left.VariablePath.UserPath -ceq 'movieRelative'
            }, $true))
    $movieReferences = @($ast.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.VariableExpressionAst] -and
                    $candidate.VariablePath.UserPath -ceq 'movieRelative'
            }, $true))
    Assert-True ($movieAssignments.Count -eq 1 -and
            $movieAssignments[0].Operator -eq 'Equals' -and
            $movieAssignments[0].Right.Extent.Text -ceq
                "(Get-RelativePath (Join-Path `$root 'game') `$Movie).Replace('\','/')" -and
            $movieReferences.Count -eq 2) `
        'Legacy capture $movieRelative derivation changed'
    $movieAssignmentGuard = Assert-ExactGuard -Node $movieAssignments[0] `
        -Block $argumentBlock -Condition '$Movie' -Label '$movieRelative derivation'
    Assert-True ($movieAssignmentGuard.Extent.StartOffset -eq
            $movieGuard.Extent.StartOffset) `
        'Legacy capture movie argv and derivation must share one guard'

    $metricsAssignments = @($argumentBlock.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.AssignmentStatementAst] -and
                    $candidate.Left -is
                        [Management.Automation.Language.VariableExpressionAst] -and
                    $candidate.Left.VariablePath.UserPath -ceq 'metricsRelative'
            }, $true))
    $metricsReferences = @($ast.FindAll({
                param($candidate)
                $candidate -is [Management.Automation.Language.VariableExpressionAst] -and
                    $candidate.VariablePath.UserPath -ceq 'metricsRelative'
            }, $true))
    Assert-True ($metricsAssignments.Count -eq 1 -and
            $metricsAssignments[0].Operator -eq 'Equals' -and
            $metricsAssignments[0].Right.Extent.Text -ceq
                "(Get-RelativePath (Join-Path `$root 'game') `$Metrics).Replace('\','/')" -and
            $metricsReferences.Count -eq 2) `
        'Legacy capture $metricsRelative derivation changed'
    $metricsAssignmentGuard = Assert-ExactGuard -Node $metricsAssignments[0] `
        -Block $argumentBlock -Condition '$Metrics' -Label '$metricsRelative derivation'
    Assert-True ($metricsAssignmentGuard.Extent.StartOffset -eq
            $metricsGuard.Extent.StartOffset) `
        'Legacy capture metrics argv and derivation must share one guard'

    Assert-True ($rendererGuard.Extent.StartOffset -ne
            $movieGuard.Extent.StartOffset -and
            $normalTerminalGuard.Extent.StartOffset -ne
                $movieGuard.Extent.StartOffset) `
        'Legacy capture renderer, movie, and normal-terminal guards must be distinct'

    foreach ($required in @(
            @($initializers[0], 'argv initialization'),
            @($pathCall, 'project path'),
            @($sceneCall, 'explicit scene'),
            @($delimiterCall, 'user-argument delimiter'),
            @($captureFlagCall, 'capture flag'),
            @($scaleCall, 'UI scale'),
            @($processCommand, 'timed process invocation')
        )) {
        Assert-UnconditionalInBlock -Node $required[0] `
            -Block $argumentBlock -Label $required[1]
    }

    $orderedNodes = @(
        $initializers[0], $rendererCall, $pathCall, $movieAssignments[0], $movieCall,
        $sceneCall, $delimiterCall, $captureFlagCall, $scaleCall,
        $normalTerminalCall, $metricsAssignments[0], $metricsCall, $processCommand)
    for ($index = 1; $index -lt $orderedNodes.Count; ++$index) {
        Assert-True ($orderedNodes[$index - 1].Extent.StartOffset -lt
                $orderedNodes[$index].Extent.StartOffset) `
            'Legacy capture executable argv statements changed relative order'
    }

    $classifiedArgumentReferences = @(
        $initializers[0].Left
        $rendererCall.Expression
        $pathCall.Expression
        $movieCall.Expression
        $sceneCall.Expression
        $delimiterCall.Expression
        $captureFlagCall.Expression
        $scaleCall.Expression
        $normalTerminalCall.Expression
        $metricsCall.Expression
        $argumentVariables[0]
    ) | Sort-Object { $_.Extent.StartOffset }
    for ($index = 0; $index -lt $allArgumentReferences.Count; ++$index) {
        if ($classifiedArgumentReferences[$index].Extent.StartOffset -ne
                $allArgumentReferences[$index].Extent.StartOffset) {
            throw $usageFailure
        }
    }

    # Runtime proof is derived only after every executable $arguments use is classified.
    $scaleArgument = $scaleCall.Arguments[0].Value.Replace('$Scale', '100')
    $derivedArguments = @(
        $rendererArguments
        $pathArguments
        $scene
        [string]$delimiterCall.Arguments[0].Value
        [string]$captureFlagCall.Arguments[0].Value
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

    $removePath = Join-Path $temporaryRoot 'capture-remove-scene.ps1'
    $removeText = Replace-AstExtent -Text $captureText -Node $contract.SceneCall `
        -Replacement ($sceneCallText + "`r`n        " +
            "`$arguments.Remove('$expectedScene')")
    [IO.File]::WriteAllText(
        $removePath, $removeText, [Text.UTF8Encoding]::new($false))
    $indexedPath = Join-Path $temporaryRoot 'capture-indexed-assignment.ps1'
    $indexedText = Replace-AstExtent -Text $captureText -Node $contract.SceneCall `
        -Replacement ($sceneCallText + "`r`n        " +
            "`$arguments[0] = 'tampered'")
    [IO.File]::WriteAllText(
        $indexedPath, $indexedText, [Text.UTF8Encoding]::new($false))
    $outsidePath = Join-Path $temporaryRoot 'capture-outside-arguments-use.ps1'
    $outsideText = $captureText + "`r`n`$null = `$arguments`r`n"
    [IO.File]::WriteAllText(
        $outsidePath, $outsideText, [Text.UTF8Encoding]::new($false))
    $usageFailures = @()
    foreach ($fixture in @(
            @($removePath, 'Remove(scene)'),
            @($indexedPath, 'indexed assignment'),
            @($outsidePath, 'unclassified use outside argv block')
        )) {
        $failure = ''
        try {
            $null = Get-CaptureArgumentContract -ScriptPath $fixture[0]
        } catch {
            $failure = $_.Exception.Message
        }
        $usageFailures += [pscustomobject]@{ Label = $fixture[1]; Failure = $failure }
    }
    $unexpectedUsageFailures = @($usageFailures | Where-Object {
            $_.Failure -cne $usageFailure
        })
    Assert-True ($unexpectedUsageFailures.Count -eq 0) `
        ('Legacy capture contract accepted or misclassified extra $arguments usage: ' +
            (($usageFailures | ForEach-Object {
                        "$($_.Label)=[$($_.Failure)]"
                    }) -join ', '))

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
        "ast_red=removed,comment,string,duplicate,remove,indexed,outside " +
        'arguments_contract=initializer+9_calls+handoff ' +
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
