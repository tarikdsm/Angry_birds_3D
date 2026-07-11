# Box3D Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a reproducible Windows x64 physics foundation that proves radial gravity, fast-projectile collision, piles, joints, deterministic scenarios, and a Godot visualization driven exclusively by Box3D v0.1.0.

**Architecture:** A headless C++20 `PhysicsWorld` owns the pinned C17 Box3D world behind generation-checked game handles. Tests and risk scenarios call that same public kernel API; a thin godot-cpp GDExtension copies batched snapshots into Godot, which remains presentation-only.

**Tech Stack:** Visual Studio Build Tools 2026 18.7.3, MSVC 14.44/v143, Windows SDK 10.0.26100.0, CMake 4.3.3, Ninja 1.13.2, C++20, Box3D v0.1.0, Godot 4.5.1, godot-cpp 4.5, PowerShell 7/Windows PowerShell 5.1, Python 3.11+.

## Global Constraints

- Target only Windows desktop x86_64 for this plan.
- Pin Box3D to commit `8441b4a06d6d09dcfb0b0f704df4d847d1437b92` and do not patch vendored source.
- Pin Godot to commit `f62fdbde15035c5576dad93e586201f4d41ef0cb` and godot-cpp to `e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77`.
- Use Visual Studio 2026 18.7.3 build 11925.98, MSVC component `Microsoft.VisualStudio.Component.VC.14.44.17.14.x86.x64`, and Windows SDK component `Microsoft.VisualStudio.Component.Windows11SDK.26100`.
- Use CMake generator `Ninja`, separate `build/debug` and `build/release`, CRT `/MTd|/MT`, and never `/fp:fast`.
- Use a fixed `1/60 s` tick, four Box3D substeps by default, six only for the approved CCD fallback.
- Keep Box3D single-threaded during this plan.
- Do not create Godot `RigidBody3D`, `StaticBody3D`, `CharacterBody3D`, or Godot collision nodes for gameplay.
- Keep Box3D IDs, pointers, and transient event arrays inside `native/kernel/src`.
- Follow RED–GREEN–REFACTOR for every behavior. A production function is added only after its focused test fails for the expected reason.
- Keep generated downloads, build trees, reports, movies, and screenshots outside Git-tracked paths.

## Planned File Map

```text
/.gitattributes                     LF/CRLF policy
/.gitignore                         build, tool, cache, and artifact exclusions
/CMakeLists.txt                     root project and subdirectories
/CMakePresets.json                  Debug/Release Ninja presets
/cmake/Dependencies.cmake           pinned FetchContent declarations
/third_party/README.md              dependency/license inventory
/tools/toolchain.lock.json          official URLs, versions, SHA-256, VS components
/tools/bootstrap.ps1                validate/install toolchain
/tools/Invoke-Native.ps1            import VsDevCmd environment and run a command
/tools/build.ps1                    configure/build selected preset
/tools/test.ps1                     native, upstream, and Godot smoke tests
/tools/run_spike.ps1                scenario/report runner
/tools/tests/bootstrap-smoke.ps1    bootstrap lock/detection smoke test
/native/kernel/include/ninho/physics/physics_types.hpp
/native/kernel/include/ninho/physics/radial_gravity.hpp
/native/kernel/include/ninho/physics/physics_world.hpp
/native/kernel/include/ninho/physics/scenario.hpp
/native/kernel/src/radial_gravity.cpp
/native/kernel/src/physics_world.cpp
/native/kernel/src/scenario.cpp
/native/kernel/src/box3d_conversions.hpp
/native/kernel/CMakeLists.txt
/native/tests/test_framework.hpp
/native/tests/test_main.cpp
/native/tests/radial_gravity_tests.cpp
/native/tests/world_lifecycle_tests.cpp
/native/tests/projectile_ccd_tests.cpp
/native/tests/pile_stability_tests.cpp
/native/tests/capability_tests.cpp
/native/tests/determinism_tests.cpp
/native/tests/CMakeLists.txt
/native/spike/main.cpp
/native/spike/CMakeLists.txt
/native/extension/include/ninho/extension/box3d_world_node.hpp
/native/extension/src/box3d_world_node.cpp
/native/extension/src/register_types.cpp
/native/extension/CMakeLists.txt
/game/project.godot
/game/bin/ninho_physics.gdextension
/game/scenes/physics_spike.tscn
/game/scripts/physics_spike_view.gd
/game/scripts/physics_spike_smoke.gd
/docs/physics/box3d-spike-report.md
```

---

### Task 1: Reproducible Toolchain Bootstrap

**Files:**
- Create: `.gitattributes`
- Create: `.gitignore`
- Create: `tools/toolchain.lock.json`
- Create: `tools/bootstrap.ps1`
- Create: `tools/Invoke-Native.ps1`
- Create: `tools/build.ps1`
- Create: `tools/tests/bootstrap-smoke.ps1`
- Create: `third_party/README.md`

**Interfaces:**
- Consumes: official download endpoints and the exact versions from Global Constraints.
- Produces: `tools/bootstrap.ps1 -ValidateLock|-CheckOnly|-InstallPortable|-InstallVisualStudio`; JSON with `ok`, `cmake`, `ninja`, `godot`, `python`, `visual_studio`, and `errors`; `tools/Invoke-Native.ps1 -Command <string>`.

- [ ] **Step 1: Write the failing bootstrap lock smoke test**

Create `tools/tests/bootstrap-smoke.ps1`:

```powershell
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$bootstrap = Join-Path $root 'tools\bootstrap.ps1'
if (-not (Test-Path -LiteralPath $bootstrap)) {
    throw "bootstrap.ps1 is missing"
}
$json = & $bootstrap -ValidateLock -Json | ConvertFrom-Json
if (-not $json.ok) { throw ($json.errors -join '; ') }
if ($json.cmake.version -ne '4.3.3') { throw 'CMake lock mismatch' }
if ($json.ninja.version -ne '1.13.2') { throw 'Ninja lock mismatch' }
if ($json.godot.version -ne '4.5.1-stable') { throw 'Godot lock mismatch' }
if ($json.visual_studio.version -ne '18.7.3') { throw 'Visual Studio lock mismatch' }
if ($json.python.minimum_version -ne '3.11.0') { throw 'Python requirement mismatch' }
Write-Output 'bootstrap lock: PASS'
```

- [ ] **Step 2: Run it and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\tests\bootstrap-smoke.ps1
```

Expected: non-zero exit with `bootstrap.ps1 is missing`.

- [ ] **Step 3: Add the exact lock and bootstrap implementation**

Create `tools/toolchain.lock.json`:

```json
{
  "cmake": {
    "version": "4.3.3",
    "url": "https://github.com/Kitware/CMake/releases/download/v4.3.3/cmake-4.3.3-windows-x86_64.zip",
    "sha256": "935ade9e5e8723583c07f44c5592cea2a1c8f65c56ca7e07b34c025c880e0bd6",
    "exe_sha256": "70fa92ce2ac9f54b0ae395b0b3790d9147ef2ebdbd7c4e0bb20852aac581baea",
    "exe": "cmake-4.3.3-windows-x86_64/bin/cmake.exe"
  },
  "ninja": {
    "version": "1.13.2",
    "url": "https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-win.zip",
    "sha256": "07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65",
    "exe_sha256": "e52a7ad9538d9618c67a0bd777964e2eec8a30f68b810a2f6adce1f2daf847b8",
    "exe": "ninja.exe"
  },
  "godot": {
    "version": "4.5.1-stable",
    "url": "https://github.com/godotengine/godot/releases/download/4.5.1-stable/Godot_v4.5.1-stable_win64.exe.zip",
    "sha256": "defccc78669e644861b4247626b01ae362cd9f23975edf19c8bfd2eb1f6a1783",
    "exe_sha256": "a829242096d640007de9fa93ea923d0f666d13d6a3b472a0231c820be6dc7627",
    "exe": "Godot_v4.5.1-stable_win64.exe"
  },
  "python": {
    "minimum_version": "3.11.0",
    "command": "python"
  },
  "visual_studio": {
    "version": "18.7.3",
    "build": "11925.98",
    "url": "https://download.visualstudio.microsoft.com/download/pr/4037ccca-d103-412b-a678-bc0aa164315e/07b09afd416dc05c781f171c881c23e42907eeb8d812fa1d2993dffb9323c869/vs_BuildTools.exe",
    "sha256": "07b09afd416dc05c781f171c881c23e42907eeb8d812fa1d2993dffb9323c869",
    "components": [
      "Microsoft.VisualStudio.Component.VC.14.44.17.14.x86.x64",
      "Microsoft.VisualStudio.Component.Windows11SDK.26100"
    ]
  }
}
```

Create `tools/bootstrap.ps1` with these exact behaviors:

```powershell
[CmdletBinding(DefaultParameterSetName='Check')]
param(
    [Parameter(ParameterSetName='Validate')][switch]$ValidateLock,
    [Parameter(ParameterSetName='Check')][switch]$CheckOnly,
    [Parameter(ParameterSetName='Portable')][switch]$InstallPortable,
    [Parameter(ParameterSetName='VisualStudio')][switch]$InstallVisualStudio,
    [switch]$Json
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$tools = Join-Path $root '.tools'
$downloads = Join-Path $tools 'downloads'
$lockPath = Join-Path $PSScriptRoot 'toolchain.lock.json'
$lock = Get-Content -Raw -LiteralPath $lockPath | ConvertFrom-Json
$errors = [System.Collections.Generic.List[string]]::new()

function Test-HexSha([string]$value) { return $value -match '^[0-9a-f]{64}$' }
foreach ($name in 'cmake','ninja','godot','visual_studio') {
    $entry = $lock.$name
    if (-not $entry.version) { $errors.Add("$name.version missing") }
    if ($entry.url -notmatch '^https://') { $errors.Add("$name.url must use https") }
    if (-not (Test-HexSha $entry.sha256)) { $errors.Add("$name.sha256 invalid") }
    if ($name -ne 'visual_studio' -and -not (Test-HexSha $entry.exe_sha256)) { $errors.Add("$name.exe_sha256 invalid") }
}

function Get-LockedArchive([string]$name) {
    $entry = $lock.$name
    New-Item -ItemType Directory -Force $downloads | Out-Null
    $extension = [IO.Path]::GetExtension(([Uri]$entry.url).AbsolutePath)
    $target = Join-Path $downloads "$name$extension"
    if (-not (Test-Path -LiteralPath $target)) {
        Invoke-WebRequest -UseBasicParsing -Uri $entry.url -OutFile $target
    }
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $target).Hash.ToLowerInvariant()
    if ($actual -ne $entry.sha256) { throw "$name checksum mismatch: $actual" }
    return $target
}

