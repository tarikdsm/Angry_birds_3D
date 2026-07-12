# First Orbit Vertical Slice Certification

Generated from the two tracked evidence snapshots. Manual edits are rejected by `--check`.

Normative-Certification-Identity: Tested-Inputs-SHA256 (content-addressed and revalidated against the current tree).

Evidence-Debug-Path: docs/gameplay/evidence/vertical-slice-debug.json
Evidence-Debug-SHA256: 221001c7d44b203ab449ee73f8dda4482388cf5e6f3ad5cb7d36509eafe25355
Generation-Head-Debug: c7b80303a216c0e2d03fc575bdf32834a8ea3aba
Tested-Inputs-Debug-SHA256: 5578688596dc6501cc7a080e6d412a09b75aa62578aec73793ffdcc111c09642
Capture-Manifest-Debug-SHA256: 7f61adfc38979feeedc35a5953faba6aa80f4dd720c18baf5c1e4841c558772f

Evidence-Release-Path: docs/gameplay/evidence/vertical-slice-release.json
Evidence-Release-SHA256: e014d965bcd31b99182db6477e9032c44ab812068d7b8991a7ae4a64664d22f0
Generation-Head-Release: c7b80303a216c0e2d03fc575bdf32834a8ea3aba
Tested-Inputs-Release-SHA256: 5578688596dc6501cc7a080e6d412a09b75aa62578aec73793ffdcc111c09642
Capture-Manifest-Release-SHA256: 23e3cd08488728808d87a4442dee8c563e7ce356d7a7c43f14ab27d0cf4e75ec

## Gate summary

| Build | Physics p95 | Vulkan p95/p99 | OpenGL p95/p99 |
| --- | ---: | ---: | ---: |
| Debug | 1.679 ms | 6.667/9.559 ms | 6.250/9.725 ms |
| Release | 1.989 ms | 12.693/17.510 ms | 6.667/6.944 ms |

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
| `vulnerable_impact` | 493 | 492 | `e727747e0fcaa95f907b51b8e408d71acd95c0a97660ab5a474d4168530de76f` |
| `result` | 2283 | 2272 | `5bf51c708e93b168976a8bcdeb713ab25f479db33ac1e99ee685bb7bc6b9dfe0` |

The 300-frame Vulkan and OpenGL records were deterministically sampled from normal-cadence source captures of 2307 and 2307 frames.

## Review and clean-room

- Independent substitute review: `independent_agents`; five new human players were `unavailable`.
- The comparative clean-room review covers names, logos, silhouettes, sounds, UI, layouts, and promotional material.
- Virela, Nox, and Talo remain codenames pending clearance.
- This engineering review is not a legal opinion and does not replace legal counsel before commercial publication.
- Review manifest SHA-256: `6bdfdff70d5ce23bf28efa1f31b30f34bec68d097a3ef9aa218acf61d8f88f2d`.

## Windows package

- Path: `artifacts/package/windows-release`
- Manifest SHA-256: `c572c1227aa25d5d54be18be63f209860ad1c499d9555fc6977d2bc70f50dd37`
- Launch from path containing spaces: `passed`

## Verdict

All recorded blocking gates passed for the certified vertical slice.
