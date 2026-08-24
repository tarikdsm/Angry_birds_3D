extends SceneTree

const ASSET_CATALOG := preload("res://scripts/data/asset_catalog.gd")
const CONTROLLER := preload("res://scripts/game/gameplay_session_controller.gd")
const BODY_VIEW_REGISTRY := preload("res://scripts/game/body_view_registry_v2.gd")
const GAMEPLAY_SCENE := "res://scenes/gameplay/gameplay_session.tscn"
const RESTART_COUNT := 20
const MARKER := "EARTH_SESSION_SMOKE_OK"

var _errors: Array[String] = []
var _gameplay: Node3D
var _standalone_registry: Node3D
var _reported_missing: Array[String] = []


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	if not ClassDB.class_exists("GameplaySessionNode"):
		_fail("GameplaySessionNode must be registered before the product scene runs")
		return
	var request_result: Dictionary = CONTROLLER.make_launch_request("earth", "farm_reaction")
	if not bool(request_result.get("ok", false)):
		_fail("the Earth campaign level must be registered")
		return
	var request := request_result.request as Dictionary

	var packed := load(GAMEPLAY_SCENE) as PackedScene
	if packed == null:
		_fail("the generic gameplay scene must be loadable")
		return
	_gameplay = packed.instantiate() as Node3D
	if _gameplay == null:
		_fail("the generic gameplay scene root must be a Node3D")
		return
	root.add_child(_gameplay)
	await process_frame

	_check(_gameplay.configure_calls == 0 and not _gameplay.session_configured,
		"an unlaunched gameplay scene must stay inert")
	var tampered := request.duplicate(true)
	tampered.level_path = "res://data/levels/first_orbit.level.json"
	_check(not _gameplay.configure_launch(tampered),
		"a launch request with an unregistered document path must fail closed")
	_check(_gameplay.configure_calls == 0 and not _gameplay.last_error.is_empty(),
		"a rejected launch request must not reach the kernel and must diagnose itself")

	if not _gameplay.configure_launch(request):
		_fail("the Earth launch request must configure the generic session: %s"
			% _gameplay.last_error)
		return
	_check(_gameplay.loaded_document_paths == [
			"res://data/materials/product_v2.materials.json",
			"res://data/archetypes/product_v2.archetypes.json",
			"res://data/levels/earth/farm_reaction.level.json",
		],
		"the controller must load the three registered documents in canonical order")
	_check(_gameplay.configure_calls == 1 and _gameplay.session_configured,
		"configure_session must be called exactly once per launch")

	await physics_frame
	var baseline: int = _gameplay.consume_calls
	for _index: int in 5:
		await physics_frame
	_check(_gameplay.consume_calls - baseline == 5,
		"consume_frame must run exactly once per physics frame")
	_check(_gameplay.configure_calls == 1,
		"running physics frames must never reconfigure the session")

	var frame := _gameplay.current_frame as Dictionary
	_check(int(frame.get("frame_schema_version", 0)) == 3 \
			and str(frame.get("gravity_kind", "")) == "uniform",
		"the Earth level must publish uniform gravity through frame schema 3")
	var registry := _gameplay.find_child("BodyViews", true, false) as Node3D
	var snapshots := frame.get("snapshots", []) as Array
	_check(snapshots.size() > 100, "the Fazenda level must publish its authored body set")
	_check(registry.view_count() == snapshots.size(),
		"the registry must instantiate one view per published body")
	var expected_ids := {}
	for snapshot: Dictionary in snapshots:
		expected_ids[str(snapshot.get("visual_id", ""))] = true
	var resolved_ids := {}
	for view: Node3D in registry.views():
		resolved_ids[str(view.get_meta("asset_id", ""))] = true
	_check(resolved_ids.keys().size() == expected_ids.keys().size(),
		"views must be instantiated by catalog visual_id, not by hardcoded archetype")
	_check(registry.missing_asset_ids().is_empty(),
		"the shipped Earth level must not reference an unregistered visual")

	var nodes_before := _count_nodes(_gameplay)
	var views_before: int = registry.view_count()
	var tick_before := int(frame.get("tick", 0))
	_check(tick_before >= 4, "the session must have advanced before the restart budget")
	var restarted: Dictionary = {}
	for index: int in RESTART_COUNT:
		if not _gameplay.restart_level():
			_check(false, "restart_level must succeed on a configured session")
			break
		await physics_frame
		if index == RESTART_COUNT - 1:
			restarted = _gameplay.current_frame as Dictionary
	await process_frame
	await process_frame
	_check(_gameplay.restart_calls == RESTART_COUNT,
		"every restart must be observable on the product controller")
	_check(_count_nodes(_gameplay) == nodes_before,
		"%d restarts must not grow the live node count" % RESTART_COUNT)
	_check(registry.view_count() == views_before,
		"%d restarts must not grow the live body view count" % RESTART_COUNT)
	_check(_gameplay.configure_calls == 1,
		"restarting must never reconfigure the kernel session")
	var restarted_tick := int(restarted.get("tick", -1))
	_check(restarted_tick >= 0 and restarted_tick < tick_before and restarted_tick <= 2
			and (restarted.get("projectiles", [1]) as Array).is_empty()
			and (restarted.get("events", [1]) as Array).is_empty(),
		"a restart must republish a reset session")

	await _check_missing_asset_diagnostic()

	if not _errors.is_empty():
		_fail(" | ".join(_errors))
		return
	print(MARKER)
	_finish(0)


func _check_missing_asset_diagnostic() -> void:
	var catalog_result := ASSET_CATALOG.load_catalog()
	if not bool(catalog_result.get("ok", false)):
		_check(false, "the asset catalog must load for the diagnostic probe")
		return
	_standalone_registry = BODY_VIEW_REGISTRY.new()
	root.add_child(_standalone_registry)
	_standalone_registry.asset_resolution_failed.connect(_on_asset_resolution_failed)
	_check(_standalone_registry.configure(catalog_result.document as Dictionary),
		"the registry must accept the validated asset catalog")
	_standalone_registry.apply_frame({
		"phase": "inspection",
		"snapshots": [{
			"entity_id": 9001,
			"part_id": 1,
			"visual_id": "KIT_Unregistered_A",
			"transform": Transform3D.IDENTITY,
			"shape": {"type": "box", "half_extents": Vector3(0.5, 0.5, 0.5),
				"radius": 0.5, "half_height": 0.5},
		}],
		"trajectory_preview": null,
	})
	await process_frame
	_check(_standalone_registry.view_count() == 0,
		"an unregistered visual must never produce a view")
	_check(_standalone_registry.missing_asset_ids() == ["KIT_Unregistered_A"],
		"an unregistered visual must be reported by ID")
	_check(_reported_missing == ["KIT_Unregistered_A"],
		"an unregistered visual must raise exactly one diagnostic signal")
	_check(_standalone_registry.diagnostics().size() == 1 \
			and str(_standalone_registry.diagnostics()[0]).contains("KIT_Unregistered_A"),
		"the diagnostic must name the unresolved asset")


func _on_asset_resolution_failed(asset_id: String, _message: String) -> void:
	_reported_missing.append(asset_id)


func _count_nodes(node: Node) -> int:
	var total := 1
	for child: Node in node.get_children():
		total += _count_nodes(child)
	return total


func _check(condition: bool, message: String) -> void:
	if not condition:
		_errors.append(message)


func _fail(message: String) -> void:
	push_error("earth session smoke: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	for node: Node in [_standalone_registry, _gameplay]:
		if is_instance_valid(node):
			root.remove_child(node)
			node.free()
	quit(exit_code)
