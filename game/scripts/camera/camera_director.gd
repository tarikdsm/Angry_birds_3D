extends Node3D

## Selects the product camera rig for a data-driven profile.
##
## The director owns no gameplay and no physics: it only chooses a rig, feeds it
## the published frame and forwards camera intents. It refuses every orbit and
## zoom while the launcher is grabbed, so the camera stays frozen from BeginGrab
## until the cancel or the release.

const CAMERA_PROFILE_CATALOG := preload("res://scripts/camera/camera_profile_catalog.gd")
const CAROUSEL_RIG := preload("res://scripts/camera/carousel_camera_rig.gd")
const ORBITAL_RIG := preload("res://scripts/camera/orbital_camera_rig.gd")
const TERRESTRIAL_RIG := preload("res://scripts/camera/terrestrial_camera_rig.gd")
const INPUT_INTENT := preload("res://scripts/input/input_intent.gd")
const RIG_NAMES := {
	"carousel": "CarouselRig",
	"terrestrial": "TerrestrialRig",
	"orbital": "OrbitalRig",
}

signal profile_activated(profile_id: String)

var last_error := ""
var recenter_calls := 0

var _catalog: Dictionary = {}
var _rigs: Dictionary = {}
var _active_rig: Camera3D = null
var _active_profile: Dictionary = {}
var _grab_locked := false
var _phase_locked := false
var _presentation_driven := false
var _reduced_motion := false
var _shake_enabled := true


static func composition_bounds_of(level_document: Dictionary) -> Dictionary:
	var world := level_document.get("world", {}) as Dictionary
	var bounds := world.get("bounds", {}) as Dictionary
	if str(world.get("kind", "")) == "uniform":
		if not bounds.has("min_m") or not bounds.has("max_m"):
			return {}
		return {
			"kind": "box",
			"min_m": _vector_of(bounds.min_m),
			"max_m": _vector_of(bounds.max_m),
		}
	if str(world.get("kind", "")) == "radial":
		if not bounds.has("radius_m") or not world.has("reference_radius_m"):
			return {}
		return {
			"kind": "sphere",
			"center_m": _vector_of(world.get("center_m", [0.0, 0.0, 0.0])),
			"radius_m": float(bounds.radius_m),
			"surface_radius_m": float(world.reference_radius_m),
		}
	return {}


static func launcher_anchor_of(level_document: Dictionary) -> Vector3:
	var slingshot := level_document.get("slingshot", {}) as Dictionary
	return _vector_of(slingshot.get("rest_position_m", [0.0, 0.0, 0.0]))


func _ready() -> void:
	for rig_kind: String in RIG_NAMES:
		var rig := Camera3D.new()
		rig.name = StringName(str(RIG_NAMES[rig_kind]))
		match rig_kind:
			"carousel":
				rig.set_script(CAROUSEL_RIG)
			"terrestrial":
				rig.set_script(TERRESTRIAL_RIG)
			_:
				rig.set_script(ORBITAL_RIG)
		rig.set_process(false)
		add_child(rig)
		_rigs[rig_kind] = rig


func configure(profile_id: String, level_document: Dictionary) -> bool:
	last_error = ""
	if _rigs.is_empty():
		last_error = "the camera director was configured before it entered the tree"
		return false
	if _catalog.is_empty():
		var result: Dictionary = CAMERA_PROFILE_CATALOG.load_catalog()
		if not bool(result.get("ok", false)):
			last_error = str(result.get("message", "camera catalog could not be loaded"))
			return false
		_catalog = result.document as Dictionary
	var profile: Dictionary = CAMERA_PROFILE_CATALOG.profile_of(_catalog, profile_id)
	if profile.is_empty():
		last_error = "camera profile is not registered: %s" % profile_id
		return false
	var bounds := composition_bounds_of(level_document)
	if bounds.is_empty():
		last_error = "the level manifest does not publish camera composition bounds"
		return false
	var rig_kind := str(profile.rig)
	if not _rigs.has(rig_kind):
		last_error = "camera rig is not available: %s" % rig_kind
		return false
	var rig := _rigs[rig_kind] as Camera3D
	if not rig.configure(profile, bounds, launcher_anchor_of(level_document)):
		last_error = "camera rig rejected the registered profile: %s" % profile_id
		return false
	_retire_inactive(rig)
	_active_rig = rig
	_active_profile = profile
	_grab_locked = false
	_phase_locked = false
	_presentation_driven = false
	rig.set_reduced_motion(_reduced_motion)
	rig.set_shake_enabled(_shake_enabled)
	rig.set_process(true)
	rig.make_current()
	profile_activated.emit(profile_id)
	return true


