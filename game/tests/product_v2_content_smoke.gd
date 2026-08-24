extends SceneTree

const ASSET_CATALOG := preload("res://scripts/data/asset_catalog.gd")
const WORLD_CATALOG := preload("res://scripts/data/world_catalog.gd")
const CONTENT_FILE_LOADER := preload("res://scripts/data/content_file_loader.gd")
const CONTROLLER_PATH := "res://scripts/game/gameplay_session_controller.gd"
const CAPTURE_DRIVER_PATH := "res://scripts/game/product_v2_capture_driver.gd"
const REGISTRY_PATH := "res://scripts/game/body_view_registry_v2.gd"
const MARKER := "PRODUCT_V2_CONTENT_SMOKE_OK"

var _errors: Array[String] = []


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	var catalog_result := ASSET_CATALOG.load_catalog()
	if not bool(catalog_result.get("ok", false)):
		_fail("shipped product asset catalog must load: %s" % catalog_result.get("message", ""))
		return
	var catalog := catalog_result.document as Dictionary
	_check(ASSET_CATALOG.validate_catalog_document(catalog).is_empty(),
		"the shipped asset catalog must satisfy its own closed schema")
	_check(ASSET_CATALOG.unavailable_required_asset_ids(catalog).is_empty(),
		"every asset declared required must already resolve to an imported resource")

	_expect_catalog_invalid(_without_key(catalog, "assets"), "root missing key")
	_expect_catalog_invalid(_with_key(catalog, "root_path", "res://outside"),
		"root unknown/path injection key")
	_expect_catalog_invalid(_with_key(catalog, "schema_version", 2.5), "non-integral schema")
	_expect_catalog_invalid(_with_key(catalog, "schema_version", 3.0), "wrong schema")
	_check(ASSET_CATALOG.validate_catalog_document(
			_with_key(catalog, "schema_version", 2.0)).is_empty(),
		"JSON-normalized integral schema 2.0 must remain semantically valid")
	_expect_catalog_invalid(_with_key(catalog, "assets", []), "empty asset registry")

	var mutated := catalog.duplicate(true)
	(mutated.assets[1] as Dictionary).id = str((mutated.assets[0] as Dictionary).id)
	_expect_catalog_invalid(mutated, "duplicated asset id")
	mutated = catalog.duplicate(true)
	(mutated.assets[0] as Dictionary).erase("node_path")
	_expect_catalog_invalid(mutated, "asset missing key")
	mutated = catalog.duplicate(true)
	(mutated.assets[0] as Dictionary)["scene_path"] = "res://outside.tscn"
	_expect_catalog_invalid(mutated, "asset unknown/path injection key")
	mutated = catalog.duplicate(true)
	(mutated.assets[0] as Dictionary).kind = "sound"
	_expect_catalog_invalid(mutated, "unregistered asset kind")
	mutated = catalog.duplicate(true)
	(mutated.assets[0] as Dictionary).status = "maybe"
	_expect_catalog_invalid(mutated, "unregistered asset status")
	mutated = catalog.duplicate(true)
	(mutated.assets[0] as Dictionary).resource_path = "res://scripts/app/app_shell.gd"
	_expect_catalog_invalid(mutated, "resource path outside the asset root")
	mutated = catalog.duplicate(true)
	(mutated.assets[0] as Dictionary).resource_path = "res://assets/../secrets.glb"
	_expect_catalog_invalid(mutated, "resource path traversal")
	mutated = catalog.duplicate(true)
	(mutated.assets[0] as Dictionary).node_path = "../Escape"
	_expect_catalog_invalid(mutated, "node path traversal")
	mutated = catalog.duplicate(true)
	(mutated.assets[0] as Dictionary).presentation_only = "false"
	_expect_catalog_invalid(mutated, "non-boolean presentation flag")

	var archetypes_result := CONTENT_FILE_LOADER.load_json(
		"res://data/archetypes/product_v2.archetypes.json")
	if not bool(archetypes_result.get("ok", false)):
		_fail("product v2 archetypes must load")
		return
	var archetypes := archetypes_result.document as Dictionary
	_check(ASSET_CATALOG.missing_presentation_asset_ids(catalog, archetypes).is_empty(),
		"every archetype presentation ID must be registered in the asset catalog")

	for level_key: String in ["earth/farm_reaction", "orbital/first_orbit_v2"]:
		var request_result: Dictionary = _controller().make_launch_request(
			level_key.get_slice("/", 0), level_key.get_slice("/", 1))
		if not bool(request_result.get("ok", false)):
			_check(false, "closed campaign registry must resolve %s" % level_key)
			continue
		var request := request_result.request as Dictionary
		var level_result: Dictionary = CONTENT_FILE_LOADER.load_json(str(request.level_path))
		if not bool(level_result.get("ok", false)):
			_check(false, "registered level document must load: %s" % request.level_path)
			continue
		var level := level_result.document as Dictionary
		_check(ASSET_CATALOG.missing_visual_asset_ids(catalog, level).is_empty(),
			"every visual referenced by %s must be registered" % level_key)
		var world := _world_entry(str(request.world_id))
		var registered_level := _level_entry(world, str(request.level_id))
		_check(str(request.scene_id) == str(registered_level.get("scene_id", "")),
			"launch request scene ID must match the shipped campaign catalog")
		_check(str(request.camera_profile_id) \
				== str(registered_level.get("camera_profile_id", "")) \
			and str(request.presentation_profile_id) \
				== str(registered_level.get("presentation_profile_id", "")),
			"launch request presentation IDs must match the shipped campaign catalog")
		_check(str(level.id) == str(request.level_id) \
				and str(level.world_id) == str(request.world_id),
			"registered level document identity must match the launch request")
		var injected := level.duplicate(true)
		(injected.bodies[0] as Dictionary).visual.asset_id = "KIT_Unregistered_A"
		_check(ASSET_CATALOG.missing_visual_asset_ids(catalog, injected) \
				== ["KIT_Unregistered_A"],
			"an unregistered visual must be reported with a canonical diagnostic")

	_check(_controller().LEVEL_REGISTRY.keys() == ["earth/farm_reaction", "orbital/first_orbit_v2"],
		"the campaign launch registry must be closed and ordered")
	var unknown: Dictionary = _controller().make_launch_request("earth", "unknown_level")
	_check(not bool(unknown.get("ok", false)) and not str(unknown.get("message", "")).is_empty(),
		"an unregistered level must fail closed with a diagnostic")

	_check_capture_boundary()
	_check_capture_arguments()

	if not _errors.is_empty():
		_fail(" | ".join(_errors))
		return
	print(MARKER)
	quit(0)


