# First Orbit Vertical Slice Certification

Generated from the two tracked evidence snapshots. Manual edits are rejected by `--check`.

Normative-Certification-Identity: Tested-Inputs-SHA256 (content-addressed and revalidated against the current tree).

Evidence-Debug-Path: docs/gameplay/evidence/vertical-slice-debug.json
Evidence-Debug-SHA256: 7aa3c939f48e5c91d4570f2bf7beb39806f9a4021cc4f13c301e20ead3334fb1
Generation-Head-Debug: 2312994724bc5b55684194158e4e2d8af64b2918
Tested-Inputs-Debug-SHA256: e2bfea17396bdc850521f3ac6fe1be55854272954fd0bf22e719102bda8401b3
Capture-Manifest-Debug-SHA256: f3d84e46839bdd4479d4418e815f1bb12fe20412aa85c972c1ed37810ea20b72

Evidence-Release-Path: docs/gameplay/evidence/vertical-slice-release.json
Evidence-Release-SHA256: 412ace958003a1303b7b27718163a559c0266afcde01d94b189367dc11bf0787
Generation-Head-Release: 2312994724bc5b55684194158e4e2d8af64b2918
Tested-Inputs-Release-SHA256: e2bfea17396bdc850521f3ac6fe1be55854272954fd0bf22e719102bda8401b3
Capture-Manifest-Release-SHA256: 6752fd273a2cff94f33e39fa617022758cce5a950b95c0bbb7c1708570045e88

## Gate summary

| Build | Physics p95 | Vulkan p95/p99 | OpenGL p95/p99 |
| --- | ---: | ---: | ---: |
| Debug | 1.650 ms | 6.250/6.250 ms | 6.061/6.061 ms |
| Release | 1.730 ms | 6.061/6.157 ms | 6.667/6.944 ms |

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
| `vulnerable_impact` | 493 | 492 | `676bc6e716ea17d317e5d7f203aef8ac958442976ff9512fd3cbd3db1f861105` |
| `result` | 2283 | 2272 | `a7ae2983476e805ae632c919b9f7e3d2f7df0053d629234be78eedecff2effd5` |

The 300-frame Vulkan and OpenGL records were deterministically sampled from normal-cadence source captures of 2307 and 2307 frames.

## Review and clean-room

- Independent substitute review: `independent_agents`; five new human players were `unavailable`.
- The comparative clean-room review covers names, logos, silhouettes, sounds, UI, layouts, and promotional material.
- Virela, Nox, and Talo remain codenames pending clearance.
- This engineering review is not a legal opinion and does not replace legal counsel before commercial publication.
- Review manifest SHA-256: `fd7edd3d3fec558b91d0855ca7b94effa362993d3a1c2fd22312b78604625281`.

## Windows package

- Path: `artifacts/package/windows-release`
- Manifest SHA-256: `1140297df1a781f4858ad35476be65ea1a860673680db6667ca556d3eb8dcb67`
- Launch from path containing spaces: `passed`

## Verdict

All recorded blocking gates passed for the certified vertical slice.
