extends Node

const MATERIALS_PATH := "res://data/materials/vertical_slice.materials.json"
const ARCHETYPES_PATH := "res://data/archetypes/vertical_slice.archetypes.json"
const LEVEL_PATH := "res://data/levels/first_orbit.level.json"
const CAPTURE_ARGUMENT := "--vertical-slice-capture"
const CAPTURE_FRAME_LIMIT := 300
const CAPTURE_MARKER := "VERTICAL_SLICE_CAPTURE_COMPLETE frame=300"
const SOURCE_CAPTURE_MARKER := "VERTICAL_SLICE_SOURCE_CAPTURE_COMPLETE"

@onready var session: Node = get_node("../OrbitalSession")
@onready var launch_controller: Node = get_node("../LaunchController")
@onready var body_views: Node = get_node("../BodyViews")
@onready var orbital_camera: Node = get_node("../OrbitalCamera")
@onready var hud: Node = get_node("../VerticalSliceHUD")
@onready var feedback: Node = get_node("../FeedbackDirector")

var current_frame: Dictionary = {}
var consume_calls := 0
var session_configured := false
var _capture_enabled := false
var _capture_launch_tick := -1
var _capture_ability_requested := false
var _capture_shot_count := 0
var _capture_next_shot_stage := 0
var _capture_render_frames := 0
var _capture_last_phase := ""
var _capture_feedback_shutdown := false
var _performance_rupture_seen := false
var _performance_vfx_seen := false
var _performance_event_render_frame := -1
var _capture_require_terminal := false
var _capture_result_render_frame := -1
var _capture_shutdown_render_frame := -1
var _metrics_path := ""
var _frame_times_ms: Array[float] = []
var _step_times_ms: Array[float] = []
var _input_started_usec := -1
var _input_feedback_ms: Array[float] = []


func _ready() -> void:
	process_priority = 0
	launch_controller.bind_session(session)
	launch_controller.recenter_requested.connect(orbital_camera.recenter)
	session.gameplay_fault.connect(_on_gameplay_fault)
	session_configured = session.configure_session(
		FileAccess.get_file_as_string(MATERIALS_PATH),
		FileAccess.get_file_as_string(ARCHETYPES_PATH),
		FileAccess.get_file_as_string(LEVEL_PATH))
	_capture_enabled = OS.get_cmdline_user_args().has(CAPTURE_ARGUMENT)
	for argument: String in OS.get_cmdline_user_args():
		if argument.begins_with("--vertical-slice-metrics="):
			_metrics_path = argument.trim_prefix("--vertical-slice-metrics=")
			if _metrics_path.begins_with("res://") or _metrics_path.begins_with("user://"):
				_metrics_path = ProjectSettings.globalize_path(_metrics_path)
		elif argument == "--ui-scale=150":
			get_window().content_scale_factor = 1.5
		elif argument == "--capture-normal-terminal":
			_capture_require_terminal = true


func _physics_process(_delta: float) -> void:
	current_frame = session.consume_frame()
	consume_calls += 1
	launch_controller.observe_frame(current_frame)
	body_views.apply_frame(current_frame)
	orbital_camera.observe_frame(current_frame)
	hud.apply_frame(current_frame)
	if not _capture_feedback_shutdown:
		feedback.apply_frame(current_frame)
	if _capture_enabled:
		_log_capture_state()
		_observe_performance_events()
	if current_frame.has("metrics"):
		_step_times_ms.append(float(current_frame.metrics.get("step_ms", 0.0)))
	if _input_started_usec >= 0 and str(current_frame.get("phase", "")) == "aim":
		_input_feedback_ms.append(float(Time.get_ticks_usec() - _input_started_usec) / 1000.0)
		_input_started_usec = -1
	if _capture_enabled:
		_drive_capture()


func _process(delta: float) -> void:
	if not _capture_enabled:
		return
	_capture_render_frames += 1
	if _capture_render_frames > 20:
		_frame_times_ms.append(delta * 1000.0)
	var movie_capture := _metrics_path.is_empty()
	var movie_completion_frame := CAPTURE_FRAME_LIMIT
	if movie_capture and _capture_require_terminal:
		movie_completion_frame = _capture_result_render_frame + 35 \
			if _capture_result_render_frame >= 0 else -1
	if movie_capture and movie_completion_frame >= 0 \
			and _capture_render_frames == movie_completion_frame - 5:
		_shutdown_capture_feedback()
	var performance_complete := not movie_capture \
		and _capture_render_frames >= CAPTURE_FRAME_LIMIT \
		and _performance_rupture_seen and _performance_vfx_seen \
		and _performance_event_render_frame >= 0 \
		and _capture_render_frames - _performance_event_render_frame >= 30 \
		and _input_feedback_ms.size() >= 3
	if not movie_capture and performance_complete and _capture_shutdown_render_frame < 0:
		_shutdown_capture_feedback()
		_capture_shutdown_render_frame = _capture_render_frames
	elif not movie_capture and _capture_shutdown_render_frame >= 0 \
			and _capture_render_frames >= _capture_shutdown_render_frame + 5:
		_write_metrics()
		print(CAPTURE_MARKER)
		get_tree().quit(0)
	elif movie_capture and movie_completion_frame >= 0 \
			and _capture_render_frames == movie_completion_frame:
		if movie_capture and _capture_require_terminal:
			print("%s frames=%d" % [SOURCE_CAPTURE_MARKER, _capture_render_frames])
		else:
			print(CAPTURE_MARKER)
		get_tree().quit(0)
	elif not movie_capture and _capture_shutdown_render_frame < 0 \
			and _capture_render_frames >= 6000:
		push_error("Performance capture did not observe rupture/VFX/input samples")
		get_tree().quit(1)


