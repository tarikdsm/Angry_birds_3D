"""Original palette and deterministic PBR material construction."""

from __future__ import annotations

import bpy
from pathlib import Path

from .contracts import safe_output_path


MATERIAL_SPECS = {
    "MAT_AsterSoil": ("aster_soil", 0.94, 0.0, 1.0),
    "MAT_AsterMoss": ("aster_moss", 0.86, 0.0, 1.0),
    "MAT_GardenGold": ("garden_gold", 0.58, 0.08, 1.0),
    "MAT_VirelaTeal": ("virela_teal", 0.48, 0.04, 1.0),
    "MAT_AnchorPlum": ("anchor_plum", 0.62, 0.02, 1.0),
    "MAT_Pine": ("pine", 0.76, 0.0, 1.0),
    "MAT_Brick": ("brick", 0.82, 0.0, 1.0),
    "MAT_Glass": ("glass", 0.18, 0.0, 0.56),
    "MAT_ImpulseTeal": ("virela_teal", 0.26, 0.22, 0.82),
    "MAT_Ink": ("ink", 0.42, 0.0, 1.0),
    "MAT_Cream": ("cream", 0.54, 0.0, 1.0),
}


def _set_input(node: bpy.types.Node, name: str, value: object) -> None:
    socket = node.inputs.get(name)
    if socket is not None:
        socket.default_value = value


def build_materials(config: dict) -> dict[str, bpy.types.Material]:
    palette = config["palette"]
    result: dict[str, bpy.types.Material] = {}
    for name in sorted(MATERIAL_SPECS):
        palette_name, roughness, metallic, alpha = MATERIAL_SPECS[name]
        color = tuple(float(component) for component in palette[palette_name])
        material = bpy.data.materials.new(name=name)
        material.use_nodes = True
        material.diffuse_color = color
        material["ninho_original"] = True
        material["palette_key"] = palette_name
        material["pipeline_seed"] = int(config["seed"])
        shader = material.node_tree.nodes.get("Principled BSDF")
        if shader is not None:
            _set_input(shader, "Base Color", color)
            _set_input(shader, "Roughness", roughness)
            _set_input(shader, "Metallic", metallic)
            _set_input(shader, "Alpha", alpha)
            if name == "MAT_ImpulseTeal":
                _set_input(shader, "Emission Color", color)
                _set_input(shader, "Emission Strength", 2.0)
            if name == "MAT_Glass":
                _set_input(shader, "Transmission Weight", 0.78)
                _set_input(shader, "Coat Weight", 0.24)
        if alpha < 1.0:
            material.surface_render_method = "DITHERED"
        result[name] = material
    return result


def _number(value: float) -> str:
    return f"{float(value):.6f}".rstrip("0").rstrip(".")


def _color(value: list[float]) -> str:
    return ", ".join(_number(component) for component in value)


def write_runtime_materials(config: dict, output_root: Path) -> list[Path]:
    palette = config["palette"]
    material_root = safe_output_path(output_root, Path("game/materials"))
    shader_root = safe_output_path(output_root, Path("game/shaders"))
    material_root.mkdir(parents=True, exist_ok=True)
    shader_root.mkdir(parents=True, exist_ok=True)
    pine = safe_output_path(output_root, Path("game/materials/pine.tres"))
    brick = safe_output_path(output_root, Path("game/materials/brick.tres"))
    glass = safe_output_path(output_root, Path("game/materials/glass.tres"))
    shader = safe_output_path(output_root, Path("game/shaders/stylized_glass.gdshader"))
    pine.write_text(
        "[gd_resource type=\"StandardMaterial3D\" format=3]\n\n[resource]\n"
        f"albedo_color = Color({_color(palette['pine'])})\nroughness = 0.76\nmetallic = 0.0\n",
        encoding="utf-8",
        newline="\n",
    )
    brick.write_text(
        "[gd_resource type=\"StandardMaterial3D\" format=3]\n\n[resource]\n"
        f"albedo_color = Color({_color(palette['brick'])})\nroughness = 0.82\nmetallic = 0.0\n",
        encoding="utf-8",
        newline="\n",
    )
    glass_color = palette["glass"]
    rim_color = [min(1.0, component * 0.6 + 0.45) for component in glass_color[:3]] + [1.0]
    shader.write_text(
        "shader_type spatial;\n"
        "render_mode blend_mix, depth_prepass_alpha, cull_back, diffuse_burley, specular_schlick_ggx;\n\n"
        f"uniform vec4 glass_tint : source_color = vec4({_color(glass_color)});\n"
        f"uniform vec4 rim_tint : source_color = vec4({_color(rim_color)});\n"
        "uniform float rim_power : hint_range(0.5, 8.0) = 3.2;\n\n"
        "void fragment() {\n"
        "\tfloat rim = pow(1.0 - max(dot(NORMAL, VIEW), 0.0), rim_power);\n"
        "\tALBEDO = mix(glass_tint.rgb, rim_tint.rgb, rim * 0.58);\n"
        "\tROUGHNESS = 0.16;\n\tMETALLIC = 0.02;\n\tSPECULAR = 0.78;\n"
        "\tALPHA = clamp(glass_tint.a + rim * 0.28, 0.0, 1.0);\n"
        "\tEMISSION = rim_tint.rgb * rim * 0.22;\n}\n",
        encoding="utf-8",
        newline="\n",
    )
    glass.write_text(
        "[gd_resource type=\"ShaderMaterial\" load_steps=2 format=3]\n\n"
        "[ext_resource type=\"Shader\" path=\"res://shaders/stylized_glass.gdshader\" id=\"1_glass\"]\n\n"
        "[resource]\nrender_priority = 0\nshader = ExtResource(\"1_glass\")\n"
        f"shader_parameter/glass_tint = Color({_color(glass_color)})\n"
        f"shader_parameter/rim_tint = Color({_color(rim_color)})\n"
        "shader_parameter/rim_power = 3.2\n",
        encoding="utf-8",
        newline="\n",
    )
    return [pine, brick, glass, shader]
