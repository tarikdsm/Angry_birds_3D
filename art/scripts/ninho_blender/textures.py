"""Pure deterministic RGBA texture generation and PNG contracts."""

from __future__ import annotations

import hashlib
import math
import struct
import zlib


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def _procedural_value(pattern: str, x: int, y: int, size: int, seed: int) -> float:
    nx = x / max(1, size - 1)
    ny = y / max(1, size - 1)
    noise = (((x * 73856093) ^ (y * 19349663) ^ seed) & 255) / 255.0 - 0.5
    if pattern == "wood":
        return 0.65 * math.sin((nx * 9.0 + 0.16 * math.sin(ny * 13.0)) * math.tau) + 0.35 * noise
    if pattern == "brick":
        mortar = (y % 8 == 0) or ((x + (4 if (y // 8) % 2 else 0)) % 12 == 0)
        return -1.0 if mortar else 0.5 * noise
    if pattern == "glass":
        return 0.55 * math.sin((nx + ny) * math.tau * 4.0) + 0.25 * noise
    if pattern == "pulse":
        return math.cos(math.hypot(nx - 0.5, ny - 0.5) * math.tau * 7.0)
    if pattern in {"woven", "feather", "scale"}:
        return 0.55 * math.sin(nx * math.tau * 5.0) * math.cos(ny * math.tau * 6.0) + 0.25 * noise
    if pattern == "moss":
        return 0.7 * noise + 0.3 * math.sin((nx + ny) * math.tau * 3.0)
    return noise


def generate_material_rgba(config: dict, spec: dict) -> bytes:
    texture = spec["texture"]
    size = int(texture["size_px"])
    color = [float(value) for value in config["palette"][spec["palette_key"]]]
    local_seed = int.from_bytes(
        hashlib.sha256(f"{config['seed']}:{spec['name']}".encode("utf-8")).digest()[:4], "little"
    )
    contrast = float(texture["contrast"])
    result = bytearray()
    for y in range(size):
        for x in range(size):
            variation = _procedural_value(texture["pattern"], x, y, size, local_seed) * contrast
            result.extend(
                round(min(1.0, max(0.0, color[channel] * (1.0 + variation))) * 255.0)
                for channel in range(3)
            )
            result.append(round(float(spec["alpha"]) * 255.0))
    return bytes(result)


def _chunk(kind: bytes, payload: bytes) -> bytes:
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload))


def encode_png_rgba(width: int, height: int, rgba: bytes) -> bytes:
    if len(rgba) != width * height * 4:
        raise ValueError("RGBA byte count differs from PNG dimensions")
    stride = width * 4
    scanlines = b"".join(b"\0" + rgba[offset : offset + stride] for offset in range(0, len(rgba), stride))
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return PNG_SIGNATURE + _chunk(b"IHDR", header) + _chunk(b"IDAT", zlib.compress(scanlines, 9)) + _chunk(b"IEND", b"")


def encode_solid_png_rgba(width: int, height: int, color: tuple[int, int, int, int]) -> bytes:
    compressor = zlib.compressobj(9)
    compressed = bytearray()
    row = b"\0" + bytes(color) * width
    for _ in range(height):
        compressed.extend(compressor.compress(row))
    compressed.extend(compressor.flush())
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return PNG_SIGNATURE + _chunk(b"IHDR", header) + _chunk(b"IDAT", bytes(compressed)) + _chunk(b"IEND", b"")


def inspect_png_dimensions(payload: bytes) -> tuple[int, int]:
    if not payload.startswith(PNG_SIGNATURE) or payload[12:16] != b"IHDR":
        raise ValueError("PNG IHDR is missing")
    width, height, depth, color_type, compression, filtering, interlace = struct.unpack(
        ">IIBBBBB", payload[16:29]
    )
    if (depth, color_type, compression, filtering, interlace) != (8, 6, 0, 0, 0):
        raise ValueError("PNG must be non-interlaced 8-bit RGBA")
    return width, height


def decoded_rgba8_mip_bytes(width: int, height: int) -> int:
    total = 0
    while True:
        total += width * height * 4
        if width == 1 and height == 1:
            return total
        width = max(1, width // 2)
        height = max(1, height // 2)


def inspect_png_rgba(payload: bytes) -> dict:
    if not payload.startswith(PNG_SIGNATURE):
        raise ValueError("embedded image is not PNG")
    offset = len(PNG_SIGNATURE)
    width, height = inspect_png_dimensions(payload)
    compressed = bytearray()
    while offset < len(payload):
        length = struct.unpack_from(">I", payload, offset)[0]
        kind = payload[offset + 4 : offset + 8]
        data = payload[offset + 8 : offset + 8 + length]
        if kind == b"IHDR":
            width, height, depth, color_type, compression, filtering, interlace = struct.unpack(">IIBBBBB", data)
            if (depth, color_type, compression, filtering, interlace) != (8, 6, 0, 0, 0):
                raise ValueError("PNG must be non-interlaced 8-bit RGBA")
        elif kind == b"IDAT":
            compressed.extend(data)
        elif kind == b"IEND":
            break
        offset += 12 + length
    raw = zlib.decompress(bytes(compressed))
    stride = width * 4
    rows = []
    for row in range(height):
        start = row * (stride + 1)
        if raw[start] != 0:
            raise ValueError("PNG uses unsupported nonzero row filter")
        rows.append(raw[start + 1 : start + 1 + stride])
    rgba = b"".join(rows)
    colors = {rgba[index : index + 4] for index in range(0, len(rgba), 4)}
    return {
        "width": width,
        "height": height,
        "pixel_sha256": hashlib.sha256(rgba).hexdigest(),
        "unique_colors": len(colors),
        "has_nonblack_rgb": any(color[:3] != b"\0\0\0" for color in colors),
        "alpha_values": sorted({color[3] for color in colors}),
    }
