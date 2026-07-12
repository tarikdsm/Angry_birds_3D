extends Node

const MATERIALS_PATH := "res://data/materials/vertical_slice.materials.json"
const ARCHETYPES_PATH := "res://data/archetypes/vertical_slice.archetypes.json"
const LEVEL_PATH := "res://data/levels/first_orbit.level.json"
const CAPTURE_ARGUMENT := "--vertical-slice-capture"
const CAPTURE_FRAME_LIMIT := 300
const CAPTURE_MARKER := "VERTICAL_SLICE_CAPTURE_COMPLETE frame=300"

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


func _physics_process(_delta: float) -> void:
	current_frame = session.consume_frame()
	consume_calls += 1
	launch_controller.observe_frame(current_frame)
	body_views.apply_frame(current_frame)
	orbital_camera.observe_frame(current_frame)
	hud.apply_frame(current_frame)
	feedback.apply_frame(current_frame)
	if _capture_enabled:
		_drive_capture()


func _drive_capture() -> void:
	if consume_calls == 3:
		launch_controller.begin_aim()
	elif consume_calls == 5:
		launch_controller.set_aim_degrees(-2.0, 0.0, 8.0)
	elif consume_calls == 55:
		launch_controller.launch_or_activate()
	for event: Dictionary in current_frame.get("events", []):
		if str(event.get("kind", "")) == "bird_launched":
			_capture_launch_tick = int(event.get("tick", current_frame.get("tick", 0)))
	if not _capture_ability_requested and _capture_launch_tick >= 0 \
			and int(current_frame.get("tick", 0)) >= _capture_launch_tick + 39:
		_capture_ability_requested = launch_controller.launch_or_activate()
	if consume_calls == CAPTURE_FRAME_LIMIT:
		print(CAPTURE_MARKER)
		get_tree().quit(0)


func _on_gameplay_fault(code: String, message: String) -> void:
	hud.show_fault(code, message)


func set_reduced_motion(enabled: bool) -> void:
	orbital_camera.set_reduced_motion(enabled)
	feedback.set_reduced_motion(enabled)
	hud.set_reduced_motion(enabled)


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("toggle_reduced_motion"):
		set_reduced_motion(not orbital_camera.reduced_motion)
