[CmdletBinding()]
param(
    [string]$CaptureScriptPath,
    [string]$ProjectPath
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $CaptureScriptPath) {
    $CaptureScriptPath = Join-Path $root 'tools\capture_vertical_slice.ps1'
}
if (-not $ProjectPath) {
    $ProjectPath = Join-Path $root 'game\project.godot'
}
$CaptureScriptPath = (Resolve-Path -LiteralPath $CaptureScriptPath).Path
$ProjectPath = (Resolve-Path -LiteralPath $ProjectPath).Path

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Get-ExplicitSceneArgument {
    param([Parameter(Mandatory)][string]$CaptureText)

    $scenePattern = '\$arguments\.Add\(''(res://[^'']+\.tscn)''\)'
    $matches = @([regex]::Matches($CaptureText, $scenePattern))
    Assert-True ($matches.Count -eq 1) `
        'Legacy capture must pass exactly one explicit res:// scene argument'
    return $matches[0].Groups[1].Value
}

function Get-ConfiguredMainScene {
    param([Parameter(Mandatory)][string]$ProjectText)

    $matches = @([regex]::Matches(
        $ProjectText, '(?m)^run/main_scene="([^"]+)"\r?$'))
    Assert-True ($matches.Count -eq 1) 'Project fixture must define exactly one run/main_scene'
    return $matches[0].Groups[1].Value
}

$capture = [IO.File]::ReadAllText($CaptureScriptPath)
$expectedScene = 'res://scenes/vertical_slice.tscn'
$explicitScene = Get-ExplicitSceneArgument -CaptureText $capture
Assert-True ($explicitScene -ceq $expectedScene) `
    "Legacy capture scene changed: $explicitScene"

$pathIndex = $capture.IndexOf("'--path','game'", [StringComparison]::Ordinal)
$sceneIndex = $capture.IndexOf(
    "`$arguments.Add('$expectedScene')", [StringComparison]::Ordinal)
$userArgumentsIndex = $capture.IndexOf(
    "`$arguments.Add('--')", [StringComparison]::Ordinal)
Assert-True ($pathIndex -ge 0 -and $sceneIndex -gt $pathIndex `
        -and $userArgumentsIndex -gt $sceneIndex) `
    'Explicit scene must remain a Godot argument after --path and before user arguments'

$temporaryParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$temporaryRoot = Join-Path $temporaryParent `
    ('ninho-legacy-scene-contract-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
try {
    $temporaryCapture = Join-Path $temporaryRoot 'capture_vertical_slice.ps1'
    $explicitSceneCall = "`$arguments.Add('$expectedScene')"
    $sceneCallCount = [regex]::Matches(
        $capture, [regex]::Escape($explicitSceneCall)).Count
    Assert-True ($sceneCallCount -eq 1) `
        'Real capture must contain exactly one explicit legacy scene call'
    $captureWithoutScene = $capture.Replace($explicitSceneCall, '')
    Assert-True ($captureWithoutScene -cne $capture) `
        'Temporary capture fixture did not remove the explicit scene call'
    [IO.File]::WriteAllText(
        $temporaryCapture, $captureWithoutScene, [Text.UTF8Encoding]::new($false))
    $causalRed = ''
    try {
        $null = Get-ExplicitSceneArgument -CaptureText `
            ([IO.File]::ReadAllText($temporaryCapture))
    } catch {
        $causalRed = $_.Exception.Message
    }
    Assert-True ($causalRed -ceq `
            'Legacy capture must pass exactly one explicit res:// scene argument') `
        "Removing only the temporary scene call did not cause the expected RED: $causalRed"

    $temporaryProject = Join-Path $temporaryRoot 'project.godot'
    $project = [IO.File]::ReadAllText($ProjectPath)
    $originalMainScene = Get-ConfiguredMainScene -ProjectText $project
    $mutatedMainScene = 'res://scenes/__legacy_contract_wrong_main__.tscn'
    $mutatedProject = [regex]::Replace(
        $project,
        '(?m)^run/main_scene="[^"]+"\r?$',
        "run/main_scene=`"$mutatedMainScene`"")
    Assert-True ($mutatedProject -cne $project) `
        'Temporary project fixture did not change run/main_scene'
    [IO.File]::WriteAllText(
        $temporaryProject, $mutatedProject, [Text.UTF8Encoding]::new($false))

    $configuredMainScene = Get-ConfiguredMainScene -ProjectText `
        ([IO.File]::ReadAllText($temporaryProject))
    Assert-True ($configuredMainScene -ceq $mutatedMainScene) `
        'Temporary project fixture did not preserve the changed main scene'
    Assert-True ((Get-ExplicitSceneArgument -CaptureText $capture) -ceq $expectedScene) `
        'Legacy capture followed the temporary run/main_scene instead of its explicit scene'
    Assert-True ($configuredMainScene -cne $explicitScene) `
        'Temporary main scene must differ from the explicit legacy capture scene'

    Write-Output (
        "vertical slice explicit scene contract: PASS " +
        "capture=$explicitScene original_main=$originalMainScene " +
        "mutated_main=$configuredMainScene causal_red=missing_explicit_scene")
} finally {
    $resolvedTemporaryRoot = [IO.Path]::GetFullPath($temporaryRoot)
    Assert-True ($resolvedTemporaryRoot.StartsWith(
            $temporaryParent, [StringComparison]::OrdinalIgnoreCase)) `
        'Refusing to remove temporary fixture outside the system temporary root'
    Assert-True ([IO.Path]::GetFileName($resolvedTemporaryRoot).StartsWith(
            'ninho-legacy-scene-contract-', [StringComparison]::Ordinal)) `
        'Refusing to remove an unexpected temporary fixture'
    if (Test-Path -LiteralPath $resolvedTemporaryRoot) {
        Remove-Item -LiteralPath $resolvedTemporaryRoot -Recurse -Force
    }
}
