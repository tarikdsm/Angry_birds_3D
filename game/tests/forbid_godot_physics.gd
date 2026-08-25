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
const ASSET_ROOT := "res://assets"
const ASSET_CATALOG_PATH := "res://data/assets/product_v2.assets.json"
const SCENE_ASSET_EXTENSIONS := [
	"glb", "gltf", "fbx", "dae", "obj", "blend", "escn", "scn", "tscn",
]
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

	var asset_violation := _scan_asset_resources()
	if not asset_violation.is_empty():
		_fail(asset_violation)
		return

	for scene_path: String in RUNTIME_SCENE_PATHS:
		var runtime_violation := await _scan_runtime_scene(scene_path)
		if not runtime_violation.is_empty():
			_fail(runtime_violation)
			return

	print(SUCCESS_MARKER)
	_finish(0)


## Authored art reaches gameplay as an instantiated PackedScene, so it is a live
## surface of the rule that no Godot physics node may authorise gameplay. A .glb
## imported with a collision suffix (-col, -colonly, -convcol) or carrying an
## authored trigger volume would otherwise enter the product through the asset
## catalog with every gate still green.
func _scan_asset_resources() -> String:
	# The scanner has to be proven able to see a violation, or a green gate would
	# only mean that nothing was scanned.
	var probe := Node3D.new()
	probe.name = &"AuthoredProbe"
	var planted: Node = ClassDB.instantiate("Area" + "3D")
	planted.name = &"AuthoredCollider"
	probe.add_child(planted)
	var detected := _scan_detached(probe, "probe")
	probe.free()
	if detected.is_empty():
		return "the authored asset scanner failed to detect a planted physics node"

	var paths: Array[String] = []
	var collect_error := _collect_scene_assets(ASSET_ROOT, paths)
	if not collect_error.is_empty():
		return collect_error
	for registered: String in _catalog_scene_paths():
		if not paths.has(registered):
			paths.append(registered)
	paths.sort()
	var scanned := 0
	for path: String in paths:
		if not ResourceLoader.exists(path, "PackedScene"):
			continue
		var packed := ResourceLoader.load(path, "PackedScene") as PackedScene
		if packed == null:
			return "authored asset could not be loaded as a scene: %s" % path
		var instance := packed.instantiate()
		if instance == null:
			return "authored asset could not be instantiated: %s" % path
		var violation := _scan_detached(instance, path)
		instance.free()
		scanned += 1
		if not violation.is_empty():
			return violation
	if scanned == 0:
		return "the authored asset scan resolved no instantiable scene under %s" % ASSET_ROOT
	return ""


func _catalog_scene_paths() -> Array[String]:
	var paths: Array[String] = []
	var parsed: Variant = JSON.parse_string(
		FileAccess.get_file_as_string(ASSET_CATALOG_PATH))
	if not parsed is Dictionary:
		return paths
	for asset: Variant in (parsed as Dictionary).get("assets", []):
		if not asset is Dictionary:
			continue
		var resource_path := str((asset as Dictionary).get("resource_path", ""))
		if not resource_path.is_empty() and not paths.has(resource_path):
			paths.append(resource_path)
	return paths


func _collect_scene_assets(directory_path: String, paths: Array[String]) -> String:
	var directory := DirAccess.open(directory_path)
	if directory == null:
		return "authored asset root is unavailable: %s" % directory_path
	for entry: String in directory.get_files():
		if entry.get_extension().to_lower() in SCENE_ASSET_EXTENSIONS:
			paths.append(directory_path.path_join(entry))
	for child: String in directory.get_directories():
		var error := _collect_scene_assets(directory_path.path_join(child), paths)
		if not error.is_empty():
			return error
	return ""


## The authored instance is never added to the tree, so the diagnostic path is
## built from the resource path and the node names instead of get_path().
func _scan_detached(node: Node, trail: String) -> String:
	var here := "%s/%s" % [trail, node.name]
	for forbidden: String in FORBIDDEN_RUNTIME_BASE_CLASSES:
		if node.is_class(forbidden):
			return "forbidden authored node %s at %s" % [forbidden, here]
	for child: Node in node.get_children():
		var violation := _scan_detached(child, here)
		if not violation.is_empty():
			return violation
	return ""


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
