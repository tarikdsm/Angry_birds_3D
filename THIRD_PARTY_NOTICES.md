# Third-Party Notices

This inventory covers the pinned physics foundation and its reproducible Windows build toolchain. The `.tools` directory and FetchContent source caches are local build inputs and are not committed to or redistributed with this repository.

## Project licensing status

Ninho Orbital has no declared project license at this milestone, so its SPDX
`licenseDeclared` and `licenseConcluded` fields remain `NOASSERTION`.
Third-party license texts and notices in this file and under `third_party/`
apply only to the corresponding third-party components; they do not declare a
license for project-authored code, assets, or other materials.

Public or commercial publication remains blocked until the owner explicitly
selects and records a project license or other approved distribution terms.
Generating a package for technical verification does not satisfy that gate.

## Runtime and linked dependencies

### Box3D 3D v0.1.0

- Upstream: https://github.com/erincatto/box3d
- Source commit: `8441b4a06d6d09dcfb0b0f704df4d847d1437b92`
- License: MIT
- Exact pinned license: [`third_party/box3d.LICENSE.txt`](third_party/box3d.LICENSE.txt)

Box3D is fetched from the immutable commit above and linked statically. The source checkout is not modified.

### nlohmann/json v3.11.3

- Upstream: https://github.com/nlohmann/json
- Source commit: `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03`
- Source archive SHA-256: `0dbc5e40a01ff142e7e68c03e85247a4dcede2f592d12d3677dee3664d17975a`
- License: MIT
- Exact pinned license: [`third_party/nlohmann-json.LICENSE.txt`](third_party/nlohmann-json.LICENSE.txt)

nlohmann/json is fetched from the immutable commit archive above and used as a header-only dependency. The source is not modified.

### Godot Engine 4.5.1-stable

- Upstream: https://github.com/godotengine/godot
- Release: https://github.com/godotengine/godot/releases/tag/4.5.1-stable
- Source commit: `f62fdbde15035c5576dad93e586201f4d41ef0cb`
- Windows x86_64 archive SHA-256: `defccc78669e644861b4247626b01ae362cd9f23975edf19c8bfd2eb1f6a1783`
- Executable SHA-256: `a829242096d640007de9fa93ea923d0f666d13d6a3b472a0231c820be6dc7627`
- License: MIT; Godot distributions also include notices for bundled third-party components
- Exact pinned project license: [`third_party/godot.LICENSE.txt`](third_party/godot.LICENSE.txt)
- Exact upstream copyright and bundled third-party license inventory: [`third_party/godot.COPYRIGHT.txt`](third_party/godot.COPYRIGHT.txt), pinned from source commit `f62fdbde15035c5576dad93e586201f4d41ef0cb` with SHA-256 `2039020f520ebd55592070ede2ef38dfbc28a6550140008da004143872789e5d`

### Godot Export Templates 4.5.1-stable

- Upstream: https://github.com/godotengine/godot
- Archive: https://github.com/godotengine/godot/releases/download/4.5.1-stable/Godot_v4.5.1-stable_export_templates.tpz
- Archive SHA-256: `1998af37f1387684e2c211cdb483daf492fc64dc6b12096bddcdca25b6910c86`
- Installed template directory: `4.5.1.stable`
- License: MIT; exported Godot binaries also carry the notices for bundled third-party components
- Exact pinned project license: [`third_party/godot-export-templates.LICENSE.txt`](third_party/godot-export-templates.LICENSE.txt)
- Exact upstream copyright and bundled third-party license inventory: [`third_party/godot.COPYRIGHT.txt`](third_party/godot.COPYRIGHT.txt)

The official templates are verified before extraction and used to produce the
redistributable Windows executable. They are a build input; only the generated
Windows executable and applicable notices are included in the game package.
The packaged `godot.COPYRIGHT.txt` preserves the upstream inventory applicable
to the generated Godot runtime, including non-MIT bundled components.

### godot-cpp godot-4.5-stable

- Upstream: https://github.com/godotengine/godot-cpp
- Source commit: `e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77`
- License: MIT
- Exact pinned license: [`third_party/godot-cpp.LICENSE.txt`](third_party/godot-cpp.LICENSE.txt)

## Build-only tools

### CMake 4.3.3

