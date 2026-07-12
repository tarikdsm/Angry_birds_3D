# First Orbit Vertical Slice Certification

Generated from the two tracked evidence snapshots. Manual edits are rejected by `--check`.

Evidence-Debug-Path: docs/gameplay/evidence/vertical-slice-debug.json
Evidence-Debug-SHA256: abf4cfaa3ec73841f622792b3da7bc040911e1dd40080343c8c98d14d6f24169
Certified-Commit-Debug: 3281c2c33e2b3e6267da2d3f65efd8108bc32ee6
Tested-Inputs-Debug-SHA256: 3e31e577cb4f2d5ae8195089bfbbdaace61757e7a23949c1d21c051ee780bfcc
Capture-Manifest-Debug-SHA256: 67421e994abc8e8d5e76f2c126e5e25581180e934ef0c27db168f025e1fece79

Evidence-Release-Path: docs/gameplay/evidence/vertical-slice-release.json
Evidence-Release-SHA256: 8d8d4bee6533383e56fabeb363d1c312e7bea0dd9de03f9545a581b6e2b0710c
Certified-Commit-Release: 3281c2c33e2b3e6267da2d3f65efd8108bc32ee6
Tested-Inputs-Release-SHA256: 3e31e577cb4f2d5ae8195089bfbbdaace61757e7a23949c1d21c051ee780bfcc
Capture-Manifest-Release-SHA256: 8dee41c0de9b3a1340dc359d8a3f0a5682f448d722306e398ba814e5110ed951

## Gate summary

| Build | Physics p95 | Vulkan p95/p99 | OpenGL p95/p99 |
| --- | ---: | ---: | ---: |
| Debug | 1.634 ms | 6.061/6.182 ms | 6.250/6.250 ms |
| Release | 1.787 ms | 6.250/6.250 ms | 6.079/8.333 ms |

## Deterministic routes

| Route | Outcome | Repeat hashes |
| --- | --- | --- |
| `virela_win` | `Victory` | `18304663578686684669 = 18304663578686684669` |
| `structural_win` | `Victory` | `15827554224084106611 = 15827554224084106611` |
| `no_ability_loss` | `Defeat` | `11323609224403413012 = 11323609224403413012` |

## Visual evidence

Release goldens are the canonical certification images; Debug goldens are diagnostic and remain configuration-specific.

| Golden | Source frame | Transition frame | SHA-256 |
| --- | ---: | ---: | --- |
| `overview` | 0 | 0 | `dfc7a30f32f2cabaf135ae12b01217b8487ef718948fcb8020582c38a4dcf4fb` |
| `aim` | 40 | 3 | `1c2faf6b7d50089fdff2c7a6b3bbc850839a8191b34d80da493e4ad07f1a1d2c` |
| `virela` | 60 | 55 | `61cddda984cfa4eb2cc415750b00bfa59e43b5edfcdc1f182537dd01435c0879` |
| `vulnerable_impact` | 493 | 492 | `09e98159583144f0ab98e8477df7c04adfb9015dda7a8dedec1cc94f8aed7830` |
| `result` | 3957 | 3936 | `4b4b21089cbd866adc974668a2dd15170873a19427c4dbc6f6998bc70286c53a` |

The 300-frame Vulkan and OpenGL records were deterministically sampled from normal-cadence source captures of 3971 and 3971 frames.

## Review and clean-room

- Independent substitute review: `independent_agents`; five new human players were `unavailable`.
- The comparative clean-room review covers names, logos, silhouettes, sounds, UI, layouts, and promotional material.
- Virela, Nox, and Talo remain codenames pending clearance.
- This engineering review is not a legal opinion and does not replace legal counsel before commercial publication.
- Review manifest SHA-256: `b1f14a8b04b167a8bff05aed06e89b818d5a56ac6b8a10daf886164bc21f3e22`.

## Windows package

- Path: `artifacts/package/windows-release`
- Manifest SHA-256: `6e0673984ad35cb1e377bd3f73b7e9b6e2385fe2f76be959bc4383dcb9df7aa8`
- Launch from path containing spaces: `passed`

## Verdict

All recorded blocking gates passed for the certified vertical slice.
