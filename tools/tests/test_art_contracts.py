import copy
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "art" / "scripts"))

from ninho_blender.contracts import (
    enforce_global_budgets,
    measure_instantiated_budgets,
    safe_output_path,
    validate_config_contract,
)
from ninho_blender.textures import encode_png_rgba, generate_material_rgba, inspect_png_rgba


class ArtContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.config = json.loads(
            (ROOT / "art" / "config" / "vertical_slice_assets.json").read_text(encoding="utf-8")
        )

    def test_rejects_asset_folder_traversal_and_absolute_paths(self) -> None:
        for malicious in ("../escape", "kit/../../escape", "C:\\escape", "/escape"):
            with self.subTest(folder=malicious):
                config = copy.deepcopy(self.config)
                config["assets"][0]["folder"] = malicious
                with self.assertRaisesRegex(ValueError, "asset folder"):
                    validate_config_contract(config)

    def test_global_budgets_reject_measured_overage(self) -> None:
        budgets = {
            "triangles": 300_000,
            "draw_calls": 180,
            "texture_bytes": 128 * 1024 * 1024,
            "particles": 30_000,
            "fragments": 120,
        }
        measured = dict(budgets)
        measured["triangles"] += 1
        with self.assertRaisesRegex(ValueError, "triangles global budget exceeded"):
            enforce_global_budgets(measured, budgets)

    def test_instantiated_budgets_multiply_runtime_occurrences(self) -> None:
        metrics = {"KIT_Test": {"triangles": 160_000, "draw_calls": 2, "fragments": 3}}
        measured = measure_instantiated_budgets(metrics, ["KIT_Test", "KIT_Test"])
        self.assertEqual(320_000, measured["triangles"])
        with self.assertRaisesRegex(ValueError, "triangles global budget exceeded"):
            enforce_global_budgets(
                {**measured, "texture_bytes": 0, "particles": 0},
                {"triangles": 300_000, "draw_calls": 180, "texture_bytes": 1, "particles": 1, "fragments": 120},
            )

    def test_procedural_pngs_are_nonconstant_distinct_and_round_trip(self) -> None:
        config = {"seed": 510201, "palette": {"soil": [0.1, 0.2, 0.15, 1.0], "gold": [0.7, 0.4, 0.1, 1.0]}}
        specs = [
            {"name": "MAT_Soil", "palette_key": "soil", "alpha": 1.0, "texture": {"pattern": "stone", "size_px": 32, "contrast": 0.2}},
            {"name": "MAT_Gold", "palette_key": "gold", "alpha": 1.0, "texture": {"pattern": "woven", "size_px": 32, "contrast": 0.2}},
        ]
        contracts = []
        for spec in specs:
            rgba = generate_material_rgba(config, spec)
            contract = inspect_png_rgba(encode_png_rgba(32, 32, rgba))
            self.assertGreater(contract["unique_colors"], 1)
            self.assertTrue(contract["has_nonblack_rgb"])
            contracts.append(contract)
        self.assertNotEqual(contracts[0]["pixel_sha256"], contracts[1]["pixel_sha256"])

    def test_safe_output_path_stays_below_root(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertEqual(
                safe_output_path(root, Path("game/assets/model.glb")),
                root.resolve() / "game" / "assets" / "model.glb",
            )
            with self.assertRaisesRegex(ValueError, "outside output root"):
                safe_output_path(root, Path("../escape.glb"))

    @unittest.skipUnless(os.name == "nt", "junction contract is Windows-specific")
    def test_safe_output_path_rejects_junction_ancestor(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            root = base / "output"
            external = base / "external"
            (root / "game").mkdir(parents=True)
            external.mkdir()
            junction = root / "game" / "materials"
            result = subprocess.run(
                ["cmd", "/c", "mklink", "/J", str(junction), str(external)],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            try:
                with self.assertRaisesRegex(ValueError, "outside output root"):
                    safe_output_path(root, Path("game/materials/pine.tres"))
            finally:
                os.rmdir(junction)


if __name__ == "__main__":
    unittest.main()
