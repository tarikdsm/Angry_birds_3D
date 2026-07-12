$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$expectedIds = @('launch', 'vortex', 'pine', 'glass', 'brick', 'helmet', 'vulnerable', 'victory', 'defeat')

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Read-Json([string]$Path) {
    Assert-True (Test-Path -LiteralPath $Path) "missing JSON: $Path"
    return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
}

function Invoke-Checked([string]$Script, [string[]]$Arguments) {
    & powershell -NoProfile -ExecutionPolicy Bypass -File $Script @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Script failed with exit code $LASTEXITCODE" }
}

function Invoke-ExpectFailure([scriptblock]$Action, [string]$MessagePattern) {
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $output = & $Action 2>&1 | Out-String
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousPreference
    }
    Assert-True ($exitCode -ne 0) "command unexpectedly succeeded: $output"
    Assert-True ($output -match $MessagePattern) "failure did not mention '$MessagePattern': $output"
}

& python (Join-Path $PSScriptRoot 'test_audio_contracts.py')
if ($LASTEXITCODE -ne 0) { throw 'audio contract tests failed' }

$requiredFiles = @(
    'tools\audio\vertical_slice_audio.json',
    'tools\audio\generate_audio.py',
    'tools\audio\build_audio.ps1',
    'tools\audio\validate_audio.ps1'
)
foreach ($relative in $requiredFiles) {
    Assert-True (Test-Path -LiteralPath (Join-Path $root $relative)) "required file missing: $relative"
}

$config = Read-Json (Join-Path $root 'tools\audio\vertical_slice_audio.json')
Assert-True ($config.generation_mode -eq 'procedural-offline') 'audio must be generated offline'
Assert-True ($config.external_sources.Count -eq 0) 'external audio sources are forbidden'
$configuredIds = @($config.sounds | ForEach-Object { $_.id })
Assert-True ((($configuredIds | Sort-Object) -join '|') -eq (($expectedIds | Sort-Object) -join '|')) 'audio coverage differs from contract'

$generatorText = Get-Content -Raw -LiteralPath (Join-Path $root 'tools\audio\generate_audio.py')
Assert-True ($generatorText -notmatch '(?i)\b(requests|urllib|httpx|socket)\b|https?://') 'audio generator may not use network sources'

