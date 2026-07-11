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


def expected_assessment(
    warmup_trimmed: float,
    growth: float,
    terminal_growth: bool,
    measured_trimmed: float,
) -> str:
    if warmup_trimmed > 0.05:
        return "unstable"
    if growth > 0.05 or terminal_growth:
        return "growth"
    if measured_trimmed > 0.05:
        return "unstable"
    return "pass"


def validate_unavailable_counter(counter: dict, label: str) -> None:
    if counter["warmup_samples"] or counter["measured_samples"]:
        raise AssertionError(f"{label} unavailable raw arrays must be empty")
    if counter["stable"] or counter["terminal_growth"]:
        raise AssertionError(f"{label} unavailable flags are incoherent")
    numeric_fields = [
        name
        for name in counter
        if name.endswith("_bytes") or name.endswith("_ratio")
    ]
    if any(counter[name] != 0 for name in numeric_fields):
        raise AssertionError(f"{label} unavailable derived values must be zero")


def validate_available_counter(
    counter: dict, label: str, include_instant: bool
) -> str:
    baseline = tail_summary(counter["warmup_samples"])
    final = tail_summary(counter["measured_samples"])
    growth = max(0.0, (final[2] - baseline[2]) / baseline[2])
    warmup_trimmed = (baseline[3] - baseline[1]) / baseline[2]
    warmup_full = (baseline[4] - baseline[0]) / baseline[2]
    measured_trimmed = (final[3] - final[1]) / final[2]
    measured_full = (final[4] - final[0]) / final[2]
    terminal_threshold = 1.05 * baseline[2]
    terminal_growth = (
        counter["measured_samples"][8] > terminal_threshold
        and counter["measured_samples"][9] > terminal_threshold
    )
    stable = warmup_trimmed <= 0.05 and measured_trimmed <= 0.05
    expected_integers = {
        "baseline_last_bytes": counter["warmup_samples"][-1],
        "baseline_full_min_bytes": baseline[0],
        "baseline_central_min_bytes": baseline[1],
        "baseline_median_bytes": baseline[2],
        "baseline_central_max_bytes": baseline[3],
        "baseline_full_max_bytes": baseline[4],
        "final_last_bytes": counter["measured_samples"][-1],
        "final_full_min_bytes": final[0],
        "final_central_min_bytes": final[1],
        "final_median_bytes": final[2],
        "final_central_max_bytes": final[3],
        "final_full_max_bytes": final[4],
    }
    for name, expected in expected_integers.items():
        if counter[name] != expected:
            raise AssertionError(f"{label} {name}: {counter[name]} != {expected}")
    expected_ratios = {
        "growth_ratio": growth,
        "warmup_trimmed_span_ratio": warmup_trimmed,
        "warmup_full_span_ratio": warmup_full,
        "measured_trimmed_span_ratio": measured_trimmed,
        "measured_full_span_ratio": measured_full,
    }
    if include_instant:
        expected_ratios["instant_growth_ratio"] = max(
            0.0,
            (counter["measured_samples"][-1] - counter["warmup_samples"][-1])
            / counter["warmup_samples"][-1],
        )
    for name, expected in expected_ratios.items():
        assert_close(counter[name], expected, f"{label} {name}")
    if counter["terminal_growth"] != terminal_growth:
        raise AssertionError(f"{label} terminal guard mismatch")
    if counter["stable"] != stable:
        raise AssertionError(f"{label} stability mismatch")
    if counter["peak_bytes"] < max(
        counter["warmup_samples"] + counter["measured_samples"]
    ):
        raise AssertionError(f"{label} peak is below a raw sample")
    return expected_assessment(
        warmup_trimmed, growth, terminal_growth, measured_trimmed
    )


def validate_stress_observation(observation: dict, release_build: bool) -> None:
    del release_build
    memory = observation["memory"]
    private = memory["private_commit"]
    working = memory["working_set"]
    if memory["gate_scope"] != "release_mt":
        raise AssertionError("unexpected private gate scope")
    if memory["gate_status"] != "diagnostic" or memory["gate_applied"]:
        raise AssertionError("PrivateUsage must remain diagnostic-only")
    if memory["budget_qualified"]:
        raise AssertionError("foundation private budget must remain unqualified")
    if memory["budget_scope"] != "future_packaged_reference_hardware":
        raise AssertionError("unexpected future private budget scope")
    if private["available"]:
        private_assessment = validate_available_counter(
            private, "private", include_instant=True
        )
    else:
        validate_unavailable_counter(private, "private")
        private_assessment = "unavailable"
    if memory["assessment_status"] != private_assessment:
        raise AssertionError("private assessment status mismatch")

    if working["gate_status"] != "diagnostic" or working["gate_applied"]:
        raise AssertionError("Working Set must remain diagnostic-only")
    if working["budget_qualified"]:
        raise AssertionError("working set budget must remain unqualified")
    if working["budget_scope"] != "future_packaged_reference_hardware":
        raise AssertionError("unexpected working set budget scope")
    if working["available"]:
        working_assessment = validate_available_counter(
            working, "working set", include_instant=True
        )
    else:
        validate_unavailable_counter(working, "working set")
        working_assessment = "unavailable"
    if working["assessment_status"] != working_assessment:
        raise AssertionError("working set assessment mismatch")


