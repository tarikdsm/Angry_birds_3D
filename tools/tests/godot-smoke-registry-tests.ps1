[CmdletBinding()]
param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
)

$ErrorActionPreference = 'Stop'
$Root = [IO.Path]::GetFullPath($Root)
Import-Module (Join-Path $Root 'tools\GodotSmokeRegistry.psm1') -Force

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Throws {
    param([scriptblock]$Action, [string]$ExpectedMessage)
    try {
        & $Action
    } catch {
        if ($_.Exception.Message -cnotlike "*$ExpectedMessage*") {
            throw "Expected '$ExpectedMessage', got '$($_.Exception.Message)'"
        }
        return
    }
    throw "Expected failure containing '$ExpectedMessage'"
}

$registry = @(Get-NinhoGodotSmokeRegistry)
$expectedNames = @(
    'import-completeness',
    'orbital-session-smoke',
    'gameplay-session-smoke',
    'vertical-slice-smoke',
    'feedback-smoke',
    'feedback-config-validation',
    'forbid-godot-physics',
    'input-router-smoke',
    'app-shell-smoke',
    'save-recovery-smoke',
    'settings-model-smoke',
    'input-remapping-smoke',
    'fan-project-notice-smoke',
    'about-screen-smoke',
    'world-carousel-smoke',
    'product-v2-catalog-smoke',
    'product-v2-content-smoke',
    'earth-session-smoke',
    'two-world-scene-smoke',
    'camera-profiles-smoke',
    'slingshot-input-smoke'
)
Assert-True ($registry.Count -eq $expectedNames.Count) `
    'Registry must contain every shipped game/tests entrypoint'
for ($index = 0; $index -lt $expectedNames.Count; ++$index) {
    Assert-True ($registry[$index].Name -ceq $expectedNames[$index]) `
        "Unexpected smoke order at index $index"
}
Assert-True ($registry[0].RequiredCompletionMarker -ceq 'NINHO_IMPORT_COMPLETENESS_OK') `
    'Import completeness must preserve its completion marker'
Assert-True ($registry[2].FixedFps -eq 60 -and $registry[3].FixedFps -eq 60 -and `
        $registry[4].FixedFps -eq 60) `
    'Gameplay smokes must preserve fixed 60 fps execution'
Assert-True ($registry[5].RequiredLogText -ceq 'feedback config validation: PASS') `
    'Feedback validation must preserve its required log text'
