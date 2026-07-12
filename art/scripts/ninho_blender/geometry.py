"""Deterministic procedural geometry for the first-orbit art set."""

from __future__ import annotations

import math

import bpy
from mathutils import Vector


def reset_scene() -> None:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.render.engine = "BLENDER_EEVEE"


def apply_authored_uvs(objects: list[bpy.types.Object]) -> None:
    """Replace operator UV state with a deterministic cylindrical unwrap."""
    for obj in objects:
        if obj.type != "MESH" or not obj.data.materials:
            continue
        mesh = obj.data
        while mesh.uv_layers:
            mesh.uv_layers.remove(mesh.uv_layers[0])
        layer = mesh.uv_layers.new(name="UV_Authored", do_init=False)
        z_values = [float(vertex.co.z) for vertex in mesh.vertices]
        z_min = min(z_values, default=0.0)
        z_span = max(max(z_values, default=1.0) - z_min, 1e-9)
        for loop in mesh.loops:
            coordinate = mesh.vertices[loop.vertex_index].co
            u = (math.atan2(float(coordinate.y), float(coordinate.x)) / math.tau + 0.5) % 1.0
            v = (float(coordinate.z) - z_min) / z_span
            layer.data[loop.index].uv = (round(u, 7), round(v, 7))
        layer.active_render = True


def _tag(obj: bpy.types.Object, asset_id: str, role: str, lod: int | None = None) -> None:
    obj["ninho_asset_id"] = asset_id
    obj["ninho_role"] = role
    obj["unit"] = "meter"
    obj["up_axis"] = "+Z"
    obj["forward_axis"] = "-Y"
    obj["pivot"] = "CENTER_OF_MASS"
    obj["scale_applied"] = True
    if lod is not None:
        obj["lod"] = lod


def _apply_dimensions(obj: bpy.types.Object, bounds: list[float]) -> None:
    bpy.context.view_layer.update()
    obj.dimensions = Vector(tuple(float(value) for value in bounds))
    bpy.context.view_layer.update()
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.select_set(False)


def _assign(obj: bpy.types.Object, material: bpy.types.Material) -> None:
    if obj.type == "MESH":
        obj.data.materials.append(material)


def _box(name: str, bounds: list[float], material: bpy.types.Material) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 0.0))
    obj = bpy.context.object
    obj.name = name
    obj.data.name = f"MESH_{name}"
    _apply_dimensions(obj, bounds)
    _assign(obj, material)
    return obj


def _sphere(
    name: str,
    bounds: list[float],
    material: bpy.types.Material,
    segments: int,
    rings: int,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=max(8, segments),
        ring_count=max(4, rings),
        radius=1.0,
        location=(0.0, 0.0, 0.0),
    )
    obj = bpy.context.object
    obj.name = name
    obj.data.name = f"MESH_{name}"
    _apply_dimensions(obj, bounds)
    _assign(obj, material)
    return obj


