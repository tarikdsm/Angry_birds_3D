extends Control

## Draws the trajectory preview published by the kernel.
##
## The renderer never simulates a trajectory and never invents a point between
## two published ones: it only selects a subset of the samples the kernel
## already published and projects them with the active camera. Every drawn point is byte-identical to
## a published sample, which is what game/tests/trajectory_renderer_smoke.gd
## asserts. The assist mode changes nothing but how many published samples are
## shown, so it can never change score, trajectory or stars.

const THEME_FACTORY := preload("res://scripts/ui/theme_factory.gd")

const DEFAULT_MAXIMUM_POINTS := 12
const ASSIST_MAXIMUM_POINTS := 24
const DEFAULT_MAXIMUM_SECONDS := 0.825
const ASSIST_MAXIMUM_SECONDS := 1.65
const DEFAULT_MAXIMUM_METERS := 6.0
const ASSIST_MAXIMUM_METERS := 12.0
## The kernel publishes one preview sample per fixed simulation tick. The value
## mirrors SessionFixedStepAccumulator::time_step and is only used to convert a
## sample index into the presentation time budget of the specification.
const SAMPLE_INTERVAL_SECONDS := 1.0 / 60.0
const DOT_DIAMETER_PX := 8.0
const ASSIST_DOT_DIAMETER_PX := 10.0
const IMPACT_DIAMETER_PX := 18.0
const SHADOW_OFFSET_PX := 3.0

var _camera: Camera3D
var _assist := false
var _points: Array[Vector3] = []
var _first_hit_point: Variant = null
var _published_hash := 0
var _draw_count := 0


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)


func set_camera(camera: Camera3D) -> void:
	_camera = camera
	queue_redraw()


func set_assist(enabled: bool) -> void:
	if _assist == enabled:
		return
	_assist = enabled
	queue_redraw()


func assist_enabled() -> bool:
	return _assist


func clear() -> void:
	_points = []
	_first_hit_point = null
	_published_hash = 0
	queue_redraw()


## Consumes one published frame. Nothing else may feed this renderer.
func apply_frame(frame: Dictionary) -> void:
	var preview: Variant = frame.get("trajectory_preview")
	if not preview is Dictionary:
		clear()
		return
	var document := preview as Dictionary
	_points = reduce_samples(document.get("samples", []), _assist)
	_published_hash = int(document.get("canonical_hash", 0))
	_first_hit_point = null
	if _assist:
		var hit: Variant = document.get("first_hit")
		if hit is Dictionary and (hit as Dictionary).get("point") is Vector3:
			_first_hit_point = (hit as Dictionary).point
	queue_redraw()


## Reduces published samples to the presentation budget. The result is always a
## subsequence of the input: no sample is created, moved or averaged.
static func reduce_samples(samples: Variant, assist: bool) -> Array[Vector3]:
	var reduced: Array[Vector3] = []
	if not samples is Array:
		return reduced
	var published := samples as Array
	if published.is_empty():
		return reduced
	var maximum_points := ASSIST_MAXIMUM_POINTS if assist else DEFAULT_MAXIMUM_POINTS
	var maximum_seconds := ASSIST_MAXIMUM_SECONDS if assist else DEFAULT_MAXIMUM_SECONDS
	var maximum_meters := ASSIST_MAXIMUM_METERS if assist else DEFAULT_MAXIMUM_METERS
	var last_index := 0
	var travelled := 0.0
	for index: int in range(1, published.size()):
		if not published[index] is Vector3 or not published[index - 1] is Vector3:
			break
		if float(index) * SAMPLE_INTERVAL_SECONDS > maximum_seconds:
			break
		var step := (published[index] as Vector3).distance_to(published[index - 1] as Vector3)
		if travelled + step > maximum_meters:
			break
		travelled += step
		last_index = index
	if last_index + 1 <= maximum_points:
		for index: int in range(last_index + 1):
			reduced.append(published[index] as Vector3)
		return reduced
	for slot: int in maximum_points:
		var index := int(round(
			float(slot) * float(last_index) / float(maximum_points - 1)))
		reduced.append(published[index] as Vector3)
	return reduced


func points() -> Array[Vector3]:
	return _points.duplicate()


func point_count() -> int:
	return _points.size()


func first_hit_point() -> Variant:
	return _first_hit_point


func published_hash() -> int:
	return _published_hash


func dot_diameter_px() -> float:
	return ASSIST_DOT_DIAMETER_PX if _assist else DOT_DIAMETER_PX


func draw_count() -> int:
	return _draw_count


func _draw() -> void:
	_draw_count += 1
	if _camera == null or _points.is_empty():
		return
	var radius := dot_diameter_px() * 0.5
	for index: int in _points.size():
		var sample := _points[index]
		if _camera.is_position_behind(sample):
			continue
		var screen := _camera.unproject_position(sample)
		if _assist:
			draw_circle(screen + Vector2(0.0, SHADOW_OFFSET_PX), radius,
				Color(THEME_FACTORY.TRAJECTORY_SHADOW, 0.55))
		var fade := 1.0 - 0.5 * float(index) / float(maxi(1, _points.size() - 1))
		draw_circle(screen, radius, Color(THEME_FACTORY.TRAJECTORY_DOT, fade))
	if _first_hit_point is Vector3 and not _camera.is_position_behind(_first_hit_point):
		draw_circle(_camera.unproject_position(_first_hit_point as Vector3),
			IMPACT_DIAMETER_PX * 0.5, THEME_FACTORY.TRAJECTORY_IMPACT, false, 2.0)
