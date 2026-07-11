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

from ninho_blender.contracts import safe_output_path, validate_config_contract


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