def _torus(
    name: str,
    bounds: list[float],
    material: bpy.types.Material,
    segments: int,
    inner_ratio: float = 0.72,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_torus_add(
        align="WORLD",
        major_segments=max(12, segments),
        minor_segments=max(6, segments // 3),
        mode="EXT_INT",
        abso_major_rad=1.0,
        abso_minor_rad=inner_ratio,
        location=(0.0, 0.0, 0.0),
        rotation=(math.radians(90.0), 0.0, 0.0),
    )
    obj = bpy.context.object
    obj.name = name
    obj.data.name = f"MESH_{name}"
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    _apply_dimensions(obj, bounds)
    _assign(obj, material)
    return obj


def _convex_hull(name: str, bounds: list[float], material: bpy.types.Material, rounded: bool) -> bpy.types.Object:
    if rounded:
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=1.0, location=(0.0, 0.0, 0.0))
        obj = bpy.context.object
        obj.name = name
        obj.data.name = f"MESH_{name}"
        _apply_dimensions(obj, bounds)
        _assign(obj, material)
        return obj
    return _box(name, bounds, material)


def _empty(name: str, asset_id: str, role: str, location: tuple[float, float, float]) -> bpy.types.Object:
    obj = bpy.data.objects.new(name, None)
    bpy.context.scene.collection.objects.link(obj)
    obj.empty_display_type = "ARROWS" if role == "SOCKET" else "PLAIN_AXES"
    obj.empty_display_size = 0.12
    obj.location = location
    _tag(obj, asset_id, role)
    return obj


def _make_lod(
    asset: dict,
    materials: dict[str, bpy.types.Material],
    lod: int,
) -> bpy.types.Object:
    asset_id = asset["id"]
    bounds = asset["bounds_m"]
    material = materials[asset["materials"][0]]
    segments = int(asset["lod_segments"][lod])
    geometry = asset["geometry"]
    name = f"VIS_{asset_id}_LOD{lod}"
    if geometry in {"aster_planet", "virela", "anchor"}:
        obj = _sphere(name, bounds, material, segments, max(4, segments // 2))
    elif geometry == "impulse_ring":
        outer_radius = float(asset["bounds_m"][0]) * 0.5
        inner_ratio = float(asset["interaction"]["inner_radius_m"]) / outer_radius
        obj = _torus(name, bounds, material, segments, inner_ratio)
    else:
        obj = _box(name, bounds, material)
    _tag(obj, asset_id, "VIS", lod)
    return obj


def _add_original_surface(asset: dict, obj: bpy.types.Object, seed: int) -> None:
    if asset["geometry"] != "aster_planet" or obj.type != "MESH":
        return
    frequency_x = 7 + seed % 5
    frequency_y = 9 + (seed // 10) % 5
    frequency_z = 11 + (seed // 100) % 5
    for vertex in obj.data.vertices:
        direction = vertex.co.normalized()
        ripple = 1.0 + 0.024 * (
            math.cos(direction.x * frequency_x)
            * math.cos(direction.y * frequency_y)
            * math.cos(direction.z * frequency_z)
        )
        vertex.co *= ripple
    obj.data.update()
    _apply_dimensions(obj, asset["bounds_m"])


def _add_detail(asset: dict, materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    asset_id = asset["id"]
    geometry = asset["geometry"]
    bounds = [float(value) for value in asset["bounds_m"]]
    result: list[bpy.types.Object] = []
    if len(asset["materials"]) < 2:
        return result
    accent = materials[asset["materials"][1]]
    if geometry == "aster_platform":
        detail = _box(f"VIS_{asset_id}_GardenInlay", [bounds[0] * 0.72, bounds[1] * 0.72, bounds[2] * 0.22], accent)
        detail.location.z = bounds[2] * 0.39
        _tag(detail, asset_id, "VIS")
        result.append(detail)
    elif geometry in {"virela", "anchor"}:
        crest = _sphere(f"VIS_{asset_id}_Crest", [bounds[0] * 0.34, bounds[1] * 0.20, bounds[2] * 0.28], accent, 12, 6)
        crest.location = (0.0, -bounds[1] * 0.34, bounds[2] * 0.23)
        _tag(crest, asset_id, "VIS")
        result.append(crest)
    elif geometry == "impulse_ring":
        outer_radius = float(bounds[0]) * 0.5
        inner_ratio = float(asset["interaction"]["inner_radius_m"]) / outer_radius
        core = _torus(f"VIS_{asset_id}_Core", [bounds[0] * 0.78, bounds[1] * 0.78, bounds[2] * 0.55], accent, 16, inner_ratio)
        _tag(core, asset_id, "VIS")
        result.append(core)
    return result


def build_asset(asset: dict, config: dict, materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    asset_id = asset["id"]
    runtime_bounds = [float(value) for value in asset["bounds_m"]]
    source_asset = dict(asset)
    source_asset["bounds_m"] = [runtime_bounds[0], runtime_bounds[2], runtime_bounds[1]]
    scene = bpy.context.scene
    scene.name = f"SCENE_{asset_id}"
    scene["ninho_asset_id"] = asset_id
    scene["pipeline_seed"] = int(config["seed"])
    scene["unit"] = "meter"
    scene["up_axis"] = "+Z"
    scene["forward_axis"] = "-Y"

    objects = [_make_lod(source_asset, materials, 0), _make_lod(source_asset, materials, 1)]
    _add_original_surface(source_asset, objects[0], int(config["seed"]))
    _add_original_surface(source_asset, objects[1], int(config["seed"]))
    objects.extend(_add_detail(source_asset, materials))

    collision_bounds = [float(value) * 0.98 for value in source_asset["bounds_m"]]
    rounded = asset["geometry"] in {"aster_planet", "virela", "anchor"}
    collision = _convex_hull(
        f"COL_{asset_id}_Hull",
        collision_bounds,
        materials[asset["materials"][0]],
        rounded,
    )
    _tag(collision, asset_id, "COL")
    collision["convex"] = True
    objects.append(collision)

    if "FRAG" in asset["roles"]:
        for index, sign in enumerate((-1.0, 1.0), start=1):
            frag_bounds = [max(0.015, float(value) * 0.42) for value in source_asset["bounds_m"]]
            fragment = _box(
                f"FRAG_{asset_id}_{index:02d}",
                frag_bounds,
                materials[asset["materials"][0]],
            )
            fragment.location.x = sign * float(source_asset["bounds_m"][0]) * 0.24
            _tag(fragment, asset_id, "FRAG")
            objects.append(fragment)

    front = _empty(
        f"SOCKET_{asset_id}_Forward",
        asset_id,
        "SOCKET",
        (0.0, -float(source_asset["bounds_m"][1]) * 0.5, 0.0),
    )
    objects.append(front)
    if "RIG" in asset["roles"]:
        objects.append(_empty(f"RIG_{asset_id}_Root", asset_id, "RIG", (0.0, 0.0, 0.0)))

    for obj in objects:
        if obj.type == "MESH":
            for polygon in obj.data.polygons:
                polygon.use_smooth = asset["geometry"] in {"aster_planet", "virela", "anchor", "impulse_ring"}
    bpy.context.view_layer.update()
    return objects