func _shutdown_capture_feedback() -> void:
	feedback.shutdown_feedback()
	_capture_feedback_shutdown = true
	var legacy_audio := get_node_or_null("../VirelaAudioFX") as AudioStreamPlayer3D
	if legacy_audio != null:
		legacy_audio.stop()
		legacy_audio.stream = null


func _drive_capture() -> void:
	if consume_calls == 3:
		_input_started_usec = Time.get_ticks_usec()
		launch_controller.begin_aim()
	elif consume_calls == 5:
		_input_started_usec = Time.get_ticks_usec()
		launch_controller.set_aim_degrees(-2.0, 0.0, 8.0)
	elif consume_calls == 10:
		_input_started_usec = Time.get_ticks_usec()
		launch_controller.set_aim_degrees(-2.0, 0.0, 8.0)
	elif consume_calls == 55:
		launch_controller.launch_or_activate()
	for event: Dictionary in current_frame.get("events", []):
		if str(event.get("kind", "")) == "bird_launched":
			_capture_shot_count += 1
			_capture_launch_tick = int(event.get("tick", current_frame.get("tick", 0)))
			_capture_ability_requested = false
	if not _capture_ability_requested and _capture_launch_tick >= 0 \
			and str(current_frame.get("phase", "")) == "flight_ability" \
			and int(current_frame.get("tick", 0)) >= _capture_launch_tick + 39:
		_capture_ability_requested = launch_controller.launch_or_activate()
	if _capture_shot_count > 0 and _capture_shot_count < 3:
		var phase := str(current_frame.get("phase", ""))
		if phase == "inspection" and _capture_next_shot_stage == 0:
			_input_started_usec = Time.get_ticks_usec()
			if launch_controller.begin_aim():
				_capture_next_shot_stage = 1
		elif phase == "aim" and _capture_next_shot_stage == 1:
			if launch_controller.set_aim_degrees(0.0, 0.0, 8.0):
				_capture_next_shot_stage = 2
		elif phase == "aim" and _capture_next_shot_stage == 2:
			if launch_controller.launch_or_activate():
				_capture_next_shot_stage = 0


func _log_capture_state() -> void:
	var phase := str(current_frame.get("phase", ""))
	if phase != _capture_last_phase:
		_capture_last_phase = phase
		if phase == "result" and str(current_frame.get("outcome", "none")) == "victory" \
				and _capture_result_render_frame < 0:
			_capture_result_render_frame = _capture_render_frames
		print("NINHO_CAPTURE_STATE frame=%d tick=%d phase=%s outcome=%s camera=%s exposure=1.0" % [
			_capture_render_frames,
			int(current_frame.get("tick", 0)),
			phase,
			str(current_frame.get("outcome", "none")),
			str(orbital_camera.global_position),
		])
	for event: Dictionary in current_frame.get("events", []):
		var profile := str(feedback.profile_for_event(event, current_frame.get("snapshots", [])))
		if profile not in ["helmet", "vulnerable"]:
			continue
		print("NINHO_CAPTURE_EVENT frame=%d tick=%d kind=%s profile=%s affected=%d damage=%.6f camera=%s exposure=1.0" % [
			_capture_render_frames,
			int(event.get("tick", current_frame.get("tick", 0))),
			str(event.get("kind", "")),
			profile,
			int(event.get("affected_entity_id", 0)),
			float(event.get("damage", 0.0)),
			str(orbital_camera.global_position),
		])


func _observe_performance_events() -> void:
	if _metrics_path.is_empty():
		return
	for event: Dictionary in current_frame.get("events", []):
		var kind := str(event.get("kind", ""))
		if kind in ["joint_broken", "piece_fracture_triggered", "piece_fractured"]:
			_performance_rupture_seen = true
		if not str(feedback.profile_for_event(event, current_frame.get("snapshots", []))).is_empty():
			_performance_vfx_seen = true
	if _performance_rupture_seen and _performance_vfx_seen \
			and _performance_event_render_frame < 0:
		_performance_event_render_frame = _capture_render_frames


func _percentile(values: Array[float], fraction: float) -> float:
	if values.is_empty():
		return 0.0
	var sorted := values.duplicate()
	sorted.sort()
	var index := ceili(fraction * float(sorted.size())) - 1
	return sorted[clampi(index, 0, sorted.size() - 1)]


func _write_metrics() -> void:
	var output := {
		"schema": "ninho.vertical-slice.runtime-metrics.v1",
		"frames_measured": _frame_times_ms.size(),
		"frame_p95_ms": _percentile(_frame_times_ms, 0.95),
		"frame_p99_ms": _percentile(_frame_times_ms, 0.99),
		"max_hitch_ms": _percentile(_frame_times_ms, 1.0),
		"input_feedback_p95_ms": _percentile(_input_feedback_ms, 0.95),
		"input_feedback_samples": _input_feedback_ms.size(),
		"physics_step_p95_ms": _percentile(_step_times_ms, 0.95),
		"rupture_observed": _performance_rupture_seen,
		"vfx_observed": _performance_vfx_seen,
		"post_vfx_frames": maxi(0, _capture_render_frames - _performance_event_render_frame),
	}
	var file := FileAccess.open(_metrics_path, FileAccess.WRITE)
	if file == null:
		push_error("Unable to write vertical slice metrics: %s" % _metrics_path)
		return
	file.store_string(JSON.stringify(output, "  ") + "\n")


func _on_gameplay_fault(code: String, message: String) -> void:
	hud.show_fault(code, message)


func set_reduced_motion(enabled: bool) -> void:
	orbital_camera.set_reduced_motion(enabled)
	feedback.set_reduced_motion(enabled)
	hud.set_reduced_motion(enabled)


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("toggle_reduced_motion"):
		set_reduced_motion(not orbital_camera.reduced_motion)
