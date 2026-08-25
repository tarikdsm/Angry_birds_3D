extends Camera3D

## Carousel product rig.
##
## Presentation only. It frames a diorama with the authored 35 degree profile
## and moves between dioramas with the authored transition; reduced motion
## replaces the move with a 0.1 s cut.

const IDLE_YAW_DEGREES_PER_SECOND := 5.0

var _profile: Dictionary = {}
var _bounds: Dictionary = {}
var _anchor := Vector3.ZERO
var _up := Vector3.UP
var _base_offset := Vector3.BACK
var _configured := false
var _locked := false
var _reduced_motion := false
var _shake_enabled := true
var _yaw_degrees := 0.0
var _inclination_degrees := 0.0
var _distance := 0.0
var _default_inclination_degrees := 0.0
var _default_distance := 0.0
var _focus := Vector3.ZERO
var _desired_focus := Vector3.ZERO
var _transition_remaining := 0.0


func configure(profile: Dictionary, bounds: Dictionary, anchor: Vector3) -> bool:
	if profile.is_empty() or bounds.is_empty():
		return false
	_profile = profile.duplicate(true)
	_bounds = bounds.duplicate(true)
	_anchor = anchor
	_up = Vector3.UP
	_base_offset = Vector3.BACK
	var inclinations := _profile.inclination_range_deg as Array
	_default_inclination_degrees = float(inclinations[0]) \
		+ (float(inclinations[1]) - float(inclinations[0])) * 0.5
	var distances := _profile.distance_range_m as Array
	_default_distance = float(distances[0]) \
		+ (float(distances[1]) - float(distances[0])) * 0.5
	fov = float(_profile.fov_degrees)
	near = 0.05
	far = 200.0
	_configured = true
	_locked = false
	recenter()
	return true


func release() -> void:
	_configured = false
	_locked = false
	_profile = {}
	_bounds = {}


func rig_kind() -> StringName:
	return &"carousel"


func profile_id() -> String:
	return str(_profile.get("id", ""))


func composition_bounds() -> Dictionary:
	return _bounds.duplicate(true)


func safe_margin_ratio() -> float:
	return float(_profile.get("safe_margin_ratio", 0.08))


func transition_seconds() -> float:
	if _reduced_motion:
		return float(_profile.get("reduced_motion_transition_seconds", 0.1))
	return float(_profile.get("transition_seconds", 0.45))


func pending_kick_degrees() -> float:
	return 0.0


func orbit_degrees() -> Vector2:
	return Vector2(_yaw_degrees, _inclination_degrees)


func zoom_distance() -> float:
	return _distance


func set_locked(value: bool) -> void:
	_locked = value


func is_locked() -> bool:
	return _locked


func set_reduced_motion(value: bool) -> void:
	_reduced_motion = value
	if value:
		_transition_remaining = 0.0
		_focus = _desired_focus


func reduced_motion() -> bool:
	return _reduced_motion


func set_shake_enabled(value: bool) -> void:
	_shake_enabled = value


func shake_enabled() -> bool:
	return _shake_enabled


func is_occluded() -> bool:
	return false


func is_safe_framing() -> bool:
	if not _configured:
		return true
	var viewport := get_viewport()
	if viewport == null:
		return true
	var size := viewport.get_visible_rect().size
	var margin := size * safe_margin_ratio()
	if is_position_behind(_desired_focus):
		return false
	var projected := unproject_position(_desired_focus)
	return projected.x >= margin.x and projected.y >= margin.y \
		and projected.x <= size.x - margin.x and projected.y <= size.y - margin.y


func recenter() -> void:
	# The freeze covers every camera motion, recentring included: the rig
	# refuses it exactly like orbit and zoom while the launcher is grabbed.
	if not _configured or _locked:
		return
	_yaw_degrees = 0.0
	_inclination_degrees = _default_inclination_degrees
	_distance = _default_distance
	_desired_focus = _anchor
	_focus = _anchor
	_transition_remaining = 0.0
	_apply_transform()


func focus_on(anchor: Vector3) -> void:
	if not _configured:
		return
	_anchor = anchor
	_desired_focus = anchor
	_transition_remaining = transition_seconds()
	if _reduced_motion:
		_focus = _desired_focus
		_transition_remaining = 0.0
		_apply_transform()


func apply_orbit(delta: Vector2) -> bool:
	if not _configured or _locked:
		return false
	var sensitivity := _profile.orbit_sensitivity_deg as Array
	if is_zero_approx(float(sensitivity[0])) and is_zero_approx(float(sensitivity[1])):
		return false
	var inclinations := _profile.inclination_range_deg as Array
	_yaw_degrees = fmod(_yaw_degrees - delta.x * float(sensitivity[0]), 360.0)
	_inclination_degrees = clampf(
		_inclination_degrees - delta.y * float(sensitivity[1]),
		float(inclinations[0]), float(inclinations[1]))
	return true


func apply_zoom(amount: float) -> bool:
	if not _configured or _locked or not is_finite(amount) \
			or is_zero_approx(float(_profile.zoom_step_m)):
		return false
	var distances := _profile.distance_range_m as Array
	_distance = clampf(
		_distance - amount * float(_profile.zoom_step_m),
		float(distances[0]), float(distances[1]))
	return true


func observe_frame(_frame: Dictionary) -> void:
	pass


func _process(delta: float) -> void:
	if not _configured or _locked:
		return
	if not _reduced_motion:
		_yaw_degrees = fmod(
			_yaw_degrees + IDLE_YAW_DEGREES_PER_SECOND * delta, 360.0)
	if _transition_remaining > 0.0:
		var step := minf(delta / maxf(_transition_remaining, 0.0001), 1.0)
		_focus = _focus.lerp(_desired_focus, step)
		_transition_remaining = maxf(0.0, _transition_remaining - delta)
	else:
		_focus = _desired_focus
	_apply_transform()


func _apply_transform() -> void:
	var inclination := deg_to_rad(_inclination_degrees)
	var direction := _base_offset.rotated(_up, deg_to_rad(_yaw_degrees))
	var offset := (direction * cos(inclination) + _up * sin(inclination)).normalized() \
		* _distance
	global_position = _focus + offset
	look_at(_focus, _up)
