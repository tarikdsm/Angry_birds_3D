extends SceneTree

const ASSET_CATALOG := preload("res://scripts/data/asset_catalog.gd")
const CAMERA_DIRECTOR := preload("res://scripts/camera/camera_director.gd")
const CONTENT_FILE_LOADER := preload("res://scripts/data/content_file_loader.gd")
const CONTROLLER := preload("res://scripts/game/gameplay_session_controller.gd")
const INPUT_INTENT := preload("res://scripts/input/input_intent.gd")
const INPUT_ROUTER := preload("res://scripts/input/input_router.gd")
const SLINGSHOT_CONTROLLER := preload("res://scripts/game/slingshot_controller.gd")
const GAMEPLAY_SCENE := "res://scenes/gameplay/gameplay_session.tscn"
const EARTH_LEVEL_PATH := "res://data/levels/earth/farm_reaction.level.json"
const ORBITAL_LEVEL_PATH := "res://data/levels/orbital/first_orbit_v2.level.json"
const ARCHETYPES_PATH := "res://data/archetypes/product_v2.archetypes.json"
const ABILITY_GATE_MS := 120
const FORBIDDEN_HIT_CLASSES := [
	"Area3D", "CollisionObject3D", "CollisionShape3D", "RayCast3D", "ShapeCast3D",
]
const MARKER := "SLINGSHOT_INPUT_SMOKE_OK"


class RecordingSession extends Node:
	var calls: Array = []
	var accepts := true

	func queue_begin_grab(camera_right: Vector3) -> bool:
		calls.append({"call": "queue_begin_grab", "camera_right": camera_right})
		return accepts

	func queue_pull(horizontal_m: float, vertical_m: float) -> bool:
		calls.append({
			"call": "queue_pull",
			"horizontal_m": horizontal_m,
			"vertical_m": vertical_m,
		})
		return accepts

	func queue_release() -> bool:
		calls.append({"call": "queue_release"})
		return accepts

	func queue_activate_ability() -> bool:
		calls.append({"call": "queue_activate_ability"})
		return accepts

	func queue_cancel_grab() -> bool:
		calls.append({"call": "queue_cancel_grab"})
		return accepts

	func call_names() -> Array[String]:
		var names: Array[String] = []
		for entry: Dictionary in calls:
			names.append(str(entry.get("call", "")))
		return names


var _errors: Array[String] = []
var _slingshot: Node3D
var _session: RecordingSession
var _camera: Camera3D
var _director: Node3D
var _gameplay: Node3D
var _router: Node
var _level: Dictionary = {}
var _archetypes: Dictionary = {}
var _catalog: Dictionary = {}


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	if not _load_documents():
		_fail(" | ".join(_errors))
		return
	await _run_unit_gesture()
	if _errors.is_empty():
		await _run_router_and_camera_lock()
	if _errors.is_empty():
		await _run_product_integration()
	if not _errors.is_empty():
		_fail(" | ".join(_errors))
		return
	print(MARKER)
	_finish(0)


func _load_documents() -> bool:
	var level_result := CONTENT_FILE_LOADER.load_json(EARTH_LEVEL_PATH)
	var archetypes_result := CONTENT_FILE_LOADER.load_json(ARCHETYPES_PATH)
	var catalog_result: Dictionary = ASSET_CATALOG.load_catalog()
	if not bool(level_result.get("ok", false)) \
			or not bool(archetypes_result.get("ok", false)) \
			or not bool(catalog_result.get("ok", false)):
		_errors.append("the product content bundle must load before the gesture smoke")
		return false
	_level = level_result.document as Dictionary
	_archetypes = archetypes_result.document as Dictionary
	_catalog = catalog_result.document as Dictionary
	return true


