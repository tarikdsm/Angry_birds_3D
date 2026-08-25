extends Camera3D

## Terrestrial product rig.
##
## The rig is presentation only: it never queries a physics solver, never
## authors gameplay and never publishes a launch velocity. Field of view, safe
## margin, ranges and impact response come from the camera profile; the
## composition bounds and the launcher anchor come from the level manifest.

const SMOOTHING_SPEED := 6.0
const KICK_DECAY_PER_SECOND := 5.0
const LOOK_AHEAD_METERS := 2.0
const TARGET_ATTENTION_METERS := 18.0

var _profile: Dictionary = {}
var _bounds: Dictionary = {}
var _anchor := Vector3.ZERO
var _up := Vector3.UP
var _base_offset := Vector3.BACK
var _configured := false
var _composed := false
var _locked := false
var _reduced_motion := false
var _shake_enabled := true
var _yaw_degrees := 0.0
var _inclination_degrees := 0.0
var _distance := 0.0
var _default_inclination_degrees := 0.0
var _default_distance := 0.0
var _phase := "inspection"
var _focus := Vector3.ZERO
var _desired_focus := Vector3.ZERO
var _impact_focus := Vector3.ZERO
var _impact_remaining := 0.0
var _pending_kick_degrees := 0.0
var _required_points: Array[Vector3] = []


func configure(profile: Dictionary, bounds: Dictionary, anchor: Vector3) -> bool:
	if profile.is_empty() or str(bounds.get("kind", "")) != "box":
		return false
	_profile = profile.duplicate(true)
	_bounds = bounds.duplicate(true)
	_anchor = anchor
	_up = Vector3.UP
	var forward := _clamped_center() - _anchor
	forward -= _up * forward.dot(_up)
	if forward.is_zero_approx():
		forward = Vector3.RIGHT
	_base_offset = forward.normalized().cross(_up).normalized()
	if _base_offset.is_zero_approx():
		_base_offset = Vector3.BACK
	var inclinations := _profile.inclination_range_deg as Array
	_default_inclination_degrees = float(inclinations[0]) \
		+ (float(inclinations[1]) - float(inclinations[0])) * 0.25
	var distances := _profile.distance_range_m as Array
	_default_distance = float(distances[0]) \
		+ (float(distances[1]) - float(distances[0])) * 0.35
	fov = float(_profile.fov_degrees)
	near = 0.08
	far = 400.0
	_configured = true
	_composed = false
	_locked = false
	_phase = "inspection"
	_required_points = [_anchor]
	recenter()
	return true


func release() -> void:
	_configured = false
	_composed = false
	_locked = false
	_profile = {}
	_bounds = {}
	_required_points = []


func rig_kind() -> StringName:
	return &"terrestrial"


func profile_id() -> String:
	return str(_profile.get("id", ""))


func composition_bounds() -> Dictionary:
	return _bounds.duplicate(true)


func safe_margin_ratio() -> float:
	return float(_profile.get("safe_margin_ratio", 0.08))


func transition_seconds() -> float:
	if _reduced_motion:
		return float(_profile.get("reduced_motion_transition_seconds", 0.1))
	return float(_profile.get("transition_seconds", 0.35))


func pending_kick_degrees() -> float:
	return _pending_kick_degrees


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
		_pending_kick_degrees = 0.0
		fov = float(_profile.get("fov_degrees", fov))


func reduced_motion() -> bool:
	return _reduced_motion


func set_shake_enabled(value: bool) -> void:
	_shake_enabled = value
	if not value:
		_pending_kick_degrees = 0.0


func shake_enabled() -> bool:
	return _shake_enabled


func is_occluded() -> bool:
	return false


func recenter() -> void:
	# The freeze covers every camera motion, recentring included: the rig
	# refuses it exactly like orbit and zoom while the launcher is grabbed.
	if not _configured or _locked:
		return
	_yaw_degrees = 0.0
	_inclination_degrees = _default_inclination_degrees
	_distance = _default_distance
	_impact_remaining = 0.0
	_desired_focus = _clamped_point(_anchor)
	_focus = _desired_focus
	_composed = false
	_compose(true)


func apply_orbit(delta: Vector2) -> bool:
	if not _configured or _locked:
		return false
	var sensitivity := _profile.orbit_sensitivity_deg as Array
	var inclinations := _profile.inclination_range_deg as Array
	_yaw_degrees = fmod(_yaw_degrees - delta.x * float(sensitivity[0]), 360.0)
	_inclination_degrees = clampf(
		_inclination_degrees - delta.y * float(sensitivity[1]),
		float(inclinations[0]), float(inclinations[1]))
	return true


func apply_zoom(amount: float) -> bool:
	if not _configured or _locked or not is_finite(amount):
		return false
	var distances := _profile.distance_range_m as Array
	_distance = clampf(
		_distance - amount * float(_profile.zoom_step_m),
		float(distances[0]), float(distances[1]))
	return true


