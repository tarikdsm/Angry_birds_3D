# Box3D Foundation Spike Report

This normative report is generated only from the two tracked foundation snapshots. Run `python tools/generate_foundation_report.py --check` to detect drift.

Evidence-Debug-Path: docs/physics/evidence/foundation-report-debug.json
Evidence-Debug-SHA256: EE9659E93AF00CD032F724FBEF0129960372112DF7C1BDDBC976E26BC7D28368
Matrix-Debug-Hash: 17104053157009575930
Matrix-Debug-Topology: 123/123/1;121/221
Recommendation-Debug: prosseguir_com_limites
Evidence-Release-Path: docs/physics/evidence/foundation-report-release.json
Evidence-Release-SHA256: 8928F298B4B260829AED5F5118FE4B1726857FD13E7B323AE2A9E98B8F6AA803
Matrix-Release-Hash: 17104053157009575930
Matrix-Release-Topology: 123/123/1;121/221
Recommendation-Release: prosseguir_com_limites

## Versions

| Component | Exact version/pin |
| --- | --- |
| Box3D 3D | `0.1.0` / `8441b4a06d6d09dcfb0b0f704df4d847d1437b92` |
| Godot | `4.5.1-stable` / `f62fdbde15035c5576dad93e586201f4d41ef0cb` |
| godot-cpp | `godot-4.5-stable` / `e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77` |
| Visual Studio Build Tools | `18.7.3 (11925.98)` |
| MSVC tools / compiler | `14.44.35207 / 19.44.35228.0` |
| Windows SDK | `10.0.26100.0` |
| CMake / Ninja / Python | `4.3.3 / 1.13.2 / 3.11.9` |

Debug uses `/MTd`; Release uses `/MT`. Both reject `/MD[d]` and `/fp:fast`.

## Snapshot Metadata

This inventory covers every global field except the scenario and capability arrays, which are rendered in their dedicated sections.

### Debug snapshot metadata

| JSON pointer | Value |
| --- | --- |
| `/schema` | `"ninho.physics.scenario.v1"` |
| `/tool/name` | `"ninho_physics_spike"` |
| `/tool/version` | `"0.1.0"` |
| `/dependencies/box3d/version` | `"0.1.0"` |
| `/dependencies/box3d/commit` | `"8441b4a06d6d09dcfb0b0f704df4d847d1437b92"` |
| `/dependencies/godot/version` | `"4.5.1-stable"` |
| `/dependencies/godot/commit` | `"f62fdbde15035c5576dad93e586201f4d41ef0cb"` |
| `/dependencies/godot_cpp/version` | `"godot-4.5-stable"` |
| `/dependencies/godot_cpp/commit` | `"e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77"` |
| `/build_type` | `"Debug"` |
| `/cpu` | `"Intel64 Family 6 Model 141 Stepping 1, GenuineIntel"` |
| `/configuration/seed` | `1` |
| `/configuration/substeps` | `4` |
| `/configuration/repeat` | `2` |
| `/configuration/time_step` | `0.016666666666666666` |
| `/process_box3d_allocator/baseline_bytes` | `0` |
| `/process_box3d_allocator/final_bytes` | `0` |
| `/process_box3d_allocator/max_abs_delta` | `0` |
| `/process_box3d_allocator/exact_return` | `true` |
| `/process_box3d_allocator/warmup_post_teardown` | `[]` |
| `/process_box3d_allocator/measured_post_teardown` | `[]` |
| `/budget_qualification/status` | `"deferred"` |
| `/budget_qualification/target_growth_ratio` | `0.05` |
| `/budget_qualification/warning` | `"private_commit_budget_unqualified"` |
| `/warnings/0/code` | `"private_commit_budget_unqualified"` |
| `/warnings/0/message` | `"PrivateUsage budget is deferred to a packaged Release build on reference hardware"` |
| `/warnings/0/details/0/name` | `"budget_scope"` |
| `/warnings/0/details/0/value` | `"future_packaged_reference_hardware"` |
| `/violations` | `[]` |
| `/recommendation` | `"prosseguir_com_limites"` |
| `/source_revision` | `"2c7e3b5afa296df1d37b376eac6b41ffeffc4fcb"` |
| `/tested_inputs_schema` | `"ninho.tested-inputs.v2"` |
| `/tested_inputs_sha256` | `"7471846c39b1ad6da160f534a4760dbf0320372409d6d851f2301bd6c5097aba"` |
| `/tested_inputs/0/path` | `"CMakeLists.txt"` |
| `/tested_inputs/0/mode` | `"text_utf8_lf"` |
| `/tested_inputs/0/size_bytes` | `3834` |
| `/tested_inputs/0/sha256` | `"f39c15ec5bfa29494d00b2954c93e055babc32e74348bf39a431723b23b69b04"` |
| `/tested_inputs/1/path` | `"CMakePresets.json"` |
| `/tested_inputs/1/mode` | `"text_utf8_lf"` |
| `/tested_inputs/1/size_bytes` | `1120` |
| `/tested_inputs/1/sha256` | `"3316126b405902b73d22d6ea299a9da7c77adcdf09ef24da8c15404273bef1aa"` |
| `/tested_inputs/2/path` | `"cmake/Dependencies.cmake"` |
| `/tested_inputs/2/mode` | `"text_utf8_lf"` |
| `/tested_inputs/2/size_bytes` | `4063` |
| `/tested_inputs/2/sha256` | `"5c49175883c6658759d99b82458996ff6aadf6ce68e2feb9e65183a8f63aa4f8"` |
| `/tested_inputs/3/path` | `"cmake/PrepareGDExtensionTest.cmake"` |
| `/tested_inputs/3/mode` | `"text_utf8_lf"` |
| `/tested_inputs/3/size_bytes` | `261` |
| `/tested_inputs/3/sha256` | `"66d8783a8394ecd593da4d2d221196e00cc5e4bb377dab83c7862f651388a1b5"` |
| `/tested_inputs/4/path` | `"cmake/RequireGodotRuntimeTests.cmake"` |
| `/tested_inputs/4/mode` | `"text_utf8_lf"` |
| `/tested_inputs/4/size_bytes` | `573` |
| `/tested_inputs/4/sha256` | `"3b08148730ad5b1abb4f42b496b21bb5f238b4c66bdfbd606f19de8ac3e7feaa"` |
| `/tested_inputs/5/path` | `"cmake/VerifyGDExtension.cmake"` |
| `/tested_inputs/5/mode` | `"text_utf8_lf"` |
| `/tested_inputs/5/size_bytes` | `3752` |
| `/tested_inputs/5/sha256` | `"612f8a47cfd3c5a31d214560797a7679c120dc8567bdb22271274dc64d582efb"` |
| `/tested_inputs/6/path` | `"cmake/WriteGDExtensionStamp.cmake"` |
| `/tested_inputs/6/mode` | `"text_utf8_lf"` |
| `/tested_inputs/6/size_bytes` | `637` |
| `/tested_inputs/6/sha256` | `"fc17c00d02334a374a63df6401a6f88fe3d1150a32f98daba8e6158b655a5a5c"` |
| `/tested_inputs/7/path` | `"native/extension/CMakeLists.txt"` |
| `/tested_inputs/7/mode` | `"text_utf8_lf"` |
| `/tested_inputs/7/size_bytes` | `3755` |
| `/tested_inputs/7/sha256` | `"1254a1d7611d1af4f6f2525351851a68325229e09331870f56f7428e807b1466"` |
| `/tested_inputs/8/path` | `"native/extension/include/ninho/extension/adapter_helpers.hpp"` |
| `/tested_inputs/8/mode` | `"text_utf8_lf"` |
| `/tested_inputs/8/size_bytes` | `9425` |
| `/tested_inputs/8/sha256` | `"301e9c169bf19a63c3d987659fb4ee57fbb2c2cae8ed10a4e77107fff5a5cb91"` |
| `/tested_inputs/9/path` | `"native/extension/include/ninho/extension/box3d_world_node.hpp"` |
| `/tested_inputs/9/mode` | `"text_utf8_lf"` |
| `/tested_inputs/9/size_bytes` | `1796` |
| `/tested_inputs/9/sha256` | `"7b85101813d8137daa510b3febdd2846a4bb2989c3dc03447593f132f3bd37e9"` |
| `/tested_inputs/10/path` | `"native/extension/include/ninho/extension/orbital_session_node.hpp"` |
| `/tested_inputs/10/mode` | `"text_utf8_lf"` |
| `/tested_inputs/10/size_bytes` | `6359` |
| `/tested_inputs/10/sha256` | `"f710cc9d3c8a220428c1d2992f3510f3f7b278dd91081d0729b180f193720ff5"` |
| `/tested_inputs/11/path` | `"native/extension/include/ninho/extension/register_types.hpp"` |
| `/tested_inputs/11/mode` | `"text_utf8_lf"` |
| `/tested_inputs/11/size_bytes` | `531` |
| `/tested_inputs/11/sha256` | `"3177e4fe903ea362d3da888f09df187417c28dce67a7c1c6c7db621afafc40bb"` |
| `/tested_inputs/12/path` | `"native/extension/src/adapter_helpers.cpp"` |
| `/tested_inputs/12/mode` | `"text_utf8_lf"` |
| `/tested_inputs/12/size_bytes` | `727` |
| `/tested_inputs/12/sha256` | `"58ea1baea32f0fcec21002bd5c030382ba4475b350f29d4657e42adac390e9e3"` |
| `/tested_inputs/13/path` | `"native/extension/src/box3d_world_node.cpp"` |
| `/tested_inputs/13/mode` | `"text_utf8_lf"` |
| `/tested_inputs/13/size_bytes` | `11710` |
| `/tested_inputs/13/sha256` | `"e3d628fdcf6296d3763446197d82fdbb168511d7c8c1633b01ec0ff011ae1642"` |
| `/tested_inputs/14/path` | `"native/extension/src/orbital_session_node.cpp"` |
| `/tested_inputs/14/mode` | `"text_utf8_lf"` |
| `/tested_inputs/14/size_bytes` | `31793` |
| `/tested_inputs/14/sha256` | `"3d17a7af772f6a96a7d36e8a1c9136f8f030a78a0cb239e40e64ae85bcf19180"` |
| `/tested_inputs/15/path` | `"native/extension/src/register_types.cpp"` |
| `/tested_inputs/15/mode` | `"text_utf8_lf"` |
| `/tested_inputs/15/size_bytes` | `2314` |
| `/tested_inputs/15/sha256` | `"779af837acf7f131da90b1906fe57540ede5c6ba1fcbf0aad759467bdad000c0"` |
| `/tested_inputs/16/path` | `"native/extension/tests/adapter_helpers_tests.cpp"` |
| `/tested_inputs/16/mode` | `"text_utf8_lf"` |
| `/tested_inputs/16/size_bytes` | `10223` |
| `/tested_inputs/16/sha256` | `"2ed3fe212d45e1bdab27304ecfea940bd65bd09833630123cef72ebe705ef53e"` |
| `/tested_inputs/17/path` | `"native/extension/tests/legacy_orbital_frame_contract_tests.cpp"` |
| `/tested_inputs/17/mode` | `"text_utf8_lf"` |
| `/tested_inputs/17/size_bytes` | `3274` |
| `/tested_inputs/17/sha256` | `"0c0ca5c3ea5f248d32fda30e9533f57efc46c70c80eb17d3a46100fb53b77625"` |
| `/tested_inputs/18/path` | `"native/extension/tests/orbital_session_adapter_tests.cpp"` |
| `/tested_inputs/18/mode` | `"text_utf8_lf"` |
| `/tested_inputs/18/size_bytes` | `9485` |
| `/tested_inputs/18/sha256` | `"1356b5a30edccbd48c3bfa4816cb39da662bd87a5288e27a8849b28793a4d371"` |
| `/tested_inputs/19/path` | `"native/extension/tests/registration_contract_tests.cpp"` |
| `/tested_inputs/19/mode` | `"text_utf8_lf"` |
| `/tested_inputs/19/size_bytes` | `856` |
| `/tested_inputs/19/sha256` | `"e4a69e3ab72e5b88e660740f5b3309be3e5f46cb14f02e2c08c6175a92715c4b"` |
| `/tested_inputs/20/path` | `"native/extension/tests/session_frame_batch_tests.cpp"` |
| `/tested_inputs/20/mode` | `"text_utf8_lf"` |
| `/tested_inputs/20/size_bytes` | `8254` |
| `/tested_inputs/20/sha256` | `"007856c724bba9cdcb81a79b192284ba55e7e0be2adde11ddaa4cbb6ee1e37e4"` |
| `/tested_inputs/21/path` | `"native/kernel/CMakeLists.txt"` |
| `/tested_inputs/21/mode` | `"text_utf8_lf"` |
| `/tested_inputs/21/size_bytes` | `1007` |
| `/tested_inputs/21/sha256` | `"2f7a6f92d53b6c9975e1175915e14e86f730935661aa3a1a691bb62f370b3323"` |
| `/tested_inputs/22/path` | `"native/kernel/include/ninho/physics/physics_limits.hpp"` |
| `/tested_inputs/22/mode` | `"text_utf8_lf"` |
| `/tested_inputs/22/size_bytes` | `105` |
| `/tested_inputs/22/sha256` | `"fa4ee6c1ecfae90e16a1ac8531a9f2012280798fd1dd70afe62c60e6b171f4ec"` |
| `/tested_inputs/23/path` | `"native/kernel/include/ninho/physics/physics_types.hpp"` |
| `/tested_inputs/23/mode` | `"text_utf8_lf"` |
| `/tested_inputs/23/size_bytes` | `2698` |
| `/tested_inputs/23/sha256` | `"2c795c1c946675998cc41da2a80d909d04b06d14e4efdf222c630c3ede1cf4b3"` |
| `/tested_inputs/24/path` | `"native/kernel/include/ninho/physics/physics_world.hpp"` |
| `/tested_inputs/24/mode` | `"text_utf8_lf"` |
| `/tested_inputs/24/size_bytes` | `6642` |
| `/tested_inputs/24/sha256` | `"0b344d5251a6ccb14c4d6dd9f09ecf9fb93d2d95bd5a60ec9b6f857e960c48a7"` |
| `/tested_inputs/25/path` | `"native/kernel/include/ninho/physics/radial_gravity.hpp"` |
| `/tested_inputs/25/mode` | `"text_utf8_lf"` |
| `/tested_inputs/25/size_bytes` | `763` |
| `/tested_inputs/25/sha256` | `"14af0b3aa42a1e26ec1bd6036c97da97d1371b5aa15ae6a32a877a526b905db2"` |
| `/tested_inputs/26/path` | `"native/kernel/include/ninho/physics/scenario.hpp"` |
| `/tested_inputs/26/mode` | `"text_utf8_lf"` |
| `/tested_inputs/26/size_bytes` | `13527` |
| `/tested_inputs/26/sha256` | `"c0d8a789c0b91cf00395308c640a8e02fb903731382ab270b42cd1c4bce6a821"` |
| `/tested_inputs/27/path` | `"native/kernel/src/box3d_allocator_probe.cpp"` |
| `/tested_inputs/27/mode` | `"text_utf8_lf"` |
| `/tested_inputs/27/size_bytes` | `213` |
| `/tested_inputs/27/sha256` | `"3a408601664e8e8af5635fd141ee7c724e2c2458e5062aad0b7baf3941d152e2"` |
| `/tested_inputs/28/path` | `"native/kernel/src/box3d_allocator_probe.hpp"` |
| `/tested_inputs/28/mode` | `"text_utf8_lf"` |
| `/tested_inputs/28/size_bytes` | `337` |
| `/tested_inputs/28/sha256` | `"5bf393d6cde5cf02db2f790ff162aba33e190d1768102466ecfe5c1616d7be4b"` |
| `/tested_inputs/29/path` | `"native/kernel/src/box3d_conversions.hpp"` |
| `/tested_inputs/29/mode` | `"text_utf8_lf"` |
| `/tested_inputs/29/size_bytes` | `1216` |
| `/tested_inputs/29/sha256` | `"563b1e1e9a33ab7a0bdbec198c97a0bc227221b4512d77d34fd8b14feae8a877"` |
| `/tested_inputs/30/path` | `"native/kernel/src/box3d_replay_conformance.cpp"` |
| `/tested_inputs/30/mode` | `"text_utf8_lf"` |
| `/tested_inputs/30/size_bytes` | `3833` |
| `/tested_inputs/30/sha256` | `"bc90cf1b14738b6dc897fa1b77f65b8d60530ecdbcde11d4753b29f4dfecb8cc"` |
| `/tested_inputs/31/path` | `"native/kernel/src/box3d_replay_conformance.hpp"` |
| `/tested_inputs/31/mode` | `"text_utf8_lf"` |
| `/tested_inputs/31/size_bytes` | `403` |
| `/tested_inputs/31/sha256` | `"607a01759002816a48ace7361ee82e26d2a6b20438dc32eac2dec306e110f8c3"` |
| `/tested_inputs/32/path` | `"native/kernel/src/physics_world.cpp"` |
| `/tested_inputs/32/mode` | `"text_utf8_lf"` |
| `/tested_inputs/32/size_bytes` | `67137` |
| `/tested_inputs/32/sha256` | `"da655462e410c3f6473fa3a9a6fd9135c193658c6bdd4f9eea874ca8c68f5a3d"` |
| `/tested_inputs/33/path` | `"native/kernel/src/physics_world_test_facade.hpp"` |
| `/tested_inputs/33/mode` | `"text_utf8_lf"` |
| `/tested_inputs/33/size_bytes` | `420` |
| `/tested_inputs/33/sha256` | `"7ddb6c079257a4e7c1a245fa043b54ab6d473d364eec3fa803a99b9afe913b2b"` |
| `/tested_inputs/34/path` | `"native/kernel/src/radial_gravity.cpp"` |
| `/tested_inputs/34/mode` | `"text_utf8_lf"` |
| `/tested_inputs/34/size_bytes` | `1863` |
| `/tested_inputs/34/sha256` | `"ef20acd6f17034cf8c5b017137796bea945fb9910d5c7ec7b8eff941b7b1ab46"` |
| `/tested_inputs/35/path` | `"native/kernel/src/scenario.cpp"` |
| `/tested_inputs/35/mode` | `"text_utf8_lf"` |
| `/tested_inputs/35/size_bytes` | `166432` |
| `/tested_inputs/35/sha256` | `"3349eb04913719a7f77229722333e894c290afa7d2ee592d89bf9b638916b90a"` |
| `/tested_inputs/36/path` | `"native/kernel/src/scenario_configuration.hpp"` |
| `/tested_inputs/36/mode` | `"text_utf8_lf"` |
| `/tested_inputs/36/size_bytes` | `1014` |
| `/tested_inputs/36/sha256` | `"1dce7723604301a83b37349f8a42969bfea8baa3ebe4a7c5b8ac3e9445331f7c"` |
| `/tested_inputs/37/path` | `"native/kernel/src/scenario_test_facade.hpp"` |
| `/tested_inputs/37/mode` | `"text_utf8_lf"` |
| `/tested_inputs/37/size_bytes` | `359` |
| `/tested_inputs/37/sha256` | `"161de04d8c791b5294c47d4a5a959384ce5f566584188c72ba984a4a9a16a1c0"` |
| `/tested_inputs/38/path` | `"native/simulation/CMakeLists.txt"` |
| `/tested_inputs/38/mode` | `"text_utf8_lf"` |
| `/tested_inputs/38/size_bytes` | `4549` |
| `/tested_inputs/38/sha256` | `"78a65fc503672d5ec696420a018b1a360b83602d66abf66a9a3ae0b68d0a104a"` |
| `/tested_inputs/39/path` | `"native/simulation/include/ninho/simulation/commands.hpp"` |
| `/tested_inputs/39/mode` | `"text_utf8_lf"` |
| `/tested_inputs/39/size_bytes` | `906` |
| `/tested_inputs/39/sha256` | `"be02293941c60d0e4d598a13bfb789a14515d77d0d977be83b1d657a5162537d"` |
| `/tested_inputs/40/path` | `"native/simulation/include/ninho/simulation/content.hpp"` |
| `/tested_inputs/40/mode` | `"text_utf8_lf"` |
| `/tested_inputs/40/size_bytes` | `7629` |
| `/tested_inputs/40/sha256` | `"575de7bd5a6a12a636197ba384cd22a3efc48e52069f496f148332ab3320b114"` |
| `/tested_inputs/41/path` | `"native/simulation/include/ninho/simulation/events.hpp"` |
| `/tested_inputs/41/mode` | `"text_utf8_lf"` |
| `/tested_inputs/41/size_bytes` | `1675` |
| `/tested_inputs/41/sha256` | `"b26bcbcb85c59394ade20e65f889976e0994fd17fe68dcfba6ad868d553591d5"` |
| `/tested_inputs/42/path` | `"native/simulation/include/ninho/simulation/session.hpp"` |
| `/tested_inputs/42/mode` | `"text_utf8_lf"` |
| `/tested_inputs/42/size_bytes` | `5196` |
| `/tested_inputs/42/sha256` | `"eb2b6a7261c322d71cd6a7e0680936282d6fb989923977f32222c0c72dd75205"` |
| `/tested_inputs/43/path` | `"native/simulation/src/canonical_state.cpp"` |
| `/tested_inputs/43/mode` | `"text_utf8_lf"` |
| `/tested_inputs/43/size_bytes` | `22123` |
| `/tested_inputs/43/sha256` | `"7db3a11f11ca40880c4689824341a9870803d39eaffdd4d90caff154671b0f7a"` |
| `/tested_inputs/44/path` | `"native/simulation/src/content.cpp"` |
| `/tested_inputs/44/mode` | `"text_utf8_lf"` |
| `/tested_inputs/44/size_bytes` | `63519` |
| `/tested_inputs/44/sha256` | `"1a92c0529f66cdd8a762cff1c93ae52be4276faab3ee1986a7118eeb2c600020"` |
| `/tested_inputs/45/path` | `"native/simulation/src/damage_system.cpp"` |
| `/tested_inputs/45/mode` | `"text_utf8_lf"` |
| `/tested_inputs/45/size_bytes` | `12792` |
| `/tested_inputs/45/sha256` | `"263bfb2aecf4100cda01f2e5a9b0b6d3220ff6193a10680f338bac8871dbe963"` |
| `/tested_inputs/46/path` | `"native/simulation/src/damage_system.hpp"` |
| `/tested_inputs/46/mode` | `"text_utf8_lf"` |
| `/tested_inputs/46/size_bytes` | `2015` |
| `/tested_inputs/46/sha256` | `"3ec71c08d1347854cae553be7f75269fa011b47f94958974836efbf6a6eb1b17"` |
| `/tested_inputs/47/path` | `"native/simulation/src/fracture_system.cpp"` |
| `/tested_inputs/47/mode` | `"text_utf8_lf"` |
| `/tested_inputs/47/size_bytes` | `15180` |
| `/tested_inputs/47/sha256` | `"0d19385c9256476cdc2c3773eedbcce8663f07405997507c1fbc29c3649ee960"` |
| `/tested_inputs/48/path` | `"native/simulation/src/gravity_field_ability_system.cpp"` |
| `/tested_inputs/48/mode` | `"text_utf8_lf"` |
| `/tested_inputs/48/size_bytes` | `6488` |
| `/tested_inputs/48/sha256` | `"f49f549bea6f67065f78a7aca587b539e760e3f9b477ad3c8285eca155a61735"` |
| `/tested_inputs/49/path` | `"native/simulation/src/launch_system.cpp"` |
| `/tested_inputs/49/mode` | `"text_utf8_lf"` |
| `/tested_inputs/49/size_bytes` | `25197` |
| `/tested_inputs/49/sha256` | `"32709496fcdbb14cac7982052dfd7c41f8503ba13feea0fa3cf409700bcee14d"` |
| `/tested_inputs/50/path` | `"native/simulation/src/material_mapping.hpp"` |
| `/tested_inputs/50/mode` | `"text_utf8_lf"` |
| `/tested_inputs/50/size_bytes` | `609` |
| `/tested_inputs/50/sha256` | `"bfc3f67e77dfec752fbf0f745a721475d0ed3f26dd4fcf865571306366542fcf"` |
| `/tested_inputs/51/path` | `"native/simulation/src/objective_system.cpp"` |
| `/tested_inputs/51/mode` | `"text_utf8_lf"` |
| `/tested_inputs/51/size_bytes` | `899` |
| `/tested_inputs/51/sha256` | `"82528ac4de52229c59b9d5773fbd6ae6de276692131f1a795d119c44c03e7356"` |
| `/tested_inputs/52/path` | `"native/simulation/src/session.cpp"` |
| `/tested_inputs/52/mode` | `"text_utf8_lf"` |
| `/tested_inputs/52/size_bytes` | `16157` |
| `/tested_inputs/52/sha256` | `"4ba94a74b769618e962717b86adb7a8bdbb706b2b6ee653e4112d6f95ecd760a"` |
| `/tested_inputs/53/path` | `"native/simulation/src/session_builder.cpp"` |
| `/tested_inputs/53/mode` | `"text_utf8_lf"` |
| `/tested_inputs/53/size_bytes` | `17518` |
| `/tested_inputs/53/sha256` | `"67c1ea77e1034340e253b46f9060c43fd5d2f5fed56ace8efd875f1f226714b2"` |
| `/tested_inputs/54/path` | `"native/simulation/src/session_internal.hpp"` |
| `/tested_inputs/54/mode` | `"text_utf8_lf"` |
| `/tested_inputs/54/size_bytes` | `6467` |
| `/tested_inputs/54/sha256` | `"670bd18fe94ecee1c6e3282bff050368e8f87e141ce5722a1ac8bd3caaf4853b"` |
| `/tested_inputs/55/path` | `"native/simulation/src/session_test_facade.hpp"` |
| `/tested_inputs/55/mode` | `"text_utf8_lf"` |
| `/tested_inputs/55/size_bytes` | `3042` |
| `/tested_inputs/55/sha256` | `"fecb6cd51c6fefba0adc238f04bdfc56dc429043b4a00e6ab07b0afcc7890679"` |
| `/tested_inputs/56/path` | `"native/simulation/tests/content_tests.cpp"` |
| `/tested_inputs/56/mode` | `"text_utf8_lf"` |
| `/tested_inputs/56/size_bytes` | `35677` |
| `/tested_inputs/56/sha256` | `"796a08013d6651a274acdaadb97f3f6eb9d1a20f218816c87edfbfab160ddddf"` |
| `/tested_inputs/57/path` | `"native/simulation/tests/damage_anchor_tests.cpp"` |
| `/tested_inputs/57/mode` | `"text_utf8_lf"` |
| `/tested_inputs/57/size_bytes` | `20494` |
| `/tested_inputs/57/sha256` | `"598f040d12c0c691bb6d7e21c783309c35722710a82e458d60f9e23ffde9bade"` |
| `/tested_inputs/58/path` | `"native/simulation/tests/fracture_objective_tests.cpp"` |
| `/tested_inputs/58/mode` | `"text_utf8_lf"` |
| `/tested_inputs/58/size_bytes` | `13161` |
| `/tested_inputs/58/sha256` | `"215c55898dde48408ffaac0a567797c5db5e0e1b039ffb0c489bc621e4eac1a7"` |
| `/tested_inputs/59/path` | `"native/simulation/tests/gravity_field_ability_tests.cpp"` |
| `/tested_inputs/59/mode` | `"text_utf8_lf"` |
| `/tested_inputs/59/size_bytes` | `23041` |
| `/tested_inputs/59/sha256` | `"300c30eb6018254854d066933b254fd47ad20e92f10a805397657303ff10e9e7"` |
| `/tested_inputs/60/path` | `"native/simulation/tests/kernel_contract_tests.cpp"` |
| `/tested_inputs/60/mode` | `"text_utf8_lf"` |
| `/tested_inputs/60/size_bytes` | `2743` |
| `/tested_inputs/60/sha256` | `"8882676ade374cdf8b76d1866e47e4adeeaf76c78f664002c5c5be093b5eae85"` |
| `/tested_inputs/61/path` | `"native/simulation/tests/launch_fsm_tests.cpp"` |
| `/tested_inputs/61/mode` | `"text_utf8_lf"` |
| `/tested_inputs/61/size_bytes` | `34247` |
| `/tested_inputs/61/sha256` | `"60bf5278a375a18024aa2adb8bec7e3dc2d98602b31602f1cc9adfcb355f089f"` |
| `/tested_inputs/62/path` | `"native/simulation/tests/legacy_orbital_characterization_tests.cpp"` |
| `/tested_inputs/62/mode` | `"text_utf8_lf"` |
| `/tested_inputs/62/size_bytes` | `17894` |
| `/tested_inputs/62/sha256` | `"9100dd4290b529b36ec6cb11fcedd486bb0828760e644e2b2ed836cf21117a8b"` |
| `/tested_inputs/63/path` | `"native/simulation/tests/playthrough_tests.cpp"` |
| `/tested_inputs/63/mode` | `"text_utf8_lf"` |
| `/tested_inputs/63/size_bytes` | `20246` |
| `/tested_inputs/63/sha256` | `"d0d0987f9ab8dca70d6f18cb185bf84ee7eabff2f3ff21f56e2cec382e6575f9"` |
| `/tested_inputs/64/path` | `"native/simulation/tests/session_tests.cpp"` |
| `/tested_inputs/64/mode` | `"text_utf8_lf"` |
| `/tested_inputs/64/size_bytes` | `41736` |
| `/tested_inputs/64/sha256` | `"1a8ee368a9e0bb3afb69fa58f2579dbbe0870ed7a55c07ea6953ff21fc698ec7"` |
| `/tested_inputs/65/path` | `"native/simulation/tests/test_framework.hpp"` |
| `/tested_inputs/65/mode` | `"text_utf8_lf"` |
| `/tested_inputs/65/size_bytes` | `1870` |
| `/tested_inputs/65/sha256` | `"a2dcdad0a827c7f557a891e8dbf2aa131f9f377f1176f14f1110ccc87079cc77"` |
| `/tested_inputs/66/path` | `"native/simulation/tests/test_main.cpp"` |
| `/tested_inputs/66/mode` | `"text_utf8_lf"` |
| `/tested_inputs/66/size_bytes` | `1240` |
| `/tested_inputs/66/sha256` | `"0fe1a9742c2b031787df553caa0d1d513decea1dd45c1ef3054eb23a640dddfb"` |
| `/tested_inputs/67/path` | `"native/spike/CMakeLists.txt"` |
| `/tested_inputs/67/mode` | `"text_utf8_lf"` |
| `/tested_inputs/67/size_bytes` | `518` |
| `/tested_inputs/67/sha256` | `"4244861de1fc0f84e85e96d44cbd5e6f1eb9c877b31a743c7c6660c5b3fdf3bd"` |
| `/tested_inputs/68/path` | `"native/spike/main.cpp"` |
| `/tested_inputs/68/mode` | `"text_utf8_lf"` |
| `/tested_inputs/68/size_bytes` | `13396` |
| `/tested_inputs/68/sha256` | `"e72ceeab9c929bf6fee638de840a9eac8fe074681c3517e023c094ee4374cbdd"` |
| `/tested_inputs/69/path` | `"native/tests/CMakeLists.txt"` |
| `/tested_inputs/69/mode` | `"text_utf8_lf"` |
| `/tested_inputs/69/size_bytes` | `4134` |
| `/tested_inputs/69/sha256` | `"d36e5eebdf52db4c8fa4d6fb00e5bce434440ebefcfdb9950fb6ef0ce9bd9e22"` |
| `/tested_inputs/70/path` | `"native/tests/capability_tests.cpp"` |
| `/tested_inputs/70/mode` | `"text_utf8_lf"` |
| `/tested_inputs/70/size_bytes` | `27540` |
| `/tested_inputs/70/sha256` | `"d495449638982624fae7d6b5ad98bbb8461dce45383175832f77ce23c50877a1"` |
| `/tested_inputs/71/path` | `"native/tests/determinism_tests.cpp"` |
| `/tested_inputs/71/mode` | `"text_utf8_lf"` |
| `/tested_inputs/71/size_bytes` | `13429` |
| `/tested_inputs/71/sha256` | `"0798f4b49c3cc52796d498d90062ee16eb85653c28335e11444c581d83399095"` |
| `/tested_inputs/72/path` | `"native/tests/json_smoke.py"` |
| `/tested_inputs/72/mode` | `"text_utf8_lf"` |
| `/tested_inputs/72/size_bytes` | `18266` |
| `/tested_inputs/72/sha256` | `"0ec7e8dcb533806ecafb810d25b5c68be8d22e964d76e676625b713d7204915b"` |
| `/tested_inputs/73/path` | `"native/tests/pile_stability_tests.cpp"` |
| `/tested_inputs/73/mode` | `"text_utf8_lf"` |
| `/tested_inputs/73/size_bytes` | `19530` |
| `/tested_inputs/73/sha256` | `"d37aa0e19bdb2cc0517feb62b664dd96b948e8f762499e11cb250e6271a01e5e"` |
| `/tested_inputs/74/path` | `"native/tests/projectile_ccd_tests.cpp"` |
| `/tested_inputs/74/mode` | `"text_utf8_lf"` |
| `/tested_inputs/74/size_bytes` | `5898` |
| `/tested_inputs/74/sha256` | `"7a620a39da06923e1b7ef5eedd8d49aefe6c6f66e414431cce409093232478d5"` |
| `/tested_inputs/75/path` | `"native/tests/radial_gravity_tests.cpp"` |
| `/tested_inputs/75/mode` | `"text_utf8_lf"` |
| `/tested_inputs/75/size_bytes` | `6247` |
| `/tested_inputs/75/sha256` | `"e9732b98d3c545af0d0a6067c566c5767e50f4d3d54b4042c5c675653919d278"` |
| `/tested_inputs/76/path` | `"native/tests/scenario_capability_tests.cpp"` |
| `/tested_inputs/76/mode` | `"text_utf8_lf"` |
| `/tested_inputs/76/size_bytes` | `12233` |
| `/tested_inputs/76/sha256` | `"da0b07669a5499b56b758a7c3ae07150d9b83af1e375cf13d3f529ab63519191"` |
| `/tested_inputs/77/path` | `"native/tests/test_framework.hpp"` |
| `/tested_inputs/77/mode` | `"text_utf8_lf"` |
| `/tested_inputs/77/size_bytes` | `3899` |
| `/tested_inputs/77/sha256` | `"76e694857c118ca7d3007d349637a52bf9b321228fdd81a8408f9547b81ee1e6"` |
| `/tested_inputs/78/path` | `"native/tests/test_main.cpp"` |
| `/tested_inputs/78/mode` | `"text_utf8_lf"` |
| `/tested_inputs/78/size_bytes` | `2566` |
| `/tested_inputs/78/sha256` | `"372df787fbb6fae307ba0ee2c862ef043913b264760358e86ce81903eb9b4626"` |
| `/tested_inputs/79/path` | `"native/tests/world_lifecycle_tests.cpp"` |
| `/tested_inputs/79/mode` | `"text_utf8_lf"` |
| `/tested_inputs/79/size_bytes` | `12095` |
| `/tested_inputs/79/sha256` | `"da2fab9c92a64d572c04e2311dca0bfa0343e9738dcbdb5ecdb16d884e9dceed"` |
| `/tested_inputs/80/path` | `"tools/FoundationEvidenceGate.psm1"` |
| `/tested_inputs/80/mode` | `"text_utf8_lf"` |
| `/tested_inputs/80/size_bytes` | `18698` |
| `/tested_inputs/80/sha256` | `"195d0cc069d74115ed1c1ab63f1c99909e8a1ba5a4e2a807a66b62a3ca729a60"` |
| `/tested_inputs/81/path` | `"tools/GodotSmokeRegistry.psm1"` |
| `/tested_inputs/81/mode` | `"text_utf8_lf"` |
| `/tested_inputs/81/size_bytes` | `5268` |
| `/tested_inputs/81/sha256` | `"82319b1f009954a5632b7991d4ca291091d2619d762ff5df44b4cc0330857b32"` |
| `/tested_inputs/82/path` | `"tools/GodotSpikeGate.psm1"` |
| `/tested_inputs/82/mode` | `"text_utf8_lf"` |
| `/tested_inputs/82/size_bytes` | `11080` |
| `/tested_inputs/82/sha256` | `"f81ed257e02861aa6f84aace45d39f33d51ceef740d09f149d5c0363087f4b1c"` |
| `/tested_inputs/83/path` | `"tools/Invoke-Native.ps1"` |
| `/tested_inputs/83/mode` | `"text_utf8_lf"` |
| `/tested_inputs/83/size_bytes` | `2185` |
| `/tested_inputs/83/sha256` | `"4cba4036cbc5007bddb3762f5b1c45f72fc63ef63d6b6dbb16c8b0a6ed9344d4"` |
| `/tested_inputs/84/path` | `"tools/SafePath.psm1"` |
| `/tested_inputs/84/mode` | `"text_utf8_lf"` |
| `/tested_inputs/84/size_bytes` | `1841` |
| `/tested_inputs/84/sha256` | `"d6a5c2daeeb3ef4369daa35bf21d6dc60c3ec736ab8c932078b3e24bd32e4ffc"` |
| `/tested_inputs/85/path` | `"tools/SpikeEvidenceValidation.psm1"` |
| `/tested_inputs/85/mode` | `"text_utf8_lf"` |
| `/tested_inputs/85/size_bytes` | `51353` |
| `/tested_inputs/85/sha256` | `"847f52d2501bf25b5136797e71600c4934bf1c03b9ba454f483e73422bd1ddd9"` |
| `/tested_inputs/86/path` | `"tools/SpikeReportGate.psm1"` |
| `/tested_inputs/86/mode` | `"text_utf8_lf"` |
| `/tested_inputs/86/size_bytes` | `2400` |
| `/tested_inputs/86/sha256` | `"762cc1cd1e6740f4eb5745511d9a565a11d7835bac863f71374e44ded9c11077"` |
| `/tested_inputs/87/path` | `"tools/TestedInputIdentity.psm1"` |
| `/tested_inputs/87/mode` | `"text_utf8_lf"` |
| `/tested_inputs/87/size_bytes` | `6567` |
| `/tested_inputs/87/sha256` | `"45160531c01d36f7eb1c0d3d2a115fd6d7541de7f2883081b5fb57a7f5cb0898"` |
| `/tested_inputs/88/path` | `"tools/ToolchainIntegrity.psm1"` |
| `/tested_inputs/88/mode` | `"text_utf8_lf"` |
| `/tested_inputs/88/size_bytes` | `3009` |
| `/tested_inputs/88/sha256` | `"c3fc2f6b8d97c27e2f1d5887c05de4c91cfa8f4f6b814c21208fb3f6d533dc4c"` |
| `/tested_inputs/89/path` | `"tools/UpstreamBox3DGate.psm1"` |
| `/tested_inputs/89/mode` | `"text_utf8_lf"` |
| `/tested_inputs/89/size_bytes` | `7219` |
| `/tested_inputs/89/sha256` | `"324fd58c0dcee1ea712e36df53390ee4deff6faac604e4f32894f57a756c7163"` |
| `/tested_inputs/90/path` | `"tools/VerticalSliceGate.psm1"` |
| `/tested_inputs/90/mode` | `"text_utf8_lf"` |
| `/tested_inputs/90/size_bytes` | `64476` |
| `/tested_inputs/90/sha256` | `"624df6fdbc6d6ce9a639b3bc972e3ed4c1ac821e5ef9b78164134ef8292bac97"` |
| `/tested_inputs/91/path` | `"tools/bootstrap.ps1"` |
| `/tested_inputs/91/mode` | `"text_utf8_lf"` |
| `/tested_inputs/91/size_bytes` | `15698` |
| `/tested_inputs/91/sha256` | `"df335d69a457724bfc9d8ec05463adde0728feb6c217c772f910777715f80246"` |
| `/tested_inputs/92/path` | `"tools/box3d-v0.1.0-compile-sources.txt"` |
| `/tested_inputs/92/mode` | `"text_utf8_lf"` |
| `/tested_inputs/92/size_bytes` | `1339` |
| `/tested_inputs/92/sha256` | `"8bc4897fbc25895f75fab44e541c12d3b0e528e2692bbabbe9df632871a7d1f0"` |
| `/tested_inputs/93/path` | `"tools/build.ps1"` |
| `/tested_inputs/93/mode` | `"text_utf8_lf"` |
| `/tested_inputs/93/size_bytes` | `1456` |
| `/tested_inputs/93/sha256` | `"5faf578e2d23965aa2c5f87d4cb86d74c9605e842ebb38dcacccaefcff185241"` |
| `/tested_inputs/94/path` | `"tools/generate_foundation_report.py"` |
| `/tested_inputs/94/mode` | `"text_utf8_lf"` |
| `/tested_inputs/94/size_bytes` | `22194` |
| `/tested_inputs/94/sha256` | `"12f47d5ff5d7f6786f78e1bf4e24b18eec01c2a3a7b74544d6116eb350efc740"` |
| `/tested_inputs/95/path` | `"tools/run_spike.ps1"` |
| `/tested_inputs/95/mode` | `"text_utf8_lf"` |
| `/tested_inputs/95/size_bytes` | `2709` |
| `/tested_inputs/95/sha256` | `"d571b34deb9f4cf708da2942b892016a13edae05472133b0e6887b3e4050b36d"` |
| `/tested_inputs/96/path` | `"tools/test.ps1"` |
| `/tested_inputs/96/mode` | `"text_utf8_lf"` |
| `/tested_inputs/96/size_bytes` | `23185` |
| `/tested_inputs/96/sha256` | `"e2220526d429cab96ce083db8c86fda7e57bbd424d0951b93e694418308d7a35"` |
| `/tested_inputs/97/path` | `"tools/toolchain.lock.json"` |
| `/tested_inputs/97/mode` | `"text_utf8_lf"` |
| `/tested_inputs/97/size_bytes` | `3047` |
| `/tested_inputs/97/sha256` | `"4242a98bf001fb9064227baf5f77ce5f28bac3abdd81c17ae2b3399b31d4cb01"` |

