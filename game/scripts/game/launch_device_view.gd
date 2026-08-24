extends Node3D

## Presentation of the launch device: forks, sockets and straps.
##
## The view animates only. It never decides a launch, never owns a simulated
## body and never publishes a velocity: it reads the metric displacement the
## kernel already accepted and moves the pouch and the straps to match.

const HIDDEN_PREFIXES := ["COL_", "FRAG_", "RIG_"]
const SOCKET_PREFIX := "SOCKET_"
const PROVISIONAL_ALBEDO := Color("6b4d33")
const STRAP_ALBEDO := Color("3d2b1f")
const STRAP_THICKNESS_M := 0.07
const FORK_SPAN_M := 0.62
const FORK_HEIGHT_M := 1.15
const FORK_THICKNESS_M := 0.14

var _configured := false
var _authored := false
var _rest := Vector3.ZERO
var _up := Vector3.UP
var _horizontal := Vector3.RIGHT
var _asset_id := ""
var _device: Node3D
var _pouch: Node3D
var _straps: Array[Node3D] = []
var _socket_offsets: Array[Vector3] = []
var _displacement := Vector3.ZERO


func configure(level_document: Dictionary, catalog_document: Dictionary) -> bool:
	release()
	var slingshot := level_document.get("slingshot", {}) as Dictionary
	if not slingshot.has("rest_position_m") or not slingshot.has("asset_id"):
		return false
	_rest = _vector_of(slingshot.rest_position_m)
	_asset_id = str(slingshot.asset_id)
	var index: Dictionary = {}
	if not catalog_document.is_empty():
		index = asset_index(catalog_document)
	_device = _instantiate_device(index)
	_device.name = &"Device"
	add_child(_device)
	_device.global_position = _rest
	_socket_offsets = _resolve_socket_offsets()
	_pouch = Node3D.new()
	_pouch.name = &"Pouch"
	var pouch_mesh := MeshInstance3D.new()
	pouch_mesh.name = &"PouchVisual"
	var pouch_shape := BoxMesh.new()
	pouch_shape.size = Vector3(0.32, 0.22, 0.1)
	pouch_shape.material = _material(STRAP_ALBEDO)
	pouch_mesh.mesh = pouch_shape
	_pouch.add_child(pouch_mesh)
	add_child(_pouch)
	for index_of_strap: int in _socket_offsets.size():
		var strap := MeshInstance3D.new()
		strap.name = StringName("Strap%d" % index_of_strap)
		var strap_shape := BoxMesh.new()
		strap_shape.size = Vector3(STRAP_THICKNESS_M, STRAP_THICKNESS_M, 1.0)
		strap_shape.material = _material(STRAP_ALBEDO)
		strap.mesh = strap_shape
		add_child(strap)
		_straps.append(strap)
	_configured = true
	apply_pull(Vector3.ZERO)
	return true


func release() -> void:
	for node: Node in [_pouch, _device]:
		if is_instance_valid(node):
			remove_child(node)
			node.queue_free()
	for strap: Node3D in _straps:
		if is_instance_valid(strap):
			remove_child(strap)
			strap.queue_free()
	_straps = []
	_socket_offsets = []
	_pouch = null
	_device = null
	_configured = false
	_authored = false
	_displacement = Vector3.ZERO


func configured() -> bool:
	return _configured


func authored() -> bool:
	return _authored


func apply_plane(up: Vector3, horizontal: Vector3) -> void:
	if up.is_normalized():
		_up = up
	if horizontal.is_normalized():
		_horizontal = horizontal
	apply_pull(_displacement)


func apply_pull(displacement: Vector3) -> void:
	if not _configured:
		return
	_displacement = displacement
	_pouch.global_position = _rest + displacement
	for index: int in _straps.size():
		_orient_strap(_straps[index], _socket_position(index), _pouch.global_position)


func pouch_position() -> Vector3:
	if not _configured:
		return _rest
	return _pouch.global_position


func socket_positions() -> Array[Vector3]:
	var positions: Array[Vector3] = []
	for index: int in _socket_offsets.size():
		positions.append(_socket_position(index))
	return positions


func strap_count() -> int:
	return _straps.size()


