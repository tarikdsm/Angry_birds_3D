Set-StrictMode -Version Latest

function Get-NinhoGodotSmokeRegistry {
    [CmdletBinding()]
    param()

    return @(
        [pscustomobject]@{
            Name = 'import-completeness'
            RelativePath = 'game/tests/import_completeness_smoke.gd'
            ResourcePath = 'res://tests/import_completeness_smoke.gd'
            FixedFps = 0
            RequiredCompletionMarker = 'NINHO_IMPORT_COMPLETENESS_OK'
            RequiredLogText = ''
        },
        [pscustomobject]@{
            Name = 'orbital-session-smoke'
            RelativePath = 'game/tests/orbital_session_node_smoke.gd'
            ResourcePath = 'res://tests/orbital_session_node_smoke.gd'
            FixedFps = 0
            RequiredCompletionMarker = 'ORBITAL_SESSION_NODE_SMOKE_OK'
            RequiredLogText = ''
        },
        [pscustomobject]@{
            Name = 'gameplay-session-smoke'
            RelativePath = 'game/tests/gameplay_session_node_smoke.gd'
            ResourcePath = 'res://tests/gameplay_session_node_smoke.gd'
            FixedFps = 60
            RequiredCompletionMarker = 'GAMEPLAY_SESSION_NODE_SMOKE_OK'
            RequiredLogText = ''
        },
        [pscustomobject]@{
            Name = 'vertical-slice-smoke'
            RelativePath = 'game/tests/vertical_slice_smoke.gd'
            ResourcePath = 'res://tests/vertical_slice_smoke.gd'
            FixedFps = 60
            RequiredCompletionMarker = 'VERTICAL_SLICE_SMOKE_OK'
            RequiredLogText = ''
        },
        [pscustomobject]@{
            Name = 'feedback-smoke'
            RelativePath = 'game/tests/feedback_smoke.gd'
            ResourcePath = 'res://tests/feedback_smoke.gd'
            FixedFps = 60
            RequiredCompletionMarker = 'FEEDBACK_SMOKE_OK'
            RequiredLogText = ''
        },
        [pscustomobject]@{
            Name = 'feedback-config-validation'
            RelativePath = 'game/tests/feedback_config_validation.gd'
            ResourcePath = 'res://tests/feedback_config_validation.gd'
            FixedFps = 0
            RequiredCompletionMarker = ''
            RequiredLogText = 'feedback config validation: PASS'
        },
        [pscustomobject]@{
            Name = 'forbid-godot-physics'
            RelativePath = 'game/tests/forbid_godot_physics.gd'
            ResourcePath = 'res://tests/forbid_godot_physics.gd'
            FixedFps = 0
            RequiredCompletionMarker = 'FORBID_GODOT_PHYSICS_OK'
            RequiredLogText = ''
        }
    )
}

function Test-NinhoGodotSmokeEntrypoint {
    param([Parameter(Mandatory)][string]$Path)

    $text = [IO.File]::ReadAllText($Path)
    return $text -match '(?m)^\s*extends\s+SceneTree\s*(?:#.*)?$' -and
        $text -match '(?m)^\s*func\s+_initialize\s*\('
}

function Assert-NinhoGodotSmokeRegistry {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][object[]]$Registry
    )

    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $testsDirectory = Join-Path $rootPath 'game\tests'
    if (-not (Test-Path -LiteralPath $testsDirectory -PathType Container)) {
        throw "Godot tests directory missing: $testsDirectory"
    }

    $registeredPaths = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::Ordinal)
    $registeredNames = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::Ordinal)
    foreach ($spec in @($Registry)) {
        $relative = ([string]$spec.RelativePath).Replace('\', '/')
        if ($relative -notmatch '^game/tests/[a-z0-9_/-]+\.gd$' -or
                $relative.Contains('/../') -or $relative.Contains('/./')) {
            throw "Godot smoke path is not canonical: $relative"
        }
        if (-not $registeredPaths.Add($relative)) {
            throw "duplicate Godot smoke path: $relative"
        }
        $name = [string]$spec.Name
        if ($name -notmatch '^[a-z0-9]+(?:-[a-z0-9]+)*$' -or
                -not $registeredNames.Add($name)) {
            throw "duplicate or invalid Godot smoke name: $name"
        }
        $expectedResource = 'res://' + $relative.Substring('game/'.Length)
        if ([string]$spec.ResourcePath -cne $expectedResource) {
            throw "Godot smoke resource path mismatch: $relative"
        }
        if ([int]$spec.FixedFps -lt 0) {
            throw "Godot smoke fixed fps is invalid: $name"
        }
        $path = Join-Path $rootPath $relative
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "registered Godot smoke missing: $relative"
        }
        if (-not (Test-NinhoGodotSmokeEntrypoint -Path $path)) {
            throw "registered Godot smoke is not an executable SceneTree script: $relative"
        }
    }

    $entrypoints = [Collections.Generic.List[string]]::new()
    foreach ($file in @(Get-ChildItem -LiteralPath $testsDirectory -Recurse -File -Filter '*.gd')) {
        if (-not (Test-NinhoGodotSmokeEntrypoint -Path $file.FullName)) {
            continue
        }
        $relative = $file.FullName.Substring($rootPath.Length + 1).Replace('\', '/')
        $entrypoints.Add($relative)
    }
    $entrypoints.Sort([StringComparer]::Ordinal)
    foreach ($relative in $entrypoints) {
        if (-not $registeredPaths.Contains($relative)) {
            throw "unregistered Godot smoke entrypoint: $relative"
        }
    }
    if ($entrypoints.Count -ne $registeredPaths.Count) {
        throw 'Godot smoke registry contains a non-entrypoint script'
    }
}

Export-ModuleMember -Function Get-NinhoGodotSmokeRegistry,Assert-NinhoGodotSmokeRegistry
