# First Orbit Vertical Slice Certification

Generated from the two tracked evidence snapshots. Manual edits are rejected by `--check`.

Normative-Certification-Identity: Tested-Inputs-SHA256 (content-addressed and revalidated against the current tree).

Evidence-Debug-Path: docs/gameplay/evidence/vertical-slice-debug.json
Evidence-Debug-SHA256: c4d239576fb2fd7f5e36e4e480df8c804be7096e28f6a5b901d81879870c4ce1
Generation-Head-Debug: 2c7e3b5afa296df1d37b376eac6b41ffeffc4fcb
Tested-Inputs-Debug-SHA256: 57bf28f13af31d893854596619133235d53d3de9e8215be7f037cede9d12c7f2
Capture-Manifest-Debug-SHA256: 7d2d02e94d7462389131b74d7b59e31dea1f321cf7bdca70b51e768bcc5af8a9

Evidence-Release-Path: docs/gameplay/evidence/vertical-slice-release.json
Evidence-Release-SHA256: 23261d71ed5493a542262b5c4bbcab044741490f7084689956e4b08bb3d52f1e
Generation-Head-Release: 2c7e3b5afa296df1d37b376eac6b41ffeffc4fcb
Tested-Inputs-Release-SHA256: 57bf28f13af31d893854596619133235d53d3de9e8215be7f037cede9d12c7f2
Capture-Manifest-Release-SHA256: 484611fb9c590de8412c3e506435d9e49407b381c577f57709383004f3c0e883

## Gate summary

| Build | Physics p95 | Vulkan p95/p99 | OpenGL p95/p99 |
| --- | ---: | ---: | ---: |
| Debug | 2.471 ms | 6.061/6.832 ms | 6.250/6.667 ms |
| Release | 2.461 ms | 6.250/6.250 ms | 6.250/6.250 ms |

## Deterministic routes

| Route | Outcome | Repeat hashes |
| --- | --- | --- |
| `virela_win` | `Victory` | `16950529107460336353 = 16950529107460336353` |
| `structural_win` | `Victory` | `9286739750595435608 = 9286739750595435608` |
| `no_ability_loss` | `Defeat` | `9146724923232777924 = 9146724923232777924` |

## Visual evidence

Release goldens are the canonical certification images; Debug goldens are diagnostic and remain configuration-specific.

| Golden | Source frame | Transition frame | SHA-256 |
| --- | ---: | ---: | --- |
| `overview` | 0 | 0 | `f781242d099e915fdca0d2303f6098caba249f3b33f903ae26e19feb5a8caa78` |
| `aim` | 39 | 3 | `8b1b33712b986c3d50a0c9dc864d7d678bb7e282e2dea7a1623f7f1e4e6f6d81` |
| `virela` | 95 | 95 | `a21d4cd8852034fbf51cea6281ceffc86cc660b051857712cb35ce4b033eabb8` |
| `vulnerable_impact` | 493 | 492 | `b7c1787721c691789784a8026e23068ab97979c5fd776f9ca2d8777311d75ad5` |
| `result` | 2283 | 2272 | `c815f5bb6b9c0111e67844c27e420375a806cff88a92882c2c83e2b30af326a8` |

The 300-frame Vulkan and OpenGL records were deterministically sampled from normal-cadence source captures of 2307 and 2307 frames.

## Review and clean-room

- Human playtest: `pending` (not performed; 0 participants). Agent reviews are technical controls and do not substitute for human usability observations.
- Four recorded agent-review roles cover code, architecture, gameplay, and art; their labels are not authenticated human identities.
- The comparative clean-room review covers names, logos, silhouettes, sounds, UI, layouts, and promotional material.
- Virela, Nox, and Talo remain codenames pending clearance.
- This engineering review is not a legal opinion and does not replace legal counsel before commercial publication.
- Review manifest SHA-256: `1104220fb0549b3c439ba51e7d32d7f23c4268e8c8354a792bd7546bc4a8f17e`.

## Windows package

- Path: `artifacts/package/windows-release`
- Manifest SHA-256: `3e235cdbb8a71a092c55c9211897e44490f7e618632418c36f19b9abaa50e4be`
- Launch from path containing spaces: `passed`

## Verdict

All recorded technical gates passed for the certified vertical slice. Human playtest remains pending and is required before product release.