func _socket_position(index: int) -> Vector3:
	var offset := _socket_offsets[index]
	return _rest + _horizontal * offset.x + _up * offset.y


func _orient_strap(strap: Node3D, from: Vector3, to: Vector3) -> void:
	var direction := to - from
	var length := direction.length()
	if length <= 0.001:
		strap.visible = false
		return
	strap.visible = true
	strap.global_position = (from + to) * 0.5
	var reference := _up
	if absf(direction.normalized().dot(reference)) > 0.999:
		reference = _horizontal
	strap.look_at(to, reference)
	strap.scale = Vector3(1.0, 1.0, length)


func _resolve_socket_offsets() -> Array[Vector3]:
	var offsets: Array[Vector3] = []
	for child: Node in _device.get_children():
		if not child is Node3D or not str(child.name).begins_with(SOCKET_PREFIX):
			continue
		var local := (child as Node3D).position
		offsets.append(Vector3(local.x, local.y, 0.0))
	if offsets.size() == 2:
		return offsets
	return [
		Vector3(-FORK_SPAN_M * 0.5, FORK_HEIGHT_M, 0.0),
		Vector3(FORK_SPAN_M * 0.5, FORK_HEIGHT_M, 0.0),
	]


func _instantiate_device(index: Dictionary) -> Node3D:
	var entry := index.get(_asset_id, {}) as Dictionary
	var resource_path := str(entry.get("resource_path", ""))
	if not resource_path.is_empty() and ResourceLoader.exists(resource_path, "PackedScene"):
		var packed := ResourceLoader.load(resource_path, "PackedScene") as PackedScene
		if packed != null:
			var instance := packed.instantiate() as Node3D
			if instance != null:
				_hide_technical_children(instance)
				_authored = true
				return instance
	return _provisional_device()


func _provisional_device() -> Node3D:
	var root_node := Node3D.new()
	var base := MeshInstance3D.new()
	base.name = &"Base"
	var base_mesh := BoxMesh.new()
	base_mesh.size = Vector3(FORK_SPAN_M, FORK_HEIGHT_M * 0.55, FORK_THICKNESS_M)
	base_mesh.material = _material(PROVISIONAL_ALBEDO)
	base.mesh = base_mesh
	base.position = Vector3(0.0, FORK_HEIGHT_M * 0.275, 0.0)
	root_node.add_child(base)
	for side: int in 2:
		var arm := MeshInstance3D.new()
		arm.name = StringName("Fork%d" % side)
		var arm_mesh := BoxMesh.new()
		arm_mesh.size = Vector3(FORK_THICKNESS_M, FORK_HEIGHT_M * 0.7, FORK_THICKNESS_M)
		arm_mesh.material = _material(PROVISIONAL_ALBEDO)
		arm.mesh = arm_mesh
		arm.position = Vector3(
			(-0.5 + float(side)) * FORK_SPAN_M, FORK_HEIGHT_M * 0.8, 0.0)
		root_node.add_child(arm)
		var socket := Node3D.new()
		socket.name = StringName("%sFork%d" % [SOCKET_PREFIX, side])
		socket.position = Vector3(
			(-0.5 + float(side)) * FORK_SPAN_M, FORK_HEIGHT_M, 0.0)
		root_node.add_child(socket)
	return root_node


func _hide_technical_children(node: Node) -> void:
	for child: Node in node.get_children():
		var child_name := str(child.name)
		for prefix: String in HIDDEN_PREFIXES:
			if child is Node3D and child_name.begins_with(prefix):
				(child as Node3D).visible = false
		_hide_technical_children(child)


func _material(albedo: Color) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = albedo
	material.roughness = 0.78
	material.metallic = 0.02
	return material


static func asset_index(catalog_document: Dictionary) -> Dictionary:
	var index := {}
	for asset: Variant in catalog_document.get("assets", []):
		if asset is Dictionary:
			index[str((asset as Dictionary).get("id", ""))] = asset as Dictionary
	return index


static func _vector_of(value: Variant) -> Vector3:
	if value is Vector3:
		return value as Vector3
	if not value is Array or (value as Array).size() != 3:
		return Vector3.ZERO
	var values := value as Array
	return Vector3(float(values[0]), float(values[1]), float(values[2]))
