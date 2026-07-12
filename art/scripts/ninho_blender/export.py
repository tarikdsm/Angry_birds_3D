"""Canonical Blender/GLB export and manifest measurements."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct

import bmesh
import bpy

from .textures import decoded_rgba8_mip_bytes, inspect_png_rgba


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


def semantic_scene_record() -> dict:
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
                polygons.append(
                    {
                        "vertices": canonical,
                        "smooth": bool(polygon.use_smooth),
                        "material_index": int(polygon.material_index),
                    }
                )
            polygons.sort(key=lambda item: (item["vertices"], item["material_index"]))
            material_names = [material.name if material else None for material in obj.data.materials]
            used_materials.update(name for name in material_names if name is not None)
            record["mesh"] = {
                "name": obj.data.name,
                "vertices": [
                    [_semantic_value(axis) for axis in vertex.co]
                    for vertex in obj.data.vertices
                ],
                "polygons": polygons,
                "uv_layers": [
                    {
                        "name": layer.name,
                        "active_render": bool(layer.active_render),
                        "faces": sorted(
                            (
                                lambda pairs: pairs[pairs.index(min(pairs, key=lambda item: item[0])) :]
                                + pairs[: pairs.index(min(pairs, key=lambda item: item[0]))]
                            )(
                                [
                                    (
                                        int(obj.data.loops[loop_index].vertex_index),
                                        [_semantic_value(axis) for axis in layer.data[loop_index].uv],
                                    )
                                    for loop_index in polygon.loop_indices
                                ]
                            )
                            for polygon in obj.data.polygons
                        ),
                    }
                    for layer in obj.data.uv_layers
                ],
                "attributes": [
                    {
                        "name": attribute.name,
                        "domain": attribute.domain,
                        "data_type": attribute.data_type,
                        "data": [
                            _semantic_value(
                                getattr(item, "value", getattr(item, "vector", getattr(item, "color", "")))
                            )
                            for item in attribute.data
                        ],
                    }
                    for attribute in obj.data.attributes
                    if not attribute.name.startswith(".")
                    and attribute.name not in {"position", *(layer.name for layer in obj.data.uv_layers)}
                ],
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
                node_record = {"name": node.name, "type": node.bl_idname, "inputs": inputs}
                if node.bl_idname == "ShaderNodeTexImage":
                    node_record.update(
                        {
                            "image": node.image.name if node.image else None,
                            "interpolation": node.interpolation,
                            "extension": node.extension,
                        }
                    )
                nodes.append(node_record)
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
    used_images = {
        node.image
        for name in used_materials
        for node in (bpy.data.materials[name].node_tree.nodes if bpy.data.materials[name].node_tree else [])
        if node.bl_idname == "ShaderNodeTexImage" and node.image is not None
    }
    image_records = []
    for image in sorted(used_images, key=lambda item: item.name):
        pixel_digest = hashlib.sha256()
        for value in image.pixels:
            pixel_digest.update(struct.pack("<f", round(float(value), 7)))
        image_records.append(
            {
                "name": image.name,
                "size": [int(image.size[0]), int(image.size[1])],
                "source": image.source,
                "colorspace": image.colorspace_settings.name,
                "pixel_sha256": pixel_digest.hexdigest(),
                "custom_properties": _custom_properties(image),
            }
        )
    unit = bpy.context.scene.unit_settings
    semantic = {
        "scene": bpy.context.scene.name,
        "scene_custom_properties": _custom_properties(bpy.context.scene),
        "unit_settings": {
            "system": unit.system,
            "scale_length": _semantic_value(unit.scale_length),
            "length_unit": unit.length_unit,
            "mass_unit": unit.mass_unit,
            "time_unit": unit.time_unit,
            "rotation_unit": unit.system_rotation,
        },
        "objects": object_records,
        "materials": material_records,
        "images": image_records,
    }
    return semantic


def semantic_scene_sha256() -> str:
    encoded = json.dumps(
        semantic_scene_record(), ensure_ascii=False, sort_keys=True, separators=(",", ":")
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def _read_glb(path: Path) -> tuple[dict, bytes]:
    data = path.read_bytes()
    if data[:4] != b"glTF":
        raise RuntimeError(f"not a binary glTF file: {path}")
    json_length, json_kind = struct.unpack_from("<II", data, 12)
    if json_kind != 0x4E4F534A:
        raise RuntimeError(f"GLB JSON chunk missing: {path}")
    document = json.loads(data[20 : 20 + json_length].decode("utf-8"))
    binary_header = 20 + json_length
    binary_length, binary_kind = struct.unpack_from("<II", data, binary_header)
    if binary_kind != 0x004E4942:
        raise RuntimeError(f"GLB binary chunk missing: {path}")
    binary_start = binary_header + 8
    if binary_start + binary_length > len(data):
        raise RuntimeError(f"truncated GLB binary chunk: {path}")
    return document, data[binary_start : binary_start + binary_length]


def inspect_glb(path: Path) -> dict:
    document, binary = _read_glb(path)
    views = document.get("bufferViews", [])
    images = []
    for index, image in enumerate(document.get("images", [])):
        if "bufferView" not in image:
            raise RuntimeError(f"GLB image is not embedded: {path}")
        view = views[int(image["bufferView"])]
        start = int(view.get("byteOffset", 0))
        length = int(view["byteLength"])
        payload = binary[start : start + length]
        pixel_contract = inspect_png_rgba(payload)
        decoded_mip_bytes = decoded_rgba8_mip_bytes(pixel_contract["width"], pixel_contract["height"])
        if pixel_contract["unique_colors"] <= 1 or not pixel_contract["has_nonblack_rgb"]:
            raise RuntimeError(f"embedded authored texture is constant or black: {path}")
        images.append(
            {
                "name": image.get("name", f"image_{index}"),
                "mime_type": image.get("mimeType", ""),
                "byte_length": length,
                "sha256": hashlib.sha256(payload).hexdigest(),
                **pixel_contract,
                "decoded_mip_bytes": decoded_mip_bytes,
            }
        )
    mesh_roles: dict[int, set[str]] = {}
    for node in document.get("nodes", []):
        if "mesh" in node:
            mesh_roles.setdefault(int(node["mesh"]), set()).add(str(node.get("extras", {}).get("ninho_role", "")))
    draw_calls = 0
    uv_primitives = 0
    visual_primitives = 0
    for mesh_index, mesh in enumerate(document.get("meshes", [])):
        is_visual = "VIS" in mesh_roles.get(mesh_index, set())
        for primitive in mesh.get("primitives", []):
            draw_calls += 1
            has_uv = "TEXCOORD_0" in primitive.get("attributes", {})
            uv_primitives += int(has_uv)
            if is_visual:
                visual_primitives += 1
                if not has_uv:
                    raise RuntimeError(f"visual primitive lacks TEXCOORD_0: {path}")
    if not images or not document.get("textures"):
        raise RuntimeError(f"GLB lacks embedded authored textures: {path}")
    material_records = []
    textures = document.get("textures", [])
    for material in document.get("materials", []):
        pbr = material.get("pbrMetallicRoughness", {})
        texture_binding = pbr.get("baseColorTexture", {}).get("index")
        image_name = None
        if texture_binding is not None:
            source_index = textures[int(texture_binding)].get("source")
            if source_index is not None:
                image_name = images[int(source_index)]["name"]
        extensions = material.get("extensions", {})
        material_records.append(
            {
                "name": material.get("name", ""),
                "base_color_factor": [float(value) for value in pbr.get("baseColorFactor", [1, 1, 1, 1])],
                "roughness_factor": float(pbr.get("roughnessFactor", 1.0)),
                "metallic_factor": float(pbr.get("metallicFactor", 1.0)),
                "base_color_image": image_name,
                "alpha_mode": material.get("alphaMode", "OPAQUE"),
                "emissive_factor": [float(value) for value in material.get("emissiveFactor", [0, 0, 0])],
                "emissive_strength": float(extensions.get("KHR_materials_emissive_strength", {}).get("emissiveStrength", 1.0)),
                "transmission_factor": float(extensions.get("KHR_materials_transmission", {}).get("transmissionFactor", 0.0)),
                "coat_factor": float(extensions.get("KHR_materials_clearcoat", {}).get("clearcoatFactor", 0.0)),
            }
        )
    return {
        "images": images,
        "texture_count": len(document.get("textures", [])),
        "texture_bytes": sum(item["byte_length"] for item in images),
        "decoded_texture_bytes": sum(item["decoded_mip_bytes"] for item in images),
        "draw_calls": draw_calls,
        "uv_primitives": uv_primitives,
        "visual_primitives": visual_primitives,
        "materials": sorted(material_records, key=lambda item: item["name"]),
    }


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
    document, _ = _read_glb(path)
    json_length = struct.unpack_from("<I", data, 12)[0]
    binary_header = 20 + json_length
    binary_length = struct.unpack_from("<I", data, binary_header)[0]
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