### Release snapshot metadata

| JSON pointer | Value |
| --- | --- |
| `/schema` | `"ninho.physics.scenario.v1"` |
| `/tool/name` | `"ninho_physics_spike"` |
| `/tool/version` | `"0.1.0"` |
| `/dependencies/box3d/version` | `"0.1.0"` |
| `/dependencies/box3d/commit` | `"8441b4a06d6d09dcfb0b0f704df4d847d1437b92"` |
| `/dependencies/godot/version` | `"4.5.1-stable"` |
| `/dependencies/godot/commit` | `"f62fdbde15035c5576dad93e586201f4d41ef0cb"` |
| `/dependencies/godot_cpp/version` | `"godot-4.5-stable"` |
| `/dependencies/godot_cpp/commit` | `"e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77"` |
| `/build_type` | `"Release"` |
| `/cpu` | `"Intel64 Family 6 Model 141 Stepping 1, GenuineIntel"` |
| `/configuration/seed` | `1` |
| `/configuration/substeps` | `4` |
| `/configuration/repeat` | `2` |
| `/configuration/time_step` | `0.016666666666666666` |
| `/process_box3d_allocator/baseline_bytes` | `0` |
| `/process_box3d_allocator/final_bytes` | `0` |
| `/process_box3d_allocator/max_abs_delta` | `0` |
| `/process_box3d_allocator/exact_return` | `true` |
| `/process_box3d_allocator/warmup_post_teardown` | `[]` |
| `/process_box3d_allocator/measured_post_teardown` | `[]` |
| `/budget_qualification/status` | `"deferred"` |
| `/budget_qualification/target_growth_ratio` | `0.05` |
| `/budget_qualification/warning` | `"private_commit_budget_unqualified"` |
| `/warnings/0/code` | `"private_commit_budget_unqualified"` |
| `/warnings/0/message` | `"PrivateUsage budget is deferred to a packaged Release build on reference hardware"` |
| `/warnings/0/details/0/name` | `"budget_scope"` |
| `/warnings/0/details/0/value` | `"future_packaged_reference_hardware"` |
| `/violations` | `[]` |
| `/recommendation` | `"prosseguir_com_limites"` |
| `/source_revision` | `"2c7e3b5afa296df1d37b376eac6b41ffeffc4fcb"` |
| `/tested_inputs_schema` | `"ninho.tested-inputs.v2"` |
| `/tested_inputs_sha256` | `"7471846c39b1ad6da160f534a4760dbf0320372409d6d851f2301bd6c5097aba"` |
| `/tested_inputs/0/path` | `"CMakeLists.txt"` |
| `/tested_inputs/0/mode` | `"text_utf8_lf"` |
| `/tested_inputs/0/size_bytes` | `3834` |
| `/tested_inputs/0/sha256` | `"f39c15ec5bfa29494d00b2954c93e055babc32e74348bf39a431723b23b69b04"` |
| `/tested_inputs/1/path` | `"CMakePresets.json"` |
| `/tested_inputs/1/mode` | `"text_utf8_lf"` |
| `/tested_inputs/1/size_bytes` | `1120` |
| `/tested_inputs/1/sha256` | `"3316126b405902b73d22d6ea299a9da7c77adcdf09ef24da8c15404273bef1aa"` |
| `/tested_inputs/2/path` | `"cmake/Dependencies.cmake"` |
| `/tested_inputs/2/mode` | `"text_utf8_lf"` |
| `/tested_inputs/2/size_bytes` | `4063` |
| `/tested_inputs/2/sha256` | `"5c49175883c6658759d99b82458996ff6aadf6ce68e2feb9e65183a8f63aa4f8"` |
| `/tested_inputs/3/path` | `"cmake/PrepareGDExtensionTest.cmake"` |
| `/tested_inputs/3/mode` | `"text_utf8_lf"` |
| `/tested_inputs/3/size_bytes` | `261` |
| `/tested_inputs/3/sha256` | `"66d8783a8394ecd593da4d2d221196e00cc5e4bb377dab83c7862f651388a1b5"` |
| `/tested_inputs/4/path` | `"cmake/RequireGodotRuntimeTests.cmake"` |
| `/tested_inputs/4/mode` | `"text_utf8_lf"` |
| `/tested_inputs/4/size_bytes` | `573` |
| `/tested_inputs/4/sha256` | `"3b08148730ad5b1abb4f42b496b21bb5f238b4c66bdfbd606f19de8ac3e7feaa"` |
| `/tested_inputs/5/path` | `"cmake/VerifyGDExtension.cmake"` |
| `/tested_inputs/5/mode` | `"text_utf8_lf"` |
| `/tested_inputs/5/size_bytes` | `3752` |
| `/tested_inputs/5/sha256` | `"612f8a47cfd3c5a31d214560797a7679c120dc8567bdb22271274dc64d582efb"` |
| `/tested_inputs/6/path` | `"cmake/WriteGDExtensionStamp.cmake"` |
| `/tested_inputs/6/mode` | `"text_utf8_lf"` |
| `/tested_inputs/6/size_bytes` | `637` |
| `/tested_inputs/6/sha256` | `"fc17c00d02334a374a63df6401a6f88fe3d1150a32f98daba8e6158b655a5a5c"` |
| `/tested_inputs/7/path` | `"native/extension/CMakeLists.txt"` |
| `/tested_inputs/7/mode` | `"text_utf8_lf"` |
| `/tested_inputs/7/size_bytes` | `3755` |
| `/tested_inputs/7/sha256` | `"1254a1d7611d1af4f6f2525351851a68325229e09331870f56f7428e807b1466"` |
| `/tested_inputs/8/path` | `"native/extension/include/ninho/extension/adapter_helpers.hpp"` |
| `/tested_inputs/8/mode` | `"text_utf8_lf"` |
| `/tested_inputs/8/size_bytes` | `9425` |
| `/tested_inputs/8/sha256` | `"301e9c169bf19a63c3d987659fb4ee57fbb2c2cae8ed10a4e77107fff5a5cb91"` |
| `/tested_inputs/9/path` | `"native/extension/include/ninho/extension/box3d_world_node.hpp"` |
| `/tested_inputs/9/mode` | `"text_utf8_lf"` |
| `/tested_inputs/9/size_bytes` | `1796` |
| `/tested_inputs/9/sha256` | `"7b85101813d8137daa510b3febdd2846a4bb2989c3dc03447593f132f3bd37e9"` |
| `/tested_inputs/10/path` | `"native/extension/include/ninho/extension/orbital_session_node.hpp"` |
| `/tested_inputs/10/mode` | `"text_utf8_lf"` |
| `/tested_inputs/10/size_bytes` | `6359` |
| `/tested_inputs/10/sha256` | `"f710cc9d3c8a220428c1d2992f3510f3f7b278dd91081d0729b180f193720ff5"` |
| `/tested_inputs/11/path` | `"native/extension/include/ninho/extension/register_types.hpp"` |
| `/tested_inputs/11/mode` | `"text_utf8_lf"` |
| `/tested_inputs/11/size_bytes` | `531` |
| `/tested_inputs/11/sha256` | `"3177e4fe903ea362d3da888f09df187417c28dce67a7c1c6c7db621afafc40bb"` |
| `/tested_inputs/12/path` | `"native/extension/src/adapter_helpers.cpp"` |
| `/tested_inputs/12/mode` | `"text_utf8_lf"` |
| `/tested_inputs/12/size_bytes` | `727` |
| `/tested_inputs/12/sha256` | `"58ea1baea32f0fcec21002bd5c030382ba4475b350f29d4657e42adac390e9e3"` |
| `/tested_inputs/13/path` | `"native/extension/src/box3d_world_node.cpp"` |
| `/tested_inputs/13/mode` | `"text_utf8_lf"` |
| `/tested_inputs/13/size_bytes` | `11710` |
| `/tested_inputs/13/sha256` | `"e3d628fdcf6296d3763446197d82fdbb168511d7c8c1633b01ec0ff011ae1642"` |
| `/tested_inputs/14/path` | `"native/extension/src/orbital_session_node.cpp"` |
| `/tested_inputs/14/mode` | `"text_utf8_lf"` |
| `/tested_inputs/14/size_bytes` | `31793` |
| `/tested_inputs/14/sha256` | `"3d17a7af772f6a96a7d36e8a1c9136f8f030a78a0cb239e40e64ae85bcf19180"` |
| `/tested_inputs/15/path` | `"native/extension/src/register_types.cpp"` |
| `/tested_inputs/15/mode` | `"text_utf8_lf"` |
| `/tested_inputs/15/size_bytes` | `2314` |
| `/tested_inputs/15/sha256` | `"779af837acf7f131da90b1906fe57540ede5c6ba1fcbf0aad759467bdad000c0"` |
| `/tested_inputs/16/path` | `"native/extension/tests/adapter_helpers_tests.cpp"` |
| `/tested_inputs/16/mode` | `"text_utf8_lf"` |
| `/tested_inputs/16/size_bytes` | `10223` |
| `/tested_inputs/16/sha256` | `"2ed3fe212d45e1bdab27304ecfea940bd65bd09833630123cef72ebe705ef53e"` |
| `/tested_inputs/17/path` | `"native/extension/tests/legacy_orbital_frame_contract_tests.cpp"` |
| `/tested_inputs/17/mode` | `"text_utf8_lf"` |
| `/tested_inputs/17/size_bytes` | `3274` |
| `/tested_inputs/17/sha256` | `"0c0ca5c3ea5f248d32fda30e9533f57efc46c70c80eb17d3a46100fb53b77625"` |
| `/tested_inputs/18/path` | `"native/extension/tests/orbital_session_adapter_tests.cpp"` |
| `/tested_inputs/18/mode` | `"text_utf8_lf"` |
| `/tested_inputs/18/size_bytes` | `9485` |
| `/tested_inputs/18/sha256` | `"1356b5a30edccbd48c3bfa4816cb39da662bd87a5288e27a8849b28793a4d371"` |
| `/tested_inputs/19/path` | `"native/extension/tests/registration_contract_tests.cpp"` |
| `/tested_inputs/19/mode` | `"text_utf8_lf"` |
| `/tested_inputs/19/size_bytes` | `856` |
| `/tested_inputs/19/sha256` | `"e4a69e3ab72e5b88e660740f5b3309be3e5f46cb14f02e2c08c6175a92715c4b"` |
| `/tested_inputs/20/path` | `"native/extension/tests/session_frame_batch_tests.cpp"` |
| `/tested_inputs/20/mode` | `"text_utf8_lf"` |
| `/tested_inputs/20/size_bytes` | `8254` |
| `/tested_inputs/20/sha256` | `"007856c724bba9cdcb81a79b192284ba55e7e0be2adde11ddaa4cbb6ee1e37e4"` |
| `/tested_inputs/21/path` | `"native/kernel/CMakeLists.txt"` |
| `/tested_inputs/21/mode` | `"text_utf8_lf"` |
| `/tested_inputs/21/size_bytes` | `1007` |
| `/tested_inputs/21/sha256` | `"2f7a6f92d53b6c9975e1175915e14e86f730935661aa3a1a691bb62f370b3323"` |
| `/tested_inputs/22/path` | `"native/kernel/include/ninho/physics/physics_limits.hpp"` |
| `/tested_inputs/22/mode` | `"text_utf8_lf"` |
| `/tested_inputs/22/size_bytes` | `105` |
| `/tested_inputs/22/sha256` | `"fa4ee6c1ecfae90e16a1ac8531a9f2012280798fd1dd70afe62c60e6b171f4ec"` |
| `/tested_inputs/23/path` | `"native/kernel/include/ninho/physics/physics_types.hpp"` |
| `/tested_inputs/23/mode` | `"text_utf8_lf"` |
| `/tested_inputs/23/size_bytes` | `2698` |
| `/tested_inputs/23/sha256` | `"2c795c1c946675998cc41da2a80d909d04b06d14e4efdf222c630c3ede1cf4b3"` |
| `/tested_inputs/24/path` | `"native/kernel/include/ninho/physics/physics_world.hpp"` |
| `/tested_inputs/24/mode` | `"text_utf8_lf"` |
| `/tested_inputs/24/size_bytes` | `6642` |
| `/tested_inputs/24/sha256` | `"0b344d5251a6ccb14c4d6dd9f09ecf9fb93d2d95bd5a60ec9b6f857e960c48a7"` |
| `/tested_inputs/25/path` | `"native/kernel/include/ninho/physics/radial_gravity.hpp"` |
| `/tested_inputs/25/mode` | `"text_utf8_lf"` |
| `/tested_inputs/25/size_bytes` | `763` |
| `/tested_inputs/25/sha256` | `"14af0b3aa42a1e26ec1bd6036c97da97d1371b5aa15ae6a32a877a526b905db2"` |
| `/tested_inputs/26/path` | `"native/kernel/include/ninho/physics/scenario.hpp"` |
| `/tested_inputs/26/mode` | `"text_utf8_lf"` |
| `/tested_inputs/26/size_bytes` | `13527` |
| `/tested_inputs/26/sha256` | `"c0d8a789c0b91cf00395308c640a8e02fb903731382ab270b42cd1c4bce6a821"` |
| `/tested_inputs/27/path` | `"native/kernel/src/box3d_allocator_probe.cpp"` |
| `/tested_inputs/27/mode` | `"text_utf8_lf"` |
| `/tested_inputs/27/size_bytes` | `213` |
| `/tested_inputs/27/sha256` | `"3a408601664e8e8af5635fd141ee7c724e2c2458e5062aad0b7baf3941d152e2"` |
| `/tested_inputs/28/path` | `"native/kernel/src/box3d_allocator_probe.hpp"` |
| `/tested_inputs/28/mode` | `"text_utf8_lf"` |
| `/tested_inputs/28/size_bytes` | `337` |
| `/tested_inputs/28/sha256` | `"5bf393d6cde5cf02db2f790ff162aba33e190d1768102466ecfe5c1616d7be4b"` |
| `/tested_inputs/29/path` | `"native/kernel/src/box3d_conversions.hpp"` |
| `/tested_inputs/29/mode` | `"text_utf8_lf"` |
| `/tested_inputs/29/size_bytes` | `1216` |
| `/tested_inputs/29/sha256` | `"563b1e1e9a33ab7a0bdbec198c97a0bc227221b4512d77d34fd8b14feae8a877"` |
| `/tested_inputs/30/path` | `"native/kernel/src/box3d_replay_conformance.cpp"` |
| `/tested_inputs/30/mode` | `"text_utf8_lf"` |
| `/tested_inputs/30/size_bytes` | `3833` |
| `/tested_inputs/30/sha256` | `"bc90cf1b14738b6dc897fa1b77f65b8d60530ecdbcde11d4753b29f4dfecb8cc"` |
| `/tested_inputs/31/path` | `"native/kernel/src/box3d_replay_conformance.hpp"` |
| `/tested_inputs/31/mode` | `"text_utf8_lf"` |
| `/tested_inputs/31/size_bytes` | `403` |
| `/tested_inputs/31/sha256` | `"607a01759002816a48ace7361ee82e26d2a6b20438dc32eac2dec306e110f8c3"` |
| `/tested_inputs/32/path` | `"native/kernel/src/physics_world.cpp"` |
| `/tested_inputs/32/mode` | `"text_utf8_lf"` |
| `/tested_inputs/32/size_bytes` | `67137` |
| `/tested_inputs/32/sha256` | `"da655462e410c3f6473fa3a9a6fd9135c193658c6bdd4f9eea874ca8c68f5a3d"` |
| `/tested_inputs/33/path` | `"native/kernel/src/physics_world_test_facade.hpp"` |
| `/tested_inputs/33/mode` | `"text_utf8_lf"` |
| `/tested_inputs/33/size_bytes` | `420` |
| `/tested_inputs/33/sha256` | `"7ddb6c079257a4e7c1a245fa043b54ab6d473d364eec3fa803a99b9afe913b2b"` |
| `/tested_inputs/34/path` | `"native/kernel/src/radial_gravity.cpp"` |
| `/tested_inputs/34/mode` | `"text_utf8_lf"` |
| `/tested_inputs/34/size_bytes` | `1863` |
| `/tested_inputs/34/sha256` | `"ef20acd6f17034cf8c5b017137796bea945fb9910d5c7ec7b8eff941b7b1ab46"` |
| `/tested_inputs/35/path` | `"native/kernel/src/scenario.cpp"` |
| `/tested_inputs/35/mode` | `"text_utf8_lf"` |
| `/tested_inputs/35/size_bytes` | `166432` |
| `/tested_inputs/35/sha256` | `"3349eb04913719a7f77229722333e894c290afa7d2ee592d89bf9b638916b90a"` |
| `/tested_inputs/36/path` | `"native/kernel/src/scenario_configuration.hpp"` |
| `/tested_inputs/36/mode` | `"text_utf8_lf"` |
| `/tested_inputs/36/size_bytes` | `1014` |
| `/tested_inputs/36/sha256` | `"1dce7723604301a83b37349f8a42969bfea8baa3ebe4a7c5b8ac3e9445331f7c"` |
| `/tested_inputs/37/path` | `"native/kernel/src/scenario_test_facade.hpp"` |
| `/tested_inputs/37/mode` | `"text_utf8_lf"` |
| `/tested_inputs/37/size_bytes` | `359` |
| `/tested_inputs/37/sha256` | `"161de04d8c791b5294c47d4a5a959384ce5f566584188c72ba984a4a9a16a1c0"` |
| `/tested_inputs/38/path` | `"native/simulation/CMakeLists.txt"` |
| `/tested_inputs/38/mode` | `"text_utf8_lf"` |
| `/tested_inputs/38/size_bytes` | `4549` |
| `/tested_inputs/38/sha256` | `"78a65fc503672d5ec696420a018b1a360b83602d66abf66a9a3ae0b68d0a104a"` |
| `/tested_inputs/39/path` | `"native/simulation/include/ninho/simulation/commands.hpp"` |
| `/tested_inputs/39/mode` | `"text_utf8_lf"` |
| `/tested_inputs/39/size_bytes` | `906` |
| `/tested_inputs/39/sha256` | `"be02293941c60d0e4d598a13bfb789a14515d77d0d977be83b1d657a5162537d"` |
| `/tested_inputs/40/path` | `"native/simulation/include/ninho/simulation/content.hpp"` |
| `/tested_inputs/40/mode` | `"text_utf8_lf"` |
| `/tested_inputs/40/size_bytes` | `7629` |
| `/tested_inputs/40/sha256` | `"575de7bd5a6a12a636197ba384cd22a3efc48e52069f496f148332ab3320b114"` |
| `/tested_inputs/41/path` | `"native/simulation/include/ninho/simulation/events.hpp"` |
| `/tested_inputs/41/mode` | `"text_utf8_lf"` |
| `/tested_inputs/41/size_bytes` | `1675` |
| `/tested_inputs/41/sha256` | `"b26bcbcb85c59394ade20e65f889976e0994fd17fe68dcfba6ad868d553591d5"` |
| `/tested_inputs/42/path` | `"native/simulation/include/ninho/simulation/session.hpp"` |
| `/tested_inputs/42/mode` | `"text_utf8_lf"` |
| `/tested_inputs/42/size_bytes` | `5196` |
| `/tested_inputs/42/sha256` | `"eb2b6a7261c322d71cd6a7e0680936282d6fb989923977f32222c0c72dd75205"` |
| `/tested_inputs/43/path` | `"native/simulation/src/canonical_state.cpp"` |
| `/tested_inputs/43/mode` | `"text_utf8_lf"` |
| `/tested_inputs/43/size_bytes` | `22123` |
| `/tested_inputs/43/sha256` | `"7db3a11f11ca40880c4689824341a9870803d39eaffdd4d90caff154671b0f7a"` |
| `/tested_inputs/44/path` | `"native/simulation/src/content.cpp"` |
| `/tested_inputs/44/mode` | `"text_utf8_lf"` |
| `/tested_inputs/44/size_bytes` | `63519` |
| `/tested_inputs/44/sha256` | `"1a92c0529f66cdd8a762cff1c93ae52be4276faab3ee1986a7118eeb2c600020"` |
| `/tested_inputs/45/path` | `"native/simulation/src/damage_system.cpp"` |
| `/tested_inputs/45/mode` | `"text_utf8_lf"` |
| `/tested_inputs/45/size_bytes` | `12792` |
| `/tested_inputs/45/sha256` | `"263bfb2aecf4100cda01f2e5a9b0b6d3220ff6193a10680f338bac8871dbe963"` |
| `/tested_inputs/46/path` | `"native/simulation/src/damage_system.hpp"` |
| `/tested_inputs/46/mode` | `"text_utf8_lf"` |
| `/tested_inputs/46/size_bytes` | `2015` |
| `/tested_inputs/46/sha256` | `"3ec71c08d1347854cae553be7f75269fa011b47f94958974836efbf6a6eb1b17"` |
| `/tested_inputs/47/path` | `"native/simulation/src/fracture_system.cpp"` |
| `/tested_inputs/47/mode` | `"text_utf8_lf"` |
| `/tested_inputs/47/size_bytes` | `15180` |
| `/tested_inputs/47/sha256` | `"0d19385c9256476cdc2c3773eedbcce8663f07405997507c1fbc29c3649ee960"` |
| `/tested_inputs/48/path` | `"native/simulation/src/gravity_field_ability_system.cpp"` |
| `/tested_inputs/48/mode` | `"text_utf8_lf"` |
| `/tested_inputs/48/size_bytes` | `6488` |
| `/tested_inputs/48/sha256` | `"f49f549bea6f67065f78a7aca587b539e760e3f9b477ad3c8285eca155a61735"` |
| `/tested_inputs/49/path` | `"native/simulation/src/launch_system.cpp"` |
| `/tested_inputs/49/mode` | `"text_utf8_lf"` |
| `/tested_inputs/49/size_bytes` | `25197` |
| `/tested_inputs/49/sha256` | `"32709496fcdbb14cac7982052dfd7c41f8503ba13feea0fa3cf409700bcee14d"` |
| `/tested_inputs/50/path` | `"native/simulation/src/material_mapping.hpp"` |
| `/tested_inputs/50/mode` | `"text_utf8_lf"` |
| `/tested_inputs/50/size_bytes` | `609` |
| `/tested_inputs/50/sha256` | `"bfc3f67e77dfec752fbf0f745a721475d0ed3f26dd4fcf865571306366542fcf"` |
| `/tested_inputs/51/path` | `"native/simulation/src/objective_system.cpp"` |
| `/tested_inputs/51/mode` | `"text_utf8_lf"` |
| `/tested_inputs/51/size_bytes` | `899` |
| `/tested_inputs/51/sha256` | `"82528ac4de52229c59b9d5773fbd6ae6de276692131f1a795d119c44c03e7356"` |
| `/tested_inputs/52/path` | `"native/simulation/src/session.cpp"` |
| `/tested_inputs/52/mode` | `"text_utf8_lf"` |
| `/tested_inputs/52/size_bytes` | `16157` |
| `/tested_inputs/52/sha256` | `"4ba94a74b769618e962717b86adb7a8bdbb706b2b6ee653e4112d6f95ecd760a"` |
| `/tested_inputs/53/path` | `"native/simulation/src/session_builder.cpp"` |
| `/tested_inputs/53/mode` | `"text_utf8_lf"` |
| `/tested_inputs/53/size_bytes` | `17518` |
| `/tested_inputs/53/sha256` | `"67c1ea77e1034340e253b46f9060c43fd5d2f5fed56ace8efd875f1f226714b2"` |
| `/tested_inputs/54/path` | `"native/simulation/src/session_internal.hpp"` |
| `/tested_inputs/54/mode` | `"text_utf8_lf"` |
| `/tested_inputs/54/size_bytes` | `6467` |
| `/tested_inputs/54/sha256` | `"670bd18fe94ecee1c6e3282bff050368e8f87e141ce5722a1ac8bd3caaf4853b"` |
| `/tested_inputs/55/path` | `"native/simulation/src/session_test_facade.hpp"` |
| `/tested_inputs/55/mode` | `"text_utf8_lf"` |
| `/tested_inputs/55/size_bytes` | `3042` |
| `/tested_inputs/55/sha256` | `"fecb6cd51c6fefba0adc238f04bdfc56dc429043b4a00e6ab07b0afcc7890679"` |
| `/tested_inputs/56/path` | `"native/simulation/tests/content_tests.cpp"` |
| `/tested_inputs/56/mode` | `"text_utf8_lf"` |
| `/tested_inputs/56/size_bytes` | `35677` |
| `/tested_inputs/56/sha256` | `"796a08013d6651a274acdaadb97f3f6eb9d1a20f218816c87edfbfab160ddddf"` |
| `/tested_inputs/57/path` | `"native/simulation/tests/damage_anchor_tests.cpp"` |
| `/tested_inputs/57/mode` | `"text_utf8_lf"` |
| `/tested_inputs/57/size_bytes` | `20494` |
| `/tested_inputs/57/sha256` | `"598f040d12c0c691bb6d7e21c783309c35722710a82e458d60f9e23ffde9bade"` |
| `/tested_inputs/58/path` | `"native/simulation/tests/fracture_objective_tests.cpp"` |
| `/tested_inputs/58/mode` | `"text_utf8_lf"` |
| `/tested_inputs/58/size_bytes` | `13161` |
| `/tested_inputs/58/sha256` | `"215c55898dde48408ffaac0a567797c5db5e0e1b039ffb0c489bc621e4eac1a7"` |
| `/tested_inputs/59/path` | `"native/simulation/tests/gravity_field_ability_tests.cpp"` |
| `/tested_inputs/59/mode` | `"text_utf8_lf"` |
| `/tested_inputs/59/size_bytes` | `23041` |
| `/tested_inputs/59/sha256` | `"300c30eb6018254854d066933b254fd47ad20e92f10a805397657303ff10e9e7"` |
| `/tested_inputs/60/path` | `"native/simulation/tests/kernel_contract_tests.cpp"` |
| `/tested_inputs/60/mode` | `"text_utf8_lf"` |
| `/tested_inputs/60/size_bytes` | `2743` |
| `/tested_inputs/60/sha256` | `"8882676ade374cdf8b76d1866e47e4adeeaf76c78f664002c5c5be093b5eae85"` |
| `/tested_inputs/61/path` | `"native/simulation/tests/launch_fsm_tests.cpp"` |
| `/tested_inputs/61/mode` | `"text_utf8_lf"` |
| `/tested_inputs/61/size_bytes` | `34247` |
| `/tested_inputs/61/sha256` | `"60bf5278a375a18024aa2adb8bec7e3dc2d98602b31602f1cc9adfcb355f089f"` |
| `/tested_inputs/62/path` | `"native/simulation/tests/legacy_orbital_characterization_tests.cpp"` |
| `/tested_inputs/62/mode` | `"text_utf8_lf"` |
| `/tested_inputs/62/size_bytes` | `17894` |
| `/tested_inputs/62/sha256` | `"9100dd4290b529b36ec6cb11fcedd486bb0828760e644e2b2ed836cf21117a8b"` |
| `/tested_inputs/63/path` | `"native/simulation/tests/playthrough_tests.cpp"` |
| `/tested_inputs/63/mode` | `"text_utf8_lf"` |
| `/tested_inputs/63/size_bytes` | `20246` |
| `/tested_inputs/63/sha256` | `"d0d0987f9ab8dca70d6f18cb185bf84ee7eabff2f3ff21f56e2cec382e6575f9"` |
| `/tested_inputs/64/path` | `"native/simulation/tests/session_tests.cpp"` |
| `/tested_inputs/64/mode` | `"text_utf8_lf"` |
| `/tested_inputs/64/size_bytes` | `41736` |
| `/tested_inputs/64/sha256` | `"1a8ee368a9e0bb3afb69fa58f2579dbbe0870ed7a55c07ea6953ff21fc698ec7"` |
| `/tested_inputs/65/path` | `"native/simulation/tests/test_framework.hpp"` |
| `/tested_inputs/65/mode` | `"text_utf8_lf"` |
| `/tested_inputs/65/size_bytes` | `1870` |
| `/tested_inputs/65/sha256` | `"a2dcdad0a827c7f557a891e8dbf2aa131f9f377f1176f14f1110ccc87079cc77"` |
| `/tested_inputs/66/path` | `"native/simulation/tests/test_main.cpp"` |
| `/tested_inputs/66/mode` | `"text_utf8_lf"` |
| `/tested_inputs/66/size_bytes` | `1240` |
| `/tested_inputs/66/sha256` | `"0fe1a9742c2b031787df553caa0d1d513decea1dd45c1ef3054eb23a640dddfb"` |
| `/tested_inputs/67/path` | `"native/spike/CMakeLists.txt"` |
| `/tested_inputs/67/mode` | `"text_utf8_lf"` |
| `/tested_inputs/67/size_bytes` | `518` |
| `/tested_inputs/67/sha256` | `"4244861de1fc0f84e85e96d44cbd5e6f1eb9c877b31a743c7c6660c5b3fdf3bd"` |
| `/tested_inputs/68/path` | `"native/spike/main.cpp"` |
| `/tested_inputs/68/mode` | `"text_utf8_lf"` |
| `/tested_inputs/68/size_bytes` | `13396` |
| `/tested_inputs/68/sha256` | `"e72ceeab9c929bf6fee638de840a9eac8fe074681c3517e023c094ee4374cbdd"` |
| `/tested_inputs/69/path` | `"native/tests/CMakeLists.txt"` |
| `/tested_inputs/69/mode` | `"text_utf8_lf"` |
| `/tested_inputs/69/size_bytes` | `4134` |
| `/tested_inputs/69/sha256` | `"d36e5eebdf52db4c8fa4d6fb00e5bce434440ebefcfdb9950fb6ef0ce9bd9e22"` |
| `/tested_inputs/70/path` | `"native/tests/capability_tests.cpp"` |
| `/tested_inputs/70/mode` | `"text_utf8_lf"` |
| `/tested_inputs/70/size_bytes` | `27540` |
| `/tested_inputs/70/sha256` | `"d495449638982624fae7d6b5ad98bbb8461dce45383175832f77ce23c50877a1"` |
| `/tested_inputs/71/path` | `"native/tests/determinism_tests.cpp"` |
| `/tested_inputs/71/mode` | `"text_utf8_lf"` |
| `/tested_inputs/71/size_bytes` | `13429` |
| `/tested_inputs/71/sha256` | `"0798f4b49c3cc52796d498d90062ee16eb85653c28335e11444c581d83399095"` |
| `/tested_inputs/72/path` | `"native/tests/json_smoke.py"` |
| `/tested_inputs/72/mode` | `"text_utf8_lf"` |
| `/tested_inputs/72/size_bytes` | `18266` |
| `/tested_inputs/72/sha256` | `"0ec7e8dcb533806ecafb810d25b5c68be8d22e964d76e676625b713d7204915b"` |
| `/tested_inputs/73/path` | `"native/tests/pile_stability_tests.cpp"` |
| `/tested_inputs/73/mode` | `"text_utf8_lf"` |
| `/tested_inputs/73/size_bytes` | `19530` |
| `/tested_inputs/73/sha256` | `"d37aa0e19bdb2cc0517feb62b664dd96b948e8f762499e11cb250e6271a01e5e"` |
| `/tested_inputs/74/path` | `"native/tests/projectile_ccd_tests.cpp"` |
| `/tested_inputs/74/mode` | `"text_utf8_lf"` |
| `/tested_inputs/74/size_bytes` | `5898` |
| `/tested_inputs/74/sha256` | `"7a620a39da06923e1b7ef5eedd8d49aefe6c6f66e414431cce409093232478d5"` |
| `/tested_inputs/75/path` | `"native/tests/radial_gravity_tests.cpp"` |
| `/tested_inputs/75/mode` | `"text_utf8_lf"` |
| `/tested_inputs/75/size_bytes` | `6247` |
| `/tested_inputs/75/sha256` | `"e9732b98d3c545af0d0a6067c566c5767e50f4d3d54b4042c5c675653919d278"` |
| `/tested_inputs/76/path` | `"native/tests/scenario_capability_tests.cpp"` |
| `/tested_inputs/76/mode` | `"text_utf8_lf"` |
| `/tested_inputs/76/size_bytes` | `12233` |
| `/tested_inputs/76/sha256` | `"da0b07669a5499b56b758a7c3ae07150d9b83af1e375cf13d3f529ab63519191"` |
| `/tested_inputs/77/path` | `"native/tests/test_framework.hpp"` |
| `/tested_inputs/77/mode` | `"text_utf8_lf"` |
| `/tested_inputs/77/size_bytes` | `3899` |
| `/tested_inputs/77/sha256` | `"76e694857c118ca7d3007d349637a52bf9b321228fdd81a8408f9547b81ee1e6"` |
| `/tested_inputs/78/path` | `"native/tests/test_main.cpp"` |
| `/tested_inputs/78/mode` | `"text_utf8_lf"` |
| `/tested_inputs/78/size_bytes` | `2566` |
| `/tested_inputs/78/sha256` | `"372df787fbb6fae307ba0ee2c862ef043913b264760358e86ce81903eb9b4626"` |
| `/tested_inputs/79/path` | `"native/tests/world_lifecycle_tests.cpp"` |
| `/tested_inputs/79/mode` | `"text_utf8_lf"` |
| `/tested_inputs/79/size_bytes` | `12095` |
| `/tested_inputs/79/sha256` | `"da2fab9c92a64d572c04e2311dca0bfa0343e9738dcbdb5ecdb16d884e9dceed"` |
| `/tested_inputs/80/path` | `"tools/FoundationEvidenceGate.psm1"` |
| `/tested_inputs/80/mode` | `"text_utf8_lf"` |
| `/tested_inputs/80/size_bytes` | `18698` |
| `/tested_inputs/80/sha256` | `"195d0cc069d74115ed1c1ab63f1c99909e8a1ba5a4e2a807a66b62a3ca729a60"` |
| `/tested_inputs/81/path` | `"tools/GodotSmokeRegistry.psm1"` |
| `/tested_inputs/81/mode` | `"text_utf8_lf"` |
| `/tested_inputs/81/size_bytes` | `5268` |
| `/tested_inputs/81/sha256` | `"82319b1f009954a5632b7991d4ca291091d2619d762ff5df44b4cc0330857b32"` |
| `/tested_inputs/82/path` | `"tools/GodotSpikeGate.psm1"` |
| `/tested_inputs/82/mode` | `"text_utf8_lf"` |
| `/tested_inputs/82/size_bytes` | `11080` |
| `/tested_inputs/82/sha256` | `"f81ed257e02861aa6f84aace45d39f33d51ceef740d09f149d5c0363087f4b1c"` |
| `/tested_inputs/83/path` | `"tools/Invoke-Native.ps1"` |
| `/tested_inputs/83/mode` | `"text_utf8_lf"` |
| `/tested_inputs/83/size_bytes` | `2185` |
| `/tested_inputs/83/sha256` | `"4cba4036cbc5007bddb3762f5b1c45f72fc63ef63d6b6dbb16c8b0a6ed9344d4"` |
| `/tested_inputs/84/path` | `"tools/SafePath.psm1"` |
| `/tested_inputs/84/mode` | `"text_utf8_lf"` |
| `/tested_inputs/84/size_bytes` | `1841` |
| `/tested_inputs/84/sha256` | `"d6a5c2daeeb3ef4369daa35bf21d6dc60c3ec736ab8c932078b3e24bd32e4ffc"` |
| `/tested_inputs/85/path` | `"tools/SpikeEvidenceValidation.psm1"` |
| `/tested_inputs/85/mode` | `"text_utf8_lf"` |
| `/tested_inputs/85/size_bytes` | `51353` |
| `/tested_inputs/85/sha256` | `"847f52d2501bf25b5136797e71600c4934bf1c03b9ba454f483e73422bd1ddd9"` |
| `/tested_inputs/86/path` | `"tools/SpikeReportGate.psm1"` |
| `/tested_inputs/86/mode` | `"text_utf8_lf"` |
| `/tested_inputs/86/size_bytes` | `2400` |
| `/tested_inputs/86/sha256` | `"762cc1cd1e6740f4eb5745511d9a565a11d7835bac863f71374e44ded9c11077"` |
| `/tested_inputs/87/path` | `"tools/TestedInputIdentity.psm1"` |
| `/tested_inputs/87/mode` | `"text_utf8_lf"` |
| `/tested_inputs/87/size_bytes` | `6567` |
| `/tested_inputs/87/sha256` | `"45160531c01d36f7eb1c0d3d2a115fd6d7541de7f2883081b5fb57a7f5cb0898"` |
| `/tested_inputs/88/path` | `"tools/ToolchainIntegrity.psm1"` |
| `/tested_inputs/88/mode` | `"text_utf8_lf"` |
| `/tested_inputs/88/size_bytes` | `3009` |
| `/tested_inputs/88/sha256` | `"c3fc2f6b8d97c27e2f1d5887c05de4c91cfa8f4f6b814c21208fb3f6d533dc4c"` |
| `/tested_inputs/89/path` | `"tools/UpstreamBox3DGate.psm1"` |
| `/tested_inputs/89/mode` | `"text_utf8_lf"` |
| `/tested_inputs/89/size_bytes` | `7219` |
| `/tested_inputs/89/sha256` | `"324fd58c0dcee1ea712e36df53390ee4deff6faac604e4f32894f57a756c7163"` |
| `/tested_inputs/90/path` | `"tools/VerticalSliceGate.psm1"` |
| `/tested_inputs/90/mode` | `"text_utf8_lf"` |
| `/tested_inputs/90/size_bytes` | `64476` |
| `/tested_inputs/90/sha256` | `"624df6fdbc6d6ce9a639b3bc972e3ed4c1ac821e5ef9b78164134ef8292bac97"` |
| `/tested_inputs/91/path` | `"tools/bootstrap.ps1"` |
| `/tested_inputs/91/mode` | `"text_utf8_lf"` |
| `/tested_inputs/91/size_bytes` | `15698` |
| `/tested_inputs/91/sha256` | `"df335d69a457724bfc9d8ec05463adde0728feb6c217c772f910777715f80246"` |
| `/tested_inputs/92/path` | `"tools/box3d-v0.1.0-compile-sources.txt"` |
| `/tested_inputs/92/mode` | `"text_utf8_lf"` |
| `/tested_inputs/92/size_bytes` | `1339` |
| `/tested_inputs/92/sha256` | `"8bc4897fbc25895f75fab44e541c12d3b0e528e2692bbabbe9df632871a7d1f0"` |
| `/tested_inputs/93/path` | `"tools/build.ps1"` |
| `/tested_inputs/93/mode` | `"text_utf8_lf"` |
| `/tested_inputs/93/size_bytes` | `1456` |
| `/tested_inputs/93/sha256` | `"5faf578e2d23965aa2c5f87d4cb86d74c9605e842ebb38dcacccaefcff185241"` |
| `/tested_inputs/94/path` | `"tools/generate_foundation_report.py"` |
| `/tested_inputs/94/mode` | `"text_utf8_lf"` |
| `/tested_inputs/94/size_bytes` | `22194` |
| `/tested_inputs/94/sha256` | `"12f47d5ff5d7f6786f78e1bf4e24b18eec01c2a3a7b74544d6116eb350efc740"` |
| `/tested_inputs/95/path` | `"tools/run_spike.ps1"` |
| `/tested_inputs/95/mode` | `"text_utf8_lf"` |
| `/tested_inputs/95/size_bytes` | `2709` |
| `/tested_inputs/95/sha256` | `"d571b34deb9f4cf708da2942b892016a13edae05472133b0e6887b3e4050b36d"` |
| `/tested_inputs/96/path` | `"tools/test.ps1"` |
| `/tested_inputs/96/mode` | `"text_utf8_lf"` |
| `/tested_inputs/96/size_bytes` | `23185` |
| `/tested_inputs/96/sha256` | `"e2220526d429cab96ce083db8c86fda7e57bbd424d0951b93e694418308d7a35"` |
| `/tested_inputs/97/path` | `"tools/toolchain.lock.json"` |
| `/tested_inputs/97/mode` | `"text_utf8_lf"` |
| `/tested_inputs/97/size_bytes` | `3047` |
| `/tested_inputs/97/sha256` | `"4242a98bf001fb9064227baf5f77ce5f28bac3abdd81c17ae2b3399b31d4cb01"` |

