import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "generate_foundation_report.py"
DEBUG_RELATIVE = Path("docs/physics/evidence/foundation-report-debug.json")
RELEASE_RELATIVE = Path("docs/physics/evidence/foundation-report-release.json")
REPORT_RELATIVE = Path("docs/physics/box3d-spike-report.md")


def load_module():
    spec = importlib.util.spec_from_file_location("generate_foundation_report", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def json_scalar(value):
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def escape_pointer_token(token):
    return str(token).replace("~", "~0").replace("/", "~1")


def leaf_rows(value, pointer=""):
    if isinstance(value, dict):
        if not value:
            yield pointer or "/", value
        for key, child in value.items():
            yield from leaf_rows(child, f"{pointer}/{escape_pointer_token(key)}")
    elif isinstance(value, list):
        if not value:
            yield pointer or "/", value
        for index, child in enumerate(value):
            yield from leaf_rows(child, f"{pointer}/{index}")
    else:
        yield pointer or "/", value


class FoundationReportGeneratorTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        for relative in (DEBUG_RELATIVE, RELEASE_RELATIVE):
            destination = self.root / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            document = json.loads((ROOT / relative).read_text(encoding="utf-8"))
            destination.write_text(
                json.dumps(document, ensure_ascii=False, indent=2) + "\n",
                encoding="utf-8",
                newline="\n",
            )

    def tearDown(self):
        self.temp.cleanup()

    def run_script(self, *arguments):
        return subprocess.run(
            [sys.executable, str(SCRIPT), "--root", str(self.root), *arguments],
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

    def test_generate_is_deterministic_and_check_detects_markdown_drift(self):
        first = self.run_script("--write")
        self.assertEqual(first.returncode, 0, first.stderr)
        report_path = self.root / REPORT_RELATIVE
        first_bytes = report_path.read_bytes()

        second = self.run_script("--write")
        self.assertEqual(second.returncode, 0, second.stderr)
        self.assertEqual(report_path.read_bytes(), first_bytes)
        self.assertEqual(self.run_script("--check").returncode, 0)

        report_path.write_bytes(first_bytes + b"drift\n")
        drift = self.run_script("--check")
        self.assertNotEqual(drift.returncode, 0)
        self.assertIn("out of date", drift.stderr)

    def test_report_covers_required_sections_pins_results_and_every_scenario_leaf(self):
        generated = self.run_script("--write")
        self.assertEqual(generated.returncode, 0, generated.stderr)
        report = (self.root / REPORT_RELATIVE).read_text(encoding="utf-8")

        for heading in (
            "# Box3D Foundation Spike Report",
            "## Versions",
            "## Capability Matrix",
            "## Scenario Metrics",
            "## Ownership, Allocator, and CRT",
            "## Footprint Diagnostic 10+10",
            "## Godot Smoke",
            "## Known Limits",
            "## Recommendation",
        ):
            self.assertIn(heading, report)

        for exact_pin in (
            "8441b4a06d6d09dcfb0b0f704df4d847d1437b92",
            "f62fdbde15035c5576dad93e586201f4d41ef0cb",
            "e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77",
            "18.7.3 (11925.98)",
            "14.44.35207 / 19.44.35228.0",
            "10.0.26100.0",
            "4.3.3 / 1.13.2 / 3.11.9",
        ):
            self.assertIn(exact_pin, report)

        for persisted_result in (
            "24/24",
            "20/20",
            "build/debug/Testing/Temporary/LastTest.log",
            "build/release/Testing/Temporary/LastTest.log",
            "artifacts/physics/upstream-box3d-debug.log",
            "artifacts/physics/upstream-box3d-release.log",
            "artifacts/physics/godot-smoke-debug.stdout.log",
            "artifacts/physics/godot-smoke-release.stdout.log",
        ):
            self.assertIn(persisted_result, report)
        movie_paths = (
            "artifacts/physics/godot-scene-debug.avi",
            "artifacts/physics/godot-scene-gl-debug.avi",
            "artifacts/physics/godot-scene-release.avi",
            "artifacts/physics/godot-scene-gl-release.avi",
        )
        for movie_path in movie_paths:
            self.assertEqual(report.count(movie_path), 1)
            movie_line = next(line for line in report.splitlines() if movie_path in line)
            for contract_fact in (
                "MJPEG",
                "1280x720",
                "300 frames",
                "5 s",
                "freshness verified",
                "frame-300 marker verified",
            ):
                self.assertIn(contract_fact, movie_line)
        for wallclock_duration in ("206.67 s", "15.34 s", "9.70 s", "0.92 s"):
            self.assertNotIn(wallclock_duration, report)

        for configuration, relative in (("Debug", DEBUG_RELATIVE), ("Release", RELEASE_RELATIVE)):
            document = json.loads((self.root / relative).read_text(encoding="utf-8"))
            self.assertIn(f"Recommendation-{configuration}: prosseguir_com_limites", report)
            metadata_start = report.index(f"### {configuration} snapshot metadata")
            metadata_end = report.find("\n### ", metadata_start + 1)
            metadata_section = report[
                metadata_start : metadata_end if metadata_end >= 0 else len(report)
            ]
            metadata = {
                key: value
                for key, value in document.items()
                if key not in ("scenarios", "matrix")
            }
            for pointer, value in leaf_rows(metadata):
                expected = f"| `{pointer}` | `{json_scalar(value)}` |"
                self.assertIn(expected, metadata_section)
            for row in document["matrix"]:
                marker = f"### {configuration} capability `{row['capability']}`"
                self.assertIn(marker, report)
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
                    expected = f"| `{key}` | `{json_scalar(row[key])}` |"
                    self.assertIn(expected, report)
                for value in row["values"]:
                    expected = (
                        f"| `{value['name']}` | `{json_scalar(value['value'])}` | "
                        f"`{json_scalar(value['unit'])}` |"
                    )
                    self.assertIn(expected, report)
                for fixture_hash in row["fixture_hashes"]:
                    self.assertIn(f"`{fixture_hash}`", report)

            for scenario in document["scenarios"]:
                section_start = report.index(f"### {configuration} scenario `{scenario['name']}`")
                next_section = report.find("\n### ", section_start + 1)
                section = report[section_start : next_section if next_section >= 0 else len(report)]
                for pointer, value in leaf_rows(scenario):
                    expected = f"| `{pointer}` | `{json_scalar(value)}` |"
                    self.assertIn(expected, section)

    def test_missing_private_instant_growth_ratio_is_rejected(self):
        debug_path = self.root / DEBUG_RELATIVE
        document = json.loads(debug_path.read_text(encoding="utf-8"))
        stress = next(item for item in document["scenarios"] if item["name"] == "stress")
        del stress["repeat_observations"][0]["memory"]["private_commit"]["instant_growth_ratio"]
        debug_path.write_text(json.dumps(document), encoding="utf-8")

        result = self.run_script("--write")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("private_commit.instant_growth_ratio", result.stderr)

    def test_check_can_compare_an_explicit_gate_report_path(self):
        generated = self.run_script("--write")
        self.assertEqual(generated.returncode, 0, generated.stderr)
        alternate = self.root / "audit" / "foundation.md"
        alternate.parent.mkdir(parents=True)
        shutil.copyfile(self.root / REPORT_RELATIVE, alternate)

        checked = self.run_script("--check", "--report-path", str(alternate))
        self.assertEqual(checked.returncode, 0, checked.stderr)
        alternate.write_text("mutated", encoding="utf-8")
        self.assertNotEqual(
            self.run_script("--check", "--report-path", str(alternate)).returncode,
            0,
        )

    def test_cli_reads_only_the_two_canonical_json_inputs(self):
        module = load_module()
        accessed = []
        original = Path.read_bytes

        def recording_read_bytes(path):
            accessed.append(Path(path).resolve())
            return original(path)

        Path.read_bytes = recording_read_bytes
        try:
            module.load_evidence(self.root)
        finally:
            Path.read_bytes = original

        self.assertEqual(
            accessed,
            [(self.root / DEBUG_RELATIVE).resolve(), (self.root / RELEASE_RELATIVE).resolve()],
        )

    @unittest.skipUnless(os.name == "nt", "junction contract is Windows-specific")
    def test_write_rejects_absent_report_below_junction_parent(self):
        external = self.root / "external-physics"
        external.mkdir()
        source_physics = self.root / "docs" / "physics"
        shutil.copytree(source_physics / "evidence", external / "evidence")
        shutil.rmtree(source_physics)
        junction = self.root / "docs" / "physics"
        created = subprocess.run(
            ["cmd", "/c", "mklink", "/J", str(junction), str(external)],
            capture_output=True,
            text=True,
            encoding="utf-8",
        )
        self.assertEqual(created.returncode, 0, created.stderr)

        try:
            result = self.run_script("--write")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("reparse point", result.stderr.lower())
            self.assertFalse((external / "box3d-spike-report.md").exists())
        finally:
            os.rmdir(junction)


if __name__ == "__main__":
    unittest.main()
