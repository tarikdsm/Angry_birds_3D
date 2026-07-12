# First Orbit Vertical Slice Certification

Generated from the two tracked evidence snapshots. Manual edits are rejected by `--check`.

Normative-Certification-Identity: Tested-Inputs-SHA256 (content-addressed and revalidated against the current tree).

Evidence-Debug-Path: docs/gameplay/evidence/vertical-slice-debug.json
Evidence-Debug-SHA256: 2095407658abd160b047703fc59fcd1f45e02fe0c3563ba9f0e458b9cfa3ffc6
Generation-Head-Debug: e53095e52914b8a451ad216d7d25570776d9a42e
Tested-Inputs-Debug-SHA256: af88a40f6f6cdf0df65dad7763d74d99246745fd2ae1f63e009f4ef53fc22904
Capture-Manifest-Debug-SHA256: b314d64febf8b6e4928305e0edc4c6a30fddab6db75d11290892b53e944529f5

Evidence-Release-Path: docs/gameplay/evidence/vertical-slice-release.json
Evidence-Release-SHA256: bdfcac230785dbb1163f7173f2810a15fdfd3348bf5223af8188d29c2d04ed5c
Generation-Head-Release: e53095e52914b8a451ad216d7d25570776d9a42e
Tested-Inputs-Release-SHA256: af88a40f6f6cdf0df65dad7763d74d99246745fd2ae1f63e009f4ef53fc22904
Capture-Manifest-Release-SHA256: 31d4a2db85b90562df3362d7384ba05df34f4c861d0aeee57d4e6324048f8e04

## Gate summary

| Build | Physics p95 | Vulkan p95/p99 | OpenGL p95/p99 |
| --- | ---: | ---: | ---: |
| Debug | 2.359 ms | 6.250/9.381 ms | 6.061/8.009 ms |
| Release | 1.869 ms | 6.250/6.250 ms | 6.136/6.667 ms |

## Deterministic routes

| Route | Outcome | Repeat hashes |
| --- | --- | --- |
| `virela_win` | `Victory` | `460897019529119944 = 460897019529119944` |
| `structural_win` | `Victory` | `13375825633044651172 = 13375825633044651172` |
| `no_ability_loss` | `Defeat` | `11323609224403413012 = 11323609224403413012` |

## Visual evidence

Release goldens are the canonical certification images; Debug goldens are diagnostic and remain configuration-specific.

| Golden | Source frame | Transition frame | SHA-256 |
| --- | ---: | ---: | --- |
| `overview` | 0 | 0 | `dfc7a30f32f2cabaf135ae12b01217b8487ef718948fcb8020582c38a4dcf4fb` |
| `aim` | 40 | 3 | `b17acb71db13e4a47c7a759a640407c1ae1bdebe4895721c51aa595293a49ab8` |
| `virela` | 60 | 55 | `29e73941612287d5e9d229ab3bfdf4668b8a1b730af6461485c85988c204fb1d` |
| `vulnerable_impact` | 493 | 492 | `433c1934992ab71f61df7b9759f49b93d47b3abf56ceac49108ac3e1e19e4a1a` |
| `result` | 3957 | 3936 | `81833a08ebdbdc9a57c734cf3ff62d4f342b8b9ce602b1cbd1cf67366690e291` |

The 300-frame Vulkan and OpenGL records were deterministically sampled from normal-cadence source captures of 3971 and 3971 frames.

## Review and clean-room

- Independent substitute review: `independent_agents`; five new human players were `unavailable`.
- The comparative clean-room review covers names, logos, silhouettes, sounds, UI, layouts, and promotional material.
- Virela, Nox, and Talo remain codenames pending clearance.
- This engineering review is not a legal opinion and does not replace legal counsel before commercial publication.
- Review manifest SHA-256: `1a5b3769f97e7d8244c4373a726b9f9ef46b0b2a32d0c32d5ca0070f7a11d169`.

## Windows package

- Path: `artifacts/package/windows-release`
- Manifest SHA-256: `286ec93c9328cc62043700ec6b25ff8c44f015fad08f6bcbde24e10673eb3d64`
- Launch from path containing spaces: `passed`

## Verdict

All recorded blocking gates passed for the certified vertical slice.
