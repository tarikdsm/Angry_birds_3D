extends Node3D

const INTERPOLATION_SPEED := 18.0
const PROVISIONAL_ALBEDO := Color("8ea4b8")
const PROVISIONAL_HIGHLIGHT := Color("d9e4ee")
const HIDDEN_PREFIXES := ["COL_", "FRAG_", "SOCKET_", "RIG_"]
const HIDDEN_SUFFIXES := ["_LOD1"]

signal asset_resolution_failed(asset_id: String, message: String)

var _entries: Dictionary = {}
var _scenes: Dictionary = {}
var _views: Dictionary = {}
var _targets: Dictionary = {}
var _authored: Dictionary = {}
var _missing_asset_ids: Dictionary = {}
var _diagnostics: Array[String] = []
var _configured := false


func configure(catalog_document: Dictionary) -> bool:
	release()
	_entries.clear()
	_scenes.clear()
	for asset: Variant in catalog_document.get("assets", []):
		if not asset is Dictionary:
			continue
		var entry := asset as Dictionary
		_entries[str(entry.get("id", ""))] = entry.duplicate(true)
	_configured = not _entries.is_empty()
	return _configured


func release() -> void:
	for key: Variant in _views.keys():
		var view: Node3D = _views[key]
		if is_instance_valid(view):
			remove_child(view)
			view.queue_free()
	_views.clear()
	_targets.clear()
	_authored.clear()
	_missing_asset_ids.clear()
	_diagnostics.clear()


func apply_frame(frame: Dictionary) -> void:
	var alive := {}
	for snapshot: Dictionary in frame.get("snapshots", []):
		var key := "%d:%d" % [int(snapshot.get("entity_id", 0)), int(snapshot.get("part_id", 0))]
		alive[key] = true
		if not _views.has(key):
			_create_view(key, snapshot)
		if _views.has(key):
			_targets[key] = snapshot.get("transform", Transform3D.IDENTITY)
	for key: Variant in _views.keys():
		if alive.has(key):
			continue
		var stale: Node3D = _views[key]
		if is_instance_valid(stale):
			remove_child(stale)
			stale.queue_free()
		_views.erase(key)
		_targets.erase(key)
		_authored.erase(key)


func _process(delta: float) -> void:
	var weight := 1.0 - exp(-INTERPOLATION_SPEED * delta)
	for key: Variant in _views:
		var view: Node3D = _views[key]
		var target: Transform3D = _targets.get(key, view.transform)
		view.transform = view.transform.interpolate_with(target, weight)


func view_count() -> int:
	return _views.size()


func authored_view_count() -> int:
	return _authored.size()


func views() -> Array[Node3D]:
	var result: Array[Node3D] = []
	for key: Variant in _views:
		result.append(_views[key] as Node3D)
	return result


func missing_asset_ids() -> Array[String]:
	var result: Array[String] = []
	for asset_id: Variant in _missing_asset_ids.keys():
		result.append(str(asset_id))
	result.sort()
	return result


func diagnostics() -> Array[String]:
	return _diagnostics.duplicate()


func _create_view(key: String, snapshot: Dictionary) -> void:
	var asset_id := str(snapshot.get("visual_id", ""))
	if not _configured:
		_reject(asset_id, "the asset catalog was not configured before the first frame")
		return
	if not _entries.has(asset_id):
		_reject(asset_id, "visual asset is not registered in the product catalog")
		return
	var entry := _entries[asset_id] as Dictionary
	if bool(entry.get("presentation_only", false)):
		_reject(asset_id, "presentation-only assets can never back a simulated body")
		return
	var authored := _instantiate_authored(asset_id, entry)
	var view := authored if authored != null else _instantiate_provisional(snapshot)
	view.name = StringName("Body_%s" % key.replace(":", "_"))
	view.set_meta("asset_id", asset_id)
	view.set_meta("authored", authored != null)
	view.transform = snapshot.get("transform", Transform3D.IDENTITY)
	add_child(view)
	_views[key] = view
	_targets[key] = view.transform
	if authored != null:
		_authored[key] = true


