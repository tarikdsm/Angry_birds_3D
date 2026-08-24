extends SceneTree

const PRODUCT_TEXT := preload("res://scripts/data/product_text_catalog.gd")
const WORLD_CATALOG := preload("res://scripts/data/world_catalog.gd")
const MARKER := "PRODUCT_V2_CATALOG_SMOKE_OK"

var _errors: Array[String] = []


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	var text_result := PRODUCT_TEXT.load_catalog("res://data/ui/product_v2.pt-BR.json")
	var narrative_result := PRODUCT_TEXT.load_narrative_catalog(
		"res://data/narrative/product_v2.pt-BR.json")
	var world_result := WORLD_CATALOG.load_catalog()
	if not bool(text_result.get("ok", false)) or not bool(narrative_result.get("ok", false)) \
			or not bool(world_result.get("ok", false)):
		_fail("shipped closed product catalogs must load")
		return
	var world := world_result.document as Dictionary
	var narrative := narrative_result.document as Dictionary
	_check(world.scene_ids == ["SCN_FarmReaction", "SCN_FirstOrbitV2"],
		"scene registry must be closed and ordered")
	_check(world.diorama_ids == ["WRD_EarthFarm_Diorama", "WRD_Aster_Diorama"],
		"diorama registry must be closed and ordered")
	_check(world.text_ids == ["TXT_WORLD_EARTH", "TXT_WORLD_ORBITAL",
		"TXT_LEVEL_FARM_REACTION", "TXT_LEVEL_FIRST_ORBIT_V2"],
		"world and phase text registry must be closed and ordered")
	var shipped_earth_level := world.worlds[0].levels[0] as Dictionary
	_check(_has_exact_keys(shipped_earth_level, [
		"id", "region_id", "camera_profile_id", "presentation_profile_id",
		"scene_id", "unlock_after_level_id",
	]), "shipped campaign levels must preserve the native closed six-field contract")
	_check(not shipped_earth_level.has("text_id"),
		"frontend phase localization must not extend the native campaign level shape")
	_check((WORLD_CATALOG.WORLD_REGISTRY.earth.level_text_ids as Array) \
			== ["TXT_LEVEL_FARM_REACTION"],
		"frontend registry must resolve Earth phase text independently of campaign JSON")
	_check((WORLD_CATALOG.WORLD_REGISTRY.orbital.level_text_ids as Array) \
			== ["TXT_LEVEL_FIRST_ORBIT_V2"],
		"frontend registry must resolve Orbital phase text independently of campaign JSON")

	var mutated := world.duplicate(true)
	mutated.erase("scene_ids")
	_expect_world_invalid(mutated, "campaign root missing key")
	mutated = world.duplicate(true)
	mutated["definition_path"] = "res://outside.json"
	_expect_world_invalid(mutated, "campaign root unknown/path injection key")
	mutated = world.duplicate(true)
	mutated.schema_version = 2.5
	_expect_world_invalid(mutated, "non-integral campaign schema")
	mutated = world.duplicate(true)
	mutated.schema_version = 3.0
	_expect_world_invalid(mutated, "wrong campaign schema")
	mutated = world.duplicate(true)
	mutated.schema_version = 2.0
	_check(WORLD_CATALOG.validate_catalog_document(mutated).is_empty(),
		"JSON-normalized integral schema 2.0 must remain semantically valid")
	mutated = world.duplicate(true)
	(mutated.worlds[0] as Dictionary).erase("text_id")
	_expect_world_invalid(mutated, "world missing key")
	mutated = world.duplicate(true)
	(mutated.worlds[0] as Dictionary)["path"] = "res://outside.json"
	_expect_world_invalid(mutated, "world unknown/path key")
	mutated = world.duplicate(true)
	(mutated.worlds[0].levels[0] as Dictionary).erase("scene_id")
	_expect_world_invalid(mutated, "level missing key")
	mutated = world.duplicate(true)
	(mutated.worlds[0].levels[0] as Dictionary)["path"] = "res://outside.json"
	_expect_world_invalid(mutated, "level unknown/path key")
	mutated = world.duplicate(true)
	(mutated.worlds[0].levels[0] as Dictionary)["text_id"] = "TXT_LEVEL_FARM_REACTION"
	_expect_world_invalid(mutated, "frontend-only level text key")
	for field: String in ["scene_ids", "diorama_ids", "text_ids"]:
		mutated = world.duplicate(true)
		(mutated[field] as Array).reverse()
		_expect_world_invalid(mutated, "%s order" % field)
		mutated = world.duplicate(true)
		(mutated[field] as Array)[1] = (mutated[field] as Array)[0]
		_expect_world_invalid(mutated, "%s uniqueness" % field)
	mutated = world.duplicate(true)
	mutated.default_world_id = "orbital"
	_expect_world_invalid(mutated, "default world invariant")
	mutated = world.duplicate(true)
	mutated.worlds[0].diorama_id = "WRD_UNKNOWN"
	_expect_world_invalid(mutated, "world diorama registry cross-reference")
	mutated = world.duplicate(true)
	mutated.worlds[0].text_id = "TXT_UNKNOWN"
	_expect_world_invalid(mutated, "world text registry cross-reference")
	mutated = world.duplicate(true)
	mutated.worlds[0].default_level_id = "unknown"
	_expect_world_invalid(mutated, "default level cross-reference")
	mutated = world.duplicate(true)
	mutated.worlds[0].level_order = ["unknown"]
	_expect_world_invalid(mutated, "level order cross-reference")
	mutated = world.duplicate(true)
	mutated.worlds[1].levels[0].id = "farm_reaction"
	_expect_world_invalid(mutated, "globally duplicate level id")
	mutated = world.duplicate(true)
	mutated.worlds[0].levels[0].scene_id = "SCN_UNKNOWN"
	_expect_world_invalid(mutated, "level scene registry cross-reference")
	mutated = world.duplicate(true)
	mutated.worlds[0].levels[0].region_id = "aster"
	_expect_world_invalid(mutated, "level region registry cross-reference")
	mutated = world.duplicate(true)
	mutated.worlds[0].levels[0].region_id = 7
	_expect_world_invalid(mutated, "level region type")
	mutated = world.duplicate(true)
	mutated.worlds[0].levels[0].camera_profile_id = "CAM_Orbital"
	_expect_world_invalid(mutated, "level camera profile registry cross-reference")
	mutated = world.duplicate(true)
	mutated.worlds[0].levels[0].camera_profile_id = false
	_expect_world_invalid(mutated, "level camera profile type")
	mutated = world.duplicate(true)
	mutated.worlds[0].levels[0].presentation_profile_id = "PRS_Orbital"
	_expect_world_invalid(mutated, "level presentation profile registry cross-reference")
	mutated = world.duplicate(true)
	mutated.worlds[0].levels[0].presentation_profile_id = []
	_expect_world_invalid(mutated, "level presentation profile type")
	mutated = world.duplicate(true)
	mutated.worlds[0].levels[0].unlock_after_level_id = "unknown"
	_expect_world_invalid(mutated, "level prerequisite cross-reference")

	var earth: Dictionary = JSON.parse_string(FileAccess.get_file_as_string(
		"res://data/worlds/earth.world.json")) as Dictionary
	var definition := earth.duplicate(true)
	definition.erase("scene_id")
	_expect_definition_invalid(definition, "earth", "world definition missing key")
	definition = earth.duplicate(true)
	definition["path"] = "res://outside.json"
	_expect_definition_invalid(definition, "earth", "world definition unknown/path key")
	definition = earth.duplicate(true)
	definition.schema_version = 2.5
	_expect_definition_invalid(definition, "earth", "world definition non-integral schema")
	definition = earth.duplicate(true)
	definition.schema_version = 3.0
	_expect_definition_invalid(definition, "earth", "world definition wrong schema")
	definition = earth.duplicate(true)
	definition.schema_version = 2.0
	_check(WORLD_CATALOG.validate_world_definition_document(definition, "earth").is_empty(),
		"JSON-normalized world schema 2.0 must remain semantically valid")

	var narrative_mutation := narrative.duplicate(true)
	narrative_mutation.erase("locale")
	_expect_narrative_invalid(narrative_mutation, "narrative root missing key")
	narrative_mutation = narrative.duplicate(true)
	narrative_mutation["unknown"] = true
	_expect_narrative_invalid(narrative_mutation, "narrative root unknown key")
	narrative_mutation = narrative.duplicate(true)
	narrative_mutation.schema_version = 1.5
	_expect_narrative_invalid(narrative_mutation, "narrative non-integral schema")
	narrative_mutation = narrative.duplicate(true)
	narrative_mutation.schema_version = 2.0
	_expect_narrative_invalid(narrative_mutation, "narrative wrong schema")
	narrative_mutation = narrative.duplicate(true)
	narrative_mutation.schema_version = 1.0
	_check(PRODUCT_TEXT.validate_narrative_catalog_document(narrative_mutation).is_empty(),
		"JSON-normalized narrative schema 1.0 must remain semantically valid")
	narrative_mutation = narrative.duplicate(true)
	(narrative_mutation.briefings.BRF_Farm as Dictionary).erase("lines")
	_expect_narrative_invalid(narrative_mutation, "briefing missing key")
	narrative_mutation = narrative.duplicate(true)
	(narrative_mutation.briefings.BRF_Farm as Dictionary)["speaker"] = "x"
	_expect_narrative_invalid(narrative_mutation, "briefing unknown key")
	for invalid_line: String in ["   ", "linha um\nlinha dois", "linha um\rlinha dois"]:
		narrative_mutation = narrative.duplicate(true)
		narrative_mutation.briefings.BRF_Farm.lines[0] = invalid_line
		_expect_narrative_invalid(narrative_mutation,
			"blank or embedded-newline briefing line")

	var malformed_text := (text_result.document as Dictionary).duplicate(true)
	malformed_text.messages.erase("app.worlds.record_format")
	_check(not PRODUCT_TEXT.validate_catalog_document(malformed_text).is_empty(),
		"UI catalog must require the closed localized record format")
	malformed_text = (text_result.document as Dictionary).duplicate(true)
	malformed_text.messages["app.worlds.record_format"] = "%s %s %d"
	_check(not PRODUCT_TEXT.validate_catalog_document(malformed_text).is_empty(),
		"UI catalog must reject an incorrect record format token sequence")
	_check(str(text_result.document.messages.TXT_LEVEL_FARM_REACTION) \
			== "Fazenda — Reação em Cadeia",
		"Earth phase copy must match the normative pt-BR catalog exactly")
	_check(str(text_result.document.messages.TXT_LEVEL_FIRST_ORBIT_V2) \
			== "Primeira Órbita — Contrapeso de Aster",
		"Orbital phase copy must match the normative pt-BR catalog exactly")

	if not _errors.is_empty():
		_fail(" | ".join(_errors))
		return
	print(MARKER)
	quit(0)


func _expect_world_invalid(value: Dictionary, label: String) -> void:
	_check(not WORLD_CATALOG.validate_catalog_document(value).is_empty(),
		"world catalog must reject %s" % label)


func _expect_definition_invalid(value: Dictionary, world_id: String, label: String) -> void:
	_check(not WORLD_CATALOG.validate_world_definition_document(value, world_id).is_empty(),
		"world catalog must reject %s" % label)


func _expect_narrative_invalid(value: Dictionary, label: String) -> void:
	_check(not PRODUCT_TEXT.validate_narrative_catalog_document(value).is_empty(),
		"narrative catalog must reject %s" % label)


func _has_exact_keys(value: Dictionary, expected: Array[String]) -> bool:
	var actual: Array[String] = []
	for key: Variant in value.keys():
		actual.append(str(key))
	actual.sort()
	var wanted := expected.duplicate()
	wanted.sort()
	return actual == wanted


func _check(condition: bool, message: String) -> void:
	if not condition:
		_errors.append(message)


func _fail(message: String) -> void:
	push_error("product v2 catalog smoke: %s" % message)
	quit(1)
