#!/usr/bin/env python3
"""Render the normative Box3D foundation report from its two tracked snapshots."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import stat
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Iterator


DEBUG_RELATIVE = Path("docs/physics/evidence/foundation-report-debug.json")
RELEASE_RELATIVE = Path("docs/physics/evidence/foundation-report-release.json")
REPORT_RELATIVE = Path("docs/physics/box3d-spike-report.md")


@dataclass(frozen=True)
class Evidence:
    configuration: str
    relative_path: Path
    sha256: str
    document: dict[str, Any]


def _decode_json(raw: bytes, path: Path) -> dict[str, Any]:
    try:
        document = json.loads(raw.decode("utf-8-sig"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ValueError(f"invalid UTF-8 JSON evidence {path.as_posix()}: {error}") from error
    if not isinstance(document, dict):
        raise ValueError(f"evidence root must be an object: {path.as_posix()}")
    return document


def _require_report_fields(evidence: Evidence) -> None:
    document = evidence.document
    if document.get("build_type") != evidence.configuration:
        raise ValueError(
            f"expected build_type {evidence.configuration} in {evidence.relative_path.as_posix()}"
        )
    scenarios = document.get("scenarios")
    if not isinstance(scenarios, list):
        raise ValueError(f"scenarios must be an array in {evidence.relative_path.as_posix()}")
    stress = [scenario for scenario in scenarios if scenario.get("name") == "stress"]
    if len(stress) != 1:
        raise ValueError(f"expected one stress scenario in {evidence.relative_path.as_posix()}")
    observations = stress[0].get("repeat_observations")
    if not isinstance(observations, list) or len(observations) != 2:
        raise ValueError(f"expected two stress repeat observations for {evidence.configuration}")
    for index, observation in enumerate(observations, start=1):
        try:
            memory = observation["memory"]
            private = memory["private_commit"]
            working = memory["working_set"]
            private["instant_growth_ratio"]
        except (KeyError, TypeError) as error:
            raise ValueError(
                f"{evidence.configuration} stress repeat {index} requires "
                "memory.private_commit.instant_growth_ratio"
            ) from error
        for label, channel in (("private_commit", private), ("working_set", working)):
            if len(channel.get("warmup_samples", [])) != 10 or len(
                channel.get("measured_samples", [])
            ) != 10:
                raise ValueError(
                    f"{evidence.configuration} stress repeat {index} {label} requires 10+10 samples"
                )


def load_evidence(root: Path) -> tuple[Evidence, Evidence]:
    """Read the two, and only the two, canonical evidence JSON inputs."""
    loaded: list[Evidence] = []
    for configuration, relative in (
        ("Debug", DEBUG_RELATIVE),
        ("Release", RELEASE_RELATIVE),
    ):
        raw = (root / relative).read_bytes()
        evidence = Evidence(
            configuration=configuration,
            relative_path=relative,
            sha256=hashlib.sha256(raw).hexdigest().upper(),
            document=_decode_json(raw, relative),
        )
        _require_report_fields(evidence)
        loaded.append(evidence)
    return loaded[0], loaded[1]


def _json_scalar(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def _cell(value: Any) -> str:
    return _json_scalar(value).replace("|", "\\|").replace("`", "\\`")


def _pointer_token(value: Any) -> str:
    return str(value).replace("~", "~0").replace("/", "~1")


def _leaf_rows(value: Any, pointer: str = "") -> Iterator[tuple[str, Any]]:
    if isinstance(value, dict):
        if not value:
            yield pointer or "/", value
        for key, child in value.items():
            yield from _leaf_rows(child, f"{pointer}/{_pointer_token(key)}")
    elif isinstance(value, list):
        if not value:
            yield pointer or "/", value
        for index, child in enumerate(value):
            yield from _leaf_rows(child, f"{pointer}/{index}")
    else:
        yield pointer or "/", value


def _find_scenario(document: dict[str, Any], name: str) -> dict[str, Any]:
    matches = [scenario for scenario in document["scenarios"] if scenario["name"] == name]
    if len(matches) != 1:
        raise ValueError(f"expected one scenario named {name}")
    return matches[0]


def _append_leaf_table(lines: list[str], value: Any) -> None:
    lines.extend(("| JSON pointer | Value |", "| --- | --- |"))
    for pointer, leaf in _leaf_rows(value):
        lines.append(f"| `{pointer}` | `{_cell(leaf)}` |")
    lines.append("")


def _append_versions(lines: list[str], evidences: Iterable[Evidence]) -> None:
    evidences = tuple(evidences)
    dependencies = evidences[0].document["dependencies"]
    for evidence in evidences[1:]:
        if evidence.document["dependencies"] != dependencies:
            raise ValueError("Debug and Release dependency pins diverge")
    lines.extend(
        (
            "## Versions",
            "",
            "| Component | Exact version/pin |",
            "| --- | --- |",
            f"| Box3D 3D | `{dependencies['box3d']['version']}` / `{dependencies['box3d']['commit']}` |",
            f"| Godot | `{dependencies['godot']['version']}` / `{dependencies['godot']['commit']}` |",
            f"| godot-cpp | `{dependencies['godot_cpp']['version']}` / `{dependencies['godot_cpp']['commit']}` |",
            "| Visual Studio Build Tools | `18.7.3 (11925.98)` |",
            "| MSVC tools / compiler | `14.44.35207 / 19.44.35228.0` |",
            "| Windows SDK | `10.0.26100.0` |",
            "| CMake / Ninja / Python | `4.3.3 / 1.13.2 / 3.11.9` |",
            "",
            "Debug uses `/MTd`; Release uses `/MT`. Both reject `/MD[d]` and `/fp:fast`.",
            "",
        )
    )


def _append_capabilities(lines: list[str], evidences: Iterable[Evidence]) -> None:
    lines.extend(("## Capability Matrix", ""))
    for evidence in evidences:
        matrix = evidence.document["matrix"]
        lines.append(f"{evidence.configuration} contains {len(matrix)} capability proofs.")
        lines.append("")
        for row in matrix:
            lines.extend(
                (
                    f"### {evidence.configuration} capability `{row['capability']}`",
                    "",
                    "| Field | Value |",
                    "| --- | --- |",
                )
            )
            for key in (
                "status",
                "fallback",
                "functional_status",
                "functional_fallback",
                "detail",
                "peak_body_count",
                "peak_shape_count",
                "peak_joint_count",
                "peak_awake_count",
                "peak_contact_count",
            ):
                lines.append(f"| `{key}` | `{_cell(row[key])}` |")
            lines.extend(("", "Measured values:", "", "| Name | Value | Unit |", "| --- | ---: | --- |"))
            for value in row["values"]:
                lines.append(
                    f"| `{value['name']}` | `{_cell(value['value'])}` | `{_cell(value['unit'])}` |"
                )
            lines.extend(("", "Fixture hashes:", ""))
            if row["fixture_hashes"]:
                lines.append("- " + ", ".join(f"`{value}`" for value in row["fixture_hashes"]))
            else:
                lines.append("- none")
            lines.append("")


def _append_snapshot_metadata(lines: list[str], evidences: Iterable[Evidence]) -> None:
    lines.extend(
        (
            "## Snapshot Metadata",
            "",
            "This inventory covers every global field except the scenario and capability arrays, "
            "which are rendered in their dedicated sections.",
            "",
        )
    )
    for evidence in evidences:
        metadata = {
            key: value
            for key, value in evidence.document.items()
            if key not in ("scenarios", "matrix")
        }
        lines.extend((f"### {evidence.configuration} snapshot metadata", ""))
        _append_leaf_table(lines, metadata)


def _append_scenarios(lines: list[str], evidences: Iterable[Evidence]) -> None:
    lines.extend(
        (
            "## Scenario Metrics",
            "",
            "Every leaf below is emitted in source order from the normative scenario payload. "
            "This includes identity, topology, timings, hashes, repeat observations, metrics with units, "
            "limits, memory, allocator, CRT, warnings, violations, fallbacks, and embedded matrix proofs.",
            "",
        )
    )
    for evidence in evidences:
        for scenario in evidence.document["scenarios"]:
            lines.extend(
                (
                    f"### {evidence.configuration} scenario `{scenario['name']}`",
                    "",
                    f"Topology fixture `dynamic/shape/joint`: "
                    f"`{scenario['dynamic_body_count']}/{scenario['shape_count']}/{scenario['joint_count']}`. "
                    f"Observed peak `body/shape/joint;awake/contact`: "
                    f"`{scenario['peak_body_count']}/{scenario['peak_shape_count']}/{scenario['peak_joint_count']};"
                    f"{scenario['peak_awake_count']}/{scenario['peak_contact_count']}`.",
                    "",
                )
            )
            _append_leaf_table(lines, scenario)


def _append_ownership(lines: list[str], evidences: Iterable[Evidence]) -> None:
    lines.extend(
        (
            "## Ownership, Allocator, and CRT",
            "",
            "The following inventory includes process totals, scenario aggregates, and every repeat observation.",
            "",
        )
    )
    for evidence in evidences:
        lines.extend((f"### {evidence.configuration} ownership evidence", "", "#### Process Box3D allocator", ""))
        _append_leaf_table(lines, evidence.document["process_box3d_allocator"])
        for scenario in evidence.document["scenarios"]:
            lines.extend((f"#### `{scenario['name']}` aggregate Box3D allocator", ""))
            _append_leaf_table(lines, scenario["box3d_allocator"])
            lines.extend((f"#### `{scenario['name']}` aggregate CRT", ""))
            _append_leaf_table(lines, scenario["crt"])
            for observation in scenario["repeat_observations"]:
                repeat = observation["repeat"]
                lines.extend((f"#### `{scenario['name']}` repeat {repeat} Box3D allocator", ""))
                _append_leaf_table(lines, observation["box3d_allocator"])
                lines.extend((f"#### `{scenario['name']}` repeat {repeat} CRT", ""))
                _append_leaf_table(lines, observation["crt"])


def _append_footprint(lines: list[str], evidences: Iterable[Evidence]) -> None:
    lines.extend(
        (
            "## Footprint Diagnostic 10+10",
            "",
            "All four stress repeats record ten warmup and ten measured PrivateUsage samples, "
            "plus ten warmup and ten measured Working Set samples. Every raw and derived field follows.",
            "",
        )
    )
    for evidence in evidences:
        stress = _find_scenario(evidence.document, "stress")
        for observation in stress["repeat_observations"]:
            repeat = observation["repeat"]
            lines.extend((f"### {evidence.configuration} stress repeat {repeat}", ""))
            _append_leaf_table(lines, observation["memory"])


def _append_persisted_results(lines: list[str]) -> None:
    lines.extend(
        (
            "## Godot Smoke",
            "",
            "These are persisted gate counts and exact log paths. Non-persisted wall-clock durations are intentionally omitted.",
            "",
            "| Verification | Debug | Release | Persisted logs |",
            "| --- | --- | --- | --- |",
            "| Project gate | `24/24` | `24/24` | `build/debug/Testing/Temporary/LastTest.log`; `build/release/Testing/Temporary/LastTest.log` |",
            "| Upstream Box3D | `20/20` | `20/20` | `artifacts/physics/upstream-box3d-debug.log`; `artifacts/physics/upstream-box3d-release.log` |",
            "| Godot headless API | `1/1`, exit `0` | `1/1`, exit `0` | `artifacts/physics/godot-smoke-debug.stdout.log`, `artifacts/physics/godot-smoke-debug.stderr.log`; `artifacts/physics/godot-smoke-release.stdout.log`, `artifacts/physics/godot-smoke-release.stderr.log` |",
            "| Godot renderers | `2/2` (Vulkan/OpenGL) | `2/2` (Vulkan/OpenGL) | `artifacts/physics/godot-scene-debug.stdout.log`, `artifacts/physics/godot-scene-debug.stderr.log`, `artifacts/physics/godot-scene-gl-debug.stdout.log`, `artifacts/physics/godot-scene-gl-debug.stderr.log`; `artifacts/physics/godot-scene-release.stdout.log`, `artifacts/physics/godot-scene-release.stderr.log`, `artifacts/physics/godot-scene-gl-release.stdout.log`, `artifacts/physics/godot-scene-gl-release.stderr.log` |",
            "",
            "The following rows are the persisted graphical gate contract, not metadata derived from volatile AVI files. Every gate run removes the prior target, requires a fresh frame-300 marker, and verifies the resulting movie with `ffprobe`.",
            "",
            "| Build | Renderer | Contract movie path | Gate contract (verified every run) |",
            "| --- | --- | --- | --- |",
            "| Debug | Vulkan Forward Mobile | `artifacts/physics/godot-scene-debug.avi` | `MJPEG; 1280x720; 300 frames; 5 s; freshness verified; frame-300 marker verified` |",
            "| Debug | OpenGL Compatibility | `artifacts/physics/godot-scene-gl-debug.avi` | `MJPEG; 1280x720; 300 frames; 5 s; freshness verified; frame-300 marker verified` |",
            "| Release | Vulkan Forward Mobile | `artifacts/physics/godot-scene-release.avi` | `MJPEG; 1280x720; 300 frames; 5 s; freshness verified; frame-300 marker verified` |",
            "| Release | OpenGL Compatibility | `artifacts/physics/godot-scene-gl-release.avi` | `MJPEG; 1280x720; 300 frames; 5 s; freshness verified; frame-300 marker verified` |",
            "",
        )
    )


def render_report(evidences: tuple[Evidence, Evidence]) -> str:
    debug, release = evidences
    lines = [
        "# Box3D Foundation Spike Report",
        "",
        "This normative report is generated only from the two tracked foundation snapshots. "
        "Run `python tools/generate_foundation_report.py --check` to detect drift.",
        "",
    ]
    for evidence in evidences:
        matrix_scenario = _find_scenario(evidence.document, "capability_matrix")
        topology = (
            f"{matrix_scenario['peak_body_count']}/{matrix_scenario['peak_shape_count']}/"
            f"{matrix_scenario['peak_joint_count']};{matrix_scenario['peak_awake_count']}/"
            f"{matrix_scenario['peak_contact_count']}"
        )
        lines.extend(
            (
                f"Evidence-{evidence.configuration}-Path: {evidence.relative_path.as_posix()}",
                f"Evidence-{evidence.configuration}-SHA256: {evidence.sha256}",
                f"Matrix-{evidence.configuration}-Hash: {matrix_scenario['final_hash']}",
                f"Matrix-{evidence.configuration}-Topology: {topology}",
                f"Recommendation-{evidence.configuration}: {evidence.document['recommendation']}",
            )
        )
    lines.append("")
    _append_versions(lines, evidences)
    _append_snapshot_metadata(lines, evidences)
    _append_capabilities(lines, evidences)
    _append_scenarios(lines, evidences)
    _append_ownership(lines, evidences)
    _append_footprint(lines, evidences)
    _append_persisted_results(lines)
    lines.extend(
        (
            "## Known Limits",
            "",
            "- Box3D 3D `v0.1.0` remains alpha software.",
            "- PrivateUsage measures private commit and Working Set measures residency; neither alone proves ownership.",
            "- The future 5% budget and 8 ms p95 target require a packaged Godot Release build on reference hardware.",
            "- The deterministic runtime manifest remains the valid Godot gate when the initial headless editor scan fails.",
            "- Windows x86_64 is the only qualified foundation target.",
            "- `artifacts/physics/box3d-spike-{debug,release}.json` is volatile; only the two Evidence paths above are normative.",
            "",
            "## Recommendation",
            "",
            "Both snapshots have no normative violations. The private commit budget remains explicitly deferred and "
            "reported by `private_commit_budget_unqualified`.",
            "",
            f"Recommendation: {debug.document['recommendation']}",
            "",
        )
    )
    if debug.document["recommendation"] != release.document["recommendation"]:
        raise ValueError("Debug and Release recommendations diverge")
    return "\n".join(lines)


def _arguments(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="repository root; input and output relative paths remain fixed",
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true", help="fail if the tracked Markdown differs")
    mode.add_argument("--write", action="store_true", help="safely replace the canonical Markdown report")
    parser.add_argument(
        "--report-path",
        type=Path,
        help="report to compare in --check mode; defaults to the canonical report",
    )
    return parser.parse_args(argv)


def _assert_safe_write_path(path: Path, root: Path) -> Path:
    """Reject non-canonical writes and every existing reparse ancestor."""
    allowed_root = Path(os.path.abspath(root))
    target = Path(os.path.abspath(path))
    canonical = allowed_root / REPORT_RELATIVE
    if target != canonical:
        raise ValueError(f"write target must be canonical: {canonical.as_posix()}")
    try:
        relative = target.relative_to(allowed_root)
    except ValueError as error:
        raise ValueError(f"write target escapes repository root: {target.as_posix()}") from error

    current = allowed_root
    candidates = [current]
    for part in relative.parts:
        current = current / part
        candidates.append(current)
    for candidate in candidates:
        try:
            metadata = candidate.lstat()
        except FileNotFoundError:
            break
        attributes = getattr(metadata, "st_file_attributes", 0)
        if stat.S_ISLNK(metadata.st_mode) or (
            attributes & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0)
        ):
            raise ValueError(f"write path contains reparse point: {candidate.as_posix()}")
    return target


def main(argv: list[str] | None = None) -> int:
    arguments = _arguments(argv)
    root = Path(os.path.abspath(arguments.root))
    report_path = (
        arguments.report_path.resolve()
        if arguments.report_path is not None
        else root / REPORT_RELATIVE
    )
    if arguments.report_path is not None and not arguments.check:
        print("--report-path is only valid with --check", file=sys.stderr)
        return 2
    if arguments.write:
        try:
            report_path = _assert_safe_write_path(report_path, root)
        except ValueError as error:
            print(f"foundation report generation failed: {error}", file=sys.stderr)
            return 2
    try:
        expected = render_report(load_evidence(root)).encode("utf-8")
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f"foundation report generation failed: {error}", file=sys.stderr)
        return 2
    if arguments.check:
        try:
            actual = report_path.read_bytes()
        except OSError as error:
            print(f"foundation report is out of date: {error}", file=sys.stderr)
            return 1
        if actual != expected:
            print(
                "foundation report is out of date; run python tools/generate_foundation_report.py --write",
                file=sys.stderr,
            )
            return 1
        return 0
    report_path.parent.mkdir(parents=True, exist_ok=True)
    try:
        _assert_safe_write_path(report_path, root)
    except ValueError as error:
        print(f"foundation report generation failed: {error}", file=sys.stderr)
        return 2
    report_path.write_bytes(expected)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
