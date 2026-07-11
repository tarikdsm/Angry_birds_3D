extends Camera3D

const BASE_FOV := 48.0
const MIN_DISTANCE := 14.0
const MAX_DISTANCE := 24.0
const MIN_INCLINATION := 15.0
const MAX_INCLINATION := 70.0
const LOOK_AHEAD_METERS := 2.0
const PLANET_RADIUS := 10.0
const IMPACT_FOCUS_SECONDS := 0.6
const SAFE_MARGIN_RATIO := 0.08
const FORTIFICATION_CENTER := Vector3(0.0, 11.8, 0.4)
const DEFAULT_RING_POINT := Vector3(-13.0, 0.0, 0.0)
const FORTIFICATION_BOUNDS := [
	Vector3(-3.2, 10.0, -1.8), Vector3(3.2, 10.0, -1.8),
	Vector3(-3.2, 14.0, -1.8), Vector3(3.2, 14.0, -1.8),
	Vector3(-3.2, 10.0, 2.2), Vector3(3.2, 10.0, 2.2),
	Vector3(-3.2, 14.0, 2.2), Vector3(3.2, 14.0, 2.2),
]

@export var reduced_motion := false

var _yaw_degrees := -32.0
var _inclination_degrees := 32.0
var _distance := 18.0
var _focus := Vector3(0.0, 11.5, 0.4)
var _desired_focus := Vector3(0.0, 11.5, 0.4)
var _impact_focus := Vector3.ZERO
var _impact_focus_remaining := 0.0
var _fov_kick := 0.0
var _phase := "inspection"
var _ring_point := DEFAULT_RING_POINT
var _first_hit := Vector3.ZERO
var _has_first_hit := false
var _anchor_point := FORTIFICATION_CENTER
var _projectile_point := Vector3.ZERO
var _has_projectile := false
var _required_points: Array[Vector3] = [DEFAULT_RING_POINT, FORTIFICATION_CENTER]


func _ready() -> void:
	fov = BASE_FOV
	near = 0.08
	far = 160.0
	make_current()
	recenter()


func recenter() -> void:
	_yaw_degrees = -32.0
	_inclination_degrees = 32.0
	_distance = 18.0
	_desired_focus = Vector3(0.0, 11.5, 0.4)


func set_reduced_motion(enabled: bool) -> void:
	reduced_motion = enabled
	if enabled:
		_fov_kick = 0.0


func is_planet_occluded() -> bool:
	return _segment_intersects_planet(global_position, _focus)


func is_safe_framing() -> bool:
	if _required_points.is_empty():
		return true
	var viewport_size := get_viewport().get_visible_rect().size
	var margin := viewport_size * SAFE_MARGIN_RATIO
	for point: Vector3 in _required_points:
		if is_position_behind(point):
			return false
		var screen_point := unproject_position(point)
		if screen_point.x < margin.x or screen_point.y < margin.y \
				or screen_point.x > viewport_size.x - margin.x \
				or screen_point.y > viewport_size.y - margin.y:
			return false
	return true


func observe_frame(frame: Dictionary) -> void:
	_phase = str(frame.get("phase", "inspection"))
	_has_projectile = false
	for snapshot: Dictionary in frame.get("snapshots", []):
		if int(snapshot.get("surface_id", 0)) == 1003:
			var transform: Transform3D = snapshot.get("transform", Transform3D.IDENTITY)
			var velocity: Vector3 = snapshot.get("linear_velocity", Vector3.ZERO)
			_projectile_point = transform.origin
			if velocity.length_squared() > 0.0001:
				_projectile_point += velocity.normalized() * LOOK_AHEAD_METERS
			_has_projectile = true
		elif snapshot.has("enemy_archetype_id"):
			var anchor_transform: Transform3D = snapshot.get("transform", Transform3D.IDENTITY)
			_anchor_point = anchor_transform.origin
	var preview: Variant = frame.get("trajectory_preview")
	_has_first_hit = false
	if _phase == "aim" and preview is Dictionary:
		var preview_aim: Dictionary = preview.get("aim", {})
		_ring_point = preview_aim.get("origin", _ring_point)
		if preview.get("first_hit") is Dictionary:
			var hit: Dictionary = preview.first_hit
			_first_hit = hit.get("point", _anchor_point)
			_has_first_hit = true
	for event: Dictionary in frame.get("events", []):
		if str(event.get("kind", "")) == "damage_applied":
			_impact_focus = event.get("position", _desired_focus)
			_impact_focus_remaining = IMPACT_FOCUS_SECONDS
			if not reduced_motion:
				_fov_kick = 2.0
	_update_composition()


