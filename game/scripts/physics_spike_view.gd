extends Node3D

const BOX_FULL_SIZE := Vector3(0.64, 0.50, 0.64)
const PROJECTILE_RADIUS := 0.45
const STEP_SAMPLE_LIMIT := 120
const BOX_COLORS := [Color("e4a15f"), Color("c87345"), Color("f0bf75")]

@onready var physics: Box3DWorldNode = $Box3DWorldNode
@onready var planet_view: MeshInstance3D = $Visuals/Planet
@onready var body_views: Node3D = $Visuals/BodyViews
@onready var metrics_label: Label = $Overlay/Telemetry/Margin/Rows/Metrics
@onready var phase_label: Label = $Overlay/Telemetry/Margin/Rows/Phase

var views: Dictionary = {}
var frames := 0
var _step_samples: Array[float] = []
var _movie_capture := false
var _materials: Array[StandardMaterial3D] = []
var _received_snapshot := false


func _ready() -> void:
	process_physics_priority = 100
	physics.set_physics_process(true)
	_movie_capture = OS.get_cmdline_args().has("--write-movie")
	physics.physics_fault.connect(_on_physics_fault)
	_prepare_materials()
	if not physics.configure_planet(10.0, 9.0):
		push_error("visual spike could not configure Box3D")
		get_tree().quit(1)
		return

	for layer in range(10):
		for column in range(12):
			var position := Vector3((column - 5.5) * 0.70, 10.55 + layer * 0.55, 0.0)
			var handle: int = physics.spawn_box(
				BOX_FULL_SIZE,
				Transform3D(Basis.IDENTITY, position),
				520.0
			)
			_add_box_view(handle, BOX_FULL_SIZE, (layer + column) % BOX_COLORS.size())

	var projectile: int = physics.spawn_projectile(
		PROJECTILE_RADIUS,
		Transform3D(Basis.IDENTITY, Vector3(-8.0, 13.0, 0.0)),
		Vector3(35.0, 0.0, 0.0)
	)
	_add_sphere_view(projectile, PROJECTILE_RADIUS)


func _physics_process(_delta: float) -> void:
	var states: Array = physics.get_body_states()
	var alive := {}
	var projectile_x := -8.0
	for state: Dictionary in states:
		var handle: int = int(state.handle)
		alive[handle] = true
		if not views.has(handle) and is_zero_approx(float(state.mass)):
			views[handle] = planet_view
		if views.has(handle):
			var view: MeshInstance3D = views[handle]
			view.transform = Transform3D(Basis(state.rotation), state.position)
			if view.name == "Projectile":
				projectile_x = float(state.position.x)

	if not states.is_empty():
		_received_snapshot = true
	if _received_snapshot:
		for handle: int in views.keys():
			if not alive.has(handle):
				var stale: MeshInstance3D = views[handle]
				stale.queue_free()
				views.erase(handle)

	var metrics: Dictionary = physics.get_metrics()
	_record_step_time(float(metrics.get("step_ms", 0.0)))
	metrics_label.text = "BODIES  %03d    CONTACTS  %03d    AWAKE  %03d    STEP P95  %.3f ms" % [
		int(metrics.get("body_count", 0)),
		int(metrics.get("contact_count", 0)),
		int(metrics.get("awake_count", 0)),
		_step_p95(),
	]
	phase_label.text = _phase_text(projectile_x)

	frames += 1
	if _movie_capture and frames == 300:
		get_tree().quit(0)


func _prepare_materials() -> void:
	for color: Color in BOX_COLORS:
		var material := StandardMaterial3D.new()
		material.albedo_color = color
		material.metallic = 0.12
		material.roughness = 0.72
		_materials.append(material)


func _add_box_view(handle: int, full_size: Vector3, palette_index: int) -> void:
	if handle == 0:
		return
	var view := MeshInstance3D.new()
	view.name = "Module_%d" % handle
	var box := BoxMesh.new()
	box.size = full_size
	box.material = _materials[palette_index]
	view.mesh = box
	view.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
	body_views.add_child(view)
	views[handle] = view


func _add_sphere_view(handle: int, radius: float) -> void:
	if handle == 0:
		return
	var material := StandardMaterial3D.new()
	material.albedo_color = Color("57e0d0")
	material.emission_enabled = true
	material.emission = Color("167c78")
	material.emission_energy_multiplier = 1.35
	material.metallic = 0.25
	material.roughness = 0.28
	var sphere := SphereMesh.new()
	sphere.radius = radius
	sphere.height = radius * 2.0
	sphere.radial_segments = 32
	sphere.rings = 16
	sphere.material = material
	var view := MeshInstance3D.new()
	view.name = "Projectile"
	view.mesh = sphere
	view.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
	body_views.add_child(view)
	view.position = Vector3(-8.0, 13.0, 0.0)
	views[handle] = view


func _record_step_time(step_ms: float) -> void:
	_step_samples.append(step_ms)
	if _step_samples.size() > STEP_SAMPLE_LIMIT:
		_step_samples.pop_front()


func _step_p95() -> float:
	if _step_samples.is_empty():
		return 0.0
	var ordered := _step_samples.duplicate()
	ordered.sort()
	var index := clampi(ceili(ordered.size() * 0.95) - 1, 0, ordered.size() - 1)
	return ordered[index]


func _phase_text(projectile_x: float) -> String:
	if frames < 5:
		return "PHASE  LAUNCH / KINETIC INBOUND"
	if frames < 12 and projectile_x < -3.5:
		return "PHASE  APPROACH / TRAJECTORY LOCKED"
	if frames < 90:
		return "PHASE  IMPACT / STRUCTURE RESPONSE"
	return "PHASE  SETTLE / RADIAL GRAVITY"


func _on_physics_fault(code: String, message: String) -> void:
	push_error("Box3D fault [%s]: %s" % [code, message])
	get_tree().quit(1)
