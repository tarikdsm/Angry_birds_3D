import json
import math
import pathlib
import shutil
import subprocess
import sys
import tempfile


def assert_close(actual: float, expected: float, label: str) -> None:
    if not math.isclose(actual, expected, rel_tol=1e-12, abs_tol=1e-12):
        raise AssertionError(f"{label}: {actual} != {expected}")


def tail_summary(samples: list[int]) -> tuple[int, int, int, int, int]:
    if len(samples) != 10 or any(value <= 0 for value in samples):
        raise AssertionError("memory protocol requires ten positive samples")
    tail = sorted(samples[5:])
    return tail[0], tail[1], tail[2], tail[3], tail[4]


def validate_stress_observation(observation: dict, release_build: bool) -> None:
    memory = observation["memory"]
    private = observation["memory"]["private_commit"]
    working = observation["memory"]["working_set"]
    if not private["available"]:
        raise AssertionError("PrivateUsage is unavailable")
    baseline_full_min, baseline_central_min, baseline_median, baseline_central_max, baseline_full_max = tail_summary(private["warmup_samples"])
    final_full_min, final_central_min, final_median, final_central_max, final_full_max = tail_summary(private["measured_samples"])
    expected_growth = max(0.0, (final_median - baseline_median) / baseline_median)
    warmup_trimmed_span = (baseline_central_max - baseline_central_min) / baseline_median
    warmup_full_span = (baseline_full_max - baseline_full_min) / baseline_median
    measured_trimmed_span = (final_central_max - final_central_min) / final_median
    measured_full_span = (final_full_max - final_full_min) / final_median
    terminal_threshold = 1.05 * baseline_median
    terminal_growth = (
        private["measured_samples"][8] > terminal_threshold
        and private["measured_samples"][9] > terminal_threshold
    )
    expected_stable = warmup_trimmed_span <= 0.05 and measured_trimmed_span <= 0.05
    expected_integers = {
        "baseline_last_bytes": private["warmup_samples"][-1],
        "baseline_full_min_bytes": baseline_full_min,
        "baseline_central_min_bytes": baseline_central_min,
        "baseline_median_bytes": baseline_median,
        "baseline_central_max_bytes": baseline_central_max,
        "baseline_full_max_bytes": baseline_full_max,
        "final_last_bytes": private["measured_samples"][-1],
        "final_full_min_bytes": final_full_min,
        "final_central_min_bytes": final_central_min,
        "final_median_bytes": final_median,
        "final_central_max_bytes": final_central_max,
        "final_full_max_bytes": final_full_max,
    }
    for name, expected in expected_integers.items():
        if private[name] != expected:
            raise AssertionError(f"private {name}: {private[name]} != {expected}")
    assert_close(private["growth_ratio"], expected_growth, "private growth")
    assert_close(private["warmup_trimmed_span_ratio"], warmup_trimmed_span, "private warmup trimmed span")
    assert_close(private["warmup_full_span_ratio"], warmup_full_span, "private warmup full span")
    assert_close(private["measured_trimmed_span_ratio"], measured_trimmed_span, "private measured trimmed span")
    assert_close(private["measured_full_span_ratio"], measured_full_span, "private measured full span")
    if private["terminal_growth"] != terminal_growth:
        raise AssertionError("private terminal guard mismatch")
    if private["stable"] != expected_stable:
        raise AssertionError("private stability mismatch")
    if private["peak_bytes"] < max(
        private["warmup_samples"] + private["measured_samples"]
    ):
        raise AssertionError("private peak is below a raw sample")
    if warmup_trimmed_span > 0.05:
        assessment_status = "unstable"
    elif expected_growth > 0.05 or terminal_growth:
        assessment_status = "growth"
    elif measured_trimmed_span > 0.05:
        assessment_status = "unstable"
    else:
        assessment_status = "pass"
    if memory["assessment_status"] != assessment_status:
        raise AssertionError("private assessment status mismatch")
    if memory["gate_scope"] != "release_mt":
        raise AssertionError("unexpected private gate scope")
    if memory["gate_applied"]:
        raise AssertionError("foundation must not apply the private budget gate")
    if memory["gate_status"] != "diagnostic":
        raise AssertionError("private gate status mismatch")
    if memory["budget_qualified"]:
        raise AssertionError("foundation private budget must remain unqualified")
    if memory["budget_scope"] != "future_packaged_reference_hardware":
        raise AssertionError("unexpected future private budget scope")

    working_baseline = tail_summary(working["warmup_samples"])
    working_final = tail_summary(working["measured_samples"])
    if not working["available"]:
        raise AssertionError("WorkingSetSize diagnostics are unavailable")
    working_expected = {
        "baseline_last_bytes": working["warmup_samples"][-1],
        "baseline_full_min_bytes": working_baseline[0],
        "baseline_central_min_bytes": working_baseline[1],
        "baseline_median_bytes": working_baseline[2],
        "baseline_central_max_bytes": working_baseline[3],
        "baseline_full_max_bytes": working_baseline[4],
        "final_last_bytes": working["measured_samples"][-1],
        "final_full_min_bytes": working_final[0],
        "final_central_min_bytes": working_final[1],
        "final_median_bytes": working_final[2],
        "final_central_max_bytes": working_final[3],
        "final_full_max_bytes": working_final[4],
    }
    for name, expected in working_expected.items():
        if working[name] != expected:
            raise AssertionError(f"working set {name}: {working[name]} != {expected}")
    working_growth = max(
        0.0, (working_final[2] - working_baseline[2]) / working_baseline[2]
    )
    working_warmup_trimmed = (
        working_baseline[3] - working_baseline[1]
    ) / working_baseline[2]
    working_warmup_full = (
        working_baseline[4] - working_baseline[0]
    ) / working_baseline[2]
    working_measured_trimmed = (
        working_final[3] - working_final[1]
    ) / working_final[2]
    working_measured_full = (
        working_final[4] - working_final[0]
    ) / working_final[2]
    working_instant = max(
        0.0,
        (working["measured_samples"][-1] - working["warmup_samples"][-1])
        / working["warmup_samples"][-1],
    )
    working_terminal_threshold = 1.05 * working_baseline[2]
    working_terminal = (
        working["measured_samples"][8] > working_terminal_threshold
        and working["measured_samples"][9] > working_terminal_threshold
    )
    working_stable = (
        working_warmup_trimmed <= 0.05 and working_measured_trimmed <= 0.05
    )
    if working_warmup_trimmed > 0.05:
        working_assessment = "unstable"
    elif working_growth > 0.05 or working_terminal:
        working_assessment = "growth"
    elif working_measured_trimmed > 0.05:
        working_assessment = "unstable"
    else:
        working_assessment = "pass"
    for name, expected in {
        "growth_ratio": working_growth,
        "instant_growth_ratio": working_instant,
        "warmup_trimmed_span_ratio": working_warmup_trimmed,
        "warmup_full_span_ratio": working_warmup_full,
        "measured_trimmed_span_ratio": working_measured_trimmed,
        "measured_full_span_ratio": working_measured_full,
    }.items():
        assert_close(working[name], expected, f"working set {name}")
    if working["terminal_growth"] != working_terminal:
        raise AssertionError("working set terminal guard mismatch")
    if working["stable"] != working_stable:
        raise AssertionError("working set stability mismatch")
    if working["assessment_status"] != working_assessment:
        raise AssertionError("working set assessment mismatch")
    if working["gate_status"] != "diagnostic" or working["gate_applied"]:
        raise AssertionError("working set must remain diagnostic-only")
    if working["budget_qualified"]:
        raise AssertionError("working set budget must remain unqualified")
    if working["budget_scope"] != "future_packaged_reference_hardware":
        raise AssertionError("unexpected working set budget scope")
    if working["peak_bytes"] < max(
        working["warmup_samples"] + working["measured_samples"]
    ):
        raise AssertionError("working set peak is below a raw sample")


