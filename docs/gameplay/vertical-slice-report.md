# First Orbit Vertical Slice Certification

Generated from the two tracked evidence snapshots. Manual edits are rejected by `--check`.

Normative-Certification-Identity: Tested-Inputs-SHA256 (content-addressed and revalidated against the current tree).

Evidence-Debug-Path: docs/gameplay/evidence/vertical-slice-debug.json
Evidence-Debug-SHA256: b1637a902e0682edd2aa01ce52fa396f9a3516ca1239bdaf65c7d115bad49932
Generation-Head-Debug: 64e9e365260d8042ac47255d0dba2224641cf659
Tested-Inputs-Debug-SHA256: e1197ead992736ca5a328619ddc5716e86335460beb2f6e82a8c794fdb0aeb5d
Capture-Manifest-Debug-SHA256: ce0efcc5e0daa91632d8ce37b3d1afb1321ecb087d0b294b9582050e60848720

Evidence-Release-Path: docs/gameplay/evidence/vertical-slice-release.json
Evidence-Release-SHA256: cae248f560197d14ef7a02beb3001563693d2f515302958f4437c05c40804e51
Generation-Head-Release: 64e9e365260d8042ac47255d0dba2224641cf659
Tested-Inputs-Release-SHA256: e1197ead992736ca5a328619ddc5716e86335460beb2f6e82a8c794fdb0aeb5d
Capture-Manifest-Release-SHA256: 2ce578355651ebde1dfeac913a010366ce7b89c0e6c8e3048d106c67c69385b4

## Gate summary

| Build | Physics p95 | Vulkan p95/p99 | OpenGL p95/p99 |
| --- | ---: | ---: | ---: |
| Debug | 1.750 ms | 6.061/6.181 ms | 6.061/6.061 ms |
| Release | 1.637 ms | 6.061/6.133 ms | 6.061/8.333 ms |

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
| `vulnerable_impact` | 493 | 492 | `104384a4ef9d7a1d2729671c59431ce7b040065e12f0c7f3be5ed46b53d3f7e7` |
| `result` | 2283 | 2272 | `6acbe72015543b5614df4c560ab0e886553b8fcee7f78d1b7d86dcb860cd26a6` |

The 300-frame Vulkan and OpenGL records were deterministically sampled from normal-cadence source captures of 2307 and 2307 frames.

## Review and clean-room

- Independent substitute review: `independent_agents`; five new human players were `unavailable`.
- The comparative clean-room review covers names, logos, silhouettes, sounds, UI, layouts, and promotional material.
- Virela, Nox, and Talo remain codenames pending clearance.
- This engineering review is not a legal opinion and does not replace legal counsel before commercial publication.
- Review manifest SHA-256: `816dec67eb18b77f97808d198dda468c96e2ec0709f1dc392bd8b30102f82ac9`.

## Windows package

- Path: `artifacts/package/windows-release`
- Manifest SHA-256: `0887030a0c04ce2311bf43724a470aec57a6ddd1d9bedfd506005566f2d83118`
- Launch from path containing spaces: `passed`

## Verdict

All recorded blocking gates passed for the certified vertical slice.
