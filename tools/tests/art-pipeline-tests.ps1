$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
& python (Join-Path $PSScriptRoot 'test_art_contracts.py')
if ($LASTEXITCODE -ne 0) { throw 'art contract tests failed' }
$expectedAssets = @(
    'AST_AsterPlanet',
    'AST_Platform',
    'CHR_Virela',
    'ENM_Anchor',
    'DEV_ImpulseRing',
    'KIT_PineBeam_A',
    'KIT_PineLintel_A',
    'KIT_PineBrace_A',
    'KIT_GlassPanel_A',
    'KIT_Brick_A'
)

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Assert-NumberArrayEqual($Actual, $Expected, [string]$Message) {
    Assert-True ($Actual.Count -eq $Expected.Count) $Message
    for ($index = 0; $index -lt $Actual.Count; $index++) {
        Assert-True ([Math]::Abs([double]$Actual[$index] - [double]$Expected[$index]) -le 1e-9) $Message
    }
}

function Read-Json([string]$Path) {
    Assert-True (Test-Path -LiteralPath $Path) "missing JSON: $Path"
    return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
}

function Invoke-Checked([string]$Script, [string[]]$Arguments) {
    & powershell -NoProfile -ExecutionPolicy Bypass -File $Script @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Script failed with exit code $LASTEXITCODE" }
}

$lock = Read-Json (Join-Path $root 'tools\toolchain.lock.json')
Assert-True ($lock.blender.version -eq '5.1.2') 'Blender must be pinned to 5.1.2'
Assert-True ($lock.blender.sha256 -match '^[0-9a-f]{64}$') 'Blender archive SHA-256 is missing'
Assert-True ($lock.blender.exe_sha256 -match '^[0-9a-f]{64}$') 'Blender executable SHA-256 is missing'

$bootstrap = Join-Path $root 'tools\bootstrap.ps1'
$bootstrapResult = & $bootstrap -ValidateLock -Json | ConvertFrom-Json
Assert-True $bootstrapResult.ok ($bootstrapResult.errors -join '; ')
Assert-True ($bootstrapResult.blender.version -eq '5.1.2') 'bootstrap does not expose Blender 5.1.2'

$requiredFiles = @(
    'art\config\vertical_slice_assets.json',
    'art\scripts\build_vertical_slice.py',
    'art\scripts\ninho_blender\__init__.py',
    'art\scripts\ninho_blender\materials.py',
    'art\scripts\ninho_blender\geometry.py',
    'art\scripts\ninho_blender\export.py',
    'tools\art\build_assets.ps1',
    'tools\art\validate_assets.ps1',
    'tools\art\vertical_slice_asset_manifest.json',
    'game\materials\pine.tres',
    'game\materials\brick.tres',
    'game\materials\glass.tres',
    'game\shaders\stylized_glass.gdshader'
)
foreach ($relative in $requiredFiles) {
    Assert-True (Test-Path -LiteralPath (Join-Path $root $relative)) "required file missing: $relative"
}

$config = Read-Json (Join-Path $root 'art\config\vertical_slice_assets.json')
Assert-True ($config.schema_version -eq 1) 'asset config schema_version must be 1'
Assert-True ($config.seed -is [long] -or $config.seed -is [int]) 'asset config seed must be an integer'
Assert-True ($config.coordinate_system.unit -eq 'meter') 'asset config unit must be meter'
Assert-True ($config.coordinate_system.up_axis -eq '+Z') 'asset config up axis must be +Z'
Assert-True ($config.coordinate_system.forward_axis -eq '-Y') 'asset config forward axis must be -Y'
Assert-True ($config.level_layout.proxies.Count -eq 22) 'canonical level layout must live in the authored config'
Assert-True ($config.level_layout.runtime_singletons.Count -eq 3) 'runtime singleton layout must be authored'
Assert-True ($config.materials.Count -ge 11) 'PBR material semantics must live in the authored config'
Assert-True ($config.global_budgets.triangles -eq 300000) 'global triangle budget must be authored'
Assert-True ($config.global_budgets.draw_calls -eq 180) 'global draw-call budget must be authored'
Assert-True ($config.global_budgets.texture_bytes -eq 134217728) 'global texture budget must be 128 MiB'
Assert-True ($config.global_budgets.particles -eq 30000) 'global particle budget must be authored'
Assert-True ($config.global_budgets.fragments -eq 120) 'global fragment budget must be authored'
$configuredIds = @($config.assets | ForEach-Object { $_.id })
Assert-True ((($configuredIds | Sort-Object) -join '|') -eq (($expectedAssets | Sort-Object) -join '|')) 'configured asset IDs differ from the vertical slice contract'
Assert-True (($configuredIds | Where-Object { $_ -match '-col($|_)' }).Count -eq 0) 'Godot -col suffix is forbidden'

