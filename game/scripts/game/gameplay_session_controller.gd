extends Node3D

const CONTENT_FILE_LOADER := preload("res://scripts/data/content_file_loader.gd")
const ASSET_CATALOG := preload("res://scripts/data/asset_catalog.gd")
const INPUT_INTENT := preload("res://scripts/input/input_intent.gd")
const PRODUCT_TEXT_CATALOG := preload("res://scripts/data/product_text_catalog.gd")
const UI_CATALOG_PATH := "res://data/ui/product_v2.pt-BR.json"
const TERMINAL_PHASE := "result"
const TERMINAL_OUTCOMES := ["victory", "defeat"]
const MATERIALS_PATH := "res://data/materials/product_v2.materials.json"
const ARCHETYPES_PATH := "res://data/archetypes/product_v2.archetypes.json"
const REQUEST_KEYS := [
	"world_id", "level_id", "scene_id", "camera_profile_id", "presentation_profile_id",
	"materials_path", "archetypes_path", "level_path", "world_scene_path",
]
const LEVEL_REGISTRY := {
	"earth/farm_reaction": {
		"world_id": "earth",
		"level_id": "farm_reaction",
		"scene_id": "SCN_FarmReaction",
		"camera_profile_id": "CAM_Farm",
		"presentation_profile_id": "PRS_Farm",
		"materials_path": MATERIALS_PATH,
		"archetypes_path": ARCHETYPES_PATH,
		"level_path": "res://data/levels/earth/farm_reaction.level.json",
		"world_scene_path": "res://scenes/worlds/earth_farm.tscn",
	},
	"orbital/first_orbit_v2": {
		"world_id": "orbital",
		"level_id": "first_orbit_v2",
		"scene_id": "SCN_FirstOrbitV2",
		"camera_profile_id": "CAM_Orbital",
		"presentation_profile_id": "PRS_Orbital",
		"materials_path": MATERIALS_PATH,
		"archetypes_path": ARCHETYPES_PATH,
		"level_path": "res://data/levels/orbital/first_orbit_v2.level.json",
		"world_scene_path": "res://scenes/worlds/orbital_first_orbit.tscn",
	},
}

signal session_faulted(code: String, message: String)
signal launch_rejected(message: String)
## Published exactly once per configured launch, on the first confirmed
## terminal frame. The save layer is only allowed to write after this.
signal level_resolved(summary: Dictionary)

@onready var _session: Node = $Session
@onready var _body_views: Node3D = $BodyViews
@onready var _world_host: Node3D = $WorldHost
@onready var _capture_driver: Node = $CaptureDriver
@onready var _camera_director: Node3D = $CameraDirector
@onready var _slingshot: Node3D = $Slingshot
@onready var _hud: CanvasLayer = $Hud

var current_frame: Dictionary = {}
var current_request: Dictionary = {}
var loaded_document_paths: Array = []
var configure_calls := 0
var consume_calls := 0
var restart_calls := 0
var session_configured := false
var last_error := ""
var resolution_count := 0
var last_resolution: Dictionary = {}

var _advancing := false
var _paused := false
var _resolved := false
var _queue_size := 0
var _reduced_motion := false
var _shake := true
var _trajectory_assist := false
var _bindings: Array = []

## Diagnostics published by the body view registry while a level is mounted.
var asset_diagnostics: Array[String] = []


static func registry_key(world_id: String, level_id: String) -> String:
	return "%s/%s" % [world_id, level_id]


static func make_launch_request(world_id: String, level_id: String) -> Dictionary:
	var key := registry_key(world_id, level_id)
	if not LEVEL_REGISTRY.has(key):
		return {
			"ok": false,
			"message": "level is not registered in the product campaign: %s" % key,
		}
	return {"ok": true, "request": (LEVEL_REGISTRY[key] as Dictionary).duplicate(true)}


static func validate_launch_request(request: Variant) -> String:
	if not request is Dictionary:
		return "launch request must be an object"
	var value := request as Dictionary
	for key: Variant in value:
		if typeof(key) != TYPE_STRING or not REQUEST_KEYS.has(str(key)):
			return "launch request has unknown key: %s" % key
	for key: String in REQUEST_KEYS:
		if not value.has(key):
			return "launch request has missing key: %s" % key
	var key := registry_key(str(value.world_id), str(value.level_id))
	if not LEVEL_REGISTRY.has(key):
		return "level is not registered in the product campaign: %s" % key
	var registered := LEVEL_REGISTRY[key] as Dictionary
	for field: String in REQUEST_KEYS:
		if typeof(value[field]) != TYPE_STRING or str(value[field]) != str(registered[field]):
			return "launch request field is not the registered value: %s" % field
	return ""


