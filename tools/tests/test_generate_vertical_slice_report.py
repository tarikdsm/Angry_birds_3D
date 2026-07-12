import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "generate_vertical_slice_report.py"


class VerticalSliceReportTests(unittest.TestCase):
    def test_missing_evidence_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run(
                [sys.executable, str(SCRIPT), "--root", directory, "--write"],
                capture_output=True, text=True, encoding="utf-8"
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("missing", result.stderr.lower())

    def test_generated_report_is_deterministic_and_check_rejects_drift(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            evidence_dir = root / "docs/gameplay/evidence"
            evidence_dir.mkdir(parents=True)
            base = {
                "schema": "ninho.vertical-slice.evidence.v1",
                "schema_version": 1,
                "commit": "0" * 40,
                "tested_inputs_sha256": "b" * 64,
                "capture_manifest": {"sha256": "c" * 64},
                "reviews_manifest": {"sha256": "d" * 64},
                "hardware": {"cpu": "cpu", "gpu": "gpu", "ram_bytes": 1, "os": "os"},
                "physics": {"step_p95_ms": 1.0, "limit_ms": 8.0},
                "renderers": [
                    {"name": name, "capture": {"source_frames": 4000}, "scales": [{"ui_scale": s, "frame_p95_ms": 10.0,
                     "frame_p99_ms": 15.0, "max_hitch_ms": 20.0,
                     "input_feedback_p95_ms": 16.7} for s in (100, 150)]}
                    for name in ("Vulkan", "OpenGL")
                ],
                "routes": [{"name": "route", "outcome": "Victory", "hashes": ["1", "1"]}],
                "golden_metadata": [{"name": "result", "source_frame": 3999,
                                     "source_transition_frame": 3970, "sha256": "e" * 64}],
                "rubric": {"critical": 0, "important": 0},
                "playtest": {"status": "unavailable", "substitute": "independent_agents"},
                "clean_room": {"approved": True, "comparative_review": "approved"},
                "package": {"path": "dist/NinhoCosmico", "manifest_sha256": "a" * 64,
                            "launch_from_space_path": "passed"},
            }
            for config in ("debug", "release"):
                doc = dict(base, configuration=config.title())
                (evidence_dir / f"vertical-slice-{config}.json").write_text(
                    json.dumps(doc, indent=2) + "\n", encoding="utf-8", newline="\n"
                )
            first = subprocess.run([sys.executable, str(SCRIPT), "--root", str(root), "--write"], capture_output=True, text=True)
            self.assertEqual(first.returncode, 0, first.stderr)
            report = root / "docs/gameplay/vertical-slice-report.md"
            original = report.read_bytes()
            self.assertIn(b"independent_agents", original)
            self.assertIn(b"not a legal opinion", original)
            self.assertIn(b"Release goldens are the canonical certification images", original)
            self.assertIn(b"Tested-Inputs-Release-SHA256", original)
            full_check = subprocess.run(
                [sys.executable, str(SCRIPT), "--root", str(root), "--check"],
                capture_output=True, text=True, encoding="utf-8"
            )
            self.assertNotEqual(full_check.returncode, 0)
            self.assertIn("full evidence gate rejected", full_check.stderr)
            report.write_bytes(original + b"drift\n")
            self.assertNotEqual(subprocess.run([sys.executable, str(SCRIPT), "--root", str(root), "--check"]).returncode, 0)


if __name__ == "__main__":
    unittest.main()
