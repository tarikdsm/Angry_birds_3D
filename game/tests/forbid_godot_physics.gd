extends SceneTree

const SCENE_PATH := "res://scenes/vertical_slice.tscn"
const SUCCESS_MARKER := "FORBID_GODOT_PHYSICS_OK"
const PRODUCTION_ROOTS := ["res://scenes", "res://scripts/game", "res://scripts/camera", "res://scripts/ui"]
const FORBIDDEN_CLASSES := [
	"Rigid" + "Body3D",
	"Static" + "Body3D",
	"Character" + "Body3D",
	"Area" + "3D",
	"Collision" + "Shape3D",
	"Collision" + "Object3D",
]

var _scene: Node


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	for root_path: String in PRODUCTION_ROOTS:
		var violation := _scan_directory(root_path)
		if not violation.is_empty():
			_fail(violation)
			return

	var packed := load(SCENE_PATH) as PackedScene
	if packed == null:
		_fail("playable scene could not be loaded for runtime scan")
		return
	_scene = packed.instantiate()
	root.add_child(_scene)
	await physics_frame
	var runtime_violation := _scan_runtime(_scene)
	if not runtime_violation.is_empty():
		_fail(runtime_violation)
		return

	print(SUCCESS_MARKER)
	_finish(0)


func _scan_directory(path: String) -> String:
	var directory := DirAccess.open(path)
	if directory == null:
		return "required production directory is missing: %s" % path
	for entry: String in directory.get_files():
		if not entry.ends_with(".gd") and not entry.ends_with(".tscn"):
			continue
		var file_path := path.path_join(entry)
		var source := FileAccess.get_file_as_string(file_path)
		for forbidden: String in FORBIDDEN_CLASSES:
			if source.contains(forbidden):
				return "forbidden Godot physics class %s in %s" % [forbidden, file_path]
	for child: String in directory.get_directories():
		var violation := _scan_directory(path.path_join(child))
		if not violation.is_empty():
			return violation
	return ""


func _scan_runtime(node: Node) -> String:
	for forbidden: String in FORBIDDEN_CLASSES:
		if node.is_class(forbidden):
			return "forbidden runtime node %s at %s" % [forbidden, node.get_path()]
	for child: Node in node.get_children():
		var violation := _scan_runtime(child)
		if not violation.is_empty():
			return violation
	return ""


func _fail(message: String) -> void:
	push_error("Godot physics scanner: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	if is_instance_valid(_scene):
		root.remove_child(_scene)
		_scene.free()
	quit(exit_code)