func _instantiate_authored(asset_id: String, entry: Dictionary) -> Node3D:
	if not _scenes.has(asset_id):
		var resource_path := str(entry.get("resource_path", ""))
		var packed: PackedScene = null
		if ResourceLoader.exists(resource_path, "PackedScene"):
			packed = ResourceLoader.load(resource_path, "PackedScene") as PackedScene
		_scenes[asset_id] = packed
	var scene: PackedScene = _scenes[asset_id]
	if scene == null:
		return null
	var instance := scene.instantiate() as Node3D
	if instance == null:
		_reject(asset_id, "authored visual root is not a Node3D")
		return null
	var selected := _select_authored_node(instance, str(entry.get("node_path", "")))
	if selected == null:
		instance.free()
		_reject(asset_id, "authored visual does not expose its registered node path")
		return null
	_prepare_authored_view(selected)
	return selected


func _select_authored_node(instance: Node3D, node_path: String) -> Node3D:
	var segments := node_path.split("/")
	if segments.size() <= 1:
		return instance
	var relative := "/".join(segments.slice(1))
	var selected := instance.get_node_or_null(NodePath(relative)) as Node3D
	if selected == null:
		return null
	selected.get_parent().remove_child(selected)
	instance.free()
	return selected


func _prepare_authored_view(node: Node) -> void:
	for child: Node in node.get_children():
		var child_name := str(child.name)
		if child is Node3D and _is_hidden_authored_child(child_name):
			(child as Node3D).visible = false
		elif child is GeometryInstance3D:
			(child as GeometryInstance3D).cast_shadow = \
				GeometryInstance3D.SHADOW_CASTING_SETTING_ON
		_prepare_authored_view(child)


func _is_hidden_authored_child(child_name: String) -> bool:
	for prefix: String in HIDDEN_PREFIXES:
		if child_name.begins_with(prefix):
			return true
	for suffix: String in HIDDEN_SUFFIXES:
		if child_name.ends_with(suffix):
			return true
	return false


func _instantiate_provisional(snapshot: Dictionary) -> Node3D:
	var view := MeshInstance3D.new()
	view.mesh = _provisional_mesh(snapshot.get("shape", {}) as Dictionary)
	view.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
	return view


func _provisional_mesh(shape: Dictionary) -> Mesh:
	var material := StandardMaterial3D.new()
	material.albedo_color = PROVISIONAL_ALBEDO
	material.roughness = 0.82
	material.metallic = 0.04
	material.emission_enabled = true
	material.emission = PROVISIONAL_HIGHLIGHT
	material.emission_energy_multiplier = 0.12
	var shape_type := str(shape.get("type", "box"))
	if shape_type == "sphere":
		var sphere := SphereMesh.new()
		sphere.radius = maxf(0.01, float(shape.get("radius", 0.5)))
		sphere.height = sphere.radius * 2.0
		sphere.material = material
		return sphere
	if shape_type == "capsule":
		var capsule := CapsuleMesh.new()
		capsule.radius = maxf(0.01, float(shape.get("radius", 0.5)))
		capsule.height = maxf(capsule.radius * 2.0,
			float(shape.get("half_height", 0.5)) * 2.0 + capsule.radius * 2.0)
		capsule.material = material
		return capsule
	var half_extents := shape.get("half_extents", Vector3.ONE * 0.5) as Vector3
	var box := BoxMesh.new()
	box.size = Vector3(
		maxf(0.02, half_extents.x * 2.0),
		maxf(0.02, half_extents.y * 2.0),
		maxf(0.02, half_extents.z * 2.0))
	box.material = material
	return box


func _reject(asset_id: String, reason: String) -> void:
	if _missing_asset_ids.has(asset_id):
		return
	_missing_asset_ids[asset_id] = true
	var message := "%s: %s" % [asset_id, reason]
	_diagnostics.append(message)
	asset_resolution_failed.emit(asset_id, message)
