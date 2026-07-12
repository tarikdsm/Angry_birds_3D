"""Build or validate the original first-orbit Blender asset set."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import shutil
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Vector

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from ninho_blender.contracts import (
    enforce_global_budgets,
    measure_instantiated_budgets,
    safe_output_path,
    validate_config_contract,
    validate_texture_bijection,
)
from ninho_blender.export import (
    collect_asset_record,
    inspect_glb,
    mesh_triangles,
    save_source_and_glb,
    semantic_scene_sha256,
    sha256_file,
)
from ninho_blender.geometry import apply_authored_uvs, build_asset, reset_scene
from ninho_blender.materials import (
    build_materials,
    material_semantics_sha256,
    runtime_material_semantics,
    validate_runtime_material_semantics,
    write_runtime_materials,
)
from ninho_blender.textures import decoded_rgba8_mip_bytes, encode_png_rgba, generate_material_rgba, inspect_png_rgba

NAME_RE = re.compile(r"^(VIS|COL|FRAG|SOCKET|RIG)_[A-Za-z0-9_]+$")
ASSET_RE = re.compile(r"^(AST|CHR|ENM|DEV|KIT)_[A-Za-z0-9_]+$")


def canonical_json(value: object) -> str:
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def parse_arguments() -> argparse.Namespace:
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-root", type=Path, required=True)
    parser.add_argument("--output-root", type=Path, required=True)
    parser.add_argument("--blender-executable-sha256", required=True)
    parser.add_argument("--validate", action="store_true")
    return parser.parse_args(arguments)


def read_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as stream:
        return json.load(stream)


def validate_config(config: dict) -> None:
    if bpy.app.version_string != "5.1.2":
        raise RuntimeError(f"Blender 5.1.2 required, got {bpy.app.version_string}")
    validate_config_contract(config)


def build_proxies(project_root: Path, config: dict) -> list[dict]:
    level = read_json(project_root / "game/data/levels/first_orbit.level.json")
    assets = {asset["id"]: asset for asset in config["assets"]}
    level_proxies = []
    for body in level["bodies"]:
        asset_id = body["visual"]["asset_id"]
        if asset_id not in assets:
            raise RuntimeError(f"level references missing authored asset: {asset_id}")
        level_proxies.append(
            {
                "body_id": body["body_id"],
                "entity_id": body["entity_id"],
                "part_id": body["part_id"],
                "asset_id": asset_id,
                "bounds_m": body["visual"]["bounds_m"],
                "transform": body["transform"],
            }
        )
    canonical = config["level_layout"]["proxies"]
    if level["id"] != config["level_layout"]["level_id"] or canonical_json(level_proxies) != canonical_json(canonical):
        raise RuntimeError("level layout differs from authored config")
    for proxy in canonical:
        if assets[proxy["asset_id"]]["bounds_m"] != proxy["bounds_m"]:
            raise RuntimeError(f"proxy bounds differ from asset config for body {proxy['body_id']}")
    return canonical


def compute_pipeline_hash(config_path: Path, executable_hash: str) -> str:
    digest = hashlib.sha256()
    pipeline_paths = [Path(__file__).resolve()] + sorted((SCRIPT_DIR / "ninho_blender").glob("*.py"))
    for pipeline_path in pipeline_paths:
        digest.update(pipeline_path.name.encode("utf-8"))
        digest.update(pipeline_path.read_bytes())
    digest.update(config_path.read_bytes())
    digest.update(executable_hash.encode("ascii"))
    return digest.hexdigest()


def compute_build_hash(
    generator: dict,
    outputs: list[dict],
    sources: list[dict],
    proxies: list[dict],
    global_budgets: dict,
    material_hash: str,
) -> str:
    source_semantics = [
        {"path": source["path"], "semantic_sha256": source["semantic_sha256"]}
        for source in sorted(sources, key=lambda item: item["path"])
    ]
    material = canonical_json(
        {
            "generator": generator,
            "outputs": sorted(outputs, key=lambda item: item["path"]),
            "source_semantics": source_semantics,
            "proxies": proxies,
            "global_budgets": global_budgets,
            "material_semantics_sha256": material_hash,
        }
    )
    return hashlib.sha256(material.encode("utf-8")).hexdigest()


def scene_particle_count(project_root: Path) -> int:
    scene_text = (project_root / "game/scenes/vertical_slice.tscn").read_text(encoding="utf-8")
    return sum(int(value) for value in re.findall(r"(?m)^amount = (\d+)$", scene_text))


def global_budget_record(measured: dict[str, int], configured: dict[str, int], texture_storage_bytes: int) -> dict:
    sources = {
        "triangles": "VIS LOD0/decor triangles multiplied by canonical runtime instances",
        "draw_calls": "VIS LOD0/decor material draws multiplied by canonical runtime instances",
        "texture_bytes": "decoded RGBA8 full mip chains for embedded images plus external glass override",
        "particles": "game/scenes/vertical_slice.tscn particle amount properties",
        "fragments": "FRAG definitions multiplied by canonical runtime instances",
    }
    enforce_global_budgets(measured, configured)
    result = {
        key: {"actual": int(measured[key]), "max": int(configured[key]), "source": sources[key]}
        for key in sources
    }
    result["texture_bytes"]["storage_bytes"] = int(texture_storage_bytes)
    result["texture_bytes"]["storage_source"] = "compressed embedded payloads plus external glass override PNG"
    return result


def write_asset_textures(config: dict, asset: dict, output_root: Path) -> tuple[dict[str, Path], list[dict]]:
    specs = {item["name"]: item for item in config["materials"]}
    paths = {}
    records = []
    for material_name in sorted(set(asset["materials"])):
        spec = specs[material_name]
        rgba = generate_material_rgba(config, spec)
        size = int(spec["texture"]["size_px"])
        payload = encode_png_rgba(size, size, rgba)
        relative = Path("game/assets/vertical_slice") / asset["folder"] / f"{asset['id']}_TEX_{material_name[4:]}.png"
        path = safe_output_path(output_root, relative)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(payload)
        contract = inspect_png_rgba(payload)
        if contract["unique_colors"] <= 1 or not contract["has_nonblack_rgb"]:
            raise RuntimeError(f"procedural texture is constant or black: {material_name}")
        paths[material_name] = path
        records.append(
            {
                "material": material_name,
                "image_name": f"TEX_{material_name[4:]}",
                "path": relative.as_posix(),
                "sha256": sha256_file(path),
                **contract,
            }
        )
    return paths, records


def expected_asset_textures(config: dict, asset: dict, output_root: Path) -> list[dict]:
    specs = {item["name"]: item for item in config["materials"]}
    records = []
    for material_name in sorted(set(asset["materials"])):
        spec = specs[material_name]
        size = int(spec["texture"]["size_px"])
        expected_payload = encode_png_rgba(size, size, generate_material_rgba(config, spec))
        relative = Path("game/assets/vertical_slice") / asset["folder"] / f"{asset['id']}_TEX_{material_name[4:]}.png"
        path = safe_output_path(output_root, relative)
        actual_payload = path.read_bytes()
        if actual_payload != expected_payload:
            raise RuntimeError(f"authored PNG differs from procedural source: {relative.as_posix()}")
        records.append(
            {
                "material": material_name,
                "image_name": f"TEX_{material_name[4:]}",
                "path": relative.as_posix(),
                "sha256": sha256_file(path),
                **inspect_png_rgba(actual_payload),
            }
        )
    return records


def asset_runtime_metrics(record: dict) -> dict[str, int]:
    visible = [
        item
        for item in record["objects"]
        if item["role"] == "VIS" and ("lod" not in item or int(item["lod"]) == 0)
    ]
    return {
        "triangles": sum(int(item["triangles"]) for item in visible),
        "draw_calls": sum(max(1, len(item["materials"])) for item in visible),
        "fragments": sum(1 for item in record["objects"] if item["role"] == "FRAG"),
    }


def validate_glb_material_contract(
    config: dict, asset_config: dict, glb_materials: list[dict], authored_textures: list[dict]
) -> None:
    specs = {item["name"]: item for item in config["materials"]}
    texture_names = {item["material"]: item["image_name"] for item in authored_textures}
    actual = {item["name"]: item for item in glb_materials}
    if set(actual) != set(asset_config["materials"]):
        raise RuntimeError(f"effective GLB materials differ from asset contract: {asset_config['id']}")
    for material_name in sorted(actual):
        material = actual[material_name]
        spec = specs[material_name]
        if material["base_color_image"] != texture_names[material_name]:
            raise RuntimeError(f"effective GLB texture binding mismatch: {material_name}")
        if any(not math.isclose(value, 1.0, abs_tol=1e-6) for value in material["base_color_factor"]):
            raise RuntimeError(f"effective GLB base color factor must preserve baked pixels: {material_name}")
        numeric_pairs = (
            (material["roughness_factor"], spec["roughness"]),
            (material["metallic_factor"], spec["metallic"]),
            (material["transmission_factor"], spec.get("transmission", 0.0)),
            (material["coat_factor"], spec.get("coat", 0.0)),
        )
        if any(not math.isclose(float(actual_value), float(expected), rel_tol=1e-5, abs_tol=1e-6) for actual_value, expected in numeric_pairs):
            raise RuntimeError(f"effective GLB PBR factors differ from config: {material_name}")
        expected_alpha_mode = "BLEND" if float(spec["alpha"]) < 1.0 else "OPAQUE"
        if material["alpha_mode"] != expected_alpha_mode:
            raise RuntimeError(f"effective GLB alpha mode differs from config: {material_name}")
        expected_emission = [
            float(value) * float(spec.get("emission_strength", 0.0))
            for value in config["palette"][spec["palette_key"]][:3]
        ]
        actual_emission = [
            float(value) * float(material["emissive_strength"])
            for value in material["emissive_factor"]
        ]
        if any(not math.isclose(a, e, rel_tol=1e-5, abs_tol=1e-6) for a, e in zip(actual_emission, expected_emission)):
            raise RuntimeError(f"effective GLB emission differs from config: {material_name}")
def build(project_root: Path, output_root: Path, executable_hash: str) -> None:
    config_path = project_root / "art/config/vertical_slice_assets.json"
    config = read_json(config_path)
    validate_config(config)
    pipeline_hash = compute_pipeline_hash(config_path, executable_hash)
    assets = []
    outputs = []
    sources = []
    measured = {"triangles": 0, "draw_calls": 0, "texture_bytes": 0, "particles": 0, "fragments": 0}
    texture_storage_bytes = 0
    config_sha256 = sha256_file(config_path)
    for asset in config["assets"]:
        reset_scene()
        texture_paths, authored_textures = write_asset_textures(config, asset, output_root)
        materials = build_materials(config, set(asset["materials"]), texture_paths)
        objects = build_asset(asset, config, materials)
        apply_authored_uvs(objects)
        record = collect_asset_record(asset, objects)
        source_relative = Path("art/source/vertical_slice") / f"{asset['id']}.blend"
        glb_relative = Path("game/assets/vertical_slice") / asset["folder"] / f"{asset['id']}.glb"
        source_path = safe_output_path(output_root, source_relative)
        glb_path = safe_output_path(output_root, glb_relative)
        semantic_hash = semantic_scene_sha256()
        save_source_and_glb(source_path, glb_path)
        glb_contract = inspect_glb(glb_path)
        authored_by_name = {item["image_name"]: item for item in authored_textures}
        validate_texture_bijection(set(authored_by_name), {item["name"] for item in glb_contract["images"]})
        for embedded in glb_contract["images"]:
            source = authored_by_name.get(embedded["name"])
            if source is None or embedded["sha256"] != source["sha256"] or embedded["pixel_sha256"] != source["pixel_sha256"]:
                raise RuntimeError(f"embedded texture differs from authored PNG: {embedded['name']}")
        validate_glb_material_contract(config, asset, glb_contract["materials"], authored_textures)
        record["source_blend"] = source_relative.as_posix()
        record["glb"] = glb_relative.as_posix()
        record["blend_file_sha256"] = sha256_file(source_path)
        record["blend_semantic_sha256"] = semantic_hash
        record["glb_sha256"] = sha256_file(glb_path)
        record["author"] = config["author"]
        record["license"] = config["license"]
        record["seed"] = config["seed"]
        record["source"] = config["source"]
        record["source_config_sha256"] = config_sha256
        record["texture_images"] = glb_contract["images"]
        record["authored_textures"] = authored_textures
        record["texture_count"] = glb_contract["texture_count"]
        record["uv_layers"] = glb_contract["uv_primitives"]
        record["draw_calls"] = glb_contract["draw_calls"]
        record["glb_materials"] = glb_contract["materials"]
        measured["texture_bytes"] += int(glb_contract["decoded_texture_bytes"])
        texture_storage_bytes += int(glb_contract["texture_bytes"])
        assets.append(record)
        sources.append(
            {
                "path": source_relative.as_posix(),
                "file_sha256": record["blend_file_sha256"],
                "semantic_sha256": semantic_hash,
            }
        )
        outputs.append({"path": glb_relative.as_posix(), "sha256": record["glb_sha256"]})
        for texture in authored_textures:
            outputs.append({"path": texture["path"], "sha256": texture["sha256"]})

    for runtime_path in write_runtime_materials(config, output_root):
        relative = runtime_path.relative_to(output_root).as_posix()
        outputs.append({"path": relative, "sha256": sha256_file(runtime_path)})

    generator = {
        "pipeline_schema_version": 1,
        "blender_version": bpy.app.version_string,
        "blender_build_hash": bpy.app.build_hash.decode("ascii"),
        "blender_executable_sha256": executable_hash,
        "seed": config["seed"],
        "config_sha256": config_sha256,
        "pipeline_sha256": pipeline_hash,
        "unit": "meter",
        "up_axis": "+Z",
        "forward_axis": "-Y",
    }
    outputs.sort(key=lambda item: item["path"])
    proxies = build_proxies(project_root, config)
    runtime_instances = list(config["level_layout"]["runtime_singletons"]) + [item["asset_id"] for item in proxies]
    instantiated = measure_instantiated_budgets(
        {asset["id"]: asset_runtime_metrics(asset) for asset in assets}, runtime_instances
    )
    measured.update(instantiated)
    measured["particles"] = scene_particle_count(project_root)
    glass_asset_record = next(asset for asset in assets if asset["id"] == "KIT_GlassPanel_A")
    glass_texture = next(item for item in glass_asset_record["authored_textures"] if item["material"] == "MAT_Glass")
    measured["texture_bytes"] += decoded_rgba8_mip_bytes(int(glass_texture["width"]), int(glass_texture["height"]))
    texture_storage_bytes += int(safe_output_path(output_root, Path(glass_texture["path"])).stat().st_size)
    budgets = global_budget_record(measured, config["global_budgets"], texture_storage_bytes)
    runtime_semantics = runtime_material_semantics(output_root)
    validate_runtime_material_semantics(config, runtime_semantics)
    material_hash = material_semantics_sha256(runtime_semantics)
    manifest = {
        "schema_version": 1,
        "generator": generator,
        "build_hash": compute_build_hash(generator, outputs, sources, proxies, budgets, material_hash),
        "reproducibility_note": "Blender 5.1.2 blend binaries contain nondeterministic internal state; blend_semantic_sha256 is normative and blend_file_sha256 is informational.",
        "assets": assets,
        "proxies": proxies,
        "sources": sources,
        "outputs": outputs,
        "global_budgets": budgets,
        "material_semantics_sha256": material_hash,
        "auxiliary_material_outputs": [
            {"path": "game/materials/pine.tres", "role": "reusable editor preview; runtime PBR authority is MAT_Pine inside GLB"},
            {"path": "game/materials/brick.tres", "role": "reusable editor preview; runtime PBR authority is MAT_Brick inside GLB"},
        ],
        "non_normative_derivatives": [
            {"glob": "game/assets/vertical_slice/**/*.import", "reason": "Godot import metadata is regenerated by the pinned editor from normative PNG/GLB outputs"},
            {"glob": "game/.godot/**", "reason": "Godot local import cache is not a shipped authored output"},
        ],
    }
    manifest_path = safe_output_path(output_root, Path("tools/art/vertical_slice_asset_manifest.json"))
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(canonical_json(manifest) + "\n", encoding="utf-8", newline="\n")
    print(f"NINHO_BUILD_HASH={manifest['build_hash']}")


def is_convex_mesh(obj: bpy.types.Object) -> bool:
    if obj.type != "MESH" or len(obj.data.vertices) < 4:
        return False
    original = bmesh.new()
    hull = bmesh.new()
    try:
        original.from_mesh(obj.data)
        original_volume = abs(float(original.calc_volume(signed=True)))
        unique_positions = sorted(
            {tuple(round(float(axis), 7) for axis in vertex.co) for vertex in obj.data.vertices}
        )
        for position in unique_positions:
            hull.verts.new(position)
        hull.verts.ensure_lookup_table()
        result = bmesh.ops.convex_hull(hull, input=list(hull.verts), use_existing_faces=False)
        unused = list(result.get("geom_unused", [])) + list(result.get("geom_interior", []))
        if unused:
            bmesh.ops.delete(hull, geom=unused, context="VERTS")
        hull_volume = abs(float(hull.calc_volume(signed=True)))
        return math.isclose(original_volume, hull_volume, rel_tol=1e-4, abs_tol=1e-6)
    finally:
        original.free()
        hull.free()


def mesh_center_of_mass(obj: bpy.types.Object) -> Vector:
    obj.data.calc_loop_triangles()
    weighted = Vector((0.0, 0.0, 0.0))
    signed_volume = 0.0
    for triangle in obj.data.loop_triangles:
        a, b, c = (obj.data.vertices[index].co for index in triangle.vertices)
        tetra_volume = float(a.dot(b.cross(c))) / 6.0
        weighted += (a + b + c) * (tetra_volume / 4.0)
        signed_volume += tetra_volume
    if abs(signed_volume) <= 1e-10:
        return sum((Vector(corner) for corner in obj.bound_box), Vector()) / 8.0
    return weighted / signed_volume


def runtime_bounds_from_blender(obj: bpy.types.Object) -> list[float]:
    dimensions = obj.dimensions
    return [float(dimensions.x), float(dimensions.z), float(dimensions.y)]


def validate(project_root: Path, output_root: Path, executable_hash: str) -> None:
    config_path = project_root / "art/config/vertical_slice_assets.json"
    config = read_json(config_path)
    validate_config(config)
    manifest = read_json(safe_output_path(output_root, Path("tools/art/vertical_slice_asset_manifest.json")))
    expected_generator = {
        "pipeline_schema_version": 1,
        "blender_version": bpy.app.version_string,
        "blender_build_hash": bpy.app.build_hash.decode("ascii"),
        "blender_executable_sha256": executable_hash,
        "seed": config["seed"],
        "config_sha256": sha256_file(config_path),
        "pipeline_sha256": compute_pipeline_hash(config_path, executable_hash),
        "unit": "meter",
        "up_axis": "+Z",
        "forward_axis": "-Y",
    }
    if manifest["generator"] != expected_generator:
        raise RuntimeError("manifest generator provenance mismatch")
    expected_proxies = build_proxies(project_root, config)
    if canonical_json(manifest["proxies"]) != canonical_json(expected_proxies):
        raise RuntimeError("manifest proxies differ from level")
    for output in manifest["outputs"]:
        output_path = safe_output_path(output_root, Path(output["path"]))
        if sha256_file(output_path) != output["sha256"]:
            raise RuntimeError(f"output hash mismatch: {output['path']}")
    expected_roles = {"VIS", "COL", "FRAG", "SOCKET", "RIG"}
    found_roles: set[str] = set()
    actual_sources = []
    configured_assets = {asset["id"]: asset for asset in config["assets"]}
    measured = {"triangles": 0, "draw_calls": 0, "texture_bytes": 0, "particles": 0, "fragments": 0}
    texture_storage_bytes = 0
    runtime_metrics = {}
    if {asset["id"] for asset in manifest["assets"]} != set(configured_assets):
        raise RuntimeError("manifest asset IDs differ from config")
    for asset in manifest["assets"]:
        source_path = safe_output_path(output_root, Path(asset["source_blend"]))
        bpy.ops.wm.open_mainfile(filepath=str(source_path), load_ui=False)
        actual_semantic_hash = semantic_scene_sha256()
        if actual_semantic_hash != asset["blend_semantic_sha256"]:
            raise RuntimeError(f"blend semantic hash mismatch: {asset['id']}")
        actual_sources.append(
            {"path": asset["source_blend"], "semantic_sha256": actual_semantic_hash}
        )
        glb_path = safe_output_path(output_root, Path(asset["glb"]))
        if sha256_file(glb_path) != asset["glb_sha256"]:
            raise RuntimeError(f"GLB hash mismatch: {asset['id']}")
        glb_contract = inspect_glb(glb_path)
        authored_textures = expected_asset_textures(config, configured_assets[asset["id"]], output_root)
        if authored_textures != asset.get("authored_textures"):
            raise RuntimeError(f"authored texture manifest mismatch: {asset['id']}")
        authored_by_name = {item["image_name"]: item for item in authored_textures}
        validate_texture_bijection(set(authored_by_name), {item["name"] for item in glb_contract["images"]})
        for embedded in glb_contract["images"]:
            source = authored_by_name.get(embedded["name"])
            if source is None or embedded["sha256"] != source["sha256"] or embedded["pixel_sha256"] != source["pixel_sha256"]:
                raise RuntimeError(f"embedded texture differs from authored PNG: {embedded['name']}")
        if (
            glb_contract["images"] != asset["texture_images"]
            or glb_contract["texture_count"] != asset["texture_count"]
            or glb_contract["uv_primitives"] != asset["uv_layers"]
            or glb_contract["draw_calls"] != asset["draw_calls"]
            or glb_contract["materials"] != asset["glb_materials"]
        ):
            raise RuntimeError(f"GLB texture/UV contract mismatch: {asset['id']}")
        validate_glb_material_contract(config, configured_assets[asset["id"]], glb_contract["materials"], authored_textures)
        measured["draw_calls"] += int(glb_contract["draw_calls"])
        measured["texture_bytes"] += int(glb_contract["decoded_texture_bytes"])
        texture_storage_bytes += int(glb_contract["texture_bytes"])
        bpy.ops.wm.read_factory_settings(use_empty=True)
        result = bpy.ops.import_scene.gltf(filepath=str(glb_path), import_scene_as_collection=False)
        if "FINISHED" not in result:
            raise RuntimeError(f"could not reopen {glb_path}")
        objects = sorted(bpy.context.scene.objects, key=lambda item: item.name)
        if not objects:
            raise RuntimeError(f"empty GLB: {asset['id']}")
        actual_names = {obj.name for obj in objects}
        expected_names = {record["name"] for record in asset["objects"]}
        if actual_names != expected_names:
            raise RuntimeError(f"object names changed in GLB {asset['id']}: {actual_names ^ expected_names}")
        expected_records = {record["name"]: record for record in asset["objects"]}
        actual_triangles = 0
        actual_materials: set[str] = set()
        actual_collision_volume = 0.0
        actual_lods = []
        for obj in objects:
            if not NAME_RE.fullmatch(obj.name) or "-col" in obj.name.lower():
                raise RuntimeError(f"invalid exported object name: {obj.name}")
            role = str(obj.get("ninho_role", ""))
            found_roles.add(role)
            if obj.get("ninho_asset_id") != asset["id"]:
                raise RuntimeError(f"custom asset ID missing on {obj.name}")
            if obj.get("unit") != "meter" or obj.get("up_axis") != "+Z" or obj.get("forward_axis") != "-Y":
                raise RuntimeError(f"coordinate custom properties missing on {obj.name}")
            if obj.get("pivot") != "CENTER_OF_MASS":
                raise RuntimeError(f"pivot custom property missing on {obj.name}")
            if any(not math.isclose(float(value), 1.0, abs_tol=1e-6) for value in obj.scale):
                raise RuntimeError(f"scale not applied on {obj.name}")
            if obj.matrix_world.to_3x3().determinant() <= 0.0:
                raise RuntimeError(f"non-positive determinant on {obj.name}")
            if obj.type == "MESH":
                obj.data.calc_loop_triangles()
                triangle_count = len(obj.data.loop_triangles)
                actual_triangles += triangle_count
                if triangle_count != expected_records[obj.name]["triangles"]:
                    raise RuntimeError(f"exported triangle count mismatch: {obj.name}")
                if "lod" in obj:
                    actual_lods.append(
                        {"name": obj.name, "level": int(obj["lod"]), "triangles": triangle_count}
                    )
                actual_materials.update(material.name for material in obj.data.materials if material is not None)
                if any(polygon.normal.length < 0.99 for polygon in obj.data.polygons):
                    raise RuntimeError(f"invalid exported normals: {obj.name}")
                center_of_mass = mesh_center_of_mass(obj)
                tolerance = max(1e-5, max(float(value) for value in obj.dimensions) * 1e-5)
                if center_of_mass.length > tolerance:
                    raise RuntimeError(f"exported pivot is not center of mass: {obj.name}")
                if role == "COL":
                    if not is_convex_mesh(obj):
                        raise RuntimeError(f"non-convex collision hull: {obj.name}")
                    bm = bmesh.new()
                    try:
                        bm.from_mesh(obj.data)
                        actual_collision_volume += abs(float(bm.calc_volume(signed=True)))
                    finally:
                        bm.free()
        main_lod = bpy.context.scene.objects.get(f"VIS_{asset['id']}_LOD0")
        if main_lod is None:
            raise RuntimeError(f"LOD0 missing after export: {asset['id']}")
        actual_bounds = runtime_bounds_from_blender(main_lod)
        if any(
            not math.isclose(actual, float(expected), rel_tol=1e-5, abs_tol=1e-5)
            for actual, expected in zip(actual_bounds, asset["bounds_m"])
        ):
            raise RuntimeError(f"exported bounds mismatch: {asset['id']} {actual_bounds} != {asset['bounds_m']}")
        if asset["id"] == "DEV_ImpulseRing":
            minimum_aperture = min(
                math.hypot(float(vertex.co.x), float(vertex.co.z))
                for vertex in main_lod.data.vertices
            )
            expected_aperture = float(configured_assets[asset["id"]]["interaction"]["inner_radius_m"])
            if not math.isclose(minimum_aperture, expected_aperture, rel_tol=0.08, abs_tol=0.015):
                raise RuntimeError("impulse ring aperture differs from authored interaction radius")
        if actual_triangles != asset["triangles"]:
            raise RuntimeError(f"exported asset triangle count mismatch: {asset['id']}")
        visible_objects = [
            obj
            for obj in objects
            if str(obj.get("ninho_role", "")) == "VIS" and ("lod" not in obj or int(obj["lod"]) == 0)
        ]
        runtime_metrics[asset["id"]] = {
            "triangles": sum(mesh_triangles(obj) for obj in visible_objects),
            "draw_calls": sum(max(1, len([slot for slot in obj.data.materials if slot])) for obj in visible_objects),
            "fragments": sum(1 for obj in objects if str(obj.get("ninho_role", "")) == "FRAG"),
        }
        if sorted(actual_materials) != sorted(asset["materials"]):
            raise RuntimeError(f"exported materials mismatch: {asset['id']}")
        if not math.isclose(actual_collision_volume, float(asset["collision_volume_m3"]), rel_tol=1e-4, abs_tol=1e-6):
            raise RuntimeError(f"exported collision volume mismatch: {asset['id']}")
        if actual_triangles > asset["triangle_budget"]:
            raise RuntimeError(f"triangle budget exceeded: {asset['id']}")
        if len(actual_materials) > asset["material_budget"]:
            raise RuntimeError(f"material budget exceeded: {asset['id']}")
        actual_lods.sort(key=lambda item: item["level"])
        if actual_lods != asset["lods"]:
            raise RuntimeError(f"exported LOD record mismatch: {asset['id']}")
        if len(actual_lods) < 2 or actual_lods[1]["triangles"] > actual_lods[0]["triangles"]:
            raise RuntimeError(f"invalid LOD chain: {asset['id']}")
        configured = configured_assets[asset["id"]]
        if (
            asset["bounds_m"] != configured["bounds_m"]
            or asset["triangle_budget"] != configured["triangle_budget"]
            or asset["material_budget"] != configured["material_budget"]
        ):
            raise RuntimeError(f"manifest asset contract differs from config: {asset['id']}")
        expected_provenance = {
            "author": config["author"],
            "license": config["license"],
            "seed": config["seed"],
            "source": config["source"],
            "source_config_sha256": expected_generator["config_sha256"],
        }
        if any(asset.get(key) != value for key, value in expected_provenance.items()):
            raise RuntimeError(f"manifest provenance mismatch: {asset['id']}")
    if not expected_roles.issubset(found_roles):
        raise RuntimeError(f"export set lacks roles: {sorted(expected_roles - found_roles)}")
    manifest_source_semantics = [
        {"path": source["path"], "semantic_sha256": source["semantic_sha256"]}
        for source in sorted(manifest["sources"], key=lambda item: item["path"])
    ]
    if manifest_source_semantics != sorted(actual_sources, key=lambda item: item["path"]):
        raise RuntimeError("manifest source semantics mismatch")
    runtime_semantics = runtime_material_semantics(output_root)
    validate_runtime_material_semantics(config, runtime_semantics)
    material_hash = material_semantics_sha256(runtime_semantics)
    if manifest.get("material_semantics_sha256") != material_hash:
        raise RuntimeError("material semantics hash mismatch")
    runtime_instances = list(config["level_layout"]["runtime_singletons"]) + [item["asset_id"] for item in expected_proxies]
    measured.update(measure_instantiated_budgets(runtime_metrics, runtime_instances))
    measured["particles"] = scene_particle_count(project_root)
    glass_asset_record = next(asset for asset in manifest["assets"] if asset["id"] == "KIT_GlassPanel_A")
    glass_texture = next(item for item in glass_asset_record["authored_textures"] if item["material"] == "MAT_Glass")
    measured["texture_bytes"] += decoded_rgba8_mip_bytes(int(glass_texture["width"]), int(glass_texture["height"]))
    texture_storage_bytes += int(safe_output_path(output_root, Path(glass_texture["path"])).stat().st_size)
    budgets = global_budget_record(measured, config["global_budgets"], texture_storage_bytes)
    if manifest.get("global_budgets") != budgets:
        raise RuntimeError("global budget measurements mismatch")
    if manifest["build_hash"] != compute_build_hash(
        expected_generator,
        manifest["outputs"],
        actual_sources,
        expected_proxies,
        budgets,
        material_hash,
    ):
        raise RuntimeError("manifest build hash mismatch")
    print(f"NINHO_VALIDATED_ASSETS={len(manifest['assets'])}")


def main() -> None:
    args = parse_arguments()
    project_root = args.project_root.resolve()
    output_root = args.output_root.resolve()
    if args.validate:
        validate(project_root, output_root, args.blender_executable_sha256)
    else:
        build(project_root, output_root, args.blender_executable_sha256)


if __name__ == "__main__":
    main()
