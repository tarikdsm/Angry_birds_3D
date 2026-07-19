import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PROJECT_ID = "SPDXRef-Package-Ninho-Orbital"


class ThirdPartyInventoryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.lock = json.loads((ROOT / "tools" / "toolchain.lock.json").read_text())
        cls.sbom = json.loads((ROOT / "third_party" / "sbom.spdx.json").read_text())
        cls.notices = (ROOT / "THIRD_PARTY_NOTICES.md").read_text(encoding="utf-8")
        cls.readme = (ROOT / "README.md").read_text(encoding="utf-8")
        cls.product_spec = (
            ROOT
            / "docs"
            / "superpowers"
            / "specs"
            / "2026-07-10-ninho-orbital-design.md"
        ).read_text(encoding="utf-8")

    def package(self, spdx_id: str) -> dict:
        matches = [p for p in self.sbom["packages"] if p["SPDXID"] == spdx_id]
        self.assertEqual(len(matches), 1, f"expected exactly one package {spdx_id}")
        return matches[0]

    def assert_build_tool_relationship(self, spdx_id: str) -> None:
        matches = [
            relationship
            for relationship in self.sbom["relationships"]
            if relationship == {
                "spdxElementId": spdx_id,
                "relationshipType": "BUILD_TOOL_OF",
                "relatedSpdxElement": PROJECT_ID,
            }
        ]
        self.assertEqual(len(matches), 1, f"expected one BUILD_TOOL_OF for {spdx_id}")

    def test_blender_inventory_matches_the_toolchain_lock(self) -> None:
        locked = self.lock["blender"]
        package = self.package("SPDXRef-Package-Blender")

        self.assertEqual(package["name"], "Blender")
        self.assertEqual(package["versionInfo"], locked["version"])
        self.assertEqual(package["downloadLocation"], locked["url"])
        self.assertEqual(
            package["checksums"],
            [{"algorithm": "SHA256", "checksumValue": locked["sha256"]}],
        )
        self.assertEqual(package["licenseDeclared"], "GPL-3.0-or-later")
        self.assertEqual(package["licenseConcluded"], "NOASSERTION")
        self.assertEqual(
            package["copyrightText"],
            "Copyright (c) 2011-2026 Blender Foundation",
        )
        self.assertIn(locked["exe_sha256"], package["sourceInfo"])
        self.assertIn("not redistributed", package["sourceInfo"])
        self.assert_build_tool_relationship(package["SPDXID"])

        for fact in (
            "### Blender 5.1.2",
            locked["url"],
            locked["sha256"],
            locked["exe_sha256"],
            "GPL-3.0-or-later",
            ".tools/blender/blender-5.1.2-windows-x64/license/license.md",
        ):
            self.assertIn(fact, self.notices)

    def test_python_is_documented_without_inventing_a_distribution(self) -> None:
        locked = self.lock["python"]
        package = self.package("SPDXRef-Package-Python")

        self.assertEqual(package["name"], "Python interpreter (environment-provided)")
        self.assertEqual(package["versionInfo"], f">={locked['minimum_version']}")
        self.assertEqual(package["downloadLocation"], "NOASSERTION")
        self.assertEqual(package["licenseDeclared"], "NOASSERTION")
        self.assertEqual(package["licenseConcluded"], "NOASSERTION")
        self.assertEqual(package["copyrightText"], "NOASSERTION")
        self.assertNotIn("checksums", package)
        self.assertIn(locked["command"], package["sourceInfo"])
        self.assertIn(locked["minimum_version"], package["sourceInfo"])
        self.assertIn("does not pin", package["sourceInfo"])
        self.assert_build_tool_relationship(package["SPDXID"])

        for fact in (
            f"### Python >= {locked['minimum_version']}",
            f"Command: `{locked['command']}`",
            "does not pin a Python distribution",
        ):
            self.assertIn(fact, self.notices)

        runner = (ROOT / "tools" / "test.ps1").read_text(encoding="utf-8")
        self.assertIn("tests\\test_third_party_inventory.py", runner)

    def test_ffmpeg_inventory_matches_the_toolchain_lock(self) -> None:
        locked = self.lock["ffmpeg"]
        package = self.package("SPDXRef-Package-FFmpeg-Gyan")

        self.assertEqual(package["name"], "FFmpeg essentials build by Gyan.dev")
        self.assertEqual(package["versionInfo"], locked["version"])
        self.assertEqual(package["downloadLocation"], locked["url"])
        self.assertEqual(
            package["checksums"],
            [{"algorithm": "SHA256", "checksumValue": locked["sha256"]}],
        )
        self.assertEqual(package["licenseDeclared"], "GPL-3.0-or-later")
        self.assertEqual(package["licenseConcluded"], "NOASSERTION")
        self.assertEqual(
            package["copyrightText"],
            "Copyright (c) 2000-2026 the FFmpeg developers",
        )
        self.assertIn(locked["exe_sha256"], package["sourceInfo"])
        self.assertIn(locked["ffprobe_exe_sha256"], package["sourceInfo"])
        self.assertIn("not redistributed", package["sourceInfo"])
        self.assert_build_tool_relationship(package["SPDXID"])

        for fact in (
            "### FFmpeg 8.1.2 essentials build",
            locked["url"],
            locked["sha256"],
            locked["exe_sha256"],
            locked["ffprobe_exe_sha256"],
            "GPL-3.0-or-later",
        ):
            self.assertIn(fact, self.notices)

        self.assertIn("FFmpeg", self.readme)

    def test_project_license_is_distinct_and_blocks_publication(self) -> None:
        project = self.package(PROJECT_ID)
        normalized_notices = " ".join(self.notices.split())
        normalized_readme = " ".join(self.readme.split())
        normalized_product_spec = " ".join(self.product_spec.split())

        self.assertEqual(list(ROOT.glob("LICENSE*")), [])
        self.assertEqual(project["licenseDeclared"], "NOASSERTION")
        self.assertEqual(project["licenseConcluded"], "NOASSERTION")

        for fact in (
            "## Project licensing status",
            "apply only to the corresponding third-party components",
            "do not declare a license for project-authored code, assets, or other materials",
            "Public or commercial publication remains blocked",
        ):
            self.assertIn(fact, normalized_notices)

        for fact in (
            "## Licenciamento e publicação",
            "não constituem uma licença para o projeto",
            "publicação pública ou comercial permanece bloqueada",
        ):
            self.assertIn(fact, normalized_readme)

        for fact in (
            "**Gate de publicação — licença do projeto:**",
            "decisão explícita do proprietário",
            "não substituem a licença ou os termos próprios do projeto",
        ):
            self.assertIn(fact, normalized_product_spec)

    def test_spdx_package_and_relationship_ids_are_unique(self) -> None:
        package_ids = [package["SPDXID"] for package in self.sbom["packages"]]
        self.assertEqual(len(package_ids), len(set(package_ids)))
        relationships = [
            (
                relationship["spdxElementId"],
                relationship["relationshipType"],
                relationship["relatedSpdxElement"],
            )
            for relationship in self.sbom["relationships"]
        ]
        self.assertEqual(len(relationships), len(set(relationships)))


if __name__ == "__main__":
    unittest.main()
