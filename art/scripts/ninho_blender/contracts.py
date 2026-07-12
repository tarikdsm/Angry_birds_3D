"""Pure validation for authored config and output path containment."""

from __future__ import annotations

import re
from pathlib import Path

ASSET_RE = re.compile(r"^(AST|CHR|ENM|DEV|KIT)_[A-Za-z0-9_]+$")
FOLDER_RE = re.compile(r"^[a-z][a-z0-9_-]*$")
GLOBAL_BUDGET_KEYS = ("triangles", "draw_calls", "texture_bytes", "particles", "fragments")


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
    budgets = config.get("global_budgets", {})
    if any(not isinstance(budgets.get(key), int) or budgets[key] < 0 for key in GLOBAL_BUDGET_KEYS):
        raise ValueError("global budgets are missing or invalid")


def enforce_global_budgets(measured: dict[str, int], budgets: dict[str, int]) -> None:
    for key in GLOBAL_BUDGET_KEYS:
        if int(measured.get(key, 0)) > int(budgets[key]):
            raise ValueError(f"{key} global budget exceeded: {measured[key]} > {budgets[key]}")


def measure_instantiated_budgets(asset_metrics: dict[str, dict[str, int]], instance_asset_ids: list[str]) -> dict:
    result = {"triangles": 0, "draw_calls": 0, "fragments": 0}
    for asset_id in instance_asset_ids:
        if asset_id not in asset_metrics:
            raise ValueError(f"runtime instance references missing asset metrics: {asset_id}")
        for key in result:
            result[key] += int(asset_metrics[asset_id][key])
    return result


def validate_texture_bijection(authored_names: set[str], embedded_names: set[str]) -> None:
    if authored_names != embedded_names:
        orphaned = sorted(authored_names - embedded_names)
        missing = sorted(embedded_names - authored_names)
        raise ValueError(f"texture bijection mismatch: orphaned={orphaned}, missing={missing}")


def safe_output_path(output_root: Path, relative: Path) -> Path:
    root = output_root.resolve()
    candidate = (root / relative).resolve(strict=False)
    if candidate != root and root not in candidate.parents:
        raise ValueError(f"output path is outside output root: {relative}")
    return candidate
