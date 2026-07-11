"""Pure validation for authored config and output path containment."""

from __future__ import annotations

import re
from pathlib import Path

ASSET_RE = re.compile(r"^(AST|CHR|ENM|DEV|KIT)_[A-Za-z0-9_]+$")
FOLDER_RE = re.compile(r"^[a-z][a-z0-9_-]*$")


def validate_config_contract(config: dict) -> None:
    if config.get("schema_version") != 1 or not isinstance(config.get("seed"), int):
        raise ValueError("invalid asset config schema or seed")
    expected_coordinate = {
        "unit": "meter",
        "unit_scale": 1.0,
        "up_axis": "+Z",
        "forward_axis": "-Y",
        "pivot": "CENTER_OF_MASS",
    }
    if config.get("coordinate_system") != expected_coordinate:
        raise ValueError("coordinate contract mismatch")
    ids = [asset.get("id", "") for asset in config.get("assets", [])]
    if len(ids) != len(set(ids)) or any(not ASSET_RE.fullmatch(asset_id) for asset_id in ids):
        raise ValueError("asset IDs are duplicated or invalid")
    if any("-col" in asset_id.lower() for asset_id in ids):
        raise ValueError("Godot -col suffix is forbidden")
    for asset in config.get("assets", []):
        folder = asset.get("folder", "")
        if not isinstance(folder, str) or not FOLDER_RE.fullmatch(folder):
            raise ValueError(f"invalid asset folder: {folder!r}")


def safe_output_path(output_root: Path, relative: Path) -> Path:
    root = output_root.resolve()
    candidate = (root / relative).resolve(strict=False)
    if candidate != root and root not in candidate.parents:
        raise ValueError(f"output path is outside output root: {relative}")
    return candidate
