"""Deterministic, standard-library-only audio authoring for the vertical slice."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import math
import shutil
import struct
import sys
import wave
from pathlib import Path
from typing import Callable


SCRIPT_PATH = Path(__file__).resolve()
REPO_ROOT = SCRIPT_PATH.parents[2]
CONFIG_PATH = SCRIPT_PATH.with_name("vertical_slice_audio.json")
MANIFEST_RELATIVE = Path("tools/audio/audio_manifest.json")
README_RELATIVE = Path("game/assets/audio/README.md")
AUDIO_DIRECTORY_RELATIVE = Path("game/assets/audio/generated")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def canonical_text_bytes(data: bytes) -> bytes:
    text = data.decode("utf-8")
    return text.replace("\r\n", "\n").replace("\r", "\n").encode("utf-8")


def text_source_sha256(data: bytes) -> str:
    return sha256_bytes(canonical_text_bytes(data))


def canonical_json(value: object) -> bytes:
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")


def resolve_output_root(candidate: Path, allowed_root: Path) -> Path:
    candidate = candidate.resolve()
    allowed_root = allowed_root.resolve()
    if candidate == allowed_root:
        raise ValueError("output root must not equal allowed root")
    try:
        candidate.relative_to(allowed_root)
    except ValueError as error:
        raise ValueError("output root is outside allowed root") from error
    return candidate


def _sound_seed(global_seed: int, sound_id: str) -> int:
    digest = hashlib.sha256(f"{global_seed}:{sound_id}".encode("utf-8")).digest()
    return int.from_bytes(digest[:4], "little") or 1


class Noise:
    def __init__(self, seed: int):
        self.state = seed & 0xFFFFFFFF or 1

    def sample(self) -> float:
        value = self.state
        value ^= (value << 13) & 0xFFFFFFFF
        value ^= value >> 17
        value ^= (value << 5) & 0xFFFFFFFF
        self.state = value & 0xFFFFFFFF
        return (self.state / 2147483647.5) - 1.0


def _attack_release(t: float, duration: float, attack: float, release: float) -> float:
    attack_gain = min(1.0, t / max(attack, 1e-9))
    release_gain = min(1.0, max(0.0, duration - t) / max(release, 1e-9))
    return attack_gain * release_gain


def _exp_decay(t: float, rate: float) -> float:
    return math.exp(-rate * max(0.0, t))


def _harmonic_rise(t: float, duration: float, p: dict, noise: Noise) -> float:
    progress = min(1.0, t / duration)
    start = float(p["start_hz"])
    end = float(p["end_hz"])
    phase = 2.0 * math.pi * (start * t + 0.5 * (end - start) * t * t / duration)
    tonal = math.sin(phase) + 0.32 * math.sin(phase * 2.01) + 0.16 * math.sin(phase * 3.02)
    pulse = math.sin(2.0 * math.pi * float(p.get("pulse_hz", 72.0)) * t) * _exp_decay(max(0.0, t - duration * 0.62), 10.0)
    return (tonal * (0.45 + progress * 0.35) + pulse * 0.22) * _attack_release(t, duration, 0.035, 0.16)


def _gravitational_vortex(t: float, duration: float, p: dict, noise: Noise) -> float:
    orbit = math.sin(2.0 * math.pi * float(p["orbit_hz"]) * t)
    base = float(p["base_hz"])
    phase = 2.0 * math.pi * base * t + orbit * 1.8
    rumble = math.sin(phase) + 0.52 * math.sin(phase * 0.5 + 0.3) + 0.22 * math.sin(phase * 2.03)
    air = noise.sample() * float(p["air"]) * (0.35 + 0.65 * abs(orbit))
    return (rumble * 0.55 + air) * _attack_release(t, duration, 0.16, 0.24)


def _fibrous_snap(t: float, duration: float, p: dict, noise: Noise) -> float:
    body = math.sin(2.0 * math.pi * float(p["body_hz"]) * t) * _exp_decay(t, 13.0)
    fiber = math.sin(2.0 * math.pi * float(p["fiber_hz"]) * t + 0.8) * _exp_decay(t, 25.0)
    crack = noise.sample() * float(p["noise"]) * _exp_decay(t, 34.0)
    delayed = 0.0
    if t > 0.055:
        local = t - 0.055
        delayed = (noise.sample() * 0.34 + math.sin(2.0 * math.pi * 890.0 * local) * 0.22) * _exp_decay(local, 28.0)
    return (body * 0.72 + fiber * 0.34 + crack + delayed) * _attack_release(t, duration, 0.002, 0.06)


def _prismatic_chime(t: float, duration: float, p: dict, noise: Noise) -> float:
    root = float(p["fundamental_hz"])
    frequencies = (root, root * float(p["ratio_a"]), root * float(p["ratio_b"]), root * 3.176)
    rates = (4.4, 5.7, 7.1, 9.0)
    tones = sum(math.sin(2.0 * math.pi * frequency * t + index * 0.47) * _exp_decay(t, rates[index]) for index, frequency in enumerate(frequencies))
    onset = noise.sample() * 0.13 * _exp_decay(t, 38.0)
    return (tones * 0.3 + onset) * _attack_release(t, duration, 0.002, 0.11)


def _dry_masonry_impact(t: float, duration: float, p: dict, noise: Noise) -> float:
    thud = math.sin(2.0 * math.pi * float(p["body_hz"]) * t) * _exp_decay(t, 16.0)
    knock = math.sin(2.0 * math.pi * float(p["knock_hz"]) * t + 0.5) * _exp_decay(t, 21.0)
    grit = noise.sample() * float(p["grit"]) * _exp_decay(t, 31.0)
    grit *= 0.55 + 0.45 * math.sin(2.0 * math.pi * 37.0 * t) ** 2
    return (thud * 0.8 + knock * 0.36 + grit) * _attack_release(t, duration, 0.0015, 0.055)


def _muted_guard_chord(t: float, duration: float, p: dict, noise: Noise) -> float:
    root = float(p["root_hz"])
    damping = float(p["damping"])
    chord = (
        math.sin(2.0 * math.pi * root * t)
        + 0.62 * math.sin(2.0 * math.pi * root * 1.25 * t + 0.2)
        + 0.46 * math.sin(2.0 * math.pi * root * 1.5 * t + 0.4)
    ) * _exp_decay(t, damping)
    spark = math.sin(2.0 * math.pi * float(p["spark_hz"]) * t) * _exp_decay(t, 32.0)
    return (chord * 0.48 + spark * 0.13) * _attack_release(t, duration, 0.002, 0.12)


def _amber_low_pulse(t: float, duration: float, p: dict, noise: Noise) -> float:
    swell = 0.56 + 0.44 * math.sin(2.0 * math.pi * float(p["swell_hz"]) * t - math.pi * 0.5)
    low = math.sin(2.0 * math.pi * float(p["root_hz"]) * t + 0.18 * math.sin(2.0 * math.pi * 4.2 * t))
    overtone = math.sin(2.0 * math.pi * float(p["overtone_hz"]) * t + 0.35)
    return (low * 0.72 + overtone * 0.26) * swell * _attack_release(t, duration, 0.055, 0.2)


def _stepped_result(t: float, duration: float, p: dict, ascending: bool) -> float:
    step_seconds = float(p["step_ms"]) / 1000.0
    notes = (1.0, 1.25, 1.5, 2.0) if ascending else (1.5, 1.25, 1.0, 0.75)
    total = 0.0
    for index, ratio in enumerate(notes):
        start = index * step_seconds
        if t < start:
            continue
        local = t - start
        frequency = float(p["root_hz"]) * ratio
        tone = math.sin(2.0 * math.pi * frequency * local)
        tone += 0.28 * math.sin(2.0 * math.pi * frequency * 2.01 * local + 0.25)
        total += tone * _exp_decay(local, 2.8 if ascending else 3.8)
    return total * 0.38 * _attack_release(t, duration, 0.015, 0.25)


def _ascending_constellation(t: float, duration: float, p: dict, noise: Noise) -> float:
    shimmer = noise.sample() * float(p["shimmer"]) * _attack_release(t, duration, 0.08, 0.3)
    return _stepped_result(t, duration, p, True) + shimmer


def _descending_eclipse(t: float, duration: float, p: dict, noise: Noise) -> float:
    air = noise.sample() * float(p["air"]) * _attack_release(t, duration, 0.03, 0.24)
    return _stepped_result(t, duration, p, False) + air


PROFILES: dict[str, Callable[[float, float, dict, Noise], float]] = {
    "harmonic_rise": _harmonic_rise,
    "gravitational_vortex": _gravitational_vortex,
    "fibrous_snap": _fibrous_snap,
    "prismatic_chime": _prismatic_chime,
    "dry_masonry_impact": _dry_masonry_impact,
    "muted_guard_chord": _muted_guard_chord,
    "amber_low_pulse": _amber_low_pulse,
    "ascending_constellation": _ascending_constellation,
    "descending_eclipse": _descending_eclipse,
}


def validate_config(config: dict) -> None:
    if config.get("schema_version") != 1 or not isinstance(config.get("seed"), int):
        raise ValueError("invalid audio config schema or seed")
    if config.get("author") != "Ninho Orbital" or config.get("license") != "proprietary-original":
        raise ValueError("invalid audio provenance")
    if config.get("generation_mode") != "procedural-offline" or config.get("external_sources") != []:
        raise ValueError("audio must be original offline synthesis without external sources")
    if (config.get("sample_rate_hz"), config.get("channels"), config.get("sample_width_bytes")) != (48000, 1, 2):
        raise ValueError("audio format must be mono 48 kHz PCM16")
    expected = {"launch", "vortex", "pine", "glass", "brick", "helmet", "vulnerable", "victory", "defeat"}
    sounds = config.get("sounds")
    if not isinstance(sounds, list) or len(sounds) != len(expected) or {sound.get("id") for sound in sounds} != expected:
        raise ValueError("audio coverage differs from vertical slice contract")
    files = {sound.get("file") for sound in sounds}
    profiles = {sound.get("profile") for sound in sounds}
    if len(files) != len(expected) or len(profiles) != len(expected) or not profiles.issubset(PROFILES):
        raise ValueError("audio files and profiles must be unique and supported")
    for sound in sounds:
        if sound["file"] != f"{sound['id']}.wav" or not 80 < int(sound["duration_ms"]) <= 2500:
            raise ValueError(f"invalid duration or filename: {sound['id']}")
        if not 0.05 < float(sound["peak"]) <= 0.95 or not isinstance(sound.get("parameters"), dict):
            raise ValueError(f"invalid synthesis parameters: {sound['id']}")


def render_wav_bytes(config: dict, sound: dict) -> bytes:
    sample_rate = int(config["sample_rate_hz"])
    frame_count = round(sample_rate * int(sound["duration_ms"]) / 1000.0)
    duration = frame_count / sample_rate
    generator = PROFILES[sound["profile"]]
    noise = Noise(_sound_seed(int(config["seed"]), sound["id"]))
    samples = [generator(index / sample_rate, duration, sound["parameters"], noise) for index in range(frame_count)]
    absolute_peak = max(abs(value) for value in samples)
    if absolute_peak <= 1e-12:
        raise ValueError(f"synthesis produced silence: {sound['id']}")
    scale = float(sound["peak"]) * 32767.0 / absolute_peak
    pcm = b"".join(struct.pack("<h", max(-32768, min(32767, round(value * scale)))) for value in samples)
    output = io.BytesIO()
    with wave.open(output, "wb") as stream:
        stream.setnchannels(int(config["channels"]))
        stream.setsampwidth(int(config["sample_width_bytes"]))
        stream.setframerate(sample_rate)
        stream.writeframes(pcm)
    return output.getvalue()


def compute_build_hash(generator: dict, outputs: list[dict]) -> str:
    normative_generator = {
        "config_sha256": generator["config_sha256"],
        "generator_sha256": generator["generator_sha256"],
        "seed": generator["seed"],
    }
    normative_outputs = [
        {"id": output["id"], "path": output["path"], "sha256": output["sha256"]}
        for output in sorted(outputs, key=lambda item: item["id"])
    ]
    return sha256_bytes(canonical_json({"generator": normative_generator, "outputs": normative_outputs}))


def inspect_wav(data: bytes) -> dict:
    with wave.open(io.BytesIO(data), "rb") as stream:
        channels = stream.getnchannels()
        width = stream.getsampwidth()
        rate = stream.getframerate()
        frames = stream.getnframes()
        compression = stream.getcomptype()
        pcm = stream.readframes(frames)
    if compression != "NONE" or width != 2:
        raise ValueError("WAV must be uncompressed PCM16")
    values = struct.unpack(f"<{len(pcm) // 2}h", pcm)
    return {
        "sample_rate_hz": rate,
        "channels": channels,
        "sample_width_bytes": width,
        "frame_count": frames,
        "duration_ms": round(frames * 1000 / rate),
        "peak_sample": max(abs(value) for value in values),
    }


def _source_contract(config_bytes: bytes) -> dict:
    config_bytes = canonical_text_bytes(config_bytes)
    config = json.loads(config_bytes.decode("utf-8"))
    return {
        "kind": "Python standard-library procedural PCM synthesis",
        "author": config["author"],
        "license": config["license"],
        "seed": config["seed"],
        "generation_mode": config["generation_mode"],
        "external_sources": config["external_sources"],
        "config_path": "tools/audio/vertical_slice_audio.json",
        "config_sha256": text_source_sha256(config_bytes),
        "generator_path": "tools/audio/generate_audio.py",
        "generator_sha256": text_source_sha256(SCRIPT_PATH.read_bytes()),
    }


def _render_readme(manifest: dict) -> str:
    generator = manifest["generator"]
    lines = [
        "# Áudio procedural do vertical slice",
        "",
        "Os arquivos desta pasta foram sintetizados offline, sem downloads, samples externos ou material de terceiros.",
        "",
        f"- Autoria: {generator['author']}",
        f"- Licença: {generator['license']}",
        f"- Seed global: {generator['seed']}",
        f"- Configuração: `{generator['config_path']}` (`{generator['config_sha256']}`)",
        f"- Gerador: `{generator['generator_path']}` (`{generator['generator_sha256']}`)",
        f"- Build hash: `{manifest['build_hash']}`",
        "- Formato: WAV mono, 48 kHz, PCM16",
        "",
        "## Arquivos",
        "",
        "| Evento | Perfil autoral | Arquivo | SHA-256 |",
        "|---|---|---|---|",
    ]
    for output in manifest["outputs"]:
        lines.append(f"| {output['id']} | {output['profile']} | `{output['path']}` | `{output['sha256']}` |")
    lines.extend([
        "",
        "Cada efeito usa osciladores, envelopes e ruído pseudoaleatório próprios definidos no gerador rastreado. A licença `proprietary-original` indica conteúdo original do projeto; não concede licença sobre marcas ou nomes ainda sujeitos a clearance.",
        "",
    ])
    return "\n".join(lines)


def build(output_root: Path, allowed_root: Path, clean: bool) -> dict:
    output_root = resolve_output_root(output_root, allowed_root)
    config_bytes = canonical_text_bytes(CONFIG_PATH.read_bytes())
    config = json.loads(config_bytes.decode("utf-8"))
    validate_config(config)
    audio_directory = output_root / AUDIO_DIRECTORY_RELATIVE
    if clean and audio_directory.exists():
        resolved_audio = audio_directory.resolve()
        resolved_audio.relative_to(output_root)
        for child in resolved_audio.iterdir():
            if child.is_file() and child.name.endswith(".wav.import"):
                continue
            if child.is_symlink() or child.is_file():
                child.unlink()
            elif child.is_dir():
                shutil.rmtree(child)
    audio_directory.mkdir(parents=True, exist_ok=True)
    generator = _source_contract(config_bytes)
    outputs = []
    for sound in config["sounds"]:
        data = render_wav_bytes(config, sound)
        relative = (AUDIO_DIRECTORY_RELATIVE / sound["file"]).as_posix()
        target = output_root / Path(relative)
        target.write_bytes(data)
        metadata = inspect_wav(data)
        outputs.append({
            "id": sound["id"],
            "path": relative,
            "profile": sound["profile"],
            "author": config["author"],
            "license": config["license"],
            "seed": _sound_seed(config["seed"], sound["id"]),
            "source_config_sha256": generator["config_sha256"],
            "generator_sha256": generator["generator_sha256"],
            "sha256": sha256_bytes(data),
            **metadata,
        })
    outputs.sort(key=lambda item: item["id"])
    manifest = {"schema_version": 1, "generator": generator, "outputs": outputs}
    manifest["build_hash"] = compute_build_hash(generator, outputs)
    manifest_path = output_root / MANIFEST_RELATIVE
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    readme_path = output_root / README_RELATIVE
    readme_path.parent.mkdir(parents=True, exist_ok=True)
    readme_path.write_text(_render_readme(manifest), encoding="utf-8", newline="\n")
    return manifest


def validate(output_root: Path, allowed_root: Path) -> dict:
    output_root = resolve_output_root(output_root, allowed_root)
    config_bytes = canonical_text_bytes(CONFIG_PATH.read_bytes())
    config = json.loads(config_bytes.decode("utf-8"))
    validate_config(config)
    manifest_path = output_root / MANIFEST_RELATIVE
    if not manifest_path.is_file():
        raise ValueError("missing audio manifest")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    expected_generator = _source_contract(config_bytes)
    if manifest.get("schema_version") != 1 or manifest.get("generator") != expected_generator:
        raise ValueError("audio manifest provenance mismatch")
    expected_sounds = {sound["id"]: sound for sound in config["sounds"]}
    outputs = manifest.get("outputs")
    if not isinstance(outputs, list) or {output.get("id") for output in outputs} != set(expected_sounds) or len(outputs) != len(expected_sounds):
        raise ValueError("audio manifest coverage mismatch")
    verified_outputs = []
    for recorded in outputs:
        sound = expected_sounds[recorded["id"]]
        expected_path = (AUDIO_DIRECTORY_RELATIVE / sound["file"]).as_posix()
        if recorded.get("path") != expected_path:
            raise ValueError(f"audio path mismatch: {sound['id']}")
        path = output_root / Path(expected_path)
        if not path.is_file():
            raise ValueError(f"missing audio output: {sound['id']}")
        actual_data = path.read_bytes()
        actual_hash = sha256_bytes(actual_data)
        if actual_hash != recorded.get("sha256"):
            raise ValueError(f"hash mismatch for audio output: {sound['id']}")
        expected_data = render_wav_bytes(config, sound)
        if actual_data != expected_data:
            raise ValueError(f"audio output differs from procedural source: {sound['id']}")
        metadata = inspect_wav(actual_data)
        expected_record = {
            "id": sound["id"],
            "path": expected_path,
            "profile": sound["profile"],
            "author": config["author"],
            "license": config["license"],
            "seed": _sound_seed(config["seed"], sound["id"]),
            "source_config_sha256": expected_generator["config_sha256"],
            "generator_sha256": expected_generator["generator_sha256"],
            "sha256": actual_hash,
            **metadata,
        }
        if recorded != expected_record:
            raise ValueError(f"audio metadata mismatch: {sound['id']}")
        verified_outputs.append(expected_record)
    verified_outputs.sort(key=lambda item: item["id"])
    if manifest.get("outputs") != verified_outputs:
        raise ValueError("audio manifest output order is not canonical")
    expected_hash = compute_build_hash(expected_generator, verified_outputs)
    if manifest.get("build_hash") != expected_hash:
        raise ValueError("audio build hash mismatch")
    readme_path = output_root / README_RELATIVE
    if not readme_path.is_file() or readme_path.read_text(encoding="utf-8") != _render_readme(manifest):
        raise ValueError("audio provenance README mismatch")
    return manifest


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=("build", "validate"))
    parser.add_argument("--output-root", type=Path, default=REPO_ROOT)
    parser.add_argument("--allowed-root", type=Path)
    parser.add_argument("--clean", action="store_true")
    args = parser.parse_args(argv)
    output_root = args.output_root.resolve()
    allowed_root = (args.allowed_root or output_root.parent).resolve()
    try:
        manifest = build(output_root, allowed_root, args.clean) if args.command == "build" else validate(output_root, allowed_root)
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
        print(f"AUDIO_PIPELINE_ERROR: {error}", file=sys.stderr)
        return 1
    print(f"NINHO_AUDIO_BUILD_HASH={manifest['build_hash']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
