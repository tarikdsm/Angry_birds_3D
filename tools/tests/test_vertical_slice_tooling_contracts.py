import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class VerticalSliceToolingContractsTest(unittest.TestCase):
    def test_release_package_build_disables_tests_and_test_facades(self) -> None:
        build = (ROOT / "tools" / "build.ps1").read_text(encoding="utf-8")
        package = (ROOT / "tools" / "package_windows.ps1").read_text(encoding="utf-8")
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

        self.assertRegex(build, r"\[switch\]\$ProductionPackage")
        self.assertRegex(
            build,
            r"(?s)if\s*\(\$ProductionPackage\).*?-DBUILD_TESTING=OFF.*?"
            r"-DNINHO_ENABLE_TEST_FACADES=OFF",
        )
        self.assertRegex(package, r"(?s)build\.ps1.*?-ProductionPackage")
        self.assertIn("option(NINHO_ENABLE_TEST_FACADES", cmake)
        self.assertIn("ninho-build-contract.json", cmake)

    def test_normal_build_reenables_tests_after_package_configuration(self) -> None:
        build = (ROOT / "tools" / "build.ps1").read_text(encoding="utf-8")

        self.assertRegex(
            build,
            r"(?s)else\s*\{\s*' -DBUILD_TESTING=ON "
            r"-DNINHO_ENABLE_TEST_FACADES=ON'\s*\}",
        )

    def test_package_records_and_verifies_native_build_contract(self) -> None:
        package = (ROOT / "tools" / "package_windows.ps1").read_text(encoding="utf-8")

        self.assertGreaterEqual(package.count("native_build_contract"), 4)
        self.assertIn("build-contract.json", package)
        self.assertRegex(package, r"build_testing\s+-cne\s+\$false")
        self.assertRegex(package, r"test_facades\s+-cne\s+\$false")
        self.assertRegex(package, r"NINHO_ENABLE_TEST_FACADES", package)

    def test_every_passthru_process_is_disposed_in_finally(self) -> None:
        package = (ROOT / "tools" / "package_windows.ps1").read_text(encoding="utf-8")
        process_variables = re.findall(
            r"\$(\w+Process)\s*=\s*Start-Process\b(?:(?!\n\s*\$\w).)*?-PassThru",
            package,
            flags=re.DOTALL,
        )

        self.assertCountEqual(
            process_variables,
            ["buildProcess", "bootstrapProcess", "exportProcess", "launchProcess"],
        )
        for variable in process_variables:
            self.assertRegex(
                package,
                rf"(?s)finally\s*\{{.*?\${variable}\.Dispose\(\)",
                f"${variable} must be disposed from a finally block",
            )

    def test_capture_preserves_runtime_metric_decimal_tokens(self) -> None:
        capture = (ROOT / "tools" / "capture_vertical_slice.ps1").read_text(encoding="utf-8")
        decimal_fields = (
            "frame_p95_ms",
            "frame_p99_ms",
            "max_hitch_ms",
            "input_feedback_p95_ms",
            "physics_step_p95_ms",
        )
        for field in decimal_fields:
            self.assertIn(f"{field} = $document.{field}", capture)
            self.assertNotIn(f"{field} = [double]$document.{field}", capture)


if __name__ == "__main__":
    unittest.main()
