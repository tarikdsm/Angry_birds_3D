extends Node3D

## Presentation and input state of the slingshot gesture.
##
## The controller never computes a launch velocity: BeginGrab publishes the
## camera right basis, the pull publishes metric coordinates measured on the
## plane the kernel already locked, and the release publishes nothing but the
## release command. Hooke, the deadzone, the circular clamp and the
## quantisation all belong to the kernel.
##
## The bird under the cursor is found by intersecting the cursor ray with the
## sphere of the ghost visual. No Godot collision node takes part in it.

const LAUNCH_DEVICE_VIEW := preload("res://scripts/game/launch_device_view.gd")

const ABILITY_GATE_MSEC := 120
const GRAB_TOLERANCE_M := 0.15
const PROVISIONAL_ALBEDO := Color("c8503c")
const RESULT_GRAB := &"grab"
const RESULT_ABILITY := &"ability"
const RESULT_RELEASE := &"release"
const RESULT_IGNORED := &"ignored"
const GHOST_PHASES := ["inspection", "grabbed"]

signal grab_started
signal grab_released
signal grab_cancelled

var last_error := ""

var _configured := false
var _rest := Vector3.ZERO
var _deadzone_m := 0.0
var _maximum_extension_m := 0.0
var _birds: Dictionary = {}
var _assets: Dictionary = {}
var _current_bird_id := 0
var _ghost_bird_id := 0
var _ghost: Node3D
var _device: Node3D
var _plane: Dictionary = {}
var _offset := Vector3.ZERO
var _phase := "inspection"
var _grabbing := false
var _release_msec := 0
var _session: Node
var _camera: Camera3D
var _director: Node3D


func configure(
		level_document: Dictionary,
		archetypes_document: Dictionary,
		catalog_document: Dictionary) -> bool:
	release()
	last_error = ""
	var slingshot := level_document.get("slingshot", {}) as Dictionary
	if not slingshot.has("rest_position_m"):
		last_error = "the level manifest does not publish a slingshot"
		return false
	_rest = _vector_of(slingshot.rest_position_m)
	_deadzone_m = float(slingshot.get("minimum_extension_m", 0.0))
	_maximum_extension_m = float(slingshot.get("maximum_extension_m", 0.0))
	for bird: Variant in archetypes_document.get("birds", []):
		if bird is Dictionary:
			_birds[int((bird as Dictionary).get("id", 0))] = (bird as Dictionary).duplicate(true)
	if _birds.is_empty():
		last_error = "the archetype bundle does not publish any bird"
		return false
	_assets = LAUNCH_DEVICE_VIEW.asset_index(catalog_document)
	var queue := level_document.get("bird_queue", []) as Array
	_current_bird_id = int(queue[0]) if not queue.is_empty() else 0
	_device = LAUNCH_DEVICE_VIEW.new()
	_device.name = &"LaunchDevice"
	add_child(_device)
	if not _device.configure(level_document, catalog_document):
		last_error = "the launch device view rejected the level manifest"
		release()
		return false
	_ghost = Node3D.new()
	_ghost.name = &"BirdGhost"
	add_child(_ghost)
	_ghost.global_position = _rest
	_configured = true
	_rebuild_ghost(_current_bird_id)
	return true


func release() -> void:
	for node: Node in [_ghost, _device]:
		if is_instance_valid(node):
			remove_child(node)
			node.queue_free()
	_ghost = null
	_device = null
	_configured = false
	_birds = {}
	_assets = {}
	_plane = {}
	_offset = Vector3.ZERO
	_phase = "inspection"
	_grabbing = false
	_current_bird_id = 0
	_ghost_bird_id = 0
	_session = null
	_camera = null
	_director = null


func configured() -> bool:
	return _configured


func bind_session(session: Node) -> void:
	_session = session


func bind_camera(camera: Camera3D) -> void:
	_camera = camera


func bind_camera_director(director: Node3D) -> void:
	_director = director


func device_view() -> Node3D:
	return _device


func rest_position() -> Vector3:
	return _rest


func ghost_position() -> Vector3:
	return _rest + _offset


func ghost_radius_m() -> float:
	var bird := _birds.get(_current_bird_id, {}) as Dictionary
	return float(bird.get("radius_m", 0.35)) + GRAB_TOLERANCE_M


func current_bird_id() -> int:
	return _current_bird_id


func is_grabbing() -> bool:
	return _grabbing


func locked_plane() -> Dictionary:
	return _plane.duplicate(true)


func observe_frame(frame: Dictionary) -> void:
	if not _configured:
		return
	_phase = str(frame.get("phase", _phase))
	# A gesture the kernel refuses — a grab requested before the level finished
	# reinstalling, for instance — must never leave the presentation holding a
	# grab the kernel does not know about.
	if _grabbing and _phase != "grabbed" and _has_rejection(frame):
		_grabbing = false
		_plane = {}
		if _director != null:
			_director.unlock()
		grab_cancelled.emit()
	var current: Variant = frame.get("current_bird")
	if current != null:
		_rebuild_ghost(int(current))
	var plane: Variant = frame.get("locked_plane")
	if plane is Dictionary:
		_plane = (plane as Dictionary).duplicate(true)
		_device.apply_plane(_plane.up as Vector3, _plane.horizontal as Vector3)
	elif _phase != "grabbed":
		_plane = {}
	_offset = _displacement_of(frame)
	_ghost.global_position = _rest + _offset
	_ghost.visible = GHOST_PHASES.has(_phase)
	_device.apply_pull(_offset)
	if _phase != "grabbed" and _phase != "inspection":
		_grabbing = false


