"""Original palette and deterministic PBR material construction."""

from __future__ import annotations

import json
import hashlib
import re
import bpy
from pathlib import Path

from .contracts import safe_output_path
from .textures import encode_png_rgba, generate_material_rgba, inspect_png_rgba


def _set_input(node: bpy.types.Node, name: str, value: object) -> None:
    socket = node.inputs.get(name)
    if socket is not None:
        socket.default_value = value


def resolved_material_semantics(config: dict) -> list[dict]:
    result = []
    for material in sorted(config["materials"], key=lambda item: item["name"]):
        resolved = dict(material)
        resolved["base_color"] = config["palette"][material["palette_key"]]
        result.append(resolved)
    return result


def material_semantics_sha256(semantics: dict) -> str:
    encoded = json.dumps(
        semantics,
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def _parse_color(text: str, key: str) -> list[float]:
    match = re.search(rf"(?m)^{re.escape(key)} = Color\(([^)]+)\)$", text)
    if not match:
        raise RuntimeError(f"generated Godot material lacks {key}")
    return [float(value.strip()) for value in match.group(1).split(",")]


def _parse_number(text: str, key: str) -> float:
    match = re.search(rf"(?m)^{re.escape(key)} = (-?\d+(?:\.\d+)?)$", text)
    if not match:
        raise RuntimeError(f"generated Godot material lacks {key}")
    return float(match.group(1))


def runtime_material_semantics(output_root: Path) -> dict:
    pine_text = safe_output_path(output_root, Path("game/materials/pine.tres")).read_text(encoding="utf-8")
    brick_text = safe_output_path(output_root, Path("game/materials/brick.tres")).read_text(encoding="utf-8")
    glass_text = safe_output_path(output_root, Path("game/materials/glass.tres")).read_text(encoding="utf-8")
    shader_text = safe_output_path(output_root, Path("game/shaders/stylized_glass.gdshader")).read_text(encoding="utf-8")
    texture_match = re.search(r'\[ext_resource type="Texture2D" path="([^"]+)" id="2_tex"\]', glass_text)
    if not texture_match or 'shader_parameter/albedo_texture = ExtResource("2_tex")' not in glass_text:
        raise RuntimeError("generated glass material does not consume its authored texture")
    texture_relative = Path("game") / texture_match.group(1).removeprefix("res://")
    texture_contract = inspect_png_rgba(safe_output_path(output_root, texture_relative).read_bytes())
    return {
        "pine": {
            "base_color": _parse_color(pine_text, "albedo_color"),
            "roughness": _parse_number(pine_text, "roughness"),
            "metallic": _parse_number(pine_text, "metallic"),
        },
        "brick": {
            "base_color": _parse_color(brick_text, "albedo_color"),
            "roughness": _parse_number(brick_text, "roughness"),
            "metallic": _parse_number(brick_text, "metallic"),
        },
        "glass": {
            "texture_path": texture_match.group(1),
            "texture_pixel_sha256": texture_contract["pixel_sha256"],
            "texture_alpha_values": texture_contract["alpha_values"],
            "roughness": _parse_number(glass_text, "shader_parameter/authored_roughness"),
            "metallic": _parse_number(glass_text, "shader_parameter/authored_metallic"),
            "transmission": _parse_number(glass_text, "shader_parameter/transmission_weight"),
            "coat": _parse_number(glass_text, "shader_parameter/coat_weight"),
            "samples_texture": "texture(albedo_texture, UV)" in shader_text,
            "uses_baked_color_once": "ALBEDO = texel.rgb;" in shader_text and "ALPHA = texel.a;" in shader_text,
        },
    }


def validate_runtime_material_semantics(config: dict, semantics: dict) -> None:
    specs = {item["name"]: item for item in resolved_material_semantics(config)}
    expected_glass_asset = next(asset for asset in config["assets"] if "MAT_Glass" in asset["materials"])
    glass_spec = specs["MAT_Glass"]
    texture_spec = next(item for item in config["materials"] if item["name"] == "MAT_Glass")
    texture_size = int(texture_spec["texture"]["size_px"])
    expected_texture = inspect_png_rgba(
        encode_png_rgba(texture_size, texture_size, generate_material_rgba(config, texture_spec))
    )
    expected = {
        "pine": {key: specs["MAT_Pine"][key] for key in ("base_color", "roughness", "metallic")},
        "brick": {key: specs["MAT_Brick"][key] for key in ("base_color", "roughness", "metallic")},
        "glass": {
            "texture_path": f"res://assets/vertical_slice/{expected_glass_asset['folder']}/{expected_glass_asset['id']}_TEX_Glass.png",
            "texture_pixel_sha256": expected_texture["pixel_sha256"],
            "texture_alpha_values": [round(float(glass_spec["alpha"]) * 255.0)],
            **{key: glass_spec[key] for key in ("roughness", "metallic", "transmission", "coat")},
            "samples_texture": True,
            "uses_baked_color_once": True,
        },
    }
    if semantics != expected:
        raise RuntimeError("effective Godot material semantics differ from authored config")


def _load_image(spec: dict, path: Path, pipeline_seed: int) -> bpy.types.Image:
    image = bpy.data.images.load(str(path), check_existing=False)
    image.name = f"TEX_{spec['name'][4:]}"
    image.colorspace_settings.name = "sRGB"
    image.pack()
    image.filepath_raw = f"//TEX_{spec['name'][4:]}.png"
    image["ninho_original"] = True
    image["pattern"] = spec["texture"]["pattern"]
    image["pipeline_seed"] = pipeline_seed
    return image


def build_materials(
    config: dict,
    used_names: set[str] | None = None,
    texture_paths: dict[str, Path] | None = None,
) -> dict[str, bpy.types.Material]:
    result: dict[str, bpy.types.Material] = {}
    for spec in resolved_material_semantics(config):
        name = spec["name"]
        if used_names is not None and name not in used_names:
            continue
        color = tuple(float(component) for component in spec["base_color"])
        material = bpy.data.materials.new(name=name)
        material.use_nodes = True
        material.diffuse_color = color
        material["ninho_original"] = True
        material["palette_key"] = spec["palette_key"]
        material["pipeline_seed"] = int(config["seed"])
        shader = material.node_tree.nodes.get("Principled BSDF")
        if shader is not None:
            _set_input(shader, "Base Color", color)
            _set_input(shader, "Roughness", float(spec["roughness"]))
            _set_input(shader, "Metallic", float(spec["metallic"]))
            _set_input(shader, "Alpha", float(spec["alpha"]))
            if float(spec.get("emission_strength", 0.0)) > 0.0:
                _set_input(shader, "Emission Color", color)
                _set_input(shader, "Emission Strength", float(spec["emission_strength"]))
            _set_input(shader, "Transmission Weight", float(spec.get("transmission", 0.0)))
            _set_input(shader, "Coat Weight", float(spec.get("coat", 0.0)))
            texture_node = material.node_tree.nodes.new("ShaderNodeTexImage")
            texture_node.name = f"NODE_{name}_Texture"
            if texture_paths is None or name not in texture_paths:
                raise ValueError(f"authored texture path missing for material {name}")
            texture_node.image = _load_image(spec, texture_paths[name], int(config["seed"]))
            texture_node.interpolation = "Closest"
            texture_node.extension = "REPEAT"
            material.node_tree.links.new(texture_node.outputs["Color"], shader.inputs["Base Color"])
            if float(spec["alpha"]) < 1.0:
                material.node_tree.links.new(texture_node.outputs["Alpha"], shader.inputs["Alpha"])
        if float(spec["alpha"]) < 1.0:
            material.surface_render_method = "DITHERED"
        result[name] = material
    return result


def _number(value: float) -> str:
    return f"{float(value):.6f}".rstrip("0").rstrip(".")


def _shader_number(value: float) -> str:
    rendered = _number(value)
    return rendered if "." in rendered else rendered + ".0"


def _color(value: list[float]) -> str:
    return ", ".join(_number(component) for component in value)


def write_runtime_materials(config: dict, output_root: Path) -> list[Path]:
    semantics = {item["name"]: item for item in resolved_material_semantics(config)}
    material_root = safe_output_path(output_root, Path("game/materials"))
    shader_root = safe_output_path(output_root, Path("game/shaders"))
    material_root.mkdir(parents=True, exist_ok=True)
    shader_root.mkdir(parents=True, exist_ok=True)
    pine = safe_output_path(output_root, Path("game/materials/pine.tres"))
    brick = safe_output_path(output_root, Path("game/materials/brick.tres"))
    glass = safe_output_path(output_root, Path("game/materials/glass.tres"))
    shader = safe_output_path(output_root, Path("game/shaders/stylized_glass.gdshader"))
    pine_spec = semantics["MAT_Pine"]
    brick_spec = semantics["MAT_Brick"]
    glass_spec = semantics["MAT_Glass"]
    glass_asset = next(asset for asset in config["assets"] if "MAT_Glass" in asset["materials"])
    glass_texture_path = f"res://assets/vertical_slice/{glass_asset['folder']}/{glass_asset['id']}_TEX_Glass.png"
    pine.write_text(
        "[gd_resource type=\"StandardMaterial3D\" format=3]\n\n[resource]\n"
        f"albedo_color = Color({_color(pine_spec['base_color'])})\n"
        f"roughness = {_number(pine_spec['roughness'])}\nmetallic = {_number(pine_spec['metallic'])}\n",
        encoding="utf-8",
        newline="\n",
    )
    brick.write_text(
        "[gd_resource type=\"StandardMaterial3D\" format=3]\n\n[resource]\n"
        f"albedo_color = Color({_color(brick_spec['base_color'])})\n"
        f"roughness = {_number(brick_spec['roughness'])}\nmetallic = {_number(brick_spec['metallic'])}\n",
        encoding="utf-8",
        newline="\n",
    )
    glass_color = glass_spec["base_color"]
    rim_color = [min(1.0, component * 0.6 + 0.45) for component in glass_color[:3]] + [1.0]
    shader.write_text(
        "shader_type spatial;\n"
        "render_mode blend_mix, depth_prepass_alpha, cull_back, diffuse_burley, specular_schlick_ggx;\n\n"
        "uniform sampler2D albedo_texture : source_color, filter_nearest_mipmap, repeat_enable;\n"
        f"uniform vec4 rim_tint : source_color = vec4({_color(rim_color)});\n"
        "uniform float rim_power : hint_range(0.5, 8.0) = 3.2;\n\n"
        f"uniform float authored_roughness = {_shader_number(glass_spec['roughness'])};\n"
        f"uniform float authored_metallic = {_shader_number(glass_spec['metallic'])};\n"
        f"uniform float transmission_weight = {_shader_number(glass_spec['transmission'])};\n"
        f"uniform float coat_weight = {_shader_number(glass_spec['coat'])};\n\n"
        "void fragment() {\n"
        "\tvec4 texel = texture(albedo_texture, UV);\n"
        "\tfloat rim = pow(1.0 - max(dot(NORMAL, VIEW), 0.0), rim_power);\n"
        "\tALBEDO = texel.rgb;\n"
        "\tROUGHNESS = authored_roughness;\n"
        "\tMETALLIC = authored_metallic;\n\tSPECULAR = clamp(0.5 + coat_weight * 0.5, 0.0, 1.0);\n"
        "\tALPHA = texel.a;\n"
        "\tEMISSION = rim_tint.rgb * rim * (0.12 + transmission_weight * 0.10);\n}\n",
        encoding="utf-8",
        newline="\n",
    )
    glass.write_text(
        "[gd_resource type=\"ShaderMaterial\" load_steps=3 format=3]\n\n"
        "[ext_resource type=\"Shader\" path=\"res://shaders/stylized_glass.gdshader\" id=\"1_glass\"]\n\n"
        f"[ext_resource type=\"Texture2D\" path=\"{glass_texture_path}\" id=\"2_tex\"]\n\n"
        "[resource]\nrender_priority = 0\nshader = ExtResource(\"1_glass\")\n"
        "shader_parameter/albedo_texture = ExtResource(\"2_tex\")\n"
        f"shader_parameter/rim_tint = Color({_color(rim_color)})\n"
        "shader_parameter/rim_power = 3.2\n"
        f"shader_parameter/authored_roughness = {_shader_number(glass_spec['roughness'])}\n"
        f"shader_parameter/authored_metallic = {_shader_number(glass_spec['metallic'])}\n"
        f"shader_parameter/transmission_weight = {_shader_number(glass_spec['transmission'])}\n"
        f"shader_parameter/coat_weight = {_shader_number(glass_spec['coat'])}\n",
        encoding="utf-8",
        newline="\n",
    )
    return [pine, brick, glass, shader]
