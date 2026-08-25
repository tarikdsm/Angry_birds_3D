extends SceneTree

## Proves that the trajectory presentation only ever shows samples the kernel
## already published, in both budgets, and that it never extrapolates.

const CONTROLLER := preload("res://scripts/game/gameplay_session_controller.gd")
const TRAJECTORY_RENDERER := preload("res://scripts/ui/trajectory_renderer.gd")
const GAMEPLAY_SCENE := "res://scenes/gameplay/gameplay_session.tscn"
const RENDERER_SOURCE := "res://scripts/ui/trajectory_renderer.gd"
## No presentation script may reintroduce an integrator. The kernel is the only
## place allowed to know about gravity, velocity or a timestep.
const FORBIDDEN_SOURCE_FRAGMENTS := [
	"gravity", "velocity", "9.81", "integrate", "accelerat", "get_physics_process_delta_time",
]
const MARKER := "TRAJECTORY_RENDERER_SMOKE_OK"

var _errors: Array[String] = []
var _renderer: Control
var _camera: Camera3D
var _gameplay: Node3D


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	_check_source_purity()
	await _check_reduction()
	if _errors.is_empty():
		await _check_published_preview()
	if not _errors.is_empty():
		_fail(" | ".join(_errors))
		return
	print(MARKER)
	_finish(0)


func _check_source_purity() -> void:
	var source := FileAccess.get_file_as_string(RENDERER_SOURCE)
	_check(not source.is_empty(), "the renderer source must be readable")
	for fragment: String in FORBIDDEN_SOURCE_FRAGMENTS:
		_check(not source.to_lower().contains(fragment),
			"the renderer must never model physics: %s" % fragment)


func _check_reduction() -> void:
	_renderer = TRAJECTORY_RENDERER.new()
	root.add_child(_renderer)
	await process_frame

	# One metre per tick keeps the distance budget binding before the time one.
	var metric := _linear_samples(300, 1.0)
	var standard: Array[Vector3] = TRAJECTORY_RENDERER.reduce_samples(metric, false)
	var assisted: Array[Vector3] = TRAJECTORY_RENDERER.reduce_samples(metric, true)
	_check(standard.size() == 7,
		"the standard budget must stop at 6 m: %d" % standard.size())
	_check(assisted.size() == 13,
		"the assisted budget must stop at 12 m: %d" % assisted.size())
	_check(_is_published_subsequence(standard, metric),
		"the standard budget must only show published samples")
	_check(_is_published_subsequence(assisted, metric),
		"the assisted budget must only show published samples")

	# Ten centimetres per tick keeps the time budget binding instead.
	var dense := _linear_samples(300, 0.1)
	standard = TRAJECTORY_RENDERER.reduce_samples(dense, false)
	assisted = TRAJECTORY_RENDERER.reduce_samples(dense, true)
	_check(standard.size() == TRAJECTORY_RENDERER.DEFAULT_MAXIMUM_POINTS,
		"the standard budget must publish 12 dots: %d" % standard.size())
	_check(assisted.size() == TRAJECTORY_RENDERER.ASSIST_MAXIMUM_POINTS,
		"the assisted budget must publish 24 dots: %d" % assisted.size())
	_check(standard.back() == dense[49],
		"the standard budget must stop at 0,825 s of published samples")
	_check(assisted.back() == dense[99],
		"the assisted budget must stop at 1,65 s of published samples")
	_check(_is_published_subsequence(standard, dense) \
			and _is_published_subsequence(assisted, dense),
		"neither budget may invent a sample")
	_check(TRAJECTORY_RENDERER.reduce_samples([], false).is_empty() \
			and TRAJECTORY_RENDERER.reduce_samples(null, true).is_empty(),
		"an absent preview must draw nothing")

	var diameter: float = _renderer.dot_diameter_px()
	_check(diameter >= 6.0 and diameter <= 12.0,
		"dots must stay between 6 and 12 px: %f" % diameter)
	_renderer.set_assist(true)
	diameter = _renderer.dot_diameter_px()
	_check(diameter >= 6.0 and diameter <= 12.0,
		"assisted dots must stay between 6 and 12 px: %f" % diameter)

	var preview_frame := {
		"trajectory_preview": {
			"samples": dense,
			"first_hit": {"entity_id": 42, "part_id": 1, "point": dense[99],
				"normal": Vector3.UP},
			"canonical_hash": 1234,
		},
	}
	_renderer.set_assist(false)
	_renderer.apply_frame(preview_frame)
	_check(_renderer.first_hit_point() == null,
		"the first impact must never appear in the standard budget")
	_check(_renderer.point_count() == TRAJECTORY_RENDERER.DEFAULT_MAXIMUM_POINTS,
		"the standard budget must reach the renderer")
	_renderer.set_assist(true)
	_renderer.apply_frame(preview_frame)
	_check(_renderer.first_hit_point() == dense[99],
		"the assisted budget must show the published first impact")
	_check(int(_renderer.published_hash()) == 1234,
		"the renderer must carry the canonical preview hash it was given")
	_renderer.apply_frame({"trajectory_preview": null})
	_check(_renderer.point_count() == 0 and _renderer.first_hit_point() == null,
		"a cleared preview must clear the renderer")

	_camera = Camera3D.new()
	root.add_child(_camera)
	_camera.global_transform = Transform3D(Basis.IDENTITY, Vector3(0.0, 0.0, 40.0))
	_renderer.set_camera(_camera)
	_renderer.apply_frame(preview_frame)
	var draws: int = int(_renderer.draw_count())
	_renderer.queue_redraw()
	await process_frame
	await process_frame
	_check(int(_renderer.draw_count()) > draws,
		"the renderer must repaint from the published frame")


