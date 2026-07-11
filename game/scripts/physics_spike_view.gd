extends Node3D

const BOX_FULL_SIZE := Vector3(0.64, 0.50, 0.64)
const PROJECTILE_RADIUS := 0.45
const STEP_SAMPLE_LIMIT := 120
const EXPECTED_BODY_COUNT := 122
const EXPECTED_VISUAL_BODY_COUNT := 121
const FIRST_SNAPSHOT_FRAME_LIMIT := 10
const MOVIE_CAPTURE_FRAME_LIMIT := 300
const MOVIE_CAPTURE_ARGUMENT := "--ninho-capture-300"
const MOVIE_CAPTURE_INITIAL_FRAME_COUNT := 1
const VISUAL_CAPTURE_COMPLETE_MARKER := "NINHO_VISUAL_CAPTURE_COMPLETE frame=300"
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
var _expected_visual_handles: Dictionary = {}
var _snapshot_wait_frames := 0
var _validated_first_snapshot := false
var _failed := false


func _ready() -> void:
	process_physics_priority = 100
	physics.set_physics_process(true)
	_movie_capture = OS.get_cmdline_user_args().has(MOVIE_CAPTURE_ARGUMENT)
	physics.physics_fault.connect(_on_physics_fault)
	_prepare_materials()
	if not physics.configure_planet(10.0, 9.0):
		_fail("could not configure Box3D")
		return

	for layer in range(10):
		for column in range(12):
			var position := Vector3((column - 5.5) * 0.70, 10.55 + layer * 0.55, 0.0)
			var handle: int = physics.spawn_box(
				BOX_FULL_SIZE,
				Transform3D(Basis.IDENTITY, position),
				520.0
			)
			if not _register_visual_handle(handle, "box at layer %d, column %d" % [layer, column]):
				return
			_add_box_view(handle, BOX_FULL_SIZE, (layer + column) % BOX_COLORS.size())

	var projectile: int = physics.spawn_projectile(
		PROJECTILE_RADIUS,
		Transform3D(Basis.IDENTITY, Vector3(-8.0, 13.0, 0.0)),
		Vector3(35.0, 0.0, 0.0)
	)
	if not _register_visual_handle(projectile, "projectile"):
		return
	_add_sphere_view(projectile, PROJECTILE_RADIUS)
	if _expected_visual_handles.size() != EXPECTED_VISUAL_BODY_COUNT:
		_fail("expected %d visual handles after spawning, got %d" % [
			EXPECTED_VISUAL_BODY_COUNT,
			_expected_visual_handles.size(),
		])


func _physics_process(_delta: float) -> void:
	if _failed:
		return
	var states: Array = physics.get_body_states()
	if states.is_empty() and not _validated_first_snapshot:
		_snapshot_wait_frames += 1
		if _snapshot_wait_frames >= FIRST_SNAPSHOT_FRAME_LIMIT:
			_fail("timed out waiting for the first Box3D snapshot")
		return

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

	if not _validated_first_snapshot:
		if not _validate_first_snapshot(states, alive):
			return
		_validated_first_snapshot = true

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
	if _movie_capture and frames == MOVIE_CAPTURE_FRAME_LIMIT - MOVIE_CAPTURE_INITIAL_FRAME_COUNT:
		print(VISUAL_CAPTURE_COMPLETE_MARKER)
		get_tree().quit(0)


func _register_visual_handle(handle: int, label: String) -> bool:
	if handle == 0:
		_fail("Box3D returned an invalid handle for %s" % label)
		return false
	if _expected_visual_handles.has(handle):
		_fail("Box3D returned duplicate handle %d for %s" % [handle, label])
		return false
	_expected_visual_handles[handle] = true
	return true


func _validate_first_snapshot(states: Array, alive: Dictionary) -> bool:
	if states.size() != EXPECTED_BODY_COUNT:
		_fail("first snapshot expected %d bodies, got %d" % [EXPECTED_BODY_COUNT, states.size()])
		return false
	if alive.size() != EXPECTED_BODY_COUNT:
		_fail("first snapshot contains duplicate body handles")
		return false

	var alive_visual_count := 0
	for handle: int in _expected_visual_handles.keys():
		if alive.has(handle):
			alive_visual_count += 1
		else:
			_fail("visual handle %d is absent from the first snapshot" % handle)
			return false
	if alive_visual_count != EXPECTED_VISUAL_BODY_COUNT:
		_fail("first snapshot expected %d live visual handles, got %d" % [
			EXPECTED_VISUAL_BODY_COUNT,
			alive_visual_count,
		])
		return false
	return true


func _prepare_materials() -> void:
	for color: Color in BOX_COLORS:
		var material := StandardMaterial3D.new()
		material.albedo_color = color
		material.metallic = 0.12
		material.roughness = 0.72
		_materials.append(material)


func _add_box_view(handle: int, full_size: Vector3, palette_index: int) -> void:
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
	_fail("Box3D fault [%s]: %s" % [code, message])


func _fail(message: String) -> void:
	if _failed:
		return
	_failed = true
	push_error("visual spike: %s" % message)
	get_tree().quit(1)