func _ready() -> void:
	process_physics_priority = 100
	_session.set_physics_process(false)
	_session.gameplay_fault.connect(_on_gameplay_fault)
	# Production listener for the registry diagnostic. Without one, an asset the
	# registry refuses mid-flight left no trace outside the smokes.
	_body_views.asset_resolution_failed.connect(_on_asset_resolution_failed)


func configure_launch(request: Variant) -> bool:
	last_error = validate_launch_request(request)
	if not last_error.is_empty():
		launch_rejected.emit(last_error)
		return false
	var accepted := (request as Dictionary).duplicate(true)
	var documents := _read_documents(accepted)
	if not bool(documents.get("ok", false)):
		last_error = str(documents.get("message", "content could not be read"))
		launch_rejected.emit(last_error)
		return false
	var catalog := ASSET_CATALOG.load_catalog()
	if not bool(catalog.get("ok", false)):
		last_error = str(catalog.get("message", "asset catalog could not be loaded"))
		launch_rejected.emit(last_error)
		return false
	var level := CONTENT_FILE_LOADER.load_json(str(accepted.level_path))
	if not bool(level.get("ok", false)):
		last_error = str(level.get("message", "level document could not be parsed"))
		launch_rejected.emit(last_error)
		return false
	var archetypes := CONTENT_FILE_LOADER.load_json(str(accepted.archetypes_path))
	if not bool(archetypes.get("ok", false)):
		last_error = str(archetypes.get("message", "archetype document could not be parsed"))
		launch_rejected.emit(last_error)
		return false
	var unregistered: Array[String] = ASSET_CATALOG.missing_visual_asset_ids(
		catalog.document as Dictionary, level.document as Dictionary)
	if not unregistered.is_empty():
		last_error = "level references unregistered visuals: %s" % ", ".join(unregistered)
		launch_rejected.emit(last_error)
		return false
	# The other two catalog validations used to run only in the content gate, so
	# a required asset that disappeared after the build reached the player as a
	# provisional primitive instead of a refused launch. Product loading is
	# fail-closed at runtime too.
	var unavailable: Array[String] = ASSET_CATALOG.unavailable_required_asset_ids(
		catalog.document as Dictionary)
	if not unavailable.is_empty():
		last_error = "required product assets are unavailable: %s" % ", ".join(unavailable)
		launch_rejected.emit(last_error)
		return false
	var catalog_document := catalog.document as Dictionary
	var unregistered_presentation: Array[String] = ASSET_CATALOG.missing_presentation_asset_ids(
		catalog_document, archetypes.document as Dictionary)
	if not unregistered_presentation.is_empty():
		last_error = "archetypes reference unregistered presentation assets: %s" % [
			", ".join(unregistered_presentation)]
		launch_rejected.emit(last_error)
		return false

	release_level()
	if not _body_views.configure(catalog.document as Dictionary):
		last_error = "body view registry rejected the product asset catalog"
		release_level()
		launch_rejected.emit(last_error)
		return false
	if not _mount_world_scene(str(accepted.world_scene_path)):
		release_level()
		launch_rejected.emit(last_error)
		return false
	if not _camera_director.configure(
			str(accepted.camera_profile_id), level.document as Dictionary):
		last_error = _camera_director.last_error
		release_level()
		launch_rejected.emit(last_error)
		return false
	if not _slingshot.configure(
			level.document as Dictionary,
			archetypes.document as Dictionary,
			catalog.document as Dictionary):
		last_error = _slingshot.last_error
		release_level()
		launch_rejected.emit(last_error)
		return false
	_slingshot.bind_session(_session)
	_slingshot.bind_camera(_camera_director.active_rig())
	_slingshot.bind_camera_director(_camera_director)
	var text_catalog := PRODUCT_TEXT_CATALOG.load_catalog(UI_CATALOG_PATH)
	if not bool(text_catalog.get("ok", false)):
		last_error = "the closed pt-BR product catalog could not be loaded"
		release_level()
		launch_rejected.emit(last_error)
		return false
	var materials := CONTENT_FILE_LOADER.load_json(str(accepted.materials_path))
	if not bool(materials.get("ok", false)):
		last_error = str(materials.get("message", "materials could not be parsed"))
		release_level()
		launch_rejected.emit(last_error)
		return false
	if not _hud.configure(
			(text_catalog.document as Dictionary).messages as Dictionary,
			level.document as Dictionary,
			archetypes.document as Dictionary,
			materials.document as Dictionary):
		last_error = "the product HUD rejected the registered content bundle"
		release_level()
		launch_rejected.emit(last_error)
		return false
	_hud.set_camera(_camera_director.active_rig())
	_hud.set_bindings(_bindings)
	_hud.set_trajectory_assist(_trajectory_assist)
	_hud.set_reduced_motion(_reduced_motion)
	configure_calls += 1
	if not _session.configure_session(
			str(documents.materials), str(documents.archetypes), str(documents.level)):
		last_error = "the kernel rejected the registered content bundle"
		release_level()
		launch_rejected.emit(last_error)
		return false
	loaded_document_paths = documents.paths as Array
	current_request = accepted
	_queue_size = ((level.document as Dictionary).get("bird_queue", []) as Array).size()
	session_configured = true
	_paused = false
	_resolved = false
	_set_advancing(true)
	return true