func _run_unit_gesture() -> void:
	_slingshot = SLINGSHOT_CONTROLLER.new()
	root.add_child(_slingshot)
	_session = RecordingSession.new()
	root.add_child(_session)
	_camera = Camera3D.new()
	root.add_child(_camera)
	await process_frame

	if not _slingshot.configure(_level, _archetypes, _catalog):
		_check(false, "the slingshot must configure from the registered product content")
		return
	_slingshot.bind_session(_session)
	_slingshot.bind_camera(_camera)
	var rest: Vector3 = _slingshot.rest_position()
	_check(rest.is_equal_approx(Vector3(-16.0, 2.0, 0.0)),
		"the rest position must come from the level manifest")
	_camera.global_transform = Transform3D(Basis.IDENTITY, rest + Vector3(0.0, 0.0, 18.0))
	_camera.look_at(rest, Vector3.UP)
	_camera.make_current()
	await process_frame

	var forbidden := _forbidden_hit_node(_slingshot)
	_check(forbidden.is_empty(),
		"the slingshot must detect the bird without Godot collision nodes: %s" % forbidden)

	var center := _camera.unproject_position(_slingshot.ghost_position())
	_check(_slingshot.hit_test(center),
		"a ray through the bird visual must be accepted by the ray-sphere test")
	_check(not _slingshot.hit_test(center + Vector2(320.0, 240.0)),
		"a ray far from the bird visual must be rejected by the ray-sphere test")
	_check(not _slingshot.hit_test(center + Vector2(0.0, -400.0)),
		"a ray above the bird visual must be rejected by the ray-sphere test")

	_check(str(_slingshot.handle_begin_grab(center + Vector2(320.0, 240.0))) == "ignored"
			and _session.calls.is_empty(),
		"a press outside the bird visual must never open a grab")

	_slingshot.observe_frame(_inspection_frame())
	_check(str(_slingshot.handle_begin_grab(center)) == "grab",
		"a press on the bird visual must open a grab")
	_check(_session.call_names() == ["queue_begin_grab"],
		"BeginGrab must be the only command sent when the grab opens")
	var begin_call := _session.calls[0] as Dictionary
	_check((begin_call.camera_right as Vector3) == _camera.global_basis.x,
		"BeginGrab must publish the camera right basis, never a launch velocity")
	_check(not begin_call.has("speed") and not begin_call.has("velocity"),
		"BeginGrab must not carry any velocity field")

	_session.calls.clear()
	_check(not _slingshot.handle_update_pull(center),
		"a pull before the kernel publishes the locked plane must be dropped")
	_check(_session.calls.is_empty(),
		"a pull without a published plane must never reach the kernel")

	_slingshot.observe_frame(_grabbed_frame(Vector3.RIGHT, Vector3.UP, 0.0, 0.0))
	var pulled_point := rest + Vector3.RIGHT * -1.5 + Vector3.UP * 0.8
	_check(_slingshot.handle_update_pull(_camera.unproject_position(pulled_point)),
		"a pull on the published plane must be accepted")
	_check(_session.call_names() == ["queue_pull"],
		"a pull must send exactly one queue_pull command")
	var pull_call := _session.calls[0] as Dictionary
	_check(pull_call.keys().size() == 3 and pull_call.has("horizontal_m")
			and pull_call.has("vertical_m"),
		"a pull must publish only metric coordinates, never a velocity")
	_check(absf(float(pull_call.horizontal_m) + 1.5) <= 0.002
			and absf(float(pull_call.vertical_m) - 0.8) <= 0.002,
		"a pull must publish metres measured on the published plane: %s" % pull_call)

	_session.calls.clear()
	_slingshot.observe_frame(_grabbed_frame(
		Vector3(1.0, 0.0, -1.0).normalized(), Vector3.UP, -1.5, 0.8))
	if not _slingshot.handle_update_pull(_camera.unproject_position(pulled_point)):
		_check(false, "a pull must follow the plane republished by the kernel")
		return
	var rotated_pull := _session.calls[0] as Dictionary
	_check(absf(float(rotated_pull.horizontal_m) + 1.5) > 0.2,
		"a rotated locked plane must change the decomposition: %s" % rotated_pull)

	_session.calls.clear()
	_check(str(_slingshot.handle_release()) == "release",
		"a mouse-up during a grab must release the bird")
	_check(_session.call_names() == ["queue_release"],
		"a release must send exactly queue_release and nothing else")
	_check((_session.calls[0] as Dictionary).keys().size() == 1,
		"a release must never carry a launch velocity")

	_session.calls.clear()
	_slingshot.observe_frame(_flight_frame())
	_check(str(_slingshot.handle_release()) == "ignored" and _session.calls.is_empty(),
		"a mouse-up must never activate an ability")
	_check(str(_slingshot.handle_begin_grab(center)) == "ignored"
			and _session.calls.is_empty(),
		"a press inside the 120 ms gate must never activate an ability")
	OS.delay_msec(ABILITY_GATE_MS + 20)
	_check(str(_slingshot.handle_begin_grab(center)) == "ability",
		"a new press after the 120 ms gate must activate the ability")
	_check(_session.call_names() == ["queue_activate_ability"],
		"an ability press must send exactly queue_activate_ability")

	_session.calls.clear()
	_slingshot.observe_frame(_inspection_frame())
	_check(str(_slingshot.handle_begin_grab(center)) == "grab",
		"the next bird must be grabbable after the shot resolves")
	_session.calls.clear()
	_check(_slingshot.handle_cancel() and _session.call_names() == ["queue_cancel_grab"],
		"Esc during a grab must cancel it in the kernel")
	_session.calls.clear()
	_check(not _slingshot.handle_cancel() and _session.calls.is_empty(),
		"Esc outside a grab must never reach the kernel")


