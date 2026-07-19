extends Node3D

const INTERPOLATION_SPEED := 18.0
const ASSET_SCENES := {
	"AST_AsterPlanet": preload("res://assets/vertical_slice/aster/AST_AsterPlanet.glb"),
	"AST_Platform": preload("res://assets/vertical_slice/aster/AST_Platform.glb"),
	"CHR_Virela": preload("res://assets/vertical_slice/characters/CHR_Virela.glb"),
	"CHR_LaunchBird": preload("res://assets/vertical_slice/characters/CHR_Virela.glb"),
	"ENM_Anchor": preload("res://assets/vertical_slice/enemies/ENM_Anchor.glb"),
	"KIT_PineBeam_A": preload("res://assets/vertical_slice/kit/KIT_PineBeam_A.glb"),
	"KIT_PineLintel_A": preload("res://assets/vertical_slice/kit/KIT_PineLintel_A.glb"),
	"KIT_PineBrace_A": preload("res://assets/vertical_slice/kit/KIT_PineBrace_A.glb"),
	"KIT_GlassPanel_A": preload("res://assets/vertical_slice/kit/KIT_GlassPanel_A.glb"),
	"KIT_Brick_A": preload("res://assets/vertical_slice/kit/KIT_Brick_A.glb"),
}
const GLASS_MATERIAL := preload("res://materials/glass.tres")

var _views: Dictionary = {}
var _targets: Dictionary = {}
var _missing_asset_ids: Dictionary = {}
var _preview_mesh: MeshInstance3D
var _impact_marker: MeshInstance3D
var _phase := "inspection"


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
	_phase = str(frame.get("phase", "inspection"))
	var alive := {}
	for snapshot: Dictionary in frame.get("snapshots", []):
		var key := "%d:%d" % [int(snapshot.get("entity_id", 0)), int(snapshot.get("part_id", 0))]
		alive[key] = true
		if not _views.has(key):
			_create_view(key, snapshot)
		_targets[key] = snapshot.get("transform", Transform3D.IDENTITY)
	for key: String in _views.keys():
		if not alive.has(key):
			var stale: Node3D = _views[key]
			stale.queue_free()
			_views.erase(key)
			_targets.erase(key)
	_apply_kernel_preview(frame.get("trajectory_preview"))


func _process(delta: float) -> void:
	var weight := 1.0 - exp(-INTERPOLATION_SPEED * delta)
	for key: String in _views:
		var view: Node3D = _views[key]
		var target: Transform3D = _targets.get(key, view.transform)
		view.transform = view.transform.interpolate_with(target, weight)


func missing_asset_ids() -> Array:
	return _missing_asset_ids.keys()


func _create_view(key: String, snapshot: Dictionary) -> void:
	var asset_id := str(snapshot.get("visual_id", ""))
	var packed: PackedScene = ASSET_SCENES.get(asset_id)
	if packed == null:
		_missing_asset_ids[asset_id] = true
		push_error("Missing authored visual asset: %s" % asset_id)
		return
	var view := packed.instantiate() as Node3D
	if view == null:
		push_error("Authored visual root is not Node3D: %s" % asset_id)
		return
	view.name = "Body_%s" % key.replace(":", "_")
	view.transform = snapshot.get("transform", Transform3D.IDENTITY)
	add_child(view)
	_prepare_authored_view(view, asset_id)
	_views[key] = view
	_targets[key] = view.transform


func _prepare_authored_view(node: Node, asset_id: String) -> void:
	for child: Node in node.get_children():
		var child_name := str(child.name)
		if child is Node3D and (
				child_name.begins_with("COL_")
				or child_name.begins_with("FRAG_")
				or child_name.begins_with("SOCKET_")
				or child_name.begins_with("RIG_")
				or child_name.ends_with("_LOD1")):
			(child as Node3D).visible = false
		elif child is GeometryInstance3D:
			(child as GeometryInstance3D).cast_shadow = \
				GeometryInstance3D.SHADOW_CASTING_SETTING_ON
			if asset_id == "KIT_GlassPanel_A":
				(child as GeometryInstance3D).material_override = GLASS_MATERIAL
		_prepare_authored_view(child, asset_id)


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
	if _phase != "aim" or preview_value == null or not preview_value is Dictionary:
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