def validate_repeat_observations(document: dict) -> None:
    release_build = document["build_type"] == "Release"
    process_allocator = document["process_box3d_allocator"]
    if (
        process_allocator["baseline_bytes"] != 0
        or process_allocator["final_bytes"] != 0
        or not process_allocator["exact_return"]
    ):
        raise AssertionError("process Box3 allocator did not return exactly to zero")
    for scenario in document["scenarios"]:
        observations = scenario["repeat_observations"]
        if len(observations) != 2:
            raise AssertionError(f"missing repeat observations: {scenario['name']}")
        for index, observation in enumerate(observations, start=1):
            if observation["repeat"] != index:
                raise AssertionError("repeat observation index mismatch")
            if observation["hash"] != scenario["hashes"][index - 1]:
                raise AssertionError("repeat observation hash mismatch")
            allocator = observation["box3d_allocator"]
            if (
                allocator["baseline_bytes"] != 0
                or allocator["final_bytes"] != 0
                or allocator["max_abs_delta"] != 0
                or not allocator["exact_return"]
            ):
                raise AssertionError("scenario Box3 allocator imbalance")
        if scenario["name"] == "stress":
            for observation in observations:
                validate_stress_observation(observation, release_build)
                allocator = observation["box3d_allocator"]
                if (
                    len(allocator["warmup_post_teardown"]) != 10
                    or len(allocator["measured_post_teardown"]) != 10
                    or any(allocator["warmup_post_teardown"])
                    or any(allocator["measured_post_teardown"])
                ):
                    raise AssertionError("Stress Box3 allocator samples are not exact zero")
                crt = observation["crt"]
                if release_build:
                    if crt["applicable"]:
                        raise AssertionError("CRT gate must be not_applicable in Release")
                elif not crt["applicable"] or not crt["balanced"]:
                    raise AssertionError("Debug CRT live-block gate failed")


