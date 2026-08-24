extends SceneTree

const CONTENT_FILE_LOADER := preload("res://scripts/data/content_file_loader.gd")
const SUCCESS_MARKER := "NINHO_IMPORT_COMPLETENESS_OK"
const IMPORT_ROOTS := [
	"res://assets/vertical_slice",
	"res://assets/audio/generated",
]
const DATA_ROOTS := [
	"res://data",
]
const REQUIRED_ENTRYPOINTS := [
	"res://scripts/game/body_view_registry.gd",
	"res://scenes/vertical_slice.tscn",
	"res://materials/brick.tres",
	"res://materials/glass.tres",
	"res://materials/pine.tres",
	"res://scripts/data/asset_catalog.gd",
	"res://scripts/game/body_view_registry_v2.gd",
	"res://scripts/game/gameplay_session_controller.gd",
	"res://scripts/game/product_v2_capture_driver.gd",
	"res://scenes/gameplay/gameplay_session.tscn",
	"res://scenes/worlds/earth_farm.tscn",
	"res://scenes/worlds/orbital_first_orbit.tscn",
	"res://scripts/camera/camera_profile_catalog.gd",
	"res://scripts/camera/camera_director.gd",
	"res://scripts/camera/terrestrial_camera_rig.gd",
	"res://scripts/camera/orbital_camera_rig.gd",
	"res://scripts/camera/carousel_camera_rig.gd",
	"res://scripts/game/slingshot_controller.gd",
	"res://scripts/game/launch_device_view.gd",
]
const IMPORTED_EXTENSIONS := ["glb", "png", "wav"]


func _initialize() -> void:
	var resources: Array[String] = []
	for root_path: String in IMPORT_ROOTS:
		if not _collect_files(root_path, IMPORTED_EXTENSIONS, resources):
			return
	resources.sort()
	if resources.is_empty():
		_fail("import manifest resolved no resources")
		return
	for path: String in resources:
		if not ResourceLoader.exists(path) or ResourceLoader.load(path) == null:
			_fail("imported resource is unavailable: %s" % path)
			return
	for path: String in REQUIRED_ENTRYPOINTS:
		if not ResourceLoader.exists(path) or ResourceLoader.load(path) == null:
			_fail("required preload entrypoint is unavailable: %s" % path)
			return
	var documents: Array[String] = []
	for root_path: String in DATA_ROOTS:
		if not _collect_files(root_path, ["json"], documents):
			return
	documents.sort()
	if documents.is_empty():
		_fail("data manifest resolved no JSON documents")
		return
	for path: String in documents:
		var load_result: Dictionary = CONTENT_FILE_LOADER.load_json(path)
		if not bool(load_result.get("ok", false)):
			_fail(str(load_result.get("message", "JSON document could not be loaded: %s" % path)))
			return
	print("%s resources=%d entrypoints=%d documents=%d" % [
		SUCCESS_MARKER, resources.size(), REQUIRED_ENTRYPOINTS.size(), documents.size()])
	quit(0)


func _collect_files(
		directory_path: String,
		extensions: Array,
		resources: Array[String]) -> bool:
	var directory := DirAccess.open(directory_path)
	if directory == null:
		_fail("scan root is unavailable: %s" % directory_path)
		return false
	directory.list_dir_begin()
	while true:
		var entry := directory.get_next()
		if entry.is_empty():
			break
		if entry == "." or entry == "..":
			continue
		var path := directory_path.path_join(entry)
		if directory.current_is_dir():
			if not _collect_files(path, extensions, resources):
				directory.list_dir_end()
				return false
		elif entry.get_extension().to_lower() in extensions:
			resources.append(path)
	directory.list_dir_end()
	return true


func _fail(message: String) -> void:
	push_error("import completeness smoke: %s" % message)
	quit(1)
