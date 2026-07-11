extends Node3D

const INTERPOLATION_SPEED := 18.0

var _views: Dictionary = {}
var _targets: Dictionary = {}
var _preview_mesh: MeshInstance3D
var _impact_marker: MeshInstance3D


func _ready() -> void:
	_preview_mesh = MeshInstance3D.new()
	_preview_mesh.name = "KernelTrajectoryPreview"
	add_child(_preview_mesh)
	_impact_marker = MeshInstance3D.new()
	_impact_marker.name = "FirstImpactMarker"
	var marker_mesh := SphereMesh.new()
	marker_mesh.radius = 0.16
	marker_mesh.height = 0.32
	marker_mesh.radial_segments = 16
	marker_mesh.rings = 8
	marker_mesh.material = _material(Color("fff0a6"), Color("d7a441"), 2.2)
	_impact_marker.mesh = marker_mesh
	_impact_marker.visible = false
	add_child(_impact_marker)


func apply_frame(frame: Dictionary) -> void:
	var alive := {}
	for snapshot: Dictionary in frame.get("snapshots", []):
		var key := "%d:%d" % [int(snapshot.get("entity_id", 0)), int(snapshot.get("part_id", 0))]
		alive[key] = true
		if not _views.has(key):
			_create_view(key, snapshot)
		_targets[key] = snapshot.get("transform", Transform3D.IDENTITY)
	for key: String in _views.keys():
		if not alive.has(key):
			var stale: MeshInstance3D = _views[key]
			stale.queue_free()
			_views.erase(key)
			_targets.erase(key)
	_apply_kernel_preview(frame.get("trajectory_preview"))


func _process(delta: float) -> void:
	var weight := 1.0 - exp(-INTERPOLATION_SPEED * delta)
	for key: String in _views.keys():
		var view: MeshInstance3D = _views[key]
		var target: Transform3D = _targets.get(key, view.transform)
		view.transform = view.transform.interpolate_with(target, weight)


func _create_view(key: String, snapshot: Dictionary) -> void:
	var view := MeshInstance3D.new()
	view.name = "Body_%s" % key.replace(":", "_")
	var shape: Dictionary = snapshot.get("shape", {})
	if str(shape.get("type", "box")) == "sphere":
		var sphere := SphereMesh.new()
		var radius := maxf(float(shape.get("radius", 0.45)), 0.02)
		sphere.radius = radius
		sphere.height = radius * 2.0
		sphere.radial_segments = 24
		sphere.rings = 12
		sphere.material = _snapshot_material(snapshot)
		view.mesh = sphere
	else:
		var box := BoxMesh.new()
		box.size = Vector3(shape.get("half_extents", Vector3.ONE * 0.25)) * 2.0
		box.material = _snapshot_material(snapshot)
		view.mesh = box
	view.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
	view.transform = snapshot.get("transform", Transform3D.IDENTITY)
	add_child(view)
	_views[key] = view
	_targets[key] = view.transform


func _snapshot_material(snapshot: Dictionary) -> StandardMaterial3D:
	if snapshot.has("enemy_archetype_id"):
		return _material(Color("674361"), Color("d7a441"), 0.25)
	if int(snapshot.get("surface_id", 0)) == 1003:
		return _material(Color("57e0d0"), Color("167c78"), 1.4)
	match int(snapshot.get("material_id", 0)):
		1:
			return _material(Color("c88e55"), Color("4b281b"), 0.0)
		5:
			return _material(Color("b85b42"), Color("572a25"), 0.0)
		9:
			var glass := _material(Color("8de0e8"), Color("286f78"), 0.5)
			glass.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
			glass.albedo_color.a = 0.58
			return glass
		_:
			return _material(Color("748c70"), Color("26382c"), 0.0)


func _material(color: Color, emission: Color, emission_energy: float) -> StandardMaterial3D:
	var result := StandardMaterial3D.new()
	result.albedo_color = color
	result.roughness = 0.68
	result.metallic = 0.08
	if emission_energy > 0.0:
		result.emission_enabled = true
		result.emission = emission
		result.emission_energy_multiplier = emission_energy
	return result


func _apply_kernel_preview(preview_value: Variant) -> void:
	if preview_value == null or not preview_value is Dictionary:
		_preview_mesh.mesh = null
		_impact_marker.visible = false
		return
	var preview: Dictionary = preview_value
	var samples: Array = preview.get("samples", [])
	if samples.size() < 2:
		_preview_mesh.mesh = null
	else:
		var immediate := ImmediateMesh.new()
		var line_material := _material(Color("bff9e8"), Color("57e0d0"), 2.0)
		line_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		immediate.surface_begin(Mesh.PRIMITIVE_LINE_STRIP, line_material)
		for sample: Vector3 in samples:
			immediate.surface_add_vertex(sample)
		immediate.surface_end()
		_preview_mesh.mesh = immediate
	var first_hit: Variant = preview.get("first_hit")
	if first_hit is Dictionary:
		_impact_marker.position = first_hit.get("point", Vector3.ZERO)
		_impact_marker.visible = true
	else:
		_impact_marker.visible = false
