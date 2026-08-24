import json
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def gdscript_string_array(source: str, constant_name: str) -> list[str]:
    block = re.search(
        rf"(?ms)^const {re.escape(constant_name)} := \[(.*?)^\]$",
        source,
    )
    if block is None:
        return []
    return [
        "".join(re.findall(r'"([^"]*)"', line))
        for line in block.group(1).splitlines()
        if '"' in line
    ]


class VerticalSliceToolingContractsTest(unittest.TestCase):
    def test_finding64_smoke_keeps_engine_options_before_user_args(self) -> None:
        runner = (ROOT / "tools" / "test_vertical_slice.ps1").read_text(
            encoding="utf-8"
        )
        smoke_path = ROOT / "tools" / "tests" / "vertical-slice-finding64-smoke.ps1"
        smoke = smoke_path.read_text(encoding="utf-8")

        invocation = (
            r"& (Join-Path $PSScriptRoot "
            r"'tests\vertical-slice-finding64-smoke.ps1') -Root $root"
        )
        self.assertEqual(runner.count(invocation), 1)
        self.assertLess(runner.index(invocation), runner.index("$visualGateRequested"))

        script_index = smoke.index("'--script', 'res://tests/vertical_slice_smoke.gd'")
        separator_index = smoke.index("'--', '--finding64-only'")
        self.assertLess(script_index, separator_index)
        self.assertIn("-ProcessIdentityToken $runToken", smoke)
        self.assertIn("AIM_COALESCING_SMOKE_OK", smoke)

    def test_finding64_smoke_requests_tree_quit_explicitly(self) -> None:
        smoke = (ROOT / "game" / "tests" / "vertical_slice_smoke.gd").read_text(
            encoding="utf-8"
        )

        focused_start = smoke.index('if "--finding64-only"')
        focused_end = smoke.index("\n\tif not _launch.begin_aim()", focused_start)
        focused_branch = smoke[focused_start:focused_end]
        self.assertIn("get_tree().quit(0)", focused_branch)

    def test_input_feedback_smoke_runs_once_before_visual_mode_branch(self) -> None:
        runner = (ROOT / "tools" / "test_vertical_slice.ps1").read_text(
            encoding="utf-8"
        )
        smoke_name = "vertical-slice-input-feedback-smoke.ps1"
        invocation = (
            r"& (Join-Path $PSScriptRoot "
            r"'tests\vertical-slice-input-feedback-smoke.ps1') -Root $root"
        )

        self.assertEqual(runner.count(smoke_name), 1)
        self.assertEqual(runner.count(invocation), 1)
        self.assertLess(runner.index(invocation), runner.index("$visualGateRequested"))

    def test_readme_documents_two_phase_capture_review_certification(self) -> None:
        readme = (ROOT / "README.md").read_text(encoding="utf-8")

        for configuration in ("Debug", "Release"):
            self.assertIn(
                ".\\tools\\test_vertical_slice.ps1 "
                f"-Configuration {configuration} -CaptureOnly",
                readme,
            )
            self.assertIn(
                ".\\tools\\test_vertical_slice.ps1 "
                f"-Configuration {configuration} -UseExistingCapture",
                readme,
            )
        self.assertIn("VERTICAL_SLICE_CAPTURE_READY", readme)
        self.assertIn("CaptureOnly não cria nem aprova revisões", readme)
        self.assertIn("UseExistingCapture nunca limpa nem recaptura", readme)

    def test_visual_capture_and_certification_have_explicit_exclusive_phases(self) -> None:
        runner = (ROOT / "tools" / "test_vertical_slice.ps1").read_text(
            encoding="utf-8"
        )

        self.assertRegex(
            runner,
            r"\[CmdletBinding\(DefaultParameterSetName\s*=\s*'Legacy'\)\]",
        )
        self.assertRegex(
            runner,
            r"\[Parameter\(ParameterSetName\s*=\s*'CaptureOnly'\)\]\s*"
            r"\[switch\]\$CaptureOnly",
        )
        self.assertRegex(
            runner,
            r"\[Parameter\(ParameterSetName\s*=\s*'UseExistingCapture'\)\]\s*"
            r"\[switch\]\$UseExistingCapture",
        )
        self.assertRegex(
            runner,
            r"\$visualGateRequested\s*=\s*\$IncludeVisualGate\s+-or\s+"
            r"\$CaptureOnly\s+-or\s+\$UseExistingCapture",
        )
        self.assertIn("VerticalSliceCaptureWorkflow.psm1", runner)
        self.assertIn("vertical-slice-capture-workflow-tests.ps1", runner)
        self.assertEqual(runner.count("Invoke-NinhoVerticalSliceCaptureWorkflow"), 1)
        self.assertRegex(
            runner,
            r"(?s)Invoke-NinhoVerticalSliceCaptureWorkflow.*?"
            r"Import-Module \(Join-Path \$PSScriptRoot 'VerticalSliceGate\.psm1'\) "
            r"-Force -Scope Local.*?"
            r"\$testedInputs\s*=\s*VerticalSliceGate\\Get-NinhoTestedInputs",
        )
        for command in (
            "Assert-NinhoAgentReviews",
            "Assert-NinhoRelativeArtifactPath",
            "Assert-NinhoCleanRoomReview",
            "Resolve-NinhoPackageManifestOutput",
            "Publish-NinhoVerticalSliceEvidence",
        ):
            self.assertIn(f"VerticalSliceGate\\{command}", runner)
        self.assertIn("SafePath\\Assert-NinhoNoReparseAncestors", runner)
        self.assertRegex(
            runner,
            r"(?s)if\s*\(\$CaptureOnly\).*?VERTICAL_SLICE_CAPTURE_READY.*?exit 0",
        )
        self.assertLess(
            runner.index("VERTICAL_SLICE_CAPTURE_READY"),
            runner.index("vertical-slice-reviews.json"),
        )
        self.assertLess(
            runner.index("Assert-NinhoCleanRoomReview"),
            runner.index("$package ="),
        )
        self.assertIn(
            "-ExpectedTestedInputsSha256 ([string]$testedInputs.sha256)",
            runner,
        )
        self.assertNotIn(
            "Remove-Item -LiteralPath $resolved -Recurse -Force",
            runner,
        )

    def test_release_package_canonicalizes_third_party_text_eol(self) -> None:
        attributes = (ROOT / ".gitattributes").read_text(encoding="utf-8")
        package = (ROOT / "tools" / "package_windows.ps1").read_text(encoding="utf-8")
        package_tests = (ROOT / "tools" / "tests" / "package-windows-tests.ps1").read_text(
            encoding="utf-8"
        )

        self.assertRegex(
            attributes,
            r"(?m)^third_party/\*\.txt text eol=lf(?:\s|$)",
        )
        self.assertIn("TestedInputIdentity.psm1", package)
        self.assertIn("Get-NinhoCanonicalTestedInputContent", package)
        self.assertIn("Package legal text is not canonical UTF-8 LF", package)
        self.assertIn("Get-NinhoCanonicalTestedInputContent", package_tests)
        self.assertNotRegex(
            package,
            r"(?s)foreach \(\$name in \$licenseSources\.Keys\).*?"
            r"Copy-Item -LiteralPath \$source -Destination \$target",
        )

    def test_agent_reviews_do_not_self_attest_human_playtest(self) -> None:
        runner = (ROOT / "tools" / "test_vertical_slice.ps1").read_text(encoding="utf-8")
        gate = (ROOT / "tools" / "VerticalSliceGate.psm1").read_text(encoding="utf-8")
        report = (ROOT / "tools" / "generate_vertical_slice_report.py").read_text(
            encoding="utf-8"
        )

        self.assertIn("status='not_performed'", runner)
        self.assertIn("participants=0", runner)
        self.assertIn("substitute='none'", runner)
        self.assertNotIn("substitute='independent_agents'", runner)
        self.assertNotIn("Assert-NinhoIndependentReviews", gate)
        self.assertIn("Assert-NinhoAgentReviews", gate)
        self.assertIn("human playtest record must remain pending", gate)
        self.assertNotIn("Independent substitute review", report)

    def test_evidence_does_not_self_attest_prerequisite_acceptance(self) -> None:
        runner = (ROOT / "tools" / "test_vertical_slice.ps1").read_text(encoding="utf-8")
        gate = (ROOT / "tools" / "VerticalSliceGate.psm1").read_text(encoding="utf-8")

        self.assertNotRegex(runner, r"(?m)^\s*acceptance\s*=")
        self.assertNotIn("$doc.acceptance", gate)
        self.assertNotIn("acceptance gate failed", gate)

    def test_clean_room_approval_comes_from_a_separate_bound_review(self) -> None:
        runner = (ROOT / "tools" / "test_vertical_slice.ps1").read_text(encoding="utf-8")
        gate = (ROOT / "tools" / "VerticalSliceGate.psm1").read_text(encoding="utf-8")

        self.assertNotIn("approved=$true", runner)
        self.assertNotIn("comparative_review='approved'", runner)
        self.assertNotIn("$doc.clean_room.approved", gate)
        self.assertIn("clean_room_review", runner)
        self.assertIn("Assert-NinhoCleanRoomReview", gate)

    def test_release_package_build_disables_tests_and_test_facades(self) -> None:
        build = (ROOT / "tools" / "build.ps1").read_text(encoding="utf-8")
        package = (ROOT / "tools" / "package_windows.ps1").read_text(encoding="utf-8")
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

        self.assertRegex(build, r"\[switch\]\$ProductionPackage")
        self.assertRegex(
            build,
            r"\[Parameter\(Mandatory\)\]\[bool\]\$BuildProductionPackage",
        )
        self.assertRegex(
            build,
            r"(?s)if\s*\(\$BuildProductionPackage\)\s*\{\s*"
            r"\$configureArguments\s*\+=\s*@\(\s*"
            r"'-DBUILD_TESTING=OFF'\s*,\s*"
            r"'-DNINHO_ENABLE_TEST_FACADES=OFF'\s*\)",
        )
        self.assertRegex(
            build,
            r"(?s)if\s*\(\$EnableGodot\).*?\$configureArguments\s*\+=\s*@\(\s*"
            r"'-DNINHO_BUILD_GDEXTENSION=ON'\s*,\s*"
            r'"-DNINHO_GODOT_EXECUTABLE=\$env:NINHO_GODOT_EXECUTABLE"\s*\)',
        )
        self.assertIn("& cmake @configureArguments", build)
        self.assertRegex(
            build,
            r"(?s)-Operation\s+\$buildOperation\s+-ArgumentList\s+@\(\s*"
            r"\$preset\s*,\s*\$WithGodot\.IsPresent\s*,\s*"
            r"\$ProductionPackage\.IsPresent\s*\)",
        )
        self.assertNotIn("$command =", build)
        self.assertNotIn("-Command $command", build)
        self.assertRegex(package, r"(?s)build\.ps1.*?-ProductionPackage")
        self.assertIn("option(NINHO_ENABLE_TEST_FACADES", cmake)
        self.assertIn("ninho-build-contract.json", cmake)

    def test_normal_build_reenables_tests_after_package_configuration(self) -> None:
        build = (ROOT / "tools" / "build.ps1").read_text(encoding="utf-8")

        self.assertRegex(
            build,
            r"(?s)if\s*\(\$BuildProductionPackage\).*?else\s*\{\s*"
            r"\$configureArguments\s*\+=\s*@\(\s*"
            r"'-DBUILD_TESTING=ON'\s*,\s*"
            r"'-DNINHO_ENABLE_TEST_FACADES=ON'\s*\)\s*\}",
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
        captured_process = (ROOT / "tools" / "CapturedProcess.psm1").read_text(
            encoding="utf-8"
        )
        process_variables = re.findall(
            r"\$(\w+Process)\s*=\s*Start-Process\b(?:(?!\n\s*\$\w).)*?-PassThru",
            package,
            flags=re.DOTALL,
        )

        self.assertCountEqual(
            process_variables,
            ["bootstrapProcess", "exportProcess", "launchProcess"],
        )
        for variable in process_variables:
            self.assertRegex(
                package,
                rf"(?s)finally\s*\{{.*?\${variable}\.Dispose\(\)",
                f"${variable} must be disposed from a finally block",
            )
        self.assertIn("PassThru = $true", captured_process)
        self.assertRegex(
            captured_process,
            r"(?s)finally\s*\{.*?\$process\.Dispose\(\)",
            "$process must be disposed from the capture module's finally block",
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

    def test_performance_capture_uses_the_pinned_console_runner(self) -> None:
        capture = (ROOT / "tools" / "capture_vertical_slice.ps1").read_text(
            encoding="utf-8"
        )
        lock = json.loads((ROOT / "tools" / "toolchain.lock.json").read_text())

        self.assertEqual(
            lock["godot"]["console_exe"],
            "Godot_v4.5.1-stable_win64_console.exe",
        )
        self.assertRegex(lock["godot"]["console_exe_sha256"], r"^[0-9a-f]{64}$")
        self.assertIn("-ExecutableProperty 'console_exe'", capture)
        self.assertIn("-HashProperty 'console_exe_sha256'", capture)
        self.assertIn("$runner = if ($Metrics) { $godotConsole } else { $godot }", capture)
        self.assertIn("$windowStyle = if ($Metrics) { 'Inherited' } else { 'Hidden' }", capture)
        self.assertIn("-FilePath $runner", capture)
        self.assertIn("-WindowStyle $windowStyle", capture)

    def test_virela_golden_uses_a_logged_ability_event(self) -> None:
        controller = (
            ROOT / "game" / "scripts" / "game" / "vertical_slice_controller.gd"
        ).read_text(encoding="utf-8")
        capture = (ROOT / "tools" / "capture_vertical_slice.ps1").read_text(
            encoding="utf-8"
        )
        gate = (ROOT / "tools" / "VerticalSliceGate.psm1").read_text(
            encoding="utf-8"
        )

        self.assertCountEqual(
            gdscript_string_array(controller, "CAPTURE_ABILITY_EVENT_KINDS"),
            ("ability_started", "ability_pulse"),
        )
        self.assertIn("kind not in CAPTURE_ABILITY_EVENT_KINDS", controller)
        self.assertIn("Get-NinhoVirelaAbilityProof", capture)
        self.assertNotIn("virela=@($virelaState,5)", capture)
        self.assertIn("Get-NinhoVirelaAbilityProof -Text $vulkanLog", gate)
        self.assertIn("Virela ability metadata differs from verified Vulkan log", gate)
        self.assertNotIn("'overview','aim','virela','result'", gate)

    def test_physics_scanner_covers_node_and_server_physics_entry_points(self) -> None:
        scanner = (ROOT / "game" / "tests" / "forbid_godot_physics.gd").read_text(
            encoding="utf-8"
        )
        source_fragments = gdscript_string_array(scanner, "FORBIDDEN_SOURCE_FRAGMENTS")
        runtime_bases = gdscript_string_array(scanner, "FORBIDDEN_RUNTIME_BASE_CLASSES")

        forbidden_sources = (
            "extends AnimatableBody3D",
            "extends VehicleBody3D",
            "extends PhysicalBone3D",
            "extends SoftBody3D",
            "var joint := HingeJoint3D.new()",
            "var ray := RayCast3D.new()",
            "var shape := ShapeCast3D.new()",
            "var polygon := CollisionPolygon3D.new()",
            "PhysicsServer3D.space_create()",
            "get_world_3d().direct_space_state.intersect_ray(query)",
        )
        for source in forbidden_sources:
            self.assertTrue(
                any(fragment in source for fragment in source_fragments),
                f"scanner permits Godot 3D physics source: {source}",
            )

        self.assertCountEqual(
            runtime_bases,
            (
                "CollisionObject3D",
                "CollisionShape3D",
                "CollisionPolygon3D",
                "Joint3D",
                "RayCast3D",
                "ShapeCast3D",
            ),
        )

    def test_body_view_interpolation_does_not_materialize_dictionary_keys(self) -> None:
        registry = (
            ROOT / "game" / "scripts" / "game" / "body_view_registry.gd"
        ).read_text(encoding="utf-8")
        apply_frame = registry.split("func apply_frame", 1)[1].split("\nfunc ", 1)[0]
        process = registry.split("func _process", 1)[1].split("\nfunc ", 1)[0]

        self.assertIn("for key: String in _views.keys():", apply_frame)
        self.assertIn("for key: String in _views:", process)
        self.assertNotIn("_views.keys()", process)

    def test_route_trace_contract_versions_include_damage_classification(self) -> None:
        playthrough = (
            ROOT / "native" / "simulation" / "tests" / "playthrough_tests.cpp"
        ).read_text(encoding="utf-8")
        capture = (ROOT / "tools" / "capture_vertical_slice.ps1").read_text(
            encoding="utf-8"
        )
        gate = (ROOT / "tools" / "VerticalSliceGate.psm1").read_text(
            encoding="utf-8"
        )
        gate_tests = (
            ROOT / "tools" / "tests" / "vertical-slice-gate-tests.ps1"
        ).read_text(encoding="utf-8")

        self.assertEqual(playthrough.count("event.damage_classification"), 1)
        self.assertIn("ordered_event_bytes(trace)", playthrough)
        for source in (playthrough, capture, gate, gate_tests):
            self.assertIn("canonical_playthrough_v4", source)
            self.assertIn("ordered_events_v2", source)
            self.assertNotIn("canonical_playthrough_v3", source)
            self.assertNotIn("ordered_events_v1", source)


if __name__ == "__main__":
    unittest.main()