- Upstream: https://github.com/Kitware/CMake
- Windows x86_64 archive: https://github.com/Kitware/CMake/releases/download/v4.3.3/cmake-4.3.3-windows-x86_64.zip
- Archive SHA-256: `935ade9e5e8723583c07f44c5592cea2a1c8f65c56ca7e07b34c025c880e0bd6`
- Executable SHA-256: `70fa92ce2ac9f54b0ae395b0b3790d9147ef2ebdbd7c4e0bb20852aac581baea`
- License: BSD-3-Clause; the exact distribution license is installed at `.tools/cmake/cmake-4.3.3-windows-x86_64/doc/cmake/LICENSE.rst`

### Ninja 1.13.2

- Upstream: https://github.com/ninja-build/ninja
- Windows archive: https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-win.zip
- Archive SHA-256: `07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65`
- Executable SHA-256: `e52a7ad9538d9618c67a0bd777964e2eec8a30f68b810a2f6adce1f2daf847b8`
- License: Apache-2.0; exact source text: https://github.com/ninja-build/ninja/blob/v1.13.2/COPYING

### Blender 5.1.2

- Windows x64 archive: https://download.blender.org/release/Blender5.1/blender-5.1.2-windows-x64.zip
- Archive SHA-256: `345bedea7b0acf7cc9666423d8553f9129622aea34ded65c23e8cb70f83f14ff`
- Executable SHA-256: `a7d09b04df8f78d432bc45d32c08f25387a78c76d12d2a6f5de07d8e1066e8f8`
- Declared license: GPL-3.0-or-later; the binary distribution contains dependencies under other licenses, so the SPDX concluded license is `NOASSERTION`
- Complete distribution license inventory: `.tools/blender/blender-5.1.2-windows-x64/license/license.md`

Blender generates the project-authored `.blend`, GLB and texture assets. The
tool and its bundled libraries are local build inputs and are not redistributed
with the repository or game package.

### FFmpeg 8.1.2 essentials build

- Upstream: https://ffmpeg.org/
- Windows build provider: https://www.gyan.dev/ffmpeg/builds/
- Windows x64 archive: https://github.com/GyanD/codexffmpeg/releases/download/8.1.2/ffmpeg-8.1.2-essentials_build.zip
- Archive SHA-256: `db580001caa24ac104c8cb856cd113a87b0a443f7bdf47d8c12b1d740584a2ec`
- `ffmpeg.exe` SHA-256: `1326dde4c84ff1f96fe6b8916c5bed29e163e9b5dccf995f6f3db069d143ec5e`
- `ffprobe.exe` SHA-256: `b49ccc7c6547b141ad5a2f6ec69cc04323d7133d7704d70b331b904c63eecb07`
- Declared license: GPL-3.0-or-later; the static distribution contains bundled libraries under additional compatible licenses, so the SPDX concluded license is `NOASSERTION`
- Complete distribution license and build inventory: `.tools/ffmpeg/ffmpeg-8.1.2-essentials_build/LICENSE`, `.tools/ffmpeg/ffmpeg-8.1.2-essentials_build/README.txt`

FFmpeg and FFprobe validate and transform local test captures. The tool and its
bundled libraries are local test/build inputs and are not redistributed with
the repository or game package.

### Python >= 3.11.0

- Command: `python`
- Source: environment-provided build tool
- Integrity and license: the toolchain lock requires version 3.11.0 or later but does not pin a Python distribution, download URL, checksum or license; the SPDX fields remain `NOASSERTION`

The selected Python interpreter runs repository build, generation and test
scripts. It is not downloaded or redistributed by this repository.

### Microsoft Visual Studio Build Tools 2026 18.7.3 (build 11925.98)

- Installer: https://download.visualstudio.microsoft.com/download/pr/4037ccca-d103-412b-a678-bc0aa164315e/07b09afd416dc05c781f171c881c23e42907eeb8d812fa1d2993dffb9323c869/vs_BuildTools.exe
- Installer SHA-256: `07b09afd416dc05c781f171c881c23e42907eeb8d812fa1d2993dffb9323c869`
- Pinned components: `Microsoft.VisualStudio.Component.VC.14.44.17.14.x86.x64`, `Microsoft.VisualStudio.Component.Windows11SDK.26100`
- License: governed by the Microsoft terms presented by the official installer. No Microsoft binaries or license terms are redistributed by this repository; the SPDX declaration is therefore `NOASSERTION`.

The SPDX 2.3 inventory is [`third_party/sbom.spdx.json`](third_party/sbom.spdx.json).