$level = Read-Json (Join-Path $root 'game\data\levels\first_orbit.level.json')
$launchText = Get-Content -Raw -LiteralPath (Join-Path $root 'game\scripts\game\launch_controller.gd')
$ringConfig = @($config.assets | Where-Object { $_.id -eq 'DEV_ImpulseRing' })[0]
Assert-True ($null -ne $ringConfig.interaction) 'impulse ring interaction radii must be authored in config'
$innerMatch = [regex]::Match($launchText, 'RING_INNER_RADIUS\s*:=\s*([0-9.]+)')
$outerMatch = [regex]::Match($launchText, 'RING_OUTER_RADIUS\s*:=\s*([0-9.]+)')
Assert-True ($innerMatch.Success -and $outerMatch.Success) 'launch controller ring radii are missing'
Assert-True ([Math]::Abs([double]$innerMatch.Groups[1].Value - [double]$ringConfig.interaction.inner_radius_m) -le 1e-9) 'visual and interaction inner radius differ'
Assert-True ([Math]::Abs([double]$outerMatch.Groups[1].Value - [double]$ringConfig.interaction.outer_radius_m) -le 1e-9) 'visual and interaction outer radius differ'
$levelVisuals = @($level.bodies | ForEach-Object { $_.visual.asset_id }) + @($level.planet.visual_id) + @($level.launch_ring.id) + @('CHR_Virela')
foreach ($assetId in ($levelVisuals | Sort-Object -Unique)) {
    Assert-True ($configuredIds -contains $assetId) "level visual has no authored asset: $assetId"
}

