# Box3D Foundation Spike Report

This normative report is generated only from the two tracked foundation snapshots. Run `python tools/generate_foundation_report.py --check` to detect drift.

Evidence-Debug-Path: docs/physics/evidence/foundation-report-debug.json
Evidence-Debug-SHA256: AD964036AE996E56EDBA14FE63A6CCE452309C1F3B66A850F754C49FD4F14205
Matrix-Debug-Hash: 17104053157009575930
Matrix-Debug-Topology: 123/123/1;121/221
Recommendation-Debug: prosseguir_com_limites
Evidence-Release-Path: docs/physics/evidence/foundation-report-release.json
Evidence-Release-SHA256: 408F63073A485A3096224B47F128E3A6D956BF96F26015231E0E29EBF72B46AA
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
| `private_commit_baseline_bytes` | `4812800` | `"bytes"` |
| `private_commit_final_bytes` | `4812800` | `"bytes"` |
| `working_set_baseline_bytes` | `7663616` | `"bytes"` |
| `working_set_final_bytes` | `7663616` | `"bytes"` |
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
| `private_commit_baseline_bytes` | `2498560` | `"bytes"` |
| `private_commit_final_bytes` | `2498560` | `"bytes"` |
| `working_set_baseline_bytes` | `5427200` | `"bytes"` |
| `working_set_final_bytes` | `5431296` | `"bytes"` |
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
| `/step_ms/p95` | `0.0604` |
| `/step_ms/max` | `0.4984` |
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
| `/step_ms/min` | `4.3358` |
| `/step_ms/p50` | `4.4865` |
| `/step_ms/p95` | `15.2908` |
| `/step_ms/max` | `15.2908` |
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
| `/step_ms/min` | `0.0264` |
| `/step_ms/p50` | `0.0291` |
| `/step_ms/p95` | `0.0626` |
| `/step_ms/max` | `17.1655` |
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
| `/step_ms/min` | `0.0259` |
| `/step_ms/p50` | `0.0373` |
| `/step_ms/p95` | `0.1135` |
| `/step_ms/max` | `23.0204` |
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
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `false` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `5935104` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `4636672` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `5058560` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `5935104` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `6029312` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `6266880` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `6402048` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `4636672` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `6205440` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `6299648` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `6402048` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `6938624` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `6938624` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0.06142167011732229` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0.07867494824016563` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0.16356107660455488` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0.274672187715666` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0.031209362808842653` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0.36540962288686607` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/0` | `5324800` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/1` | `5578752` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/2` | `5009408` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/3` | `4825088` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/4` | `6189056` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/5` | `5058560` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/6` | `6266880` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/7` | `6029312` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/8` | `4636672` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/9` | `5935104` |
| `/repeat_observations/0/memory/private_commit/measured_samples/0` | `5963776` |
| `/repeat_observations/0/memory/private_commit/measured_samples/1` | `6082560` |
| `/repeat_observations/0/memory/private_commit/measured_samples/2` | `6397952` |
| `/repeat_observations/0/memory/private_commit/measured_samples/3` | `6119424` |
| `/repeat_observations/0/memory/private_commit/measured_samples/4` | `5976064` |
| `/repeat_observations/0/memory/private_commit/measured_samples/5` | `4636672` |
| `/repeat_observations/0/memory/private_commit/measured_samples/6` | `6299648` |
| `/repeat_observations/0/memory/private_commit/measured_samples/7` | `6938624` |
| `/repeat_observations/0/memory/private_commit/measured_samples/8` | `6205440` |
| `/repeat_observations/0/memory/private_commit/measured_samples/9` | `6402048` |
| `/repeat_observations/0/memory/working_set/available` | `true` |
| `/repeat_observations/0/memory/working_set/stable` | `false` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"unstable"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `8286208` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `7389184` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `7397376` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `8282112` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `8286208` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `8306688` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `8409088` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `7397376` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `8323072` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `8396800` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `8400896` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `8409088` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `10047488` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0.013847675568743818` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0.014829461196243203` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0.10731948565776459` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0.11078140454995054` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0.009268292682926829` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0.12048780487804878` |
| `/repeat_observations/0/memory/working_set/warmup_samples/0` | `7680000` |
| `/repeat_observations/0/memory/working_set/warmup_samples/1` | `7413760` |
| `/repeat_observations/0/memory/working_set/warmup_samples/2` | `7438336` |
| `/repeat_observations/0/memory/working_set/warmup_samples/3` | `7421952` |
| `/repeat_observations/0/memory/working_set/warmup_samples/4` | `8306688` |
| `/repeat_observations/0/memory/working_set/warmup_samples/5` | `7397376` |
| `/repeat_observations/0/memory/working_set/warmup_samples/6` | `8306688` |
| `/repeat_observations/0/memory/working_set/warmup_samples/7` | `8282112` |
| `/repeat_observations/0/memory/working_set/warmup_samples/8` | `7389184` |
| `/repeat_observations/0/memory/working_set/warmup_samples/9` | `8286208` |
| `/repeat_observations/0/memory/working_set/measured_samples/0` | `8310784` |
| `/repeat_observations/0/memory/working_set/measured_samples/1` | `8323072` |
| `/repeat_observations/0/memory/working_set/measured_samples/2` | `8302592` |
| `/repeat_observations/0/memory/working_set/measured_samples/3` | `8339456` |
| `/repeat_observations/0/memory/working_set/measured_samples/4` | `8306688` |
| `/repeat_observations/0/memory/working_set/measured_samples/5` | `7397376` |
| `/repeat_observations/0/memory/working_set/measured_samples/6` | `8323072` |
| `/repeat_observations/0/memory/working_set/measured_samples/7` | `8400896` |
| `/repeat_observations/0/memory/working_set/measured_samples/8` | `8396800` |
| `/repeat_observations/0/memory/working_set/measured_samples/9` | `8409088` |
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
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `5165056` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `4734976` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `5165056` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `6537216` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `6549504` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `6647808` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `6537216` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `6287360` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `6344704` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `6520832` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `6537216` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `7094272` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `7094272` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0.2656621728786677` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0.21177944862155387` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0.2926065162907268` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0.029522613065326633` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0.12374371859296482` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/0` | `6316032` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/1` | `6201344` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/2` | `6115328` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/3` | `6262784` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/4` | `6545408` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/5` | `6549504` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/6` | `6537216` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/7` | `6647808` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/8` | `4734976` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/9` | `5165056` |
| `/repeat_observations/1/memory/private_commit/measured_samples/0` | `6283264` |
| `/repeat_observations/1/memory/private_commit/measured_samples/1` | `6746112` |
| `/repeat_observations/1/memory/private_commit/measured_samples/2` | `6365184` |
| `/repeat_observations/1/memory/private_commit/measured_samples/3` | `6721536` |
| `/repeat_observations/1/memory/private_commit/measured_samples/4` | `6590464` |
| `/repeat_observations/1/memory/private_commit/measured_samples/5` | `6520832` |
| `/repeat_observations/1/memory/private_commit/measured_samples/6` | `6344704` |
| `/repeat_observations/1/memory/private_commit/measured_samples/7` | `7094272` |
| `/repeat_observations/1/memory/private_commit/measured_samples/8` | `6287360` |
| `/repeat_observations/1/memory/private_commit/measured_samples/9` | `6537216` |
| `/repeat_observations/1/memory/working_set/available` | `true` |
| `/repeat_observations/1/memory/working_set/stable` | `false` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"unstable"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `7524352` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `7458816` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `7524352` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `8413184` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `8421376` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `8421376` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `8548352` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `8409088` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `8425472` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `8433664` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `8544256` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `8548352` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `10047488` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0.0024342745861733205` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0.1360914534567229` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0.10662122687439143` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0.11441090555014606` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0.014084507042253521` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0.01651287032540068` |
| `/repeat_observations/1/memory/working_set/warmup_samples/0` | `8388608` |
| `/repeat_observations/1/memory/working_set/warmup_samples/1` | `8413184` |
| `/repeat_observations/1/memory/working_set/warmup_samples/2` | `8400896` |
| `/repeat_observations/1/memory/working_set/warmup_samples/3` | `8404992` |
| `/repeat_observations/1/memory/working_set/warmup_samples/4` | `8404992` |
| `/repeat_observations/1/memory/working_set/warmup_samples/5` | `8421376` |
| `/repeat_observations/1/memory/working_set/warmup_samples/6` | `8421376` |
| `/repeat_observations/1/memory/working_set/warmup_samples/7` | `8413184` |
| `/repeat_observations/1/memory/working_set/warmup_samples/8` | `7458816` |
| `/repeat_observations/1/memory/working_set/warmup_samples/9` | `7524352` |
| `/repeat_observations/1/memory/working_set/measured_samples/0` | `8437760` |
| `/repeat_observations/1/memory/working_set/measured_samples/1` | `8421376` |
| `/repeat_observations/1/memory/working_set/measured_samples/2` | `8421376` |
| `/repeat_observations/1/memory/working_set/measured_samples/3` | `8421376` |
| `/repeat_observations/1/memory/working_set/measured_samples/4` | `8548352` |
| `/repeat_observations/1/memory/working_set/measured_samples/5` | `8425472` |
| `/repeat_observations/1/memory/working_set/measured_samples/6` | `8409088` |
| `/repeat_observations/1/memory/working_set/measured_samples/7` | `8544256` |
| `/repeat_observations/1/memory/working_set/measured_samples/8` | `8433664` |
| `/repeat_observations/1/memory/working_set/measured_samples/9` | `8548352` |
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
| `/step_ms/min` | `0.4305` |
| `/step_ms/p50` | `0.5145` |
| `/step_ms/p95` | `1.3692` |
| `/step_ms/max` | `2.8128` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unstable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `true` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `false` |
| `/memory/private_commit/baseline_last_bytes` | `5935104` |
| `/memory/private_commit/baseline_full_min_bytes` | `4636672` |
| `/memory/private_commit/baseline_central_min_bytes` | `5058560` |
| `/memory/private_commit/baseline_median_bytes` | `5935104` |
| `/memory/private_commit/baseline_central_max_bytes` | `6029312` |
| `/memory/private_commit/baseline_full_max_bytes` | `6266880` |
| `/memory/private_commit/final_last_bytes` | `6402048` |
| `/memory/private_commit/final_full_min_bytes` | `4636672` |
| `/memory/private_commit/final_central_min_bytes` | `6205440` |
| `/memory/private_commit/final_median_bytes` | `6299648` |
| `/memory/private_commit/final_central_max_bytes` | `6402048` |
| `/memory/private_commit/final_full_max_bytes` | `6938624` |
| `/memory/private_commit/peak_bytes` | `6938624` |
| `/memory/private_commit/growth_ratio` | `0.06142167011732229` |
| `/memory/private_commit/instant_growth_ratio` | `0.07867494824016563` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0.16356107660455488` |
| `/memory/private_commit/warmup_full_span_ratio` | `0.274672187715666` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0.031209362808842653` |
| `/memory/private_commit/measured_full_span_ratio` | `0.36540962288686607` |
| `/memory/private_commit/warmup_samples/0` | `5324800` |
| `/memory/private_commit/warmup_samples/1` | `5578752` |
| `/memory/private_commit/warmup_samples/2` | `5009408` |
| `/memory/private_commit/warmup_samples/3` | `4825088` |
| `/memory/private_commit/warmup_samples/4` | `6189056` |
| `/memory/private_commit/warmup_samples/5` | `5058560` |
| `/memory/private_commit/warmup_samples/6` | `6266880` |
| `/memory/private_commit/warmup_samples/7` | `6029312` |
| `/memory/private_commit/warmup_samples/8` | `4636672` |
| `/memory/private_commit/warmup_samples/9` | `5935104` |
| `/memory/private_commit/measured_samples/0` | `5963776` |
| `/memory/private_commit/measured_samples/1` | `6082560` |
| `/memory/private_commit/measured_samples/2` | `6397952` |
| `/memory/private_commit/measured_samples/3` | `6119424` |
| `/memory/private_commit/measured_samples/4` | `5976064` |
| `/memory/private_commit/measured_samples/5` | `4636672` |
| `/memory/private_commit/measured_samples/6` | `6299648` |
| `/memory/private_commit/measured_samples/7` | `6938624` |
| `/memory/private_commit/measured_samples/8` | `6205440` |
| `/memory/private_commit/measured_samples/9` | `6402048` |
| `/memory/working_set/available` | `true` |
| `/memory/working_set/stable` | `false` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"unstable"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `8286208` |
| `/memory/working_set/baseline_full_min_bytes` | `7389184` |
| `/memory/working_set/baseline_central_min_bytes` | `7397376` |
| `/memory/working_set/baseline_median_bytes` | `8282112` |
| `/memory/working_set/baseline_central_max_bytes` | `8286208` |
| `/memory/working_set/baseline_full_max_bytes` | `8306688` |
| `/memory/working_set/final_last_bytes` | `8409088` |
| `/memory/working_set/final_full_min_bytes` | `7397376` |
| `/memory/working_set/final_central_min_bytes` | `8323072` |
| `/memory/working_set/final_median_bytes` | `8396800` |
| `/memory/working_set/final_central_max_bytes` | `8400896` |
| `/memory/working_set/final_full_max_bytes` | `8409088` |
| `/memory/working_set/peak_bytes` | `10047488` |
| `/memory/working_set/growth_ratio` | `0.013847675568743818` |
| `/memory/working_set/instant_growth_ratio` | `0.014829461196243203` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0.10731948565776459` |
| `/memory/working_set/warmup_full_span_ratio` | `0.11078140454995054` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0.009268292682926829` |
| `/memory/working_set/measured_full_span_ratio` | `0.12048780487804878` |
| `/memory/working_set/warmup_samples/0` | `7680000` |
| `/memory/working_set/warmup_samples/1` | `7413760` |
| `/memory/working_set/warmup_samples/2` | `7438336` |
| `/memory/working_set/warmup_samples/3` | `7421952` |
| `/memory/working_set/warmup_samples/4` | `8306688` |
| `/memory/working_set/warmup_samples/5` | `7397376` |
| `/memory/working_set/warmup_samples/6` | `8306688` |
| `/memory/working_set/warmup_samples/7` | `8282112` |
| `/memory/working_set/warmup_samples/8` | `7389184` |
| `/memory/working_set/warmup_samples/9` | `8286208` |
| `/memory/working_set/measured_samples/0` | `8310784` |
| `/memory/working_set/measured_samples/1` | `8323072` |
| `/memory/working_set/measured_samples/2` | `8302592` |
| `/memory/working_set/measured_samples/3` | `8339456` |
| `/memory/working_set/measured_samples/4` | `8306688` |
| `/memory/working_set/measured_samples/5` | `7397376` |
| `/memory/working_set/measured_samples/6` | `8323072` |
| `/memory/working_set/measured_samples/7` | `8400896` |
| `/memory/working_set/measured_samples/8` | `8396800` |
| `/memory/working_set/measured_samples/9` | `8409088` |
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
| `/metrics/4/value` | `5935104` |
| `/metrics/4/unit` | `"bytes"` |
| `/metrics/5/name` | `"private_commit_final_median_bytes"` |
| `/metrics/5/value` | `6299648` |
| `/metrics/5/unit` | `"bytes"` |
| `/metrics/6/name` | `"private_commit_growth_ratio"` |
| `/metrics/6/value` | `0.06142167011732229` |
| `/metrics/6/unit` | `"ratio"` |
| `/metrics/7/name` | `"private_commit_instant_growth_ratio"` |
| `/metrics/7/value` | `0.07867494824016563` |
| `/metrics/7/unit` | `"ratio"` |
| `/metrics/8/name` | `"private_commit_warmup_trimmed_span_ratio"` |
| `/metrics/8/value` | `0.16356107660455488` |
| `/metrics/8/unit` | `"ratio"` |
| `/metrics/9/name` | `"private_commit_measured_trimmed_span_ratio"` |
| `/metrics/9/value` | `0.031209362808842653` |
| `/metrics/9/unit` | `"ratio"` |
| `/metrics/10/name` | `"working_set_baseline_bytes"` |
| `/metrics/10/value` | `8286208` |
| `/metrics/10/unit` | `"bytes"` |
| `/metrics/11/name` | `"working_set_baseline_low_bytes"` |
| `/metrics/11/value` | `7389184` |
| `/metrics/11/unit` | `"bytes"` |
| `/metrics/12/name` | `"working_set_peak_bytes"` |
| `/metrics/12/value` | `10047488` |
| `/metrics/12/unit` | `"bytes"` |
| `/metrics/13/name` | `"working_set_final_bytes"` |
| `/metrics/13/value` | `8409088` |
| `/metrics/13/unit` | `"bytes"` |
| `/metrics/14/name` | `"working_set_final_low_bytes"` |
| `/metrics/14/value` | `7397376` |
| `/metrics/14/unit` | `"bytes"` |
| `/metrics/15/name` | `"working_set_growth_ratio"` |
| `/metrics/15/value` | `0.013847675568743818` |
| `/metrics/15/unit` | `"ratio"` |
| `/metrics/16/name` | `"working_set_instant_growth_ratio"` |
| `/metrics/16/value` | `0.014829461196243203` |
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
| `/step_ms/min` | `0.026` |
| `/step_ms/p50` | `0.0387` |
| `/step_ms/p95` | `0.1172` |
| `/step_ms/max` | `25.9073` |
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
| `/matrix/6/values/4/value` | `4812800` |
| `/matrix/6/values/4/unit` | `"bytes"` |
| `/matrix/6/values/5/name` | `"private_commit_final_bytes"` |
| `/matrix/6/values/5/value` | `4812800` |
| `/matrix/6/values/5/unit` | `"bytes"` |
| `/matrix/6/values/6/name` | `"working_set_baseline_bytes"` |
| `/matrix/6/values/6/value` | `7663616` |
| `/matrix/6/values/6/unit` | `"bytes"` |
| `/matrix/6/values/7/name` | `"working_set_final_bytes"` |
| `/matrix/6/values/7/value` | `7663616` |
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
| `/step_ms/p95` | `0.0048` |
| `/step_ms/max` | `0.1457` |
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
| `/step_ms/min` | `0.3086` |
| `/step_ms/p50` | `0.3333` |
| `/step_ms/p95` | `1.8019` |
| `/step_ms/max` | `1.8019` |
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
| `/step_ms/min` | `0.0024` |
| `/step_ms/p50` | `0.0025` |
| `/step_ms/p95` | `0.0028` |
| `/step_ms/max` | `1.4151` |
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
| `/step_ms/min` | `0.0024` |
| `/step_ms/p50` | `0.0026` |
| `/step_ms/p95` | `0.0048` |
| `/step_ms/max` | `1.4543` |
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
| `/repeat_observations/0/memory/assessment_status` | `"unstable"` |
| `/repeat_observations/0/memory/gate_applied` | `false` |
| `/repeat_observations/0/memory/budget_qualified` | `false` |
| `/repeat_observations/0/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/private_commit/available` | `true` |
| `/repeat_observations/0/memory/private_commit/stable` | `false` |
| `/repeat_observations/0/memory/private_commit/terminal_growth` | `true` |
| `/repeat_observations/0/memory/private_commit/baseline_last_bytes` | `2396160` |
| `/repeat_observations/0/memory/private_commit/baseline_full_min_bytes` | `2285568` |
| `/repeat_observations/0/memory/private_commit/baseline_central_min_bytes` | `2396160` |
| `/repeat_observations/0/memory/private_commit/baseline_median_bytes` | `2461696` |
| `/repeat_observations/0/memory/private_commit/baseline_central_max_bytes` | `3444736` |
| `/repeat_observations/0/memory/private_commit/baseline_full_max_bytes` | `3514368` |
| `/repeat_observations/0/memory/private_commit/final_last_bytes` | `3940352` |
| `/repeat_observations/0/memory/private_commit/final_full_min_bytes` | `3395584` |
| `/repeat_observations/0/memory/private_commit/final_central_min_bytes` | `3411968` |
| `/repeat_observations/0/memory/private_commit/final_median_bytes` | `3452928` |
| `/repeat_observations/0/memory/private_commit/final_central_max_bytes` | `3940352` |
| `/repeat_observations/0/memory/private_commit/final_full_max_bytes` | `4427776` |
| `/repeat_observations/0/memory/private_commit/peak_bytes` | `4427776` |
| `/repeat_observations/0/memory/private_commit/growth_ratio` | `0.40266222961730447` |
| `/repeat_observations/0/memory/private_commit/instant_growth_ratio` | `0.6444444444444445` |
| `/repeat_observations/0/memory/private_commit/warmup_trimmed_span_ratio` | `0.4259567387687188` |
| `/repeat_observations/0/memory/private_commit/warmup_full_span_ratio` | `0.49916805324459235` |
| `/repeat_observations/0/memory/private_commit/measured_trimmed_span_ratio` | `0.15302491103202848` |
| `/repeat_observations/0/memory/private_commit/measured_full_span_ratio` | `0.298932384341637` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/0` | `1978368` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/1` | `1925120` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/2` | `2596864` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/3` | `2281472` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/4` | `2928640` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/5` | `3514368` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/6` | `3444736` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/7` | `2285568` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/8` | `2461696` |
| `/repeat_observations/0/memory/private_commit/warmup_samples/9` | `2396160` |
| `/repeat_observations/0/memory/private_commit/measured_samples/0` | `2379776` |
| `/repeat_observations/0/memory/private_commit/measured_samples/1` | `3739648` |
| `/repeat_observations/0/memory/private_commit/measured_samples/2` | `2363392` |
| `/repeat_observations/0/memory/private_commit/measured_samples/3` | `2281472` |
| `/repeat_observations/0/memory/private_commit/measured_samples/4` | `3768320` |
| `/repeat_observations/0/memory/private_commit/measured_samples/5` | `4427776` |
| `/repeat_observations/0/memory/private_commit/measured_samples/6` | `3395584` |
| `/repeat_observations/0/memory/private_commit/measured_samples/7` | `3411968` |
| `/repeat_observations/0/memory/private_commit/measured_samples/8` | `3452928` |
| `/repeat_observations/0/memory/private_commit/measured_samples/9` | `3940352` |
| `/repeat_observations/0/memory/working_set/available` | `true` |
| `/repeat_observations/0/memory/working_set/stable` | `true` |
| `/repeat_observations/0/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/0/memory/working_set/assessment_status` | `"pass"` |
| `/repeat_observations/0/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/0/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/0/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/0/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/0/memory/working_set/baseline_last_bytes` | `5529600` |
| `/repeat_observations/0/memory/working_set/baseline_full_min_bytes` | `5468160` |
| `/repeat_observations/0/memory/working_set/baseline_central_min_bytes` | `5500928` |
| `/repeat_observations/0/memory/working_set/baseline_median_bytes` | `5529600` |
| `/repeat_observations/0/memory/working_set/baseline_central_max_bytes` | `5668864` |
| `/repeat_observations/0/memory/working_set/baseline_full_max_bytes` | `5672960` |
| `/repeat_observations/0/memory/working_set/final_last_bytes` | `5787648` |
| `/repeat_observations/0/memory/working_set/final_full_min_bytes` | `5672960` |
| `/repeat_observations/0/memory/working_set/final_central_min_bytes` | `5689344` |
| `/repeat_observations/0/memory/working_set/final_median_bytes` | `5697536` |
| `/repeat_observations/0/memory/working_set/final_central_max_bytes` | `5787648` |
| `/repeat_observations/0/memory/working_set/final_full_max_bytes` | `5804032` |
| `/repeat_observations/0/memory/working_set/peak_bytes` | `7045120` |
| `/repeat_observations/0/memory/working_set/growth_ratio` | `0.03037037037037037` |
| `/repeat_observations/0/memory/working_set/instant_growth_ratio` | `0.04666666666666667` |
| `/repeat_observations/0/memory/working_set/warmup_trimmed_span_ratio` | `0.03037037037037037` |
| `/repeat_observations/0/memory/working_set/warmup_full_span_ratio` | `0.037037037037037035` |
| `/repeat_observations/0/memory/working_set/measured_trimmed_span_ratio` | `0.017253774263120056` |
| `/repeat_observations/0/memory/working_set/measured_full_span_ratio` | `0.023005032350826744` |
| `/repeat_observations/0/memory/working_set/warmup_samples/0` | `5283840` |
| `/repeat_observations/0/memory/working_set/warmup_samples/1` | `5267456` |
| `/repeat_observations/0/memory/working_set/warmup_samples/2` | `5369856` |
| `/repeat_observations/0/memory/working_set/warmup_samples/3` | `5402624` |
| `/repeat_observations/0/memory/working_set/warmup_samples/4` | `5660672` |
| `/repeat_observations/0/memory/working_set/warmup_samples/5` | `5668864` |
| `/repeat_observations/0/memory/working_set/warmup_samples/6` | `5672960` |
| `/repeat_observations/0/memory/working_set/warmup_samples/7` | `5468160` |
| `/repeat_observations/0/memory/working_set/warmup_samples/8` | `5500928` |
| `/repeat_observations/0/memory/working_set/warmup_samples/9` | `5529600` |
| `/repeat_observations/0/memory/working_set/measured_samples/0` | `5525504` |
| `/repeat_observations/0/memory/working_set/measured_samples/1` | `5681152` |
| `/repeat_observations/0/memory/working_set/measured_samples/2` | `5529600` |
| `/repeat_observations/0/memory/working_set/measured_samples/3` | `5468160` |
| `/repeat_observations/0/memory/working_set/measured_samples/4` | `5705728` |
| `/repeat_observations/0/memory/working_set/measured_samples/5` | `5804032` |
| `/repeat_observations/0/memory/working_set/measured_samples/6` | `5697536` |
| `/repeat_observations/0/memory/working_set/measured_samples/7` | `5672960` |
| `/repeat_observations/0/memory/working_set/measured_samples/8` | `5689344` |
| `/repeat_observations/0/memory/working_set/measured_samples/9` | `5787648` |
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
| `/repeat_observations/1/memory/private_commit/terminal_growth` | `true` |
| `/repeat_observations/1/memory/private_commit/baseline_last_bytes` | `4042752` |
| `/repeat_observations/1/memory/private_commit/baseline_full_min_bytes` | `2285568` |
| `/repeat_observations/1/memory/private_commit/baseline_central_min_bytes` | `2363392` |
| `/repeat_observations/1/memory/private_commit/baseline_median_bytes` | `3588096` |
| `/repeat_observations/1/memory/private_commit/baseline_central_max_bytes` | `3772416` |
| `/repeat_observations/1/memory/private_commit/baseline_full_max_bytes` | `4042752` |
| `/repeat_observations/1/memory/private_commit/final_last_bytes` | `4255744` |
| `/repeat_observations/1/memory/private_commit/final_full_min_bytes` | `3698688` |
| `/repeat_observations/1/memory/private_commit/final_central_min_bytes` | `4018176` |
| `/repeat_observations/1/memory/private_commit/final_median_bytes` | `4042752` |
| `/repeat_observations/1/memory/private_commit/final_central_max_bytes` | `4255744` |
| `/repeat_observations/1/memory/private_commit/final_full_max_bytes` | `4288512` |
| `/repeat_observations/1/memory/private_commit/peak_bytes` | `4288512` |
| `/repeat_observations/1/memory/private_commit/growth_ratio` | `0.1267123287671233` |
| `/repeat_observations/1/memory/private_commit/instant_growth_ratio` | `0.05268490374873354` |
| `/repeat_observations/1/memory/private_commit/warmup_trimmed_span_ratio` | `0.3926940639269406` |
| `/repeat_observations/1/memory/private_commit/warmup_full_span_ratio` | `0.4897260273972603` |
| `/repeat_observations/1/memory/private_commit/measured_trimmed_span_ratio` | `0.05876393110435663` |
| `/repeat_observations/1/memory/private_commit/measured_full_span_ratio` | `0.1458966565349544` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/0` | `3514368` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/1` | `3530752` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/2` | `3710976` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/3` | `2437120` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/4` | `3944448` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/5` | `2285568` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/6` | `2363392` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/7` | `3588096` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/8` | `3772416` |
| `/repeat_observations/1/memory/private_commit/warmup_samples/9` | `4042752` |
| `/repeat_observations/1/memory/private_commit/measured_samples/0` | `3428352` |
| `/repeat_observations/1/memory/private_commit/measured_samples/1` | `3850240` |
| `/repeat_observations/1/memory/private_commit/measured_samples/2` | `4128768` |
| `/repeat_observations/1/memory/private_commit/measured_samples/3` | `3940352` |
| `/repeat_observations/1/memory/private_commit/measured_samples/4` | `3981312` |
| `/repeat_observations/1/memory/private_commit/measured_samples/5` | `4018176` |
| `/repeat_observations/1/memory/private_commit/measured_samples/6` | `4042752` |
| `/repeat_observations/1/memory/private_commit/measured_samples/7` | `3698688` |
| `/repeat_observations/1/memory/private_commit/measured_samples/8` | `4288512` |
| `/repeat_observations/1/memory/private_commit/measured_samples/9` | `4255744` |
| `/repeat_observations/1/memory/working_set/available` | `true` |
| `/repeat_observations/1/memory/working_set/stable` | `true` |
| `/repeat_observations/1/memory/working_set/terminal_growth` | `false` |
| `/repeat_observations/1/memory/working_set/assessment_status` | `"pass"` |
| `/repeat_observations/1/memory/working_set/gate_status` | `"diagnostic"` |
| `/repeat_observations/1/memory/working_set/gate_applied` | `false` |
| `/repeat_observations/1/memory/working_set/budget_qualified` | `false` |
| `/repeat_observations/1/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/repeat_observations/1/memory/working_set/baseline_last_bytes` | `5804032` |
| `/repeat_observations/1/memory/working_set/baseline_full_min_bytes` | `5472256` |
| `/repeat_observations/1/memory/working_set/baseline_central_min_bytes` | `5513216` |
| `/repeat_observations/1/memory/working_set/baseline_median_bytes` | `5685248` |
| `/repeat_observations/1/memory/working_set/baseline_central_max_bytes` | `5689344` |
| `/repeat_observations/1/memory/working_set/baseline_full_max_bytes` | `5804032` |
| `/repeat_observations/1/memory/working_set/final_last_bytes` | `5894144` |
| `/repeat_observations/1/memory/working_set/final_full_min_bytes` | `5652480` |
| `/repeat_observations/1/memory/working_set/final_central_min_bytes` | `5734400` |
| `/repeat_observations/1/memory/working_set/final_median_bytes` | `5873664` |
| `/repeat_observations/1/memory/working_set/final_central_max_bytes` | `5873664` |
| `/repeat_observations/1/memory/working_set/final_full_max_bytes` | `5894144` |
| `/repeat_observations/1/memory/working_set/peak_bytes` | `7045120` |
| `/repeat_observations/1/memory/working_set/growth_ratio` | `0.03314121037463977` |
| `/repeat_observations/1/memory/working_set/instant_growth_ratio` | `0.015525758645024701` |
| `/repeat_observations/1/memory/working_set/warmup_trimmed_span_ratio` | `0.030979827089337175` |
| `/repeat_observations/1/memory/working_set/warmup_full_span_ratio` | `0.058357348703170026` |
| `/repeat_observations/1/memory/working_set/measured_trimmed_span_ratio` | `0.023709902370990237` |
| `/repeat_observations/1/memory/working_set/measured_full_span_ratio` | `0.041143654114365415` |
| `/repeat_observations/1/memory/working_set/warmup_samples/0` | `5693440` |
| `/repeat_observations/1/memory/working_set/warmup_samples/1` | `5664768` |
| `/repeat_observations/1/memory/working_set/warmup_samples/2` | `5681152` |
| `/repeat_observations/1/memory/working_set/warmup_samples/3` | `5517312` |
| `/repeat_observations/1/memory/working_set/warmup_samples/4` | `5795840` |
| `/repeat_observations/1/memory/working_set/warmup_samples/5` | `5472256` |
| `/repeat_observations/1/memory/working_set/warmup_samples/6` | `5513216` |
| `/repeat_observations/1/memory/working_set/warmup_samples/7` | `5685248` |
| `/repeat_observations/1/memory/working_set/warmup_samples/8` | `5689344` |
| `/repeat_observations/1/memory/working_set/warmup_samples/9` | `5804032` |
| `/repeat_observations/1/memory/working_set/measured_samples/0` | `5697536` |
| `/repeat_observations/1/memory/working_set/measured_samples/1` | `5832704` |
| `/repeat_observations/1/memory/working_set/measured_samples/2` | `5828608` |
| `/repeat_observations/1/memory/working_set/measured_samples/3` | `5808128` |
| `/repeat_observations/1/memory/working_set/measured_samples/4` | `5898240` |
| `/repeat_observations/1/memory/working_set/measured_samples/5` | `5652480` |
| `/repeat_observations/1/memory/working_set/measured_samples/6` | `5873664` |
| `/repeat_observations/1/memory/working_set/measured_samples/7` | `5734400` |
| `/repeat_observations/1/memory/working_set/measured_samples/8` | `5873664` |
| `/repeat_observations/1/memory/working_set/measured_samples/9` | `5894144` |
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
| `/step_ms/min` | `0.0442` |
| `/step_ms/p50` | `0.0466` |
| `/step_ms/p95` | `0.1087` |
| `/step_ms/max` | `0.4484` |
| `/memory/gate_scope` | `"release_mt"` |
| `/memory/gate_status` | `"diagnostic"` |
| `/memory/assessment_status` | `"unstable"` |
| `/memory/gate_applied` | `false` |
| `/memory/budget_qualified` | `false` |
| `/memory/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/private_commit/available` | `true` |
| `/memory/private_commit/stable` | `false` |
| `/memory/private_commit/terminal_growth` | `true` |
| `/memory/private_commit/baseline_last_bytes` | `2396160` |
| `/memory/private_commit/baseline_full_min_bytes` | `2285568` |
| `/memory/private_commit/baseline_central_min_bytes` | `2396160` |
| `/memory/private_commit/baseline_median_bytes` | `2461696` |
| `/memory/private_commit/baseline_central_max_bytes` | `3444736` |
| `/memory/private_commit/baseline_full_max_bytes` | `3514368` |
| `/memory/private_commit/final_last_bytes` | `3940352` |
| `/memory/private_commit/final_full_min_bytes` | `3395584` |
| `/memory/private_commit/final_central_min_bytes` | `3411968` |
| `/memory/private_commit/final_median_bytes` | `3452928` |
| `/memory/private_commit/final_central_max_bytes` | `3940352` |
| `/memory/private_commit/final_full_max_bytes` | `4427776` |
| `/memory/private_commit/peak_bytes` | `4427776` |
| `/memory/private_commit/growth_ratio` | `0.40266222961730447` |
| `/memory/private_commit/instant_growth_ratio` | `0.6444444444444445` |
| `/memory/private_commit/warmup_trimmed_span_ratio` | `0.4259567387687188` |
| `/memory/private_commit/warmup_full_span_ratio` | `0.49916805324459235` |
| `/memory/private_commit/measured_trimmed_span_ratio` | `0.15302491103202848` |
| `/memory/private_commit/measured_full_span_ratio` | `0.298932384341637` |
| `/memory/private_commit/warmup_samples/0` | `1978368` |
| `/memory/private_commit/warmup_samples/1` | `1925120` |
| `/memory/private_commit/warmup_samples/2` | `2596864` |
| `/memory/private_commit/warmup_samples/3` | `2281472` |
| `/memory/private_commit/warmup_samples/4` | `2928640` |
| `/memory/private_commit/warmup_samples/5` | `3514368` |
| `/memory/private_commit/warmup_samples/6` | `3444736` |
| `/memory/private_commit/warmup_samples/7` | `2285568` |
| `/memory/private_commit/warmup_samples/8` | `2461696` |
| `/memory/private_commit/warmup_samples/9` | `2396160` |
| `/memory/private_commit/measured_samples/0` | `2379776` |
| `/memory/private_commit/measured_samples/1` | `3739648` |
| `/memory/private_commit/measured_samples/2` | `2363392` |
| `/memory/private_commit/measured_samples/3` | `2281472` |
| `/memory/private_commit/measured_samples/4` | `3768320` |
| `/memory/private_commit/measured_samples/5` | `4427776` |
| `/memory/private_commit/measured_samples/6` | `3395584` |
| `/memory/private_commit/measured_samples/7` | `3411968` |
| `/memory/private_commit/measured_samples/8` | `3452928` |
| `/memory/private_commit/measured_samples/9` | `3940352` |
| `/memory/working_set/available` | `true` |
| `/memory/working_set/stable` | `true` |
| `/memory/working_set/terminal_growth` | `false` |
| `/memory/working_set/assessment_status` | `"pass"` |
| `/memory/working_set/gate_status` | `"diagnostic"` |
| `/memory/working_set/gate_applied` | `false` |
| `/memory/working_set/budget_qualified` | `false` |
| `/memory/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/memory/working_set/baseline_last_bytes` | `5529600` |
| `/memory/working_set/baseline_full_min_bytes` | `5468160` |
| `/memory/working_set/baseline_central_min_bytes` | `5500928` |
| `/memory/working_set/baseline_median_bytes` | `5529600` |
| `/memory/working_set/baseline_central_max_bytes` | `5668864` |
| `/memory/working_set/baseline_full_max_bytes` | `5672960` |
| `/memory/working_set/final_last_bytes` | `5787648` |
| `/memory/working_set/final_full_min_bytes` | `5672960` |
| `/memory/working_set/final_central_min_bytes` | `5689344` |
| `/memory/working_set/final_median_bytes` | `5697536` |
| `/memory/working_set/final_central_max_bytes` | `5787648` |
| `/memory/working_set/final_full_max_bytes` | `5804032` |
| `/memory/working_set/peak_bytes` | `7045120` |
| `/memory/working_set/growth_ratio` | `0.03037037037037037` |
| `/memory/working_set/instant_growth_ratio` | `0.04666666666666667` |
| `/memory/working_set/warmup_trimmed_span_ratio` | `0.03037037037037037` |
| `/memory/working_set/warmup_full_span_ratio` | `0.037037037037037035` |
| `/memory/working_set/measured_trimmed_span_ratio` | `0.017253774263120056` |
| `/memory/working_set/measured_full_span_ratio` | `0.023005032350826744` |
| `/memory/working_set/warmup_samples/0` | `5283840` |
| `/memory/working_set/warmup_samples/1` | `5267456` |
| `/memory/working_set/warmup_samples/2` | `5369856` |
| `/memory/working_set/warmup_samples/3` | `5402624` |
| `/memory/working_set/warmup_samples/4` | `5660672` |
| `/memory/working_set/warmup_samples/5` | `5668864` |
| `/memory/working_set/warmup_samples/6` | `5672960` |
| `/memory/working_set/warmup_samples/7` | `5468160` |
| `/memory/working_set/warmup_samples/8` | `5500928` |
| `/memory/working_set/warmup_samples/9` | `5529600` |
| `/memory/working_set/measured_samples/0` | `5525504` |
| `/memory/working_set/measured_samples/1` | `5681152` |
| `/memory/working_set/measured_samples/2` | `5529600` |
| `/memory/working_set/measured_samples/3` | `5468160` |
| `/memory/working_set/measured_samples/4` | `5705728` |
| `/memory/working_set/measured_samples/5` | `5804032` |
| `/memory/working_set/measured_samples/6` | `5697536` |
| `/memory/working_set/measured_samples/7` | `5672960` |
| `/memory/working_set/measured_samples/8` | `5689344` |
| `/memory/working_set/measured_samples/9` | `5787648` |
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
| `/metrics/4/value` | `2461696` |
| `/metrics/4/unit` | `"bytes"` |
| `/metrics/5/name` | `"private_commit_final_median_bytes"` |
| `/metrics/5/value` | `3452928` |
| `/metrics/5/unit` | `"bytes"` |
| `/metrics/6/name` | `"private_commit_growth_ratio"` |
| `/metrics/6/value` | `0.40266222961730447` |
| `/metrics/6/unit` | `"ratio"` |
| `/metrics/7/name` | `"private_commit_instant_growth_ratio"` |
| `/metrics/7/value` | `0.6444444444444445` |
| `/metrics/7/unit` | `"ratio"` |
| `/metrics/8/name` | `"private_commit_warmup_trimmed_span_ratio"` |
| `/metrics/8/value` | `0.4259567387687188` |
| `/metrics/8/unit` | `"ratio"` |
| `/metrics/9/name` | `"private_commit_measured_trimmed_span_ratio"` |
| `/metrics/9/value` | `0.15302491103202848` |
| `/metrics/9/unit` | `"ratio"` |
| `/metrics/10/name` | `"working_set_baseline_bytes"` |
| `/metrics/10/value` | `5529600` |
| `/metrics/10/unit` | `"bytes"` |
| `/metrics/11/name` | `"working_set_baseline_low_bytes"` |
| `/metrics/11/value` | `5468160` |
| `/metrics/11/unit` | `"bytes"` |
| `/metrics/12/name` | `"working_set_peak_bytes"` |
| `/metrics/12/value` | `7045120` |
| `/metrics/12/unit` | `"bytes"` |
| `/metrics/13/name` | `"working_set_final_bytes"` |
| `/metrics/13/value` | `5787648` |
| `/metrics/13/unit` | `"bytes"` |
| `/metrics/14/name` | `"working_set_final_low_bytes"` |
| `/metrics/14/value` | `5672960` |
| `/metrics/14/unit` | `"bytes"` |
| `/metrics/15/name` | `"working_set_growth_ratio"` |
| `/metrics/15/value` | `0.03037037037037037` |
| `/metrics/15/unit` | `"ratio"` |
| `/metrics/16/name` | `"working_set_instant_growth_ratio"` |
| `/metrics/16/value` | `0.04666666666666667` |
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
| `/step_ms/min` | `0.0023` |
| `/step_ms/p50` | `0.0028` |
| `/step_ms/p95` | `0.013` |
| `/step_ms/max` | `1.2772` |
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
| `/matrix/6/values/4/value` | `2498560` |
| `/matrix/6/values/4/unit` | `"bytes"` |
| `/matrix/6/values/5/name` | `"private_commit_final_bytes"` |
| `/matrix/6/values/5/value` | `2498560` |
| `/matrix/6/values/5/unit` | `"bytes"` |
| `/matrix/6/values/6/name` | `"working_set_baseline_bytes"` |
| `/matrix/6/values/6/value` | `5427200` |
| `/matrix/6/values/6/unit` | `"bytes"` |
| `/matrix/6/values/7/name` | `"working_set_final_bytes"` |
| `/matrix/6/values/7/value` | `5431296` |
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
| `/private_commit/terminal_growth` | `false` |
| `/private_commit/baseline_last_bytes` | `5935104` |
| `/private_commit/baseline_full_min_bytes` | `4636672` |
| `/private_commit/baseline_central_min_bytes` | `5058560` |
| `/private_commit/baseline_median_bytes` | `5935104` |
| `/private_commit/baseline_central_max_bytes` | `6029312` |
| `/private_commit/baseline_full_max_bytes` | `6266880` |
| `/private_commit/final_last_bytes` | `6402048` |
| `/private_commit/final_full_min_bytes` | `4636672` |
| `/private_commit/final_central_min_bytes` | `6205440` |
| `/private_commit/final_median_bytes` | `6299648` |
| `/private_commit/final_central_max_bytes` | `6402048` |
| `/private_commit/final_full_max_bytes` | `6938624` |
| `/private_commit/peak_bytes` | `6938624` |
| `/private_commit/growth_ratio` | `0.06142167011732229` |
| `/private_commit/instant_growth_ratio` | `0.07867494824016563` |
| `/private_commit/warmup_trimmed_span_ratio` | `0.16356107660455488` |
| `/private_commit/warmup_full_span_ratio` | `0.274672187715666` |
| `/private_commit/measured_trimmed_span_ratio` | `0.031209362808842653` |
| `/private_commit/measured_full_span_ratio` | `0.36540962288686607` |
| `/private_commit/warmup_samples/0` | `5324800` |
| `/private_commit/warmup_samples/1` | `5578752` |
| `/private_commit/warmup_samples/2` | `5009408` |
| `/private_commit/warmup_samples/3` | `4825088` |
| `/private_commit/warmup_samples/4` | `6189056` |
| `/private_commit/warmup_samples/5` | `5058560` |
| `/private_commit/warmup_samples/6` | `6266880` |
| `/private_commit/warmup_samples/7` | `6029312` |
| `/private_commit/warmup_samples/8` | `4636672` |
| `/private_commit/warmup_samples/9` | `5935104` |
| `/private_commit/measured_samples/0` | `5963776` |
| `/private_commit/measured_samples/1` | `6082560` |
| `/private_commit/measured_samples/2` | `6397952` |
| `/private_commit/measured_samples/3` | `6119424` |
| `/private_commit/measured_samples/4` | `5976064` |
| `/private_commit/measured_samples/5` | `4636672` |
| `/private_commit/measured_samples/6` | `6299648` |
| `/private_commit/measured_samples/7` | `6938624` |
| `/private_commit/measured_samples/8` | `6205440` |
| `/private_commit/measured_samples/9` | `6402048` |
| `/working_set/available` | `true` |
| `/working_set/stable` | `false` |
| `/working_set/terminal_growth` | `false` |
| `/working_set/assessment_status` | `"unstable"` |
| `/working_set/gate_status` | `"diagnostic"` |
| `/working_set/gate_applied` | `false` |
| `/working_set/budget_qualified` | `false` |
| `/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/working_set/baseline_last_bytes` | `8286208` |
| `/working_set/baseline_full_min_bytes` | `7389184` |
| `/working_set/baseline_central_min_bytes` | `7397376` |
| `/working_set/baseline_median_bytes` | `8282112` |
| `/working_set/baseline_central_max_bytes` | `8286208` |
| `/working_set/baseline_full_max_bytes` | `8306688` |
| `/working_set/final_last_bytes` | `8409088` |
| `/working_set/final_full_min_bytes` | `7397376` |
| `/working_set/final_central_min_bytes` | `8323072` |
| `/working_set/final_median_bytes` | `8396800` |
| `/working_set/final_central_max_bytes` | `8400896` |
| `/working_set/final_full_max_bytes` | `8409088` |
| `/working_set/peak_bytes` | `10047488` |
| `/working_set/growth_ratio` | `0.013847675568743818` |
| `/working_set/instant_growth_ratio` | `0.014829461196243203` |
| `/working_set/warmup_trimmed_span_ratio` | `0.10731948565776459` |
| `/working_set/warmup_full_span_ratio` | `0.11078140454995054` |
| `/working_set/measured_trimmed_span_ratio` | `0.009268292682926829` |
| `/working_set/measured_full_span_ratio` | `0.12048780487804878` |
| `/working_set/warmup_samples/0` | `7680000` |
| `/working_set/warmup_samples/1` | `7413760` |
| `/working_set/warmup_samples/2` | `7438336` |
| `/working_set/warmup_samples/3` | `7421952` |
| `/working_set/warmup_samples/4` | `8306688` |
| `/working_set/warmup_samples/5` | `7397376` |
| `/working_set/warmup_samples/6` | `8306688` |
| `/working_set/warmup_samples/7` | `8282112` |
| `/working_set/warmup_samples/8` | `7389184` |
| `/working_set/warmup_samples/9` | `8286208` |
| `/working_set/measured_samples/0` | `8310784` |
| `/working_set/measured_samples/1` | `8323072` |
| `/working_set/measured_samples/2` | `8302592` |
| `/working_set/measured_samples/3` | `8339456` |
| `/working_set/measured_samples/4` | `8306688` |
| `/working_set/measured_samples/5` | `7397376` |
| `/working_set/measured_samples/6` | `8323072` |
| `/working_set/measured_samples/7` | `8400896` |
| `/working_set/measured_samples/8` | `8396800` |
| `/working_set/measured_samples/9` | `8409088` |

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
| `/private_commit/baseline_last_bytes` | `5165056` |
| `/private_commit/baseline_full_min_bytes` | `4734976` |
| `/private_commit/baseline_central_min_bytes` | `5165056` |
| `/private_commit/baseline_median_bytes` | `6537216` |
| `/private_commit/baseline_central_max_bytes` | `6549504` |
| `/private_commit/baseline_full_max_bytes` | `6647808` |
| `/private_commit/final_last_bytes` | `6537216` |
| `/private_commit/final_full_min_bytes` | `6287360` |
| `/private_commit/final_central_min_bytes` | `6344704` |
| `/private_commit/final_median_bytes` | `6520832` |
| `/private_commit/final_central_max_bytes` | `6537216` |
| `/private_commit/final_full_max_bytes` | `7094272` |
| `/private_commit/peak_bytes` | `7094272` |
| `/private_commit/growth_ratio` | `0` |
| `/private_commit/instant_growth_ratio` | `0.2656621728786677` |
| `/private_commit/warmup_trimmed_span_ratio` | `0.21177944862155387` |
| `/private_commit/warmup_full_span_ratio` | `0.2926065162907268` |
| `/private_commit/measured_trimmed_span_ratio` | `0.029522613065326633` |
| `/private_commit/measured_full_span_ratio` | `0.12374371859296482` |
| `/private_commit/warmup_samples/0` | `6316032` |
| `/private_commit/warmup_samples/1` | `6201344` |
| `/private_commit/warmup_samples/2` | `6115328` |
| `/private_commit/warmup_samples/3` | `6262784` |
| `/private_commit/warmup_samples/4` | `6545408` |
| `/private_commit/warmup_samples/5` | `6549504` |
| `/private_commit/warmup_samples/6` | `6537216` |
| `/private_commit/warmup_samples/7` | `6647808` |
| `/private_commit/warmup_samples/8` | `4734976` |
| `/private_commit/warmup_samples/9` | `5165056` |
| `/private_commit/measured_samples/0` | `6283264` |
| `/private_commit/measured_samples/1` | `6746112` |
| `/private_commit/measured_samples/2` | `6365184` |
| `/private_commit/measured_samples/3` | `6721536` |
| `/private_commit/measured_samples/4` | `6590464` |
| `/private_commit/measured_samples/5` | `6520832` |
| `/private_commit/measured_samples/6` | `6344704` |
| `/private_commit/measured_samples/7` | `7094272` |
| `/private_commit/measured_samples/8` | `6287360` |
| `/private_commit/measured_samples/9` | `6537216` |
| `/working_set/available` | `true` |
| `/working_set/stable` | `false` |
| `/working_set/terminal_growth` | `false` |
| `/working_set/assessment_status` | `"unstable"` |
| `/working_set/gate_status` | `"diagnostic"` |
| `/working_set/gate_applied` | `false` |
| `/working_set/budget_qualified` | `false` |
| `/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/working_set/baseline_last_bytes` | `7524352` |
| `/working_set/baseline_full_min_bytes` | `7458816` |
| `/working_set/baseline_central_min_bytes` | `7524352` |
| `/working_set/baseline_median_bytes` | `8413184` |
| `/working_set/baseline_central_max_bytes` | `8421376` |
| `/working_set/baseline_full_max_bytes` | `8421376` |
| `/working_set/final_last_bytes` | `8548352` |
| `/working_set/final_full_min_bytes` | `8409088` |
| `/working_set/final_central_min_bytes` | `8425472` |
| `/working_set/final_median_bytes` | `8433664` |
| `/working_set/final_central_max_bytes` | `8544256` |
| `/working_set/final_full_max_bytes` | `8548352` |
| `/working_set/peak_bytes` | `10047488` |
| `/working_set/growth_ratio` | `0.0024342745861733205` |
| `/working_set/instant_growth_ratio` | `0.1360914534567229` |
| `/working_set/warmup_trimmed_span_ratio` | `0.10662122687439143` |
| `/working_set/warmup_full_span_ratio` | `0.11441090555014606` |
| `/working_set/measured_trimmed_span_ratio` | `0.014084507042253521` |
| `/working_set/measured_full_span_ratio` | `0.01651287032540068` |
| `/working_set/warmup_samples/0` | `8388608` |
| `/working_set/warmup_samples/1` | `8413184` |
| `/working_set/warmup_samples/2` | `8400896` |
| `/working_set/warmup_samples/3` | `8404992` |
| `/working_set/warmup_samples/4` | `8404992` |
| `/working_set/warmup_samples/5` | `8421376` |
| `/working_set/warmup_samples/6` | `8421376` |
| `/working_set/warmup_samples/7` | `8413184` |
| `/working_set/warmup_samples/8` | `7458816` |
| `/working_set/warmup_samples/9` | `7524352` |
| `/working_set/measured_samples/0` | `8437760` |
| `/working_set/measured_samples/1` | `8421376` |
| `/working_set/measured_samples/2` | `8421376` |
| `/working_set/measured_samples/3` | `8421376` |
| `/working_set/measured_samples/4` | `8548352` |
| `/working_set/measured_samples/5` | `8425472` |
| `/working_set/measured_samples/6` | `8409088` |
| `/working_set/measured_samples/7` | `8544256` |
| `/working_set/measured_samples/8` | `8433664` |
| `/working_set/measured_samples/9` | `8548352` |

### Release stress repeat 1

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
| `/private_commit/baseline_last_bytes` | `2396160` |
| `/private_commit/baseline_full_min_bytes` | `2285568` |
| `/private_commit/baseline_central_min_bytes` | `2396160` |
| `/private_commit/baseline_median_bytes` | `2461696` |
| `/private_commit/baseline_central_max_bytes` | `3444736` |
| `/private_commit/baseline_full_max_bytes` | `3514368` |
| `/private_commit/final_last_bytes` | `3940352` |
| `/private_commit/final_full_min_bytes` | `3395584` |
| `/private_commit/final_central_min_bytes` | `3411968` |
| `/private_commit/final_median_bytes` | `3452928` |
| `/private_commit/final_central_max_bytes` | `3940352` |
| `/private_commit/final_full_max_bytes` | `4427776` |
| `/private_commit/peak_bytes` | `4427776` |
| `/private_commit/growth_ratio` | `0.40266222961730447` |
| `/private_commit/instant_growth_ratio` | `0.6444444444444445` |
| `/private_commit/warmup_trimmed_span_ratio` | `0.4259567387687188` |
| `/private_commit/warmup_full_span_ratio` | `0.49916805324459235` |
| `/private_commit/measured_trimmed_span_ratio` | `0.15302491103202848` |
| `/private_commit/measured_full_span_ratio` | `0.298932384341637` |
| `/private_commit/warmup_samples/0` | `1978368` |
| `/private_commit/warmup_samples/1` | `1925120` |
| `/private_commit/warmup_samples/2` | `2596864` |
| `/private_commit/warmup_samples/3` | `2281472` |
| `/private_commit/warmup_samples/4` | `2928640` |
| `/private_commit/warmup_samples/5` | `3514368` |
| `/private_commit/warmup_samples/6` | `3444736` |
| `/private_commit/warmup_samples/7` | `2285568` |
| `/private_commit/warmup_samples/8` | `2461696` |
| `/private_commit/warmup_samples/9` | `2396160` |
| `/private_commit/measured_samples/0` | `2379776` |
| `/private_commit/measured_samples/1` | `3739648` |
| `/private_commit/measured_samples/2` | `2363392` |
| `/private_commit/measured_samples/3` | `2281472` |
| `/private_commit/measured_samples/4` | `3768320` |
| `/private_commit/measured_samples/5` | `4427776` |
| `/private_commit/measured_samples/6` | `3395584` |
| `/private_commit/measured_samples/7` | `3411968` |
| `/private_commit/measured_samples/8` | `3452928` |
| `/private_commit/measured_samples/9` | `3940352` |
| `/working_set/available` | `true` |
| `/working_set/stable` | `true` |
| `/working_set/terminal_growth` | `false` |
| `/working_set/assessment_status` | `"pass"` |
| `/working_set/gate_status` | `"diagnostic"` |
| `/working_set/gate_applied` | `false` |
| `/working_set/budget_qualified` | `false` |
| `/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/working_set/baseline_last_bytes` | `5529600` |
| `/working_set/baseline_full_min_bytes` | `5468160` |
| `/working_set/baseline_central_min_bytes` | `5500928` |
| `/working_set/baseline_median_bytes` | `5529600` |
| `/working_set/baseline_central_max_bytes` | `5668864` |
| `/working_set/baseline_full_max_bytes` | `5672960` |
| `/working_set/final_last_bytes` | `5787648` |
| `/working_set/final_full_min_bytes` | `5672960` |
| `/working_set/final_central_min_bytes` | `5689344` |
| `/working_set/final_median_bytes` | `5697536` |
| `/working_set/final_central_max_bytes` | `5787648` |
| `/working_set/final_full_max_bytes` | `5804032` |
| `/working_set/peak_bytes` | `7045120` |
| `/working_set/growth_ratio` | `0.03037037037037037` |
| `/working_set/instant_growth_ratio` | `0.04666666666666667` |
| `/working_set/warmup_trimmed_span_ratio` | `0.03037037037037037` |
| `/working_set/warmup_full_span_ratio` | `0.037037037037037035` |
| `/working_set/measured_trimmed_span_ratio` | `0.017253774263120056` |
| `/working_set/measured_full_span_ratio` | `0.023005032350826744` |
| `/working_set/warmup_samples/0` | `5283840` |
| `/working_set/warmup_samples/1` | `5267456` |
| `/working_set/warmup_samples/2` | `5369856` |
| `/working_set/warmup_samples/3` | `5402624` |
| `/working_set/warmup_samples/4` | `5660672` |
| `/working_set/warmup_samples/5` | `5668864` |
| `/working_set/warmup_samples/6` | `5672960` |
| `/working_set/warmup_samples/7` | `5468160` |
| `/working_set/warmup_samples/8` | `5500928` |
| `/working_set/warmup_samples/9` | `5529600` |
| `/working_set/measured_samples/0` | `5525504` |
| `/working_set/measured_samples/1` | `5681152` |
| `/working_set/measured_samples/2` | `5529600` |
| `/working_set/measured_samples/3` | `5468160` |
| `/working_set/measured_samples/4` | `5705728` |
| `/working_set/measured_samples/5` | `5804032` |
| `/working_set/measured_samples/6` | `5697536` |
| `/working_set/measured_samples/7` | `5672960` |
| `/working_set/measured_samples/8` | `5689344` |
| `/working_set/measured_samples/9` | `5787648` |

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
| `/private_commit/terminal_growth` | `true` |
| `/private_commit/baseline_last_bytes` | `4042752` |
| `/private_commit/baseline_full_min_bytes` | `2285568` |
| `/private_commit/baseline_central_min_bytes` | `2363392` |
| `/private_commit/baseline_median_bytes` | `3588096` |
| `/private_commit/baseline_central_max_bytes` | `3772416` |
| `/private_commit/baseline_full_max_bytes` | `4042752` |
| `/private_commit/final_last_bytes` | `4255744` |
| `/private_commit/final_full_min_bytes` | `3698688` |
| `/private_commit/final_central_min_bytes` | `4018176` |
| `/private_commit/final_median_bytes` | `4042752` |
| `/private_commit/final_central_max_bytes` | `4255744` |
| `/private_commit/final_full_max_bytes` | `4288512` |
| `/private_commit/peak_bytes` | `4288512` |
| `/private_commit/growth_ratio` | `0.1267123287671233` |
| `/private_commit/instant_growth_ratio` | `0.05268490374873354` |
| `/private_commit/warmup_trimmed_span_ratio` | `0.3926940639269406` |
| `/private_commit/warmup_full_span_ratio` | `0.4897260273972603` |
| `/private_commit/measured_trimmed_span_ratio` | `0.05876393110435663` |
| `/private_commit/measured_full_span_ratio` | `0.1458966565349544` |
| `/private_commit/warmup_samples/0` | `3514368` |
| `/private_commit/warmup_samples/1` | `3530752` |
| `/private_commit/warmup_samples/2` | `3710976` |
| `/private_commit/warmup_samples/3` | `2437120` |
| `/private_commit/warmup_samples/4` | `3944448` |
| `/private_commit/warmup_samples/5` | `2285568` |
| `/private_commit/warmup_samples/6` | `2363392` |
| `/private_commit/warmup_samples/7` | `3588096` |
| `/private_commit/warmup_samples/8` | `3772416` |
| `/private_commit/warmup_samples/9` | `4042752` |
| `/private_commit/measured_samples/0` | `3428352` |
| `/private_commit/measured_samples/1` | `3850240` |
| `/private_commit/measured_samples/2` | `4128768` |
| `/private_commit/measured_samples/3` | `3940352` |
| `/private_commit/measured_samples/4` | `3981312` |
| `/private_commit/measured_samples/5` | `4018176` |
| `/private_commit/measured_samples/6` | `4042752` |
| `/private_commit/measured_samples/7` | `3698688` |
| `/private_commit/measured_samples/8` | `4288512` |
| `/private_commit/measured_samples/9` | `4255744` |
| `/working_set/available` | `true` |
| `/working_set/stable` | `true` |
| `/working_set/terminal_growth` | `false` |
| `/working_set/assessment_status` | `"pass"` |
| `/working_set/gate_status` | `"diagnostic"` |
| `/working_set/gate_applied` | `false` |
| `/working_set/budget_qualified` | `false` |
| `/working_set/budget_scope` | `"future_packaged_reference_hardware"` |
| `/working_set/baseline_last_bytes` | `5804032` |
| `/working_set/baseline_full_min_bytes` | `5472256` |
| `/working_set/baseline_central_min_bytes` | `5513216` |
| `/working_set/baseline_median_bytes` | `5685248` |
| `/working_set/baseline_central_max_bytes` | `5689344` |
| `/working_set/baseline_full_max_bytes` | `5804032` |
| `/working_set/final_last_bytes` | `5894144` |
| `/working_set/final_full_min_bytes` | `5652480` |
| `/working_set/final_central_min_bytes` | `5734400` |
| `/working_set/final_median_bytes` | `5873664` |
| `/working_set/final_central_max_bytes` | `5873664` |
| `/working_set/final_full_max_bytes` | `5894144` |
| `/working_set/peak_bytes` | `7045120` |
| `/working_set/growth_ratio` | `0.03314121037463977` |
| `/working_set/instant_growth_ratio` | `0.015525758645024701` |
| `/working_set/warmup_trimmed_span_ratio` | `0.030979827089337175` |
| `/working_set/warmup_full_span_ratio` | `0.058357348703170026` |
| `/working_set/measured_trimmed_span_ratio` | `0.023709902370990237` |
| `/working_set/measured_full_span_ratio` | `0.041143654114365415` |
| `/working_set/warmup_samples/0` | `5693440` |
| `/working_set/warmup_samples/1` | `5664768` |
| `/working_set/warmup_samples/2` | `5681152` |
| `/working_set/warmup_samples/3` | `5517312` |
| `/working_set/warmup_samples/4` | `5795840` |
| `/working_set/warmup_samples/5` | `5472256` |
| `/working_set/warmup_samples/6` | `5513216` |
| `/working_set/warmup_samples/7` | `5685248` |
| `/working_set/warmup_samples/8` | `5689344` |
| `/working_set/warmup_samples/9` | `5804032` |
| `/working_set/measured_samples/0` | `5697536` |
| `/working_set/measured_samples/1` | `5832704` |
| `/working_set/measured_samples/2` | `5828608` |
| `/working_set/measured_samples/3` | `5808128` |
| `/working_set/measured_samples/4` | `5898240` |
| `/working_set/measured_samples/5` | `5652480` |
| `/working_set/measured_samples/6` | `5873664` |
| `/working_set/measured_samples/7` | `5734400` |
| `/working_set/measured_samples/8` | `5873664` |
| `/working_set/measured_samples/9` | `5894144` |

## Godot Smoke

These are persisted gate counts and exact log paths. Non-persisted wall-clock durations are intentionally omitted.

| Verification | Debug | Release | Persisted logs |
| --- | --- | --- | --- |
| Project gate | `24/24` | `24/24` | `build/debug/Testing/Temporary/LastTest.log`; `build/release/Testing/Temporary/LastTest.log` |
| Upstream Box3D | `20/20` | `20/20` | `artifacts/physics/upstream-box3d-debug.log`; `artifacts/physics/upstream-box3d-release.log` |
| Godot headless API | `1/1`, exit `0` | `1/1`, exit `0` | `artifacts/physics/godot-smoke-debug.stdout.log`, `artifacts/physics/godot-smoke-debug.stderr.log`; `artifacts/physics/godot-smoke-release.stdout.log`, `artifacts/physics/godot-smoke-release.stderr.log` |
| Godot renderers | `2/2` (Vulkan/OpenGL) | `2/2` (Vulkan/OpenGL) | `artifacts/physics/godot-scene-debug.stdout.log`, `artifacts/physics/godot-scene-debug.stderr.log`, `artifacts/physics/godot-scene-gl-debug.stdout.log`, `artifacts/physics/godot-scene-gl-debug.stderr.log`; `artifacts/physics/godot-scene-release.stdout.log`, `artifacts/physics/godot-scene-release.stderr.log`, `artifacts/physics/godot-scene-gl-release.stdout.log`, `artifacts/physics/godot-scene-gl-release.stderr.log` |

The following rows are the persisted graphical gate contract, not metadata derived from volatile AVI files. Every gate run removes the prior target, requires a fresh frame-300 marker, and verifies the resulting movie with `ffprobe`.

| Build | Renderer | Contract movie path | Gate contract (verified every run) |
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
- `artifacts/physics/box3d-spike-{debug,release}.json` is volatile; only the two Evidence paths above are normative.

## Recommendation

Both snapshots have no normative violations. The private commit budget remains explicitly deferred and reported by `private_commit_budget_unqualified`.

Recommendation: prosseguir_com_limites