## Capability Matrix

Debug contains 8 capability proofs.

### Debug capability `ccd_dynamic_dynamic`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"all 20 fixed seeds contacted a dynamic pile body at 35 m/s and four substeps"` |
| `peak_body_count` | `123` |
| `peak_shape_count` | `123` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `121` |
| `peak_contact_count` | `221` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `primary_passes` | `20` | `"seeds"` |
| `fallback_passes` | `0` | `"seeds"` |
| `primary_speed` | `35` | `"m/s"` |
| `primary_substeps` | `4` | `"count"` |
| `fallback_speed` | `30` | `"m/s"` |
| `fallback_substeps` | `6` | `"count"` |
| `fallback_evaluated` | `0` | `"bool"` |
| `primary_invalid_states` | `0` | `"count"` |
| `fallback_invalid_states` | `0` | `"count"` |

Fixture hashes:

- `1431096453509785832`, `5466060815846980033`, `8526278817898170285`, `2675255426310783444`, `10494942409035688541`, `7390650343477109358`, `14725012844580521641`, `14001126938762718496`, `2400058324701230849`, `10732887959698602526`, `5915366692680379584`, `14334879908416984375`, `6577404406665493359`, `16513243213841685896`, `9721495563768596328`, `13937513265211812359`, `9628435969084559192`, `18192646134465658732`, `18426867884952182109`, `11807578939564631579`

### Debug capability `shape_cast_overlap`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"the first three-meter sphere cast handle agrees with overlap at contact"` |
| `peak_body_count` | `1` |
| `peak_shape_count` | `1` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `0` |
| `peak_contact_count` | `0` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `cast_distance` | `3` | `"m"` |
| `first_handle_index` | `1` | `"index"` |
| `first_handle_matches_overlap` | `1` | `"bool"` |
| `fraction` | `0.810775876045227` | `"ratio"` |

Fixture hashes:

- `8388802905423810155`

### Debug capability `contact_hit_events`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"real hit data is finite, energetic, material-tagged, and pair-unique at six substeps"` |
| `peak_body_count` | `2` |
| `peak_shape_count` | `2` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `2` |
| `peak_contact_count` | `1` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `approach_speed` | `15` | `"m/s"` |
| `effective_mass` | `41.88790512084961` | `"kg"` |
| `derived_energy` | `4105.0146484375` | `"J"` |
| `normal_length` | `1` | `"ratio"` |
| `material_a` | `111` | `"id"` |
| `material_b` | `222` | `"id"` |
| `unique_pairs` | `1` | `"count"` |
| `substeps` | `6` | `"count"` |

Fixture hashes:

- `10155543446163919611`

### Debug capability `joint_force_torque`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"joint reaction grows within 50 N tolerance and crosses 10 kN"` |
| `peak_body_count` | `2` |
| `peak_shape_count` | `2` |
| `peak_joint_count` | `1` |
| `peak_awake_count` | `1` |
| `peak_contact_count` | `1` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `monotonic_tolerance` | `50` | `"N"` |
| `maximum_force` | `12693.1640625` | `"N"` |
| `rupture_threshold` | `10000` | `"N"` |
| `maximum_deformation` | `0.01363062858581543` | `"m"` |
| `deformation_threshold` | `0.01` | `"m"` |
| `consecutive_deformation_ticks` | `0` | `"ticks"` |

Fixture hashes:

- `14239377403397705414`

### Debug capability `hulls_compounds`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"one compound containing eight hull children has valid mass, bounds, and contact"` |
| `peak_body_count` | `2` |
| `peak_shape_count` | `9` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `1` |
| `peak_contact_count` | `8` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `hull_count` | `8` | `"count"` |
| `expected_mass` | `42.666666666666664` | `"kg"` |
| `mass` | `42.66667175292969` | `"kg"` |
| `bounds_lower_x` | `-1.6200000047683716` | `"m"` |
| `bounds_lower_y` | `2.7799999713897705` | `"m"` |
| `bounds_lower_z` | `-0.2199999988079071` | `"m"` |
| `bounds_upper_x` | `1.6200000047683716` | `"m"` |
| `bounds_upper_y` | `3.2200000286102295` | `"m"` |
| `bounds_upper_z` | `0.2199999988079071` | `"m"` |
| `bounds_tolerance` | `0.0001` | `"m"` |
| `contacted` | `1` | `"bool"` |
| `state_valid` | `1` | `"bool"` |
| `box3d_allocator_baseline_bytes` | `0` | `"bytes"` |
| `box3d_allocator_final_bytes` | `0` | `"bytes"` |

Fixture hashes:

- `7710058609812386530`

### Debug capability `radial_sleep`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"the public radial-gravity pile satisfies every fixed sleep limit"` |
| `peak_body_count` | `81` |
| `peak_shape_count` | `81` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `80` |
| `peak_contact_count` | `204` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `sleep_ratio` | `1` | `"ratio"` |
| `p95_linear_speed` | `0` | `"m/s"` |
| `p95_angular_speed` | `0` | `"rad/s"` |
| `max_penetration` | `0.0015351474285125732` | `"m"` |
| `energy_growth` | `0` | `"ratio"` |

Fixture hashes:

- `10363635776067367757`

### Debug capability `batch_lifecycle`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"10,000 create-destroy cycles preserved handle generations without persistent growth"` |
| `peak_body_count` | `1` |
| `peak_shape_count` | `1` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `1` |
| `peak_contact_count` | `0` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `generation_cycles` | `10000` | `"count"` |
| `invalid_handles` | `0` | `"count"` |
| `private_commit_available` | `1` | `"bool"` |
| `private_commit_growth` | `0` | `"ratio"` |
| `private_commit_baseline_bytes` | `4562944` | `"bytes"` |
| `private_commit_final_bytes` | `4562944` | `"bytes"` |
| `working_set_baseline_bytes` | `7446528` | `"bytes"` |
| `working_set_final_bytes` | `7446528` | `"bytes"` |
| `crt_normal_count_delta` | `0` | `"count"` |
| `crt_normal_bytes_delta` | `0` | `"bytes"` |
| `crt_client_count_delta` | `0` | `"count"` |
| `crt_client_bytes_delta` | `0` | `"bytes"` |
| `box3d_allocator_baseline_bytes` | `0` | `"bytes"` |
| `box3d_allocator_final_bytes` | `0` | `"bytes"` |

Fixture hashes:

- `43224550866945`

### Debug capability `upstream_replay`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"the pinned official recorder saved, loaded, and validated a minimal replay"` |
| `peak_body_count` | `1` |
| `peak_shape_count` | `1` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `1` |
| `peak_contact_count` | `0` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `saved` | `1` | `"bool"` |
| `loaded` | `1` | `"bool"` |
| `validated` | `1` | `"bool"` |
| `recording_bytes` | `6982` | `"bytes"` |
| `temporary_file_removed` | `1` | `"bool"` |
| `box3d_allocator_baseline_bytes` | `0` | `"bytes"` |
| `box3d_allocator_final_bytes` | `0` | `"bytes"` |

Fixture hashes:

- `13184605773762244167`

Release contains 8 capability proofs.

### Release capability `ccd_dynamic_dynamic`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"all 20 fixed seeds contacted a dynamic pile body at 35 m/s and four substeps"` |
| `peak_body_count` | `123` |
| `peak_shape_count` | `123` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `121` |
| `peak_contact_count` | `221` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `primary_passes` | `20` | `"seeds"` |
| `fallback_passes` | `0` | `"seeds"` |
| `primary_speed` | `35` | `"m/s"` |
| `primary_substeps` | `4` | `"count"` |
| `fallback_speed` | `30` | `"m/s"` |
| `fallback_substeps` | `6` | `"count"` |
| `fallback_evaluated` | `0` | `"bool"` |
| `primary_invalid_states` | `0` | `"count"` |
| `fallback_invalid_states` | `0` | `"count"` |

Fixture hashes:

- `1431096453509785832`, `5466060815846980033`, `8526278817898170285`, `2675255426310783444`, `10494942409035688541`, `7390650343477109358`, `14725012844580521641`, `14001126938762718496`, `2400058324701230849`, `10732887959698602526`, `5915366692680379584`, `14334879908416984375`, `6577404406665493359`, `16513243213841685896`, `9721495563768596328`, `13937513265211812359`, `9628435969084559192`, `18192646134465658732`, `18426867884952182109`, `11807578939564631579`

### Release capability `shape_cast_overlap`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"the first three-meter sphere cast handle agrees with overlap at contact"` |
| `peak_body_count` | `1` |
| `peak_shape_count` | `1` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `0` |
| `peak_contact_count` | `0` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `cast_distance` | `3` | `"m"` |
| `first_handle_index` | `1` | `"index"` |
| `first_handle_matches_overlap` | `1` | `"bool"` |
| `fraction` | `0.810775876045227` | `"ratio"` |

Fixture hashes:

- `8388802905423810155`

### Release capability `contact_hit_events`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"real hit data is finite, energetic, material-tagged, and pair-unique at six substeps"` |
| `peak_body_count` | `2` |
| `peak_shape_count` | `2` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `2` |
| `peak_contact_count` | `1` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `approach_speed` | `15` | `"m/s"` |
| `effective_mass` | `41.88790512084961` | `"kg"` |
| `derived_energy` | `4105.0146484375` | `"J"` |
| `normal_length` | `1` | `"ratio"` |
| `material_a` | `111` | `"id"` |
| `material_b` | `222` | `"id"` |
| `unique_pairs` | `1` | `"count"` |
| `substeps` | `6` | `"count"` |

Fixture hashes:

- `10155543446163919611`

### Release capability `joint_force_torque`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"joint reaction grows within 50 N tolerance and crosses 10 kN"` |
| `peak_body_count` | `2` |
| `peak_shape_count` | `2` |
| `peak_joint_count` | `1` |
| `peak_awake_count` | `1` |
| `peak_contact_count` | `1` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `monotonic_tolerance` | `50` | `"N"` |
| `maximum_force` | `12693.1640625` | `"N"` |
| `rupture_threshold` | `10000` | `"N"` |
| `maximum_deformation` | `0.01363062858581543` | `"m"` |
| `deformation_threshold` | `0.01` | `"m"` |
| `consecutive_deformation_ticks` | `0` | `"ticks"` |

Fixture hashes:

- `14239377403397705414`

### Release capability `hulls_compounds`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"one compound containing eight hull children has valid mass, bounds, and contact"` |
| `peak_body_count` | `2` |
| `peak_shape_count` | `9` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `1` |
| `peak_contact_count` | `8` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `hull_count` | `8` | `"count"` |
| `expected_mass` | `42.666666666666664` | `"kg"` |
| `mass` | `42.66667175292969` | `"kg"` |
| `bounds_lower_x` | `-1.6200000047683716` | `"m"` |
| `bounds_lower_y` | `2.7799999713897705` | `"m"` |
| `bounds_lower_z` | `-0.2199999988079071` | `"m"` |
| `bounds_upper_x` | `1.6200000047683716` | `"m"` |
| `bounds_upper_y` | `3.2200000286102295` | `"m"` |
| `bounds_upper_z` | `0.2199999988079071` | `"m"` |
| `bounds_tolerance` | `0.0001` | `"m"` |
| `contacted` | `1` | `"bool"` |
| `state_valid` | `1` | `"bool"` |
| `box3d_allocator_baseline_bytes` | `0` | `"bytes"` |
| `box3d_allocator_final_bytes` | `0` | `"bytes"` |

Fixture hashes:

- `7710058609812386530`

### Release capability `radial_sleep`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"the public radial-gravity pile satisfies every fixed sleep limit"` |
| `peak_body_count` | `81` |
| `peak_shape_count` | `81` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `80` |
| `peak_contact_count` | `204` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `sleep_ratio` | `1` | `"ratio"` |
| `p95_linear_speed` | `0` | `"m/s"` |
| `p95_angular_speed` | `0` | `"rad/s"` |
| `max_penetration` | `0.0015351474285125732` | `"m"` |
| `energy_growth` | `0` | `"ratio"` |

Fixture hashes:

- `10363635776067367757`

### Release capability `batch_lifecycle`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"10,000 create-destroy cycles preserved handle generations without persistent growth"` |
| `peak_body_count` | `1` |
| `peak_shape_count` | `1` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `1` |
| `peak_contact_count` | `0` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `generation_cycles` | `10000` | `"count"` |
| `invalid_handles` | `0` | `"count"` |
| `private_commit_available` | `1` | `"bool"` |
| `private_commit_growth` | `0` | `"ratio"` |
| `private_commit_baseline_bytes` | `2097152` | `"bytes"` |
| `private_commit_final_bytes` | `2097152` | `"bytes"` |
| `working_set_baseline_bytes` | `5214208` | `"bytes"` |
| `working_set_final_bytes` | `5214208` | `"bytes"` |
| `crt_normal_count_delta` | `0` | `"count"` |
| `crt_normal_bytes_delta` | `0` | `"bytes"` |
| `crt_client_count_delta` | `0` | `"count"` |
| `crt_client_bytes_delta` | `0` | `"bytes"` |
| `box3d_allocator_baseline_bytes` | `0` | `"bytes"` |
| `box3d_allocator_final_bytes` | `0` | `"bytes"` |

Fixture hashes:

- `43224550866945`

### Release capability `upstream_replay`

| Field | Value |
| --- | --- |
| `status` | `"pass"` |
| `fallback` | `null` |
| `functional_status` | `"pass"` |
| `functional_fallback` | `null` |
| `detail` | `"the pinned official recorder saved, loaded, and validated a minimal replay"` |
| `peak_body_count` | `1` |
| `peak_shape_count` | `1` |
| `peak_joint_count` | `0` |
| `peak_awake_count` | `1` |
| `peak_contact_count` | `0` |

Measured values:

| Name | Value | Unit |
| --- | ---: | --- |
| `saved` | `1` | `"bool"` |
| `loaded` | `1` | `"bool"` |
| `validated` | `1` | `"bool"` |
| `recording_bytes` | `6982` | `"bytes"` |
| `temporary_file_removed` | `1` | `"bool"` |
| `box3d_allocator_baseline_bytes` | `0` | `"bytes"` |
| `box3d_allocator_final_bytes` | `0` | `"bytes"` |

Fixture hashes:

- `13184605773762244167`

## Scenario Metrics

Every leaf below is emitted in source order from the normative scenario payload. This includes identity, topology, timings, hashes, repeat observations, metrics with units, limits, memory, allocator, CRT, warnings, violations, fallbacks, and embedded matrix proofs.

### Debug scenario `radial_fall`

Topology fixture `dynamic/shape/joint`: `1/2/0`. Observed peak `body/shape/joint;awake/contact`: `2/2/0;1/1`.

| JSON pointer | Value |
| --- | --- |
| `/name` | `"radial_fall"` |
| `/seed` | `1` |
| `/substeps` | `4` |
| `/ticks` | `600` |
| `/final_hash` | `12100112409900625846` |
| `/hashes/0` | `12100112409900625846` |
| `/hashes/1` | `12100112409900625846` |
| `/repeat_observations/0/repeat` | `1` |
| `/repeat_observations/0/hash` | `12100112409900625846` |
| `/repeat_observations/0/peak_body_count` | `2` |
| `/repeat_observations/0/peak_shape_count` | `2` |
| `/repeat_observations/0/peak_joint_count` | `0` |
| `/repeat_observations/0/peak_awake_count` | `1` |
| `/repeat_observations/0/peak_contact_count` | `1` |
| `/repeat_observations/0/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/0/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `false` |
| `/repeat_observations/0/memory/private_commit/stable` | `false` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/available` | `false` |
| `/repeat_observations/0/memory/working_set/stable` | `false` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/0/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/0/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/0/crt/applicable` | `false` |
| `/repeat_observations/0/crt/balanced` | `true` |
| `/repeat_observations/0/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/0/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/0/crt/client_block_count_delta` | `0` |
| `/repeat_observations/0/crt/client_block_bytes_delta` | `0` |
| `/repeat_observations/1/repeat` | `2` |
| `/repeat_observations/1/hash` | `12100112409900625846` |
| `/repeat_observations/1/peak_body_count` | `2` |
| `/repeat_observations/1/peak_shape_count` | `2` |
| `/repeat_observations/1/peak_joint_count` | `0` |
| `/repeat_observations/1/peak_awake_count` | `1` |
| `/repeat_observations/1/peak_contact_count` | `1` |
| `/repeat_observations/1/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/1/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/gate_applied` | `false` |
| `/repeat_observations/1/memory/budget_qualified` | `false` |
| `/repeat_observations/1/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/private_commit/available` | `false` |
| `/repeat_observations/1/memory/private_commit/stable` | `false` |
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/available` | `false` |
| `/repeat_observations/1/memory/working_set/stable` | `false` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/1/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/1/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/1/crt/applicable` | `false` |
| `/repeat_observations/1/crt/balanced` | `true` |
| `/repeat_observations/1/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/1/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/1/crt/client_block_count_delta` | `0` |
| `/repeat_observations/1/crt/client_block_bytes_delta` | `0` |
| `/contact_before_pile_exit` | `false` |
| `/ccd_primary_pass_count` | `0` |
| `/ccd_fallback_pass_count` | `0` |
| `/projectile_speed` | `0` |
| `/surface_separation` | `-5.951523780822754e-05` |
| `/final_linear_speed` | `0` |
| `/final_angular_speed` | `0` |
| `/p95_linear_speed` | `0` |
| `/p95_angular_speed` | `0` |
| `/sleep_ratio` | `0` |
| `/max_penetration` | `0` |
| `/minimum_density` | `0` |
| `/maximum_density` | `0` |
| `/energy_at_tick_600` | `0` |
| `/max_energy_growth_ratio` | `0` |
| `/dynamic_body_count` | `1` |
| `/shape_count` | `2` |
| `/joint_count` | `0` |
| `/peak_body_count` | `2` |
| `/peak_shape_count` | `2` |
| `/peak_joint_count` | `0` |
| `/peak_awake_count` | `1` |
| `/peak_contact_count` | `1` |
| `/step_ms/min` | `0.0016` |
| `/step_ms/p50` | `0.0017` |
| `/step_ms/p95` | `0.06` |
| `/step_ms/max` | `0.3207` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unavailable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `false` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `false` |
| `/memory/private_commit/baseline_last_bytes` | `0` |
| `/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/memory/private_commit/baseline_median_bytes` | `0` |
| `/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/memory/private_commit/final_last_bytes` | `0` |
| `/memory/private_commit/final_full_min_bytes` | `0` |
| `/memory/private_commit/final_central_min_bytes` | `0` |
| `/memory/private_commit/final_median_bytes` | `0` |
| `/memory/private_commit/final_central_max_bytes` | `0` |
| `/memory/private_commit/final_full_max_bytes` | `0` |
| `/memory/private_commit/peak_bytes` | `0` |
| `/memory/private_commit/growth_ratio` | `0` |
| `/memory/private_commit/instant_growth_ratio` | `0` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/memory/private_commit/measured_full_span_ratio` | `0` |
| `/memory/private_commit/warmup_samples` | `[]` |
| `/memory/private_commit/measured_samples` | `[]` |
| `/memory/working_set/available` | `false` |
| `/memory/working_set/stable` | `false` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"unavailable"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `0` |
| `/memory/working_set/baseline_full_min_bytes` | `0` |
| `/memory/working_set/baseline_central_min_bytes` | `0` |
| `/memory/working_set/baseline_median_bytes` | `0` |
| `/memory/working_set/baseline_central_max_bytes` | `0` |
| `/memory/working_set/baseline_full_max_bytes` | `0` |
| `/memory/working_set/final_last_bytes` | `0` |
| `/memory/working_set/final_full_min_bytes` | `0` |
| `/memory/working_set/final_central_min_bytes` | `0` |
| `/memory/working_set/final_median_bytes` | `0` |
| `/memory/working_set/final_central_max_bytes` | `0` |
| `/memory/working_set/final_full_max_bytes` | `0` |
| `/memory/working_set/peak_bytes` | `0` |
| `/memory/working_set/growth_ratio` | `0` |
| `/memory/working_set/instant_growth_ratio` | `0` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/memory/working_set/warmup_full_span_ratio` | `0` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/memory/working_set/measured_full_span_ratio` | `0` |
| `/memory/working_set/warmup_samples` | `[]` |
| `/memory/working_set/measured_samples` | `[]` |
| `/box3d_allocator/baseline_bytes` | `0` |
| `/box3d_allocator/final_bytes` | `0` |
| `/box3d_allocator/max_abs_delta` | `0` |
| `/box3d_allocator/exact_return` | `true` |
| `/box3d_allocator/warmup_post_teardown` | `[]` |
| `/box3d_allocator/measured_post_teardown` | `[]` |
| `/crt/applicable` | `false` |
| `/crt/balanced` | `true` |
| `/crt/normal_block_count_delta` | `0` |
| `/crt/normal_block_bytes_delta` | `0` |
| `/crt/client_block_count_delta` | `0` |
| `/crt/client_block_bytes_delta` | `0` |
| `/warmup_ticks` | `0` |
| `/measurement_ticks` | `0` |
| `/allocator_warmup_cycles` | `0` |
| `/stress_cycles` | `0` |
| `/fallback` | `""` |
| `/limits/0/name` | `"surface_separation_min"` |
| `/limits/0/comparison` | `">="` |
| `/limits/0/value` | `-0.02` |
| `/limits/0/unit` | `"m"` |
| `/limits/1/name` | `"surface_separation_max"` |
| `/limits/1/comparison` | `"<="` |
| `/limits/1/value` | `0.03` |
| `/limits/1/unit` | `"m"` |
| `/limits/2/name` | `"final_linear_speed"` |
| `/limits/2/comparison` | `"<"` |
| `/limits/2/value` | `0.05` |
| `/limits/2/unit` | `"m/s"` |
| `/limits/3/name` | `"final_angular_speed"` |
| `/limits/3/comparison` | `"<"` |
| `/limits/3/value` | `0.1` |
| `/limits/3/unit` | `"rad/s"` |
| `/limits/4/name` | `"max_energy_growth_ratio"` |
| `/limits/4/comparison` | `"<="` |
| `/limits/4/value` | `0.02` |
| `/limits/4/unit` | `"ratio"` |
| `/metrics/0/name` | `"surface_separation"` |
| `/metrics/0/value` | `-5.951523780822754e-05` |
| `/metrics/0/unit` | `"m"` |
| `/metrics/1/name` | `"final_linear_speed"` |
| `/metrics/1/value` | `0` |
| `/metrics/1/unit` | `"m/s"` |
| `/metrics/2/name` | `"final_angular_speed"` |
| `/metrics/2/value` | `0` |
| `/metrics/2/unit` | `"rad/s"` |
| `/metrics/3/name` | `"max_energy_growth_ratio"` |
| `/metrics/3/value` | `0` |
| `/metrics/3/unit` | `"ratio"` |
| `/metrics/4/name` | `"energy_window"` |
| `/metrics/4/value` | `120` |
| `/metrics/4/unit` | `"ticks"` |
| `/metrics/5/name` | `"energy_epsilon"` |
| `/metrics/5/value` | `1e-09` |
| `/metrics/5/unit` | `"J"` |
| `/matrix` | `[]` |
| `/warnings` | `[]` |
| `/violations` | `[]` |

### Debug scenario `projectile_pile`

Topology fixture `dynamic/shape/joint`: `121/123/0`. Observed peak `body/shape/joint;awake/contact`: `123/123/0;121/221`.

| JSON pointer | Value |
| --- | --- |
| `/name` | `"projectile_pile"` |
| `/seed` | `1` |
| `/substeps` | `4` |
| `/ticks` | `11` |
| `/final_hash` | `1431096453509785832` |
| `/hashes/0` | `1431096453509785832` |
| `/hashes/1` | `1431096453509785832` |
| `/repeat_observations/0/repeat` | `1` |
| `/repeat_observations/0/hash` | `1431096453509785832` |
| `/repeat_observations/0/peak_body_count` | `123` |
| `/repeat_observations/0/peak_shape_count` | `123` |
| `/repeat_observations/0/peak_joint_count` | `0` |
| `/repeat_observations/0/peak_awake_count` | `121` |
| `/repeat_observations/0/peak_contact_count` | `221` |
| `/repeat_observations/0/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/0/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `false` |
| `/repeat_observations/0/memory/private_commit/stable` | `false` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/available` | `false` |
| `/repeat_observations/0/memory/working_set/stable` | `false` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/0/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/0/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/0/crt/applicable` | `false` |
| `/repeat_observations/0/crt/balanced` | `true` |
| `/repeat_observations/0/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/0/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/0/crt/client_block_count_delta` | `0` |
| `/repeat_observations/0/crt/client_block_bytes_delta` | `0` |
| `/repeat_observations/1/repeat` | `2` |
| `/repeat_observations/1/hash` | `1431096453509785832` |
| `/repeat_observations/1/peak_body_count` | `123` |
| `/repeat_observations/1/peak_shape_count` | `123` |
| `/repeat_observations/1/peak_joint_count` | `0` |
| `/repeat_observations/1/peak_awake_count` | `121` |
| `/repeat_observations/1/peak_contact_count` | `221` |
| `/repeat_observations/1/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/1/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/gate_applied` | `false` |
| `/repeat_observations/1/memory/budget_qualified` | `false` |
| `/repeat_observations/1/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/private_commit/available` | `false` |
| `/repeat_observations/1/memory/private_commit/stable` | `false` |
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/available` | `false` |
| `/repeat_observations/1/memory/working_set/stable` | `false` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/1/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/1/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/1/crt/applicable` | `false` |
| `/repeat_observations/1/crt/balanced` | `true` |
| `/repeat_observations/1/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/1/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/1/crt/client_block_count_delta` | `0` |
| `/repeat_observations/1/crt/client_block_bytes_delta` | `0` |
| `/contact_before_pile_exit` | `true` |
| `/ccd_primary_pass_count` | `20` |
| `/ccd_fallback_pass_count` | `0` |
| `/projectile_speed` | `35` |
| `/surface_separation` | `0` |
| `/final_linear_speed` | `0` |
| `/final_angular_speed` | `0` |
| `/p95_linear_speed` | `0` |
| `/p95_angular_speed` | `0` |
| `/sleep_ratio` | `0` |
| `/max_penetration` | `0` |
| `/minimum_density` | `0` |
| `/maximum_density` | `0` |
| `/energy_at_tick_600` | `0` |
| `/max_energy_growth_ratio` | `0` |
| `/dynamic_body_count` | `121` |
| `/shape_count` | `123` |
| `/joint_count` | `0` |
| `/peak_body_count` | `123` |
| `/peak_shape_count` | `123` |
| `/peak_joint_count` | `0` |
| `/peak_awake_count` | `121` |
| `/peak_contact_count` | `221` |
| `/step_ms/min` | `4.6268` |
| `/step_ms/p50` | `4.9018` |
| `/step_ms/p95` | `19.7841` |
| `/step_ms/max` | `19.7841` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unavailable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `false` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `false` |
| `/memory/private_commit/baseline_last_bytes` | `0` |
| `/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/memory/private_commit/baseline_median_bytes` | `0` |
| `/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/memory/private_commit/final_last_bytes` | `0` |
| `/memory/private_commit/final_full_min_bytes` | `0` |
| `/memory/private_commit/final_central_min_bytes` | `0` |
| `/memory/private_commit/final_median_bytes` | `0` |
| `/memory/private_commit/final_central_max_bytes` | `0` |
| `/memory/private_commit/final_full_max_bytes` | `0` |
| `/memory/private_commit/peak_bytes` | `0` |
| `/memory/private_commit/growth_ratio` | `0` |
| `/memory/private_commit/instant_growth_ratio` | `0` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/memory/private_commit/measured_full_span_ratio` | `0` |
| `/memory/private_commit/warmup_samples` | `[]` |
| `/memory/private_commit/measured_samples` | `[]` |
| `/memory/working_set/available` | `false` |
| `/memory/working_set/stable` | `false` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"unavailable"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `0` |
| `/memory/working_set/baseline_full_min_bytes` | `0` |
| `/memory/working_set/baseline_central_min_bytes` | `0` |
| `/memory/working_set/baseline_median_bytes` | `0` |
| `/memory/working_set/baseline_central_max_bytes` | `0` |
| `/memory/working_set/baseline_full_max_bytes` | `0` |
| `/memory/working_set/final_last_bytes` | `0` |
| `/memory/working_set/final_full_min_bytes` | `0` |
| `/memory/working_set/final_central_min_bytes` | `0` |
| `/memory/working_set/final_median_bytes` | `0` |
| `/memory/working_set/final_central_max_bytes` | `0` |
| `/memory/working_set/final_full_max_bytes` | `0` |
| `/memory/working_set/peak_bytes` | `0` |
| `/memory/working_set/growth_ratio` | `0` |
| `/memory/working_set/instant_growth_ratio` | `0` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/memory/working_set/warmup_full_span_ratio` | `0` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/memory/working_set/measured_full_span_ratio` | `0` |
| `/memory/working_set/warmup_samples` | `[]` |
| `/memory/working_set/measured_samples` | `[]` |
| `/box3d_allocator/baseline_bytes` | `0` |
| `/box3d_allocator/final_bytes` | `0` |
| `/box3d_allocator/max_abs_delta` | `0` |
| `/box3d_allocator/exact_return` | `true` |
| `/box3d_allocator/warmup_post_teardown` | `[]` |
| `/box3d_allocator/measured_post_teardown` | `[]` |
| `/crt/applicable` | `false` |
| `/crt/balanced` | `true` |
| `/crt/normal_block_count_delta` | `0` |
| `/crt/normal_block_bytes_delta` | `0` |
| `/crt/client_block_count_delta` | `0` |
| `/crt/client_block_bytes_delta` | `0` |
| `/warmup_ticks` | `0` |
| `/measurement_ticks` | `0` |
| `/allocator_warmup_cycles` | `0` |
| `/stress_cycles` | `0` |
| `/fallback` | `""` |
| `/limits/0/name` | `"primary_seed_passes"` |
| `/limits/0/comparison` | `"=="` |
| `/limits/0/value` | `20` |
| `/limits/0/unit` | `"seeds"` |
| `/limits/1/name` | `"fallback_seed_passes"` |
| `/limits/1/comparison` | `"=="` |
| `/limits/1/value` | `0` |
| `/limits/1/unit` | `"seeds"` |
| `/limits/2/name` | `"invalid_states"` |
| `/limits/2/comparison` | `"=="` |
| `/limits/2/value` | `0` |
| `/limits/2/unit` | `"states"` |
| `/metrics/0/name` | `"primary_seed_passes"` |
| `/metrics/0/value` | `20` |
| `/metrics/0/unit` | `"seeds"` |
| `/metrics/1/name` | `"fallback_seed_passes"` |
| `/metrics/1/value` | `0` |
| `/metrics/1/unit` | `"seeds"` |
| `/metrics/2/name` | `"projectile_speed"` |
| `/metrics/2/value` | `35` |
| `/metrics/2/unit` | `"m/s"` |
| `/metrics/3/name` | `"primary_invalid_states"` |
| `/metrics/3/value` | `0` |
| `/metrics/3/unit` | `"count"` |
| `/metrics/4/name` | `"fallback_invalid_states"` |
| `/metrics/4/value` | `0` |
| `/metrics/4/unit` | `"count"` |
| `/metrics/5/name` | `"fallback_seed_evaluated"` |
| `/metrics/5/value` | `0` |
| `/metrics/5/unit` | `"bool"` |
| `/matrix` | `[]` |
| `/warnings` | `[]` |
| `/violations` | `[]` |

### Debug scenario `radial_pile`

Topology fixture `dynamic/shape/joint`: `80/81/0`. Observed peak `body/shape/joint;awake/contact`: `81/81/0;80/204`.

