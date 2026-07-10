# Private Commit Gate Correction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Gate exact allocator return, Debug CRT ownership, and Release `/MT` configuration while preserving Private/Working Set as unqualified foundation diagnostics with a limits recommendation.

**Architecture:** A private Box3 helper exposes `b3GetByteCount`; Debug uses CRT checkpoints around a prewarmed cycle; a single Windows process sampler returns private commit, working set, and peak working set. Private/WS assessments are diagnostic in both builds and defer the 5% budget to packaged reference hardware. Repeat observations preserve Private/WS/Box3/CRT from every run, all excluded from hashes.

**Tech Stack:** C++20, MSVC, Box3D, Win32 `PROCESS_MEMORY_COUNTERS_EX`, CMake/CTest, Python 3.11 JSON smoke.

## Global Constraints

- Keep 500 bodies, 800 shapes, 250 joints, 300+1,200 ticks, 10 full allocator warmups, and 10 full measured cycles.
- Require exact Box3 allocator baseline return in every build and zero Debug CRT live-block deltas.
- Keep `PrivateUsage` and Working Set diagnostic/unqualified in both builds; Release `/MD` remains a toolchain mismatch.
- Use tail samples 6-10, median-of-five, central trimmed span `(s3-s1)/s2`, growth, and the M9+M10 terminal guard to classify diagnostic footprint telemetry; full span is diagnostic too, and no footprint assessment gates exit, matrix, or hash.
- Keep the 60-second watchdog per scenario and 150-second aggregate smoke timeout unless measured normal runtime requires only the aggregate to increase.
- Preserve the completed Debug/Release footprint audits as evidence of an unqualified future budget; do not rerun them.

---

### Task 1: Pure private-commit protocol

**Files:**
- Modify: `native/kernel/include/ninho/physics/scenario.hpp`
- Modify: `native/kernel/src/scenario.cpp`
- Test: `native/tests/pile_stability_tests.cpp`

**Interfaces:**
- Produces: `PrivateCommitAssessment assess_private_commit(warmup, measured)` with full/central min/max, medians, growth, terminal growth, trimmed/full spans, stability, availability, and diagnostic classification.

- [ ] Add RED tests for median tail-five with low/high outliers, persistent shift, two-sample terminal shift, unstable warmup/measured tails, and unavailable/zero/wrong-length inputs.
- [ ] Run the focused test and confirm failure for missing assessment API.
- [ ] Implement the exact median/span/growth/terminal protocol with no Working Set fallback.
- [ ] Run focused tests and confirm every classification and threshold.

### Task 2: Allocator, CRT, process sampling, and Stress telemetry

**Files:**
- Modify: `native/kernel/include/ninho/physics/scenario.hpp`
- Modify: `native/kernel/src/scenario.cpp`
- Test: `native/tests/pile_stability_tests.cpp`
- Test: `native/tests/determinism_tests.cpp`

**Interfaces:**
- Produces: `ProcessMemorySample { private_usage_bytes, working_set_bytes, peak_working_set_bytes }` and 10+10 private/WS arrays in `ScenarioResult`.

- [ ] Add RED assertions that Stress exposes both 10+10 sample sets and all private derived fields.
- [ ] Add the sole direct `b3GetByteCount` helper and require exact process/scenario/10+10 teardown return.
- [ ] Add the Debug CRT checkpoint cycle and replace individual Working Set reads with one process sampler after complete teardown.
- [ ] Classify unavailable, unstable, persistent growth, and terminal growth diagnostically while preserving raw/peak fields and never affecting violations, exit, matrix, or hash.
- [ ] Serialize private and Working Set data without adding either to canonical hashes.

### Task 3: Lifecycle and repeat observability

**Files:**
- Modify: `native/kernel/include/ninho/physics/scenario.hpp`
- Modify: `native/kernel/src/scenario.cpp`
- Modify: `native/spike/main.cpp`
- Test: `native/tests/scenario_capability_tests.cpp`
- Test: `native/tests/determinism_tests.cpp`

**Interfaces:**
- Produces: lifecycle functional/Box3/CRT gates plus global Release `/MT` configuration validation and `repeat_observations[]` containing hash and full Private/WS/Box3/CRT telemetry.

