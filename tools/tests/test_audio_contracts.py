import hashlib
import io
import importlib.util
import json
import sys
import tempfile
import unittest
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MODULE_PATH = ROOT / "tools" / "audio" / "generate_audio.py"
CONFIG_PATH = ROOT / "tools" / "audio" / "vertical_slice_audio.json"
EXPECTED_IDS = {
    "launch",
    "vortex",
    "pine",
    "glass",
    "brick",
    "helmet",
    "vulnerable",
    "victory",
    "defeat",
}


def load_audio_module():
    if not MODULE_PATH.is_file():
        raise AssertionError(f"missing audio generator: {MODULE_PATH.relative_to(ROOT)}")
    spec = importlib.util.spec_from_file_location("ninho_audio_generator", MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class AudioSourceContractTests(unittest.TestCase):
    def test_authored_config_has_exact_vertical_slice_coverage(self):
        self.assertTrue(CONFIG_PATH.is_file(), "missing authored audio config")
        config = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
        self.assertEqual(config["schema_version"], 1)
        self.assertIsInstance(config["seed"], int)
        self.assertEqual(config["author"], "Ninho Orbital")
        self.assertEqual(config["license"], "proprietary-original")
        self.assertEqual(config["sample_rate_hz"], 48_000)
        self.assertEqual(config["channels"], 1)
        self.assertEqual(config["sample_width_bytes"], 2)
        sounds = config["sounds"]
        self.assertEqual({sound["id"] for sound in sounds}, EXPECTED_IDS)
        self.assertEqual(len(sounds), len(EXPECTED_IDS))
        self.assertEqual(len({sound["file"] for sound in sounds}), len(EXPECTED_IDS))
        self.assertEqual(len({sound["profile"] for sound in sounds}), len(EXPECTED_IDS))
        for sound in sounds:
            self.assertRegex(sound["file"], rf"^{sound['id']}\.wav$")
            self.assertGreater(sound["duration_ms"], 80)
            self.assertLessEqual(sound["duration_ms"], 2_500)
            self.assertGreater(sound["peak"], 0.05)
            self.assertLessEqual(sound["peak"], 0.95)

    def test_output_root_rejects_escape_and_root_itself(self):
        module = load_audio_module()
        with tempfile.TemporaryDirectory() as temporary:
            allowed = Path(temporary).resolve()
            with self.assertRaisesRegex(ValueError, "outside allowed root"):
                module.resolve_output_root(allowed.parent, allowed)
            with self.assertRaisesRegex(ValueError, "must not equal allowed root"):
                module.resolve_output_root(allowed, allowed)

    def test_build_hash_is_canonical_and_sensitive_to_outputs(self):
        module = load_audio_module()
        generator = {
            "config_sha256": "a" * 64,
            "generator_sha256": "b" * 64,
            "seed": 123,
        }
        outputs = [
            {"id": "launch", "path": "game/assets/audio/generated/launch.wav", "sha256": "c" * 64},
            {"id": "vortex", "path": "game/assets/audio/generated/vortex.wav", "sha256": "d" * 64},
        ]
        first = module.compute_build_hash(generator, outputs)
        self.assertRegex(first, "^[0-9a-f]{64}$")
        self.assertEqual(first, module.compute_build_hash(generator, list(reversed(outputs))))
        changed = [dict(outputs[0]), dict(outputs[1])]
        changed[0]["sha256"] = "e" * 64
        self.assertNotEqual(first, module.compute_build_hash(generator, changed))

    def test_text_source_hash_is_line_ending_independent_and_sensitive(self):
        module = load_audio_module()
        lf = b"first line\nsecond line\n"
        crlf = b"first line\r\nsecond line\r\n"
        changed = b"first line\nchanged line\n"
        self.assertEqual(module.text_source_sha256(lf), module.text_source_sha256(crlf))
        self.assertNotEqual(module.text_source_sha256(lf), module.text_source_sha256(changed))

    def test_pcm_encoder_is_deterministic_and_bounded(self):
        module = load_audio_module()
        config = {
            "sample_rate_hz": 48_000,
            "channels": 1,
            "sample_width_bytes": 2,
            "seed": 17,
        }
        sound = {
            "id": "launch",
            "file": "launch.wav",
            "profile": "harmonic_rise",
            "duration_ms": 240,
            "peak": 0.72,
            "parameters": {"start_hz": 180.0, "end_hz": 720.0},
        }
        first = module.render_wav_bytes(config, sound)
        second = module.render_wav_bytes(config, sound)
        self.assertEqual(first, second)
        self.assertEqual(hashlib.sha256(first).hexdigest(), hashlib.sha256(second).hexdigest())
        with wave.open(io.BytesIO(first), "rb") as stream:
            self.assertEqual(stream.getnchannels(), 1)
            self.assertEqual(stream.getsampwidth(), 2)
            self.assertEqual(stream.getframerate(), 48_000)
            self.assertEqual(stream.getnframes(), 11_520)


if __name__ == "__main__":
    unittest.main(verbosity=2)
