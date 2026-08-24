extends Node3D

const CONTENT_FILE_LOADER := preload("res://scripts/data/content_file_loader.gd")
const ASSET_CATALOG := preload("res://scripts/data/asset_catalog.gd")
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

@onready var _session: Node = $Session
@onready var _body_views: Node3D = $BodyViews
@onready var _world_host: Node3D = $WorldHost
@onready var _capture_driver: Node = $CaptureDriver

var current_frame: Dictionary = {}
var current_request: Dictionary = {}
var loaded_document_paths: Array = []
var configure_calls := 0
var consume_calls := 0
var restart_calls := 0
var session_configured := false
var last_error := ""


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
	var unregistered: Array[String] = ASSET_CATALOG.missing_visual_asset_ids(
		catalog.document as Dictionary, level.document as Dictionary)
	if not unregistered.is_empty():
		last_error = "level references unregistered visuals: %s" % ", ".join(unregistered)
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
	configure_calls += 1
	if not _session.configure_session(
			str(documents.materials), str(documents.archetypes), str(documents.level)):
		last_error = "the kernel rejected the registered content bundle"
		release_level()
		launch_rejected.emit(last_error)
		return false
	loaded_document_paths = documents.paths as Array
	current_request = accepted
	session_configured = true
	_session.set_physics_process(true)
	return true


func restart_level() -> bool:
	if not session_configured or not _session.restart_level():
		return false
	restart_calls += 1
	return true


func release_level() -> void:
	session_configured = false
	_session.set_physics_process(false)
	current_frame = {}
	current_request = {}
	loaded_document_paths = []
	_body_views.release()
	for child: Node in _world_host.get_children():
		_world_host.remove_child(child)
		child.queue_free()


func body_views() -> Node3D:
	return _body_views


func _physics_process(_delta: float) -> void:
	if not session_configured:
		return
	current_frame = _session.consume_frame()
	consume_calls += 1
	_body_views.apply_frame(current_frame)
	_capture_driver.observe_frame(current_frame)


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
	var instance: Node3D = null
	if packed != null:
		instance = packed.instantiate() as Node3D
	if instance == null:
		last_error = "registered world scene root is not a Node3D: %s" % scene_path
		return false
	_world_host.add_child(instance)
	return true


func _on_gameplay_fault(code: String, message: String) -> void:
	last_error = "%s: %s" % [code, message]
	session_faulted.emit(code, message)