- [ ] Add REDs for lifecycle unavailable/private diagnostics, memory-insensitive lifecycle hash, and repeat observation JSON.
- [ ] Apply the same private sampler/protocol to lifecycle without changing its 10,000 functional cycles.
- [ ] Preserve first scenario result while appending an observation for every repeat in `main.cpp`.
- [ ] Verify private/WS mutations do not affect lifecycle hash while handles/cycles/fixture still do.

### Task 4: Smoke recomputation and tracked docs

**Files:**
- Modify: `native/tests/json_smoke.py`
- Modify: `docs/superpowers/specs/2026-07-10-box3d-foundation-design.md`
- Modify: `docs/superpowers/plans/2026-07-10-box3d-foundation-implementation.md`
- Append: `.superpowers/sdd/task-5-report.md`

**Interfaces:**
- Consumes: raw private samples from both repeat observations.
- Produces: independent Python recomputation of median, growth, spans, terminal guard, and documented PrivateUsage contract.

- [ ] Extend smoke assertions to recompute all private fields for both repeats and require 10+10 private/WS samples.
- [ ] Update design sections 6.5, 6.7, and 8 plus implementation Tasks 5 and 8.
- [ ] Preserve failure-report instrumentation and append prior/new audit evidence to the ignored Task 5 report.

### Task 5: Frozen verification and commit

**Files:**
- Create through executable: `build/artifacts/release-memory-audit-{1,2,3}.json`

**Interfaces:**
- Produces: final Debug/Release evidence plus one `prosseguir_com_limites` report; historical audits remain immutable diagnostics.

- [ ] Build and run focused tests; confirm Stress runtime remains below 60 seconds.
- [ ] Do not rerun footprint audits; run full Debug, full Release, one Release spike, `git diff --check`, and temp replay scan.
- [ ] Require zero normative violations, exact Box3/CRT gates, diagnostic Private/WS telemetry, permanent warning, and `prosseguir_com_limites`.
- [ ] Append final evidence, stage docs/code/tests, and commit one atomic corrective commit.

### Task 6: Complete diagnostic Working Set protocol

**Files:**
- Modify: `native/kernel/include/ninho/physics/scenario.hpp`
- Modify: `native/kernel/src/scenario.cpp`
- Modify: `native/tests/pile_stability_tests.cpp`
- Modify: `native/tests/json_smoke.py`

- [ ] Add RED assertions for every Working Set tail statistic, span, growth, terminal/stability flag, assessment enum, and diagnostic qualification field.
- [ ] Mirror the frozen 10+10 private calculation for Working Set and serialize all derived values per repeat.
- [ ] Remove runtime footprint thresholds from CTest; prove synthetic Growth/Unstable assessments do not alter lifecycle status or scenario exit.

### Task 7: Canonical functional row state

**Files:**
- Modify: `native/kernel/include/ninho/physics/scenario.hpp`
- Modify: `native/kernel/src/scenario.cpp`
- Modify: `native/tests/scenario_capability_tests.cpp`

- [ ] Add RED tests showing hull and replay gate-only changes leave the hash unchanged while functional status/fallback changes alter it.
- [ ] Store `functional_status` and `functional_fallback` before allocator/CRT/tooling gates on every capability row.
- [ ] Serialize both functional fields and hash only them plus functional values and fixture hashes.

### Task 8: Global runtime configuration gate

**Files:**
- Modify: `native/kernel/include/ninho/physics/scenario.hpp`
- Modify: `native/kernel/src/scenario.cpp`
- Modify: `native/tests/pile_stability_tests.cpp`

- [ ] Add RED coverage dispatching all six scenario kinds through a simulated Release `/MD` configuration.
- [ ] Return `configuration_mismatch` before scenario execution for every kind; keep Debug `/MTd` diagnostic and valid.

### Task 9: Final policy verification

**Files:**
- Modify: `tools/run_spike.ps1`
- Modify: `.superpowers/sdd/task-5-report.md`

- [ ] Validate the permanent warning and deferred budget directly in `run_spike.ps1`.
- [ ] Run focused RED/GREEN checks, full Debug, full Release, and one final Release spike without rerunning historical footprint audits.
- [ ] Require zero compiler warnings, clean diff, no temp replay files, and commit the correction atomically.