| JSON pointer | Value |
| --- | --- |
| `/name` | `"radial_pile"` |
| `/seed` | `1` |
| `/substeps` | `4` |
| `/ticks` | `1800` |
| `/final_hash` | `10363635776067367757` |
| `/hashes/0` | `10363635776067367757` |
| `/hashes/1` | `10363635776067367757` |
| `/repeat_observations/0/repeat` | `1` |
| `/repeat_observations/0/hash` | `10363635776067367757` |
| `/repeat_observations/0/peak_body_count` | `81` |
| `/repeat_observations/0/peak_shape_count` | `81` |
| `/repeat_observations/0/peak_joint_count` | `0` |
| `/repeat_observations/0/peak_awake_count` | `80` |
| `/repeat_observations/0/peak_contact_count` | `204` |
| `/repeat_observations/0/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/0/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `false` |
| `/repeat_observations/0/memory/private_commit/stable` | `false` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/available` | `false` |
| `/repeat_observations/0/memory/working_set/stable` | `false` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/0/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/0/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/0/crt/applicable` | `false` |
| `/repeat_observations/0/crt/balanced` | `true` |
| `/repeat_observations/0/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/0/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/0/crt/client_block_count_delta` | `0` |
| `/repeat_observations/0/crt/client_block_bytes_delta` | `0` |
| `/repeat_observations/1/repeat` | `2` |
| `/repeat_observations/1/hash` | `10363635776067367757` |
| `/repeat_observations/1/peak_body_count` | `81` |
| `/repeat_observations/1/peak_shape_count` | `81` |
| `/repeat_observations/1/peak_joint_count` | `0` |
| `/repeat_observations/1/peak_awake_count` | `80` |
| `/repeat_observations/1/peak_contact_count` | `204` |
| `/repeat_observations/1/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/1/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/gate_applied` | `false` |
| `/repeat_observations/1/memory/budget_qualified` | `false` |
| `/repeat_observations/1/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/private_commit/available` | `false` |
| `/repeat_observations/1/memory/private_commit/stable` | `false` |
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/available` | `false` |
| `/repeat_observations/1/memory/working_set/stable` | `false` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/1/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/1/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/1/crt/applicable` | `false` |
| `/repeat_observations/1/crt/balanced` | `true` |
| `/repeat_observations/1/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/1/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/1/crt/client_block_count_delta` | `0` |
| `/repeat_observations/1/crt/client_block_bytes_delta` | `0` |
| `/contact_before_pile_exit` | `false` |
| `/ccd_primary_pass_count` | `0` |
| `/ccd_fallback_pass_count` | `0` |
| `/projectile_speed` | `0` |
| `/surface_separation` | `0` |
| `/final_linear_speed` | `0` |
| `/final_angular_speed` | `0` |
| `/p95_linear_speed` | `0` |
| `/p95_angular_speed` | `0` |
| `/sleep_ratio` | `1` |
| `/max_penetration` | `0.0015351474285125732` |
| `/minimum_density` | `480` |
| `/maximum_density` | `520` |
| `/energy_at_tick_600` | `0` |
| `/max_energy_growth_ratio` | `0` |
| `/dynamic_body_count` | `80` |
| `/shape_count` | `81` |
| `/joint_count` | `0` |
| `/peak_body_count` | `81` |
| `/peak_shape_count` | `81` |
| `/peak_joint_count` | `0` |
| `/peak_awake_count` | `80` |
| `/peak_contact_count` | `204` |
| `/step_ms/min` | `0.0274` |
| `/step_ms/p50` | `0.0314` |
| `/step_ms/p95` | `0.0696` |
| `/step_ms/max` | `18.1558` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unavailable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `false` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `false` |
| `/memory/private_commit/baseline_last_bytes` | `0` |
| `/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/memory/private_commit/baseline_median_bytes` | `0` |
| `/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/memory/private_commit/final_last_bytes` | `0` |
| `/memory/private_commit/final_full_min_bytes` | `0` |
| `/memory/private_commit/final_central_min_bytes` | `0` |
| `/memory/private_commit/final_median_bytes` | `0` |
| `/memory/private_commit/final_central_max_bytes` | `0` |
| `/memory/private_commit/final_full_max_bytes` | `0` |
| `/memory/private_commit/peak_bytes` | `0` |
| `/memory/private_commit/growth_ratio` | `0` |
| `/memory/private_commit/instant_growth_ratio` | `0` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/memory/private_commit/measured_full_span_ratio` | `0` |
| `/memory/private_commit/warmup_samples` | `[]` |
| `/memory/private_commit/measured_samples` | `[]` |
| `/memory/working_set/available` | `false` |
| `/memory/working_set/stable` | `false` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"unavailable"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `0` |
| `/memory/working_set/baseline_full_min_bytes` | `0` |
| `/memory/working_set/baseline_central_min_bytes` | `0` |
| `/memory/working_set/baseline_median_bytes` | `0` |
| `/memory/working_set/baseline_central_max_bytes` | `0` |
| `/memory/working_set/baseline_full_max_bytes` | `0` |
| `/memory/working_set/final_last_bytes` | `0` |
| `/memory/working_set/final_full_min_bytes` | `0` |
| `/memory/working_set/final_central_min_bytes` | `0` |
| `/memory/working_set/final_median_bytes` | `0` |
| `/memory/working_set/final_central_max_bytes` | `0` |
| `/memory/working_set/final_full_max_bytes` | `0` |
| `/memory/working_set/peak_bytes` | `0` |
| `/memory/working_set/growth_ratio` | `0` |
| `/memory/working_set/instant_growth_ratio` | `0` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/memory/working_set/warmup_full_span_ratio` | `0` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/memory/working_set/measured_full_span_ratio` | `0` |
| `/memory/working_set/warmup_samples` | `[]` |
| `/memory/working_set/measured_samples` | `[]` |
| `/box3d_allocator/baseline_bytes` | `0` |
| `/box3d_allocator/final_bytes` | `0` |
| `/box3d_allocator/max_abs_delta` | `0` |
| `/box3d_allocator/exact_return` | `true` |
| `/box3d_allocator/warmup_post_teardown` | `[]` |
| `/box3d_allocator/measured_post_teardown` | `[]` |
| `/crt/applicable` | `false` |
| `/crt/balanced` | `true` |
| `/crt/normal_block_count_delta` | `0` |
| `/crt/normal_block_bytes_delta` | `0` |
| `/crt/client_block_count_delta` | `0` |
| `/crt/client_block_bytes_delta` | `0` |
| `/warmup_ticks` | `0` |
| `/measurement_ticks` | `0` |
| `/allocator_warmup_cycles` | `0` |
| `/stress_cycles` | `0` |
| `/fallback` | `""` |
| `/limits/0/name` | `"sleep_ratio"` |
| `/limits/0/comparison` | `">="` |
| `/limits/0/value` | `0.9` |
| `/limits/0/unit` | `"ratio"` |
| `/limits/1/name` | `"p95_linear_speed"` |
| `/limits/1/comparison` | `"<"` |
| `/limits/1/value` | `0.05` |
| `/limits/1/unit` | `"m/s"` |
| `/limits/2/name` | `"p95_angular_speed"` |
| `/limits/2/comparison` | `"<"` |
| `/limits/2/value` | `0.1` |
| `/limits/2/unit` | `"rad/s"` |
| `/limits/3/name` | `"max_penetration"` |
| `/limits/3/comparison` | `"<"` |
| `/limits/3/value` | `0.02` |
| `/limits/3/unit` | `"m"` |
| `/limits/4/name` | `"spontaneous_speed"` |
| `/limits/4/comparison` | `"<="` |
| `/limits/4/value` | `100` |
| `/limits/4/unit` | `"m/s"` |
| `/limits/5/name` | `"world_radius"` |
| `/limits/5/comparison` | `"<="` |
| `/limits/5/value` | `60` |
| `/limits/5/unit` | `"m"` |
| `/limits/6/name` | `"energy_growth_from_tick_600"` |
| `/limits/6/comparison` | `"<="` |
| `/limits/6/value` | `0.02` |
| `/limits/6/unit` | `"ratio"` |
| `/metrics/0/name` | `"sleep_ratio"` |
| `/metrics/0/value` | `1` |
| `/metrics/0/unit` | `"ratio"` |
| `/metrics/1/name` | `"p95_linear_speed"` |
| `/metrics/1/value` | `0` |
| `/metrics/1/unit` | `"m/s"` |
| `/metrics/2/name` | `"p95_angular_speed"` |
| `/metrics/2/value` | `0` |
| `/metrics/2/unit` | `"rad/s"` |
| `/metrics/3/name` | `"max_penetration"` |
| `/metrics/3/value` | `0.0015351474285125732` |
| `/metrics/3/unit` | `"m"` |
| `/metrics/4/name` | `"max_platform_penetration"` |
| `/metrics/4/value` | `0.00030809640884399414` |
| `/metrics/4/unit` | `"m"` |
| `/metrics/5/name` | `"max_pair_penetration"` |
| `/metrics/5/value` | `0.0015351474285125732` |
| `/metrics/5/unit` | `"m"` |
| `/metrics/6/name` | `"minimum_density"` |
| `/metrics/6/value` | `480` |
| `/metrics/6/unit` | `"kg/m3"` |
| `/metrics/7/name` | `"maximum_density"` |
| `/metrics/7/value` | `520` |
| `/metrics/7/unit` | `"kg/m3"` |
| `/metrics/8/name` | `"energy_at_tick_600"` |
| `/metrics/8/value` | `0` |
| `/metrics/8/unit` | `"J"` |
| `/metrics/9/name` | `"max_energy_growth_ratio"` |
| `/metrics/9/value` | `0` |
| `/metrics/9/unit` | `"ratio"` |
| `/matrix` | `[]` |
| `/warnings` | `[]` |
| `/violations` | `[]` |

### Debug scenario `mass_ratio`

Topology fixture `dynamic/shape/joint`: `80/81/0`. Observed peak `body/shape/joint;awake/contact`: `81/81/0;80/227`.

| JSON pointer | Value |
| --- | --- |
| `/name` | `"mass_ratio"` |
| `/seed` | `1` |
| `/substeps` | `4` |
| `/ticks` | `1800` |
| `/final_hash` | `17464060736204574665` |
| `/hashes/0` | `17464060736204574665` |
| `/hashes/1` | `17464060736204574665` |
| `/repeat_observations/0/repeat` | `1` |
| `/repeat_observations/0/hash` | `17464060736204574665` |
| `/repeat_observations/0/peak_body_count` | `81` |
| `/repeat_observations/0/peak_shape_count` | `81` |
| `/repeat_observations/0/peak_joint_count` | `0` |
| `/repeat_observations/0/peak_awake_count` | `80` |
| `/repeat_observations/0/peak_contact_count` | `227` |
| `/repeat_observations/0/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/0/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `false` |
| `/repeat_observations/0/memory/private_commit/stable` | `false` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/available` | `false` |
| `/repeat_observations/0/memory/working_set/stable` | `false` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/0/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/0/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/0/crt/applicable` | `false` |
| `/repeat_observations/0/crt/balanced` | `true` |
| `/repeat_observations/0/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/0/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/0/crt/client_block_count_delta` | `0` |
| `/repeat_observations/0/crt/client_block_bytes_delta` | `0` |
| `/repeat_observations/1/repeat` | `2` |
| `/repeat_observations/1/hash` | `17464060736204574665` |
| `/repeat_observations/1/peak_body_count` | `81` |
| `/repeat_observations/1/peak_shape_count` | `81` |
| `/repeat_observations/1/peak_joint_count` | `0` |
| `/repeat_observations/1/peak_awake_count` | `80` |
| `/repeat_observations/1/peak_contact_count` | `227` |
| `/repeat_observations/1/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/1/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/gate_applied` | `false` |
| `/repeat_observations/1/memory/budget_qualified` | `false` |
| `/repeat_observations/1/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/private_commit/available` | `false` |
| `/repeat_observations/1/memory/private_commit/stable` | `false` |
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/available` | `false` |
| `/repeat_observations/1/memory/working_set/stable` | `false` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/1/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/1/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/1/crt/applicable` | `false` |
| `/repeat_observations/1/crt/balanced` | `true` |
| `/repeat_observations/1/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/1/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/1/crt/client_block_count_delta` | `0` |
| `/repeat_observations/1/crt/client_block_bytes_delta` | `0` |
| `/contact_before_pile_exit` | `false` |
| `/ccd_primary_pass_count` | `0` |
| `/ccd_fallback_pass_count` | `0` |
| `/projectile_speed` | `0` |
| `/surface_separation` | `0` |
| `/final_linear_speed` | `0` |
| `/final_angular_speed` | `0` |
| `/p95_linear_speed` | `0` |
| `/p95_angular_speed` | `0` |
| `/sleep_ratio` | `1` |
| `/max_penetration` | `0.014884665608406067` |
| `/minimum_density` | `85` |
| `/maximum_density` | `3400` |
| `/energy_at_tick_600` | `0` |
| `/max_energy_growth_ratio` | `0` |
| `/dynamic_body_count` | `80` |
| `/shape_count` | `81` |
| `/joint_count` | `0` |
| `/peak_body_count` | `81` |
| `/peak_shape_count` | `81` |
| `/peak_joint_count` | `0` |
| `/peak_awake_count` | `80` |
| `/peak_contact_count` | `227` |
| `/step_ms/min` | `0.0274` |
| `/step_ms/p50` | `0.0458` |
| `/step_ms/p95` | `0.0894` |
| `/step_ms/max` | `25.8477` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unavailable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `false` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `false` |
| `/memory/private_commit/baseline_last_bytes` | `0` |
| `/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/memory/private_commit/baseline_median_bytes` | `0` |
| `/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/memory/private_commit/final_last_bytes` | `0` |
| `/memory/private_commit/final_full_min_bytes` | `0` |
| `/memory/private_commit/final_central_min_bytes` | `0` |
| `/memory/private_commit/final_median_bytes` | `0` |
| `/memory/private_commit/final_central_max_bytes` | `0` |
| `/memory/private_commit/final_full_max_bytes` | `0` |
| `/memory/private_commit/peak_bytes` | `0` |
| `/memory/private_commit/growth_ratio` | `0` |
| `/memory/private_commit/instant_growth_ratio` | `0` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/memory/private_commit/measured_full_span_ratio` | `0` |
| `/memory/private_commit/warmup_samples` | `[]` |
| `/memory/private_commit/measured_samples` | `[]` |
| `/memory/working_set/available` | `false` |
| `/memory/working_set/stable` | `false` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"unavailable"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `0` |
| `/memory/working_set/baseline_full_min_bytes` | `0` |
| `/memory/working_set/baseline_central_min_bytes` | `0` |
| `/memory/working_set/baseline_median_bytes` | `0` |
| `/memory/working_set/baseline_central_max_bytes` | `0` |
| `/memory/working_set/baseline_full_max_bytes` | `0` |
| `/memory/working_set/final_last_bytes` | `0` |
| `/memory/working_set/final_full_min_bytes` | `0` |
| `/memory/working_set/final_central_min_bytes` | `0` |
| `/memory/working_set/final_median_bytes` | `0` |
| `/memory/working_set/final_central_max_bytes` | `0` |
| `/memory/working_set/final_full_max_bytes` | `0` |
| `/memory/working_set/peak_bytes` | `0` |
| `/memory/working_set/growth_ratio` | `0` |
| `/memory/working_set/instant_growth_ratio` | `0` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/memory/working_set/warmup_full_span_ratio` | `0` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/memory/working_set/measured_full_span_ratio` | `0` |
| `/memory/working_set/warmup_samples` | `[]` |
| `/memory/working_set/measured_samples` | `[]` |
| `/box3d_allocator/baseline_bytes` | `0` |
| `/box3d_allocator/final_bytes` | `0` |
| `/box3d_allocator/max_abs_delta` | `0` |
| `/box3d_allocator/exact_return` | `true` |
| `/box3d_allocator/warmup_post_teardown` | `[]` |
| `/box3d_allocator/measured_post_teardown` | `[]` |
| `/crt/applicable` | `false` |
| `/crt/balanced` | `true` |
| `/crt/normal_block_count_delta` | `0` |
| `/crt/normal_block_bytes_delta` | `0` |
| `/crt/client_block_count_delta` | `0` |
| `/crt/client_block_bytes_delta` | `0` |
| `/warmup_ticks` | `0` |
| `/measurement_ticks` | `0` |
| `/allocator_warmup_cycles` | `0` |
| `/stress_cycles` | `0` |
| `/fallback` | `""` |
| `/limits/0/name` | `"sleep_ratio"` |
| `/limits/0/comparison` | `">="` |
| `/limits/0/value` | `0.8` |
| `/limits/0/unit` | `"ratio"` |
| `/limits/1/name` | `"p95_linear_speed"` |
| `/limits/1/comparison` | `"<"` |
| `/limits/1/value` | `0.1` |
| `/limits/1/unit` | `"m/s"` |
| `/limits/2/name` | `"p95_angular_speed"` |
| `/limits/2/comparison` | `"<"` |
| `/limits/2/value` | `0.2` |
| `/limits/2/unit` | `"rad/s"` |
| `/limits/3/name` | `"max_penetration"` |
| `/limits/3/comparison` | `"<"` |
| `/limits/3/value` | `0.025` |
| `/limits/3/unit` | `"m"` |
| `/limits/4/name` | `"spontaneous_speed"` |
| `/limits/4/comparison` | `"<="` |
| `/limits/4/value` | `100` |
| `/limits/4/unit` | `"m/s"` |
| `/limits/5/name` | `"world_radius"` |
| `/limits/5/comparison` | `"<="` |
| `/limits/5/value` | `60` |
| `/limits/5/unit` | `"m"` |
| `/metrics/0/name` | `"sleep_ratio"` |
| `/metrics/0/value` | `1` |
| `/metrics/0/unit` | `"ratio"` |
| `/metrics/1/name` | `"p95_linear_speed"` |
| `/metrics/1/value` | `0` |
| `/metrics/1/unit` | `"m/s"` |
| `/metrics/2/name` | `"p95_angular_speed"` |
| `/metrics/2/value` | `0` |
| `/metrics/2/unit` | `"rad/s"` |
| `/metrics/3/name` | `"max_penetration"` |
| `/metrics/3/value` | `0.014884665608406067` |
| `/metrics/3/unit` | `"m"` |
| `/metrics/4/name` | `"max_platform_penetration"` |
| `/metrics/4/value` | `0.004117727279663086` |
| `/metrics/4/unit` | `"m"` |
| `/metrics/5/name` | `"max_pair_penetration"` |
| `/metrics/5/value` | `0.014884665608406067` |
| `/metrics/5/unit` | `"m"` |
| `/metrics/6/name` | `"minimum_density"` |
| `/metrics/6/value` | `85` |
| `/metrics/6/unit` | `"kg/m3"` |
| `/metrics/7/name` | `"maximum_density"` |
| `/metrics/7/value` | `3400` |
| `/metrics/7/unit` | `"kg/m3"` |
| `/metrics/8/name` | `"energy_at_tick_600"` |
| `/metrics/8/value` | `0` |
| `/metrics/8/unit` | `"J"` |
| `/metrics/9/name` | `"max_energy_growth_ratio"` |
| `/metrics/9/value` | `0` |
| `/metrics/9/unit` | `"ratio"` |
| `/matrix` | `[]` |
| `/warnings` | `[]` |
| `/violations` | `[]` |

### Debug scenario `stress`

Topology fixture `dynamic/shape/joint`: `500/800/250`. Observed peak `body/shape/joint;awake/contact`: `500/800/250;500/0`.