func _check_capture_boundary() -> void:
	var controller_source := FileAccess.get_file_as_string(CONTROLLER_PATH)
	var driver_source := FileAccess.get_file_as_string(CAPTURE_DRIVER_PATH)
	var registry_source := FileAccess.get_file_as_string(REGISTRY_PATH)
	_check(not controller_source.is_empty() and not driver_source.is_empty() \
			and not registry_source.is_empty(),
		"product gameplay sources must be readable")
	for fragment: String in ["get_cmdline", "--product-v2", "get_tree().quit"]:
		_check(not controller_source.contains(fragment),
			"capture automation must not live in the product controller: %s" % fragment)
	_check(driver_source.contains("OS.get_cmdline_user_args"),
		"the capture driver must own command line parsing")
	_check(driver_source.contains("--product-v2-capture"),
		"the capture driver must own the product capture argument")
	_check(not registry_source.contains("preload(\"res://assets/"),
		"the v2 body view registry must not hardcode per-bird authored preloads")
	var legacy_registry := FileAccess.get_file_as_string(
		"res://scripts/game/body_view_registry.gd")
	_check(legacy_registry.contains("preload(\"res://assets/vertical_slice/"),
		"the certified v1 registry must remain untouched as a fixture")


func _check_capture_arguments() -> void:
	var driver := load(CAPTURE_DRIVER_PATH)
	var idle: Dictionary = driver.parse_arguments([])
	_check(not bool(idle.enabled) and str(idle.error).is_empty() \
			and int(idle.frame_limit) == int(driver.DEFAULT_FRAME_LIMIT),
		"an ordinary run must leave the capture driver inert")
	var unrelated: Dictionary = driver.parse_arguments(["--vertical-slice-capture"])
	_check(not bool(unrelated.enabled) and str(unrelated.error).is_empty(),
		"the legacy capture argument must never enable the product driver")
	var enabled: Dictionary = driver.parse_arguments(["--product-v2-capture"])
	_check(bool(enabled.enabled) and str(enabled.error).is_empty() \
			and int(enabled.frame_limit) == int(driver.DEFAULT_FRAME_LIMIT),
		"the product capture argument must enable the driver with the default budget")
	var budgeted: Dictionary = driver.parse_arguments(
		["--product-v2-capture", "--product-v2-capture-frames=120"])
	_check(bool(budgeted.enabled) and int(budgeted.frame_limit) == 120 \
			and str(budgeted.error).is_empty(),
		"an explicit frame budget must be honoured")
	for invalid: String in [
		"--product-v2-capture-frames=0",
		"--product-v2-capture-frames=-5",
		"--product-v2-capture-frames=abc",
		"--product-v2-capture-frames=12.5",
		"--product-v2-unknown",
	]:
		var rejected: Dictionary = driver.parse_arguments(["--product-v2-capture", invalid])
		_check(not str(rejected.error).is_empty() and not bool(rejected.enabled),
			"the capture driver must fail closed on %s" % invalid)


func _controller() -> GDScript:
	return load(CONTROLLER_PATH) as GDScript


func _world_entry(world_id: String) -> Dictionary:
	var loaded := WORLD_CATALOG.load_catalog()
	for world: Variant in (loaded.get("document", {}) as Dictionary).get("worlds", []):
		if world is Dictionary and str((world as Dictionary).get("id", "")) == world_id:
			return world as Dictionary
	return {}


func _level_entry(world: Dictionary, level_id: String) -> Dictionary:
	for level: Variant in world.get("levels", []):
		if level is Dictionary and str((level as Dictionary).get("id", "")) == level_id:
			return level as Dictionary
	return {}


func _without_key(document: Dictionary, key: String) -> Dictionary:
	var value := document.duplicate(true)
	value.erase(key)
	return value


func _with_key(document: Dictionary, key: String, replacement: Variant) -> Dictionary:
	var value := document.duplicate(true)
	value[key] = replacement
	return value


func _expect_catalog_invalid(value: Dictionary, label: String) -> void:
	_check(not ASSET_CATALOG.validate_catalog_document(value).is_empty(),
		"the asset catalog must reject %s" % label)


func _check(condition: bool, message: String) -> void:
	if not condition:
		_errors.append(message)


func _fail(message: String) -> void:
	push_error("product v2 content smoke: %s" % message)
	quit(1)