def validate_unavailable_memory_contract() -> None:
    private = {
        "available": False,
        "stable": False,
        "terminal_growth": False,
        "baseline_last_bytes": 0,
        "baseline_full_min_bytes": 0,
        "baseline_central_min_bytes": 0,
        "baseline_median_bytes": 0,
        "baseline_central_max_bytes": 0,
        "baseline_full_max_bytes": 0,
        "final_last_bytes": 0,
        "final_full_min_bytes": 0,
        "final_central_min_bytes": 0,
        "final_median_bytes": 0,
        "final_central_max_bytes": 0,
        "final_full_max_bytes": 0,
        "peak_bytes": 0,
        "growth_ratio": 0.0,
        "instant_growth_ratio": 0.0,
        "warmup_trimmed_span_ratio": 0.0,
        "warmup_full_span_ratio": 0.0,
        "measured_trimmed_span_ratio": 0.0,
        "measured_full_span_ratio": 0.0,
        "warmup_samples": [],
        "measured_samples": [],
    }
    working = {
        **private,
        "assessment_status": "unavailable",
        "gate_status": "diagnostic",
        "gate_applied": False,
        "budget_qualified": False,
        "budget_scope": "future_packaged_reference_hardware",
        "instant_growth_ratio": 0.0,
    }
    validate_stress_observation(
        {
            "memory": {
                "gate_scope": "release_mt",
                "gate_status": "diagnostic",
                "assessment_status": "unavailable",
                "gate_applied": False,
                "budget_qualified": False,
                "budget_scope": "future_packaged_reference_hardware",
                "private_commit": private,
                "working_set": working,
            }
        },
        release_build=False,
    )


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
            for peak_name in (
                "peak_body_count",
                "peak_shape_count",
                "peak_joint_count",
                "peak_awake_count",
                "peak_contact_count",
            ):
                if observation[peak_name] != scenario[peak_name]:
                    raise AssertionError(
                        f"repeat topology mismatch: {scenario['name']}/{peak_name}"
                    )
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
    validate_unavailable_memory_contract()
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
        expected_topology = {
            "radial_fall": ((1, 2, 0), (2, 2, 0, 1, 1)),
            "projectile_pile": ((121, 123, 0), (123, 123, 0, 121, 221)),
            "radial_pile": ((80, 81, 0), (81, 81, 0, 80, 204)),
            "mass_ratio": ((80, 81, 0), (81, 81, 0, 80, 227)),
            "stress": ((500, 800, 250), (500, 800, 250, 500, 0)),
            "capability_matrix": ((0, 0, 0), (123, 123, 1, 121, 221)),
        }
        for scenario in document["scenarios"]:
            if len(scenario["hashes"]) != 2 or len(set(scenario["hashes"])) != 1:
                raise AssertionError(f"non-deterministic hashes: {scenario['name']}")
            fixture = (
                scenario["dynamic_body_count"],
                scenario["shape_count"],
                scenario["joint_count"],
            )
            peak = (
                scenario["peak_body_count"],
                scenario["peak_shape_count"],
                scenario["peak_joint_count"],
                scenario["peak_awake_count"],
                scenario["peak_contact_count"],
            )
            if (fixture, peak) != expected_topology[scenario["name"]]:
                raise AssertionError(
                    f"topology mismatch for {scenario['name']}: {fixture}/{peak}"
                )
        hashes = {scenario["name"]: scenario["hashes"][0] for scenario in document["scenarios"]}
        if hashes["capability_matrix"] == hashes["radial_pile"]:
            raise AssertionError("capability matrix reused the radial pile hash")
        if len(document["matrix"]) != 8:
            raise AssertionError("expected all eight capability rows")
        if any(row["status"] == "blocked" for row in document["matrix"]):
            raise AssertionError("capability matrix contains a blocked row")
        capability_scenario = next(
            scenario
            for scenario in document["scenarios"]
            if scenario["name"] == "capability_matrix"
        )
        for peak_name in (
            "peak_body_count",
            "peak_shape_count",
            "peak_joint_count",
            "peak_awake_count",
            "peak_contact_count",
        ):
            if any(peak_name not in row for row in document["matrix"]):
                raise AssertionError(f"capability row is missing {peak_name}")
            if max(row[peak_name] for row in document["matrix"]) != capability_scenario[peak_name]:
                raise AssertionError(f"capability aggregate mismatch: {peak_name}")
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