| JSON pointer | Value |
| --- | --- |
| `/name` | `"stress"` |
| `/seed` | `1` |
| `/substeps` | `4` |
| `/ticks` | `15000` |
| `/final_hash` | `10292935394449293550` |
| `/hashes/0` | `10292935394449293550` |
| `/hashes/1` | `10292935394449293550` |
| `/repeat_observations/0/repeat` | `1` |
| `/repeat_observations/0/hash` | `10292935394449293550` |
| `/repeat_observations/0/peak_body_count` | `500` |
| `/repeat_observations/0/peak_shape_count` | `800` |
| `/repeat_observations/0/peak_joint_count` | `250` |
| `/repeat_observations/0/peak_awake_count` | `500` |
| `/repeat_observations/0/peak_contact_count` | `0` |
| `/repeat_observations/0/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/0/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/assessment_status` | `"unstable"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `true` |
| `/repeat_observations/0/memory/private_commit/stable` | `false` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `true` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `6979584` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `4345856` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `4878336` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `5726208` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `6959104` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `6979584` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `6934528` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `4370432` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `6377472` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `6807552` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `6934528` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `6971392` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `6979584` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0.1888412017167382` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0.36337625178826893` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0.45994277539341916` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0.08182912154031288` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0.38206979542719616` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/0` | `4382720` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/1` | `5488640` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/2` | `4366336` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/3` | `5697536` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/4` | `6979584` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/5` | `5726208` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/6` | `4878336` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/7` | `4345856` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/8` | `6959104` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/9` | `6979584` |
| `/repeat_observations/0/memory/private_commit/measured_samples/0` | `4378624` |
| `/repeat_observations/0/memory/private_commit/measured_samples/1` | `6885376` |
| `/repeat_observations/0/memory/private_commit/measured_samples/2` | `5234688` |
| `/repeat_observations/0/memory/private_commit/measured_samples/3` | `4374528` |
| `/repeat_observations/0/memory/private_commit/measured_samples/4` | `6144000` |
| `/repeat_observations/0/memory/private_commit/measured_samples/5` | `4370432` |
| `/repeat_observations/0/memory/private_commit/measured_samples/6` | `6971392` |
| `/repeat_observations/0/memory/private_commit/measured_samples/7` | `6377472` |
| `/repeat_observations/0/memory/private_commit/measured_samples/8` | `6807552` |
| `/repeat_observations/0/memory/private_commit/measured_samples/9` | `6934528` |
| `/repeat_observations/0/memory/working_set/available` | `true` |
| `/repeat_observations/0/memory/working_set/stable` | `false` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `true` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"unstable"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `8032256` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `7094272` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `7131136` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `7327744` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `8032256` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `8032256` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `8052736` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `7127040` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `8044544` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `8052736` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `8052736` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `8060928` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `9588736` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0.09893795416433762` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0.0025497195308516064` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0.12297372833985466` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0.12800447177193963` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0.001017293997965412` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0.11597151576805696` |
| `/repeat_observations/0/memory/working_set/warmup_samples/0` | `7118848` |
| `/repeat_observations/0/memory/working_set/warmup_samples/1` | `7340032` |
| `/repeat_observations/0/memory/working_set/warmup_samples/2` | `7102464` |
| `/repeat_observations/0/memory/working_set/warmup_samples/3` | `7323648` |
| `/repeat_observations/0/memory/working_set/warmup_samples/4` | `8019968` |
| `/repeat_observations/0/memory/working_set/warmup_samples/5` | `7327744` |
| `/repeat_observations/0/memory/working_set/warmup_samples/6` | `7131136` |
| `/repeat_observations/0/memory/working_set/warmup_samples/7` | `7094272` |
| `/repeat_observations/0/memory/working_set/warmup_samples/8` | `8032256` |
| `/repeat_observations/0/memory/working_set/warmup_samples/9` | `8032256` |
| `/repeat_observations/0/memory/working_set/measured_samples/0` | `7135232` |
| `/repeat_observations/0/memory/working_set/measured_samples/1` | `8060928` |
| `/repeat_observations/0/memory/working_set/measured_samples/2` | `7356416` |
| `/repeat_observations/0/memory/working_set/measured_samples/3` | `7135232` |
| `/repeat_observations/0/memory/working_set/measured_samples/4` | `8065024` |
| `/repeat_observations/0/memory/working_set/measured_samples/5` | `7127040` |
| `/repeat_observations/0/memory/working_set/measured_samples/6` | `8060928` |
| `/repeat_observations/0/memory/working_set/measured_samples/7` | `8052736` |
| `/repeat_observations/0/memory/working_set/measured_samples/8` | `8044544` |
| `/repeat_observations/0/memory/working_set/measured_samples/9` | `8052736` |
| `/repeat_observations/0/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/0/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/0` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/1` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/2` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/3` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/4` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/5` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/6` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/7` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/8` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/9` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/0` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/1` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/2` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/3` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/4` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/5` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/6` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/7` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/8` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/9` | `0` |
| `/repeat_observations/0/crt/applicable` | `true` |
| `/repeat_observations/0/crt/balanced` | `true` |
| `/repeat_observations/0/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/0/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/0/crt/client_block_count_delta` | `0` |
| `/repeat_observations/0/crt/client_block_bytes_delta` | `0` |
| `/repeat_observations/1/repeat` | `2` |
| `/repeat_observations/1/hash` | `10292935394449293550` |
| `/repeat_observations/1/peak_body_count` | `500` |
| `/repeat_observations/1/peak_shape_count` | `800` |
| `/repeat_observations/1/peak_joint_count` | `250` |
| `/repeat_observations/1/peak_awake_count` | `500` |
| `/repeat_observations/1/peak_contact_count` | `0` |
| `/repeat_observations/1/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/1/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/assessment_status` | `"unstable"` |
| `/repeat_observations/1/memory/gate_applied` | `false` |
| `/repeat_observations/1/memory/budget_qualified` | `false` |
| `/repeat_observations/1/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/private_commit/available` | `true` |
| `/repeat_observations/1/memory/private_commit/stable` | `false` |
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `6664192` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `6664192` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `6692864` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `6746112` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `6787072` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `6934528` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `6430720` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `5914624` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `5931008` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `6213632` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `6430720` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `6561792` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `6934528` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0.013964784456587736` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0.04007285974499089` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0.08042188529993408` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0.1041529334212261` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/0` | `6635520` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/1` | `4378624` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/2` | `6799360` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/3` | `6684672` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/4` | `6733824` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/5` | `6692864` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/6` | `6787072` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/7` | `6934528` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/8` | `6746112` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/9` | `6664192` |
| `/repeat_observations/1/memory/private_commit/measured_samples/0` | `6737920` |
| `/repeat_observations/1/memory/private_commit/measured_samples/1` | `6463488` |
| `/repeat_observations/1/memory/private_commit/measured_samples/2` | `6754304` |
| `/repeat_observations/1/memory/private_commit/measured_samples/3` | `6918144` |
| `/repeat_observations/1/memory/private_commit/measured_samples/4` | `4370432` |
| `/repeat_observations/1/memory/private_commit/measured_samples/5` | `6561792` |
| `/repeat_observations/1/memory/private_commit/measured_samples/6` | `5914624` |
| `/repeat_observations/1/memory/private_commit/measured_samples/7` | `6213632` |
| `/repeat_observations/1/memory/private_commit/measured_samples/8` | `5931008` |
| `/repeat_observations/1/memory/private_commit/measured_samples/9` | `6430720` |
| `/repeat_observations/1/memory/working_set/available` | `true` |
| `/repeat_observations/1/memory/working_set/stable` | `true` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"pass"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `8052736` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `8052736` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `8056832` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `8056832` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `8060928` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `8069120` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `8122368` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `8052736` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `8060928` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `8122368` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `8134656` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `8142848` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `9695232` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0.00813421453990849` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0.008646998982706003` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0.0005083884087442806` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0.0020335536349771225` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0.009077155824508321` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0.011094301563287948` |
| `/repeat_observations/1/memory/working_set/warmup_samples/0` | `8073216` |
| `/repeat_observations/1/memory/working_set/warmup_samples/1` | `7135232` |
| `/repeat_observations/1/memory/working_set/warmup_samples/2` | `8056832` |
| `/repeat_observations/1/memory/working_set/warmup_samples/3` | `8056832` |
| `/repeat_observations/1/memory/working_set/warmup_samples/4` | `8056832` |
| `/repeat_observations/1/memory/working_set/warmup_samples/5` | `8056832` |
| `/repeat_observations/1/memory/working_set/warmup_samples/6` | `8069120` |
| `/repeat_observations/1/memory/working_set/warmup_samples/7` | `8056832` |
| `/repeat_observations/1/memory/working_set/warmup_samples/8` | `8060928` |
| `/repeat_observations/1/memory/working_set/warmup_samples/9` | `8052736` |
| `/repeat_observations/1/memory/working_set/measured_samples/0` | `8077312` |
| `/repeat_observations/1/memory/working_set/measured_samples/1` | `8065024` |
| `/repeat_observations/1/memory/working_set/measured_samples/2` | `8069120` |
| `/repeat_observations/1/memory/working_set/measured_samples/3` | `8073216` |
| `/repeat_observations/1/memory/working_set/measured_samples/4` | `7139328` |
| `/repeat_observations/1/memory/working_set/measured_samples/5` | `8134656` |
| `/repeat_observations/1/memory/working_set/measured_samples/6` | `8052736` |
| `/repeat_observations/1/memory/working_set/measured_samples/7` | `8142848` |
| `/repeat_observations/1/memory/working_set/measured_samples/8` | `8060928` |
| `/repeat_observations/1/memory/working_set/measured_samples/9` | `8122368` |
| `/repeat_observations/1/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/1/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/0` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/1` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/2` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/3` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/4` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/5` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/6` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/7` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/8` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/9` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/0` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/1` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/2` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/3` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/4` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/5` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/6` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/7` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/8` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/9` | `0` |
| `/repeat_observations/1/crt/applicable` | `true` |
| `/repeat_observations/1/crt/balanced` | `true` |
| `/repeat_observations/1/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/1/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/1/crt/client_block_count_delta` | `0` |
| `/repeat_observations/1/crt/client_block_bytes_delta` | `0` |
| `/contact_before_pile_exit` | `false` |
| `/ccd_primary_pass_count` | `0` |
| `/ccd_fallback_pass_count` | `0` |
| `/projectile_speed` | `0` |
| `/surface_separation` | `0` |
| `/final_linear_speed` | `0` |
| `/final_angular_speed` | `0` |
| `/p95_linear_speed` | `0` |
| `/p95_angular_speed` | `0` |
| `/sleep_ratio` | `0` |
| `/max_penetration` | `0` |
| `/minimum_density` | `0` |
| `/maximum_density` | `0` |
| `/energy_at_tick_600` | `0` |
| `/max_energy_growth_ratio` | `0` |
| `/dynamic_body_count` | `500` |
| `/shape_count` | `800` |
| `/joint_count` | `250` |
| `/peak_body_count` | `500` |
| `/peak_shape_count` | `800` |
| `/peak_joint_count` | `250` |
| `/peak_awake_count` | `500` |
| `/peak_contact_count` | `0` |
| `/step_ms/min` | `0.4409` |
| `/step_ms/p50` | `0.6468` |
| `/step_ms/p95` | `0.8599` |
| `/step_ms/max` | `14.484` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unstable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `true` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `true` |
| `/memory/private_commit/baseline_last_bytes` | `6979584` |
| `/memory/private_commit/baseline_full_min_bytes` | `4345856` |
| `/memory/private_commit/baseline_central_min_bytes` | `4878336` |
| `/memory/private_commit/baseline_median_bytes` | `5726208` |
| `/memory/private_commit/baseline_central_max_bytes` | `6959104` |
| `/memory/private_commit/baseline_full_max_bytes` | `6979584` |
| `/memory/private_commit/final_last_bytes` | `6934528` |
| `/memory/private_commit/final_full_min_bytes` | `4370432` |
| `/memory/private_commit/final_central_min_bytes` | `6377472` |
| `/memory/private_commit/final_median_bytes` | `6807552` |
| `/memory/private_commit/final_central_max_bytes` | `6934528` |
| `/memory/private_commit/final_full_max_bytes` | `6971392` |
| `/memory/private_commit/peak_bytes` | `6979584` |
| `/memory/private_commit/growth_ratio` | `0.1888412017167382` |
| `/memory/private_commit/instant_growth_ratio` | `0` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0.36337625178826893` |
| `/memory/private_commit/warmup_full_span_ratio` | `0.45994277539341916` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0.08182912154031288` |
| `/memory/private_commit/measured_full_span_ratio` | `0.38206979542719616` |
| `/memory/private_commit/warmup_samples/0` | `4382720` |
| `/memory/private_commit/warmup_samples/1` | `5488640` |
| `/memory/private_commit/warmup_samples/2` | `4366336` |
| `/memory/private_commit/warmup_samples/3` | `5697536` |
| `/memory/private_commit/warmup_samples/4` | `6979584` |
| `/memory/private_commit/warmup_samples/5` | `5726208` |
| `/memory/private_commit/warmup_samples/6` | `4878336` |
| `/memory/private_commit/warmup_samples/7` | `4345856` |
| `/memory/private_commit/warmup_samples/8` | `6959104` |
| `/memory/private_commit/warmup_samples/9` | `6979584` |
| `/memory/private_commit/measured_samples/0` | `4378624` |
| `/memory/private_commit/measured_samples/1` | `6885376` |
| `/memory/private_commit/measured_samples/2` | `5234688` |
| `/memory/private_commit/measured_samples/3` | `4374528` |
| `/memory/private_commit/measured_samples/4` | `6144000` |
| `/memory/private_commit/measured_samples/5` | `4370432` |
| `/memory/private_commit/measured_samples/6` | `6971392` |
| `/memory/private_commit/measured_samples/7` | `6377472` |
| `/memory/private_commit/measured_samples/8` | `6807552` |
| `/memory/private_commit/measured_samples/9` | `6934528` |
| `/memory/working_set/available` | `true` |
| `/memory/working_set/stable` | `false` |
| `/memory/working_set/terminal_growth` | `true` |
| `/memory/working_set/assessment_status` | `"unstable"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `8032256` |
| `/memory/working_set/baseline_full_min_bytes` | `7094272` |
| `/memory/working_set/baseline_central_min_bytes` | `7131136` |
| `/memory/working_set/baseline_median_bytes` | `7327744` |
| `/memory/working_set/baseline_central_max_bytes` | `8032256` |
| `/memory/working_set/baseline_full_max_bytes` | `8032256` |
| `/memory/working_set/final_last_bytes` | `8052736` |
| `/memory/working_set/final_full_min_bytes` | `7127040` |
| `/memory/working_set/final_central_min_bytes` | `8044544` |
| `/memory/working_set/final_median_bytes` | `8052736` |
| `/memory/working_set/final_central_max_bytes` | `8052736` |
| `/memory/working_set/final_full_max_bytes` | `8060928` |
| `/memory/working_set/peak_bytes` | `9588736` |
| `/memory/working_set/growth_ratio` | `0.09893795416433762` |
| `/memory/working_set/instant_growth_ratio` | `0.0025497195308516064` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0.12297372833985466` |
| `/memory/working_set/warmup_full_span_ratio` | `0.12800447177193963` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0.001017293997965412` |
| `/memory/working_set/measured_full_span_ratio` | `0.11597151576805696` |
| `/memory/working_set/warmup_samples/0` | `7118848` |
| `/memory/working_set/warmup_samples/1` | `7340032` |
| `/memory/working_set/warmup_samples/2` | `7102464` |
| `/memory/working_set/warmup_samples/3` | `7323648` |
| `/memory/working_set/warmup_samples/4` | `8019968` |
| `/memory/working_set/warmup_samples/5` | `7327744` |
| `/memory/working_set/warmup_samples/6` | `7131136` |
| `/memory/working_set/warmup_samples/7` | `7094272` |
| `/memory/working_set/warmup_samples/8` | `8032256` |
| `/memory/working_set/warmup_samples/9` | `8032256` |
| `/memory/working_set/measured_samples/0` | `7135232` |
| `/memory/working_set/measured_samples/1` | `8060928` |
| `/memory/working_set/measured_samples/2` | `7356416` |
| `/memory/working_set/measured_samples/3` | `7135232` |
| `/memory/working_set/measured_samples/4` | `8065024` |
| `/memory/working_set/measured_samples/5` | `7127040` |
| `/memory/working_set/measured_samples/6` | `8060928` |
| `/memory/working_set/measured_samples/7` | `8052736` |
| `/memory/working_set/measured_samples/8` | `8044544` |
| `/memory/working_set/measured_samples/9` | `8052736` |
| `/box3d_allocator/baseline_bytes` | `0` |
| `/box3d_allocator/final_bytes` | `0` |
| `/box3d_allocator/max_abs_delta` | `0` |
| `/box3d_allocator/exact_return` | `true` |
| `/box3d_allocator/warmup_post_teardown/0` | `0` |
| `/box3d_allocator/warmup_post_teardown/1` | `0` |
| `/box3d_allocator/warmup_post_teardown/2` | `0` |
| `/box3d_allocator/warmup_post_teardown/3` | `0` |
| `/box3d_allocator/warmup_post_teardown/4` | `0` |
| `/box3d_allocator/warmup_post_teardown/5` | `0` |
| `/box3d_allocator/warmup_post_teardown/6` | `0` |
| `/box3d_allocator/warmup_post_teardown/7` | `0` |
| `/box3d_allocator/warmup_post_teardown/8` | `0` |
| `/box3d_allocator/warmup_post_teardown/9` | `0` |
| `/box3d_allocator/measured_post_teardown/0` | `0` |
| `/box3d_allocator/measured_post_teardown/1` | `0` |
| `/box3d_allocator/measured_post_teardown/2` | `0` |
| `/box3d_allocator/measured_post_teardown/3` | `0` |
| `/box3d_allocator/measured_post_teardown/4` | `0` |
| `/box3d_allocator/measured_post_teardown/5` | `0` |
| `/box3d_allocator/measured_post_teardown/6` | `0` |
| `/box3d_allocator/measured_post_teardown/7` | `0` |
| `/box3d_allocator/measured_post_teardown/8` | `0` |
| `/box3d_allocator/measured_post_teardown/9` | `0` |
| `/crt/applicable` | `true` |
| `/crt/balanced` | `true` |
| `/crt/normal_block_count_delta` | `0` |
| `/crt/normal_block_bytes_delta` | `0` |
| `/crt/client_block_count_delta` | `0` |
| `/crt/client_block_bytes_delta` | `0` |
| `/warmup_ticks` | `300` |
| `/measurement_ticks` | `1200` |
| `/allocator_warmup_cycles` | `10` |
| `/stress_cycles` | `10` |
| `/fallback` | `""` |
| `/limits/0/name` | `"allocator_warmup_cycles"` |
| `/limits/0/comparison` | `"=="` |
| `/limits/0/value` | `10` |
| `/limits/0/unit` | `"cycles"` |
| `/limits/1/name` | `"scenario_timeout"` |
| `/limits/1/comparison` | `"<="` |
| `/limits/1/value` | `60` |
| `/limits/1/unit` | `"s"` |
| `/limits/2/name` | `"spontaneous_speed"` |
| `/limits/2/comparison` | `"<="` |
| `/limits/2/value` | `100` |
| `/limits/2/unit` | `"m/s"` |
| `/metrics/0/name` | `"private_commit_diagnostic_growth_target"` |
| `/metrics/0/value` | `0.05` |
| `/metrics/0/unit` | `"ratio"` |
| `/metrics/1/name` | `"private_commit_diagnostic_trimmed_span_target"` |
| `/metrics/1/value` | `0.05` |
| `/metrics/1/unit` | `"ratio"` |
| `/metrics/2/name` | `"working_set_diagnostic_growth_target"` |
| `/metrics/2/value` | `0.05` |
| `/metrics/2/unit` | `"ratio"` |
| `/metrics/3/name` | `"working_set_diagnostic_trimmed_span_target"` |
| `/metrics/3/value` | `0.05` |
| `/metrics/3/unit` | `"ratio"` |
| `/metrics/4/name` | `"private_commit_baseline_median_bytes"` |
| `/metrics/4/value` | `5726208` |
| `/metrics/4/unit` | `"bytes"` |
| `/metrics/5/name` | `"private_commit_final_median_bytes"` |
| `/metrics/5/value` | `6807552` |
| `/metrics/5/unit` | `"bytes"` |
| `/metrics/6/name` | `"private_commit_growth_ratio"` |
| `/metrics/6/value` | `0.1888412017167382` |
| `/metrics/6/unit` | `"ratio"` |
| `/metrics/7/name` | `"private_commit_instant_growth_ratio"` |
| `/metrics/7/value` | `0` |
| `/metrics/7/unit` | `"ratio"` |
| `/metrics/8/name` | `"private_commit_warmup_trimmed_span_ratio"` |
| `/metrics/8/value` | `0.36337625178826893` |
| `/metrics/8/unit` | `"ratio"` |
| `/metrics/9/name` | `"private_commit_measured_trimmed_span_ratio"` |
| `/metrics/9/value` | `0.08182912154031288` |
| `/metrics/9/unit` | `"ratio"` |
| `/metrics/10/name` | `"working_set_baseline_bytes"` |
| `/metrics/10/value` | `8032256` |
| `/metrics/10/unit` | `"bytes"` |
| `/metrics/11/name` | `"working_set_baseline_low_bytes"` |
| `/metrics/11/value` | `7094272` |
| `/metrics/11/unit` | `"bytes"` |
| `/metrics/12/name` | `"working_set_peak_bytes"` |
| `/metrics/12/value` | `9588736` |
| `/metrics/12/unit` | `"bytes"` |
| `/metrics/13/name` | `"working_set_final_bytes"` |
| `/metrics/13/value` | `8052736` |
| `/metrics/13/unit` | `"bytes"` |
| `/metrics/14/name` | `"working_set_final_low_bytes"` |
| `/metrics/14/value` | `7127040` |
| `/metrics/14/unit` | `"bytes"` |
| `/metrics/15/name` | `"working_set_growth_ratio"` |
| `/metrics/15/value` | `0.09893795416433762` |
| `/metrics/15/unit` | `"ratio"` |
| `/metrics/16/name` | `"working_set_instant_growth_ratio"` |
| `/metrics/16/value` | `0.0025497195308516064` |
| `/metrics/16/unit` | `"ratio"` |
| `/metrics/17/name` | `"executed_substeps"` |
| `/metrics/17/value` | `4` |
| `/metrics/17/unit` | `"count"` |
| `/metrics/18/name` | `"allocator_warmup_cycles"` |
| `/metrics/18/value` | `10` |
| `/metrics/18/unit` | `"cycles"` |
| `/metrics/19/name` | `"completed_cycles"` |
| `/metrics/19/value` | `10` |
| `/metrics/19/unit` | `"count"` |
| `/matrix` | `[]` |
| `/warnings/0/code` | `"private_commit_budget_unqualified"` |
| `/warnings/0/message` | `"PrivateUsage is diagnostic in the foundation; qualify the 5% budget in a packaged Release build on reference hardware"` |
| `/warnings/0/details/0/name` | `"assessment"` |
| `/warnings/0/details/0/value` | `"unstable"` |
| `/warnings/0/details/1/name` | `"budget_scope"` |
| `/warnings/0/details/1/value` | `"future_packaged_reference_hardware"` |
| `/violations` | `[]` |

### Debug scenario `capability_matrix`

Topology fixture `dynamic/shape/joint`: `0/0/0`. Observed peak `body/shape/joint;awake/contact`: `123/123/1;121/221`.

| JSON pointer | Value |
| --- | --- |
| `/name` | `"capability_matrix"` |
| `/seed` | `1` |
| `/substeps` | `4` |
| `/ticks` | `21800` |
| `/final_hash` | `17104053157009575930` |
| `/hashes/0` | `17104053157009575930` |
| `/hashes/1` | `17104053157009575930` |
| `/repeat_observations/0/repeat` | `1` |
| `/repeat_observations/0/hash` | `17104053157009575930` |
| `/repeat_observations/0/peak_body_count` | `123` |
| `/repeat_observations/0/peak_shape_count` | `123` |
| `/repeat_observations/0/peak_joint_count` | `1` |
| `/repeat_observations/0/peak_awake_count` | `121` |
| `/repeat_observations/0/peak_contact_count` | `221` |
| `/repeat_observations/0/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/0/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `false` |
| `/repeat_observations/0/memory/private_commit/stable` | `false` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/available` | `false` |
| `/repeat_observations/0/memory/working_set/stable` | `false` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/0/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/0/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/0/crt/applicable` | `false` |
| `/repeat_observations/0/crt/balanced` | `true` |
| `/repeat_observations/0/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/0/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/0/crt/client_block_count_delta` | `0` |
| `/repeat_observations/0/crt/client_block_bytes_delta` | `0` |
| `/repeat_observations/1/repeat` | `2` |
| `/repeat_observations/1/hash` | `17104053157009575930` |
| `/repeat_observations/1/peak_body_count` | `123` |
| `/repeat_observations/1/peak_shape_count` | `123` |
| `/repeat_observations/1/peak_joint_count` | `1` |
| `/repeat_observations/1/peak_awake_count` | `121` |
| `/repeat_observations/1/peak_contact_count` | `221` |
| `/repeat_observations/1/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/1/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/gate_applied` | `false` |
| `/repeat_observations/1/memory/budget_qualified` | `false` |
| `/repeat_observations/1/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/private_commit/available` | `false` |
| `/repeat_observations/1/memory/private_commit/stable` | `false` |
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/available` | `false` |
| `/repeat_observations/1/memory/working_set/stable` | `false` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/1/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/1/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/1/crt/applicable` | `false` |
| `/repeat_observations/1/crt/balanced` | `true` |
| `/repeat_observations/1/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/1/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/1/crt/client_block_count_delta` | `0` |
| `/repeat_observations/1/crt/client_block_bytes_delta` | `0` |
| `/contact_before_pile_exit` | `false` |
| `/ccd_primary_pass_count` | `0` |
| `/ccd_fallback_pass_count` | `0` |
| `/projectile_speed` | `0` |
| `/surface_separation` | `0` |
| `/final_linear_speed` | `0` |
| `/final_angular_speed` | `0` |
| `/p95_linear_speed` | `0` |
| `/p95_angular_speed` | `0` |
| `/sleep_ratio` | `0` |
| `/max_penetration` | `0` |
| `/minimum_density` | `0` |
| `/maximum_density` | `0` |
| `/energy_at_tick_600` | `0` |
| `/max_energy_growth_ratio` | `0` |
| `/dynamic_body_count` | `0` |
| `/shape_count` | `0` |
| `/joint_count` | `0` |
| `/peak_body_count` | `123` |
| `/peak_shape_count` | `123` |
| `/peak_joint_count` | `1` |
| `/peak_awake_count` | `121` |
| `/peak_contact_count` | `221` |
| `/step_ms/min` | `0.0274` |
| `/step_ms/p50` | `0.042` |
| `/step_ms/p95` | `0.0767` |
| `/step_ms/max` | `18.7385` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unavailable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `false` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `false` |
| `/memory/private_commit/baseline_last_bytes` | `0` |
| `/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/memory/private_commit/baseline_median_bytes` | `0` |
| `/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/memory/private_commit/final_last_bytes` | `0` |
| `/memory/private_commit/final_full_min_bytes` | `0` |
| `/memory/private_commit/final_central_min_bytes` | `0` |
| `/memory/private_commit/final_median_bytes` | `0` |
| `/memory/private_commit/final_central_max_bytes` | `0` |
| `/memory/private_commit/final_full_max_bytes` | `0` |
| `/memory/private_commit/peak_bytes` | `0` |
| `/memory/private_commit/growth_ratio` | `0` |
| `/memory/private_commit/instant_growth_ratio` | `0` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/memory/private_commit/measured_full_span_ratio` | `0` |
| `/memory/private_commit/warmup_samples` | `[]` |
| `/memory/private_commit/measured_samples` | `[]` |
| `/memory/working_set/available` | `false` |
| `/memory/working_set/stable` | `false` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"unavailable"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `0` |
| `/memory/working_set/baseline_full_min_bytes` | `0` |
| `/memory/working_set/baseline_central_min_bytes` | `0` |
| `/memory/working_set/baseline_median_bytes` | `0` |
| `/memory/working_set/baseline_central_max_bytes` | `0` |
| `/memory/working_set/baseline_full_max_bytes` | `0` |
| `/memory/working_set/final_last_bytes` | `0` |
| `/memory/working_set/final_full_min_bytes` | `0` |
| `/memory/working_set/final_central_min_bytes` | `0` |
| `/memory/working_set/final_median_bytes` | `0` |
| `/memory/working_set/final_central_max_bytes` | `0` |
| `/memory/working_set/final_full_max_bytes` | `0` |
| `/memory/working_set/peak_bytes` | `0` |
| `/memory/working_set/growth_ratio` | `0` |
| `/memory/working_set/instant_growth_ratio` | `0` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/memory/working_set/warmup_full_span_ratio` | `0` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/memory/working_set/measured_full_span_ratio` | `0` |
| `/memory/working_set/warmup_samples` | `[]` |
| `/memory/working_set/measured_samples` | `[]` |
| `/box3d_allocator/baseline_bytes` | `0` |
| `/box3d_allocator/final_bytes` | `0` |
| `/box3d_allocator/max_abs_delta` | `0` |
| `/box3d_allocator/exact_return` | `true` |
| `/box3d_allocator/warmup_post_teardown` | `[]` |
| `/box3d_allocator/measured_post_teardown` | `[]` |
| `/crt/applicable` | `false` |
| `/crt/balanced` | `true` |
| `/crt/normal_block_count_delta` | `0` |
| `/crt/normal_block_bytes_delta` | `0` |
| `/crt/client_block_count_delta` | `0` |
| `/crt/client_block_bytes_delta` | `0` |
| `/warmup_ticks` | `0` |
| `/measurement_ticks` | `0` |
| `/allocator_warmup_cycles` | `0` |
| `/stress_cycles` | `0` |
| `/fallback` | `""` |
| `/limits` | `[]` |
| `/metrics` | `[]` |
| `/matrix/0/capability` | `"ccd_dynamic_dynamic"` |
| `/matrix/0/status` | `"pass"` |
| `/matrix/0/fallback` | `null` |
| `/matrix/0/functional_status` | `"pass"` |
| `/matrix/0/functional_fallback` | `null` |
| `/matrix/0/detail` | `"all 20 fixed seeds contacted a dynamic pile body at 35 m/s and four substeps"` |
| `/matrix/0/peak_body_count` | `123` |
| `/matrix/0/peak_shape_count` | `123` |
| `/matrix/0/peak_joint_count` | `0` |
| `/matrix/0/peak_awake_count` | `121` |
| `/matrix/0/peak_contact_count` | `221` |
| `/matrix/0/values/0/name` | `"primary_passes"` |
| `/matrix/0/values/0/value` | `20` |
| `/matrix/0/values/0/unit` | `"seeds"` |
| `/matrix/0/values/1/name` | `"fallback_passes"` |
| `/matrix/0/values/1/value` | `0` |
| `/matrix/0/values/1/unit` | `"seeds"` |
| `/matrix/0/values/2/name` | `"primary_speed"` |
| `/matrix/0/values/2/value` | `35` |
| `/matrix/0/values/2/unit` | `"m/s"` |
| `/matrix/0/values/3/name` | `"primary_substeps"` |
| `/matrix/0/values/3/value` | `4` |
| `/matrix/0/values/3/unit` | `"count"` |
| `/matrix/0/values/4/name` | `"fallback_speed"` |
| `/matrix/0/values/4/value` | `30` |
| `/matrix/0/values/4/unit` | `"m/s"` |
| `/matrix/0/values/5/name` | `"fallback_substeps"` |
| `/matrix/0/values/5/value` | `6` |
| `/matrix/0/values/5/unit` | `"count"` |
| `/matrix/0/values/6/name` | `"fallback_evaluated"` |
| `/matrix/0/values/6/value` | `0` |
| `/matrix/0/values/6/unit` | `"bool"` |
| `/matrix/0/values/7/name` | `"primary_invalid_states"` |
| `/matrix/0/values/7/value` | `0` |
| `/matrix/0/values/7/unit` | `"count"` |
| `/matrix/0/values/8/name` | `"fallback_invalid_states"` |
| `/matrix/0/values/8/value` | `0` |
| `/matrix/0/values/8/unit` | `"count"` |
| `/matrix/0/fixture_hashes/0` | `1431096453509785832` |
| `/matrix/0/fixture_hashes/1` | `5466060815846980033` |
| `/matrix/0/fixture_hashes/2` | `8526278817898170285` |
| `/matrix/0/fixture_hashes/3` | `2675255426310783444` |
| `/matrix/0/fixture_hashes/4` | `10494942409035688541` |
| `/matrix/0/fixture_hashes/5` | `7390650343477109358` |
| `/matrix/0/fixture_hashes/6` | `14725012844580521641` |
| `/matrix/0/fixture_hashes/7` | `14001126938762718496` |
| `/matrix/0/fixture_hashes/8` | `2400058324701230849` |
| `/matrix/0/fixture_hashes/9` | `10732887959698602526` |
| `/matrix/0/fixture_hashes/10` | `5915366692680379584` |
| `/matrix/0/fixture_hashes/11` | `14334879908416984375` |
| `/matrix/0/fixture_hashes/12` | `6577404406665493359` |
| `/matrix/0/fixture_hashes/13` | `16513243213841685896` |
| `/matrix/0/fixture_hashes/14` | `9721495563768596328` |
| `/matrix/0/fixture_hashes/15` | `13937513265211812359` |
| `/matrix/0/fixture_hashes/16` | `9628435969084559192` |
| `/matrix/0/fixture_hashes/17` | `18192646134465658732` |
| `/matrix/0/fixture_hashes/18` | `18426867884952182109` |
| `/matrix/0/fixture_hashes/19` | `11807578939564631579` |
| `/matrix/1/capability` | `"shape_cast_overlap"` |
| `/matrix/1/status` | `"pass"` |
| `/matrix/1/fallback` | `null` |
| `/matrix/1/functional_status` | `"pass"` |
| `/matrix/1/functional_fallback` | `null` |
| `/matrix/1/detail` | `"the first three-meter sphere cast handle agrees with overlap at contact"` |
| `/matrix/1/peak_body_count` | `1` |
| `/matrix/1/peak_shape_count` | `1` |
| `/matrix/1/peak_joint_count` | `0` |
| `/matrix/1/peak_awake_count` | `0` |
| `/matrix/1/peak_contact_count` | `0` |
| `/matrix/1/values/0/name` | `"cast_distance"` |
| `/matrix/1/values/0/value` | `3` |
| `/matrix/1/values/0/unit` | `"m"` |
| `/matrix/1/values/1/name` | `"first_handle_index"` |
| `/matrix/1/values/1/value` | `1` |
| `/matrix/1/values/1/unit` | `"index"` |
| `/matrix/1/values/2/name` | `"first_handle_matches_overlap"` |
| `/matrix/1/values/2/value` | `1` |
| `/matrix/1/values/2/unit` | `"bool"` |
| `/matrix/1/values/3/name` | `"fraction"` |
| `/matrix/1/values/3/value` | `0.810775876045227` |
| `/matrix/1/values/3/unit` | `"ratio"` |
| `/matrix/1/fixture_hashes/0` | `8388802905423810155` |
| `/matrix/2/capability` | `"contact_hit_events"` |
| `/matrix/2/status` | `"pass"` |
| `/matrix/2/fallback` | `null` |
| `/matrix/2/functional_status` | `"pass"` |
| `/matrix/2/functional_fallback` | `null` |
| `/matrix/2/detail` | `"real hit data is finite, energetic, material-tagged, and pair-unique at six substeps"` |
| `/matrix/2/peak_body_count` | `2` |
| `/matrix/2/peak_shape_count` | `2` |
| `/matrix/2/peak_joint_count` | `0` |
| `/matrix/2/peak_awake_count` | `2` |
| `/matrix/2/peak_contact_count` | `1` |
| `/matrix/2/values/0/name` | `"approach_speed"` |
| `/matrix/2/values/0/value` | `15` |
| `/matrix/2/values/0/unit` | `"m/s"` |
| `/matrix/2/values/1/name` | `"effective_mass"` |
| `/matrix/2/values/1/value` | `41.88790512084961` |
| `/matrix/2/values/1/unit` | `"kg"` |
| `/matrix/2/values/2/name` | `"derived_energy"` |
| `/matrix/2/values/2/value` | `4105.0146484375` |
| `/matrix/2/values/2/unit` | `"J"` |
| `/matrix/2/values/3/name` | `"normal_length"` |
| `/matrix/2/values/3/value` | `1` |
| `/matrix/2/values/3/unit` | `"ratio"` |
| `/matrix/2/values/4/name` | `"material_a"` |
| `/matrix/2/values/4/value` | `111` |
| `/matrix/2/values/4/unit` | `"id"` |
| `/matrix/2/values/5/name` | `"material_b"` |
| `/matrix/2/values/5/value` | `222` |
| `/matrix/2/values/5/unit` | `"id"` |
| `/matrix/2/values/6/name` | `"unique_pairs"` |
| `/matrix/2/values/6/value` | `1` |
| `/matrix/2/values/6/unit` | `"count"` |
| `/matrix/2/values/7/name` | `"substeps"` |
| `/matrix/2/values/7/value` | `6` |
| `/matrix/2/values/7/unit` | `"count"` |
| `/matrix/2/fixture_hashes/0` | `10155543446163919611` |
| `/matrix/3/capability` | `"joint_force_torque"` |
| `/matrix/3/status` | `"pass"` |
| `/matrix/3/fallback` | `null` |
| `/matrix/3/functional_status` | `"pass"` |
| `/matrix/3/functional_fallback` | `null` |
| `/matrix/3/detail` | `"joint reaction grows within 50 N tolerance and crosses 10 kN"` |
| `/matrix/3/peak_body_count` | `2` |
| `/matrix/3/peak_shape_count` | `2` |
| `/matrix/3/peak_joint_count` | `1` |
| `/matrix/3/peak_awake_count` | `1` |
| `/matrix/3/peak_contact_count` | `1` |
| `/matrix/3/values/0/name` | `"monotonic_tolerance"` |
| `/matrix/3/values/0/value` | `50` |
| `/matrix/3/values/0/unit` | `"N"` |
| `/matrix/3/values/1/name` | `"maximum_force"` |
| `/matrix/3/values/1/value` | `12693.1640625` |
| `/matrix/3/values/1/unit` | `"N"` |
| `/matrix/3/values/2/name` | `"rupture_threshold"` |
| `/matrix/3/values/2/value` | `10000` |
| `/matrix/3/values/2/unit` | `"N"` |
| `/matrix/3/values/3/name` | `"maximum_deformation"` |
| `/matrix/3/values/3/value` | `0.01363062858581543` |
| `/matrix/3/values/3/unit` | `"m"` |
| `/matrix/3/values/4/name` | `"deformation_threshold"` |
| `/matrix/3/values/4/value` | `0.01` |
| `/matrix/3/values/4/unit` | `"m"` |
| `/matrix/3/values/5/name` | `"consecutive_deformation_ticks"` |
| `/matrix/3/values/5/value` | `0` |
| `/matrix/3/values/5/unit` | `"ticks"` |
| `/matrix/3/fixture_hashes/0` | `14239377403397705414` |
| `/matrix/4/capability` | `"hulls_compounds"` |
| `/matrix/4/status` | `"pass"` |
| `/matrix/4/fallback` | `null` |
| `/matrix/4/functional_status` | `"pass"` |
| `/matrix/4/functional_fallback` | `null` |
| `/matrix/4/detail` | `"one compound containing eight hull children has valid mass, bounds, and contact"` |
| `/matrix/4/peak_body_count` | `2` |
| `/matrix/4/peak_shape_count` | `9` |
| `/matrix/4/peak_joint_count` | `0` |
| `/matrix/4/peak_awake_count` | `1` |
| `/matrix/4/peak_contact_count` | `8` |
| `/matrix/4/values/0/name` | `"hull_count"` |
| `/matrix/4/values/0/value` | `8` |
| `/matrix/4/values/0/unit` | `"count"` |
| `/matrix/4/values/1/name` | `"expected_mass"` |
| `/matrix/4/values/1/value` | `42.666666666666664` |
| `/matrix/4/values/1/unit` | `"kg"` |
| `/matrix/4/values/2/name` | `"mass"` |
| `/matrix/4/values/2/value` | `42.66667175292969` |
| `/matrix/4/values/2/unit` | `"kg"` |
| `/matrix/4/values/3/name` | `"bounds_lower_x"` |
| `/matrix/4/values/3/value` | `-1.6200000047683716` |
| `/matrix/4/values/3/unit` | `"m"` |
| `/matrix/4/values/4/name` | `"bounds_lower_y"` |
| `/matrix/4/values/4/value` | `2.7799999713897705` |
| `/matrix/4/values/4/unit` | `"m"` |
| `/matrix/4/values/5/name` | `"bounds_lower_z"` |
| `/matrix/4/values/5/value` | `-0.2199999988079071` |
| `/matrix/4/values/5/unit` | `"m"` |
| `/matrix/4/values/6/name` | `"bounds_upper_x"` |
| `/matrix/4/values/6/value` | `1.6200000047683716` |
| `/matrix/4/values/6/unit` | `"m"` |
| `/matrix/4/values/7/name` | `"bounds_upper_y"` |
| `/matrix/4/values/7/value` | `3.2200000286102295` |
| `/matrix/4/values/7/unit` | `"m"` |
| `/matrix/4/values/8/name` | `"bounds_upper_z"` |
| `/matrix/4/values/8/value` | `0.2199999988079071` |
| `/matrix/4/values/8/unit` | `"m"` |
| `/matrix/4/values/9/name` | `"bounds_tolerance"` |
| `/matrix/4/values/9/value` | `0.0001` |
| `/matrix/4/values/9/unit` | `"m"` |
| `/matrix/4/values/10/name` | `"contacted"` |
| `/matrix/4/values/10/value` | `1` |
| `/matrix/4/values/10/unit` | `"bool"` |
| `/matrix/4/values/11/name` | `"state_valid"` |
| `/matrix/4/values/11/value` | `1` |
| `/matrix/4/values/11/unit` | `"bool"` |
| `/matrix/4/values/12/name` | `"box3d_allocator_baseline_bytes"` |
| `/matrix/4/values/12/value` | `0` |
| `/matrix/4/values/12/unit` | `"bytes"` |
| `/matrix/4/values/13/name` | `"box3d_allocator_final_bytes"` |
| `/matrix/4/values/13/value` | `0` |
| `/matrix/4/values/13/unit` | `"bytes"` |
| `/matrix/4/fixture_hashes/0` | `7710058609812386530` |
| `/matrix/5/capability` | `"radial_sleep"` |
| `/matrix/5/status` | `"pass"` |
| `/matrix/5/fallback` | `null` |
| `/matrix/5/functional_status` | `"pass"` |
| `/matrix/5/functional_fallback` | `null` |
| `/matrix/5/detail` | `"the public radial-gravity pile satisfies every fixed sleep limit"` |
| `/matrix/5/peak_body_count` | `81` |
| `/matrix/5/peak_shape_count` | `81` |
| `/matrix/5/peak_joint_count` | `0` |
| `/matrix/5/peak_awake_count` | `80` |
| `/matrix/5/peak_contact_count` | `204` |
| `/matrix/5/values/0/name` | `"sleep_ratio"` |
| `/matrix/5/values/0/value` | `1` |
| `/matrix/5/values/0/unit` | `"ratio"` |
| `/matrix/5/values/1/name` | `"p95_linear_speed"` |
| `/matrix/5/values/1/value` | `0` |
| `/matrix/5/values/1/unit` | `"m/s"` |
| `/matrix/5/values/2/name` | `"p95_angular_speed"` |
| `/matrix/5/values/2/value` | `0` |
| `/matrix/5/values/2/unit` | `"rad/s"` |
| `/matrix/5/values/3/name` | `"max_penetration"` |
| `/matrix/5/values/3/value` | `0.0015351474285125732` |
| `/matrix/5/values/3/unit` | `"m"` |
| `/matrix/5/values/4/name` | `"energy_growth"` |
| `/matrix/5/values/4/value` | `0` |
| `/matrix/5/values/4/unit` | `"ratio"` |
| `/matrix/5/fixture_hashes/0` | `10363635776067367757` |
| `/matrix/6/capability` | `"batch_lifecycle"` |
| `/matrix/6/status` | `"pass"` |
| `/matrix/6/fallback` | `null` |
| `/matrix/6/functional_status` | `"pass"` |
| `/matrix/6/functional_fallback` | `null` |
| `/matrix/6/detail` | `"10,000 create-destroy cycles preserved handle generations without persistent growth"` |
| `/matrix/6/peak_body_count` | `1` |
| `/matrix/6/peak_shape_count` | `1` |
| `/matrix/6/peak_joint_count` | `0` |
| `/matrix/6/peak_awake_count` | `1` |
| `/matrix/6/peak_contact_count` | `0` |
| `/matrix/6/values/0/name` | `"generation_cycles"` |
| `/matrix/6/values/0/value` | `10000` |
| `/matrix/6/values/0/unit` | `"count"` |
| `/matrix/6/values/1/name` | `"invalid_handles"` |
| `/matrix/6/values/1/value` | `0` |
| `/matrix/6/values/1/unit` | `"count"` |
| `/matrix/6/values/2/name` | `"private_commit_available"` |
| `/matrix/6/values/2/value` | `1` |
| `/matrix/6/values/2/unit` | `"bool"` |
| `/matrix/6/values/3/name` | `"private_commit_growth"` |
| `/matrix/6/values/3/value` | `0` |
| `/matrix/6/values/3/unit` | `"ratio"` |
| `/matrix/6/values/4/name` | `"private_commit_baseline_bytes"` |
| `/matrix/6/values/4/value` | `4562944` |
| `/matrix/6/values/4/unit` | `"bytes"` |
| `/matrix/6/values/5/name` | `"private_commit_final_bytes"` |
| `/matrix/6/values/5/value` | `4562944` |
| `/matrix/6/values/5/unit` | `"bytes"` |
| `/matrix/6/values/6/name` | `"working_set_baseline_bytes"` |
| `/matrix/6/values/6/value` | `7446528` |
| `/matrix/6/values/6/unit` | `"bytes"` |
| `/matrix/6/values/7/name` | `"working_set_final_bytes"` |
| `/matrix/6/values/7/value` | `7446528` |
| `/matrix/6/values/7/unit` | `"bytes"` |
| `/matrix/6/values/8/name` | `"crt_normal_count_delta"` |
| `/matrix/6/values/8/value` | `0` |
| `/matrix/6/values/8/unit` | `"count"` |
| `/matrix/6/values/9/name` | `"crt_normal_bytes_delta"` |
| `/matrix/6/values/9/value` | `0` |
| `/matrix/6/values/9/unit` | `"bytes"` |
| `/matrix/6/values/10/name` | `"crt_client_count_delta"` |
| `/matrix/6/values/10/value` | `0` |
| `/matrix/6/values/10/unit` | `"count"` |
| `/matrix/6/values/11/name` | `"crt_client_bytes_delta"` |
| `/matrix/6/values/11/value` | `0` |
| `/matrix/6/values/11/unit` | `"bytes"` |
| `/matrix/6/values/12/name` | `"box3d_allocator_baseline_bytes"` |
| `/matrix/6/values/12/value` | `0` |
| `/matrix/6/values/12/unit` | `"bytes"` |
| `/matrix/6/values/13/name` | `"box3d_allocator_final_bytes"` |
| `/matrix/6/values/13/value` | `0` |
| `/matrix/6/values/13/unit` | `"bytes"` |
| `/matrix/6/fixture_hashes/0` | `43224550866945` |
| `/matrix/7/capability` | `"upstream_replay"` |
| `/matrix/7/status` | `"pass"` |
| `/matrix/7/fallback` | `null` |
| `/matrix/7/functional_status` | `"pass"` |
| `/matrix/7/functional_fallback` | `null` |
| `/matrix/7/detail` | `"the pinned official recorder saved, loaded, and validated a minimal replay"` |
| `/matrix/7/peak_body_count` | `1` |
| `/matrix/7/peak_shape_count` | `1` |
| `/matrix/7/peak_joint_count` | `0` |
| `/matrix/7/peak_awake_count` | `1` |
| `/matrix/7/peak_contact_count` | `0` |
| `/matrix/7/values/0/name` | `"saved"` |
| `/matrix/7/values/0/value` | `1` |
| `/matrix/7/values/0/unit` | `"bool"` |
| `/matrix/7/values/1/name` | `"loaded"` |
| `/matrix/7/values/1/value` | `1` |
| `/matrix/7/values/1/unit` | `"bool"` |
| `/matrix/7/values/2/name` | `"validated"` |
| `/matrix/7/values/2/value` | `1` |
| `/matrix/7/values/2/unit` | `"bool"` |
| `/matrix/7/values/3/name` | `"recording_bytes"` |
| `/matrix/7/values/3/value` | `6982` |
| `/matrix/7/values/3/unit` | `"bytes"` |
| `/matrix/7/values/4/name` | `"temporary_file_removed"` |
| `/matrix/7/values/4/value` | `1` |
| `/matrix/7/values/4/unit` | `"bool"` |
| `/matrix/7/values/5/name` | `"box3d_allocator_baseline_bytes"` |
| `/matrix/7/values/5/value` | `0` |
| `/matrix/7/values/5/unit` | `"bytes"` |
| `/matrix/7/values/6/name` | `"box3d_allocator_final_bytes"` |
| `/matrix/7/values/6/value` | `0` |
| `/matrix/7/values/6/unit` | `"bytes"` |
| `/matrix/7/fixture_hashes/0` | `13184605773762244167` |
| `/warnings` | `[]` |
| `/violations` | `[]` |

### Release scenario `radial_fall`

Topology fixture `dynamic/shape/joint`: `1/2/0`. Observed peak `body/shape/joint;awake/contact`: `2/2/0;1/1`.

| JSON pointer | Value |
| --- | --- |
| `/name` | `"radial_fall"` |
| `/seed` | `1` |
| `/substeps` | `4` |
| `/ticks` | `600` |
| `/final_hash` | `12100112409900625846` |
| `/hashes/0` | `12100112409900625846` |
| `/hashes/1` | `12100112409900625846` |
| `/repeat_observations/0/repeat` | `1` |
| `/repeat_observations/0/hash` | `12100112409900625846` |
| `/repeat_observations/0/peak_body_count` | `2` |
| `/repeat_observations/0/peak_shape_count` | `2` |
| `/repeat_observations/0/peak_joint_count` | `0` |
| `/repeat_observations/0/peak_awake_count` | `1` |
| `/repeat_observations/0/peak_contact_count` | `1` |
| `/repeat_observations/0/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/0/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `false` |
| `/repeat_observations/0/memory/private_commit/stable` | `false` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/available` | `false` |
| `/repeat_observations/0/memory/working_set/stable` | `false` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/0/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/0/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/0/crt/applicable` | `false` |
| `/repeat_observations/0/crt/balanced` | `true` |
| `/repeat_observations/0/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/0/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/0/crt/client_block_count_delta` | `0` |
| `/repeat_observations/0/crt/client_block_bytes_delta` | `0` |
| `/repeat_observations/1/repeat` | `2` |
| `/repeat_observations/1/hash` | `12100112409900625846` |
| `/repeat_observations/1/peak_body_count` | `2` |
| `/repeat_observations/1/peak_shape_count` | `2` |
| `/repeat_observations/1/peak_joint_count` | `0` |
| `/repeat_observations/1/peak_awake_count` | `1` |
| `/repeat_observations/1/peak_contact_count` | `1` |
| `/repeat_observations/1/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/1/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/gate_applied` | `false` |
| `/repeat_observations/1/memory/budget_qualified` | `false` |
| `/repeat_observations/1/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/private_commit/available` | `false` |
| `/repeat_observations/1/memory/private_commit/stable` | `false` |
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/available` | `false` |
| `/repeat_observations/1/memory/working_set/stable` | `false` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/1/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/1/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/1/crt/applicable` | `false` |
| `/repeat_observations/1/crt/balanced` | `true` |
| `/repeat_observations/1/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/1/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/1/crt/client_block_count_delta` | `0` |
| `/repeat_observations/1/crt/client_block_bytes_delta` | `0` |
| `/contact_before_pile_exit` | `false` |
| `/ccd_primary_pass_count` | `0` |
| `/ccd_fallback_pass_count` | `0` |
| `/projectile_speed` | `0` |
| `/surface_separation` | `-5.951523780822754e-05` |
| `/final_linear_speed` | `0` |
| `/final_angular_speed` | `0` |
| `/p95_linear_speed` | `0` |
| `/p95_angular_speed` | `0` |
| `/sleep_ratio` | `0` |
| `/max_penetration` | `0` |
| `/minimum_density` | `0` |
| `/maximum_density` | `0` |
| `/energy_at_tick_600` | `0` |
| `/max_energy_growth_ratio` | `0` |
| `/dynamic_body_count` | `1` |
| `/shape_count` | `2` |
| `/joint_count` | `0` |
| `/peak_body_count` | `2` |
| `/peak_shape_count` | `2` |
| `/peak_joint_count` | `0` |
| `/peak_awake_count` | `1` |
| `/peak_contact_count` | `1` |
| `/step_ms/min` | `0.0002` |
| `/step_ms/p50` | `0.0003` |
| `/step_ms/p95` | `0.0052` |
| `/step_ms/max` | `0.1157` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unavailable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `false` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `false` |
| `/memory/private_commit/baseline_last_bytes` | `0` |
| `/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/memory/private_commit/baseline_median_bytes` | `0` |
| `/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/memory/private_commit/final_last_bytes` | `0` |
| `/memory/private_commit/final_full_min_bytes` | `0` |
| `/memory/private_commit/final_central_min_bytes` | `0` |
| `/memory/private_commit/final_median_bytes` | `0` |
| `/memory/private_commit/final_central_max_bytes` | `0` |
| `/memory/private_commit/final_full_max_bytes` | `0` |
| `/memory/private_commit/peak_bytes` | `0` |
| `/memory/private_commit/growth_ratio` | `0` |
| `/memory/private_commit/instant_growth_ratio` | `0` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/memory/private_commit/measured_full_span_ratio` | `0` |
| `/memory/private_commit/warmup_samples` | `[]` |
| `/memory/private_commit/measured_samples` | `[]` |
| `/memory/working_set/available` | `false` |
| `/memory/working_set/stable` | `false` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"unavailable"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `0` |
| `/memory/working_set/baseline_full_min_bytes` | `0` |
| `/memory/working_set/baseline_central_min_bytes` | `0` |
| `/memory/working_set/baseline_median_bytes` | `0` |
| `/memory/working_set/baseline_central_max_bytes` | `0` |
| `/memory/working_set/baseline_full_max_bytes` | `0` |
| `/memory/working_set/final_last_bytes` | `0` |
| `/memory/working_set/final_full_min_bytes` | `0` |
| `/memory/working_set/final_central_min_bytes` | `0` |
| `/memory/working_set/final_median_bytes` | `0` |
| `/memory/working_set/final_central_max_bytes` | `0` |
| `/memory/working_set/final_full_max_bytes` | `0` |
| `/memory/working_set/peak_bytes` | `0` |
| `/memory/working_set/growth_ratio` | `0` |
| `/memory/working_set/instant_growth_ratio` | `0` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/memory/working_set/warmup_full_span_ratio` | `0` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/memory/working_set/measured_full_span_ratio` | `0` |
| `/memory/working_set/warmup_samples` | `[]` |
| `/memory/working_set/measured_samples` | `[]` |
| `/box3d_allocator/baseline_bytes` | `0` |
| `/box3d_allocator/final_bytes` | `0` |
| `/box3d_allocator/max_abs_delta` | `0` |
| `/box3d_allocator/exact_return` | `true` |
| `/box3d_allocator/warmup_post_teardown` | `[]` |
| `/box3d_allocator/measured_post_teardown` | `[]` |
| `/crt/applicable` | `false` |
| `/crt/balanced` | `true` |
| `/crt/normal_block_count_delta` | `0` |
| `/crt/normal_block_bytes_delta` | `0` |
| `/crt/client_block_count_delta` | `0` |
| `/crt/client_block_bytes_delta` | `0` |
| `/warmup_ticks` | `0` |
| `/measurement_ticks` | `0` |
| `/allocator_warmup_cycles` | `0` |
| `/stress_cycles` | `0` |
| `/fallback` | `""` |
| `/limits/0/name` | `"surface_separation_min"` |
| `/limits/0/comparison` | `">="` |
| `/limits/0/value` | `-0.02` |
| `/limits/0/unit` | `"m"` |
| `/limits/1/name` | `"surface_separation_max"` |
| `/limits/1/comparison` | `"<="` |
| `/limits/1/value` | `0.03` |
| `/limits/1/unit` | `"m"` |
| `/limits/2/name` | `"final_linear_speed"` |
| `/limits/2/comparison` | `"<"` |
| `/limits/2/value` | `0.05` |
| `/limits/2/unit` | `"m/s"` |
| `/limits/3/name` | `"final_angular_speed"` |
| `/limits/3/comparison` | `"<"` |
| `/limits/3/value` | `0.1` |
| `/limits/3/unit` | `"rad/s"` |
| `/limits/4/name` | `"max_energy_growth_ratio"` |
| `/limits/4/comparison` | `"<="` |
| `/limits/4/value` | `0.02` |
| `/limits/4/unit` | `"ratio"` |
| `/metrics/0/name` | `"surface_separation"` |
| `/metrics/0/value` | `-5.951523780822754e-05` |
| `/metrics/0/unit` | `"m"` |
| `/metrics/1/name` | `"final_linear_speed"` |
| `/metrics/1/value` | `0` |
| `/metrics/1/unit` | `"m/s"` |
| `/metrics/2/name` | `"final_angular_speed"` |
| `/metrics/2/value` | `0` |
| `/metrics/2/unit` | `"rad/s"` |
| `/metrics/3/name` | `"max_energy_growth_ratio"` |
| `/metrics/3/value` | `0` |
| `/metrics/3/unit` | `"ratio"` |
| `/metrics/4/name` | `"energy_window"` |
| `/metrics/4/value` | `120` |
| `/metrics/4/unit` | `"ticks"` |
| `/metrics/5/name` | `"energy_epsilon"` |
| `/metrics/5/value` | `1e-09` |
| `/metrics/5/unit` | `"J"` |
| `/matrix` | `[]` |
| `/warnings` | `[]` |
| `/violations` | `[]` |

### Release scenario `projectile_pile`

Topology fixture `dynamic/shape/joint`: `121/123/0`. Observed peak `body/shape/joint;awake/contact`: `123/123/0;121/221`.

| JSON pointer | Value |
| --- | --- |
| `/name` | `"projectile_pile"` |
| `/seed` | `1` |
| `/substeps` | `4` |
| `/ticks` | `11` |
| `/final_hash` | `1431096453509785832` |
| `/hashes/0` | `1431096453509785832` |
| `/hashes/1` | `1431096453509785832` |
| `/repeat_observations/0/repeat` | `1` |
| `/repeat_observations/0/hash` | `1431096453509785832` |
| `/repeat_observations/0/peak_body_count` | `123` |
| `/repeat_observations/0/peak_shape_count` | `123` |
| `/repeat_observations/0/peak_joint_count` | `0` |
| `/repeat_observations/0/peak_awake_count` | `121` |
| `/repeat_observations/0/peak_contact_count` | `221` |
| `/repeat_observations/0/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/0/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `false` |
| `/repeat_observations/0/memory/private_commit/stable` | `false` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/available` | `false` |
| `/repeat_observations/0/memory/working_set/stable` | `false` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/0/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/0/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/0/crt/applicable` | `false` |
| `/repeat_observations/0/crt/balanced` | `true` |
| `/repeat_observations/0/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/0/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/0/crt/client_block_count_delta` | `0` |
| `/repeat_observations/0/crt/client_block_bytes_delta` | `0` |
| `/repeat_observations/1/repeat` | `2` |
| `/repeat_observations/1/hash` | `1431096453509785832` |
| `/repeat_observations/1/peak_body_count` | `123` |
| `/repeat_observations/1/peak_shape_count` | `123` |
| `/repeat_observations/1/peak_joint_count` | `0` |
| `/repeat_observations/1/peak_awake_count` | `121` |
| `/repeat_observations/1/peak_contact_count` | `221` |
| `/repeat_observations/1/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/1/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/gate_applied` | `false` |
| `/repeat_observations/1/memory/budget_qualified` | `false` |
| `/repeat_observations/1/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/private_commit/available` | `false` |
| `/repeat_observations/1/memory/private_commit/stable` | `false` |
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/available` | `false` |
| `/repeat_observations/1/memory/working_set/stable` | `false` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/1/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/1/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/1/crt/applicable` | `false` |
| `/repeat_observations/1/crt/balanced` | `true` |
| `/repeat_observations/1/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/1/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/1/crt/client_block_count_delta` | `0` |
| `/repeat_observations/1/crt/client_block_bytes_delta` | `0` |
| `/contact_before_pile_exit` | `true` |
| `/ccd_primary_pass_count` | `20` |
| `/ccd_fallback_pass_count` | `0` |
| `/projectile_speed` | `35` |
| `/surface_separation` | `0` |
| `/final_linear_speed` | `0` |
| `/final_angular_speed` | `0` |
| `/p95_linear_speed` | `0` |
| `/p95_angular_speed` | `0` |
| `/sleep_ratio` | `0` |
| `/max_penetration` | `0` |
| `/minimum_density` | `0` |
| `/maximum_density` | `0` |
| `/energy_at_tick_600` | `0` |
| `/max_energy_growth_ratio` | `0` |
| `/dynamic_body_count` | `121` |
| `/shape_count` | `123` |
| `/joint_count` | `0` |
| `/peak_body_count` | `123` |
| `/peak_shape_count` | `123` |
| `/peak_joint_count` | `0` |
| `/peak_awake_count` | `121` |
| `/peak_contact_count` | `221` |
| `/step_ms/min` | `0.2416` |
| `/step_ms/p50` | `0.2499` |
| `/step_ms/p95` | `1.3023` |
| `/step_ms/max` | `1.3023` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unavailable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `false` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `false` |
| `/memory/private_commit/baseline_last_bytes` | `0` |
| `/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/memory/private_commit/baseline_median_bytes` | `0` |
| `/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/memory/private_commit/final_last_bytes` | `0` |
| `/memory/private_commit/final_full_min_bytes` | `0` |
| `/memory/private_commit/final_central_min_bytes` | `0` |
| `/memory/private_commit/final_median_bytes` | `0` |
| `/memory/private_commit/final_central_max_bytes` | `0` |
| `/memory/private_commit/final_full_max_bytes` | `0` |
| `/memory/private_commit/peak_bytes` | `0` |
| `/memory/private_commit/growth_ratio` | `0` |
| `/memory/private_commit/instant_growth_ratio` | `0` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/memory/private_commit/measured_full_span_ratio` | `0` |
| `/memory/private_commit/warmup_samples` | `[]` |
| `/memory/private_commit/measured_samples` | `[]` |
| `/memory/working_set/available` | `false` |
| `/memory/working_set/stable` | `false` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"unavailable"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `0` |
| `/memory/working_set/baseline_full_min_bytes` | `0` |
| `/memory/working_set/baseline_central_min_bytes` | `0` |
| `/memory/working_set/baseline_median_bytes` | `0` |
| `/memory/working_set/baseline_central_max_bytes` | `0` |
| `/memory/working_set/baseline_full_max_bytes` | `0` |
| `/memory/working_set/final_last_bytes` | `0` |
| `/memory/working_set/final_full_min_bytes` | `0` |
| `/memory/working_set/final_central_min_bytes` | `0` |
| `/memory/working_set/final_median_bytes` | `0` |
| `/memory/working_set/final_central_max_bytes` | `0` |
| `/memory/working_set/final_full_max_bytes` | `0` |
| `/memory/working_set/peak_bytes` | `0` |
| `/memory/working_set/growth_ratio` | `0` |
| `/memory/working_set/instant_growth_ratio` | `0` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/memory/working_set/warmup_full_span_ratio` | `0` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/memory/working_set/measured_full_span_ratio` | `0` |
| `/memory/working_set/warmup_samples` | `[]` |
| `/memory/working_set/measured_samples` | `[]` |
| `/box3d_allocator/baseline_bytes` | `0` |
| `/box3d_allocator/final_bytes` | `0` |
| `/box3d_allocator/max_abs_delta` | `0` |
| `/box3d_allocator/exact_return` | `true` |
| `/box3d_allocator/warmup_post_teardown` | `[]` |
| `/box3d_allocator/measured_post_teardown` | `[]` |
| `/crt/applicable` | `false` |
| `/crt/balanced` | `true` |
| `/crt/normal_block_count_delta` | `0` |
| `/crt/normal_block_bytes_delta` | `0` |
| `/crt/client_block_count_delta` | `0` |
| `/crt/client_block_bytes_delta` | `0` |
| `/warmup_ticks` | `0` |
| `/measurement_ticks` | `0` |
| `/allocator_warmup_cycles` | `0` |
| `/stress_cycles` | `0` |
| `/fallback` | `""` |
| `/limits/0/name` | `"primary_seed_passes"` |
| `/limits/0/comparison` | `"=="` |
| `/limits/0/value` | `20` |
| `/limits/0/unit` | `"seeds"` |
| `/limits/1/name` | `"fallback_seed_passes"` |
| `/limits/1/comparison` | `"=="` |
| `/limits/1/value` | `0` |
| `/limits/1/unit` | `"seeds"` |
| `/limits/2/name` | `"invalid_states"` |
| `/limits/2/comparison` | `"=="` |
| `/limits/2/value` | `0` |
| `/limits/2/unit` | `"states"` |
| `/metrics/0/name` | `"primary_seed_passes"` |
| `/metrics/0/value` | `20` |
| `/metrics/0/unit` | `"seeds"` |
| `/metrics/1/name` | `"fallback_seed_passes"` |
| `/metrics/1/value` | `0` |
| `/metrics/1/unit` | `"seeds"` |
| `/metrics/2/name` | `"projectile_speed"` |
| `/metrics/2/value` | `35` |
| `/metrics/2/unit` | `"m/s"` |
| `/metrics/3/name` | `"primary_invalid_states"` |
| `/metrics/3/value` | `0` |
| `/metrics/3/unit` | `"count"` |
| `/metrics/4/name` | `"fallback_invalid_states"` |
| `/metrics/4/value` | `0` |
| `/metrics/4/unit` | `"count"` |
| `/metrics/5/name` | `"fallback_seed_evaluated"` |
| `/metrics/5/value` | `0` |
| `/metrics/5/unit` | `"bool"` |
| `/matrix` | `[]` |
| `/warnings` | `[]` |
| `/violations` | `[]` |

### Release scenario `radial_pile`

Topology fixture `dynamic/shape/joint`: `80/81/0`. Observed peak `body/shape/joint;awake/contact`: `81/81/0;80/204`.

| JSON pointer | Value |
| --- | --- |
| `/name` | `"radial_pile"` |
| `/seed` | `1` |
| `/substeps` | `4` |
| `/ticks` | `1800` |
| `/final_hash` | `10363635776067367757` |
| `/hashes/0` | `10363635776067367757` |
| `/hashes/1` | `10363635776067367757` |
| `/repeat_observations/0/repeat` | `1` |
| `/repeat_observations/0/hash` | `10363635776067367757` |
| `/repeat_observations/0/peak_body_count` | `81` |
| `/repeat_observations/0/peak_shape_count` | `81` |
| `/repeat_observations/0/peak_joint_count` | `0` |
| `/repeat_observations/0/peak_awake_count` | `80` |
| `/repeat_observations/0/peak_contact_count` | `204` |
| `/repeat_observations/0/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/0/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `false` |
| `/repeat_observations/0/memory/private_commit/stable` | `false` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/available` | `false` |
| `/repeat_observations/0/memory/working_set/stable` | `false` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/0/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/0/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/0/crt/applicable` | `false` |
| `/repeat_observations/0/crt/balanced` | `true` |
| `/repeat_observations/0/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/0/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/0/crt/client_block_count_delta` | `0` |
| `/repeat_observations/0/crt/client_block_bytes_delta` | `0` |
| `/repeat_observations/1/repeat` | `2` |
| `/repeat_observations/1/hash` | `10363635776067367757` |
| `/repeat_observations/1/peak_body_count` | `81` |
| `/repeat_observations/1/peak_shape_count` | `81` |
| `/repeat_observations/1/peak_joint_count` | `0` |
| `/repeat_observations/1/peak_awake_count` | `80` |
| `/repeat_observations/1/peak_contact_count` | `204` |
| `/repeat_observations/1/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/1/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/gate_applied` | `false` |
| `/repeat_observations/1/memory/budget_qualified` | `false` |
| `/repeat_observations/1/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/private_commit/available` | `false` |
| `/repeat_observations/1/memory/private_commit/stable` | `false` |
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/available` | `false` |
| `/repeat_observations/1/memory/working_set/stable` | `false` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/1/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/1/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/1/crt/applicable` | `false` |
| `/repeat_observations/1/crt/balanced` | `true` |
| `/repeat_observations/1/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/1/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/1/crt/client_block_count_delta` | `0` |
| `/repeat_observations/1/crt/client_block_bytes_delta` | `0` |
| `/contact_before_pile_exit` | `false` |
| `/ccd_primary_pass_count` | `0` |
| `/ccd_fallback_pass_count` | `0` |
| `/projectile_speed` | `0` |
| `/surface_separation` | `0` |
| `/final_linear_speed` | `0` |
| `/final_angular_speed` | `0` |
| `/p95_linear_speed` | `0` |
| `/p95_angular_speed` | `0` |
| `/sleep_ratio` | `1` |
| `/max_penetration` | `0.0015351474285125732` |
| `/minimum_density` | `480` |
| `/maximum_density` | `520` |
| `/energy_at_tick_600` | `0` |
| `/max_energy_growth_ratio` | `0` |
| `/dynamic_body_count` | `80` |
| `/shape_count` | `81` |
| `/joint_count` | `0` |
| `/peak_body_count` | `81` |
| `/peak_shape_count` | `81` |
| `/peak_joint_count` | `0` |
| `/peak_awake_count` | `80` |
| `/peak_contact_count` | `204` |
| `/step_ms/min` | `0.003` |
| `/step_ms/p50` | `0.0037` |
| `/step_ms/p95` | `0.008` |
| `/step_ms/max` | `8.3347` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unavailable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `false` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `false` |
| `/memory/private_commit/baseline_last_bytes` | `0` |
| `/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/memory/private_commit/baseline_median_bytes` | `0` |
| `/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/memory/private_commit/final_last_bytes` | `0` |
| `/memory/private_commit/final_full_min_bytes` | `0` |
| `/memory/private_commit/final_central_min_bytes` | `0` |
| `/memory/private_commit/final_median_bytes` | `0` |
| `/memory/private_commit/final_central_max_bytes` | `0` |
| `/memory/private_commit/final_full_max_bytes` | `0` |
| `/memory/private_commit/peak_bytes` | `0` |
| `/memory/private_commit/growth_ratio` | `0` |
| `/memory/private_commit/instant_growth_ratio` | `0` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/memory/private_commit/measured_full_span_ratio` | `0` |
| `/memory/private_commit/warmup_samples` | `[]` |
| `/memory/private_commit/measured_samples` | `[]` |
| `/memory/working_set/available` | `false` |
| `/memory/working_set/stable` | `false` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"unavailable"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `0` |
| `/memory/working_set/baseline_full_min_bytes` | `0` |
| `/memory/working_set/baseline_central_min_bytes` | `0` |
| `/memory/working_set/baseline_median_bytes` | `0` |
| `/memory/working_set/baseline_central_max_bytes` | `0` |
| `/memory/working_set/baseline_full_max_bytes` | `0` |
| `/memory/working_set/final_last_bytes` | `0` |
| `/memory/working_set/final_full_min_bytes` | `0` |
| `/memory/working_set/final_central_min_bytes` | `0` |
| `/memory/working_set/final_median_bytes` | `0` |
| `/memory/working_set/final_central_max_bytes` | `0` |
| `/memory/working_set/final_full_max_bytes` | `0` |
| `/memory/working_set/peak_bytes` | `0` |
| `/memory/working_set/growth_ratio` | `0` |
| `/memory/working_set/instant_growth_ratio` | `0` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/memory/working_set/warmup_full_span_ratio` | `0` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/memory/working_set/measured_full_span_ratio` | `0` |
| `/memory/working_set/warmup_samples` | `[]` |
| `/memory/working_set/measured_samples` | `[]` |
| `/box3d_allocator/baseline_bytes` | `0` |
| `/box3d_allocator/final_bytes` | `0` |
| `/box3d_allocator/max_abs_delta` | `0` |
| `/box3d_allocator/exact_return` | `true` |
| `/box3d_allocator/warmup_post_teardown` | `[]` |
| `/box3d_allocator/measured_post_teardown` | `[]` |
| `/crt/applicable` | `false` |
| `/crt/balanced` | `true` |
| `/crt/normal_block_count_delta` | `0` |
| `/crt/normal_block_bytes_delta` | `0` |
| `/crt/client_block_count_delta` | `0` |
| `/crt/client_block_bytes_delta` | `0` |
| `/warmup_ticks` | `0` |
| `/measurement_ticks` | `0` |
| `/allocator_warmup_cycles` | `0` |
| `/stress_cycles` | `0` |
| `/fallback` | `""` |
| `/limits/0/name` | `"sleep_ratio"` |
| `/limits/0/comparison` | `">="` |
| `/limits/0/value` | `0.9` |
| `/limits/0/unit` | `"ratio"` |
| `/limits/1/name` | `"p95_linear_speed"` |
| `/limits/1/comparison` | `"<"` |
| `/limits/1/value` | `0.05` |
| `/limits/1/unit` | `"m/s"` |
| `/limits/2/name` | `"p95_angular_speed"` |
| `/limits/2/comparison` | `"<"` |
| `/limits/2/value` | `0.1` |
| `/limits/2/unit` | `"rad/s"` |
| `/limits/3/name` | `"max_penetration"` |
| `/limits/3/comparison` | `"<"` |
| `/limits/3/value` | `0.02` |
| `/limits/3/unit` | `"m"` |
| `/limits/4/name` | `"spontaneous_speed"` |
| `/limits/4/comparison` | `"<="` |
| `/limits/4/value` | `100` |
| `/limits/4/unit` | `"m/s"` |
| `/limits/5/name` | `"world_radius"` |
| `/limits/5/comparison` | `"<="` |
| `/limits/5/value` | `60` |
| `/limits/5/unit` | `"m"` |
| `/limits/6/name` | `"energy_growth_from_tick_600"` |
| `/limits/6/comparison` | `"<="` |
| `/limits/6/value` | `0.02` |
| `/limits/6/unit` | `"ratio"` |
| `/metrics/0/name` | `"sleep_ratio"` |
| `/metrics/0/value` | `1` |
| `/metrics/0/unit` | `"ratio"` |
| `/metrics/1/name` | `"p95_linear_speed"` |
| `/metrics/1/value` | `0` |
| `/metrics/1/unit` | `"m/s"` |
| `/metrics/2/name` | `"p95_angular_speed"` |
| `/metrics/2/value` | `0` |
| `/metrics/2/unit` | `"rad/s"` |
| `/metrics/3/name` | `"max_penetration"` |
| `/metrics/3/value` | `0.0015351474285125732` |
| `/metrics/3/unit` | `"m"` |
| `/metrics/4/name` | `"max_platform_penetration"` |
| `/metrics/4/value` | `0.00030809640884399414` |
| `/metrics/4/unit` | `"m"` |
| `/metrics/5/name` | `"max_pair_penetration"` |
| `/metrics/5/value` | `0.0015351474285125732` |
| `/metrics/5/unit` | `"m"` |
| `/metrics/6/name` | `"minimum_density"` |
| `/metrics/6/value` | `480` |
| `/metrics/6/unit` | `"kg/m3"` |
| `/metrics/7/name` | `"maximum_density"` |
| `/metrics/7/value` | `520` |
| `/metrics/7/unit` | `"kg/m3"` |
| `/metrics/8/name` | `"energy_at_tick_600"` |
| `/metrics/8/value` | `0` |
| `/metrics/8/unit` | `"J"` |
| `/metrics/9/name` | `"max_energy_growth_ratio"` |
| `/metrics/9/value` | `0` |
| `/metrics/9/unit` | `"ratio"` |
| `/matrix` | `[]` |
| `/warnings` | `[]` |
| `/violations` | `[]` |

### Release scenario `mass_ratio`

Topology fixture `dynamic/shape/joint`: `80/81/0`. Observed peak `body/shape/joint;awake/contact`: `81/81/0;80/227`.

| JSON pointer | Value |
| --- | --- |
| `/name` | `"mass_ratio"` |
| `/seed` | `1` |
| `/substeps` | `4` |
| `/ticks` | `1800` |
| `/final_hash` | `17464060736204574665` |
| `/hashes/0` | `17464060736204574665` |
| `/hashes/1` | `17464060736204574665` |
| `/repeat_observations/0/repeat` | `1` |
| `/repeat_observations/0/hash` | `17464060736204574665` |
| `/repeat_observations/0/peak_body_count` | `81` |
| `/repeat_observations/0/peak_shape_count` | `81` |
| `/repeat_observations/0/peak_joint_count` | `0` |
| `/repeat_observations/0/peak_awake_count` | `80` |
| `/repeat_observations/0/peak_contact_count` | `227` |
| `/repeat_observations/0/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/0/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `false` |
| `/repeat_observations/0/memory/private_commit/stable` | `false` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/available` | `false` |
| `/repeat_observations/0/memory/working_set/stable` | `false` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/0/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/0/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/0/crt/applicable` | `false` |
| `/repeat_observations/0/crt/balanced` | `true` |
| `/repeat_observations/0/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/0/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/0/crt/client_block_count_delta` | `0` |
| `/repeat_observations/0/crt/client_block_bytes_delta` | `0` |
| `/repeat_observations/1/repeat` | `2` |
| `/repeat_observations/1/hash` | `17464060736204574665` |
| `/repeat_observations/1/peak_body_count` | `81` |
| `/repeat_observations/1/peak_shape_count` | `81` |
| `/repeat_observations/1/peak_joint_count` | `0` |
| `/repeat_observations/1/peak_awake_count` | `80` |
| `/repeat_observations/1/peak_contact_count` | `227` |
| `/repeat_observations/1/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/1/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/gate_applied` | `false` |
| `/repeat_observations/1/memory/budget_qualified` | `false` |
| `/repeat_observations/1/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/private_commit/available` | `false` |
| `/repeat_observations/1/memory/private_commit/stable` | `false` |
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/available` | `false` |
| `/repeat_observations/1/memory/working_set/stable` | `false` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/1/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/1/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/1/crt/applicable` | `false` |
| `/repeat_observations/1/crt/balanced` | `true` |
| `/repeat_observations/1/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/1/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/1/crt/client_block_count_delta` | `0` |
| `/repeat_observations/1/crt/client_block_bytes_delta` | `0` |
| `/contact_before_pile_exit` | `false` |
| `/ccd_primary_pass_count` | `0` |
| `/ccd_fallback_pass_count` | `0` |
| `/projectile_speed` | `0` |
| `/surface_separation` | `0` |
| `/final_linear_speed` | `0` |
| `/final_angular_speed` | `0` |
| `/p95_linear_speed` | `0` |
| `/p95_angular_speed` | `0` |
| `/sleep_ratio` | `1` |
| `/max_penetration` | `0.014884665608406067` |
| `/minimum_density` | `85` |
| `/maximum_density` | `3400` |
| `/energy_at_tick_600` | `0` |
| `/max_energy_growth_ratio` | `0` |
| `/dynamic_body_count` | `80` |
| `/shape_count` | `81` |
| `/joint_count` | `0` |
| `/peak_body_count` | `81` |
| `/peak_shape_count` | `81` |
| `/peak_joint_count` | `0` |
| `/peak_awake_count` | `80` |
| `/peak_contact_count` | `227` |
| `/step_ms/min` | `0.0029` |
| `/step_ms/p50` | `0.0034` |
| `/step_ms/p95` | `0.0107` |
| `/step_ms/max` | `1.4911` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unavailable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `false` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `false` |
| `/memory/private_commit/baseline_last_bytes` | `0` |
| `/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/memory/private_commit/baseline_median_bytes` | `0` |
| `/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/memory/private_commit/final_last_bytes` | `0` |
| `/memory/private_commit/final_full_min_bytes` | `0` |
| `/memory/private_commit/final_central_min_bytes` | `0` |
| `/memory/private_commit/final_median_bytes` | `0` |
| `/memory/private_commit/final_central_max_bytes` | `0` |
| `/memory/private_commit/final_full_max_bytes` | `0` |
| `/memory/private_commit/peak_bytes` | `0` |
| `/memory/private_commit/growth_ratio` | `0` |
| `/memory/private_commit/instant_growth_ratio` | `0` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/memory/private_commit/measured_full_span_ratio` | `0` |
| `/memory/private_commit/warmup_samples` | `[]` |
| `/memory/private_commit/measured_samples` | `[]` |
| `/memory/working_set/available` | `false` |
| `/memory/working_set/stable` | `false` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"unavailable"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `0` |
| `/memory/working_set/baseline_full_min_bytes` | `0` |
| `/memory/working_set/baseline_central_min_bytes` | `0` |
| `/memory/working_set/baseline_median_bytes` | `0` |
| `/memory/working_set/baseline_central_max_bytes` | `0` |
| `/memory/working_set/baseline_full_max_bytes` | `0` |
| `/memory/working_set/final_last_bytes` | `0` |
| `/memory/working_set/final_full_min_bytes` | `0` |
| `/memory/working_set/final_central_min_bytes` | `0` |
| `/memory/working_set/final_median_bytes` | `0` |
| `/memory/working_set/final_central_max_bytes` | `0` |
| `/memory/working_set/final_full_max_bytes` | `0` |
| `/memory/working_set/peak_bytes` | `0` |
| `/memory/working_set/growth_ratio` | `0` |
| `/memory/working_set/instant_growth_ratio` | `0` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/memory/working_set/warmup_full_span_ratio` | `0` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/memory/working_set/measured_full_span_ratio` | `0` |
| `/memory/working_set/warmup_samples` | `[]` |
| `/memory/working_set/measured_samples` | `[]` |
| `/box3d_allocator/baseline_bytes` | `0` |
| `/box3d_allocator/final_bytes` | `0` |
| `/box3d_allocator/max_abs_delta` | `0` |
| `/box3d_allocator/exact_return` | `true` |
| `/box3d_allocator/warmup_post_teardown` | `[]` |
| `/box3d_allocator/measured_post_teardown` | `[]` |
| `/crt/applicable` | `false` |
| `/crt/balanced` | `true` |
| `/crt/normal_block_count_delta` | `0` |
| `/crt/normal_block_bytes_delta` | `0` |
| `/crt/client_block_count_delta` | `0` |
| `/crt/client_block_bytes_delta` | `0` |
| `/warmup_ticks` | `0` |
| `/measurement_ticks` | `0` |
| `/allocator_warmup_cycles` | `0` |
| `/stress_cycles` | `0` |
| `/fallback` | `""` |
| `/limits/0/name` | `"sleep_ratio"` |
| `/limits/0/comparison` | `">="` |
| `/limits/0/value` | `0.8` |
| `/limits/0/unit` | `"ratio"` |
| `/limits/1/name` | `"p95_linear_speed"` |
| `/limits/1/comparison` | `"<"` |
| `/limits/1/value` | `0.1` |
| `/limits/1/unit` | `"m/s"` |
| `/limits/2/name` | `"p95_angular_speed"` |
| `/limits/2/comparison` | `"<"` |
| `/limits/2/value` | `0.2` |
| `/limits/2/unit` | `"rad/s"` |
| `/limits/3/name` | `"max_penetration"` |
| `/limits/3/comparison` | `"<"` |
| `/limits/3/value` | `0.025` |
| `/limits/3/unit` | `"m"` |
| `/limits/4/name` | `"spontaneous_speed"` |
| `/limits/4/comparison` | `"<="` |
| `/limits/4/value` | `100` |
| `/limits/4/unit` | `"m/s"` |
| `/limits/5/name` | `"world_radius"` |
| `/limits/5/comparison` | `"<="` |
| `/limits/5/value` | `60` |
| `/limits/5/unit` | `"m"` |
| `/metrics/0/name` | `"sleep_ratio"` |
| `/metrics/0/value` | `1` |
| `/metrics/0/unit` | `"ratio"` |
| `/metrics/1/name` | `"p95_linear_speed"` |
| `/metrics/1/value` | `0` |
| `/metrics/1/unit` | `"m/s"` |
| `/metrics/2/name` | `"p95_angular_speed"` |
| `/metrics/2/value` | `0` |
| `/metrics/2/unit` | `"rad/s"` |
| `/metrics/3/name` | `"max_penetration"` |
| `/metrics/3/value` | `0.014884665608406067` |
| `/metrics/3/unit` | `"m"` |
| `/metrics/4/name` | `"max_platform_penetration"` |
| `/metrics/4/value` | `0.004117727279663086` |
| `/metrics/4/unit` | `"m"` |
| `/metrics/5/name` | `"max_pair_penetration"` |
| `/metrics/5/value` | `0.014884665608406067` |
| `/metrics/5/unit` | `"m"` |
| `/metrics/6/name` | `"minimum_density"` |
| `/metrics/6/value` | `85` |
| `/metrics/6/unit` | `"kg/m3"` |
| `/metrics/7/name` | `"maximum_density"` |
| `/metrics/7/value` | `3400` |
| `/metrics/7/unit` | `"kg/m3"` |
| `/metrics/8/name` | `"energy_at_tick_600"` |
| `/metrics/8/value` | `0` |
| `/metrics/8/unit` | `"J"` |
| `/metrics/9/name` | `"max_energy_growth_ratio"` |
| `/metrics/9/value` | `0` |
| `/metrics/9/unit` | `"ratio"` |
| `/matrix` | `[]` |
| `/warnings` | `[]` |
| `/violations` | `[]` |

### Release scenario `stress`

Topology fixture `dynamic/shape/joint`: `500/800/250`. Observed peak `body/shape/joint;awake/contact`: `500/800/250;500/0`.

| JSON pointer | Value |
| --- | --- |
| `/name` | `"stress"` |
| `/seed` | `1` |
| `/substeps` | `4` |
| `/ticks` | `15000` |
| `/final_hash` | `10292935394449293550` |
| `/hashes/0` | `10292935394449293550` |
| `/hashes/1` | `10292935394449293550` |
| `/repeat_observations/0/repeat` | `1` |
| `/repeat_observations/0/hash` | `10292935394449293550` |
| `/repeat_observations/0/peak_body_count` | `500` |
| `/repeat_observations/0/peak_shape_count` | `800` |
| `/repeat_observations/0/peak_joint_count` | `250` |
| `/repeat_observations/0/peak_awake_count` | `500` |
| `/repeat_observations/0/peak_contact_count` | `0` |
| `/repeat_observations/0/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/0/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/assessment_status` | `"pass"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `true` |
| `/repeat_observations/0/memory/private_commit/stable` | `true` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `2019328` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `2019328` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `2146304` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `2220032` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `2224128` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `2347008` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `1978368` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `1978368` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `1978368` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `1990656` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `1994752` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `2162688` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `2363392` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0.03505535055350553` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0.14760147601476015` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0.00823045267489712` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0.09259259259259259` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/0` | `1646592` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/1` | `1708032` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/2` | `2121728` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/3` | `1953792` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/4` | `1957888` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/5` | `2146304` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/6` | `2224128` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/7` | `2220032` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/8` | `2347008` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/9` | `2019328` |
| `/repeat_observations/0/memory/private_commit/measured_samples/0` | `2314240` |
| `/repeat_observations/0/memory/private_commit/measured_samples/1` | `2068480` |
| `/repeat_observations/0/memory/private_commit/measured_samples/2` | `2363392` |
| `/repeat_observations/0/memory/private_commit/measured_samples/3` | `2035712` |
| `/repeat_observations/0/memory/private_commit/measured_samples/4` | `1994752` |
| `/repeat_observations/0/memory/private_commit/measured_samples/5` | `1978368` |
| `/repeat_observations/0/memory/private_commit/measured_samples/6` | `1990656` |
| `/repeat_observations/0/memory/private_commit/measured_samples/7` | `2162688` |
| `/repeat_observations/0/memory/private_commit/measured_samples/8` | `1994752` |
| `/repeat_observations/0/memory/private_commit/measured_samples/9` | `1978368` |
| `/repeat_observations/0/memory/working_set/available` | `true` |
| `/repeat_observations/0/memory/working_set/stable` | `true` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"pass"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `5185536` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `5185536` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `5193728` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `5197824` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `5206016` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `5214208` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `5189632` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `5189632` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `5189632` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `5201920` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `5206016` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `5222400` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `6701056` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0.0007880220646178094` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0.0007898894154818325` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0.002364066193853428` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0.005516154452324665` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0.0031496062992125984` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0.006299212598425197` |
| `/repeat_observations/0/memory/working_set/warmup_samples/0` | `5009408` |
| `/repeat_observations/0/memory/working_set/warmup_samples/1` | `5070848` |
| `/repeat_observations/0/memory/working_set/warmup_samples/2` | `5181440` |
| `/repeat_observations/0/memory/working_set/warmup_samples/3` | `5124096` |
| `/repeat_observations/0/memory/working_set/warmup_samples/4` | `5140480` |
| `/repeat_observations/0/memory/working_set/warmup_samples/5` | `5193728` |
| `/repeat_observations/0/memory/working_set/warmup_samples/6` | `5206016` |
| `/repeat_observations/0/memory/working_set/warmup_samples/7` | `5197824` |
| `/repeat_observations/0/memory/working_set/warmup_samples/8` | `5214208` |
| `/repeat_observations/0/memory/working_set/warmup_samples/9` | `5185536` |
| `/repeat_observations/0/memory/working_set/measured_samples/0` | `5255168` |
| `/repeat_observations/0/memory/working_set/measured_samples/1` | `5238784` |
| `/repeat_observations/0/memory/working_set/measured_samples/2` | `5255168` |
| `/repeat_observations/0/memory/working_set/measured_samples/3` | `5234688` |
| `/repeat_observations/0/memory/working_set/measured_samples/4` | `5206016` |
| `/repeat_observations/0/memory/working_set/measured_samples/5` | `5189632` |
| `/repeat_observations/0/memory/working_set/measured_samples/6` | `5201920` |
| `/repeat_observations/0/memory/working_set/measured_samples/7` | `5222400` |
| `/repeat_observations/0/memory/working_set/measured_samples/8` | `5206016` |
| `/repeat_observations/0/memory/working_set/measured_samples/9` | `5189632` |
| `/repeat_observations/0/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/0/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/0` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/1` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/2` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/3` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/4` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/5` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/6` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/7` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/8` | `0` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown/9` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/0` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/1` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/2` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/3` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/4` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/5` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/6` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/7` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/8` | `0` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown/9` | `0` |
| `/repeat_observations/0/crt/applicable` | `false` |
| `/repeat_observations/0/crt/balanced` | `true` |
| `/repeat_observations/0/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/0/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/0/crt/client_block_count_delta` | `0` |
| `/repeat_observations/0/crt/client_block_bytes_delta` | `0` |
| `/repeat_observations/1/repeat` | `2` |
| `/repeat_observations/1/hash` | `10292935394449293550` |
| `/repeat_observations/1/peak_body_count` | `500` |
| `/repeat_observations/1/peak_shape_count` | `800` |
| `/repeat_observations/1/peak_joint_count` | `250` |
| `/repeat_observations/1/peak_awake_count` | `500` |
| `/repeat_observations/1/peak_contact_count` | `0` |
| `/repeat_observations/1/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/1/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/assessment_status` | `"unstable"` |
| `/repeat_observations/1/memory/gate_applied` | `false` |
| `/repeat_observations/1/memory/budget_qualified` | `false` |
| `/repeat_observations/1/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/private_commit/available` | `true` |
| `/repeat_observations/1/memory/private_commit/stable` | `false` |
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `3264512` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `1974272` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `1998848` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `3264512` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `3432448` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `3547136` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `3129344` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `3129344` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `3551232` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `3559424` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `3588096` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `3592192` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `3665920` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0.0903387703889586` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0.43914680050188204` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0.48180677540777916` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0.010356731875719217` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0.13003452243958574` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/0` | `1982464` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/1` | `2121728` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/2` | `1982464` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/3` | `1974272` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/4` | `3358720` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/5` | `1974272` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/6` | `3432448` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/7` | `3547136` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/8` | `1998848` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/9` | `3264512` |
| `/repeat_observations/1/memory/private_commit/measured_samples/0` | `3665920` |
| `/repeat_observations/1/memory/private_commit/measured_samples/1` | `3633152` |
| `/repeat_observations/1/memory/private_commit/measured_samples/2` | `3567616` |
| `/repeat_observations/1/memory/private_commit/measured_samples/3` | `3563520` |
| `/repeat_observations/1/memory/private_commit/measured_samples/4` | `3256320` |
| `/repeat_observations/1/memory/private_commit/measured_samples/5` | `3559424` |
| `/repeat_observations/1/memory/private_commit/measured_samples/6` | `3592192` |
| `/repeat_observations/1/memory/private_commit/measured_samples/7` | `3551232` |
| `/repeat_observations/1/memory/private_commit/measured_samples/8` | `3588096` |
| `/repeat_observations/1/memory/private_commit/measured_samples/9` | `3129344` |
| `/repeat_observations/1/memory/working_set/available` | `true` |
| `/repeat_observations/1/memory/working_set/stable` | `true` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"pass"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `5406720` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `5185536` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `5201920` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `5386240` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `5406720` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `5410816` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `5414912` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `5414912` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `5419008` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `5423104` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `5431296` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `5435392` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `6701056` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0.006844106463878327` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0.0015151515151515152` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0.03802281368821293` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0.04182509505703422` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0.0022658610271903325` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0.0037764350453172208` |
| `/repeat_observations/1/memory/working_set/warmup_samples/0` | `5193728` |
| `/repeat_observations/1/memory/working_set/warmup_samples/1` | `5214208` |
| `/repeat_observations/1/memory/working_set/warmup_samples/2` | `5193728` |
| `/repeat_observations/1/memory/working_set/warmup_samples/3` | `5185536` |
| `/repeat_observations/1/memory/working_set/warmup_samples/4` | `5414912` |
| `/repeat_observations/1/memory/working_set/warmup_samples/5` | `5185536` |
| `/repeat_observations/1/memory/working_set/warmup_samples/6` | `5410816` |
| `/repeat_observations/1/memory/working_set/warmup_samples/7` | `5386240` |
| `/repeat_observations/1/memory/working_set/warmup_samples/8` | `5201920` |
| `/repeat_observations/1/memory/working_set/warmup_samples/9` | `5406720` |
| `/repeat_observations/1/memory/working_set/measured_samples/0` | `5410816` |
| `/repeat_observations/1/memory/working_set/measured_samples/1` | `5414912` |
| `/repeat_observations/1/memory/working_set/measured_samples/2` | `5427200` |
| `/repeat_observations/1/memory/working_set/measured_samples/3` | `5410816` |
| `/repeat_observations/1/memory/working_set/measured_samples/4` | `5414912` |
| `/repeat_observations/1/memory/working_set/measured_samples/5` | `5419008` |
| `/repeat_observations/1/memory/working_set/measured_samples/6` | `5431296` |
| `/repeat_observations/1/memory/working_set/measured_samples/7` | `5423104` |
| `/repeat_observations/1/memory/working_set/measured_samples/8` | `5435392` |
| `/repeat_observations/1/memory/working_set/measured_samples/9` | `5414912` |
| `/repeat_observations/1/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/1/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/0` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/1` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/2` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/3` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/4` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/5` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/6` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/7` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/8` | `0` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown/9` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/0` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/1` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/2` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/3` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/4` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/5` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/6` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/7` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/8` | `0` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown/9` | `0` |
| `/repeat_observations/1/crt/applicable` | `false` |
| `/repeat_observations/1/crt/balanced` | `true` |
| `/repeat_observations/1/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/1/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/1/crt/client_block_count_delta` | `0` |
| `/repeat_observations/1/crt/client_block_bytes_delta` | `0` |
| `/contact_before_pile_exit` | `false` |
| `/ccd_primary_pass_count` | `0` |
| `/ccd_fallback_pass_count` | `0` |
| `/projectile_speed` | `0` |
| `/surface_separation` | `0` |
| `/final_linear_speed` | `0` |
| `/final_angular_speed` | `0` |
| `/p95_linear_speed` | `0` |
| `/p95_angular_speed` | `0` |
| `/sleep_ratio` | `0` |
| `/max_penetration` | `0` |
| `/minimum_density` | `0` |
| `/maximum_density` | `0` |
| `/energy_at_tick_600` | `0` |
| `/max_energy_growth_ratio` | `0` |
| `/dynamic_body_count` | `500` |
| `/shape_count` | `800` |
| `/joint_count` | `250` |
| `/peak_body_count` | `500` |
| `/peak_shape_count` | `800` |
| `/peak_joint_count` | `250` |
| `/peak_awake_count` | `500` |
| `/peak_contact_count` | `0` |
| `/step_ms/min` | `0.048` |
| `/step_ms/p50` | `0.0704` |
| `/step_ms/p95` | `0.1023` |
| `/step_ms/max` | `2.1994` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"pass"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `true` |
| `/memory/private_commit/stable` | `true` |
| `/memory/private_commit/terminal_growth` | `false` |
| `/memory/private_commit/baseline_last_bytes` | `2019328` |
| `/memory/private_commit/baseline_full_min_bytes` | `2019328` |
| `/memory/private_commit/baseline_central_min_bytes` | `2146304` |
| `/memory/private_commit/baseline_median_bytes` | `2220032` |
| `/memory/private_commit/baseline_central_max_bytes` | `2224128` |
| `/memory/private_commit/baseline_full_max_bytes` | `2347008` |
| `/memory/private_commit/final_last_bytes` | `1978368` |
| `/memory/private_commit/final_full_min_bytes` | `1978368` |
| `/memory/private_commit/final_central_min_bytes` | `1978368` |
| `/memory/private_commit/final_median_bytes` | `1990656` |
| `/memory/private_commit/final_central_max_bytes` | `1994752` |
| `/memory/private_commit/final_full_max_bytes` | `2162688` |
| `/memory/private_commit/peak_bytes` | `2363392` |
| `/memory/private_commit/growth_ratio` | `0` |
| `/memory/private_commit/instant_growth_ratio` | `0` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0.03505535055350553` |
| `/memory/private_commit/warmup_full_span_ratio` | `0.14760147601476015` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0.00823045267489712` |
| `/memory/private_commit/measured_full_span_ratio` | `0.09259259259259259` |
| `/memory/private_commit/warmup_samples/0` | `1646592` |
| `/memory/private_commit/warmup_samples/1` | `1708032` |
| `/memory/private_commit/warmup_samples/2` | `2121728` |
| `/memory/private_commit/warmup_samples/3` | `1953792` |
| `/memory/private_commit/warmup_samples/4` | `1957888` |
| `/memory/private_commit/warmup_samples/5` | `2146304` |
| `/memory/private_commit/warmup_samples/6` | `2224128` |
| `/memory/private_commit/warmup_samples/7` | `2220032` |
| `/memory/private_commit/warmup_samples/8` | `2347008` |
| `/memory/private_commit/warmup_samples/9` | `2019328` |
| `/memory/private_commit/measured_samples/0` | `2314240` |
| `/memory/private_commit/measured_samples/1` | `2068480` |
| `/memory/private_commit/measured_samples/2` | `2363392` |
| `/memory/private_commit/measured_samples/3` | `2035712` |
| `/memory/private_commit/measured_samples/4` | `1994752` |
| `/memory/private_commit/measured_samples/5` | `1978368` |
| `/memory/private_commit/measured_samples/6` | `1990656` |
| `/memory/private_commit/measured_samples/7` | `2162688` |
| `/memory/private_commit/measured_samples/8` | `1994752` |
| `/memory/private_commit/measured_samples/9` | `1978368` |
| `/memory/working_set/available` | `true` |
| `/memory/working_set/stable` | `true` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"pass"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `5185536` |
| `/memory/working_set/baseline_full_min_bytes` | `5185536` |
| `/memory/working_set/baseline_central_min_bytes` | `5193728` |
| `/memory/working_set/baseline_median_bytes` | `5197824` |
| `/memory/working_set/baseline_central_max_bytes` | `5206016` |
| `/memory/working_set/baseline_full_max_bytes` | `5214208` |
| `/memory/working_set/final_last_bytes` | `5189632` |
| `/memory/working_set/final_full_min_bytes` | `5189632` |
| `/memory/working_set/final_central_min_bytes` | `5189632` |
| `/memory/working_set/final_median_bytes` | `5201920` |
| `/memory/working_set/final_central_max_bytes` | `5206016` |
| `/memory/working_set/final_full_max_bytes` | `5222400` |
| `/memory/working_set/peak_bytes` | `6701056` |
| `/memory/working_set/growth_ratio` | `0.0007880220646178094` |
| `/memory/working_set/instant_growth_ratio` | `0.0007898894154818325` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0.002364066193853428` |
| `/memory/working_set/warmup_full_span_ratio` | `0.005516154452324665` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0.0031496062992125984` |
| `/memory/working_set/measured_full_span_ratio` | `0.006299212598425197` |
| `/memory/working_set/warmup_samples/0` | `5009408` |
| `/memory/working_set/warmup_samples/1` | `5070848` |
| `/memory/working_set/warmup_samples/2` | `5181440` |
| `/memory/working_set/warmup_samples/3` | `5124096` |
| `/memory/working_set/warmup_samples/4` | `5140480` |
| `/memory/working_set/warmup_samples/5` | `5193728` |
| `/memory/working_set/warmup_samples/6` | `5206016` |
| `/memory/working_set/warmup_samples/7` | `5197824` |
| `/memory/working_set/warmup_samples/8` | `5214208` |
| `/memory/working_set/warmup_samples/9` | `5185536` |
| `/memory/working_set/measured_samples/0` | `5255168` |
| `/memory/working_set/measured_samples/1` | `5238784` |
| `/memory/working_set/measured_samples/2` | `5255168` |
| `/memory/working_set/measured_samples/3` | `5234688` |
| `/memory/working_set/measured_samples/4` | `5206016` |
| `/memory/working_set/measured_samples/5` | `5189632` |
| `/memory/working_set/measured_samples/6` | `5201920` |
| `/memory/working_set/measured_samples/7` | `5222400` |
| `/memory/working_set/measured_samples/8` | `5206016` |
| `/memory/working_set/measured_samples/9` | `5189632` |
| `/box3d_allocator/baseline_bytes` | `0` |
| `/box3d_allocator/final_bytes` | `0` |
| `/box3d_allocator/max_abs_delta` | `0` |
| `/box3d_allocator/exact_return` | `true` |
| `/box3d_allocator/warmup_post_teardown/0` | `0` |
| `/box3d_allocator/warmup_post_teardown/1` | `0` |
| `/box3d_allocator/warmup_post_teardown/2` | `0` |
| `/box3d_allocator/warmup_post_teardown/3` | `0` |
| `/box3d_allocator/warmup_post_teardown/4` | `0` |
| `/box3d_allocator/warmup_post_teardown/5` | `0` |
| `/box3d_allocator/warmup_post_teardown/6` | `0` |
| `/box3d_allocator/warmup_post_teardown/7` | `0` |
| `/box3d_allocator/warmup_post_teardown/8` | `0` |
| `/box3d_allocator/warmup_post_teardown/9` | `0` |
| `/box3d_allocator/measured_post_teardown/0` | `0` |
| `/box3d_allocator/measured_post_teardown/1` | `0` |
| `/box3d_allocator/measured_post_teardown/2` | `0` |
| `/box3d_allocator/measured_post_teardown/3` | `0` |
| `/box3d_allocator/measured_post_teardown/4` | `0` |
| `/box3d_allocator/measured_post_teardown/5` | `0` |
| `/box3d_allocator/measured_post_teardown/6` | `0` |
| `/box3d_allocator/measured_post_teardown/7` | `0` |
| `/box3d_allocator/measured_post_teardown/8` | `0` |
| `/box3d_allocator/measured_post_teardown/9` | `0` |
| `/crt/applicable` | `false` |
| `/crt/balanced` | `true` |
| `/crt/normal_block_count_delta` | `0` |
| `/crt/normal_block_bytes_delta` | `0` |
| `/crt/client_block_count_delta` | `0` |
| `/crt/client_block_bytes_delta` | `0` |
| `/warmup_ticks` | `300` |
| `/measurement_ticks` | `1200` |
| `/allocator_warmup_cycles` | `10` |
| `/stress_cycles` | `10` |
| `/fallback` | `""` |
| `/limits/0/name` | `"allocator_warmup_cycles"` |
| `/limits/0/comparison` | `"=="` |
| `/limits/0/value` | `10` |
| `/limits/0/unit` | `"cycles"` |
| `/limits/1/name` | `"scenario_timeout"` |
| `/limits/1/comparison` | `"<="` |
| `/limits/1/value` | `60` |
| `/limits/1/unit` | `"s"` |
| `/limits/2/name` | `"spontaneous_speed"` |
| `/limits/2/comparison` | `"<="` |
| `/limits/2/value` | `100` |
| `/limits/2/unit` | `"m/s"` |
| `/metrics/0/name` | `"private_commit_diagnostic_growth_target"` |
| `/metrics/0/value` | `0.05` |
| `/metrics/0/unit` | `"ratio"` |
| `/metrics/1/name` | `"private_commit_diagnostic_trimmed_span_target"` |
| `/metrics/1/value` | `0.05` |
| `/metrics/1/unit` | `"ratio"` |
| `/metrics/2/name` | `"working_set_diagnostic_growth_target"` |
| `/metrics/2/value` | `0.05` |
| `/metrics/2/unit` | `"ratio"` |
| `/metrics/3/name` | `"working_set_diagnostic_trimmed_span_target"` |
| `/metrics/3/value` | `0.05` |
| `/metrics/3/unit` | `"ratio"` |
| `/metrics/4/name` | `"private_commit_baseline_median_bytes"` |
| `/metrics/4/value` | `2220032` |
| `/metrics/4/unit` | `"bytes"` |
| `/metrics/5/name` | `"private_commit_final_median_bytes"` |
| `/metrics/5/value` | `1990656` |
| `/metrics/5/unit` | `"bytes"` |
| `/metrics/6/name` | `"private_commit_growth_ratio"` |
| `/metrics/6/value` | `0` |
| `/metrics/6/unit` | `"ratio"` |
| `/metrics/7/name` | `"private_commit_instant_growth_ratio"` |
| `/metrics/7/value` | `0` |
| `/metrics/7/unit` | `"ratio"` |
| `/metrics/8/name` | `"private_commit_warmup_trimmed_span_ratio"` |
| `/metrics/8/value` | `0.03505535055350553` |
| `/metrics/8/unit` | `"ratio"` |
| `/metrics/9/name` | `"private_commit_measured_trimmed_span_ratio"` |
| `/metrics/9/value` | `0.00823045267489712` |
| `/metrics/9/unit` | `"ratio"` |
| `/metrics/10/name` | `"working_set_baseline_bytes"` |
| `/metrics/10/value` | `5185536` |
| `/metrics/10/unit` | `"bytes"` |
| `/metrics/11/name` | `"working_set_baseline_low_bytes"` |
| `/metrics/11/value` | `5185536` |
| `/metrics/11/unit` | `"bytes"` |
| `/metrics/12/name` | `"working_set_peak_bytes"` |
| `/metrics/12/value` | `6701056` |
| `/metrics/12/unit` | `"bytes"` |
| `/metrics/13/name` | `"working_set_final_bytes"` |
| `/metrics/13/value` | `5189632` |
| `/metrics/13/unit` | `"bytes"` |
| `/metrics/14/name` | `"working_set_final_low_bytes"` |
| `/metrics/14/value` | `5189632` |
| `/metrics/14/unit` | `"bytes"` |
| `/metrics/15/name` | `"working_set_growth_ratio"` |
| `/metrics/15/value` | `0.0007880220646178094` |
| `/metrics/15/unit` | `"ratio"` |
| `/metrics/16/name` | `"working_set_instant_growth_ratio"` |
| `/metrics/16/value` | `0.0007898894154818325` |
| `/metrics/16/unit` | `"ratio"` |
| `/metrics/17/name` | `"executed_substeps"` |
| `/metrics/17/value` | `4` |
| `/metrics/17/unit` | `"count"` |
| `/metrics/18/name` | `"allocator_warmup_cycles"` |
| `/metrics/18/value` | `10` |
| `/metrics/18/unit` | `"cycles"` |
| `/metrics/19/name` | `"completed_cycles"` |
| `/metrics/19/value` | `10` |
| `/metrics/19/unit` | `"count"` |
| `/matrix` | `[]` |
| `/warnings/0/code` | `"private_commit_budget_unqualified"` |
| `/warnings/0/message` | `"PrivateUsage is diagnostic in the foundation; qualify the 5% budget in a packaged Release build on reference hardware"` |
| `/warnings/0/details/0/name` | `"assessment"` |
| `/warnings/0/details/0/value` | `"pass"` |
| `/warnings/0/details/1/name` | `"budget_scope"` |
| `/warnings/0/details/1/value` | `"future_packaged_reference_hardware"` |
| `/violations` | `[]` |

### Release scenario `capability_matrix`

Topology fixture `dynamic/shape/joint`: `0/0/0`. Observed peak `body/shape/joint;awake/contact`: `123/123/1;121/221`.

| JSON pointer | Value |
| --- | --- |
| `/name` | `"capability_matrix"` |
| `/seed` | `1` |
| `/substeps` | `4` |
| `/ticks` | `21800` |
| `/final_hash` | `17104053157009575930` |
| `/hashes/0` | `17104053157009575930` |
| `/hashes/1` | `17104053157009575930` |
| `/repeat_observations/0/repeat` | `1` |
| `/repeat_observations/0/hash` | `17104053157009575930` |
| `/repeat_observations/0/peak_body_count` | `123` |
| `/repeat_observations/0/peak_shape_count` | `123` |
| `/repeat_observations/0/peak_joint_count` | `1` |
| `/repeat_observations/0/peak_awake_count` | `121` |
| `/repeat_observations/0/peak_contact_count` | `221` |
| `/repeat_observations/0/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/0/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `false` |
| `/repeat_observations/0/memory/private_commit/stable` | `false` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/available` | `false` |
| `/repeat_observations/0/memory/working_set/stable` | `false` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/0/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/0/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/0/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/0/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/0/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/0/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/0/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/0/crt/applicable` | `false` |
| `/repeat_observations/0/crt/balanced` | `true` |
| `/repeat_observations/0/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/0/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/0/crt/client_block_count_delta` | `0` |
| `/repeat_observations/0/crt/client_block_bytes_delta` | `0` |
| `/repeat_observations/1/repeat` | `2` |
| `/repeat_observations/1/hash` | `17104053157009575930` |
| `/repeat_observations/1/peak_body_count` | `123` |
| `/repeat_observations/1/peak_shape_count` | `123` |
| `/repeat_observations/1/peak_joint_count` | `1` |
| `/repeat_observations/1/peak_awake_count` | `121` |
| `/repeat_observations/1/peak_contact_count` | `221` |
| `/repeat_observations/1/memory/gate_scope` | `"release_mt"` |
| `/repeat_observations/1/memory/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/gate_applied` | `false` |
| `/repeat_observations/1/memory/budget_qualified` | `false` |
| `/repeat_observations/1/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/private_commit/available` | `false` |
| `/repeat_observations/1/memory/private_commit/stable` | `false` |
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `0` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/private_commit/measured_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/available` | `false` |
| `/repeat_observations/1/memory/working_set/stable` | `false` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"unavailable"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `0` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0` |
| `/repeat_observations/1/memory/working_set/warmup_samples` | `[]` |
| `/repeat_observations/1/memory/working_set/measured_samples` | `[]` |
| `/repeat_observations/1/box3d_allocator/baseline_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/final_bytes` | `0` |
| `/repeat_observations/1/box3d_allocator/max_abs_delta` | `0` |
| `/repeat_observations/1/box3d_allocator/exact_return` | `true` |
| `/repeat_observations/1/box3d_allocator/warmup_post_teardown` | `[]` |
| `/repeat_observations/1/box3d_allocator/measured_post_teardown` | `[]` |
| `/repeat_observations/1/crt/applicable` | `false` |
| `/repeat_observations/1/crt/balanced` | `true` |
| `/repeat_observations/1/crt/normal_block_count_delta` | `0` |
| `/repeat_observations/1/crt/normal_block_bytes_delta` | `0` |
| `/repeat_observations/1/crt/client_block_count_delta` | `0` |
| `/repeat_observations/1/crt/client_block_bytes_delta` | `0` |
| `/contact_before_pile_exit` | `false` |
| `/ccd_primary_pass_count` | `0` |
| `/ccd_fallback_pass_count` | `0` |
| `/projectile_speed` | `0` |
| `/surface_separation` | `0` |
| `/final_linear_speed` | `0` |
| `/final_angular_speed` | `0` |
| `/p95_linear_speed` | `0` |
| `/p95_angular_speed` | `0` |
| `/sleep_ratio` | `0` |
| `/max_penetration` | `0` |
| `/minimum_density` | `0` |
| `/maximum_density` | `0` |
| `/energy_at_tick_600` | `0` |
| `/max_energy_growth_ratio` | `0` |
| `/dynamic_body_count` | `0` |
| `/shape_count` | `0` |
| `/joint_count` | `0` |
| `/peak_body_count` | `123` |
| `/peak_shape_count` | `123` |
| `/peak_joint_count` | `1` |
| `/peak_awake_count` | `121` |
| `/peak_contact_count` | `221` |
| `/step_ms/min` | `0.0029` |
| `/step_ms/p50` | `0.005` |
| `/step_ms/p95` | `0.0108` |
| `/step_ms/max` | `1.7802` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unavailable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `false` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `false` |
| `/memory/private_commit/baseline_last_bytes` | `0` |
| `/memory/private_commit/baseline_full_min_bytes` | `0` |
| `/memory/private_commit/baseline_central_min_bytes` | `0` |
| `/memory/private_commit/baseline_median_bytes` | `0` |
| `/memory/private_commit/baseline_central_max_bytes` | `0` |
| `/memory/private_commit/baseline_full_max_bytes` | `0` |
| `/memory/private_commit/final_last_bytes` | `0` |
| `/memory/private_commit/final_full_min_bytes` | `0` |
| `/memory/private_commit/final_central_min_bytes` | `0` |
| `/memory/private_commit/final_median_bytes` | `0` |
| `/memory/private_commit/final_central_max_bytes` | `0` |
| `/memory/private_commit/final_full_max_bytes` | `0` |
| `/memory/private_commit/peak_bytes` | `0` |
| `/memory/private_commit/growth_ratio` | `0` |
| `/memory/private_commit/instant_growth_ratio` | `0` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0` |
| `/memory/private_commit/warmup_full_span_ratio` | `0` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0` |
| `/memory/private_commit/measured_full_span_ratio` | `0` |
| `/memory/private_commit/warmup_samples` | `[]` |
| `/memory/private_commit/measured_samples` | `[]` |
| `/memory/working_set/available` | `false` |
| `/memory/working_set/stable` | `false` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"unavailable"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `0` |
| `/memory/working_set/baseline_full_min_bytes` | `0` |
| `/memory/working_set/baseline_central_min_bytes` | `0` |
| `/memory/working_set/baseline_median_bytes` | `0` |
| `/memory/working_set/baseline_central_max_bytes` | `0` |
| `/memory/working_set/baseline_full_max_bytes` | `0` |
| `/memory/working_set/final_last_bytes` | `0` |
| `/memory/working_set/final_full_min_bytes` | `0` |
| `/memory/working_set/final_central_min_bytes` | `0` |
| `/memory/working_set/final_median_bytes` | `0` |
| `/memory/working_set/final_central_max_bytes` | `0` |
| `/memory/working_set/final_full_max_bytes` | `0` |
| `/memory/working_set/peak_bytes` | `0` |
| `/memory/working_set/growth_ratio` | `0` |
| `/memory/working_set/instant_growth_ratio` | `0` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0` |
| `/memory/working_set/warmup_full_span_ratio` | `0` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0` |
| `/memory/working_set/measured_full_span_ratio` | `0` |
| `/memory/working_set/warmup_samples` | `[]` |
| `/memory/working_set/measured_samples` | `[]` |
| `/box3d_allocator/baseline_bytes` | `0` |
| `/box3d_allocator/final_bytes` | `0` |
| `/box3d_allocator/max_abs_delta` | `0` |
| `/box3d_allocator/exact_return` | `true` |
| `/box3d_allocator/warmup_post_teardown` | `[]` |
| `/box3d_allocator/measured_post_teardown` | `[]` |
| `/crt/applicable` | `false` |
| `/crt/balanced` | `true` |
| `/crt/normal_block_count_delta` | `0` |
| `/crt/normal_block_bytes_delta` | `0` |
| `/crt/client_block_count_delta` | `0` |
| `/crt/client_block_bytes_delta` | `0` |
| `/warmup_ticks` | `0` |
| `/measurement_ticks` | `0` |
| `/allocator_warmup_cycles` | `0` |
| `/stress_cycles` | `0` |
| `/fallback` | `""` |
| `/limits` | `[]` |
| `/metrics` | `[]` |
| `/matrix/0/capability` | `"ccd_dynamic_dynamic"` |
| `/matrix/0/status` | `"pass"` |
| `/matrix/0/fallback` | `null` |
| `/matrix/0/functional_status` | `"pass"` |
| `/matrix/0/functional_fallback` | `null` |
| `/matrix/0/detail` | `"all 20 fixed seeds contacted a dynamic pile body at 35 m/s and four substeps"` |
| `/matrix/0/peak_body_count` | `123` |
| `/matrix/0/peak_shape_count` | `123` |
| `/matrix/0/peak_joint_count` | `0` |
| `/matrix/0/peak_awake_count` | `121` |
| `/matrix/0/peak_contact_count` | `221` |
| `/matrix/0/values/0/name` | `"primary_passes"` |
| `/matrix/0/values/0/value` | `20` |
| `/matrix/0/values/0/unit` | `"seeds"` |
| `/matrix/0/values/1/name` | `"fallback_passes"` |
| `/matrix/0/values/1/value` | `0` |
| `/matrix/0/values/1/unit` | `"seeds"` |
| `/matrix/0/values/2/name` | `"primary_speed"` |
| `/matrix/0/values/2/value` | `35` |
| `/matrix/0/values/2/unit` | `"m/s"` |
| `/matrix/0/values/3/name` | `"primary_substeps"` |
| `/matrix/0/values/3/value` | `4` |
| `/matrix/0/values/3/unit` | `"count"` |
| `/matrix/0/values/4/name` | `"fallback_speed"` |
| `/matrix/0/values/4/value` | `30` |
| `/matrix/0/values/4/unit` | `"m/s"` |
| `/matrix/0/values/5/name` | `"fallback_substeps"` |
| `/matrix/0/values/5/value` | `6` |
| `/matrix/0/values/5/unit` | `"count"` |
| `/matrix/0/values/6/name` | `"fallback_evaluated"` |
| `/matrix/0/values/6/value` | `0` |
| `/matrix/0/values/6/unit` | `"bool"` |
| `/matrix/0/values/7/name` | `"primary_invalid_states"` |
| `/matrix/0/values/7/value` | `0` |
| `/matrix/0/values/7/unit` | `"count"` |
| `/matrix/0/values/8/name` | `"fallback_invalid_states"` |
| `/matrix/0/values/8/value` | `0` |
| `/matrix/0/values/8/unit` | `"count"` |
| `/matrix/0/fixture_hashes/0` | `1431096453509785832` |
| `/matrix/0/fixture_hashes/1` | `5466060815846980033` |
| `/matrix/0/fixture_hashes/2` | `8526278817898170285` |
| `/matrix/0/fixture_hashes/3` | `2675255426310783444` |
| `/matrix/0/fixture_hashes/4` | `10494942409035688541` |
| `/matrix/0/fixture_hashes/5` | `7390650343477109358` |
| `/matrix/0/fixture_hashes/6` | `14725012844580521641` |
| `/matrix/0/fixture_hashes/7` | `14001126938762718496` |
| `/matrix/0/fixture_hashes/8` | `2400058324701230849` |
| `/matrix/0/fixture_hashes/9` | `10732887959698602526` |
| `/matrix/0/fixture_hashes/10` | `5915366692680379584` |
| `/matrix/0/fixture_hashes/11` | `14334879908416984375` |
| `/matrix/0/fixture_hashes/12` | `6577404406665493359` |
| `/matrix/0/fixture_hashes/13` | `16513243213841685896` |
| `/matrix/0/fixture_hashes/14` | `9721495563768596328` |
| `/matrix/0/fixture_hashes/15` | `13937513265211812359` |
| `/matrix/0/fixture_hashes/16` | `9628435969084559192` |
| `/matrix/0/fixture_hashes/17` | `18192646134465658732` |
| `/matrix/0/fixture_hashes/18` | `18426867884952182109` |
| `/matrix/0/fixture_hashes/19` | `11807578939564631579` |
| `/matrix/1/capability` | `"shape_cast_overlap"` |
| `/matrix/1/status` | `"pass"` |
| `/matrix/1/fallback` | `null` |
| `/matrix/1/functional_status` | `"pass"` |
| `/matrix/1/functional_fallback` | `null` |
| `/matrix/1/detail` | `"the first three-meter sphere cast handle agrees with overlap at contact"` |
| `/matrix/1/peak_body_count` | `1` |
| `/matrix/1/peak_shape_count` | `1` |
| `/matrix/1/peak_joint_count` | `0` |
| `/matrix/1/peak_awake_count` | `0` |
| `/matrix/1/peak_contact_count` | `0` |
| `/matrix/1/values/0/name` | `"cast_distance"` |
| `/matrix/1/values/0/value` | `3` |
| `/matrix/1/values/0/unit` | `"m"` |
| `/matrix/1/values/1/name` | `"first_handle_index"` |
| `/matrix/1/values/1/value` | `1` |
| `/matrix/1/values/1/unit` | `"index"` |
| `/matrix/1/values/2/name` | `"first_handle_matches_overlap"` |
| `/matrix/1/values/2/value` | `1` |
| `/matrix/1/values/2/unit` | `"bool"` |
| `/matrix/1/values/3/name` | `"fraction"` |
| `/matrix/1/values/3/value` | `0.810775876045227` |
| `/matrix/1/values/3/unit` | `"ratio"` |
| `/matrix/1/fixture_hashes/0` | `8388802905423810155` |
| `/matrix/2/capability` | `"contact_hit_events"` |
| `/matrix/2/status` | `"pass"` |
| `/matrix/2/fallback` | `null` |
| `/matrix/2/functional_status` | `"pass"` |
| `/matrix/2/functional_fallback` | `null` |
| `/matrix/2/detail` | `"real hit data is finite, energetic, material-tagged, and pair-unique at six substeps"` |
| `/matrix/2/peak_body_count` | `2` |
| `/matrix/2/peak_shape_count` | `2` |
| `/matrix/2/peak_joint_count` | `0` |
| `/matrix/2/peak_awake_count` | `2` |
| `/matrix/2/peak_contact_count` | `1` |
| `/matrix/2/values/0/name` | `"approach_speed"` |
| `/matrix/2/values/0/value` | `15` |
| `/matrix/2/values/0/unit` | `"m/s"` |
| `/matrix/2/values/1/name` | `"effective_mass"` |
| `/matrix/2/values/1/value` | `41.88790512084961` |
| `/matrix/2/values/1/unit` | `"kg"` |
| `/matrix/2/values/2/name` | `"derived_energy"` |
| `/matrix/2/values/2/value` | `4105.0146484375` |
| `/matrix/2/values/2/unit` | `"J"` |
| `/matrix/2/values/3/name` | `"normal_length"` |
| `/matrix/2/values/3/value` | `1` |
| `/matrix/2/values/3/unit` | `"ratio"` |
| `/matrix/2/values/4/name` | `"material_a"` |
| `/matrix/2/values/4/value` | `111` |
| `/matrix/2/values/4/unit` | `"id"` |
| `/matrix/2/values/5/name` | `"material_b"` |
| `/matrix/2/values/5/value` | `222` |
| `/matrix/2/values/5/unit` | `"id"` |
| `/matrix/2/values/6/name` | `"unique_pairs"` |
| `/matrix/2/values/6/value` | `1` |
| `/matrix/2/values/6/unit` | `"count"` |
| `/matrix/2/values/7/name` | `"substeps"` |
| `/matrix/2/values/7/value` | `6` |
| `/matrix/2/values/7/unit` | `"count"` |
| `/matrix/2/fixture_hashes/0` | `10155543446163919611` |
| `/matrix/3/capability` | `"joint_force_torque"` |
| `/matrix/3/status` | `"pass"` |
| `/matrix/3/fallback` | `null` |
| `/matrix/3/functional_status` | `"pass"` |
| `/matrix/3/functional_fallback` | `null` |
| `/matrix/3/detail` | `"joint reaction grows within 50 N tolerance and crosses 10 kN"` |
| `/matrix/3/peak_body_count` | `2` |
| `/matrix/3/peak_shape_count` | `2` |
| `/matrix/3/peak_joint_count` | `1` |
| `/matrix/3/peak_awake_count` | `1` |
| `/matrix/3/peak_contact_count` | `1` |
| `/matrix/3/values/0/name` | `"monotonic_tolerance"` |
| `/matrix/3/values/0/value` | `50` |
| `/matrix/3/values/0/unit` | `"N"` |
| `/matrix/3/values/1/name` | `"maximum_force"` |
| `/matrix/3/values/1/value` | `12693.1640625` |
| `/matrix/3/values/1/unit` | `"N"` |
| `/matrix/3/values/2/name` | `"rupture_threshold"` |
| `/matrix/3/values/2/value` | `10000` |
| `/matrix/3/values/2/unit` | `"N"` |
| `/matrix/3/values/3/name` | `"maximum_deformation"` |
| `/matrix/3/values/3/value` | `0.01363062858581543` |
| `/matrix/3/values/3/unit` | `"m"` |
| `/matrix/3/values/4/name` | `"deformation_threshold"` |
| `/matrix/3/values/4/value` | `0.01` |
| `/matrix/3/values/4/unit` | `"m"` |
| `/matrix/3/values/5/name` | `"consecutive_deformation_ticks"` |
| `/matrix/3/values/5/value` | `0` |
| `/matrix/3/values/5/unit` | `"ticks"` |
| `/matrix/3/fixture_hashes/0` | `14239377403397705414` |
| `/matrix/4/capability` | `"hulls_compounds"` |
| `/matrix/4/status` | `"pass"` |
| `/matrix/4/fallback` | `null` |
| `/matrix/4/functional_status` | `"pass"` |
| `/matrix/4/functional_fallback` | `null` |
| `/matrix/4/detail` | `"one compound containing eight hull children has valid mass, bounds, and contact"` |
| `/matrix/4/peak_body_count` | `2` |
| `/matrix/4/peak_shape_count` | `9` |
| `/matrix/4/peak_joint_count` | `0` |
| `/matrix/4/peak_awake_count` | `1` |
| `/matrix/4/peak_contact_count` | `8` |
| `/matrix/4/values/0/name` | `"hull_count"` |
| `/matrix/4/values/0/value` | `8` |
| `/matrix/4/values/0/unit` | `"count"` |
| `/matrix/4/values/1/name` | `"expected_mass"` |
| `/matrix/4/values/1/value` | `42.666666666666664` |
| `/matrix/4/values/1/unit` | `"kg"` |
| `/matrix/4/values/2/name` | `"mass"` |
| `/matrix/4/values/2/value` | `42.66667175292969` |
| `/matrix/4/values/2/unit` | `"kg"` |
| `/matrix/4/values/3/name` | `"bounds_lower_x"` |
| `/matrix/4/values/3/value` | `-1.6200000047683716` |
| `/matrix/4/values/3/unit` | `"m"` |
| `/matrix/4/values/4/name` | `"bounds_lower_y"` |
| `/matrix/4/values/4/value` | `2.7799999713897705` |
| `/matrix/4/values/4/unit` | `"m"` |
| `/matrix/4/values/5/name` | `"bounds_lower_z"` |
| `/matrix/4/values/5/value` | `-0.2199999988079071` |
| `/matrix/4/values/5/unit` | `"m"` |
| `/matrix/4/values/6/name` | `"bounds_upper_x"` |
| `/matrix/4/values/6/value` | `1.6200000047683716` |
| `/matrix/4/values/6/unit` | `"m"` |
| `/matrix/4/values/7/name` | `"bounds_upper_y"` |
| `/matrix/4/values/7/value` | `3.2200000286102295` |
| `/matrix/4/values/7/unit` | `"m"` |
| `/matrix/4/values/8/name` | `"bounds_upper_z"` |
| `/matrix/4/values/8/value` | `0.2199999988079071` |
| `/matrix/4/values/8/unit` | `"m"` |
| `/matrix/4/values/9/name` | `"bounds_tolerance"` |
| `/matrix/4/values/9/value` | `0.0001` |
| `/matrix/4/values/9/unit` | `"m"` |
| `/matrix/4/values/10/name` | `"contacted"` |
| `/matrix/4/values/10/value` | `1` |
| `/matrix/4/values/10/unit` | `"bool"` |
| `/matrix/4/values/11/name` | `"state_valid"` |
| `/matrix/4/values/11/value` | `1` |
| `/matrix/4/values/11/unit` | `"bool"` |
| `/matrix/4/values/12/name` | `"box3d_allocator_baseline_bytes"` |
| `/matrix/4/values/12/value` | `0` |
| `/matrix/4/values/12/unit` | `"bytes"` |
| `/matrix/4/values/13/name` | `"box3d_allocator_final_bytes"` |
| `/matrix/4/values/13/value` | `0` |
| `/matrix/4/values/13/unit` | `"bytes"` |
| `/matrix/4/fixture_hashes/0` | `7710058609812386530` |
| `/matrix/5/capability` | `"radial_sleep"` |
| `/matrix/5/status` | `"pass"` |
| `/matrix/5/fallback` | `null` |
| `/matrix/5/functional_status` | `"pass"` |
| `/matrix/5/functional_fallback` | `null` |
| `/matrix/5/detail` | `"the public radial-gravity pile satisfies every fixed sleep limit"` |
| `/matrix/5/peak_body_count` | `81` |
| `/matrix/5/peak_shape_count` | `81` |
| `/matrix/5/peak_joint_count` | `0` |
| `/matrix/5/peak_awake_count` | `80` |
| `/matrix/5/peak_contact_count` | `204` |
| `/matrix/5/values/0/name` | `"sleep_ratio"` |
| `/matrix/5/values/0/value` | `1` |
| `/matrix/5/values/0/unit` | `"ratio"` |
| `/matrix/5/values/1/name` | `"p95_linear_speed"` |
| `/matrix/5/values/1/value` | `0` |
| `/matrix/5/values/1/unit` | `"m/s"` |
| `/matrix/5/values/2/name` | `"p95_angular_speed"` |
| `/matrix/5/values/2/value` | `0` |
| `/matrix/5/values/2/unit` | `"rad/s"` |
| `/matrix/5/values/3/name` | `"max_penetration"` |
| `/matrix/5/values/3/value` | `0.0015351474285125732` |
| `/matrix/5/values/3/unit` | `"m"` |
| `/matrix/5/values/4/name` | `"energy_growth"` |
| `/matrix/5/values/4/value` | `0` |
| `/matrix/5/values/4/unit` | `"ratio"` |
| `/matrix/5/fixture_hashes/0` | `10363635776067367757` |
| `/matrix/6/capability` | `"batch_lifecycle"` |
| `/matrix/6/status` | `"pass"` |
| `/matrix/6/fallback` | `null` |
| `/matrix/6/functional_status` | `"pass"` |
| `/matrix/6/functional_fallback` | `null` |
| `/matrix/6/detail` | `"10,000 create-destroy cycles preserved handle generations without persistent growth"` |
| `/matrix/6/peak_body_count` | `1` |
| `/matrix/6/peak_shape_count` | `1` |
| `/matrix/6/peak_joint_count` | `0` |
| `/matrix/6/peak_awake_count` | `1` |
| `/matrix/6/peak_contact_count` | `0` |
| `/matrix/6/values/0/name` | `"generation_cycles"` |
| `/matrix/6/values/0/value` | `10000` |
| `/matrix/6/values/0/unit` | `"count"` |
| `/matrix/6/values/1/name` | `"invalid_handles"` |
| `/matrix/6/values/1/value` | `0` |
| `/matrix/6/values/1/unit` | `"count"` |
| `/matrix/6/values/2/name` | `"private_commit_available"` |
| `/matrix/6/values/2/value` | `1` |
| `/matrix/6/values/2/unit` | `"bool"` |
| `/matrix/6/values/3/name` | `"private_commit_growth"` |
| `/matrix/6/values/3/value` | `0` |
| `/matrix/6/values/3/unit` | `"ratio"` |
| `/matrix/6/values/4/name` | `"private_commit_baseline_bytes"` |
| `/matrix/6/values/4/value` | `2097152` |
| `/matrix/6/values/4/unit` | `"bytes"` |
| `/matrix/6/values/5/name` | `"private_commit_final_bytes"` |
| `/matrix/6/values/5/value` | `2097152` |
| `/matrix/6/values/5/unit` | `"bytes"` |
| `/matrix/6/values/6/name` | `"working_set_baseline_bytes"` |
| `/matrix/6/values/6/value` | `5214208` |
| `/matrix/6/values/6/unit` | `"bytes"` |
| `/matrix/6/values/7/name` | `"working_set_final_bytes"` |
| `/matrix/6/values/7/value` | `5214208` |
| `/matrix/6/values/7/unit` | `"bytes"` |
| `/matrix/6/values/8/name` | `"crt_normal_count_delta"` |
| `/matrix/6/values/8/value` | `0` |
| `/matrix/6/values/8/unit` | `"count"` |
| `/matrix/6/values/9/name` | `"crt_normal_bytes_delta"` |
| `/matrix/6/values/9/value` | `0` |
| `/matrix/6/values/9/unit` | `"bytes"` |
| `/matrix/6/values/10/name` | `"crt_client_count_delta"` |
| `/matrix/6/values/10/value` | `0` |
| `/matrix/6/values/10/unit` | `"count"` |
| `/matrix/6/values/11/name` | `"crt_client_bytes_delta"` |
| `/matrix/6/values/11/value` | `0` |
| `/matrix/6/values/11/unit` | `"bytes"` |
| `/matrix/6/values/12/name` | `"box3d_allocator_baseline_bytes"` |
| `/matrix/6/values/12/value` | `0` |
| `/matrix/6/values/12/unit` | `"bytes"` |
| `/matrix/6/values/13/name` | `"box3d_allocator_final_bytes"` |
| `/matrix/6/values/13/value` | `0` |
| `/matrix/6/values/13/unit` | `"bytes"` |
| `/matrix/6/fixture_hashes/0` | `43224550866945` |
| `/matrix/7/capability` | `"upstream_replay"` |
| `/matrix/7/status` | `"pass"` |
| `/matrix/7/fallback` | `null` |
| `/matrix/7/functional_status` | `"pass"` |
| `/matrix/7/functional_fallback` | `null` |
| `/matrix/7/detail` | `"the pinned official recorder saved, loaded, and validated a minimal replay"` |
| `/matrix/7/peak_body_count` | `1` |
| `/matrix/7/peak_shape_count` | `1` |
| `/matrix/7/peak_joint_count` | `0` |
| `/matrix/7/peak_awake_count` | `1` |
| `/matrix/7/peak_contact_count` | `0` |
| `/matrix/7/values/0/name` | `"saved"` |
| `/matrix/7/values/0/value` | `1` |
| `/matrix/7/values/0/unit` | `"bool"` |
| `/matrix/7/values/1/name` | `"loaded"` |
| `/matrix/7/values/1/value` | `1` |
| `/matrix/7/values/1/unit` | `"bool"` |
| `/matrix/7/values/2/name` | `"validated"` |
| `/matrix/7/values/2/value` | `1` |
| `/matrix/7/values/2/unit` | `"bool"` |
| `/matrix/7/values/3/name` | `"recording_bytes"` |
| `/matrix/7/values/3/value` | `6982` |
| `/matrix/7/values/3/unit` | `"bytes"` |
| `/matrix/7/values/4/name` | `"temporary_file_removed"` |
| `/matrix/7/values/4/value` | `1` |
| `/matrix/7/values/4/unit` | `"bool"` |
| `/matrix/7/values/5/name` | `"box3d_allocator_baseline_bytes"` |
| `/matrix/7/values/5/value` | `0` |
| `/matrix/7/values/5/unit` | `"bytes"` |
| `/matrix/7/values/6/name` | `"box3d_allocator_final_bytes"` |
| `/matrix/7/values/6/value` | `0` |
| `/matrix/7/values/6/unit` | `"bytes"` |
| `/matrix/7/fixture_hashes/0` | `13184605773762244167` |
| `/warnings` | `[]` |
| `/violations` | `[]` |