$buildScript = Join-Path $root 'tools\art\build_assets.ps1'
$validateScript = Join-Path $root 'tools\art\validate_assets.ps1'
$tempBase = Join-Path ([IO.Path]::GetTempPath()) ('ninho-art-pipeline-' + [Guid]::NewGuid().ToString('N'))
$firstRoot = Join-Path $tempBase 'first'
$secondRoot = Join-Path $tempBase 'second'
try {
    Invoke-Checked $buildScript @('-OutputRoot', $firstRoot, '-Clean')
    Invoke-Checked $validateScript @('-OutputRoot', $firstRoot)
    Invoke-Checked $buildScript @('-OutputRoot', $secondRoot, '-Clean')
    Invoke-Checked $validateScript @('-OutputRoot', $secondRoot)

    $firstManifest = Read-Json (Join-Path $firstRoot 'tools\art\vertical_slice_asset_manifest.json')
    $secondManifest = Read-Json (Join-Path $secondRoot 'tools\art\vertical_slice_asset_manifest.json')
    Assert-True ($firstManifest.generator.blender_version -eq '5.1.2') 'manifest Blender version mismatch'
    Assert-True ($firstManifest.generator.blender_executable_sha256 -eq $lock.blender.exe_sha256) 'manifest Blender hash mismatch'
    Assert-True ($firstManifest.generator.seed -eq $config.seed) 'manifest seed mismatch'
    Assert-True ($firstManifest.build_hash -eq $secondManifest.build_hash) 'two clean builds have different build hashes'

    $firstOutputs = @{}
    foreach ($output in $firstManifest.outputs) { $firstOutputs[$output.path] = $output.sha256 }
    $secondOutputs = @{}
    foreach ($output in $secondManifest.outputs) { $secondOutputs[$output.path] = $output.sha256 }
    Assert-True ($firstOutputs.Count -eq $secondOutputs.Count) 'clean builds produced different output counts'
    foreach ($path in $firstOutputs.Keys) {
        Assert-True ($secondOutputs.ContainsKey($path)) "second build omitted $path"
        Assert-True ($firstOutputs[$path] -eq $secondOutputs[$path]) "non-deterministic output: $path"
    }

    Assert-True ($firstManifest.assets.Count -eq $expectedAssets.Count) 'manifest asset count mismatch'
    foreach ($asset in $firstManifest.assets) {
        Assert-True ($expectedAssets -contains $asset.id) "unexpected manifest asset: $($asset.id)"
        Assert-True ($asset.id -match '^(AST|CHR|ENM|DEV|KIT)_[A-Za-z0-9_]+$') "invalid asset ID: $($asset.id)"
        Assert-True ($asset.pivot -eq 'CENTER_OF_MASS') "$($asset.id) pivot is not COM"
        Assert-True ($asset.scale_applied -eq $true) "$($asset.id) scale is not applied"
        Assert-True ($asset.determinant_positive -eq $true) "$($asset.id) has a mirrored transform"
        Assert-True ($asset.lods.Count -ge 2) "$($asset.id) must have at least two LODs"
        Assert-True ($asset.triangles -le $asset.triangle_budget) "$($asset.id) exceeds its triangle budget"
        Assert-True ($asset.materials.Count -le $asset.material_budget) "$($asset.id) exceeds its material budget"
        Assert-True ($asset.author -eq $config.author) "$($asset.id) lacks author provenance"
        Assert-True ($asset.license -eq $config.license) "$($asset.id) lacks license provenance"
        Assert-True ($asset.seed -eq $config.seed) "$($asset.id) lacks seed provenance"
        Assert-True ($asset.source_config_sha256 -eq $firstManifest.generator.config_sha256) "$($asset.id) lacks source hash provenance"
        Assert-True ($asset.texture_images.Count -ge 1) "$($asset.id) GLB lacks embedded procedural texture"
        Assert-True ($asset.authored_textures.Count -ge $asset.texture_images.Count) "$($asset.id) lacks normative authored PNG outputs"
        foreach ($image in $asset.texture_images) {
            Assert-True ($image.unique_colors -gt 1 -and $image.has_nonblack_rgb) "$($asset.id) embeds a constant or black texture"
            $sourceImage = @($asset.authored_textures | Where-Object { $_.image_name -eq $image.name })
            Assert-True ($sourceImage.Count -eq 1) "$($asset.id) embedded image lacks a source PNG"
            Assert-True ($sourceImage[0].sha256 -eq $image.sha256 -and $sourceImage[0].pixel_sha256 -eq $image.pixel_sha256) "$($asset.id) embedded pixels differ from source PNG"
            Assert-True (@($firstManifest.outputs | Where-Object { $_.path -eq $sourceImage[0].path -and $_.sha256 -eq $sourceImage[0].sha256 }).Count -eq 1) "$($asset.id) source PNG is not a normative output"
        }
        Assert-True ($asset.uv_layers -ge 1) "$($asset.id) GLB lacks UV coordinates"
        Assert-True ($asset.blend_file_sha256 -match '^[0-9a-f]{64}$') "$($asset.id) lacks informational blend file hash"
        Assert-True ($asset.blend_semantic_sha256 -match '^[0-9a-f]{64}$') "$($asset.id) lacks normative blend semantic hash"
        $otherAsset = @($secondManifest.assets | Where-Object { $_.id -eq $asset.id })
        Assert-True ($otherAsset.Count -eq 1) "second build omitted asset $($asset.id)"
        Assert-True ($asset.blend_semantic_sha256 -eq $otherAsset[0].blend_semantic_sha256) "semantic blend drift: $($asset.id)"
        foreach ($object in $asset.objects) {
            Assert-True ($object.name -match '^(VIS|COL|FRAG|SOCKET|RIG)_[A-Za-z0-9_]+$') "invalid object name: $($object.name)"
            Assert-True ($object.name -notmatch '-col($|_)') "forbidden -col suffix: $($object.name)"
            if ($object.role -eq 'COL') {
                Assert-True ($object.convex -eq $true) "collision hull is not convex: $($object.name)"
            }
        }
    }

    foreach ($body in $level.bodies) {
        $proxy = @($firstManifest.proxies | Where-Object { $_.body_id -eq $body.body_id })
        Assert-True ($proxy.Count -eq 1) "proxy count mismatch for body $($body.body_id)"
        Assert-True ($proxy[0].asset_id -eq $body.visual.asset_id) "proxy asset mismatch for body $($body.body_id)"
        Assert-NumberArrayEqual $proxy[0].bounds_m $body.visual.bounds_m "proxy bounds mismatch for body $($body.body_id)"
        Assert-NumberArrayEqual $proxy[0].transform.position_m $body.transform.position_m "proxy position mismatch for body $($body.body_id)"
        Assert-NumberArrayEqual $proxy[0].transform.rotation_xyzw $body.transform.rotation_xyzw "proxy rotation mismatch for body $($body.body_id)"
    }
    Assert-True ($firstManifest.global_budgets.triangles.actual -le 300000) 'global triangles exceed budget'
    Assert-True ($firstManifest.global_budgets.draw_calls.actual -le 180) 'global draw calls exceed budget'
    Assert-True ($firstManifest.global_budgets.texture_bytes.actual -gt 0) 'embedded texture bytes were not measured'
    Assert-True ($firstManifest.global_budgets.texture_bytes.actual -le 134217728) 'global textures exceed budget'
    Assert-True ($firstManifest.global_budgets.particles.actual -le 30000) 'global particles exceed budget'
    Assert-True ($firstManifest.global_budgets.fragments.actual -le 120) 'global fragments exceed budget'
    Assert-True ($firstManifest.material_semantics_sha256 -match '^[0-9a-f]{64}$') 'cross-runtime material semantics hash is missing'
    Assert-True (@($firstManifest.non_normative_derivatives | Where-Object { $_.glob -match '\.import' }).Count -eq 1) 'Godot .import derivatives must be explicitly non-normative'
    $distinctMaterialPixels = @($firstManifest.assets.authored_textures | ForEach-Object { $_ } | Group-Object material | ForEach-Object { $_.Group[0].pixel_sha256 } | Sort-Object -Unique)
    Assert-True ($distinctMaterialPixels.Count -ge 8) 'different procedural material specs produced identical pixels'
    Assert-True ($firstManifest.global_budgets.triangles.source -match 'multiplied by canonical runtime instances') 'triangle budget is not instance-aware'
    Assert-True ($firstManifest.global_budgets.draw_calls.source -match 'multiplied by canonical runtime instances') 'draw-call budget is not instance-aware'
    Assert-True ($firstManifest.global_budgets.fragments.source -match 'multiplied by canonical runtime instances') 'fragment budget is not instance-aware'

    $contentRoot = Join-Path $tempBase 'content-drift'
    New-Item -ItemType Directory -Force -Path (Join-Path $contentRoot 'art\config'), (Join-Path $contentRoot 'game\data\levels') | Out-Null
    Copy-Item -LiteralPath (Join-Path $root 'art\config\vertical_slice_assets.json') -Destination (Join-Path $contentRoot 'art\config\vertical_slice_assets.json')
    $tamperedLevel = Read-Json (Join-Path $root 'game\data\levels\first_orbit.level.json')
    $tamperedLevel.bodies[0].transform.position_m[0] = [double]$tamperedLevel.bodies[0].transform.position_m[0] + 0.125
    $tamperedLevelJson = $tamperedLevel | ConvertTo-Json -Depth 100 -Compress
    [IO.File]::WriteAllText((Join-Path $contentRoot 'game\data\levels\first_orbit.level.json'), $tamperedLevelJson, [Text.UTF8Encoding]::new($false))
    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $layoutOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $validateScript -OutputRoot $firstRoot -ContentRoot $contentRoot 2>&1
    $layoutExitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousPreference
    Assert-True ($layoutExitCode -ne 0) 'validator accepted a level transform that differs from authored config'
    Assert-True (($layoutOutput -join "`n") -match 'level layout differs from authored config') 'validator did not diagnose level transform drift'

    Copy-Item -LiteralPath (Join-Path $root 'game\data\levels\first_orbit.level.json') -Destination (Join-Path $contentRoot 'game\data\levels\first_orbit.level.json') -Force
    New-Item -ItemType Directory -Force -Path (Join-Path $contentRoot 'game\scenes') | Out-Null
    $particleScene = (Get-Content -Raw -LiteralPath (Join-Path $root 'game\scenes\vertical_slice.tscn')) -replace '(?m)^amount = 24$', 'amount = 30001'
    [IO.File]::WriteAllText((Join-Path $contentRoot 'game\scenes\vertical_slice.tscn'), $particleScene, [Text.UTF8Encoding]::new($false))
    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $budgetOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $validateScript -OutputRoot $firstRoot -ContentRoot $contentRoot 2>&1
    $budgetExitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousPreference
    Assert-True ($budgetExitCode -ne 0) 'validator accepted a measured global particle budget excess'
    Assert-True (($budgetOutput -join "`n") -match 'particles global budget exceeded') 'validator did not diagnose global budget excess'

    $texturePath = Join-Path $firstRoot $firstManifest.assets[0].authored_textures[0].path
    $manifestPath = Join-Path $firstRoot 'tools\art\vertical_slice_asset_manifest.json'
    $originalTexture = [IO.File]::ReadAllBytes($texturePath)
    $originalManifest = [IO.File]::ReadAllText($manifestPath)
    $tamperedTexture = [byte[]]$originalTexture.Clone()
    $tamperedTexture[$tamperedTexture.Length - 16] = $tamperedTexture[$tamperedTexture.Length - 16] -bxor 1
    [IO.File]::WriteAllBytes($texturePath, $tamperedTexture)
    $tamperedManifest = Read-Json $manifestPath
    $tamperedSha = (Get-FileHash -Algorithm SHA256 -LiteralPath $texturePath).Hash.ToLowerInvariant()
    ($tamperedManifest.outputs | Where-Object { $_.path -eq $firstManifest.assets[0].authored_textures[0].path }).sha256 = $tamperedSha
    [IO.File]::WriteAllText($manifestPath, ($tamperedManifest | ConvertTo-Json -Depth 100 -Compress), [Text.UTF8Encoding]::new($false))
    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $textureOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $validateScript -OutputRoot $firstRoot 2>&1
    $textureExitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousPreference
    Assert-True ($textureExitCode -ne 0) 'validator accepted source PNG pixel tamper'
    Assert-True (($textureOutput -join "`n") -match 'authored PNG differs from procedural source') 'validator did not diagnose source PNG pixel tamper'
    [IO.File]::WriteAllBytes($texturePath, $originalTexture)
    [IO.File]::WriteAllText($manifestPath, $originalManifest, [Text.UTF8Encoding]::new($false))

    $pinePath = Join-Path $firstRoot 'game\materials\pine.tres'
    $originalPine = [IO.File]::ReadAllText($pinePath)
    [IO.File]::WriteAllText($pinePath, ($originalPine -replace '(?m)^roughness = 0\.76$', 'roughness = 0.75'), [Text.UTF8Encoding]::new($false))
    $materialManifest = Read-Json $manifestPath
    ($materialManifest.outputs | Where-Object { $_.path -eq 'game/materials/pine.tres' }).sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $pinePath).Hash.ToLowerInvariant()
    [IO.File]::WriteAllText($manifestPath, ($materialManifest | ConvertTo-Json -Depth 100 -Compress), [Text.UTF8Encoding]::new($false))
    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $materialOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $validateScript -OutputRoot $firstRoot 2>&1
    $materialExitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousPreference
    Assert-True ($materialExitCode -ne 0) 'validator accepted effective Godot material drift'
    Assert-True (($materialOutput -join "`n") -match 'effective Godot material semantics differ') 'validator did not diagnose effective Godot material drift'
    [IO.File]::WriteAllText($pinePath, $originalPine, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText($manifestPath, $originalManifest, [Text.UTF8Encoding]::new($false))

    $driftSource = Join-Path $firstRoot 'art\source\vertical_slice\KIT_Brick_A.blend'
    $driftTarget = Join-Path $firstRoot 'art\source\vertical_slice\KIT_PineBeam_A.blend'
    Copy-Item -LiteralPath $driftSource -Destination $driftTarget -Force
    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $driftOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $validateScript -OutputRoot $firstRoot 2>&1
    $driftExitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousPreference
    Assert-True ($driftExitCode -ne 0) 'validator accepted semantic drift in a regenerated blend source'
    Assert-True (($driftOutput -join "`n") -match 'blend semantic hash mismatch') 'validator did not diagnose blend semantic drift'

    $boundsManifestPath = Join-Path $secondRoot 'tools\art\vertical_slice_asset_manifest.json'
    $boundsManifest = Read-Json $boundsManifestPath
    $boundsManifest.assets[0].bounds_m[0] = [double]$boundsManifest.assets[0].bounds_m[0] + 1.0
    $boundsJson = $boundsManifest | ConvertTo-Json -Depth 100 -Compress
    [IO.File]::WriteAllText($boundsManifestPath, $boundsJson, [Text.UTF8Encoding]::new($false))
    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $boundsOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $validateScript -OutputRoot $secondRoot 2>&1
    $boundsExitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousPreference
    Assert-True ($boundsExitCode -ne 0) 'validator accepted exported bounds drift'
    Assert-True (($boundsOutput -join "`n") -match 'exported bounds mismatch') 'validator did not diagnose exported bounds drift'
}
finally {
    if (Test-Path -LiteralPath $tempBase) { Remove-Item -LiteralPath $tempBase -Recurse -Force }
}