func observe_frame(frame: Dictionary) -> void:
	if not _configured:
		return
	_phase = str(frame.get("phase", _phase))
	_locked = _phase == "grabbed"
	if _locked:
		return
	_pending_kick_degrees = 0.0
	for event: Variant in frame.get("events", []):
		if not event is Dictionary:
			continue
		if str((event as Dictionary).get("kind", "")) != "damage_applied":
			continue
		_impact_focus = _clamped_point((event as Dictionary).get(
			"position", _desired_focus) as Vector3)
		_impact_remaining = float(_profile.get("impact_focus_seconds", 0.0))
		if not _reduced_motion and _shake_enabled:
			_pending_kick_degrees = float(_profile.get("impact_fov_kick_degrees", 0.0))
	if _reduced_motion:
		_impact_remaining = 0.0
	_required_points = _composition_points(frame)
	_desired_focus = _clamped_point(_average(_required_points))
	fov = float(_profile.fov_degrees) + _pending_kick_degrees
	_compose(not _composed)


func is_safe_framing() -> bool:
	if _required_points.is_empty():
		return true
	var viewport := get_viewport()
	if viewport == null:
		return true
	var size := viewport.get_visible_rect().size
	var margin := size * safe_margin_ratio()
	for point: Vector3 in _required_points:
		if is_position_behind(point):
			return false
		var projected := unproject_position(point)
		if projected.x < margin.x or projected.y < margin.y \
				or projected.x > size.x - margin.x or projected.y > size.y - margin.y:
			return false
	return true


func _process(delta: float) -> void:
	if not _configured or _locked:
		return
	if _impact_remaining > 0.0:
		_impact_remaining = maxf(0.0, _impact_remaining - delta)
		_desired_focus = _impact_focus
	var weight := 1.0 - exp(-SMOOTHING_SPEED * delta)
	_focus = _focus.lerp(_desired_focus, weight)
	_pending_kick_degrees = move_toward(
		_pending_kick_degrees, 0.0, delta * KICK_DECAY_PER_SECOND)
	fov = float(_profile.fov_degrees) + (0.0 if _reduced_motion else _pending_kick_degrees)
	_apply_transform(_focus, weight)


func _compose(snap: bool) -> void:
	if not _configured or not snap:
		_composed = true
		return
	_focus = _desired_focus
	_apply_transform(_focus, 1.0)
	_composed = true


func _apply_transform(focus: Vector3, weight: float) -> void:
	var distance := maxf(_distance, _fit_distance(focus))
	var target := focus + _offset(distance)
	if weight >= 1.0:
		global_position = target
	else:
		global_position = global_position.lerp(target, weight)
	look_at(focus, _up)


func _offset(distance: float) -> Vector3:
	var inclination := deg_to_rad(_inclination_degrees)
	var direction := _base_offset.rotated(_up, deg_to_rad(_yaw_degrees))
	return (direction * cos(inclination) + _up * sin(inclination)).normalized() * distance


func _fit_distance(focus: Vector3) -> float:
	var radius := 0.0
	for point: Vector3 in _required_points:
		radius = maxf(radius, point.distance_to(focus))
	var distances := _profile.distance_range_m as Array
	if radius <= 0.01:
		return float(distances[0])
	var usable := deg_to_rad(float(_profile.fov_degrees) * 0.5) \
		* (1.0 - safe_margin_ratio() * 2.0)
	return clampf(radius / tan(usable) + 2.0, float(distances[0]), float(distances[1]))


func _composition_points(frame: Dictionary) -> Array[Vector3]:
	var targets: Array[Vector3] = []
	for snapshot: Variant in frame.get("snapshots", []):
		if not snapshot is Dictionary or not (snapshot as Dictionary).has("enemy_archetype_id"):
			continue
		targets.append(((snapshot as Dictionary).get(
			"transform", Transform3D.IDENTITY) as Transform3D).origin)
	var projectiles: Array[Vector3] = []
	for projectile: Variant in frame.get("projectiles", []):
		if not projectile is Dictionary:
			continue
		var body := projectile as Dictionary
		var origin := (body.get("transform", Transform3D.IDENTITY) as Transform3D).origin
		var velocity := body.get("linear_velocity", Vector3.ZERO) as Vector3
		if velocity.length_squared() > 0.0001:
			origin += velocity.normalized() * LOOK_AHEAD_METERS
		projectiles.append(origin)
	if _phase == "flight_ability" and not projectiles.is_empty():
		var points := projectiles.duplicate() as Array[Vector3]
		for target: Vector3 in targets:
			if target.distance_to(projectiles[0]) <= TARGET_ATTENTION_METERS:
				points.append(target)
		return points
	if _phase in ["resolution", "evaluation", "result"] and not targets.is_empty():
		return targets
	var framing: Array[Vector3] = [_anchor]
	for target: Vector3 in targets:
		framing.append(target)
	return framing


func _average(points: Array[Vector3]) -> Vector3:
	if points.is_empty():
		return _anchor
	var total := Vector3.ZERO
	for point: Vector3 in points:
		total += point
	return total / float(points.size())


func _clamped_center() -> Vector3:
	return ((_bounds.min_m as Vector3) + (_bounds.max_m as Vector3)) * 0.5


func _clamped_point(value: Vector3) -> Vector3:
	var minimum := _bounds.min_m as Vector3
	var maximum := _bounds.max_m as Vector3
	return Vector3(
		clampf(value.x, minimum.x, maximum.x),
		clampf(value.y, minimum.y, maximum.y),
		clampf(value.z, minimum.z, maximum.z))