$buildScript = Join-Path $root 'tools\audio\build_audio.ps1'
$validateScript = Join-Path $root 'tools\audio\validate_audio.ps1'
Invoke-Checked $buildScript @('-Clean')
Invoke-Checked $validateScript @()
$tempBase = Join-Path ([IO.Path]::GetTempPath()) ('ninho-audio-pipeline-' + [Guid]::NewGuid().ToString('N'))
$firstRoot = Join-Path $tempBase 'first'
$secondRoot = Join-Path $tempBase 'second'
try {
    Invoke-Checked $buildScript @('-OutputRoot', $firstRoot, '-Clean')
    Invoke-Checked $validateScript @('-OutputRoot', $firstRoot)
    $sidecarPath = Join-Path $firstRoot 'game\assets\audio\generated\launch.wav.import'
    [IO.File]::WriteAllText($sidecarPath, 'stable-import-sidecar')
    Invoke-Checked $buildScript @('-OutputRoot', $firstRoot, '-Clean')
    Assert-True ((Get-Content -Raw -LiteralPath $sidecarPath) -eq 'stable-import-sidecar') `
        'clean audio build must preserve Godot .wav.import sidecars'
    Invoke-Checked $buildScript @('-OutputRoot', $secondRoot, '-Clean')
    Invoke-Checked $validateScript @('-OutputRoot', $secondRoot)

    $firstManifest = Read-Json (Join-Path $firstRoot 'tools\audio\audio_manifest.json')
    $secondManifest = Read-Json (Join-Path $secondRoot 'tools\audio\audio_manifest.json')
    Assert-True ($firstManifest.schema_version -eq 1) 'manifest schema mismatch'
    Assert-True ($firstManifest.generator.seed -eq $config.seed) 'manifest seed mismatch'
    Assert-True ($firstManifest.generator.author -eq $config.author) 'manifest author mismatch'
    Assert-True ($firstManifest.generator.license -eq $config.license) 'manifest license mismatch'
    Assert-True ($firstManifest.generator.config_sha256 -match '^[0-9a-f]{64}$') 'config hash missing'
    Assert-True ($firstManifest.generator.generator_sha256 -match '^[0-9a-f]{64}$') 'generator hash missing'
    Assert-True ($firstManifest.build_hash -match '^[0-9a-f]{64}$') 'build hash missing'
    Assert-True ($firstManifest.build_hash -eq $secondManifest.build_hash) 'two clean generations have different build hashes'
    Assert-True ($firstManifest.outputs.Count -eq $expectedIds.Count) 'manifest output count mismatch'
    Assert-True ((($firstManifest.outputs.id | Sort-Object) -join '|') -eq (($expectedIds | Sort-Object) -join '|')) 'manifest output coverage mismatch'

    foreach ($output in $firstManifest.outputs) {
        Assert-True ($output.sha256 -match '^[0-9a-f]{64}$') "$($output.id) lacks SHA-256"
        Assert-True ($output.author -eq $config.author) "$($output.id) author mismatch"
        Assert-True ($output.license -eq $config.license) "$($output.id) license mismatch"
        Assert-True ($output.seed -is [long] -or $output.seed -is [int]) "$($output.id) seed missing"
        Assert-True ($output.sample_rate_hz -eq 48000) "$($output.id) sample rate mismatch"
        Assert-True ($output.channels -eq 1) "$($output.id) must be mono"
        Assert-True ($output.sample_width_bytes -eq 2) "$($output.id) must be PCM16"
        Assert-True ($output.frame_count -gt 0) "$($output.id) is empty"
        Assert-True ($output.peak_sample -gt 0 -and $output.peak_sample -le 31129) "$($output.id) is silent or clips"
        $other = @($secondManifest.outputs | Where-Object { $_.id -eq $output.id })
        Assert-True ($other.Count -eq 1 -and $other[0].sha256 -eq $output.sha256) "$($output.id) is not byte deterministic"
    }

    $firstReadme = Get-Content -Raw -LiteralPath (Join-Path $firstRoot 'game\assets\audio\README.md')
    $secondReadme = Get-Content -Raw -LiteralPath (Join-Path $secondRoot 'game\assets\audio\README.md')
    Assert-True ($firstReadme -eq $secondReadme) 'generated provenance README is not deterministic'
    foreach ($requiredText in @($config.author, $config.license, [string]$config.seed, $firstManifest.build_hash)) {
        Assert-True ($firstReadme.Contains($requiredText)) "README omits provenance value: $requiredText"
    }
    foreach ($output in $firstManifest.outputs) {
        Assert-True ($firstReadme.Contains($output.path) -and $firstReadme.Contains($output.sha256)) "README omits $($output.id) hash"
    }

    $tamperedPath = Join-Path $firstRoot 'game\assets\audio\generated\pine.wav'
    [IO.File]::AppendAllText($tamperedPath, 'tamper')
    Invoke-ExpectFailure { & powershell -NoProfile -ExecutionPolicy Bypass -File $validateScript -OutputRoot $firstRoot } 'hash mismatch.*pine'

    Invoke-Checked $buildScript @('-OutputRoot', $firstRoot, '-Clean')
    Remove-Item -LiteralPath (Join-Path $firstRoot 'game\assets\audio\generated\glass.wav')
    Invoke-ExpectFailure { & powershell -NoProfile -ExecutionPolicy Bypass -File $validateScript -OutputRoot $firstRoot } 'missing audio output.*glass'

    Invoke-ExpectFailure { & powershell -NoProfile -ExecutionPolicy Bypass -File $buildScript -OutputRoot $tempBase -AllowedRoot $tempBase -Clean } 'must not equal allowed root'
}
finally {
    if (Test-Path -LiteralPath $tempBase) {
        Remove-Item -LiteralPath $tempBase -Recurse -Force
    }
}

Write-Output 'audio pipeline: PASS'
exit 0