func _process(delta: float) -> void:
	if _impact_focus_remaining > 0.0:
		_impact_focus_remaining -= delta
		_desired_focus = _impact_focus
		_required_points = [_impact_focus]
	elif _phase in ["resolution", "evaluation", "result"]:
		_desired_focus = _anchor_point
		_required_points = [_anchor_point]
	var aspect := get_viewport().get_visible_rect().size.aspect()
	var aspect_distance := maxf(_distance, _distance_to_fit(_required_points, _desired_focus))
	aspect_distance += maxf(0.0, (1.7777778 - aspect) * 4.0)
	aspect_distance = clampf(aspect_distance, MIN_DISTANCE, MAX_DISTANCE)
	var yaw := deg_to_rad(_yaw_degrees)
	var inclination := deg_to_rad(_inclination_degrees)
	var horizontal := cos(inclination) * aspect_distance
	var offset := Vector3(sin(yaw) * horizontal, sin(inclination) * aspect_distance, cos(yaw) * horizontal)
	var desired_position := _desired_focus + offset
	if _segment_intersects_planet(desired_position, _desired_focus):
		desired_position = _safe_camera_position(
			desired_position, _desired_focus, aspect_distance)
	var weight := 1.0 - exp(-6.0 * delta)
	_focus = _focus.lerp(_desired_focus, weight)
	if _focus.length() < PLANET_RADIUS + 0.15:
		_focus = _focus.normalized() * (PLANET_RADIUS + 0.15)
	var next_position := global_position.lerp(desired_position, weight)
	global_position = _safe_camera_position(next_position, _focus, aspect_distance)
	look_at(_focus, Vector3.UP)
	if _phase != "flight_ability" and _fortification_bounds_occluded(global_position):
		var fortification_position := _safe_camera_position(
			global_position, _anchor_point, aspect_distance)
		global_position = _safe_camera_position(
			fortification_position, _focus, aspect_distance)
		look_at(_focus, Vector3.UP)
	if not is_safe_framing():
		_focus = _outside_planet_focus(_desired_focus)
		global_position = _safe_camera_position(
			_camera_position(_focus, MAX_DISTANCE), _focus, MAX_DISTANCE)
		look_at(_focus, Vector3.UP)
	_fov_kick = move_toward(_fov_kick, 0.0, delta * 5.0)
	fov = BASE_FOV + (0.0 if reduced_motion else clampf(_fov_kick, 0.0, 2.0))


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion and (event.button_mask & MOUSE_BUTTON_MASK_RIGHT) != 0:
		_yaw_degrees = fmod(_yaw_degrees - event.relative.x * 0.18, 360.0)
		_inclination_degrees = clampf(
			_inclination_degrees - event.relative.y * 0.14,
			MIN_INCLINATION,
			MAX_INCLINATION)
	if event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_distance = clampf(_distance - 1.0, MIN_DISTANCE, MAX_DISTANCE)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_distance = clampf(_distance + 1.0, MIN_DISTANCE, MAX_DISTANCE)


func _segment_intersects_planet(origin: Vector3, target: Vector3) -> bool:
	var segment := target - origin
	var length_squared := segment.length_squared()
	if length_squared <= 0.0001:
		return false
	var closest_t := clampf(-origin.dot(segment) / length_squared, 0.0, 1.0)
	return (origin + segment * closest_t).length_squared() < PLANET_RADIUS * PLANET_RADIUS


func _safe_camera_position(candidate: Vector3, focus: Vector3, distance: float) -> Vector3:
	if not _segment_intersects_planet(candidate, focus):
		return candidate
	var radial := focus.normalized()
	if radial.is_zero_approx():
		radial = Vector3.UP
	var candidate_offset := candidate - focus
	var tangent := candidate_offset - radial * candidate_offset.dot(radial)
	if tangent.is_zero_approx():
		tangent = radial.cross(Vector3.UP)
	if tangent.is_zero_approx():
		tangent = radial.cross(Vector3.RIGHT)
	return focus + radial * maxf(3.0, distance * 0.55) \
		+ tangent.normalized() * distance * 0.85


func _update_composition() -> void:
	match _phase:
		"inspection":
			_required_points = [_ring_point, _anchor_point]
		"aim":
			_required_points = [_ring_point, _anchor_point]
			if _has_first_hit:
				_required_points.append(_first_hit)
		"flight_ability":
			if _has_projectile:
				_required_points = [_projectile_point]
				if _projectile_point.distance_to(_anchor_point) <= 16.0:
					_required_points.append(_anchor_point)
			else:
				_required_points = [_anchor_point]
		_:
			_required_points = [_anchor_point]
	_desired_focus = _outside_planet_focus(_average_point(_required_points))


func _average_point(points: Array[Vector3]) -> Vector3:
	if points.is_empty():
		return FORTIFICATION_CENTER
	var total := Vector3.ZERO
	for point: Vector3 in points:
		total += point
	return total / float(points.size())


func _distance_to_fit(points: Array[Vector3], focus: Vector3) -> float:
	var radius := 0.0
	for point: Vector3 in points:
		radius = maxf(radius, point.distance_to(focus))
	if radius <= 0.01:
		return MIN_DISTANCE
	var usable_half_fov := deg_to_rad(BASE_FOV * 0.5) * (1.0 - SAFE_MARGIN_RATIO * 2.0)
	return clampf(radius / tan(usable_half_fov) + 2.0, MIN_DISTANCE, MAX_DISTANCE)


func _camera_position(focus: Vector3, distance: float) -> Vector3:
	var yaw := deg_to_rad(_yaw_degrees)
	var inclination := deg_to_rad(_inclination_degrees)
	var horizontal := cos(inclination) * distance
	return focus + Vector3(
		sin(yaw) * horizontal,
		sin(inclination) * distance,
		cos(yaw) * horizontal)


func _fortification_bounds_occluded(camera_position: Vector3) -> bool:
	var hidden_corners := 0
	for point: Vector3 in FORTIFICATION_BOUNDS:
		if _segment_intersects_planet(camera_position, point):
			hidden_corners += 1
	return hidden_corners > FORTIFICATION_BOUNDS.size() / 2


func _outside_planet_focus(value: Vector3) -> Vector3:
	if value.length() >= PLANET_RADIUS + 0.15:
		return value
	if value.is_zero_approx():
		return Vector3.UP * (PLANET_RADIUS + 0.15)
	return value.normalized() * (PLANET_RADIUS + 0.15)
