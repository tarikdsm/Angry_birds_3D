# First Orbit Vertical Slice Certification

Generated from the two tracked evidence snapshots. Manual edits are rejected by `--check`.

Normative-Certification-Identity: Tested-Inputs-SHA256 (content-addressed and revalidated against the current tree).

Evidence-Debug-Path: docs/gameplay/evidence/vertical-slice-debug.json
Evidence-Debug-SHA256: ce6962b6ceafa8e435192ca7d9c748dc966170c10d376e855be176ec8e0bb0eb
Generation-Head-Debug: 8a6e62e2456c87ff9cba8b67e3cad2747ea92aec
Tested-Inputs-Debug-SHA256: 707ae24fafcd15c3f1df3179d76110685d909e4d54e4466af703b1834811a4b8
Capture-Manifest-Debug-SHA256: 9b457ae4155f9b8d16b74c5f7a678d6a09871e2056ce07a09c5f44eb3a9264a4

Evidence-Release-Path: docs/gameplay/evidence/vertical-slice-release.json
Evidence-Release-SHA256: 52f0275bdd608647ff355bdb0717307904809c1ae32473429ee7030f16293ec7
Generation-Head-Release: 8a6e62e2456c87ff9cba8b67e3cad2747ea92aec
Tested-Inputs-Release-SHA256: 707ae24fafcd15c3f1df3179d76110685d909e4d54e4466af703b1834811a4b8
Capture-Manifest-Release-SHA256: 19543c123e0bf3197f983c78f5703f783a4caee03065935817402e5c74ed5c45

## Gate summary

| Build | Physics p95 | Vulkan p95/p99 | OpenGL p95/p99 |
| --- | ---: | ---: | ---: |
| Debug | 1.925 ms | 6.061/6.138 ms | 6.061/6.095 ms |
| Release | 1.950 ms | 6.061/6.137 ms | 6.061/6.101 ms |

## Deterministic routes

| Route | Outcome | Repeat hashes |
| --- | --- | --- |
| `virela_win` | `Victory` | `14930337922665451601 = 14930337922665451601` |
| `structural_win` | `Victory` | `9772566425278638545 = 9772566425278638545` |
| `no_ability_loss` | `Defeat` | `11323609224403413012 = 11323609224403413012` |

## Visual evidence

Release goldens are the canonical certification images; Debug goldens are diagnostic and remain configuration-specific.

| Golden | Source frame | Transition frame | SHA-256 |
| --- | ---: | ---: | --- |
| `overview` | 0 | 0 | `cf1718912d511a5382a36451a36b8688476dda2ec076d10aa14ffbae68b72d08` |
| `aim` | 39 | 3 | `9142a81b833679ff28f622a191b28a457bca6dce62ba4d9f5cf4c49320280f59` |
| `virela` | 60 | 55 | `b823c2996cbb231e6a15e9f881be0b64b5c5479cf81d01a926c4c662ab070feb` |
| `vulnerable_impact` | 493 | 492 | `52bfe4751149e3fe0d17bef08f65d3a75e7387df164ee6ed95d5d60c26be3341` |
| `result` | 2283 | 2272 | `7e827977701c032bfbb755bb0b7921a00a10b11684668ba6e33e8d6db27389ad` |

The 300-frame Vulkan and OpenGL records were deterministically sampled from normal-cadence source captures of 2307 and 2307 frames.

## Review and clean-room

- Independent substitute review: `independent_agents`; five new human players were `unavailable`.
- The comparative clean-room review covers names, logos, silhouettes, sounds, UI, layouts, and promotional material.
- Virela, Nox, and Talo remain codenames pending clearance.
- This engineering review is not a legal opinion and does not replace legal counsel before commercial publication.
- Review manifest SHA-256: `dd05419c0c3e4d2df36fda92e1f70c760a808bd6a3a1f2559c5329ab48ab165e`.

## Windows package

- Path: `artifacts/package/windows-release`
- Manifest SHA-256: `fff70e71ff3c86c61c11370a5da8e77e2946c7ccf8596ee75625b40dbbf935af`
- Launch from path containing spaces: `passed`

## Verdict

All recorded blocking gates passed for the certified vertical slice.