## Restarting rewinds the kernel, so no presentation state from the interrupted
## attempt may survive it: an open gesture would keep the camera frozen and
## refuse every new grab, and a surviving body view would interpolate from the
## previous debris back into the reinstalled layout.
func restart_level() -> bool:
	if not session_configured or not _session.restart_level():
		return false
	restart_calls += 1
	_resolved = false
	_paused = false
	_slingshot.reset_gesture()
	_camera_director.unlock()
	_body_views.release()
	_set_advancing(true)
	return true


func release_level() -> void:
	session_configured = false
	asset_diagnostics = []
	_advancing = false
	_paused = false
	_resolved = false
	_queue_size = 0
	_session.set_physics_process(false)
	current_frame = {}
	current_request = {}
	loaded_document_paths = []
	_body_views.release()
	_slingshot.release()
	_camera_director.release()
	if _hud != null:
		_hud.release()
	for child: Node in _world_host.get_children():
		_world_host.remove_child(child)
		child.queue_free()


func body_views() -> Node3D:
	return _body_views


func camera_director() -> Node3D:
	return _camera_director


func slingshot() -> Node3D:
	return _slingshot


func hud() -> CanvasLayer:
	return _hud


## Pause is an overlay, never an authoritative simulation state: it only stops
## the presentation from advancing the kernel. No command is queued and no
## frame is consumed while it is open.
func set_paused(value: bool) -> void:
	if not session_configured or _paused == value:
		return
	_paused = value
	_set_advancing(not value and not _resolved)


func is_paused() -> bool:
	return _paused


func is_advancing() -> bool:
	return _advancing


func is_resolved() -> bool:
	return _resolved


## Applies the persisted accessibility options to the presentation. None of
## them reaches the kernel, so none of them can change score or trajectory.
func apply_accessibility(settings: Dictionary) -> void:
	set_reduced_motion(bool(settings.get("reduced_motion", false)))
	set_shake(bool(settings.get("shake", true)))
	set_trajectory_assist(bool(settings.get("trajectory_assist", false)))


func set_reduced_motion(enabled: bool) -> void:
	_reduced_motion = enabled
	_camera_director.set_reduced_motion(enabled)
	if _hud != null:
		_hud.set_reduced_motion(enabled)


func reduced_motion() -> bool:
	return _reduced_motion


func set_shake(enabled: bool) -> void:
	_shake = enabled
	_camera_director.set_shake_enabled(enabled)


func shake_enabled() -> bool:
	return _shake


func set_trajectory_assist(enabled: bool) -> void:
	_trajectory_assist = enabled
	if _hud != null:
		_hud.set_trajectory_assist(enabled)


func trajectory_assist() -> bool:
	return _trajectory_assist


func set_bindings(bindings: Array) -> void:
	_bindings = bindings.duplicate(true)
	if _hud != null:
		_hud.set_bindings(_bindings)