## Ownership, Allocator, and CRT

The following inventory includes process totals, scenario aggregates, and every repeat observation.

### Debug ownership evidence

#### Process Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_fall` aggregate Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_fall` aggregate CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `radial_fall` repeat 1 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_fall` repeat 1 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `radial_fall` repeat 2 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_fall` repeat 2 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `projectile_pile` aggregate Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `projectile_pile` aggregate CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `projectile_pile` repeat 1 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `projectile_pile` repeat 1 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `projectile_pile` repeat 2 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `projectile_pile` repeat 2 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `radial_pile` aggregate Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_pile` aggregate CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `radial_pile` repeat 1 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_pile` repeat 1 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `radial_pile` repeat 2 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_pile` repeat 2 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `mass_ratio` aggregate Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `mass_ratio` aggregate CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `mass_ratio` repeat 1 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `mass_ratio` repeat 1 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `mass_ratio` repeat 2 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `mass_ratio` repeat 2 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `stress` aggregate Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown/0` | `0` |
| `/warmup_post_teardown/1` | `0` |
| `/warmup_post_teardown/2` | `0` |
| `/warmup_post_teardown/3` | `0` |
| `/warmup_post_teardown/4` | `0` |
| `/warmup_post_teardown/5` | `0` |
| `/warmup_post_teardown/6` | `0` |
| `/warmup_post_teardown/7` | `0` |
| `/warmup_post_teardown/8` | `0` |
| `/warmup_post_teardown/9` | `0` |
| `/measured_post_teardown/0` | `0` |
| `/measured_post_teardown/1` | `0` |
| `/measured_post_teardown/2` | `0` |
| `/measured_post_teardown/3` | `0` |
| `/measured_post_teardown/4` | `0` |
| `/measured_post_teardown/5` | `0` |
| `/measured_post_teardown/6` | `0` |
| `/measured_post_teardown/7` | `0` |
| `/measured_post_teardown/8` | `0` |
| `/measured_post_teardown/9` | `0` |