def main() -> int:
    executable = pathlib.Path(sys.argv[1])
    failure_report = executable.parent / "ninho-spike-smoke-failure.json"
    failure_report.unlink(missing_ok=True)
    invalid_commands = [
        ["--unknown"],
        ["--seed", "1"],
        ["--all", "--scenario", "radial_fall"],
        ["--scenario", "unknown"],
        ["--all", "--seed", "bad"],
        ["--all", "--substeps", "0"],
        ["--all", "--repeat", "0"],
        ["--all", "--json"],
    ]
    for arguments in invalid_commands:
        completed = subprocess.run([str(executable), *arguments], check=False)
        if completed.returncode != 2:
            raise AssertionError(
                f"invalid arguments returned {completed.returncode}: {arguments}"
            )
    with tempfile.TemporaryDirectory(prefix="ninho-json-") as directory:
        write_failure = subprocess.run(
            [
                str(executable),
                "--scenario",
                "radial_fall",
                "--json",
                directory,
            ],
            check=False,
        )
        if write_failure.returncode != 1:
            raise AssertionError(
                f"JSON write failure returned {write_failure.returncode}, expected 1"
            )
        report = pathlib.Path(directory) / "relatorio-çã.json"
        completed = subprocess.run(
            [
                str(executable),
                "--all",
                "--repeat",
                "2",
                "--json",
                str(report),
            ],
            check=False,
        )
        if completed.returncode != 0:
            if report.is_file():
                shutil.copyfile(report, failure_report)
                with report.open("r", encoding="utf-8") as stream:
                    failed_document = json.load(stream)
                validate_repeat_observations(failed_document)
                diagnostic = {
                    "returncode": completed.returncode,
                    "preserved_report": str(failure_report),
                    "scenarios": [
                        {
                            "name": scenario["name"],
                            "hashes": scenario["hashes"],
                            "violations": scenario["violations"],
                            "memory": scenario["memory"],
                        }
                        for scenario in failed_document["scenarios"]
                    ],
                    "blocked_rows": [
                        row
                        for row in failed_document["matrix"]
                        if row["status"] == "blocked"
                    ],
                }
                print(json.dumps(diagnostic, ensure_ascii=False), file=sys.stderr)
            return completed.returncode
        with report.open("r", encoding="utf-8") as stream:
            document = json.load(stream)
        validate_repeat_observations(document)
        if document["schema"] != "ninho.physics.scenario.v1":
            raise AssertionError("unexpected JSON schema")
        if document["dependencies"]["box3d"]["commit"] != "8441b4a06d6d09dcfb0b0f704df4d847d1437b92":
            raise AssertionError("unexpected Box3D commit")
        if document["dependencies"]["godot"]["commit"] != "f62fdbde15035c5576dad93e586201f4d41ef0cb":
            raise AssertionError("unexpected Godot commit")
        if document["dependencies"]["godot_cpp"]["commit"] != "e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77":
            raise AssertionError("unexpected godot-cpp commit")
        if document["budget_qualification"] != {
            "status": "deferred",
            "target_growth_ratio": 0.05,
            "warning": "private_commit_budget_unqualified",
        }:
            raise AssertionError("unexpected budget qualification")
        if not any(
            warning["code"] == "private_commit_budget_unqualified"
            for warning in document["warnings"]
        ):
            raise AssertionError("missing private budget warning")
        if document["violations"]:
            raise AssertionError("successful report contains normative violations")
        if document["recommendation"] != "prosseguir_com_limites":
            raise AssertionError("unexpected limits recommendation")
        if len(document["scenarios"]) != 6:
            raise AssertionError("expected all six scenarios")
        for scenario in document["scenarios"]:
            if len(scenario["hashes"]) != 2 or len(set(scenario["hashes"])) != 1:
                raise AssertionError(f"non-deterministic hashes: {scenario['name']}")
        hashes = {scenario["name"]: scenario["hashes"][0] for scenario in document["scenarios"]}
        if hashes["capability_matrix"] == hashes["radial_pile"]:
            raise AssertionError("capability matrix reused the radial pile hash")
        if len(document["matrix"]) != 8:
            raise AssertionError("expected all eight capability rows")
        if any(row["status"] == "blocked" for row in document["matrix"]):
            raise AssertionError("capability matrix contains a blocked row")
        for row in document["matrix"]:
            if row["functional_status"] not in {"pass", "fallback", "blocked"}:
                raise AssertionError("invalid functional capability status")
            if row["functional_status"] == "fallback":
                if not row["functional_fallback"]:
                    raise AssertionError("functional fallback is missing")
            elif row["functional_fallback"] is not None:
                raise AssertionError("unexpected functional fallback")
        if any(not row["fixture_hashes"] for row in document["matrix"]):
            raise AssertionError("capability matrix row is missing fixture hashes")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