## Translates a semantic intent into presentation state. Gameplay authority
## stays in the kernel: the slingshot only forwards the camera basis, the
## metric pull and the release, and the camera only recomposes.
func handle_intent(intent: RefCounted) -> bool:
	if intent == null or not intent is INPUT_INTENT or not session_configured:
		return false
	var kind: StringName = intent.kind
	if kind == INPUT_INTENT.KIND_BEGIN_GRAB:
		var grab: StringName = _slingshot.handle_begin_grab(
			intent.payload.get("position", Vector2.ZERO) as Vector2)
		return grab != &"ignored"
	if kind == INPUT_INTENT.KIND_UPDATE_PULL:
		return _slingshot.handle_update_pull(
			intent.payload.get("position", Vector2.ZERO) as Vector2)
	if kind == INPUT_INTENT.KIND_RELEASE:
		return _slingshot.handle_release() != &"ignored"
	if kind == INPUT_INTENT.KIND_ACTIVATE_ABILITY:
		return _slingshot.handle_activate_ability() != &"ignored"
	if kind == INPUT_INTENT.KIND_BACK:
		return _slingshot.handle_cancel()
	if kind == INPUT_INTENT.KIND_RESTART:
		return restart_level()
	return _camera_director.handle_intent(intent)


func _physics_process(_delta: float) -> void:
	if not session_configured or not _advancing:
		return
	current_frame = _session.consume_frame()
	consume_calls += 1
	_body_views.apply_frame(current_frame)
	_slingshot.observe_frame(current_frame)
	_camera_director.observe_frame(current_frame)
	_hud.apply_frame(current_frame)
	_capture_driver.observe_frame(current_frame)
	_publish_resolution(current_frame)


func _set_advancing(value: bool) -> void:
	_advancing = value and session_configured
	_session.set_physics_process(_advancing)


## Publishes the terminal frame exactly once. Everything downstream, including
## the save, depends on this single confirmation.
func _publish_resolution(frame: Dictionary) -> void:
	if _resolved or str(frame.get("phase", "")) != TERMINAL_PHASE:
		return
	var outcome := str(frame.get("outcome", "none"))
	if not TERMINAL_OUTCOMES.has(outcome):
		return
	var remaining := (frame.get("bird_queue", []) as Array).size()
	_resolved = true
	_set_advancing(false)
	resolution_count += 1
	last_resolution = {
		"world_id": str(current_request.get("world_id", "")),
		"level_id": str(current_request.get("level_id", "")),
		"outcome": outcome,
		"score": int(frame.get("score", 0)),
		"stars": int(frame.get("stars", 0)),
		"birds_used": maxi(0, _queue_size - remaining),
	}
	level_resolved.emit(last_resolution.duplicate(true))


func _read_documents(request: Dictionary) -> Dictionary:
	var documents := {"ok": true, "paths": []}
	for spec: Array in [
		["materials", str(request.materials_path)],
		["archetypes", str(request.archetypes_path)],
		["level", str(request.level_path)],
	]:
		var result := CONTENT_FILE_LOADER.read_text(str(spec[1]))
		if not bool(result.get("ok", false)):
			return result
		documents[str(spec[0])] = str(result.get("text", ""))
		(documents.paths as Array).append(str(spec[1]))
	return documents


func _mount_world_scene(scene_path: String) -> bool:
	if not ResourceLoader.exists(scene_path, "PackedScene"):
		last_error = "registered world scene is unavailable: %s" % scene_path
		return false
	var packed := ResourceLoader.load(scene_path, "PackedScene") as PackedScene
	var root: Node = packed.instantiate() if packed != null else null
	var instance := root as Node3D
	if instance == null:
		# release_level() only frees what reached the host, so the failed cast has
		# to be freed here or it survives the whole session.
		if root != null:
			root.free()
		last_error = "registered world scene root is not a Node3D: %s" % scene_path
		return false
	_world_host.add_child(instance)
	return true


func _on_asset_resolution_failed(_asset_id: String, message: String) -> void:
	if not asset_diagnostics.has(message):
		asset_diagnostics.append(message)
	last_error = message


func _on_gameplay_fault(code: String, message: String) -> void:
	last_error = "%s: %s" % [code, message]
	session_faulted.emit(code, message)
