"""Generate the normative vertical-slice report from exactly two evidence files."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

EVIDENCE = {
    "Debug": Path("docs/gameplay/evidence/vertical-slice-debug.json"),
    "Release": Path("docs/gameplay/evidence/vertical-slice-release.json"),
}
REPORT = Path("docs/gameplay/vertical-slice-report.md")


def canonical_text_bytes(raw: bytes) -> bytes:
    """Return a checkout-independent UTF-8 representation for tracked text."""
    text = raw.decode("utf-8")
    return text.replace("\r\n", "\n").replace("\r", "\n").encode("utf-8")


def load(root: Path) -> list[tuple[str, Path, bytes, dict]]:
    result = []
    for configuration, relative in EVIDENCE.items():
        path = root / relative
        if not path.is_file():
            raise ValueError(f"evidence missing: {relative.as_posix()}")
        raw = canonical_text_bytes(path.read_bytes())
        doc = json.loads(raw)
        if doc.get("schema") != "ninho.vertical-slice.evidence.v1" or doc.get("schema_version") != 1:
            raise ValueError(f"evidence schema/version mismatch: {relative.as_posix()}")
        if doc.get("configuration") != configuration:
            raise ValueError(f"evidence configuration mismatch: {relative.as_posix()}")
        result.append((configuration, relative, raw, doc))
    return result


def render(entries: list[tuple[str, Path, bytes, dict]]) -> str:
    lines = [
        "# First Orbit Vertical Slice Certification", "",
        "Generated from the two tracked evidence snapshots. Manual edits are rejected by `--check`.", "",
        "Normative-Certification-Identity: Tested-Inputs-SHA256 (content-addressed and revalidated against the current tree).", "",
    ]
    for configuration, relative, raw, doc in entries:
        lines += [
            f"Evidence-{configuration}-Path: {relative.as_posix()}",
            f"Evidence-{configuration}-SHA256: {hashlib.sha256(raw).hexdigest()}",
            f"Generation-Head-{configuration}: {doc['source_revision']}",
            f"Tested-Inputs-{configuration}-SHA256: {doc['tested_inputs_sha256']}",
            f"Capture-Manifest-{configuration}-SHA256: {doc['capture_manifest']['sha256']}", "",
        ]
    lines += ["## Gate summary", "", "| Build | Physics p95 | Vulkan p95/p99 | OpenGL p95/p99 |", "| --- | ---: | ---: | ---: |"]
    for configuration, _, _, doc in entries:
        renderer = {item["name"]: item for item in doc["renderers"]}
        vk = renderer["Vulkan"]["scales"][0]
        gl = renderer["OpenGL"]["scales"][0]
        lines.append(f"| {configuration} | {doc['physics']['step_p95_ms']:.3f} ms | {vk['frame_p95_ms']:.3f}/{vk['frame_p99_ms']:.3f} ms | {gl['frame_p95_ms']:.3f}/{gl['frame_p99_ms']:.3f} ms |")
    release = entries[1][3]
    lines += [
        "", "## Deterministic routes", "", "| Route | Outcome | Repeat hashes |", "| --- | --- | --- |",
    ]
    for route in release["routes"]:
        lines.append(f"| `{route['name']}` | `{route['outcome']}` | `{' = '.join(route['hashes'])}` |")
    lines += [
        "", "## Visual evidence", "",
        "Release goldens are the canonical certification images; Debug goldens are diagnostic and remain configuration-specific.", "",
        "| Golden | Source frame | Transition frame | SHA-256 |", "| --- | ---: | ---: | --- |",
    ]
    for golden in release["golden_metadata"]:
        lines.append(
            f"| `{golden['name']}` | {golden['source_frame']} | "
            f"{golden['source_transition_frame']} | `{golden['sha256']}` |"
        )
    renderer = {item["name"]: item for item in release["renderers"]}
    lines += [
        "",
        f"The 300-frame Vulkan and OpenGL records were deterministically sampled from normal-cadence source captures of "
        f"{renderer['Vulkan']['capture']['source_frames']} and {renderer['OpenGL']['capture']['source_frames']} frames.",
    ]
    lines += [
        "", "## Review and clean-room", "",
        f"- Independent substitute review: `{release['playtest']['substitute']}`; five new human players were `{release['playtest']['status']}`.",
        "- The comparative clean-room review covers names, logos, silhouettes, sounds, UI, layouts, and promotional material.",
        "- Virela, Nox, and Talo remain codenames pending clearance.",
        "- This engineering review is not a legal opinion and does not replace legal counsel before commercial publication.",
        f"- Review manifest SHA-256: `{release['reviews_manifest']['sha256']}`.",
        "", "## Windows package", "",
        f"- Path: `{release['package']['path']}`",
        f"- Manifest SHA-256: `{release['package']['manifest_sha256']}`",
        f"- Launch from path containing spaces: `{release['package']['launch_from_space_path']}`",
        "", "## Verdict", "", "All recorded blocking gates passed for the certified vertical slice.", "",
    ]
    return "\n".join(lines)


def safe_report(root: Path) -> Path:
    root = Path(os.path.abspath(root))
    report = Path(os.path.abspath(root / REPORT))
    report.relative_to(root)
    current = root
    for part in REPORT.parts:
        current /= part
        if current.exists() and current.is_symlink():
            raise ValueError(f"report path contains symlink: {current}")
    return report


def validate_evidence(root: Path) -> None:
    checker = Path(__file__).with_name("check_vertical_slice_evidence.ps1")
    try:
        result = subprocess.run(
            ["powershell.exe", "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass",
             "-File", str(checker), "-Root", str(root)],
            cwd=root, capture_output=True, text=True, encoding="utf-8", errors="replace",
            timeout=600,
        )
    except (OSError, subprocess.TimeoutExpired) as error:
        raise ValueError(f"full evidence gate could not run: {error}") from error
    if result.returncode != 0:
        diagnostic = (result.stdout + "\n" + result.stderr).strip()
        diagnostic = diagnostic.encode("ascii", "backslashreplace").decode("ascii")
        raise ValueError(f"full evidence gate rejected report inputs: {diagnostic}")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true")
    mode.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    try:
        root = Path(os.path.abspath(args.root))
        expected = render(load(root)).encode("utf-8")
        report = safe_report(root)
        if args.check:
            validate_evidence(root)
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
        print(f"vertical slice report generation failed: {error}", file=sys.stderr)
        return 2
    if args.check:
        try:
            actual = report.read_bytes()
        except OSError as error:
            print(f"vertical slice report is out of date: {error}", file=sys.stderr)
            return 1
        if actual != expected:
            print("vertical slice report is out of date; run with --write", file=sys.stderr)
            return 1
        return 0
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_bytes(expected)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