$sceneText = Get-Content -Raw -LiteralPath (Join-Path $root 'game\scenes\vertical_slice.tscn')
$registryText = Get-Content -Raw -LiteralPath (Join-Path $root 'game\scripts\game\body_view_registry.gd')
$glassShaderText = Get-Content -Raw -LiteralPath (Join-Path $root 'game\shaders\stylized_glass.gdshader')
Assert-True ($sceneText -notmatch 'Placeholder') 'Task 9 placeholders remain in vertical_slice.tscn'
Assert-True (($sceneText + $registryText) -match 'res://assets/vertical_slice/aster/AST_AsterPlanet.glb') 'Aster GLB is not instanced by a view'
Assert-True ($sceneText -match 'res://assets/vertical_slice/devices/DEV_ImpulseRing.glb') 'impulse ring GLB is not instanced by the scene'
Assert-True ($registryText -match 'res://assets/vertical_slice/') 'body view registry does not load authored GLBs'
Assert-True ($registryText -match '"CHR_LaunchBird"\s*:\s*preload\("res://assets/vertical_slice/characters/CHR_Virela.glb"\)') 'runtime Virela alias is not mapped to the authored GLB'
Assert-True (($sceneText + $registryText) -notmatch '(CollisionShape3D|StaticBody3D|RigidBody3D|CharacterBody3D)') 'Godot physics nodes are forbidden'
Assert-True ($glassShaderText -match 'depth_prepass_alpha') 'glass shader must use the Godot 4.5 depth prepass render mode'
Assert-True ($glassShaderText -notmatch 'depth_draw_alpha_prepass') 'obsolete glass shader render mode is forbidden'
Assert-True ($glassShaderText -match 'texture\(albedo_texture, UV\)') 'glass override must sample its authored texture'
Assert-True ($glassShaderText -match '(?m)^\s*ALBEDO = texel\.rgb;$') 'glass shader must not reapply authored base color'
Assert-True ($glassShaderText -match '(?m)^\s*ALPHA = texel\.a;$') 'glass shader must not reapply authored alpha'
Assert-True ($glassShaderText -notmatch 'glass_tint|authored_alpha|texel\.rgb\s*\*') 'glass shader reapplies baked texture semantics'
foreach ($uniform in 'authored_roughness','authored_metallic','transmission_weight','coat_weight') {
    Assert-True ($glassShaderText -match "uniform float $uniform") "glass shader lacks effective $uniform semantics"
}

$forbidden = @(Get-ChildItem -Path (Join-Path $root 'game\assets\vertical_slice') -Recurse -File -ErrorAction SilentlyContinue | Where-Object { $_.Name -match '-col($|[._-])' })
Assert-True ($forbidden.Count -eq 0) 'Godot -col files are forbidden'
Write-Output 'art pipeline: PASS'
