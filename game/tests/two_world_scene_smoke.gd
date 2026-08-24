extends SceneTree

const CONTROLLER := preload("res://scripts/game/gameplay_session_controller.gd")
const GAMEPLAY_SCENE := "res://scenes/gameplay/gameplay_session.tscn"
const PRESENTATION_CLASSES := [
	"Node3D", "WorldEnvironment", "DirectionalLight3D", "OmniLight3D", "Marker3D",
]
const WORLD_EXPECTATIONS := [
	{
		"world_id": "earth",
		"level_id": "farm_reaction",
		"scene_path": "res://scenes/worlds/earth_farm.tscn",
		"root_name": "EarthFarm",
		"gravity_kind": "uniform",
	},
	{
		"world_id": "orbital",
		"level_id": "first_orbit_v2",
		"scene_path": "res://scenes/worlds/orbital_first_orbit.tscn",
		"root_name": "OrbitalFirstOrbit",
		"gravity_kind": "radial",
	},
]
const MARKER := "TWO_WORLD_SCENE_SMOKE_OK"

var _errors: Array[String] = []
var _gameplay: Node3D
var _world_probe: Node3D


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	if not ClassDB.class_exists("GameplaySessionNode"):
		_fail("GameplaySessionNode must be registered before the product scene runs")
		return
	for expectation: Dictionary in WORLD_EXPECTATIONS:
		await _check_world_scene_is_presentation_only(expectation)

	var packed := load(GAMEPLAY_SCENE) as PackedScene
	if packed == null:
		_fail("the generic gameplay scene must be loadable")
		return
	var baseline_children := root.get_child_count()
	_gameplay = packed.instantiate() as Node3D
	root.add_child(_gameplay)
	await process_frame

	for expectation: Dictionary in WORLD_EXPECTATIONS:
		await _play_world(expectation)

	_gameplay.release_level()
	await process_frame
	await process_frame
	root.remove_child(_gameplay)
	_gameplay.free()
	_gameplay = null
	await process_frame
	_check(root.get_child_count() == baseline_children,
		"returning to the menu must leave no gameplay node behind")

	if not _errors.is_empty():
		_fail(" | ".join(_errors))
		return
	print(MARKER)
	_finish(0)


func _play_world(expectation: Dictionary) -> void:
	var request_result: Dictionary = CONTROLLER.make_launch_request(
		str(expectation.world_id), str(expectation.level_id))
	if not bool(request_result.get("ok", false)):
		_check(false, "the %s campaign level must be registered" % expectation.world_id)
		return
	if not _gameplay.configure_launch(request_result.request as Dictionary):
		_check(false, "the %s launch request must configure the generic session: %s"
			% [expectation.world_id, _gameplay.last_error])
		return
	for _index: int in 4:
		await physics_frame
	var world_host := _gameplay.find_child("WorldHost", true, false) as Node3D
	_check(world_host.get_child_count() == 1 \
			and world_host.get_child(0).name == StringName(str(expectation.root_name)),
		"the generic scene must host exactly the registered %s world scene"
			% expectation.world_id)
	var frame := _gameplay.current_frame as Dictionary
	_check(str(frame.get("gravity_kind", "")) == str(expectation.gravity_kind),
		"%s must publish %s gravity" % [expectation.world_id, expectation.gravity_kind])
	var registry := _gameplay.find_child("BodyViews", true, false) as Node3D
	_check(registry.view_count() == (frame.get("snapshots", []) as Array).size() \
			and registry.view_count() > 0,
		"%s must instantiate one view per published body" % expectation.world_id)
	_check(registry.missing_asset_ids().is_empty(),
		"%s must not reference an unregistered visual" % expectation.world_id)
	if str(expectation.world_id) == "orbital":
		_check(registry.authored_view_count() > 0,
			"the Orbital world must resolve already-authored assets from the catalog")
	if not _gameplay.restart_level():
		_check(false, "%s must be restartable" % expectation.world_id)
		return
	await physics_frame
	var restarted := _gameplay.current_frame as Dictionary
	var restarted_tick := int(restarted.get("tick", -1))
	_check(restarted_tick >= 0 and restarted_tick <= 1
			and (restarted.get("projectiles", [1]) as Array).is_empty(),
		"%s must republish a reset session after a restart" % expectation.world_id)

	_gameplay.release_level()
	await process_frame
	await process_frame
	_check(world_host.get_child_count() == 0 and registry.view_count() == 0 \
			and not _gameplay.session_configured,
		"leaving %s must release its world scene and body views" % expectation.world_id)


func _check_world_scene_is_presentation_only(expectation: Dictionary) -> void:
	var packed := load(str(expectation.scene_path)) as PackedScene
	if packed == null:
		_check(false, "the %s world scene must be loadable" % expectation.world_id)
		return
	_world_probe = packed.instantiate() as Node3D
	if _world_probe == null:
		_check(false, "the %s world scene root must be a Node3D" % expectation.world_id)
		return
	root.add_child(_world_probe)
	await process_frame
	_check(_world_probe.name == StringName(str(expectation.root_name)),
		"the %s world scene must keep its registered root name" % expectation.world_id)
	_check(_scripted_node_path(_world_probe).is_empty(),
		"world scenes must not carry gameplay scripts: %s"
			% _scripted_node_path(_world_probe))
	var foreign := _foreign_class_path(_world_probe)
	_check(foreign.is_empty(),
		"world scenes must only contain presentation, lights and markers: %s" % foreign)
	root.remove_child(_world_probe)
	_world_probe.free()
	_world_probe = null


func _scripted_node_path(node: Node) -> String:
	if node.get_script() != null:
		return str(node.name)
	for child: Node in node.get_children():
		var found := _scripted_node_path(child)
		if not found.is_empty():
			return "%s/%s" % [node.name, found]
	return ""


func _foreign_class_path(node: Node) -> String:
	if not PRESENTATION_CLASSES.has(node.get_class()):
		return "%s(%s)" % [node.name, node.get_class()]
	for child: Node in node.get_children():
		var found := _foreign_class_path(child)
		if not found.is_empty():
			return "%s/%s" % [node.name, found]
	return ""


func _check(condition: bool, message: String) -> void:
	if not condition:
		_errors.append(message)


func _fail(message: String) -> void:
	push_error("two world scene smoke: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	for node: Node in [_world_probe, _gameplay]:
		if is_instance_valid(node):
			root.remove_child(node)
			node.free()
	quit(exit_code)
