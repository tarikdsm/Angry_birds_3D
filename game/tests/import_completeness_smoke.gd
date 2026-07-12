extends SceneTree

const SUCCESS_MARKER := "NINHO_IMPORT_COMPLETENESS_OK"
const IMPORT_ROOTS := [
	"res://assets/vertical_slice",
	"res://assets/audio/generated",
]
const REQUIRED_ENTRYPOINTS := [
	"res://scripts/game/body_view_registry.gd",
	"res://scenes/vertical_slice.tscn",
	"res://materials/brick.tres",
	"res://materials/glass.tres",
	"res://materials/pine.tres",
]
const IMPORTED_EXTENSIONS := ["glb", "png", "wav"]


func _initialize() -> void:
	var resources: Array[String] = []
	for root_path: String in IMPORT_ROOTS:
		if not _collect_imported_resources(root_path, resources):
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
	print("%s resources=%d entrypoints=%d" % [
		SUCCESS_MARKER, resources.size(), REQUIRED_ENTRYPOINTS.size()])
	quit(0)


func _collect_imported_resources(directory_path: String, resources: Array[String]) -> bool:
	var directory := DirAccess.open(directory_path)
	if directory == null:
		_fail("import root is unavailable: %s" % directory_path)
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
			if not _collect_imported_resources(path, resources):
				directory.list_dir_end()
				return false
		elif entry.get_extension().to_lower() in IMPORTED_EXTENSIONS:
			resources.append(path)
	directory.list_dir_end()
	return true


func _fail(message: String) -> void:
	push_error("import completeness smoke: %s" % message)
	quit(1)
