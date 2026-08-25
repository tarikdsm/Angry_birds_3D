extends SceneTree

const CAMERA_DIRECTOR := preload("res://scripts/camera/camera_director.gd")
const CAMERA_PROFILE_CATALOG := preload("res://scripts/camera/camera_profile_catalog.gd")
const CONTENT_FILE_LOADER := preload("res://scripts/data/content_file_loader.gd")
const EARTH_LEVEL_PATH := "res://data/levels/earth/farm_reaction.level.json"
const ORBITAL_LEVEL_PATH := "res://data/levels/orbital/first_orbit_v2.level.json"
const LEGACY_ORBITAL_CAMERA := "res://scripts/camera/orbital_camera.gd"
const MARKER := "CAMERA_PROFILES_SMOKE_OK"

var _errors: Array[String] = []
var _director: Node3D
var _earth: Dictionary = {}
var _orbital: Dictionary = {}


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	if not _load_levels():
		_fail(" | ".join(_errors))
		return
	_check_catalog()
	if _errors.is_empty():
		await _check_profiles()
	if not _errors.is_empty():
		_fail(" | ".join(_errors))
		return
	print(MARKER)
	_finish(0)


func _load_levels() -> bool:
	var earth := CONTENT_FILE_LOADER.load_json(EARTH_LEVEL_PATH)
	var orbital := CONTENT_FILE_LOADER.load_json(ORBITAL_LEVEL_PATH)
	if not bool(earth.get("ok", false)) or not bool(orbital.get("ok", false)):
		_errors.append("the shipped level manifests must load before the camera smoke")
		return false
	_earth = earth.document as Dictionary
	_orbital = orbital.document as Dictionary
	return true


func _check_catalog() -> void:
	var result: Dictionary = CAMERA_PROFILE_CATALOG.load_catalog()
	if not bool(result.get("ok", false)):
		_check(false, "the camera profile catalog must load: %s" % result.get("message", ""))
		return
	var document := result.document as Dictionary
	var ids: Array[String] = []
	for profile: Variant in document.get("profiles", []):
		ids.append(str((profile as Dictionary).get("id", "")))
	_check(ids == ["CAM_Carousel", "CAM_Farm", "CAM_Orbital"],
		"the catalog must publish the three product camera profiles in canonical order")
	_check(str(CAMERA_PROFILE_CATALOG.profile_of(document, "CAM_Farm").get("rig", "")) \
			== "terrestrial",
		"CAM_Farm must select the terrestrial rig")
	_check(str(CAMERA_PROFILE_CATALOG.profile_of(document, "CAM_Orbital").get("rig", "")) \
			== "orbital",
		"CAM_Orbital must select the orbital rig")
	_check(CAMERA_PROFILE_CATALOG.profile_of(document, "CAM_Unknown").is_empty(),
		"an unregistered camera profile must resolve to nothing")

	var tampered := document.duplicate(true)
	(tampered.profiles[1] as Dictionary).fov_degrees = 60.0
	_check(not CAMERA_PROFILE_CATALOG.validate_catalog_document(tampered).is_empty(),
		"the terrestrial field of view must stay pinned at 44 degrees")
	tampered = document.duplicate(true)
	(tampered.profiles[2] as Dictionary).safe_margin_ratio = 0.02
	_check(not CAMERA_PROFILE_CATALOG.validate_catalog_document(tampered).is_empty(),
		"the Orbital safe margin must stay pinned at 8 percent")
	tampered = document.duplicate(true)
	(tampered.profiles[0] as Dictionary)["extra_key"] = true
	_check(not CAMERA_PROFILE_CATALOG.validate_catalog_document(tampered).is_empty(),
		"the camera catalog must be a strict closed schema")