func release() -> void:
	_retire_inactive(null)
	_active_rig = null
	_active_profile = {}
	_grab_locked = false
	_phase_locked = false
	_presentation_driven = false


func active_rig() -> Camera3D:
	return _active_rig


func active_rig_kind() -> StringName:
	if _active_rig == null:
		return &""
	return _active_rig.rig_kind()


func active_profile_id() -> String:
	return str(_active_profile.get("id", ""))


func composition_bounds() -> Dictionary:
	if _active_rig == null:
		return {}
	return _active_rig.composition_bounds()


func safe_margin_ratio() -> float:
	if _active_rig == null:
		return 0.0
	return _active_rig.safe_margin_ratio()


func transition_seconds() -> float:
	if _active_rig == null:
		return 0.0
	return _active_rig.transition_seconds()


func pending_kick_degrees() -> float:
	if _active_rig == null:
		return 0.0
	return _active_rig.pending_kick_degrees()


func orbit_degrees() -> Vector2:
	if _active_rig == null:
		return Vector2.ZERO
	return _active_rig.orbit_degrees()


func is_occluded() -> bool:
	if _active_rig == null:
		return false
	return _active_rig.is_occluded()


func is_safe_framing() -> bool:
	if _active_rig == null:
		return true
	return _active_rig.is_safe_framing()


## The grab freeze is owned by the presentation: it opens at BeginGrab and
## closes at the cancel or the release, without waiting for the kernel to
## republish the phase. Until a gesture takes over, the published phase is the
## only source of truth.
func lock() -> void:
	_presentation_driven = true
	_grab_locked = true
	_phase_locked = false
	_apply_lock()


func unlock() -> void:
	_presentation_driven = true
	_grab_locked = false
	_phase_locked = false
	_apply_lock()


func is_locked() -> bool:
	return _grab_locked or _phase_locked


func set_reduced_motion(value: bool) -> void:
	_reduced_motion = value
	for rig_kind: Variant in _rigs:
		(_rigs[rig_kind] as Camera3D).set_reduced_motion(value)


func reduced_motion() -> bool:
	return _reduced_motion


## Camera shake is the impact kick of the profile. It is independent from
## reduced motion: a player may keep the guided transitions and still turn the
## kick off.
func set_shake_enabled(value: bool) -> void:
	_shake_enabled = value
	for rig_kind: Variant in _rigs:
		(_rigs[rig_kind] as Camera3D).set_shake_enabled(value)


func shake_enabled() -> bool:
	return _shake_enabled


func recenter() -> void:
	if _active_rig == null or is_locked():
		return
	recenter_calls += 1
	_active_rig.recenter()


func apply_orbit(delta: Vector2) -> bool:
	if _active_rig == null or is_locked():
		return false
	return _active_rig.apply_orbit(delta)


func apply_zoom(amount: float) -> bool:
	if _active_rig == null or is_locked():
		return false
	return _active_rig.apply_zoom(amount)


func handle_intent(intent: RefCounted) -> bool:
	if intent == null or not intent is INPUT_INTENT:
		return false
	if intent.kind == INPUT_INTENT.KIND_ORBIT:
		return apply_orbit(intent.payload.get("delta", Vector2.ZERO) as Vector2)
	if intent.kind == INPUT_INTENT.KIND_ZOOM:
		return apply_zoom(float(intent.payload.get("amount", 0.0)))
	if intent.kind == INPUT_INTENT.KIND_RECENTER:
		var before := recenter_calls
		recenter()
		return recenter_calls > before
	return false


func observe_frame(frame: Dictionary) -> void:
	_phase_locked = not _presentation_driven \
		and str(frame.get("phase", "")) == "grabbed"
	if _active_rig == null:
		return
	_apply_lock()
	if is_locked():
		return
	_active_rig.observe_frame(frame)


func _apply_lock() -> void:
	if _active_rig != null:
		_active_rig.set_locked(is_locked())


func _retire_inactive(keep: Camera3D) -> void:
	for rig_kind: Variant in _rigs:
		var rig := _rigs[rig_kind] as Camera3D
		if rig == keep:
			continue
		rig.set_process(false)
		if rig.current:
			rig.clear_current(false)
		rig.release()


static func _vector_of(value: Variant) -> Vector3:
	if value is Vector3:
		return value as Vector3
	if not value is Array or (value as Array).size() != 3:
		return Vector3.ZERO
	var values := value as Array
	return Vector3(float(values[0]), float(values[1]), float(values[2]))
