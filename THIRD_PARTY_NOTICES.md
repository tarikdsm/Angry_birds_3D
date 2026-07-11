# Third-Party Notices

This inventory covers the pinned physics foundation and its reproducible Windows build toolchain. The `.tools` directory and FetchContent source caches are local build inputs and are not committed to or redistributed with this repository.

## Runtime and linked dependencies

### Box3D 3D v0.1.0

- Upstream: https://github.com/erincatto/box3d
- Source commit: `8441b4a06d6d09dcfb0b0f704df4d847d1437b92`
- License: MIT
- Exact pinned license: [`third_party/box3d.LICENSE.txt`](third_party/box3d.LICENSE.txt)

Box3D is fetched from the immutable commit above and linked statically. The source checkout is not modified.

### Godot Engine 4.5.1-stable

- Upstream: https://github.com/godotengine/godot
- Release: https://github.com/godotengine/godot/releases/tag/4.5.1-stable
- Source commit: `f62fdbde15035c5576dad93e586201f4d41ef0cb`
- Windows x86_64 archive SHA-256: `defccc78669e644861b4247626b01ae362cd9f23975edf19c8bfd2eb1f6a1783`
- Executable SHA-256: `a829242096d640007de9fa93ea923d0f666d13d6a3b472a0231c820be6dc7627`
- License: MIT; Godot distributions also include notices for bundled third-party components
- Exact pinned project license: [`third_party/godot.LICENSE.txt`](third_party/godot.LICENSE.txt)

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

### Microsoft Visual Studio Build Tools 2026 18.7.3 (build 11925.98)

- Installer: https://download.visualstudio.microsoft.com/download/pr/4037ccca-d103-412b-a678-bc0aa164315e/07b09afd416dc05c781f171c881c23e42907eeb8d812fa1d2993dffb9323c869/vs_BuildTools.exe
- Installer SHA-256: `07b09afd416dc05c781f171c881c23e42907eeb8d812fa1d2993dffb9323c869`
- Pinned components: `Microsoft.VisualStudio.Component.VC.14.44.17.14.x86.x64`, `Microsoft.VisualStudio.Component.Windows11SDK.26100`
- License: governed by the Microsoft terms presented by the official installer. No Microsoft binaries or license terms are redistributed by this repository; the SPDX declaration is therefore `NOASSERTION`.

The SPDX 2.3 inventory is [`third_party/sbom.spdx.json`](third_party/sbom.spdx.json). The Ninho Orbital project itself has no declared repository license at this milestone, so its SPDX license fields are intentionally `NOASSERTION`.