func _check_profiles() -> void:
	_director = CAMERA_DIRECTOR.new()
	root.add_child(_director)
	await process_frame

	_check(not _director.configure("CAM_Unknown", _earth),
		"an unregistered camera profile must fail closed")
	if not _director.configure("CAM_Farm", _earth):
		_check(false, "the Earth profile must configure: %s" % _director.last_error)
		return
	var terrestrial: Camera3D = _director.active_rig()
	_check(is_equal_approx(terrestrial.fov, 44.0),
		"the Earth camera must keep a 44 degree field of view")
	_check(str(_director.active_profile_id()) == "CAM_Farm"
			and str(_director.active_rig_kind()) == "terrestrial",
		"the Earth profile must select the terrestrial rig")
	_director.observe_frame(_frame("inspection", []))
	await process_frame
	_check(absf(terrestrial.global_basis.x.y) <= 0.001,
		"the Earth camera must never roll")
	var earth_bounds: Dictionary = _director.composition_bounds()
	_check(str(earth_bounds.get("kind", "")) == "box"
			and (earth_bounds.get("min_m", Vector3.ZERO) as Vector3).is_equal_approx(
				Vector3(-24.0, -12.0, -12.0))
			and (earth_bounds.get("max_m", Vector3.ZERO) as Vector3).is_equal_approx(
				Vector3(48.0, 32.0, 12.0)),
		"Earth composition bounds must come from the level manifest: %s" % earth_bounds)
	var narrowed := _earth.duplicate(true)
	((narrowed.world as Dictionary).bounds as Dictionary).max_m = [30.0, 20.0, 8.0]
	var narrowed_ok: bool = _director.configure("CAM_Farm", narrowed)
	var narrowed_bounds: Dictionary = _director.composition_bounds()
	_check(narrowed_ok
			and (narrowed_bounds.get("max_m", Vector3.ZERO) as Vector3).is_equal_approx(
				Vector3(30.0, 20.0, 8.0)),
		"camera bounds must follow the manifest, never a hardcoded fortification")
	_check(_director.configure("CAM_Farm", _earth), "the Earth profile must reconfigure")

	_check(_director.transition_seconds() > 0.0,
		"every camera profile must publish a transition duration")
	_director.observe_frame(_frame("resolution", [_damage_event()]))
	_check(is_equal_approx(float(_director.pending_kick_degrees()), 2.0),
		"an impact must kick the camera by at most 2 degrees")
	_director.set_reduced_motion(true)
	_director.observe_frame(_frame("resolution", [_damage_event()]))
	_check(is_equal_approx(float(_director.pending_kick_degrees()), 0.0),
		"reduced motion must remove the impact kick")
	_check(is_equal_approx(terrestrial.fov, 44.0),
		"reduced motion must keep the profile field of view exact")
	_check(is_equal_approx(_director.transition_seconds(), 0.1),
		"reduced motion must collapse camera transitions to 0.1 s")
	_director.set_reduced_motion(false)

	if not _director.configure("CAM_Orbital", _orbital):
		_check(false, "the Orbital profile must configure: %s" % _director.last_error)
		return
	var orbital: Camera3D = _director.active_rig()
	_check(is_equal_approx(orbital.fov, 48.0),
		"the Orbital camera must preserve its 48 degree field of view")
	_check(is_equal_approx(_director.safe_margin_ratio(), 0.08),
		"the Orbital camera must preserve its 8 percent safe margin")
	_check(not terrestrial.current and orbital.current,
		"exactly one rig may be current at a time")
	_director.observe_frame(_frame("inspection", []))
	await process_frame
	_check(not _director.is_occluded(),
		"the Orbital camera must never compose through the planet")
	_check(_director.is_safe_framing(),
		"the Orbital camera must keep its required points inside the safe margin")
	var orbital_bounds: Dictionary = _director.composition_bounds()
	_check(str(orbital_bounds.get("kind", "")) == "sphere"
			and (orbital_bounds.get("center_m", Vector3.ONE) as Vector3).is_equal_approx(
				Vector3.ZERO)
			and is_equal_approx(float(orbital_bounds.get("radius_m", 0.0)), 60.0)
			and is_equal_approx(float(orbital_bounds.get("surface_radius_m", 0.0)), 10.0),
		"Orbital composition bounds must come from the level manifest: %s" % orbital_bounds)

	var before_position := orbital.global_position
	_director.observe_frame(_frame("grabbed", []))
	_check(_director.is_locked(), "the grabbed phase must freeze the camera")
	_check(not _director.apply_orbit(Vector2(30.0, 12.0)),
		"orbit must be refused while the launcher is grabbed")
	_check(not _director.apply_zoom(-2.0),
		"zoom must be refused while the launcher is grabbed")
	await process_frame
	_check(orbital.global_position.is_equal_approx(before_position),
		"a frozen camera must not drift during a grab")
	_director.observe_frame(_frame("flight_ability", []))
	_check(not _director.is_locked(), "leaving the grab must unfreeze the camera")
	_check(_director.apply_orbit(Vector2(30.0, 12.0)) and _director.apply_zoom(-2.0),
		"orbit and zoom must resume outside the grab")
	var orbited: Vector2 = _director.orbit_degrees()
	_director.recenter()
	_check(_director.orbit_degrees() != orbited,
		"recenter must restore the authored composition")

	# The presentation owns the freeze only until the kernel confirms the
	# gesture is over. If it kept that ownership forever, the phase lock — the
	# safety net against a desynchronised presentation — would retire after
	# the very first grab of the level.
	_director.lock()
	_check(_director.is_locked(), "a presentation grab must freeze the camera")
	_director.unlock()
	_director.observe_frame(_frame("inspection", []))
	_check(not _director.is_locked(), "the release must unfreeze the camera")
	_director.observe_frame(_frame("grabbed", []))
	_check(_director.is_locked(),
		"a published grabbed phase must still freeze the camera after a gesture")
	_check(not _director.apply_orbit(Vector2(20.0, 5.0)),
		"orbit must stay refused under the phase lock after a gesture")
	_director.observe_frame(_frame("flight_ability", []))
	_check(not _director.is_locked(),
		"leaving the published grab must unfreeze the camera again")

	if not _director.configure("CAM_Carousel", _earth):
		_check(false, "the carousel profile must configure: %s" % _director.last_error)
		return
	_check(is_equal_approx((_director.active_rig() as Camera3D).fov, 35.0),
		"the carousel camera must keep a 35 degree field of view")
	_check(is_equal_approx(_director.transition_seconds(), 0.45),
		"the carousel must transition in 0.45 s")
	_director.set_reduced_motion(true)
	_check(is_equal_approx(_director.transition_seconds(), 0.1),
		"reduced motion must replace the carousel transition with a 0.1 s cut")
	_director.set_reduced_motion(false)

	var legacy := FileAccess.get_file_as_string(LEGACY_ORBITAL_CAMERA)
	_check(legacy.contains("const BASE_FOV := 48.0")
			and legacy.contains("const FORTIFICATION_CENTER"),
		"the v1 orbital camera must stay untouched next to the product rigs")

	_director.release()
	_check(_director.active_rig() == null and str(_director.active_profile_id()).is_empty(),
		"releasing the level must retire the active rig")


func _frame(phase: String, events: Array) -> Dictionary:
	return {
		"phase": phase,
		"launcher": null,
		"locked_plane": null,
		"projectiles": [],
		"snapshots": [],
		"objectives": {"complete": false, "targets": []},
		"events": events,
	}


func _damage_event() -> Dictionary:
	return {
		"kind": "damage_applied",
		"position": Vector3(6.0, 3.0, 0.0),
		"entity_id": 2001,
	}


func _check(condition: bool, message: String) -> void:
	if not condition:
		_errors.append(message)


func _fail(message: String) -> void:
	push_error("camera profiles smoke: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	if is_instance_valid(_director):
		root.remove_child(_director)
		_director.free()
	quit(exit_code)
