extends Node

const CAPTURE_ARGUMENT := "--product-v2-capture"
const FRAMES_PREFIX := "--product-v2-capture-frames="
const METRICS_PREFIX := "--product-v2-metrics="
const ARGUMENT_NAMESPACE := "--product-v2"
const STATE_MARKER := "PRODUCT_V2_CAPTURE_STATE"
const COMPLETION_MARKER := "PRODUCT_V2_CAPTURE_COMPLETE"
const DEFAULT_FRAME_LIMIT := 300
const MAXIMUM_FRAME_LIMIT := 100000

signal capture_completed(rendered_frames: int)

var capture_enabled := false
var frame_limit := DEFAULT_FRAME_LIMIT
var metrics_path := ""
var rendered_frames := 0
var configuration_error := ""

var _last_phase := ""
var _observed_frames := 0


static func parse_arguments(arguments: Array) -> Dictionary:
	var result := {
		"enabled": false,
		"frame_limit": DEFAULT_FRAME_LIMIT,
		"metrics_path": "",
		"error": "",
	}
	var enabled := false
	for raw: Variant in arguments:
		var argument := str(raw)
		if not argument.begins_with(ARGUMENT_NAMESPACE):
			continue
		if argument == CAPTURE_ARGUMENT:
			enabled = true
		elif argument.begins_with(FRAMES_PREFIX):
			var budget := argument.trim_prefix(FRAMES_PREFIX)
			if not budget.is_valid_int():
				result.error = "capture frame budget must be an integer: %s" % argument
				return result
			var frames := budget.to_int()
			if frames <= 0 or frames > MAXIMUM_FRAME_LIMIT:
				result.error = "capture frame budget is out of range: %s" % argument
				return result
			result.frame_limit = frames
		elif argument.begins_with(METRICS_PREFIX):
			var path := argument.trim_prefix(METRICS_PREFIX)
			if path.is_empty() or path.contains(".."):
				result.error = "capture metrics path is not canonical: %s" % argument
				return result
			result.metrics_path = path
		else:
			result.error = "unknown product capture argument: %s" % argument
			return result
	result.enabled = enabled
	return result


func _ready() -> void:
	var parsed := parse_arguments(OS.get_cmdline_user_args())
	configuration_error = str(parsed.error)
	if not configuration_error.is_empty():
		capture_enabled = false
		set_process(false)
		return
	capture_enabled = bool(parsed.enabled)
	frame_limit = int(parsed.frame_limit)
	metrics_path = str(parsed.metrics_path)
	set_process(capture_enabled)


func observe_frame(frame: Dictionary) -> void:
	if not capture_enabled:
		return
	_observed_frames += 1
	var phase := str(frame.get("phase", ""))
	if phase == _last_phase:
		return
	_last_phase = phase
	print("%s frame=%d tick=%d phase=%s outcome=%s" % [
		STATE_MARKER,
		rendered_frames,
		int(frame.get("tick", 0)),
		phase,
		str(frame.get("outcome", "none")),
	])


func observed_frames() -> int:
	return _observed_frames


func _process(_delta: float) -> void:
	if not capture_enabled:
		return
	rendered_frames += 1
	if rendered_frames < frame_limit:
		return
	capture_enabled = false
	set_process(false)
	print("%s frames=%d" % [COMPLETION_MARKER, rendered_frames])
	capture_completed.emit(rendered_frames)
	if not metrics_path.is_empty():
		_write_metrics()
	get_tree().quit(0)


func _write_metrics() -> void:
	var globalized := metrics_path
	if globalized.begins_with("res://") or globalized.begins_with("user://"):
		globalized = ProjectSettings.globalize_path(globalized)
	var file := FileAccess.open(globalized, FileAccess.WRITE)
	if file == null:
		return
	file.store_string(JSON.stringify({
		"schema": "ninho.product-v2.capture-metrics.v1",
		"rendered_frames": rendered_frames,
		"observed_frames": _observed_frames,
	}, "  ") + "\n")