#### `stress` aggregate CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `true` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `stress` repeat 1 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown/0` | `0` |
| `/warmup_post_teardown/1` | `0` |
| `/warmup_post_teardown/2` | `0` |
| `/warmup_post_teardown/3` | `0` |
| `/warmup_post_teardown/4` | `0` |
| `/warmup_post_teardown/5` | `0` |
| `/warmup_post_teardown/6` | `0` |
| `/warmup_post_teardown/7` | `0` |
| `/warmup_post_teardown/8` | `0` |
| `/warmup_post_teardown/9` | `0` |
| `/measured_post_teardown/0` | `0` |
| `/measured_post_teardown/1` | `0` |
| `/measured_post_teardown/2` | `0` |
| `/measured_post_teardown/3` | `0` |
| `/measured_post_teardown/4` | `0` |
| `/measured_post_teardown/5` | `0` |
| `/measured_post_teardown/6` | `0` |
| `/measured_post_teardown/7` | `0` |
| `/measured_post_teardown/8` | `0` |
| `/measured_post_teardown/9` | `0` |

#### `stress` repeat 1 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `true` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `stress` repeat 2 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown/0` | `0` |
| `/warmup_post_teardown/1` | `0` |
| `/warmup_post_teardown/2` | `0` |
| `/warmup_post_teardown/3` | `0` |
| `/warmup_post_teardown/4` | `0` |
| `/warmup_post_teardown/5` | `0` |
| `/warmup_post_teardown/6` | `0` |
| `/warmup_post_teardown/7` | `0` |
| `/warmup_post_teardown/8` | `0` |
| `/warmup_post_teardown/9` | `0` |
| `/measured_post_teardown/0` | `0` |
| `/measured_post_teardown/1` | `0` |
| `/measured_post_teardown/2` | `0` |
| `/measured_post_teardown/3` | `0` |
| `/measured_post_teardown/4` | `0` |
| `/measured_post_teardown/5` | `0` |
| `/measured_post_teardown/6` | `0` |
| `/measured_post_teardown/7` | `0` |
| `/measured_post_teardown/8` | `0` |
| `/measured_post_teardown/9` | `0` |

