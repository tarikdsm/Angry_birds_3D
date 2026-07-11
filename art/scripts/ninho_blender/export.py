"""Canonical Blender/GLB export and manifest measurements."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct

import bmesh
import bpy


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def mesh_triangles(obj: bpy.types.Object) -> int:
    if obj.type != "MESH":
        return 0
    obj.data.calc_loop_triangles()
    return len(obj.data.loop_triangles)


def mesh_volume(obj: bpy.types.Object) -> float:
    if obj.type != "MESH":
        return 0.0
    bm = bmesh.new()
    try:
        bm.from_mesh(obj.data)
        return abs(float(bm.calc_volume(signed=True)))
    finally:
        bm.free()


def _round(value: float) -> float:
    return round(float(value), 6)


def _semantic_value(value: object) -> object:
    if isinstance(value, (bool, int, str)):
        return value
    if isinstance(value, float):
        return round(value, 7)
    if hasattr(value, "to_list"):
        return [_semantic_value(item) for item in value.to_list()]
    if hasattr(value, "to_tuple"):
        return [_semantic_value(item) for item in value.to_tuple()]
    if isinstance(value, (list, tuple)):
        return [_semantic_value(item) for item in value]
    return str(value)


def _custom_properties(value: object) -> dict:
    return {
        key: _semantic_value(value[key])
        for key in sorted(value.keys())
        if key != "_RNA_UI"
    }


def semantic_scene_sha256() -> str:
    object_records = []
    used_materials = set()
    for obj in sorted(bpy.context.scene.objects, key=lambda item: item.name):
        record = {
            "name": obj.name,
            "type": obj.type,
            "parent": obj.parent.name if obj.parent else None,
            "location": [_semantic_value(value) for value in obj.location],
            "rotation_quaternion": [_semantic_value(value) for value in obj.rotation_quaternion],
            "rotation_euler": [_semantic_value(value) for value in obj.rotation_euler],
            "scale": [_semantic_value(value) for value in obj.scale],
            "hide_render": bool(obj.hide_render),
            "hide_viewport": bool(obj.hide_viewport),
            "custom_properties": _custom_properties(obj),
        }
        if obj.type == "MESH":
            polygons = []
            for polygon in obj.data.polygons:
                indices = tuple(int(index) for index in polygon.vertices)
                minimum_index = indices.index(min(indices))
                canonical = indices[minimum_index:] + indices[:minimum_index]
                polygons.append({"vertices": canonical, "smooth": bool(polygon.use_smooth)})
            polygons.sort(key=lambda item: item["vertices"])
            material_names = [material.name if material else None for material in obj.data.materials]
            used_materials.update(name for name in material_names if name is not None)
            record["mesh"] = {
                "name": obj.data.name,
                "vertices": [
                    [_semantic_value(axis) for axis in vertex.co]
                    for vertex in obj.data.vertices
                ],
                "polygons": polygons,
                "materials": material_names,
                "custom_properties": _custom_properties(obj.data),
            }
        object_records.append(record)
    material_records = []
    for name in sorted(used_materials):
        material = bpy.data.materials[name]
        nodes = []
        links = []
        if material.use_nodes and material.node_tree is not None:
            for node in sorted(material.node_tree.nodes, key=lambda item: item.name):
                inputs = []
                for socket in node.inputs:
                    if hasattr(socket, "default_value"):
                        inputs.append(
                            {
                                "identifier": socket.identifier,
                                "default": _semantic_value(socket.default_value),
                            }
                        )
                nodes.append({"name": node.name, "type": node.bl_idname, "inputs": inputs})
            links = sorted(
                (
                    link.from_node.name,
                    link.from_socket.identifier,
                    link.to_node.name,
                    link.to_socket.identifier,
                )
                for link in material.node_tree.links
            )
        material_records.append(
            {
                "name": name,
                "diffuse_color": [_semantic_value(value) for value in material.diffuse_color],
                "roughness": _semantic_value(material.roughness),
                "metallic": _semantic_value(material.metallic),
                "surface_render_method": material.surface_render_method,
                "use_nodes": bool(material.use_nodes),
                "nodes": nodes,
                "links": links,
                "custom_properties": _custom_properties(material),
            }
        )
    semantic = {
        "scene": bpy.context.scene.name,
        "scene_custom_properties": _custom_properties(bpy.context.scene),
        "objects": object_records,
        "materials": material_records,
    }
    encoded = json.dumps(semantic, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def _object_record(obj: bpy.types.Object) -> dict:
    role = str(obj.get("ninho_role", ""))
    materials = []
    if obj.type == "MESH":
        materials = sorted({slot.name for slot in obj.data.materials if slot is not None})
    record = {
        "name": obj.name,
        "role": role,
        "type": obj.type,
        "triangles": mesh_triangles(obj),
        "volume_m3": _round(mesh_volume(obj)),
        "materials": materials,
        "location_m": [_round(value) for value in obj.location],
        "rotation_euler_rad": [_round(value) for value in obj.rotation_euler],
        "scale": [_round(value) for value in obj.scale],
        "determinant": _round(obj.matrix_world.to_3x3().determinant()),
    }
    if "lod" in obj:
        record["lod"] = int(obj["lod"])
    if role == "COL":
        record["convex"] = bool(obj.get("convex", False))
    return record


def collect_asset_record(asset: dict, objects: list[bpy.types.Object]) -> dict:
    records = [_object_record(obj) for obj in sorted(objects, key=lambda item: item.name)]
    lods = [
        {"name": record["name"], "level": record["lod"], "triangles": record["triangles"]}
        for record in records
        if record["role"] == "VIS" and "lod" in record
    ]
    material_names = sorted(
        {
            material
            for record in records
            for material in record["materials"]
        }
    )
    collision_volume = sum(record["volume_m3"] for record in records if record["role"] == "COL")
    return {
        "id": asset["id"],
        "bounds_m": [float(value) for value in asset["bounds_m"]],
        "pivot": "CENTER_OF_MASS",
        "scale_applied": all(record["scale"] == [1.0, 1.0, 1.0] for record in records),
        "determinant_positive": all(record["determinant"] > 0.0 for record in records),
        "triangles": sum(record["triangles"] for record in records),
        "triangle_budget": int(asset["triangle_budget"]),
        "materials": material_names,
        "material_budget": int(asset["material_budget"]),
        "collision_volume_m3": _round(collision_volume),
        "lods": sorted(lods, key=lambda item: item["level"]),
        "objects": records,
    }


def canonicalize_glb_indices(path: Path) -> None:
    data = bytearray(path.read_bytes())
    if data[:4] != b"glTF":
        raise RuntimeError(f"not a binary glTF file: {path}")
    json_length, json_kind = struct.unpack_from("<II", data, 12)
    if json_kind != 0x4E4F534A:
        raise RuntimeError(f"GLB JSON chunk missing: {path}")
    document = json.loads(bytes(data[20 : 20 + json_length]).decode("utf-8"))
    binary_header = 20 + json_length
    binary_length, binary_kind = struct.unpack_from("<II", data, binary_header)
    if binary_kind != 0x004E4942:
        raise RuntimeError(f"GLB binary chunk missing: {path}")
    binary_start = binary_header + 8
    component_formats = {5121: ("B", 1), 5123: ("H", 2), 5125: ("I", 4)}
    for mesh in document.get("meshes", []):
        for primitive in mesh.get("primitives", []):
            accessor_index = primitive.get("indices")
            if accessor_index is None:
                continue
            accessor = document["accessors"][accessor_index]
            if accessor.get("type") != "SCALAR" or accessor["count"] % 3 != 0:
                raise RuntimeError(f"non-triangle index accessor in {path}")
            component_format, component_size = component_formats[accessor["componentType"]]
            view = document["bufferViews"][accessor["bufferView"]]
            offset = binary_start + int(view.get("byteOffset", 0)) + int(accessor.get("byteOffset", 0))
            count = int(accessor["count"])
            values = struct.unpack_from(f"<{count}{component_format}", data, offset)
            triangles = []
            for index in range(0, count, 3):
                triangle = values[index : index + 3]
                minimum_index = triangle.index(min(triangle))
                triangles.append(triangle[minimum_index:] + triangle[:minimum_index])
            triangles.sort()
            flattened = [value for triangle in triangles for value in triangle]
            packed = struct.pack(f"<{count}{component_format}", *flattened)
            expected_size = count * component_size
            if len(packed) != expected_size:
                raise RuntimeError(f"index canonicalization size mismatch in {path}")
            data[offset : offset + expected_size] = packed
    if binary_start + binary_length > len(data):
        raise RuntimeError(f"truncated GLB binary chunk: {path}")
    path.write_bytes(data)


def save_source_and_glb(source_path: Path, glb_path: Path) -> None:
    source_path.parent.mkdir(parents=True, exist_ok=True)
    glb_path.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(source_path), check_existing=False, compress=True)
    result = bpy.ops.export_scene.gltf(
        filepath=str(glb_path),
        check_existing=False,
        export_format="GLB",
        use_selection=False,
        export_yup=True,
        export_apply=True,
        export_extras=True,
        export_materials="EXPORT",
        export_normals=True,
        export_tangents=False,
        export_texcoords=True,
        export_vertex_color="NONE",
        export_animations=False,
        export_cameras=False,
        export_lights=False,
        export_skins=False,
        export_morph=False,
        export_draco_mesh_compression_enable=False,
        export_use_gltfpack=False,
    )
    if "FINISHED" not in result:
        raise RuntimeError(f"glTF export failed for {glb_path}: {result}")
    canonicalize_glb_indices(glb_path)