function Install-Portable([string]$name) {
    $entry = $lock.$name
    $destination = Join-Path $tools $name
    $exe = Join-Path $destination $entry.exe
    if (-not (Test-Path -LiteralPath $exe)) {
        $archive = Get-LockedArchive $name
        New-Item -ItemType Directory -Force $destination | Out-Null
        Expand-Archive -LiteralPath $archive -DestinationPath $destination -Force
    }
    return $exe
}

$cmake = Join-Path $tools ('cmake\' + $lock.cmake.exe)
$ninja = Join-Path $tools ('ninja\' + $lock.ninja.exe)
$godot = Join-Path $tools ('godot\' + $lock.godot.exe)
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsInstall = $null
function Get-LockedVisualStudio {
    if (-not (Test-Path -LiteralPath $vswhere)) { return $null }
    $requirements = @($lock.visual_studio.components)
    $items = & $vswhere -version '[18.7,18.8)' -products '*' -requires $requirements -format json | ConvertFrom-Json
    $match = $items | Where-Object {
        $_.catalog.productDisplayVersion -eq '18.7.3' -and $_.installationVersion -like '18.7.11925.98*'
    } | Select-Object -First 1
    return $match.installationPath
}
if (Test-Path -LiteralPath $vswhere) {
    $vsInstall = Get-LockedVisualStudio
}

if ($InstallPortable -and $errors.Count -eq 0) {
    $cmake = Install-Portable 'cmake'
    $ninja = Install-Portable 'ninja'
    $godot = Install-Portable 'godot'
}
if ($InstallVisualStudio -and $errors.Count -eq 0) {
    if (-not $vsInstall) {
        $installer = Get-LockedArchive 'visual_studio'
        $args = @('--quiet','--wait','--norestart','--nocache')
        foreach ($component in $lock.visual_studio.components) { $args += @('--add', $component) }
        $process = Start-Process -FilePath $installer -ArgumentList $args -Wait -PassThru -WindowStyle Hidden
        if ($process.ExitCode -notin 0,3010) { throw "VS installer failed: $($process.ExitCode)" }
        $vsInstall = Get-LockedVisualStudio
    }
}

if ($CheckOnly -or $InstallPortable) {
    foreach ($pair in @(@('cmake',$cmake),@('ninja',$ninja),@('godot',$godot))) {
        if (-not (Test-Path -LiteralPath $pair[1])) { $errors.Add("$($pair[0]) missing: $($pair[1])") }
    }
    if (Test-Path $cmake) { if ((& $cmake --version | Select-Object -First 1) -notmatch '4\.3\.3') { $errors.Add('CMake executable version mismatch') } }
    if (Test-Path $ninja) { if ((& $ninja --version) -ne '1.13.2') { $errors.Add('Ninja executable version mismatch') } }
    if (Test-Path $godot) { if ((& $godot --version) -notmatch '^4\.5\.1\.stable') { $errors.Add('Godot executable version mismatch') } }
    foreach ($pair in @(@('cmake',$cmake),@('ninja',$ninja),@('godot',$godot))) {
        if (Test-Path $pair[1]) {
            $actualExe=(Get-FileHash -Algorithm SHA256 -LiteralPath $pair[1]).Hash.ToLowerInvariant()
            if ($actualExe -ne $lock.($pair[0]).exe_sha256) { $errors.Add("$($pair[0]) executable checksum mismatch") }
        }
    }
    $pythonVersion = $null
    $pythonCommand = Get-Command $lock.python.command -ErrorAction SilentlyContinue
    if (-not $pythonCommand) {
        $errors.Add('Python 3.11+ missing')
    } else {
        try { $pythonVersion = & $pythonCommand.Source -c "import sys; print('.'.join(map(str,sys.version_info[:3])))" }
        catch { $errors.Add("Python detection failed: $($_.Exception.Message)") }
        if ($pythonVersion -and [version]$pythonVersion -lt [version]$lock.python.minimum_version) { $errors.Add("Python too old: $pythonVersion") }
    }
}
if ($CheckOnly -or $InstallVisualStudio) {
    if (-not $vsInstall) { $errors.Add('Visual Studio components missing') }
}

$result = [ordered]@{
    ok = ($errors.Count -eq 0)
    cmake = @{ version=$lock.cmake.version; path=$cmake }
    ninja = @{ version=$lock.ninja.version; path=$ninja }
    godot = @{ version=$lock.godot.version; path=$godot }
    python = @{ minimum_version=$lock.python.minimum_version; detected_version=$pythonVersion }
    visual_studio = @{ version=$lock.visual_studio.version; path=$vsInstall }
    errors = @($errors)
}
if ($Json) { $result | ConvertTo-Json -Depth 5 } else { $result }
if (-not $result.ok) { exit 1 }
```

Create `tools/Invoke-Native.ps1`:

```powershell
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Command)
$ErrorActionPreference = 'Stop'
$bootstrap = Join-Path $PSScriptRoot 'bootstrap.ps1'
$check = & $bootstrap -CheckOnly -Json | ConvertFrom-Json
if (-not $check.ok) { throw ($check.errors -join '; ') }
$vs = $check.visual_studio.path
if (-not $vs) { throw 'Locked Visual Studio 18.7.3 was not found; run tools/bootstrap.ps1 -InstallVisualStudio' }
$devCmd = Join-Path $vs 'Common7\Tools\VsDevCmd.bat'
$environment = & cmd.exe /s /c "`"$devCmd`" -no_logo -arch=x64 -host_arch=x64 -vcvars_ver=14.44 -winsdk=10.0.26100.0 && set"
foreach ($line in $environment) {
    $split = $line.IndexOf('=')
    if ($split -gt 0) { Set-Item -Path "Env:$($line.Substring(0,$split))" -Value $line.Substring($split+1) }
}
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$cmakeBin = Split-Path (Join-Path $root '.tools\cmake\cmake-4.3.3-windows-x86_64\bin\cmake.exe')
$ninjaBin = Split-Path (Join-Path $root '.tools\ninja\ninja.exe')
$env:PATH = "$cmakeBin;$ninjaBin;$env:PATH"
& powershell -NoProfile -ExecutionPolicy Bypass -Command $Command
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
```

Create `tools/build.ps1`:

```powershell
[CmdletBinding()]
param(
    [ValidateSet('Debug','Release')][string]$Configuration='Debug',
    [switch]$WithGodot
)
$preset = $Configuration.ToLowerInvariant()
$godot = if ($WithGodot) { '-DNINHO_BUILD_GDEXTENSION=ON' } else { '-DNINHO_BUILD_GDEXTENSION=OFF' }
$command = "cmake --preset $preset $godot; if (`$LASTEXITCODE) { exit `$LASTEXITCODE }; cmake --build --preset $preset"
& (Join-Path $PSScriptRoot 'Invoke-Native.ps1') -Command $command
exit $LASTEXITCODE
```

Create `.gitattributes`:

```gitattributes
* text=auto
*.md text eol=lf
*.json text eol=lf
*.cpp text eol=lf
*.hpp text eol=lf
*.h text eol=lf
*.gd text eol=lf
*.tscn text eol=lf
*.gdextension text eol=lf
*.ps1 text eol=crlf
```

Create `.gitignore`:

```gitignore
/.tools/
/build/
/.fetchcontent-cache/
/artifacts/
/.godot/
/game/.godot/
/game/bin/*.dll
/game/bin/*.pdb
*.user
*.suo
```

Create `third_party/README.md` with the three pinned Git commits, Godot download version, their MIT licenses, and links from the specs. Do not copy license text yet; Task 8 creates the distributable notices.

- [ ] **Step 4: Verify GREEN and install/check the toolchain**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\tests\bootstrap-smoke.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\bootstrap.ps1 -InstallPortable -Json
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\bootstrap.ps1 -InstallVisualStudio -Json
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\bootstrap.ps1 -CheckOnly -Json
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\Invoke-Native.ps1 -Command 'cl 2>&1 | Select-Object -First 1; cmake --version; ninja --version'
```

Expected: `bootstrap lock: PASS`; install result has `"ok": true`; compiler reports MSVC 19.44, CMake 4.3.3, Ninja 1.13.2. A VS installer exit `3010` means reboot required; finish the current safe file work, reboot outside the script, then rerun `-CheckOnly` before Task 2.

- [ ] **Step 5: Commit**

```powershell
git add .gitattributes .gitignore tools third_party/README.md
git commit -m "build: preparar toolchain reproduzivel"
```

---

### Task 2: CMake Skeleton, Test Harness, and Radial Gravity

**Files:**
- Create: `CMakeLists.txt`
- Create: `CMakePresets.json`
- Create: `cmake/Dependencies.cmake`
- Create: `native/kernel/CMakeLists.txt`
- Create: `native/kernel/include/ninho/physics/physics_types.hpp`
- Create: `native/kernel/include/ninho/physics/radial_gravity.hpp`
- Create: `native/kernel/src/radial_gravity.cpp`
- Create: `native/tests/CMakeLists.txt`
- Create: `native/tests/test_framework.hpp`
- Create: `native/tests/test_main.cpp`
- Create: `native/tests/radial_gravity_tests.cpp`

**Interfaces:**
- Consumes: pinned Box3D CMake target `box3d::box3d`; toolchain from Task 1.
- Produces: `ninho::physics::Vec3`; `RadialGravity::acceleration(Vec3)`; native test executable supporting `--filter <substring>`.

- [ ] **Step 1: Write the failing radial-gravity tests and minimal runner**

Create `native/tests/test_framework.hpp` as a small registry with `NINHO_TEST(name)`, `NINHO_REQUIRE(expr)`, and `NINHO_REQUIRE_NEAR(actual, expected, epsilon)` macros. Store `TestCase { const char* name; void(*run)(); }` in a function-local vector; exceptions carry file, line, and expression. `test_main.cpp` parses optional `--filter`, runs matching tests, prints `[PASS]`/`[FAIL]`, and returns the failure count.

Use this complete macro surface so every later test has one vocabulary:

```cpp
#pragma once
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace ninho::test {
struct Case { std::string name; void(*run)(); };
inline std::vector<Case>& registry(){ static std::vector<Case> value; return value; }
struct Registrar { Registrar(const char* name,void(*run)()){ registry().push_back({name,run}); } };
[[noreturn]] inline void fail(const char* file,int line,const std::string& message){
    std::ostringstream out; out<<file<<':'<<line<<": "<<message; throw std::runtime_error(out.str());
}
}
#define NINHO_JOIN_INNER(a,b) a##b
#define NINHO_JOIN(a,b) NINHO_JOIN_INNER(a,b)
#define NINHO_TEST(name) NINHO_TEST_IMPL(name,__COUNTER__)
#define NINHO_TEST_IMPL(name,n) \
 static void NINHO_JOIN(ninho_test_,n)(); \
 static ::ninho::test::Registrar NINHO_JOIN(ninho_reg_,n)(name,&NINHO_JOIN(ninho_test_,n)); \
 static void NINHO_JOIN(ninho_test_,n)()
#define NINHO_REQUIRE(expr) do { if(!(expr)) ::ninho::test::fail(__FILE__,__LINE__,"require failed: " #expr); } while(false)
#define NINHO_REQUIRE_NEAR(actual,expected,epsilon) do { \
 const double a_=(actual),e_=(expected),d_=(epsilon); \
 if(std::abs(a_-e_)>d_) { std::ostringstream m_; m_<<#actual<<'='<<a_<<", expected "<<e_<<" ± "<<d_; \
 ::ninho::test::fail(__FILE__,__LINE__,m_.str()); } } while(false)
```

Create `native/tests/radial_gravity_tests.cpp`:

```cpp
#include "test_framework.hpp"
#include <ninho/physics/radial_gravity.hpp>
using namespace ninho::physics;

NINHO_TEST("gravity points toward center on every axis") {
    RadialGravity gravity({.center={0,0,0}, .radius=10.0f, .surface_acceleration=9.0f});
    const Vec3 x = gravity.acceleration({10,0,0});
    const Vec3 y = gravity.acceleration({0,10,0});
    const Vec3 z = gravity.acceleration({0,0,-10});
    NINHO_REQUIRE_NEAR(x.x, -9.0f, 1e-4f);
    NINHO_REQUIRE_NEAR(y.y, -9.0f, 1e-4f);
    NINHO_REQUIRE_NEAR(z.z,  9.0f, 1e-4f);
}

NINHO_TEST("gravity decays, caps near center, and is finite at center") {
    RadialGravity gravity({.center={0,0,0}, .radius=10.0f, .surface_acceleration=9.0f});
    NINHO_REQUIRE_NEAR(length(gravity.acceleration({20,0,0})), 2.25f, 1e-4f);
    NINHO_REQUIRE_NEAR(length(gravity.acceleration({6,0,0})), 18.0f, 1e-4f);
    const Vec3 center = gravity.acceleration({0,0,0});
    NINHO_REQUIRE(center == Vec3{});
    NINHO_REQUIRE(is_finite(center));
}

NINHO_TEST("ejection predicate requires radius speed and duration") {
    EjectionTracker tracker;
    const BodyHandle body{1,1};
    for (int i=0; i<29; ++i) {
        NINHO_REQUIRE(!tracker.update(body, 41.0f, 2.1f, 1.0f/60.0f, 10.0f));
    }
    NINHO_REQUIRE(tracker.update(body, 41.0f, 2.1f, 1.0f/60.0f, 10.0f));
    tracker.reset(body);
    NINHO_REQUIRE(!tracker.update(body, 39.9f, 3.0f, 1.0f, 10.0f));
}
```

- [ ] **Step 2: Configure/build and verify RED**

Create only enough root/test CMake to name the source files, then run:

```powershell
.\tools\Invoke-Native.ps1 -Command 'cmake --preset debug; cmake --build --preset debug --target ninho_physics_tests'
```

Expected: compilation fails because `ninho/physics/radial_gravity.hpp` does not exist.

- [ ] **Step 3: Implement math, gravity, dependencies, and presets**

`physics_types.hpp` defines trivial `Vec3`, `Quat`, `Transform`, generation handles, arithmetic, `dot`, `cross`, `length`, `normalized_or_zero`, and `is_finite`. Use `constexpr` where possible. `BodyHandle` is `struct BodyHandle { std::uint32_t index{},generation{}; bool valid() const; auto operator<=>(const BodyHandle&) const = default; };`; validity requires nonzero index and generation.

`radial_gravity.hpp` exposes:

```cpp
struct RadialGravityConfig { Vec3 center{}; float radius{10}; float surface_acceleration{9}; };
class RadialGravity {
public:
    explicit RadialGravity(RadialGravityConfig config) : config_(config) {}
    [[nodiscard]] Vec3 acceleration(Vec3 position) const noexcept;
private: RadialGravityConfig config_;
};
class EjectionTracker {
public:
    bool update(BodyHandle, float radius, float radial_speed, float dt, float planet_radius);
    void reset(BodyHandle);
private: std::unordered_map<std::uint64_t,float> elapsed_;
};
```

`radial_gravity.cpp` implements `a=min(g*R²/max(r²,(0.6R)²),18)`, returns zero below `0.001 m`, and requires `radius≥4R`, radial speed `≥2 m/s`, and accumulated time `≥0.5 s`. Key map is `(uint64_t(generation)<<32)|index`; reset elapsed on any failed predicate.

`cmake/Dependencies.cmake` must use full commit hashes:

```cmake
include(FetchContent)
set(FETCHCONTENT_BASE_DIR "${PROJECT_SOURCE_DIR}/.fetchcontent-cache" CACHE PATH "" FORCE)
FetchContent_Declare(box3d
  GIT_REPOSITORY https://github.com/erincatto/box3d.git
  GIT_TAG 8441b4a06d6d09dcfb0b0f704df4d847d1437b92
  GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(box3d)
```

Root CMake sets C17/C++20, `CMAKE_MSVC_RUNTIME_LIBRARY`, `NINHO_BUILD_GDEXTENSION` default OFF, includes dependencies, enables CTest, and adds `native/kernel`, `native/tests`, and later subdirectories conditionally. Presets use generator Ninja, `build/debug|release`, and set `CMAKE_BUILD_TYPE` plus `GODOTCPP_TARGET=template_debug|template_release`.

Create root `CMakeLists.txt` exactly as the foundation grows; at Task 2 it is:

```cmake
cmake_minimum_required(VERSION 3.22)
project(ninho_orbital VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
option(NINHO_BUILD_GDEXTENSION "Build the Godot presentation adapter" OFF)
include(cmake/Dependencies.cmake)
include(CTest)
add_subdirectory(native/kernel)
if(BUILD_TESTING)
  add_subdirectory(native/tests)
endif()
```

Create `CMakePresets.json`:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "base",
      "hidden": true,
      "generator": "Ninja",
      "cacheVariables": {
        "CMAKE_C_COMPILER": "cl",
        "CMAKE_CXX_COMPILER": "cl",
        "CMAKE_SYSTEM_VERSION": "10.0.26100.0",
        "BUILD_SHARED_LIBS": "OFF"
      }
    },
    {
      "name": "debug",
      "inherits": "base",
      "binaryDir": "${sourceDir}/build/debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "GODOTCPP_TARGET": "template_debug"
      }
    },
    {
      "name": "release",
      "inherits": "base",
      "binaryDir": "${sourceDir}/build/release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "GODOTCPP_TARGET": "template_release"
      }
    }
  ],
  "buildPresets": [
    { "name": "debug", "configurePreset": "debug" },
    { "name": "release", "configurePreset": "release" }
  ],
  "testPresets": [
    { "name": "debug", "configurePreset": "debug", "output": { "outputOnFailure": true } },
    { "name": "release", "configurePreset": "release", "output": { "outputOnFailure": true } }
  ]
}
```

`native/kernel/CMakeLists.txt` creates static target `ninho_physics_kernel`, alias `ninho::physics`, exposes `include`, links `box3d::box3d`, and enables `/W4 /permissive- /fp:precise` for project C++ only. `native/tests/CMakeLists.txt` creates `ninho_physics_tests`, links the kernel, registers one CTest per filter (`radial`, later `world`, `capability`, `scenario`), and never compiles Box3D internals directly.

- [ ] **Step 4: Verify GREEN in Debug and Release**

```powershell
.\tools\Invoke-Native.ps1 -Command 'cmake --preset debug; cmake --build --preset debug; ctest --preset debug --output-on-failure'
.\tools\Invoke-Native.ps1 -Command 'cmake --preset release; cmake --build --preset release; ctest --preset release --output-on-failure'
```

Expected: three radial tests pass in both presets; configure output identifies Box3D 0.1.0 and MSVC; no warning/error from project sources.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt CMakePresets.json cmake native/kernel native/tests
git commit -m "feat: adicionar gravidade radial testada"
```

---

### Task 3: Generation-Safe Physics World and Primitive Bodies

**Files:**
- Create: `native/kernel/include/ninho/physics/physics_world.hpp`
- Create: `native/kernel/src/physics_world.cpp`
- Create: `native/kernel/src/box3d_conversions.hpp`
- Create: `native/tests/world_lifecycle_tests.cpp`
- Modify: `native/kernel/CMakeLists.txt`
- Modify: `native/tests/CMakeLists.txt`

**Interfaces:**
- Consumes: Task 2 math and `box3d::box3d`.
- Produces: `WorldConfig`, `ShapeDesc`, `BodyDesc`, `BodyState`, `Result<T>`, and `PhysicsWorld` lifecycle/step/snapshot API. Hull/query/joint variants return explicit `Unsupported` until Task 4.

- [ ] **Step 1: Write failing lifecycle, handle, and radial-fall tests**

`world_lifecycle_tests.cpp` must cover:

```cpp
NINHO_TEST("destroyed handle is rejected after slot reuse") {
    PhysicsWorld world(WorldConfig{});
    BodyDesc box = BodyDesc::dynamic_box({0.5f,0.5f,0.5f}, {{0,15,0},{}}, 520.0f);
    const BodyHandle old = world.create_body(box).value;
    world.step();
    NINHO_REQUIRE(world.destroy_body(old).ok()); world.step();
    const BodyHandle replacement = world.create_body(box).value;
    NINHO_REQUIRE(old.index == replacement.index);
    NINHO_REQUIRE(old.generation != replacement.generation);
    NINHO_REQUIRE(world.apply_impulse(old,{1,0,0},{0,0,0}).code == StatusCode::InvalidHandle);
}

NINHO_TEST("dynamic sphere falls radially and settles on planet") {
    WorldConfig config{.substeps=4,.planet_radius=10,.surface_gravity=9};
    PhysicsWorld world(config);
    world.create_body(BodyDesc::static_sphere(10.0f, {{0,0,0},{}}));
    const auto ball = world.create_body(BodyDesc::dynamic_sphere(0.4f, {{0,15,0},{}}, 520.0f)).value;
    for (int i=0; i<600; ++i) world.step();
    const auto state = world.state(ball);
    NINHO_REQUIRE(state.has_value());
    NINHO_REQUIRE_NEAR(length(state->transform.position), 10.4f, 0.03f);
    NINHO_REQUIRE(length(state->linear_velocity) < 0.05f);
}

NINHO_TEST("snapshot contains only live public handles") {
    PhysicsWorld world(WorldConfig{});
    auto a = world.create_body(BodyDesc::dynamic_sphere(0.4f, {{0,15,0},{}}, 520)).value;
    auto b = world.create_body(BodyDesc::dynamic_box({1,1,1}, {{0,18,0},{}}, 520)).value;
    world.step(); world.destroy_body(a); world.step();
    NINHO_REQUIRE(world.states().size() == 1);
    NINHO_REQUIRE(world.states()[0].handle == b);
}
```

- [ ] **Step 2: Build and verify RED**

```powershell
.\tools\Invoke-Native.ps1 -Command 'cmake --build --preset debug --target ninho_physics_tests'
```

Expected: compile failure for missing `physics_world.hpp`.

- [ ] **Step 3: Implement the public API and Box3D adapter**

Define exact public variants in `physics_world.hpp`:

```cpp
enum class StatusCode { Ok, InvalidArgument, InvalidHandle, CapacityExceeded, Unsupported, Box3DFault };
struct Status { StatusCode code{StatusCode::Ok}; std::string message{}; bool ok() const { return code==StatusCode::Ok; } };
template<class T> struct Result { T value{}; Status status{}; explicit operator bool() const { return status.ok(); } };
enum class BodyType { Static, Kinematic, Dynamic };
struct SphereShape { float radius{}; Transform local{}; };
struct BoxShape { Vec3 half_extents{}; Transform local{}; };
struct CapsuleShape { float half_height{},radius{}; Transform local{}; };
struct HullShape { std::vector<Vec3> vertices; Transform local{}; };
using PrimitiveShape = std::variant<SphereShape,BoxShape,CapsuleShape,HullShape>;
struct CompoundShape { std::vector<PrimitiveShape> children; };
using ShapeGeometry = std::variant<SphereShape,BoxShape,CapsuleShape,HullShape,CompoundShape>;
struct ShapeDesc { ShapeGeometry geometry; float density{1}; float friction{0.5f}; float restitution{0}; std::uint64_t material_id{}; bool hit_events{true}; };
struct BodyDesc {
    BodyType type{BodyType::Static}; Transform transform{}; Vec3 linear_velocity{}; Vec3 angular_velocity{};
    std::vector<ShapeDesc> shapes; bool bullet{}; bool enable_sleep{true}; bool radial_gravity{true}; bool remove_beyond_six_r{true}; std::string name;
    static BodyDesc static_sphere(float,Transform);
    static BodyDesc static_box(Vec3,Transform);
    static BodyDesc dynamic_sphere(float,Transform,float);
    static BodyDesc dynamic_box(Vec3,Transform,float);
};
struct WorldConfig { float time_step{1.0f/60.0f}; int substeps{4}; float planet_radius{10}; float surface_gravity{9}; std::size_t max_bodies{500}; };
struct BodyState { BodyHandle handle{}; Transform transform{}; Vec3 linear_velocity{}; Vec3 angular_velocity{}; float mass{}; bool awake{}; bool ejected{}; };
class PhysicsWorld {
public:
    explicit PhysicsWorld(WorldConfig); ~PhysicsWorld();
    PhysicsWorld(PhysicsWorld&&) noexcept; PhysicsWorld& operator=(PhysicsWorld&&) noexcept;
    PhysicsWorld(const PhysicsWorld&)=delete; PhysicsWorld& operator=(const PhysicsWorld&)=delete;
    Result<BodyHandle> create_body(const BodyDesc&); Status destroy_body(BodyHandle);
    Status apply_force(BodyHandle,Vec3,Vec3,bool wake=true);
    Status apply_impulse(BodyHandle,Vec3,Vec3,bool wake=true);
    void step(); std::optional<BodyState> state(BodyHandle) const;
    std::span<const BodyState> states() const; const WorldConfig& config() const;
private: struct Impl; std::unique_ptr<Impl> impl_;
};
```

`physics_world.cpp` validates finite transforms, positive extents/radius/density, at least one shape on dynamic bodies, and capacity. Reserve slot zero and reserve/reuse generation handles when commands are enqueued. `create_body`, `destroy_body`, joint creation/destruction, forces, and impulses append immutable command variants; `step()` applies the whole queue in insertion order before gravity and Box3D stepping. A create followed by destroy before the next tick cancels safely while still advancing the generation.

Convert sphere/capsule centers from their local `Transform`. Convert boxes with `b3MakeTransformedBoxHull`; never edit hull vertices in place. Custom hulls use `b3CreateHull` followed by `b3CreateTransformedHullShape(body,def,hull,local_transform,{1,1,1})`, then destroy the temporary source hull after Box3D copies it. Expand `CompoundShape.children` into multiple shapes on the same body through the same primitive adapter. Set `b3ShapeDef.baseMaterial.friction`, `.restitution`, `.userMaterialId`, density, and hit-events. Each tick: apply `mass * radial.acceleration(position)` only to awake eligible dynamic bodies with `wake=false`; call `b3World_Step`; copy states; update ejection; queue bodies with `remove_beyond_six_r` once they reach `6R`, then destroy that queue at the start of the next tick. No Box3D type appears in the header.

- [ ] **Step 4: Verify GREEN and upstream hello behavior**

```powershell
.\tools\Invoke-Native.ps1 -Command 'cmake --build --preset debug; ctest --preset debug --output-on-failure -R world'
```

Expected: handle reuse, fall/settle, and snapshot tests pass. Then run all native tests and confirm zero failures.

- [ ] **Step 5: Commit**

```powershell
git add native/kernel native/tests
git commit -m "feat: encapsular mundo Box3D"
```

---

### Task 4: Hulls, Compounds, Queries, Joints, and Copied Events

**Files:**
- Modify: `native/kernel/include/ninho/physics/physics_types.hpp`
- Modify: `native/kernel/include/ninho/physics/physics_world.hpp`
- Modify: `native/kernel/src/physics_world.cpp`
- Modify: `native/kernel/src/box3d_conversions.hpp`
- Create: `native/tests/capability_tests.cpp`
- Modify: `native/tests/CMakeLists.txt`

**Interfaces:**
- Consumes: generation-safe body mapping from Task 3.
- Produces: `JointHandle`, distance/weld joint creation, sphere cast/overlap, hull and multi-shape bodies, copied `ContactHit` and `JointReaction` spans, `WorldMetrics`.

- [ ] **Step 1: Write failing capability tests through the public kernel**

Tests must prove:

```cpp
NINHO_TEST("compound body has expected mass and can be queried") {
    PhysicsWorld world(WorldConfig{.surface_gravity=0});
    BodyDesc body{.type=BodyType::Dynamic,.transform={{0,5,0},{}}};
    body.shapes={
        ShapeDesc{.geometry=BoxShape{{0.5f,0.5f,0.5f},{{-0.75f,0,0},{}}},.density=500},
        ShapeDesc{.geometry=BoxShape{{0.5f,0.5f,0.5f},{{ 0.75f,0,0},{}}},.density=500}
    };
    const auto handle=world.create_body(body).value;
    world.step();
    const auto hits=world.overlap_sphere({0,5,0},2.0f);
    NINHO_REQUIRE(hits.size()==1);
    NINHO_REQUIRE(hits.front().body==handle);
    NINHO_REQUIRE(world.state(handle)->mass>0.0f);
}
NINHO_TEST("sphere cast returns first hit") {
    PhysicsWorld world(WorldConfig{.surface_gravity=0});
    const auto high=world.create_body(BodyDesc::static_box({1,1,1},{{0,8,0},{}})).value;
    world.create_body(BodyDesc::static_box({1,1,1},{{0,4,0},{}}));
    world.step();
    const auto hit=world.cast_sphere({0,12,0},0.25f,{0,-12,0});
    NINHO_REQUIRE(hit.has_value());
    NINHO_REQUIRE(hit->body==high);
    NINHO_REQUIRE(hit->fraction>=0.0f && hit->fraction<=1.0f);
}
NINHO_TEST("distance joint exposes finite force and destroys safely") {
    PhysicsWorld world(WorldConfig{.surface_gravity=0});
    auto a=world.create_body(BodyDesc::dynamic_box({0.5f,0.5f,0.5f},{{-1,5,0},{}},500)).value;
    auto b=world.create_body(BodyDesc::dynamic_box({0.5f,0.5f,0.5f},{{ 1,5,0},{}},500)).value;
    world.step(); auto joint=world.create_joint(DistanceJointDesc{.a=a,.b=b,.length=2}).value;
    world.apply_impulse(b,{100,0,0},{1,5,0}); world.step();
    NINHO_REQUIRE(!world.joint_reactions().empty());
    NINHO_REQUIRE(is_finite(world.joint_reactions().front().force));
    NINHO_REQUIRE(world.destroy_joint(joint).ok());
    NINHO_REQUIRE(world.destroy_joint(joint).code==StatusCode::InvalidHandle);
}
NINHO_TEST("weld joint keeps loaded bodies together") {
    PhysicsWorld world(WorldConfig{.surface_gravity=0});
    auto a=world.create_body(BodyDesc::dynamic_box({0.5f,0.5f,0.5f},{{0,5,0},{}},500)).value;
    auto b=world.create_body(BodyDesc::dynamic_box({0.5f,0.5f,0.5f},{{0,6,0},{}},500)).value;
    world.step(); auto joint=world.create_joint(WeldJointDesc{.a=a,.b=b}).value;
    for(int i=0;i<120;++i){ world.apply_force(b,{500,0,0},{0,6,0}); world.step(); }
    auto reaction=world.joint_reaction(joint);
    NINHO_REQUIRE(reaction.has_value());
    NINHO_REQUIRE(std::abs(reaction->linear_separation)<0.02f);
}
NINHO_TEST("hit events are copied before mutation") {
    PhysicsWorld world(WorldConfig{.surface_gravity=0});
    auto wall=world.create_body(BodyDesc::static_box({2,0.5f,2},{{0,5,0},{}})).value;
    auto ballDesc=BodyDesc::dynamic_sphere(0.4f,{{0,10,0},{}},520); ballDesc.linear_velocity={0,-20,0}; ballDesc.bullet=true;
    world.create_body(ballDesc);
    for(int i=0;i<30 && world.contact_hits().empty();++i) world.step();
    NINHO_REQUIRE(!world.contact_hits().empty());
    const ContactHit retained=world.contact_hits().front();
    world.destroy_body(wall); world.step();
    NINHO_REQUIRE(retained.approach_speed>0.0f);
    NINHO_REQUIRE(is_finite(retained.point));
}
NINHO_TEST("invalid hull is rejected before Box3D") {
    PhysicsWorld world(WorldConfig{});
    BodyDesc body{.type=BodyType::Dynamic};
    body.shapes={ShapeDesc{.geometry=HullShape{{{0,0,0},{1,0,0},{0,1,0},{0,0,0}},{}},.density=500}};
    const auto result=world.create_body(body);
    NINHO_REQUIRE(result.status.code==StatusCode::InvalidArgument);
}
```

Add three matrix-specific tests in the same file:

```cpp
NINHO_TEST("three meter shape cast hits transformed hull") {
    PhysicsWorld world(WorldConfig{.surface_gravity=0});
    BodyDesc target{.type=BodyType::Static,.transform={{0,0,0},{}}};
    target.shapes={ShapeDesc{.geometry=HullShape{{{-0.5f,-0.5f,-0.5f},{0.5f,-0.5f,-0.5f},{0,0.5f,-0.5f},{0,0,0.5f}},{}},.density=1}};
    const auto handle=world.create_body(target).value; world.step();
    const SphereShape query{.radius=0.1f};
    const Vec3 origin{0,2.5f,0}, translation{0,-3,0};
    auto hit=world.cast_shape(query,{origin,{}},translation);
    NINHO_REQUIRE(hit.has_value()); NINHO_REQUIRE(hit->body==handle);
    const Vec3 inside=origin+translation*std::min(1.0f,hit->fraction+0.05f);
    const auto overlaps=world.overlap_shape(query,{inside,{}});
    NINHO_REQUIRE(std::ranges::any_of(overlaps,[&](const QueryHit& value){ return value.body==handle; }));
}
NINHO_TEST("eight hull compound reports mass bounds and contact") {
    PhysicsWorld world(WorldConfig{.surface_gravity=0});
    BodyDesc compound{.type=BodyType::Dynamic,.transform={{0,3,0},{}}};
    for(int i=0;i<8;++i) compound.shapes.push_back(ShapeDesc{.geometry=BoxShape{{0.25f,0.25f,0.25f},{{(i-3.5f)*0.5f,0,0},{}}},.density=500});
    const auto body=world.create_body(compound).value;
    world.create_body(BodyDesc::static_box({4,0.25f,4},{{0,0,0},{}})); world.step();
    auto bounds=world.body_bounds(body); NINHO_REQUIRE(bounds.has_value());
    NINHO_REQUIRE(bounds->upper.x-bounds->lower.x>3.5f);
    NINHO_REQUIRE(world.state(body)->mass>0.0f);
    for(int i=0;i<180 && world.contact_hits().empty();++i){ world.apply_force(body,{0,-4000,0},{0,3,0}); world.step(); }
    NINHO_REQUIRE(!world.contact_hits().empty());
}
NINHO_TEST("hit events deduplicate substeps and joint load crosses threshold") {
    PhysicsWorld world(WorldConfig{.substeps=6,.surface_gravity=0});
    auto a=world.create_body(BodyDesc::static_box({0.5f,0.5f,0.5f},{{0,0,0},{}})).value;
    auto b=world.create_body(BodyDesc::dynamic_box({0.5f,0.5f,0.5f},{{0,1,0},{}},500)).value; world.step();
    auto joint=world.create_joint(DistanceJointDesc{.a=a,.b=b,.length=1}).value;
    world.create_body(BodyDesc::static_box({0.25f,2,2},{{5,5,0},{}}));
    auto projectile=BodyDesc::dynamic_sphere(0.25f,{{0,5,0},{}},520); projectile.linear_velocity={20,0,0}; projectile.bullet=true;
    world.create_body(projectile);
    for(int i=0;i<30 && world.contact_hits().empty();++i) world.step();
    NINHO_REQUIRE(!world.contact_hits().empty());
    std::set<std::pair<BodyHandle,BodyHandle>> hit_pairs;
    for(const auto& hit:world.contact_hits()) {
        NINHO_REQUIRE(hit_pairs.insert(std::minmax(hit.a,hit.b)).second);
        NINHO_REQUIRE(hit.approach_speed>0.0f);
        NINHO_REQUIRE(hit.effective_mass>0.0f);
        NINHO_REQUIRE(hit.derived_energy>0.0f);
    }
    float previous=0;
    for(int load=1000;load<=12000;load+=1000){
        world.apply_force(b,{float(load),0,0},{0,1,0}); world.step();
        const float current=length(world.joint_reaction(joint)->force);
        NINHO_REQUIRE(current+50.0f>=previous); previous=current;
    }
    NINHO_REQUIRE(previous>10000.0f);
}
```

Expose `Aabb { Vec3 lower{},upper{}; }` and `body_bounds(BodyHandle)` for the fixture. If controlled joint force is not monotonic within the fixed `50 N` tolerance, the force/torque matrix row is blocked; do not widen it after the run.

Use real bodies and no mocks. Assert query ordering, finite normals/fractions, material IDs, force/torque, and public handles.

- [ ] **Step 2: Build and verify RED**

```powershell
.\tools\Invoke-Native.ps1 -Command 'cmake --build --preset debug --target ninho_physics_tests'
```

Expected: compile failure for missing query/joint API.

- [ ] **Step 3: Add exact public types and adapter behavior**

Add:

```cpp
struct JointHandle { std::uint32_t index{},generation{}; bool valid() const; auto operator<=>(const JointHandle&) const = default; };
struct DistanceJointDesc { BodyHandle a{},b{}; Transform frame_a{},frame_b{}; float length{1}; float hertz{4}; float damping_ratio{1}; bool collide_connected{}; };
struct WeldJointDesc { BodyHandle a{},b{}; Transform frame_a{},frame_b{}; float hertz{8}; float damping_ratio{1}; bool collide_connected{}; };
using JointDesc=std::variant<DistanceJointDesc,WeldJointDesc>;
struct QueryHit { BodyHandle body{}; Vec3 point{},normal{}; float fraction{}; std::uint64_t material_id{}; };
struct ContactHit { BodyHandle a{},b{}; Vec3 point{},normal{}; float approach_speed{},effective_mass{},derived_energy{}; std::uint64_t material_a{},material_b{}; };
struct JointReaction { JointHandle joint{}; Vec3 force{},torque{}; float linear_separation{},angular_separation{}; };
struct WorldMetrics { int body_count{},shape_count{},joint_count{},contact_count{},awake_count{}; double step_ms{}; };
using QueryShape = PrimitiveShape;
```

Add generic methods `cast_shape(const QueryShape&,Transform,Vec3)` and `overlap_shape(const QueryShape&,Transform)`, plus sphere convenience wrappers used by tests; also add `create_joint`, `destroy_joint`, `contact_hits`, `joint_reactions`, `joint_reaction`, and `metrics`. Map shapes to parent public handles when created. Query callbacks copy hits into context vectors; return cast fractions so Box3D clips to nearest hit. Sort overlap hits by handle; cast hits by quantized fraction then handle. Copy contact arrays immediately after `b3World_Step`, deduplicating each ordered body pair by greatest `approach_speed` for that tick. At copy time compute `effective_mass=mA*mB/(mA+mB)` for two dynamic bodies or the dynamic mass against static, then `derived_energy=0.5*effective_mass*max(0,approach_speed-1)^2`; no nonexistent Box3D impulse field is assumed. Read `b3Joint_GetConstraintForce/Torque` only for live joint slots. Hull creation uses `b3CreateHull` and `b3CreateTransformedHullShape`; reject null results and destroy the temporary source hull only after Box3D has copied it.

- [ ] **Step 4: Verify GREEN, all tests, and invalid-handle checks**

```powershell
.\tools\Invoke-Native.ps1 -Command 'cmake --build --preset debug; ctest --preset debug --output-on-failure -R capability; ctest --preset debug --output-on-failure'
```

Expected: all capability and earlier tests pass; retained events remain readable after mutation; no Box3D assertion.

- [ ] **Step 5: Commit**

```powershell
git add native/kernel native/tests
git commit -m "feat: expor capacidades fisicas do kernel"
```

---

### Task 5: Risk Scenarios, Deterministic Hashes, and JSON Reports

**Files:**
- Create: `native/kernel/include/ninho/physics/scenario.hpp`
- Create: `native/kernel/src/scenario.cpp`
- Create: `native/spike/main.cpp`
- Create: `native/spike/CMakeLists.txt`
- Create: `native/tests/projectile_ccd_tests.cpp`
- Create: `native/tests/pile_stability_tests.cpp`
- Create: `native/tests/determinism_tests.cpp`
- Modify: `native/tests/CMakeLists.txt`
- Modify: `native/kernel/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Create: `tools/run_spike.ps1`

**Interfaces:**
- Consumes: only the public `PhysicsWorld` API from Tasks 3–4.
- Produces: `ScenarioRunner::run(ScenarioKind, seed, substeps) -> ScenarioResult`; `ninho_physics_spike.exe --all --repeat 2 --json <path>`; valid UTF-8 report JSON.

- [ ] **Step 1: Write failing scenario and determinism tests**

Tests instantiate the same `ScenarioRunner` used by the executable:

```cpp
NINHO_TEST("bullet contacts pile in all fixed seeds") {
    for (std::uint64_t seed=1; seed<=20; ++seed) {
        auto result=ScenarioRunner{}.run(ScenarioKind::ProjectilePile,seed,4);
        NINHO_REQUIRE(result.contact_before_pile_exit);
        NINHO_REQUIRE(result.violations.empty());
    }
}
NINHO_TEST("same scenario has exact canonical hash") {
    auto a=ScenarioRunner{}.run(ScenarioKind::MassRatio,42,4);
    auto b=ScenarioRunner{}.run(ScenarioKind::MassRatio,42,4);
    NINHO_REQUIRE(a.final_hash==b.final_hash);
}
NINHO_TEST("radial pile meets predetermined sleep limits") {
    auto r=ScenarioRunner{}.run(ScenarioKind::RadialPile,7,4);
    NINHO_REQUIRE(r.p95_linear_speed < 0.05);
    NINHO_REQUIRE(r.p95_angular_speed < 0.10);
    NINHO_REQUIRE(r.sleep_ratio >= 0.90);
    NINHO_REQUIRE(r.max_penetration < 0.02);
}
```

Also test JSON escaping/parsing by running the spike to a temp file and invoking Python `json.load` through CTest.

- [ ] **Step 2: Build and verify RED**

Expected compile failure for missing `scenario.hpp`:

```powershell
.\tools\Invoke-Native.ps1 -Command 'cmake --build --preset debug --target ninho_physics_tests'
```

- [ ] **Step 3: Implement all six scenarios and canonical hashing**

Define `ScenarioKind { RadialFall, ProjectilePile, RadialPile, MassRatio, Stress, CapabilityMatrix }` and a `ScenarioResult` carrying every metric/tolerance from foundation spec §6 plus `to_json()`.

Canonical hash algorithm:

```cpp
std::uint64_t hash_states(std::span<const BodyState> states) {
    std::vector<BodyState> ordered(states.begin(),states.end());
    std::ranges::sort(ordered,{},&BodyState::handle);
    std::uint64_t hash=14695981039346656037ull;
    auto mix=[&](std::int64_t value){
        for(int i=0;i<8;++i){ hash^=std::uint8_t(std::uint64_t(value)>>(i*8)); hash*=1099511628211ull; }
    };
    for(const auto& s:ordered){
        mix(s.handle.index); mix(s.handle.generation);
        mix(std::llround(s.transform.position.x*1000));
        mix(std::llround(s.transform.position.y*1000));
        mix(std::llround(s.transform.position.z*1000));
        mix(std::llround(s.transform.rotation.x*32767));
        mix(std::llround(s.transform.rotation.y*32767));
        mix(std::llround(s.transform.rotation.z*32767));
        mix(std::llround(s.transform.rotation.w*32767));
        mix(std::llround(s.linear_velocity.x*1000));
        mix(std::llround(s.linear_velocity.y*1000));
        mix(std::llround(s.linear_velocity.z*1000));
        mix(std::llround(s.angular_velocity.x*1000));
        mix(std::llround(s.angular_velocity.y*1000));
        mix(std::llround(s.angular_velocity.z*1000));
        mix(s.awake); mix(s.ejected);
    }
    return hash;
}
```

Use deterministic xorshift64 seed generation, stable handle creation order, warm-up/measurement tick counts from the spec, `std::chrono::steady_clock` for timing only, and numeric violations fixed before the run. `Stress` pre-touches fixed buffers, performs ten complete allocator warm-up cycles followed by ten distinct measured create/simulate/destroy cycles, and samples only after complete teardown. A private helper is the only new direct Box3 caller and exposes `b3GetByteCount()`; process/scenario baseline must be zero and all 10+10 post-teardown samples must return exactly to it. In Debug `/MTd`, checkpoint one prewarmed full cycle with `_CrtMemCheckpoint` and require zero `_NORMAL_BLOCK`/`_CLIENT_BLOCK` count+bytes delta. One `GetProcessMemoryInfo` call records `PrivateUsage`, `WorkingSetSize`, and `PeakWorkingSetSize`; preserve full/central min/max, median, growth, instant growth, trimmed/full spans, stability, terminal guard and assessment for both counters in both builds, but mark both permanently diagnostic/unqualified in this foundation. Se um contador estiver indisponível, serialize raws vazios/coerentes e assessment `unavailable` sem falhar CTest/exit. Validate Release `/MD` as a global configuration mismatch before dispatching any scenario through an internal pure evaluator; do not expose a configurable execution bypass on `ScenarioRunner`. Emit `private_commit_budget_unqualified`, defer the 5% gate to packaged Godot Release on reference hardware, and recommend `prosseguir_com_limites` when normative gates pass. Serialize Private/WS/Box3/CRT for both repeats. Every capability row stores `functional_status`/`functional_fallback` before allocator/CRT/tooling checks; timing, footprint, allocator, CRT, warnings and gate status stay outside the canonical hash.

`CapabilityMatrix` additionally performs 10,000 body/handle create-destroy cycles, exercises the public sphere cast/overlap and joint reaction paths, and calls one private conformance helper that records and validates a minimal Box3D replay from the same pinned build. Direct `b3*` use is allowed only inside that helper under `native/kernel/src`; all gameplay assertions still pass through `PhysicsWorld`. Link `Psapi.lib` on Windows for working-set sampling.

Projectile gating is algorithmic: run each seed at 35 m/s and four substeps; if any seed misses, rerun the complete 20-seed set at 30 m/s and six substeps. Record `fallback="speed30_substeps6"` only when that second set passes 20/20. If both fail, add a fatal `ccd_dynamic_dynamic` violation. No other speed/substep combination is attempted. Shape-cast, joint, compound, sleep, lifecycle, and replay rows similarly serialize `pass|fallback|blocked` and the exact measured values from their Task 4 fixtures.

The shape-query row passes only when the 3 m cast's first handle and the overlap-at-contact fixture agree; the documented multi-ray fallback is recorded as preview-only and does not turn the runtime row green. The contact row passes only with at least one real hit, finite positive `approach_speed`, `effective_mass`, `derived_energy`, finite normal/material IDs, and unique ordered body pairs after six substeps. If raw hit events are unusable, the only allowed fallback computes relative normal speed from copied body velocities for begin-contact pairs, applies the same effective-mass/energy equation, and records `fallback="derived_relative_energy"`; if energy or pair deduplication still fails, the row is blocked.

`main.cpp` parses `--scenario`, `--all`, `--seed`, `--substeps`, `--repeat`, and `--json`; invalid options return 2, violations return 1, success returns 0. JSON includes tool/dependency commits, build type, CPU, scenario values, p50/p95/max step, process/scenario Box3 allocator proof, final hashes, violations, and `repeat_observations[]` containing each repeat index, hash, Private/Working Set, Box3 and CRT. The Python smoke recomputes todos os derivados de PrivateUsage e Working Set a partir dos raws dos dois repeats em Debug e Release, verifica o escopo diagnóstico, retorno Box3 exato e deltas CRT Debug.

Create `tools/run_spike.ps1` to build Release, create `artifacts/physics`, run all scenarios twice, validate JSON with `python -m json.tool`, and propagate nonzero exit.

- [ ] **Step 4: Verify GREEN in Debug and Release**

```powershell
.\tools\Invoke-Native.ps1 -Command 'cmake --build --preset debug; ctest --preset debug --output-on-failure'
.\tools\run_spike.ps1 -Configuration Release
```

Expected: native suite passes; scenario JSON parses; repeated hashes match per configuration; any Box3D capability failure is reported with the exact matrix line and approved fallback, never silently relaxed.

- [ ] **Step 5: Commit**

```powershell
git add native/kernel native/spike native/tests CMakeLists.txt tools/run_spike.ps1
git commit -m "test: validar cenarios de risco do Box3D"
```

---

### Task 6: Godot GDExtension Adapter

**Files:**
- Create: `native/extension/include/ninho/extension/box3d_world_node.hpp`
- Create: `native/extension/src/box3d_world_node.cpp`
- Create: `native/extension/src/register_types.cpp`
- Create: `native/extension/CMakeLists.txt`
- Modify: `cmake/Dependencies.cmake`
- Modify: `CMakeLists.txt`
- Create: `game/bin/ninho_physics.gdextension`

**Interfaces:**
- Consumes: `PhysicsWorld`; godot-cpp pinned commit.
- Produces: Godot class `Box3DWorldNode` with `configure_planet`, `spawn_box`, `spawn_projectile`, `apply_impulse`, `step_fixed`, `get_body_states`, `get_metrics`, `reset_world`, and signal `physics_fault(code,message)`.

- [ ] **Step 1: Add a failing link/load probe**

Create an initial CTest `gdextension_binary_exists` that expects the DLL named by `game/bin/ninho_physics.gdextension`. Run the Debug build before adding the target.

Expected: failure because the DLL does not exist.

- [ ] **Step 2: Add pinned godot-cpp and extension target**

Append to `Dependencies.cmake` only when `NINHO_BUILD_GDEXTENSION` is ON:

```cmake
FetchContent_Declare(godot_cpp
  GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
  GIT_TAG e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77
  GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(godot_cpp)
```

The extension CMake creates a SHARED library, links `ninho_physics_kernel` and `godot::cpp`, reads the suffix from the actual target, and writes the DLL directly to `game/bin`:

```cmake
add_library(ninho_physics_extension SHARED
  src/box3d_world_node.cpp src/register_types.cpp)
target_include_directories(ninho_physics_extension PRIVATE include)
target_link_libraries(ninho_physics_extension PRIVATE ninho::physics godot::cpp)
get_target_property(GODOTCPP_SUFFIX godot-cpp GODOTCPP_SUFFIX)
if(NOT GODOTCPP_SUFFIX)
  message(FATAL_ERROR "godot-cpp did not expose GODOTCPP_SUFFIX")
endif()
set_target_properties(ninho_physics_extension PROPERTIES
  PREFIX ""
  OUTPUT_NAME "ninho_physics${GODOTCPP_SUFFIX}"
  RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/game/bin>")
```

Use MSVC `/EHsc` consistently and build godot-cpp with exceptions enabled, as
approved by the foundation architecture. Exceptions remain an internal C++
mechanism: the GDExtension boundary catches and translates every failure to
`physics_fault` plus a safe 0/false/empty return value.

- [ ] **Step 3: Implement `Box3DWorldNode` and registration**

The header derives from `godot::Node3D`, owns `std::unique_ptr<PhysicsWorld>`, and stores accumulator/configuration. `_bind_methods()` binds the eight methods and signal. Conversion rules:

- Godot `Vector3` ↔ kernel `Vec3` component-for-component;
- Godot `Quaternion(x,y,z,w)` ↔ kernel `Quat{x,y,z,w}`;
- handles cross the boundary as packed `int64_t = generation<<32 | index`;
- snapshots return `Array<Dictionary>` with `handle`, `position`, `rotation`, `linear_velocity`, `angular_velocity`, `mass`, `awake`, `ejected`;
- all errors emit `physics_fault` and return 0/false/empty without exposing Box3D.

`_physics_process(delta)` clamps accumulator to 0.1 s, runs at most four fixed ticks, and never passes variable delta to the kernel. `step_fixed()` performs one tick for headless tests. `reset_world()` destroys/recreates the kernel.

Create `register_types.cpp` with exported entry symbol `ninho_physics_library_init`, Scene initialization level, `GDREGISTER_CLASS(Box3DWorldNode)`, and matching terminator.

Create `game/bin/ninho_physics.gdextension`:

```ini
[configuration]
entry_symbol = "ninho_physics_library_init"
compatibility_minimum = "4.5"
reloadable = true

[libraries]
windows.debug.x86_64 = "res://bin/ninho_physics.windows.template_debug.x86_64.dll"
windows.release.x86_64 = "res://bin/ninho_physics.windows.template_release.x86_64.dll"
```

- [ ] **Step 4: Build and verify DLL naming/export symbol**

```powershell
.\tools\Invoke-Native.ps1 -Command 'cmake --preset debug -DNINHO_BUILD_GDEXTENSION=ON; cmake --build --preset debug --target ninho_physics_extension; ctest --preset debug --output-on-failure -R gdextension'
```

Expected: the exact Debug DLL exists under `game/bin`; `dumpbin /exports` lists `ninho_physics_library_init`; CTest passes.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt cmake native/extension game/bin/ninho_physics.gdextension
git commit -m "feat: integrar Box3D ao Godot"
```

---

### Task 7: Godot Headless Smoke and Visual Spike Scene

**Files:**
- Create: `game/project.godot`
- Create: `game/scripts/physics_spike_smoke.gd`
- Create: `game/scripts/physics_spike_view.gd`
- Create: `game/scenes/physics_spike.tscn`
- Create: `tools/test.ps1`

**Interfaces:**
- Consumes: `Box3DWorldNode` from Task 6.
- Produces: headless exit-code smoke; graphical planet/pile/projectile visualization with meshes synchronized only from Box3D.

- [ ] **Step 1: Write the failing headless smoke script**

`physics_spike_smoke.gd` extends `SceneTree`; in `_initialize`, assert `ClassDB.class_exists("Box3DWorldNode")`, instantiate it, configure radius 10/gravity 9, spawn a static planet plus dynamic ball at `(0,15,0)`, step 120 ticks, assert final radius is less than 15 and state count is 2, call reset, assert states empty, then `quit(0)`. Any failure prints `push_error` and `quit(1)`.

Run before the extension/project is complete:

```powershell
.\.tools\godot\Godot_v4.5.1-stable_win64.exe --headless --path game --script res://scripts/physics_spike_smoke.gd
```

Expected: nonzero exit due to missing project/script/class.

- [ ] **Step 2: Create the minimal Godot project and smoke path**

`project.godot` sets main scene, Vulkan Mobile renderer with GL compatibility fallback, 1280×720 viewport, physics tick 60, and no Godot 3D physics nodes. `tools/test.ps1` builds selected configuration, runs CTest, resolves the pinned Godot path, executes the smoke script with `--headless`, saves stdout/stderr under `artifacts/physics`, and returns the first failing exit code.

Run and expect headless PASS with no `ERROR:` or `SCRIPT ERROR:` in the captured log.

- [ ] **Step 3: Create the visual scene and batched synchronization**

`physics_spike.tscn` contains only `Node3D`, `Box3DWorldNode`, `Camera3D`, `DirectionalLight3D`, `WorldEnvironment`, `MultiMeshInstance3D`/mesh views, and UI labels. It contains no class name matching `*Body3D` or `CollisionShape3D`.

`physics_spike_view.gd`:

- configures the planet and creates 120 boxes in stable nested-loop order;
- spawns one projectile at 35 m/s with the kernel bullet flag;
- maintains `Dictionary<int,MeshInstance3D>` keyed by packed handle;
- reads one batched `get_body_states()` per physics frame and updates transforms;
- deletes views missing from the latest snapshot;
- displays bodies, contacts, awake count, and step p95;
- exits cleanly after 300 frames when `--write-movie` is active.

Use this synchronization shape; helper `_add_box_view(handle,size,color)` creates and stores a `MeshInstance3D` with `BoxMesh` and `StandardMaterial3D`:

```gdscript
extends Node3D
@onready var physics: Box3DWorldNode = $Box3DWorldNode
var views: Dictionary = {}
var frames := 0

func _ready() -> void:
    physics.configure_planet(10.0, 9.0)
    for layer in range(10):
        for column in range(12):
            var position := Vector3((column - 5.5) * 0.7, 10.55 + layer * 0.55, 0.0)
            var handle: int = physics.spawn_box(Vector3(0.32, 0.25, 0.32), Transform3D(Basis.IDENTITY, position), 520.0)
            _add_box_view(handle, Vector3(0.64, 0.5, 0.64), Color("d58b45"))
    var projectile := physics.spawn_projectile(0.45, Transform3D(Basis.IDENTITY, Vector3(-8, 13, 0)), Vector3(35, 0, 0))
    _add_sphere_view(projectile, 0.45, Color("5fe1d2"))

func _physics_process(_delta: float) -> void:
    var alive := {}
    for state: Dictionary in physics.get_body_states():
        var handle: int = state.handle
        alive[handle] = true
        if views.has(handle):
            views[handle].transform = Transform3D(Basis(state.rotation), state.position)
    for handle in views.keys():
        if not alive.has(handle):
            views[handle].queue_free()
            views.erase(handle)
    frames += 1
```

- [ ] **Step 4: Verify headless, structural scene rules, and graphical capture**

```powershell
.\tools\test.ps1 -Configuration Debug
rg -n "RigidBody3D|StaticBody3D|CharacterBody3D|CollisionShape3D" game
.\.tools\godot\Godot_v4.5.1-stable_win64.exe --path game --write-movie artifacts/physics/spike.avi --fixed-fps 60 --quit-after 300
```

Expected: full tests pass; `rg` returns no matches; Godot writes a nonempty movie and logs no engine/script error. Inspect at least frames near launch, contact, and settled pile; record any rendering-only defect without changing physics gates.

- [ ] **Step 5: Commit**

```powershell
git add game/project.godot game/scenes game/scripts tools/test.ps1
git commit -m "feat: adicionar spike visual no Godot"
```

---

### Task 8: Upstream Tests, Licenses, Final Report, and Foundation Gate

**Files:**
- Modify: `tools/test.ps1`
- Create: `docs/physics/box3d-spike-report.md`
- Create: `THIRD_PARTY_NOTICES.md`
- Create: `third_party/box3d.LICENSE.txt`
- Create: `third_party/godot.LICENSE.txt`
- Create: `third_party/godot-cpp.LICENSE.txt`
- Create: `third_party/sbom.spdx.json`

**Interfaces:**
- Consumes: every artifact/test from Tasks 1–7 and Box3D source populated by FetchContent.
- Produces: one-command verification, upstream Box3D test evidence, notices/SBOM, and `prosseguir|prosseguir com limites|bloquear` recommendation.

- [ ] **Step 1: Add a failing report completeness test**

Extend `tools/test.ps1` to require that the report contains headings `Versions`, `Capability Matrix`, `Scenario Metrics`, `Godot Smoke`, `Known Limits`, and `Recommendation`, and that recommendation is one of the three allowed values. Run it before creating the report.

Expected: failure `box3d-spike-report.md missing`.

- [ ] **Step 2: Add upstream Box3D configure/test to the verification script**

After the project configure populates `.fetchcontent-cache`, locate the exact `box3d-src` directory by verifying its `git rev-parse HEAD`. Configure it as a separate top-level project in `build/upstream-box3d/<configuration>` with samples/benchmarks/docs OFF, unit tests ON, same MSVC/SDK/CRT, then run its CTest. Abort if commit differs.

- [ ] **Step 3: Generate legal inventory and report from fresh evidence**

Copy exact MIT license texts from the pinned sources/Godot release into the three named files. Create SPDX 2.3 JSON listing project, Box3D, Godot, godot-cpp, CMake, Ninja, and Visual Studio Build Tools with versions, download locations, checksums where distributed, and `relationship: DEPENDS_ON`.

Populate the report only from fresh Debug/Release outputs:

- versions/commits/toolchain paths;
- every go/no-go row with measured result and fallback used;
- p50/p95/max, awake/contact/body/shape/joint peaks, hashes, Box3 exact-return evidence, Debug CRT live-block deltas, diagnostic Private/Working Set 10+10 samples and unqualified budget warning for every repeat;
- upstream test counts;
- Godot headless log and graphical capture path;
- known alpha limits and reproducible artifact paths;
- recommendation. Use `bloquear` if any normative violation exists; otherwise require `prosseguir_com_limites` because the private-commit budget is deferred to packaged reference-hardware qualification.

- [ ] **Step 4: Run the complete fresh gate**

```powershell
.\tools\bootstrap.ps1 -CheckOnly -Json
.\tools\test.ps1 -Configuration Debug -IncludeUpstream
.\tools\test.ps1 -Configuration Release -IncludeUpstream
.\tools\run_spike.ps1 -Configuration Release
git diff --check
git status --short
```

Expected: bootstrap `ok`; project and upstream suites have zero failures in both configurations; headless Godot exits 0 without error; scenario runner exits 0 and repeats hashes; `git diff --check` is clean. If evidence disagrees, the report must say `bloquear` or `prosseguir com limites`; do not convert a failure into a passing criterion.

- [ ] **Step 5: Independent code/spec review**

Dispatch one reviewer for spec compliance and one for code quality. Give each the foundation spec, this plan, `git diff caca102..HEAD`, test logs, and scenario JSON. Resolve every blocking/high finding with a failing regression test before changing production code, then rerun Step 4.

- [ ] **Step 6: Commit**

```powershell
git add tools/test.ps1 docs/physics THIRD_PARTY_NOTICES.md third_party
git commit -m "docs: registrar gate da fundacao Box3D"
```

## Plan Self-Review Checklist

- [x] Every foundation-spec file and acceptance criterion maps to a task above.
- [x] Tests exercise the public kernel rather than direct `b3*` calls.
- [x] Public signatures and packed-handle format are identical across Tasks 3, 4, 6, and 7.
- [x] Debug/Release use separate Ninja directories and matching godot-cpp targets.
- [x] No task weakens numeric gates after observing results.
- [x] Placeholder scan and `git diff --check` are clean.

## Autonomous Execution Choice

Use **Subagent-Driven Development**. The user explicitly requested autonomous development and specialized agents. Dispatch a fresh implementation agent per task, followed by spec-compliance and code-quality reviewers, while the root agent verifies every diff and full-suite gate before accepting the task.