#### `stress` repeat 2 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `true` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `capability_matrix` aggregate Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `capability_matrix` aggregate CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `capability_matrix` repeat 1 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `capability_matrix` repeat 1 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `capability_matrix` repeat 2 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `capability_matrix` repeat 2 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

### Release ownership evidence

#### Process Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_fall` aggregate Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_fall` aggregate CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `radial_fall` repeat 1 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_fall` repeat 1 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `radial_fall` repeat 2 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_fall` repeat 2 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `projectile_pile` aggregate Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `projectile_pile` aggregate CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `projectile_pile` repeat 1 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `projectile_pile` repeat 1 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `projectile_pile` repeat 2 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `projectile_pile` repeat 2 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `radial_pile` aggregate Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_pile` aggregate CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `radial_pile` repeat 1 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_pile` repeat 1 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `radial_pile` repeat 2 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `radial_pile` repeat 2 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `mass_ratio` aggregate Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `mass_ratio` aggregate CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `mass_ratio` repeat 1 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `mass_ratio` repeat 1 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `mass_ratio` repeat 2 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `mass_ratio` repeat 2 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `stress` aggregate Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown/0` | `0` |
| `/warmup_post_teardown/1` | `0` |
| `/warmup_post_teardown/2` | `0` |
| `/warmup_post_teardown/3` | `0` |
| `/warmup_post_teardown/4` | `0` |
| `/warmup_post_teardown/5` | `0` |
| `/warmup_post_teardown/6` | `0` |
| `/warmup_post_teardown/7` | `0` |
| `/warmup_post_teardown/8` | `0` |
| `/warmup_post_teardown/9` | `0` |
| `/measured_post_teardown/0` | `0` |
| `/measured_post_teardown/1` | `0` |
| `/measured_post_teardown/2` | `0` |
| `/measured_post_teardown/3` | `0` |
| `/measured_post_teardown/4` | `0` |
| `/measured_post_teardown/5` | `0` |
| `/measured_post_teardown/6` | `0` |
| `/measured_post_teardown/7` | `0` |
| `/measured_post_teardown/8` | `0` |
| `/measured_post_teardown/9` | `0` |

#### `stress` aggregate CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `stress` repeat 1 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown/0` | `0` |
| `/warmup_post_teardown/1` | `0` |
| `/warmup_post_teardown/2` | `0` |
| `/warmup_post_teardown/3` | `0` |
| `/warmup_post_teardown/4` | `0` |
| `/warmup_post_teardown/5` | `0` |
| `/warmup_post_teardown/6` | `0` |
| `/warmup_post_teardown/7` | `0` |
| `/warmup_post_teardown/8` | `0` |
| `/warmup_post_teardown/9` | `0` |
| `/measured_post_teardown/0` | `0` |
| `/measured_post_teardown/1` | `0` |
| `/measured_post_teardown/2` | `0` |
| `/measured_post_teardown/3` | `0` |
| `/measured_post_teardown/4` | `0` |
| `/measured_post_teardown/5` | `0` |
| `/measured_post_teardown/6` | `0` |
| `/measured_post_teardown/7` | `0` |
| `/measured_post_teardown/8` | `0` |
| `/measured_post_teardown/9` | `0` |

#### `stress` repeat 1 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `stress` repeat 2 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown/0` | `0` |
| `/warmup_post_teardown/1` | `0` |
| `/warmup_post_teardown/2` | `0` |
| `/warmup_post_teardown/3` | `0` |
| `/warmup_post_teardown/4` | `0` |
| `/warmup_post_teardown/5` | `0` |
| `/warmup_post_teardown/6` | `0` |
| `/warmup_post_teardown/7` | `0` |
| `/warmup_post_teardown/8` | `0` |
| `/warmup_post_teardown/9` | `0` |
| `/measured_post_teardown/0` | `0` |
| `/measured_post_teardown/1` | `0` |
| `/measured_post_teardown/2` | `0` |
| `/measured_post_teardown/3` | `0` |
| `/measured_post_teardown/4` | `0` |
| `/measured_post_teardown/5` | `0` |
| `/measured_post_teardown/6` | `0` |
| `/measured_post_teardown/7` | `0` |
| `/measured_post_teardown/8` | `0` |
| `/measured_post_teardown/9` | `0` |

#### `stress` repeat 2 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `capability_matrix` aggregate Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `capability_matrix` aggregate CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `capability_matrix` repeat 1 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `capability_matrix` repeat 1 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

#### `capability_matrix` repeat 2 Box3D allocator

| JSON pointer | Value |
| --- | --- |
| `/baseline_bytes` | `0` |
| `/final_bytes` | `0` |
| `/max_abs_delta` | `0` |
| `/exact_return` | `true` |
| `/warmup_post_teardown` | `[]` |
| `/measured_post_teardown` | `[]` |

#### `capability_matrix` repeat 2 CRT

| JSON pointer | Value |
| --- | --- |
| `/applicable` | `false` |
| `/balanced` | `true` |
| `/normal_block_count_delta` | `0` |
| `/normal_block_bytes_delta` | `0` |
| `/client_block_count_delta` | `0` |
| `/client_block_bytes_delta` | `0` |

## Footprint Diagnostic 10+10

All four stress repeats record ten warmup and ten measured PrivateUsage samples, plus ten warmup and ten measured Working Set samples. Every raw and derived field follows.

### Debug stress repeat 1

| JSON pointer | Value |
| --- | --- |
| `/gate_scope` | `"release_mt"` |
| `/gate_status` | `"diagnostic"` |
| `/assessment_status` | `"unstable"` |
| `/gate_applied` | `false` |
| `/budget_qualified` | `false` |
| `/budget_scope` | `"future_packaged_reference_hardware"` |
| `/private_commit/available` | `true` |
| `/private_commit/stable` | `false` |
| `/private_commit/terminal_growth` | `true` |
| `/private_commit/baseline_last_bytes` | `6979584` |
| `/private_commit/baseline_full_min_bytes` | `4345856` |
| `/private_commit/baseline_central_min_bytes` | `4878336` |
| `/private_commit/baseline_median_bytes` | `5726208` |
| `/private_commit/baseline_central_max_bytes` | `6959104` |
| `/private_commit/baseline_full_max_bytes` | `6979584` |
| `/private_commit/final_last_bytes` | `6934528` |
| `/private_commit/final_full_min_bytes` | `4370432` |
| `/private_commit/final_central_min_bytes` | `6377472` |
| `/private_commit/final_median_bytes` | `6807552` |
| `/private_commit/final_central_max_bytes` | `6934528` |
| `/private_commit/final_full_max_bytes` | `6971392` |
| `/private_commit/peak_bytes` | `6979584` |
| `/private_commit/growth_ratio` | `0.1888412017167382` |
| `/private_commit/instant_growth_ratio` | `0` |
| `/private_commit/warmup_trimmed_span_ratio` | `0.36337625178826893` |
| `/private_commit/warmup_full_span_ratio` | `0.45994277539341916` |
| `/private_commit/measured_trimmed_span_ratio` | `0.08182912154031288` |
| `/private_commit/measured_full_span_ratio` | `0.38206979542719616` |
| `/private_commit/warmup_samples/0` | `4382720` |
| `/private_commit/warmup_samples/1` | `5488640` |
| `/private_commit/warmup_samples/2` | `4366336` |
| `/private_commit/warmup_samples/3` | `5697536` |
| `/private_commit/warmup_samples/4` | `6979584` |
| `/private_commit/warmup_samples/5` | `5726208` |
| `/private_commit/warmup_samples/6` | `4878336` |
| `/private_commit/warmup_samples/7` | `4345856` |
| `/private_commit/warmup_samples/8` | `6959104` |
| `/private_commit/warmup_samples/9` | `6979584` |
| `/private_commit/measured_samples/0` | `4378624` |
| `/private_commit/measured_samples/1` | `6885376` |
| `/private_commit/measured_samples/2` | `5234688` |
| `/private_commit/measured_samples/3` | `4374528` |
| `/private_commit/measured_samples/4` | `6144000` |
| `/private_commit/measured_samples/5` | `4370432` |
| `/private_commit/measured_samples/6` | `6971392` |
| `/private_commit/measured_samples/7` | `6377472` |
| `/private_commit/measured_samples/8` | `6807552` |
| `/private_commit/measured_samples/9` | `6934528` |
| `/working_set/available` | `true` |
| `/working_set/stable` | `false` |
| `/working_set/terminal_growth` | `true` |
| `/working_set/assessment_status` | `"unstable"` |
| `/working_set/gate_status` | `"diagnostic"` |
| `/working_set/gate_applied` | `false` |
| `/working_set/budget_qualified` | `false` |
| `/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/working_set/baseline_last_bytes` | `8032256` |
| `/working_set/baseline_full_min_bytes` | `7094272` |
| `/working_set/baseline_central_min_bytes` | `7131136` |
| `/working_set/baseline_median_bytes` | `7327744` |
| `/working_set/baseline_central_max_bytes` | `8032256` |
| `/working_set/baseline_full_max_bytes` | `8032256` |
| `/working_set/final_last_bytes` | `8052736` |
| `/working_set/final_full_min_bytes` | `7127040` |
| `/working_set/final_central_min_bytes` | `8044544` |
| `/working_set/final_median_bytes` | `8052736` |
| `/working_set/final_central_max_bytes` | `8052736` |
| `/working_set/final_full_max_bytes` | `8060928` |
| `/working_set/peak_bytes` | `9588736` |
| `/working_set/growth_ratio` | `0.09893795416433762` |
| `/working_set/instant_growth_ratio` | `0.0025497195308516064` |
| `/working_set/warmup_trimmed_span_ratio` | `0.12297372833985466` |
| `/working_set/warmup_full_span_ratio` | `0.12800447177193963` |
| `/working_set/measured_trimmed_span_ratio` | `0.001017293997965412` |
| `/working_set/measured_full_span_ratio` | `0.11597151576805696` |
| `/working_set/warmup_samples/0` | `7118848` |
| `/working_set/warmup_samples/1` | `7340032` |
| `/working_set/warmup_samples/2` | `7102464` |
| `/working_set/warmup_samples/3` | `7323648` |
| `/working_set/warmup_samples/4` | `8019968` |
| `/working_set/warmup_samples/5` | `7327744` |
| `/working_set/warmup_samples/6` | `7131136` |
| `/working_set/warmup_samples/7` | `7094272` |
| `/working_set/warmup_samples/8` | `8032256` |
| `/working_set/warmup_samples/9` | `8032256` |
| `/working_set/measured_samples/0` | `7135232` |
| `/working_set/measured_samples/1` | `8060928` |
| `/working_set/measured_samples/2` | `7356416` |
| `/working_set/measured_samples/3` | `7135232` |
| `/working_set/measured_samples/4` | `8065024` |
| `/working_set/measured_samples/5` | `7127040` |
| `/working_set/measured_samples/6` | `8060928` |
| `/working_set/measured_samples/7` | `8052736` |
| `/working_set/measured_samples/8` | `8044544` |
| `/working_set/measured_samples/9` | `8052736` |

### Debug stress repeat 2

| JSON pointer | Value |
| --- | --- |
| `/gate_scope` | `"release_mt"` |
| `/gate_status` | `"diagnostic"` |
| `/assessment_status` | `"unstable"` |
| `/gate_applied` | `false` |
| `/budget_qualified` | `false` |
| `/budget_scope` | `"future_packaged_reference_hardware"` |
| `/private_commit/available` | `true` |
| `/private_commit/stable` | `false` |
| `/private_commit/terminal_growth` | `false` |
| `/private_commit/baseline_last_bytes` | `6664192` |
| `/private_commit/baseline_full_min_bytes` | `6664192` |
| `/private_commit/baseline_central_min_bytes` | `6692864` |
| `/private_commit/baseline_median_bytes` | `6746112` |
| `/private_commit/baseline_central_max_bytes` | `6787072` |
| `/private_commit/baseline_full_max_bytes` | `6934528` |
| `/private_commit/final_last_bytes` | `6430720` |
| `/private_commit/final_full_min_bytes` | `5914624` |
| `/private_commit/final_central_min_bytes` | `5931008` |
| `/private_commit/final_median_bytes` | `6213632` |
| `/private_commit/final_central_max_bytes` | `6430720` |
| `/private_commit/final_full_max_bytes` | `6561792` |
| `/private_commit/peak_bytes` | `6934528` |
| `/private_commit/growth_ratio` | `0` |
| `/private_commit/instant_growth_ratio` | `0` |
| `/private_commit/warmup_trimmed_span_ratio` | `0.013964784456587736` |
| `/private_commit/warmup_full_span_ratio` | `0.04007285974499089` |
| `/private_commit/measured_trimmed_span_ratio` | `0.08042188529993408` |
| `/private_commit/measured_full_span_ratio` | `0.1041529334212261` |
| `/private_commit/warmup_samples/0` | `6635520` |
| `/private_commit/warmup_samples/1` | `4378624` |
| `/private_commit/warmup_samples/2` | `6799360` |
| `/private_commit/warmup_samples/3` | `6684672` |
| `/private_commit/warmup_samples/4` | `6733824` |
| `/private_commit/warmup_samples/5` | `6692864` |
| `/private_commit/warmup_samples/6` | `6787072` |
| `/private_commit/warmup_samples/7` | `6934528` |
| `/private_commit/warmup_samples/8` | `6746112` |
| `/private_commit/warmup_samples/9` | `6664192` |
| `/private_commit/measured_samples/0` | `6737920` |
| `/private_commit/measured_samples/1` | `6463488` |
| `/private_commit/measured_samples/2` | `6754304` |
| `/private_commit/measured_samples/3` | `6918144` |
| `/private_commit/measured_samples/4` | `4370432` |
| `/private_commit/measured_samples/5` | `6561792` |
| `/private_commit/measured_samples/6` | `5914624` |
| `/private_commit/measured_samples/7` | `6213632` |
| `/private_commit/measured_samples/8` | `5931008` |
| `/private_commit/measured_samples/9` | `6430720` |
| `/working_set/available` | `true` |
| `/working_set/stable` | `true` |
| `/working_set/terminal_growth` | `false` |
| `/working_set/assessment_status` | `"pass"` |
| `/working_set/gate_status` | `"diagnostic"` |
| `/working_set/gate_applied` | `false` |
| `/working_set/budget_qualified` | `false` |
| `/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/working_set/baseline_last_bytes` | `8052736` |
| `/working_set/baseline_full_min_bytes` | `8052736` |
| `/working_set/baseline_central_min_bytes` | `8056832` |
| `/working_set/baseline_median_bytes` | `8056832` |
| `/working_set/baseline_central_max_bytes` | `8060928` |
| `/working_set/baseline_full_max_bytes` | `8069120` |
| `/working_set/final_last_bytes` | `8122368` |
| `/working_set/final_full_min_bytes` | `8052736` |
| `/working_set/final_central_min_bytes` | `8060928` |
| `/working_set/final_median_bytes` | `8122368` |
| `/working_set/final_central_max_bytes` | `8134656` |
| `/working_set/final_full_max_bytes` | `8142848` |
| `/working_set/peak_bytes` | `9695232` |
| `/working_set/growth_ratio` | `0.00813421453990849` |
| `/working_set/instant_growth_ratio` | `0.008646998982706003` |
| `/working_set/warmup_trimmed_span_ratio` | `0.0005083884087442806` |
| `/working_set/warmup_full_span_ratio` | `0.0020335536349771225` |
| `/working_set/measured_trimmed_span_ratio` | `0.009077155824508321` |
| `/working_set/measured_full_span_ratio` | `0.011094301563287948` |
| `/working_set/warmup_samples/0` | `8073216` |
| `/working_set/warmup_samples/1` | `7135232` |
| `/working_set/warmup_samples/2` | `8056832` |
| `/working_set/warmup_samples/3` | `8056832` |
| `/working_set/warmup_samples/4` | `8056832` |
| `/working_set/warmup_samples/5` | `8056832` |
| `/working_set/warmup_samples/6` | `8069120` |
| `/working_set/warmup_samples/7` | `8056832` |
| `/working_set/warmup_samples/8` | `8060928` |
| `/working_set/warmup_samples/9` | `8052736` |
| `/working_set/measured_samples/0` | `8077312` |
| `/working_set/measured_samples/1` | `8065024` |
| `/working_set/measured_samples/2` | `8069120` |
| `/working_set/measured_samples/3` | `8073216` |
| `/working_set/measured_samples/4` | `7139328` |
| `/working_set/measured_samples/5` | `8134656` |
| `/working_set/measured_samples/6` | `8052736` |
| `/working_set/measured_samples/7` | `8142848` |
| `/working_set/measured_samples/8` | `8060928` |
| `/working_set/measured_samples/9` | `8122368` |

### Release stress repeat 1

| JSON pointer | Value |
| --- | --- |
| `/gate_scope` | `"release_mt"` |
| `/gate_status` | `"diagnostic"` |
| `/assessment_status` | `"pass"` |
| `/gate_applied` | `false` |
| `/budget_qualified` | `false` |
| `/budget_scope` | `"future_packaged_reference_hardware"` |
| `/private_commit/available` | `true` |
| `/private_commit/stable` | `true` |
| `/private_commit/terminal_growth` | `false` |
| `/private_commit/baseline_last_bytes` | `2019328` |
| `/private_commit/baseline_full_min_bytes` | `2019328` |
| `/private_commit/baseline_central_min_bytes` | `2146304` |
| `/private_commit/baseline_median_bytes` | `2220032` |
| `/private_commit/baseline_central_max_bytes` | `2224128` |
| `/private_commit/baseline_full_max_bytes` | `2347008` |
| `/private_commit/final_last_bytes` | `1978368` |
| `/private_commit/final_full_min_bytes` | `1978368` |
| `/private_commit/final_central_min_bytes` | `1978368` |
| `/private_commit/final_median_bytes` | `1990656` |
| `/private_commit/final_central_max_bytes` | `1994752` |
| `/private_commit/final_full_max_bytes` | `2162688` |
| `/private_commit/peak_bytes` | `2363392` |
| `/private_commit/growth_ratio` | `0` |
| `/private_commit/instant_growth_ratio` | `0` |
| `/private_commit/warmup_trimmed_span_ratio` | `0.03505535055350553` |
| `/private_commit/warmup_full_span_ratio` | `0.14760147601476015` |
| `/private_commit/measured_trimmed_span_ratio` | `0.00823045267489712` |
| `/private_commit/measured_full_span_ratio` | `0.09259259259259259` |
| `/private_commit/warmup_samples/0` | `1646592` |
| `/private_commit/warmup_samples/1` | `1708032` |
| `/private_commit/warmup_samples/2` | `2121728` |
| `/private_commit/warmup_samples/3` | `1953792` |
| `/private_commit/warmup_samples/4` | `1957888` |
| `/private_commit/warmup_samples/5` | `2146304` |
| `/private_commit/warmup_samples/6` | `2224128` |
| `/private_commit/warmup_samples/7` | `2220032` |
| `/private_commit/warmup_samples/8` | `2347008` |
| `/private_commit/warmup_samples/9` | `2019328` |
| `/private_commit/measured_samples/0` | `2314240` |
| `/private_commit/measured_samples/1` | `2068480` |
| `/private_commit/measured_samples/2` | `2363392` |
| `/private_commit/measured_samples/3` | `2035712` |
| `/private_commit/measured_samples/4` | `1994752` |
| `/private_commit/measured_samples/5` | `1978368` |
| `/private_commit/measured_samples/6` | `1990656` |
| `/private_commit/measured_samples/7` | `2162688` |
| `/private_commit/measured_samples/8` | `1994752` |
| `/private_commit/measured_samples/9` | `1978368` |
| `/working_set/available` | `true` |
| `/working_set/stable` | `true` |
| `/working_set/terminal_growth` | `false` |
| `/working_set/assessment_status` | `"pass"` |
| `/working_set/gate_status` | `"diagnostic"` |
| `/working_set/gate_applied` | `false` |
| `/working_set/budget_qualified` | `false` |
| `/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/working_set/baseline_last_bytes` | `5185536` |
| `/working_set/baseline_full_min_bytes` | `5185536` |
| `/working_set/baseline_central_min_bytes` | `5193728` |
| `/working_set/baseline_median_bytes` | `5197824` |
| `/working_set/baseline_central_max_bytes` | `5206016` |
| `/working_set/baseline_full_max_bytes` | `5214208` |
| `/working_set/final_last_bytes` | `5189632` |
| `/working_set/final_full_min_bytes` | `5189632` |
| `/working_set/final_central_min_bytes` | `5189632` |
| `/working_set/final_median_bytes` | `5201920` |
| `/working_set/final_central_max_bytes` | `5206016` |
| `/working_set/final_full_max_bytes` | `5222400` |
| `/working_set/peak_bytes` | `6701056` |
| `/working_set/growth_ratio` | `0.0007880220646178094` |
| `/working_set/instant_growth_ratio` | `0.0007898894154818325` |
| `/working_set/warmup_trimmed_span_ratio` | `0.002364066193853428` |
| `/working_set/warmup_full_span_ratio` | `0.005516154452324665` |
| `/working_set/measured_trimmed_span_ratio` | `0.0031496062992125984` |
| `/working_set/measured_full_span_ratio` | `0.006299212598425197` |
| `/working_set/warmup_samples/0` | `5009408` |
| `/working_set/warmup_samples/1` | `5070848` |
| `/working_set/warmup_samples/2` | `5181440` |
| `/working_set/warmup_samples/3` | `5124096` |
| `/working_set/warmup_samples/4` | `5140480` |
| `/working_set/warmup_samples/5` | `5193728` |
| `/working_set/warmup_samples/6` | `5206016` |
| `/working_set/warmup_samples/7` | `5197824` |
| `/working_set/warmup_samples/8` | `5214208` |
| `/working_set/warmup_samples/9` | `5185536` |
| `/working_set/measured_samples/0` | `5255168` |
| `/working_set/measured_samples/1` | `5238784` |
| `/working_set/measured_samples/2` | `5255168` |
| `/working_set/measured_samples/3` | `5234688` |
| `/working_set/measured_samples/4` | `5206016` |
| `/working_set/measured_samples/5` | `5189632` |
| `/working_set/measured_samples/6` | `5201920` |
| `/working_set/measured_samples/7` | `5222400` |
| `/working_set/measured_samples/8` | `5206016` |
| `/working_set/measured_samples/9` | `5189632` |

### Release stress repeat 2

| JSON pointer | Value |
| --- | --- |
| `/gate_scope` | `"release_mt"` |
| `/gate_status` | `"diagnostic"` |
| `/assessment_status` | `"unstable"` |
| `/gate_applied` | `false` |
| `/budget_qualified` | `false` |
| `/budget_scope` | `"future_packaged_reference_hardware"` |
| `/private_commit/available` | `true` |
| `/private_commit/stable` | `false` |
| `/private_commit/terminal_growth` | `false` |
| `/private_commit/baseline_last_bytes` | `3264512` |
| `/private_commit/baseline_full_min_bytes` | `1974272` |
| `/private_commit/baseline_central_min_bytes` | `1998848` |
| `/private_commit/baseline_median_bytes` | `3264512` |
| `/private_commit/baseline_central_max_bytes` | `3432448` |
| `/private_commit/baseline_full_max_bytes` | `3547136` |
| `/private_commit/final_last_bytes` | `3129344` |
| `/private_commit/final_full_min_bytes` | `3129344` |
| `/private_commit/final_central_min_bytes` | `3551232` |
| `/private_commit/final_median_bytes` | `3559424` |
| `/private_commit/final_central_max_bytes` | `3588096` |
| `/private_commit/final_full_max_bytes` | `3592192` |
| `/private_commit/peak_bytes` | `3665920` |
| `/private_commit/growth_ratio` | `0.0903387703889586` |
| `/private_commit/instant_growth_ratio` | `0` |
| `/private_commit/warmup_trimmed_span_ratio` | `0.43914680050188204` |
| `/private_commit/warmup_full_span_ratio` | `0.48180677540777916` |
| `/private_commit/measured_trimmed_span_ratio` | `0.010356731875719217` |
| `/private_commit/measured_full_span_ratio` | `0.13003452243958574` |
| `/private_commit/warmup_samples/0` | `1982464` |
| `/private_commit/warmup_samples/1` | `2121728` |
| `/private_commit/warmup_samples/2` | `1982464` |
| `/private_commit/warmup_samples/3` | `1974272` |
| `/private_commit/warmup_samples/4` | `3358720` |
| `/private_commit/warmup_samples/5` | `1974272` |
| `/private_commit/warmup_samples/6` | `3432448` |
| `/private_commit/warmup_samples/7` | `3547136` |
| `/private_commit/warmup_samples/8` | `1998848` |
| `/private_commit/warmup_samples/9` | `3264512` |
| `/private_commit/measured_samples/0` | `3665920` |
| `/private_commit/measured_samples/1` | `3633152` |
| `/private_commit/measured_samples/2` | `3567616` |
| `/private_commit/measured_samples/3` | `3563520` |
| `/private_commit/measured_samples/4` | `3256320` |
| `/private_commit/measured_samples/5` | `3559424` |
| `/private_commit/measured_samples/6` | `3592192` |
| `/private_commit/measured_samples/7` | `3551232` |
| `/private_commit/measured_samples/8` | `3588096` |
| `/private_commit/measured_samples/9` | `3129344` |
| `/working_set/available` | `true` |
| `/working_set/stable` | `true` |
| `/working_set/terminal_growth` | `false` |
| `/working_set/assessment_status` | `"pass"` |
| `/working_set/gate_status` | `"diagnostic"` |
| `/working_set/gate_applied` | `false` |
| `/working_set/budget_qualified` | `false` |
| `/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/working_set/baseline_last_bytes` | `5406720` |
| `/working_set/baseline_full_min_bytes` | `5185536` |
| `/working_set/baseline_central_min_bytes` | `5201920` |
| `/working_set/baseline_median_bytes` | `5386240` |
| `/working_set/baseline_central_max_bytes` | `5406720` |
| `/working_set/baseline_full_max_bytes` | `5410816` |
| `/working_set/final_last_bytes` | `5414912` |
| `/working_set/final_full_min_bytes` | `5414912` |
| `/working_set/final_central_min_bytes` | `5419008` |
| `/working_set/final_median_bytes` | `5423104` |
| `/working_set/final_central_max_bytes` | `5431296` |
| `/working_set/final_full_max_bytes` | `5435392` |
| `/working_set/peak_bytes` | `6701056` |
| `/working_set/growth_ratio` | `0.006844106463878327` |
| `/working_set/instant_growth_ratio` | `0.0015151515151515152` |
| `/working_set/warmup_trimmed_span_ratio` | `0.03802281368821293` |
| `/working_set/warmup_full_span_ratio` | `0.04182509505703422` |
| `/working_set/measured_trimmed_span_ratio` | `0.0022658610271903325` |
| `/working_set/measured_full_span_ratio` | `0.0037764350453172208` |
| `/working_set/warmup_samples/0` | `5193728` |
| `/working_set/warmup_samples/1` | `5214208` |
| `/working_set/warmup_samples/2` | `5193728` |
| `/working_set/warmup_samples/3` | `5185536` |
| `/working_set/warmup_samples/4` | `5414912` |
| `/working_set/warmup_samples/5` | `5185536` |
| `/working_set/warmup_samples/6` | `5410816` |
| `/working_set/warmup_samples/7` | `5386240` |
| `/working_set/warmup_samples/8` | `5201920` |
| `/working_set/warmup_samples/9` | `5406720` |
| `/working_set/measured_samples/0` | `5410816` |
| `/working_set/measured_samples/1` | `5414912` |
| `/working_set/measured_samples/2` | `5427200` |
| `/working_set/measured_samples/3` | `5410816` |
| `/working_set/measured_samples/4` | `5414912` |
| `/working_set/measured_samples/5` | `5419008` |
| `/working_set/measured_samples/6` | `5431296` |
| `/working_set/measured_samples/7` | `5423104` |
| `/working_set/measured_samples/8` | `5435392` |
| `/working_set/measured_samples/9` | `5414912` |

## Godot Smoke

Gate counts are recorded in this generated report. The paths name volatile local outputs ignored by Git; they may be absent after cleanup or in a clean checkout. Non-recorded wall-clock durations are intentionally omitted.

| Verification | Debug | Release | Volatile local outputs |
| --- | --- | --- | --- |
| Project gate | `24/24` | `24/24` | `build/debug/Testing/Temporary/LastTest.log`; `build/release/Testing/Temporary/LastTest.log` |
| Upstream Box3D | `20/20` | `20/20` | `artifacts/physics/upstream-box3d-debug.log`; `artifacts/physics/upstream-box3d-release.log` |
| Godot headless API | `1/1`, exit `0` | `1/1`, exit `0` | `artifacts/physics/godot-smoke-debug.stdout.log`, `artifacts/physics/godot-smoke-debug.stderr.log`; `artifacts/physics/godot-smoke-release.stdout.log`, `artifacts/physics/godot-smoke-release.stderr.log` |
| Godot renderers | `2/2` (Vulkan/OpenGL) | `2/2` (Vulkan/OpenGL) | `artifacts/physics/godot-scene-debug.stdout.log`, `artifacts/physics/godot-scene-debug.stderr.log`, `artifacts/physics/godot-scene-gl-debug.stdout.log`, `artifacts/physics/godot-scene-gl-debug.stderr.log`; `artifacts/physics/godot-scene-release.stdout.log`, `artifacts/physics/godot-scene-release.stderr.log`, `artifacts/physics/godot-scene-gl-release.stdout.log`, `artifacts/physics/godot-scene-gl-release.stderr.log` |

The following rows are the recorded graphical gate contract, not metadata retained from volatile AVI files. Every gate run removes the prior target, requires a fresh frame-300 marker, and verifies the resulting movie with `ffprobe`.

| Build | Renderer | Volatile local movie path | Gate contract (verified every run) |
| --- | --- | --- | --- |
| Debug | Vulkan Forward Mobile | `artifacts/physics/godot-scene-debug.avi` | `MJPEG; 1280x720; 300 frames; 5 s; freshness verified; frame-300 marker verified` |
| Debug | OpenGL Compatibility | `artifacts/physics/godot-scene-gl-debug.avi` | `MJPEG; 1280x720; 300 frames; 5 s; freshness verified; frame-300 marker verified` |
| Release | Vulkan Forward Mobile | `artifacts/physics/godot-scene-release.avi` | `MJPEG; 1280x720; 300 frames; 5 s; freshness verified; frame-300 marker verified` |
| Release | OpenGL Compatibility | `artifacts/physics/godot-scene-gl-release.avi` | `MJPEG; 1280x720; 300 frames; 5 s; freshness verified; frame-300 marker verified` |

## Known Limits

- Box3D 3D `v0.1.0` remains alpha software.
- PrivateUsage measures private commit and Working Set measures residency; neither alone proves ownership.
- The future 5% budget and 8 ms p95 target require a packaged Godot Release build on reference hardware.
- The deterministic runtime manifest remains the valid Godot gate when the initial headless editor scan fails.
- Windows x86_64 is the only qualified foundation target.
- The determinism gate guarantees repeatability only within one executable and build configuration produced by the pinned MSVC x64 toolchain. Matching Debug/Release hashes in the captured snapshots are observed evidence, not a requirement, and do not guarantee identical results across build configurations, machines, CPU models, MSVC/toolset versions, compiler families, or architectures.
- Canonical state and fixture hashes are regression oracles for that qualified environment; they are not portable serialization, network-consensus, or cross-platform replay contracts.
- `build/` and `artifacts/physics/` contain volatile local outputs ignored by Git; an empty or absent directory is expected after cleanup and in a clean checkout. Only the two Evidence paths above are normative.

## Recommendation

Both snapshots have no normative violations. The private commit budget remains explicitly deferred and reported by `private_commit_budget_unqualified`.

Recommendation: prosseguir_com_limites
