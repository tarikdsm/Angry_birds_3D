extends Camera3D

## Orbital product rig.
##
## Presentation only. It keeps the Orbital composition rules of the vertical
## slice — 48 degrees, 8 percent safe margin and no framing through the
## planet — but reads every number from the camera profile and the level
## manifest instead of the constants of a single fortification. The legacy
## scripts/camera/orbital_camera.gd stays untouched for the v1 regression.

const SMOOTHING_SPEED := 6.0
const KICK_DECAY_PER_SECOND := 5.0
const LOOK_AHEAD_METERS := 2.0
const TARGET_ATTENTION_METERS := 16.0
const SURFACE_CLEARANCE_METERS := 0.15

var _profile: Dictionary = {}
var _bounds: Dictionary = {}
var _anchor := Vector3.ZERO
var _center := Vector3.ZERO
var _surface_radius := 0.0
var _shell_radius := 0.0
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
	if profile.is_empty() or str(bounds.get("kind", "")) != "sphere":
		return false
	_profile = profile.duplicate(true)
	_bounds = bounds.duplicate(true)
	_anchor = anchor
	_center = _bounds.center_m as Vector3
	_shell_radius = float(_bounds.radius_m)
	_surface_radius = float(_bounds.surface_radius_m)
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
	return &"orbital"


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


func recenter() -> void:
	# The freeze covers every camera motion, recentring included: the rig
	# refuses it exactly like orbit and zoom while the launcher is grabbed.
	if not _configured or _locked:
		return
	_yaw_degrees = 0.0
	_inclination_degrees = _default_inclination_degrees
	_distance = _default_distance
	_impact_remaining = 0.0
	_desired_focus = _outside_surface(_anchor)
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
		_impact_focus = _outside_surface((event as Dictionary).get(
			"position", _desired_focus) as Vector3)
		_impact_remaining = float(_profile.get("impact_focus_seconds", 0.0))
		if not _reduced_motion and _shake_enabled:
			_pending_kick_degrees = float(_profile.get("impact_fov_kick_degrees", 0.0))
	if _reduced_motion:
		_impact_remaining = 0.0
	_required_points = _composition_points(frame)
	_desired_focus = _outside_surface(_average(_required_points))
	fov = float(_profile.fov_degrees) + _pending_kick_degrees
	_compose(not _composed)


func is_occluded() -> bool:
	return _segment_hits_surface(global_position, _focus)


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
	_focus = _outside_surface(_focus.lerp(_desired_focus, weight))
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
	var distances := _profile.distance_range_m as Array
	var distance := clampf(
		maxf(_distance, _fit_distance(focus)),
		float(distances[0]), float(distances[1]))
	var target := _clear_position(focus + _offset(focus, distance), focus, distance)
	if weight >= 1.0:
		global_position = target
	else:
		global_position = _clear_position(
			global_position.lerp(target, weight), focus, distance)
	look_at(focus, _radial(focus))


func _radial(point: Vector3) -> Vector3:
	var radial := point - _center
	if radial.is_zero_approx():
		return Vector3.UP
	return radial.normalized()


func _offset(focus: Vector3, distance: float) -> Vector3:
	var up := _radial(focus)
	var tangent := up.cross(Vector3.UP)
	if tangent.is_zero_approx():
		tangent = up.cross(Vector3.RIGHT)
	tangent = tangent.normalized()
	var inclination := deg_to_rad(_inclination_degrees)
	var direction := tangent.rotated(up, deg_to_rad(_yaw_degrees))
	return (direction * cos(inclination) + up * sin(inclination)).normalized() * distance


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


func _clear_position(candidate: Vector3, focus: Vector3, distance: float) -> Vector3:
	var clamped := candidate
	var from_center := clamped - _center
	if from_center.length() > _shell_radius:
		clamped = _center + from_center.normalized() * _shell_radius
	if not _segment_hits_surface(clamped, focus):
		return clamped
	var up := _radial(focus)
	var offset := clamped - focus
	var tangent := offset - up * offset.dot(up)
	if tangent.is_zero_approx():
		tangent = up.cross(Vector3.UP)
	if tangent.is_zero_approx():
		tangent = up.cross(Vector3.RIGHT)
	var safe := up * maxf(_surface_radius * 0.3, distance * 0.55) \
		+ tangent.normalized() * distance * 0.85
	return focus + safe.normalized() * minf(safe.length(), distance)


func _segment_hits_surface(origin: Vector3, target: Vector3) -> bool:
	var segment := target - origin
	var length_squared := segment.length_squared()
	if length_squared <= 0.0001:
		return false
	var relative := origin - _center
	var closest := clampf(-relative.dot(segment) / length_squared, 0.0, 1.0)
	return (relative + segment * closest).length_squared() \
		< _surface_radius * _surface_radius


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


func _outside_surface(value: Vector3) -> Vector3:
	var relative := value - _center
	var minimum := _surface_radius + SURFACE_CLEARANCE_METERS
	if relative.length() >= minimum:
		if relative.length() <= _shell_radius:
			return value
		return _center + relative.normalized() * _shell_radius
	if relative.is_zero_approx():
		return _center + Vector3.UP * minimum
	return _center + relative.normalized() * minimum