func _run_router_and_camera_lock() -> void:
	_director = CAMERA_DIRECTOR.new()
	root.add_child(_director)
	await process_frame
	if not _director.configure("CAM_Farm", _level):
		_check(false, "the camera director must accept the Earth camera profile: %s"
			% _director.last_error)
		return
	_slingshot.bind_camera(_director.active_rig())
	_slingshot.bind_camera_director(_director)
	_slingshot.observe_frame(_inspection_frame())
	_director.observe_frame(_inspection_frame())
	await process_frame
	var rig: Camera3D = _director.active_rig()
	_check(not rig.is_position_behind(_slingshot.ghost_position()),
		"the Earth rig must frame the launcher during inspection")
	var center := rig.unproject_position(_slingshot.ghost_position())

	_check(not _director.is_locked(), "the camera must move before a grab opens")
	_check(_director.apply_orbit(Vector2(12.0, 4.0)),
		"orbit must be accepted outside a grab")
	_session.calls.clear()
	_check(str(_slingshot.handle_begin_grab(center)) == "grab",
		"the product camera rig must still allow the ray-sphere grab")
	_check(_director.is_locked(),
		"BeginGrab must freeze the camera until cancel or release")
	_check(not _director.apply_orbit(Vector2(12.0, 4.0))
			and not _director.apply_zoom(1.0),
		"orbit and zoom must be blocked while the launcher is grabbed")
	_check(str(_slingshot.handle_release()) == "release" and not _director.is_locked(),
		"release must unfreeze the camera")
	_slingshot.observe_frame(_inspection_frame())
	_check(str(_slingshot.handle_begin_grab(center)) == "grab" and _director.is_locked(),
		"a second grab must freeze the camera again")
	_check(_slingshot.handle_cancel() and not _director.is_locked(),
		"a cancelled grab must unfreeze the camera")

	_router = INPUT_ROUTER.new()
	root.add_child(_router)
	_router.set_context(&"gameplay")
	var intents: Array = []
	_router.intent_submitted.connect(intents.append)
	var recenter_event := InputEventKey.new()
	recenter_event.physical_keycode = KEY_F
	recenter_event.pressed = true
	_check(_router.route_raw_event(recenter_event) and intents.size() == 1
			and intents[0].kind == INPUT_INTENT.KIND_RECENTER,
		"F must reach the camera as a recenter intent")
	_check(_director.apply_orbit(Vector2(60.0, 20.0))
			and _director.apply_zoom(2.0),
		"the camera must accept a manual composition before the recenter")
	var displaced: Vector2 = _director.orbit_degrees()
	var recenters: int = _director.recenter_calls
	_check(_director.handle_intent(intents[0] as RefCounted)
			and _director.recenter_calls == recenters + 1
			and _director.orbit_degrees() != displaced,
		"the recenter intent must restore the authored composition")
	intents.clear()
	var escape_event := InputEventKey.new()
	escape_event.physical_keycode = KEY_ESCAPE
	escape_event.pressed = true
	_check(_router.route_raw_event(escape_event) and intents.size() == 1
			and intents[0].kind == INPUT_INTENT.KIND_BACK,
		"Esc must reach gameplay as a semantic back intent")
	_check((_router.supported_intent_kinds(&"gameplay", &"keyboard") as Array).has(
			INPUT_INTENT.KIND_BACK),
		"the gameplay capability matrix must publish the cancel path")


func _run_product_integration() -> void:
	var packed := load(GAMEPLAY_SCENE) as PackedScene
	if packed == null:
		_check(false, "the generic gameplay scene must be loadable")
		return
	_gameplay = packed.instantiate() as Node3D
	root.add_child(_gameplay)
	await process_frame
	for level_key: Array in [["earth", "farm_reaction"], ["orbital", "first_orbit_v2"]]:
		await _play_gesture(str(level_key[0]), str(level_key[1]))
		if not _errors.is_empty():
			return
	_gameplay.release_level()
	await process_frame