$expectedMarkers = @{
    'import-completeness' = 'NINHO_IMPORT_COMPLETENESS_OK'
    'orbital-session-smoke' = 'ORBITAL_SESSION_NODE_SMOKE_OK'
    'gameplay-session-smoke' = 'GAMEPLAY_SESSION_NODE_SMOKE_OK'
    'vertical-slice-smoke' = 'VERTICAL_SLICE_SMOKE_OK'
    'feedback-smoke' = 'FEEDBACK_SMOKE_OK'
    'forbid-godot-physics' = 'FORBID_GODOT_PHYSICS_OK'
    'input-router-smoke' = 'INPUT_ROUTER_SMOKE_OK'
    'app-shell-smoke' = 'APP_SHELL_SMOKE_OK'
    'save-recovery-smoke' = 'SAVE_RECOVERY_SMOKE_OK'
    'settings-model-smoke' = 'SETTINGS_MODEL_SMOKE_OK'
    'input-remapping-smoke' = 'INPUT_REMAPPING_SMOKE_OK'
    'fan-project-notice-smoke' = 'FAN_PROJECT_NOTICE_SMOKE_OK'
    'about-screen-smoke' = 'ABOUT_SCREEN_SMOKE_OK'
    'world-carousel-smoke' = 'WORLD_CAROUSEL_SMOKE_OK'
    'product-v2-catalog-smoke' = 'PRODUCT_V2_CATALOG_SMOKE_OK'
    'product-v2-content-smoke' = 'PRODUCT_V2_CONTENT_SMOKE_OK'
    'earth-session-smoke' = 'EARTH_SESSION_SMOKE_OK'
    'two-world-scene-smoke' = 'TWO_WORLD_SCENE_SMOKE_OK'
    'camera-profiles-smoke' = 'CAMERA_PROFILES_SMOKE_OK'
    'slingshot-input-smoke' = 'SLINGSHOT_INPUT_SMOKE_OK'
}
foreach ($spec in $registry) {
    if ($expectedMarkers.ContainsKey([string]$spec.Name)) {
        Assert-True (
            $spec.RequiredCompletionMarker -ceq $expectedMarkers[[string]$spec.Name]) `
            "Smoke completion marker drifted: $($spec.Name)"
    }
}

Assert-NinhoGodotSmokeRegistry -Root $Root -Registry $registry

$runner = [IO.File]::ReadAllText((Join-Path $Root 'tools\test.ps1'))
Assert-True ($runner.Contains("GodotSmokeRegistry.psm1")) `
    'Official gate must import the Godot smoke registry'
Assert-True ($runner.Contains("tests\godot-smoke-registry-tests.ps1")) `
    'Official gate must execute the registry contract tests'
Assert-True ($runner.Contains('Assert-NinhoGodotSmokeRegistry')) `
    'Official gate must reject registration drift before launching Godot'
Assert-True ($runner.Contains('Invoke-NinhoRegisteredGodotSmoke')) `
    'Official gate must execute game/tests entrypoints through the registry'
foreach ($spec in $registry) {
    Assert-True (-not $runner.Contains([string]$spec.ResourcePath)) `
        "Official gate duplicates registry resource path: $($spec.ResourcePath)"
}

$fixtureRoot = Join-Path ([IO.Path]::GetTempPath()) `
    ('ninho-godot-smoke-registry-' + [Guid]::NewGuid().ToString('N'))
$testsDirectory = Join-Path $fixtureRoot 'game\tests'
New-Item -ItemType Directory -Force -Path $testsDirectory | Out-Null
try {
    foreach ($spec in $registry) {
        $path = Join-Path $fixtureRoot $spec.RelativePath
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $path) | Out-Null
        [IO.File]::WriteAllText(
            $path,
            "extends SceneTree`n`nfunc _initialize() -> void:`n`tquit(0)`n",
            [Text.UTF8Encoding]::new($false))
    }
    [IO.File]::WriteAllText(
        (Join-Path $testsDirectory 'fixture_helper.gd'),
        "extends RefCounted`n`nstatic func make_value() -> int:`n`treturn 1`n",
        [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText(
        (Join-Path $testsDirectory 'scene_helper.gd'),
        "extends Node`n`nfunc attach() -> void:`n`tpass`n",
        [Text.UTF8Encoding]::new($false))
    Assert-NinhoGodotSmokeRegistry -Root $fixtureRoot -Registry $registry

    $forgotten = Join-Path $testsDirectory 'forgotten_smoke.gd'
    [IO.File]::WriteAllText(
        $forgotten,
        "extends SceneTree`n`nfunc _initialize() -> void:`n`tquit(0)`n",
        [Text.UTF8Encoding]::new($false))
    Assert-Throws {
        Assert-NinhoGodotSmokeRegistry -Root $fixtureRoot -Registry $registry
    } 'unregistered Godot smoke entrypoint: game/tests/forgotten_smoke.gd'
    Remove-Item -LiteralPath $forgotten -Force

    $registeredPath = Join-Path $fixtureRoot $registry[0].RelativePath
    [IO.File]::WriteAllText(
        $registeredPath,
        "extends RefCounted`n",
        [Text.UTF8Encoding]::new($false))
    Assert-Throws {
        Assert-NinhoGodotSmokeRegistry -Root $fixtureRoot -Registry $registry
    } "registered Godot smoke is not an executable SceneTree script: $($registry[0].RelativePath)"
} finally {
    if (Test-Path -LiteralPath $fixtureRoot) {
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}

Write-Output 'godot smoke registry tests: PASS'
