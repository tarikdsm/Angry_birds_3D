extends Node

signal recenter_requested
signal pause_changed(paused: bool)

const LAUNCH_RADIUS := 13.0
const MIN_THETA_DEGREES := -50.0
const MAX_THETA_DEGREES := 50.0
const MIN_SPEED := 8.0
const MAX_SPEED := 16.0
const RESTART_HOLD_SECONDS := 0.5
const RING_PICK_RADIUS := 1.5

@onready var _camera: Camera3D = get_node("../OrbitalCamera")
@onready var _ring: Node3D = get_node("../ImpulseRingPlaceholder")

var _session: Node
var _frame: Dictionary = {}
var _theta_degrees := 0.0
var _phase_degrees := 0.0
var _speed := 10.5
var _restart_hold := 0.0
var _restart_latched := false


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS


func bind_session(session: Node) -> void:
	_session = session


func observe_frame(frame: Dictionary) -> void:
	_frame = frame


func begin_aim() -> bool:
	return is_instance_valid(_session) and _session.queue_begin_aim()


func can_begin_aim_at(screen_position: Vector2) -> bool:
	if _camera.is_position_behind(_ring.global_position):
		return false
	var ray_origin := _camera.project_ray_origin(screen_position)
	var ray_direction := _camera.project_ray_normal(screen_position)
	var along_ray := (_ring.global_position - ray_origin).dot(ray_direction)
	if along_ray <= 0.0:
		return false
	var nearest_point := ray_origin + ray_direction * along_ray
	return nearest_point.distance_to(_ring.global_position) <= RING_PICK_RADIUS


func try_begin_aim_at(screen_position: Vector2) -> bool:
	return can_begin_aim_at(screen_position) and begin_aim()


func set_aim_degrees(theta_degrees: float, phase_degrees: float, speed: float) -> bool:
	if not is_instance_valid(_session):
		return false
	_theta_degrees = clampf(theta_degrees, MIN_THETA_DEGREES, MAX_THETA_DEGREES)
	_phase_degrees = clampf(phase_degrees, -80.0, 80.0)
	_speed = clampf(speed, MIN_SPEED, MAX_SPEED)
	var theta := deg_to_rad(_theta_degrees)
	var phase := deg_to_rad(_phase_degrees)
	var origin := Vector3(-LAUNCH_RADIUS * cos(theta), 0.0, LAUNCH_RADIUS * sin(theta))
	var azimuth := Vector3(sin(theta), 0.0, cos(theta))
	var tangent := (Vector3.UP * cos(phase) + azimuth * sin(phase)).normalized()
	_ring.global_position = origin
	return _session.queue_aim(origin, tangent, _speed)


func solve_aim_from_screen(screen_position: Vector2) -> Dictionary:
	var ray_origin := _camera.project_ray_origin(screen_position)
	var ray_direction := _camera.project_ray_normal(screen_position).normalized()
	var projection := ray_origin.dot(ray_direction)
	var discriminant := projection * projection \
		- (ray_origin.length_squared() - LAUNCH_RADIUS * LAUNCH_RADIUS)
	if discriminant < 0.0:
		return {}
	var root_distance := sqrt(discriminant)
	var ray_distance := -projection - root_distance
	if ray_distance <= 0.0:
		ray_distance = -projection + root_distance
	if ray_distance <= 0.0:
		return {}
	var shell_hit := ray_origin + ray_direction * ray_distance
	var equatorial := Vector3(shell_hit.x, 0.0, shell_hit.z)
	if equatorial.is_zero_approx():
		return {}
	var theta_degrees := clampf(
		rad_to_deg(atan2(equatorial.z, -equatorial.x)),
		MIN_THETA_DEGREES,
		MAX_THETA_DEGREES)
	var theta := deg_to_rad(theta_degrees)
	var origin := Vector3(-LAUNCH_RADIUS * cos(theta), 0.0, LAUNCH_RADIUS * sin(theta))
	var radial := origin.normalized()
	var north := (Vector3.UP - radial * radial.dot(Vector3.UP)).normalized()
	var azimuth := north.cross(radial).normalized()
	var phase := deg_to_rad(_phase_degrees)
	var tangent := (north * cos(phase) + azimuth * sin(phase)).normalized()
	return {
		"origin": origin,
		"tangent": tangent,
		"theta_degrees": theta_degrees,
		"speed": _speed,
	}


func author_aim_from_screen(screen_position: Vector2) -> bool:
	var solved := solve_aim_from_screen(screen_position)
	if solved.is_empty() or not is_instance_valid(_session):
		return false
	_theta_degrees = float(solved.theta_degrees)
	_ring.global_position = solved.origin
	return _session.queue_aim(solved.origin, solved.tangent, solved.speed)


func launch_or_activate() -> bool:
	if not is_instance_valid(_session):
		return false
	var phase := str(_frame.get("phase", ""))
	if phase == "aim":
		return _session.queue_launch()
	if phase == "flight_ability":
		return _session.queue_activate_ability()
	return false


func restart_now() -> bool:
	return is_instance_valid(_session) and _session.restart_level()


func cancel_aim_or_toggle_pause() -> void:
	if str(_frame.get("phase", "")) == "aim":
		_session.queue_cancel_aim()
		return
	get_tree().paused = not get_tree().paused
	pause_changed.emit(get_tree().paused)


func _process(delta: float) -> void:
	if Input.is_action_just_pressed("cancel_or_pause"):
		cancel_aim_or_toggle_pause()
	if Input.is_action_just_pressed("recenter_camera"):
		recenter_requested.emit()
	if Input.is_action_pressed("restart_level"):
		_restart_hold += delta
		if _restart_hold >= RESTART_HOLD_SECONDS and not _restart_latched:
			_restart_latched = true
			restart_now()
	else:
		_restart_hold = 0.0
		_restart_latched = false
	if get_tree().paused:
		return
	if Input.is_action_just_pressed("launch_or_ability"):
		launch_or_activate()
	if str(_frame.get("phase", "")) != "aim":
		return
	if Input.is_action_pressed("aim_left"):
		set_aim_degrees(_theta_degrees - 24.0 * delta, _phase_degrees, _speed)
	if Input.is_action_pressed("aim_right"):
		set_aim_degrees(_theta_degrees + 24.0 * delta, _phase_degrees, _speed)
	if Input.is_action_pressed("aim_power_down"):
		set_aim_degrees(_theta_degrees, _phase_degrees, _speed - 4.0 * delta)
	if Input.is_action_pressed("aim_power_up"):
		set_aim_degrees(_theta_degrees, _phase_degrees, _speed + 4.0 * delta)


func _unhandled_input(event: InputEvent) -> void:
	if get_tree().paused:
		return
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT and event.pressed \
				and str(_frame.get("phase", "")) == "inspection":
			try_begin_aim_at(event.position)
	if event is InputEventMouseMotion and event.alt_pressed \
			and (event.button_mask & MOUSE_BUTTON_MASK_LEFT) != 0 \
			and str(_frame.get("phase", "")) == "aim":
		_phase_degrees = clampf(_phase_degrees - event.relative.y * 0.12, -80.0, 80.0)
		author_aim_from_screen(event.position)
