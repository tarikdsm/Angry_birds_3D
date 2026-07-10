import json
import pathlib
import subprocess
import sys
import tempfile


def main() -> int:
    executable = pathlib.Path(sys.argv[1])
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
            return completed.returncode
        with report.open("r", encoding="utf-8") as stream:
            document = json.load(stream)
        if document["schema"] != "ninho.physics.scenario.v1":
            raise AssertionError("unexpected JSON schema")
        if document["dependencies"]["box3d"]["commit"] != "8441b4a06d6d09dcfb0b0f704df4d847d1437b92":
            raise AssertionError("unexpected Box3D commit")
        if document["dependencies"]["godot"]["commit"] != "f62fdbde15035c5576dad93e586201f4d41ef0cb":
            raise AssertionError("unexpected Godot commit")
        if document["dependencies"]["godot_cpp"]["commit"] != "e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77":
            raise AssertionError("unexpected godot-cpp commit")
        if len(document["scenarios"]) != 6:
            raise AssertionError("expected all six scenarios")
        for scenario in document["scenarios"]:
            if len(scenario["hashes"]) != 2 or len(set(scenario["hashes"])) != 1:
                raise AssertionError(f"non-deterministic hashes: {scenario['name']}")
        hashes = {scenario["name"]: scenario["hashes"][0] for scenario in document["scenarios"]}
        if hashes["capability_matrix"] == hashes["radial_pile"]:
            raise AssertionError("capability matrix reused the radial pile hash")
        if len(document["matrix"]) != 8:
            raise AssertionError("expected all eight capability rows")
        if any(row["status"] == "blocked" for row in document["matrix"]):
            raise AssertionError("capability matrix contains a blocked row")
        if any(not row["fixture_hashes"] for row in document["matrix"]):
            raise AssertionError("capability matrix row is missing fixture hashes")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