## Ray-sphere intersection against the ghost visual. No collision node, no
## solver and no space state take part in the decision.
func hit_test(screen_position: Vector2) -> bool:
	if not _configured or _camera == null:
		return false
	var center := ghost_position()
	if _camera.is_position_behind(center):
		return false
	var origin := _camera.project_ray_origin(screen_position)
	var direction := _camera.project_ray_normal(screen_position)
	if not direction.is_normalized():
		return false
	var along := (center - origin).dot(direction)
	if along <= 0.0:
		return false
	var radius := ghost_radius_m()
	return (origin + direction * along).distance_squared_to(center) <= radius * radius


func handle_begin_grab(screen_position: Vector2) -> StringName:
	if not _configured or _session == null or _camera == null:
		return RESULT_IGNORED
	if _phase == "inspection":
		if _grabbing or not hit_test(screen_position):
			return RESULT_IGNORED
		if not _session.queue_begin_grab(_camera.global_basis.x):
			return RESULT_IGNORED
		_grabbing = true
		_plane = {}
		if _director != null:
			_director.lock()
		grab_started.emit()
		return RESULT_GRAB
	return handle_activate_ability()


## Abilities are dispatched by the kernel from the AbilityKind of the shot. The
## presentation only asks for an activation, and never before the 120 ms gate
## that keeps the releasing mouse-up from consuming the ability.
func handle_activate_ability() -> StringName:
	if not _configured or _session == null or _phase != "flight_ability":
		return RESULT_IGNORED
	if Time.get_ticks_msec() - _release_msec < ABILITY_GATE_MSEC:
		return RESULT_IGNORED
	if not _session.queue_activate_ability():
		return RESULT_IGNORED
	return RESULT_ABILITY


func handle_update_pull(screen_position: Vector2) -> bool:
	if not _configured or not _grabbing or _session == null or _camera == null:
		return false
	if not _plane.has("plane_normal") or not _plane.has("horizontal") or not _plane.has("up"):
		return false
	var normal := _plane.plane_normal as Vector3
	var origin := _camera.project_ray_origin(screen_position)
	var direction := _camera.project_ray_normal(screen_position)
	if not direction.is_normalized() or not normal.is_normalized():
		return false
	var denominator := direction.dot(normal)
	if absf(denominator) <= 0.00001:
		return false
	var along := (_rest - origin).dot(normal) / denominator
	if along <= 0.0:
		return false
	var displacement := origin + direction * along - _rest
	return _session.queue_pull(
		displacement.dot(_plane.horizontal as Vector3),
		displacement.dot(_plane.up as Vector3))


func handle_release() -> StringName:
	if not _configured or not _grabbing or _session == null:
		return RESULT_IGNORED
	_grabbing = false
	_release_msec = Time.get_ticks_msec()
	if _director != null:
		_director.unlock()
	if not _session.queue_release():
		return RESULT_IGNORED
	grab_released.emit()
	return RESULT_RELEASE


func handle_cancel() -> bool:
	if not _configured or not _grabbing or _session == null:
		return false
	_grabbing = false
	_plane = {}
	if _director != null:
		_director.unlock()
	if not _session.queue_cancel_grab():
		return false
	grab_cancelled.emit()
	return true


static func _has_rejection(frame: Dictionary) -> bool:
	for event: Variant in frame.get("events", []):
		if event is Dictionary 				and str((event as Dictionary).get("kind", "")) == "command_rejected":
			return true
	return false


func _displacement_of(frame: Dictionary) -> Vector3:
	var launcher: Variant = frame.get("launcher")
	if not launcher is Dictionary or _plane.is_empty():
		return Vector3.ZERO
	var state := launcher as Dictionary
	return (_plane.horizontal as Vector3) * float(state.get("pull_horizontal_m", 0.0)) \
		+ (_plane.up as Vector3) * float(state.get("pull_vertical_m", 0.0))


func _rebuild_ghost(bird_id: int) -> void:
	_current_bird_id = bird_id
	if _ghost == null or _ghost_bird_id == bird_id:
		return
	_ghost_bird_id = bird_id
	for child: Node in _ghost.get_children():
		_ghost.remove_child(child)
		child.queue_free()
	var bird := _birds.get(bird_id, {}) as Dictionary
	var authored := _authored_visual(str(bird.get("projectile_visual_id", "")))
	_ghost.add_child(authored if authored != null else _provisional_visual(bird))


func _authored_visual(asset_id: String) -> Node3D:
	var entry := _assets.get(asset_id, {}) as Dictionary
	var resource_path := str(entry.get("resource_path", ""))
	if resource_path.is_empty() or not ResourceLoader.exists(resource_path, "PackedScene"):
		return null
	var packed := ResourceLoader.load(resource_path, "PackedScene") as PackedScene
	if packed == null:
		return null
	return packed.instantiate() as Node3D


func _provisional_visual(bird: Dictionary) -> Node3D:
	var view := MeshInstance3D.new()
	view.name = &"GhostVisual"
	var sphere := SphereMesh.new()
	sphere.radius = maxf(0.05, float(bird.get("radius_m", 0.35)))
	sphere.height = sphere.radius * 2.0
	var material := StandardMaterial3D.new()
	material.albedo_color = PROVISIONAL_ALBEDO
	material.roughness = 0.6
	material.metallic = 0.02
	sphere.material = material
	view.mesh = sphere
	return view


static func _vector_of(value: Variant) -> Vector3:
	if value is Vector3:
		return value as Vector3
	if not value is Array or (value as Array).size() != 3:
		return Vector3.ZERO
	var values := value as Array
	return Vector3(float(values[0]), float(values[1]), float(values[2]))
