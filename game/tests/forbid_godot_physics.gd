extends SceneTree

const SUCCESS_MARKER := "FORBID_GODOT_PHYSICS_OK"
const PRODUCTION_ROOTS := [
	"res://scenes", "res://scripts",
]
const RUNTIME_SCENE_PATHS := [
	"res://scenes/vertical_slice.tscn",
	"res://scenes/physics_spike.tscn",
	"res://scenes/gameplay/gameplay_session.tscn",
	"res://scenes/worlds/earth_farm.tscn",
	"res://scenes/worlds/orbital_first_orbit.tscn",
	"res://scenes/frontend/pause_menu.tscn",
	"res://scenes/frontend/result_screen.tscn",
]
const RUNTIME_SCAN_PHYSICS_FRAMES := 3
const FORBIDDEN_SOURCE_FRAGMENTS := [
	"Body" + "3D",
	"Joint" + "3D",
	"Cast" + "3D",
	"Area" + "3D",
	"Physical" + "Bone3D",
	"Collision" + "Shape3D",
	"Collision" + "Object3D",
	"Collision" + "Polygon3D",
	"Physics" + "Server3D",
	"direct" + "_space_state",
]
const FORBIDDEN_RUNTIME_BASE_CLASSES := [
	"Collision" + "Object3D",
	"Collision" + "Shape3D",
	"Collision" + "Polygon3D",
	"Joint" + "3D",
	"Ray" + "Cast3D",
	"Shape" + "Cast3D",
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

	for scene_path: String in RUNTIME_SCENE_PATHS:
		var runtime_violation := await _scan_runtime_scene(scene_path)
		if not runtime_violation.is_empty():
			_fail(runtime_violation)
			return

	print(SUCCESS_MARKER)
	_finish(0)


func _scan_runtime_scene(scene_path: String) -> String:
	var packed := load(scene_path) as PackedScene
	if packed == null:
		return "production scene could not be loaded for runtime scan: %s" % scene_path
	_scene = packed.instantiate()
	root.add_child(_scene)
	var runtime_violation := _scan_runtime(_scene)
	if not runtime_violation.is_empty():
		return runtime_violation
	for _frame in range(RUNTIME_SCAN_PHYSICS_FRAMES):
		await physics_frame
		runtime_violation = _scan_runtime(_scene)
		if not runtime_violation.is_empty():
			return runtime_violation

	_release_scene()
	return ""


func _scan_directory(path: String) -> String:
	var directory := DirAccess.open(path)
	if directory == null:
		return "required production directory is missing: %s" % path
	for entry: String in directory.get_files():
		# Scan only source formats guaranteed to be textual. Godot .res/.scn files
		# may be binary; .tres resources do not create physics nodes by themselves.
		# Physics nodes loaded by production scenes are covered by the runtime scan.
		if not entry.ends_with(".gd") and not entry.ends_with(".tscn"):
			continue
		var file_path := path.path_join(entry)
		var source := FileAccess.get_file_as_string(file_path)
		for forbidden: String in FORBIDDEN_SOURCE_FRAGMENTS:
			if source.contains(forbidden):
				return "forbidden Godot physics symbol %s in %s" % [forbidden, file_path]
	for child: String in directory.get_directories():
		var violation := _scan_directory(path.path_join(child))
		if not violation.is_empty():
			return violation
	return ""


func _scan_runtime(node: Node) -> String:
	for forbidden: String in FORBIDDEN_RUNTIME_BASE_CLASSES:
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
	_release_scene()
	quit(exit_code)


func _release_scene() -> void:
	if is_instance_valid(_scene):
		root.remove_child(_scene)
		_scene.free()
	_scene = null
