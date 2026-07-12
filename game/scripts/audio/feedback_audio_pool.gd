extends Node3D

const CUE_PATHS := {
	"launch": "res://assets/audio/generated/launch.wav",
	"vortex": "res://assets/audio/generated/vortex.wav",
	"pine": "res://assets/audio/generated/pine.wav",
	"glass": "res://assets/audio/generated/glass.wav",
	"brick": "res://assets/audio/generated/brick.wav",
	"helmet": "res://assets/audio/generated/helmet.wav",
	"vulnerable": "res://assets/audio/generated/vulnerable.wav",
	"victory": "res://assets/audio/generated/victory.wav",
	"defeat": "res://assets/audio/generated/defeat.wav",
}

@export_range(1, 32, 1) var voice_count := 12

var _voices: Array[AudioStreamPlayer3D] = []
var _streams: Dictionary = {}
var _load_failures: Array[String] = []
var _cursor := 0
var _playback_requests := 0
var _playback_successes := 0


func _ready() -> void:
	for index in range(voice_count):
		var voice := AudioStreamPlayer3D.new()
		voice.name = "Voice%02d" % index
		voice.max_distance = 36.0
		voice.attenuation_model = AudioStreamPlayer3D.ATTENUATION_INVERSE_DISTANCE
		voice.unit_size = 5.0
		voice.volume_db = -4.0
		add_child(voice)
		_voices.append(voice)
	for cue: String in CUE_PATHS:
		var path := str(CUE_PATHS[cue])
		if not ResourceLoader.exists(path):
			_load_failures.append(cue)
			push_error("procedural audio import is unavailable: %s" % path)
			continue
		var stream := load(path) as AudioStreamWAV
		if stream == null:
			_load_failures.append(cue)
			push_error("procedural audio import is not AudioStreamWAV: %s" % path)
			continue
		_streams[cue] = stream


func play_cue(cue: String, position: Vector3) -> bool:
	_playback_requests += 1
	if cue not in _streams or _voices.is_empty():
		return false
	var voice := _voices[_cursor]
	_cursor = (_cursor + 1) % _voices.size()
	voice.stop()
	voice.stream = _streams[cue]
	voice.global_position = position
	voice.play()
	_playback_successes += 1
	return true


func reset_pool() -> void:
	for voice: AudioStreamPlayer3D in _voices:
		voice.stop()
		voice.stream = null
	_cursor = 0


func available_cues() -> Array:
	var result: Array[String] = []
	for cue: String in _streams:
		result.append(cue)
	result.sort()
	return result


func active_voice_count() -> int:
	var count := 0
	for voice: AudioStreamPlayer3D in _voices:
		if voice.playing:
			count += 1
	return count


func pool_metrics() -> Dictionary:
	var assigned_voices := 0
	for voice: AudioStreamPlayer3D in _voices:
		if voice.stream is AudioStreamWAV:
			assigned_voices += 1
	return {
		"voice_capacity": _voices.size(),
		"loaded_cues": _streams.size(),
		"load_failures": _load_failures.size(),
		"playback_requests": _playback_requests,
		"playback_successes": _playback_successes,
		"active_voices": active_voice_count(),
		"assigned_wav_voices": assigned_voices,
		"all_streams_wav": _streams.values().all(func(stream: Variant) -> bool:
			return stream is AudioStreamWAV),
	}


func pooled_stream_resource_ids() -> Array:
	var ids: Array[int] = []
	for stream: AudioStreamWAV in _streams.values():
		ids.append(stream.get_instance_id())
	ids.sort()
	return ids