func _play_gesture(world_id: String, level_id: String) -> void:
	var request_result: Dictionary = CONTROLLER.make_launch_request(world_id, level_id)
	if not bool(request_result.get("ok", false)):
		_check(false, "the %s level must be registered" % world_id)
		return
	if not _gameplay.configure_launch(request_result.request as Dictionary):
		_check(false, "the %s launch must configure the product session: %s"
			% [world_id, _gameplay.last_error])
		return
	for _index: int in 3:
		await physics_frame
	var slingshot := _gameplay.slingshot() as Node3D
	var director := _gameplay.camera_director() as Node3D
	var rig := director.active_rig() as Camera3D
	_check(rig != null and rig.current,
		"%s must activate exactly one product camera rig" % world_id)
	var ghost: Vector3 = slingshot.ghost_position()
	_check(not rig.is_position_behind(ghost),
		"%s must frame the launcher before the gesture" % world_id)
	var center := rig.unproject_position(ghost)

	_check(_gameplay.handle_intent(_intent(INPUT_INTENT.KIND_BEGIN_GRAB, {"position": center})),
		"%s must accept a press on the bird visual" % world_id)
	_check(director.is_locked(), "%s must freeze the camera during the grab" % world_id)
	await physics_frame
	await physics_frame
	var grabbed := _gameplay.current_frame as Dictionary
	_check(str(grabbed.get("phase", "")) == "grabbed" and grabbed.get("locked_plane") != null,
		"%s must reach the grabbed phase with a published plane" % world_id)
	var plane := grabbed.locked_plane as Dictionary
	var pulled_point: Vector3 = slingshot.rest_position() \
		+ (plane.horizontal as Vector3) * -1.2 + (plane.up as Vector3) * 0.6
	_check(_gameplay.handle_intent(_intent(INPUT_INTENT.KIND_UPDATE_PULL, {
			"position": rig.unproject_position(pulled_point)})),
		"%s must accept a pull on the published plane" % world_id)
	await physics_frame
	await physics_frame
	var pulled := _gameplay.current_frame as Dictionary
	var launcher := pulled.get("launcher") as Dictionary
	_check(launcher != null and float(launcher.get("extension_m", 0.0)) > 0.5,
		"%s must extend the launcher from the metric pull" % world_id)
	_check(_gameplay.handle_intent(_intent(INPUT_INTENT.KIND_RELEASE, {"position": center})),
		"%s must release the bird on mouse-up" % world_id)
	_check(not director.is_locked(), "%s must unfreeze the camera on release" % world_id)
	await physics_frame
	await physics_frame
	var released := _gameplay.current_frame as Dictionary
	_check(str(released.get("phase", "")) == "flight_ability"
			and (released.get("projectiles", []) as Array).size() >= 1,
		"%s must launch exactly through the kernel Hooke solver" % world_id)
	_check(not slingshot.is_grabbing(),
		"%s must close the presentation grab after the release" % world_id)
	_gameplay.release_level()
	await process_frame
	await process_frame


func _intent(kind: StringName, payload: Dictionary) -> RefCounted:
	return INPUT_INTENT.new(kind, &"mouse", payload)


func _inspection_frame() -> Dictionary:
	return {
		"phase": "inspection",
		"launcher": null,
		"locked_plane": null,
		"current_bird": 2,
		"bird_queue": [2, 3, 4, 5],
		"events": [],
		"snapshots": [],
		"ability_readiness": "unavailable",
	}


func _grabbed_frame(
		horizontal: Vector3, up: Vector3,
		pull_horizontal_m: float, pull_vertical_m: float) -> Dictionary:
	return {
		"phase": "grabbed",
		"launcher": {
			"rest_position": _slingshot.rest_position(),
			"pull_horizontal_m": pull_horizontal_m,
			"pull_vertical_m": pull_vertical_m,
			"extension_m": sqrt(
				pull_horizontal_m * pull_horizontal_m + pull_vertical_m * pull_vertical_m),
			"deadzone_m": 0.2,
			"maximum_extension_m": 4.25,
		},
		"locked_plane": {
			"camera_right": horizontal,
			"up": up,
			"horizontal": horizontal,
			"plane_normal": horizontal.cross(up).normalized(),
		},
		"current_bird": 2,
		"bird_queue": [2, 3, 4, 5],
		"events": [],
		"snapshots": [],
		"ability_readiness": "unavailable",
	}


func _flight_frame() -> Dictionary:
	return {
		"phase": "flight_ability",
		"launcher": null,
		"locked_plane": null,
		"current_bird": 3,
		"bird_queue": [3, 4, 5],
		"events": [],
		"snapshots": [],
		"ability_readiness": "armed",
	}


func _forbidden_hit_node(node: Node) -> String:
	for forbidden: String in FORBIDDEN_HIT_CLASSES:
		if node.is_class(forbidden):
			return "%s(%s)" % [node.name, forbidden]
	for child: Node in node.get_children():
		var found := _forbidden_hit_node(child)
		if not found.is_empty():
			return found
	return ""


func _check(condition: bool, message: String) -> void:
	if not condition:
		_errors.append(message)


func _fail(message: String) -> void:
	push_error("slingshot input smoke: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	for node: Node in [_gameplay, _router, _director, _camera, _session, _slingshot]:
		if is_instance_valid(node):
			root.remove_child(node)
			node.free()
	quit(exit_code)