func _check_published_preview() -> void:
	if not ClassDB.class_exists("GameplaySessionNode"):
		_check(false, "GameplaySessionNode must be registered before the preview probe")
		return
	var request_result: Dictionary = CONTROLLER.make_launch_request("earth", "farm_reaction")
	if not bool(request_result.get("ok", false)):
		_check(false, "the Earth campaign level must be registered")
		return
	var packed := load(GAMEPLAY_SCENE) as PackedScene
	_gameplay = packed.instantiate() as Node3D
	root.add_child(_gameplay)
	await process_frame
	if not _gameplay.configure_launch(request_result.request as Dictionary):
		_check(false, "the Earth launch must configure: %s" % _gameplay.last_error)
		return
	var session: Node = _gameplay.get_node("Session")
	await physics_frame
	if not session.queue_begin_grab(Vector3.RIGHT) or not session.queue_pull(-3.5, -0.6):
		_check(false, "the kernel must accept the probe gesture")
		return
	await physics_frame
	await physics_frame
	var frame := _gameplay.current_frame as Dictionary
	var preview: Variant = frame.get("trajectory_preview")
	if not preview is Dictionary:
		_check(false, "a grabbed launcher must publish a trajectory preview")
		return
	var published := (preview as Dictionary).get("samples", []) as Array
	_check(published.size() > 24,
		"the kernel preview must publish more samples than the presentation shows")
	var hud: CanvasLayer = _gameplay.hud()
	var renderer: Node = hud.trajectory_renderer()
	var shown: Array[Vector3] = renderer.points()
	_check(not shown.is_empty(), "the mounted HUD must render the published preview")
	_check(shown.size() <= TRAJECTORY_RENDERER.DEFAULT_MAXIMUM_POINTS,
		"the mounted HUD must respect the standard budget: %d" % shown.size())
	_check(_is_published_subsequence(shown, published),
		"every drawn dot must be a sample the kernel published")
	# Both budgets are replayed against the very same published frame, so the
	# comparison cannot be blurred by the world still settling between ticks.
	var captured := (preview as Dictionary).duplicate(true)
	var published_hash := int(captured.get("canonical_hash", 0))
	_gameplay.set_trajectory_assist(false)
	renderer.apply_frame({"trajectory_preview": captured})
	var standard_shown: Array[Vector3] = renderer.points()
	var standard_hash: int = int(renderer.published_hash())
	_gameplay.set_trajectory_assist(true)
	renderer.apply_frame({"trajectory_preview": captured})
	var assisted_shown: Array[Vector3] = renderer.points()
	_check(assisted_shown.size() > standard_shown.size() \
			and assisted_shown.size() <= TRAJECTORY_RENDERER.ASSIST_MAXIMUM_POINTS,
		"the assist must widen the budget: %d then %d" % [
			standard_shown.size(), assisted_shown.size()])
	_check(_is_published_subsequence(assisted_shown, published),
		"the assisted budget must stay inside the published samples")
	_check(int(renderer.published_hash()) == published_hash \
			and standard_hash == published_hash \
			and int((captured.get("samples", []) as Array).size()) == published.size(),
		"the assist must never change the canonical preview the kernel published")
	var tick_before := int((_gameplay.current_frame as Dictionary).get("tick", -1))
	_gameplay.set_trajectory_assist(false)
	await physics_frame
	_check(int((_gameplay.current_frame as Dictionary).get("tick", -1)) == tick_before + 1,
		"toggling the assist must never queue a kernel command")


func _linear_samples(count: int, step: float) -> Array:
	var samples: Array = []
	for index: int in count:
		samples.append(Vector3(float(index) * step, 0.0, 0.0))
	return samples


func _is_published_subsequence(shown: Array[Vector3], published: Array) -> bool:
	var cursor := 0
	for point: Vector3 in shown:
		var found := -1
		for index: int in range(cursor, published.size()):
			if published[index] is Vector3 and (published[index] as Vector3) == point:
				found = index
				break
		if found < 0:
			return false
		cursor = found + 1
	return true


func _check(condition: bool, message: String) -> void:
	if not condition:
		_errors.append(message)


func _fail(message: String) -> void:
	push_error("trajectory renderer smoke: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	for node: Node in [_gameplay, _renderer, _camera]:
		if is_instance_valid(node):
			if node.get_parent() == root:
				root.remove_child(node)
			node.free()
	quit(exit_code)
