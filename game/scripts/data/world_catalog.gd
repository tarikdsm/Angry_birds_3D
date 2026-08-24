extends RefCounted

const CONTENT_FILE_LOADER := preload("res://scripts/data/content_file_loader.gd")
const CATALOG_PATH := "res://data/worlds/world_catalog.v2.json"
const WORLD_ORDER := ["earth", "orbital"]
const SCENE_IDS := ["SCN_FarmReaction", "SCN_FirstOrbitV2"]
const DIORAMA_IDS := ["WRD_EarthFarm_Diorama", "WRD_Aster_Diorama"]
const TEXT_IDS := [
	"TXT_WORLD_EARTH", "TXT_WORLD_ORBITAL",
	"TXT_LEVEL_FARM_REACTION", "TXT_LEVEL_FIRST_ORBIT_V2",
]
const ROOT_KEYS := [
	"schema_version", "default_world_id", "world_order", "scene_ids",
	"diorama_ids", "text_ids", "worlds",
]
const WORLD_KEYS := [
	"id", "diorama_id", "text_id", "default_level_id", "level_order", "levels",
]
const LEVEL_KEYS := [
	"id", "region_id", "camera_profile_id", "presentation_profile_id", "scene_id",
	"unlock_after_level_id",
]
const DEFINITION_KEYS := ["schema_version", "id", "definition_type", "scene_id"]
const WORLD_REGISTRY := {
	"earth": {
		"path": "res://data/worlds/earth.world.json",
		"scene_id": "SCN_FarmReaction",
		"diorama_id": "WRD_EarthFarm_Diorama",
		"text_id": "TXT_WORLD_EARTH",
		"level_ids": ["farm_reaction"],
		"level_text_ids": ["TXT_LEVEL_FARM_REACTION"],
		"level_region_ids": ["farm"],
		"level_camera_profile_ids": ["CAM_Farm"],
		"level_presentation_profile_ids": ["PRS_Farm"],
		"definition_type": "uniform",
	},
	"orbital": {
		"path": "res://data/worlds/orbital.world.json",
		"scene_id": "SCN_FirstOrbitV2",
		"diorama_id": "WRD_Aster_Diorama",
		"text_id": "TXT_WORLD_ORBITAL",
		"level_ids": ["first_orbit_v2"],
		"level_text_ids": ["TXT_LEVEL_FIRST_ORBIT_V2"],
		"level_region_ids": ["aster"],
		"level_camera_profile_ids": ["CAM_Orbital"],
		"level_presentation_profile_ids": ["PRS_Orbital"],
		"definition_type": "radial",
	},
}


static func load_catalog() -> Dictionary:
	var result := CONTENT_FILE_LOADER.load_json(CATALOG_PATH)
	if not bool(result.get("ok", false)):
		return result
	var error := validate_catalog_document(result.get("document"))
	if not error.is_empty():
		return {"ok": false, "error_kind": "schema", "message": error}
	for world_id: String in WORLD_ORDER:
		var definition := CONTENT_FILE_LOADER.load_json(str(WORLD_REGISTRY[world_id].path))
		if not bool(definition.get("ok", false)):
			return {"ok": false, "error_kind": "registry",
				"message": "could not load registered world: %s" % world_id}
		error = validate_world_definition_document(definition.get("document"), world_id)
		if not error.is_empty():
			return {"ok": false, "error_kind": "registry", "message": error}
	return result


static func validate_catalog_document(document: Variant) -> String:
	if not document is Dictionary:
		return "$ must be an object"
	var value := document as Dictionary
	var error := _exact_keys_error(value, ROOT_KEYS, "$")
	if not error.is_empty():
		return error
	if not _is_integer(value.schema_version) or int(value.schema_version) != 2:
		return "$.schema_version must be integer 2"
	if typeof(value.default_world_id) != TYPE_STRING or value.default_world_id != "earth":
		return "$.default_world_id must be earth"
	error = _closed_array_error(value.world_order, WORLD_ORDER, "$.world_order")
	if not error.is_empty():
		return error
	error = _closed_array_error(value.scene_ids, SCENE_IDS, "$.scene_ids")
	if not error.is_empty():
		return error
	error = _closed_array_error(value.diorama_ids, DIORAMA_IDS, "$.diorama_ids")
	if not error.is_empty():
		return error
	error = _closed_array_error(value.text_ids, TEXT_IDS, "$.text_ids")
	if not error.is_empty():
		return error
	if not value.worlds is Array or (value.worlds as Array).size() != WORLD_ORDER.size():
		return "$.worlds must match the closed world registry"
	var seen_level_ids := {}
	for index: int in WORLD_ORDER.size():
		var candidate: Variant = value.worlds[index]
		if not candidate is Dictionary:
			return "$.worlds[%d] must be an object" % index
		var world := candidate as Dictionary
		error = _exact_keys_error(world, WORLD_KEYS, "$.worlds[%d]" % index)
		if not error.is_empty():
			return error
		var world_id := str(WORLD_ORDER[index])
		var registry := WORLD_REGISTRY[world_id] as Dictionary
		if not _matches_registered_string(world.id, world_id):
			return "$.worlds[%d].id must preserve world_order" % index
		if not _matches_registered_string(world.diorama_id, registry.diorama_id) \
				or not _matches_registered_string(world.text_id, registry.text_id):
			return "$.worlds[%d] presentation IDs must be registered" % index
		error = _closed_array_error(world.level_order, registry.level_ids,
			"$.worlds[%d].level_order" % index)
		if not error.is_empty():
			return error
		if typeof(world.default_level_id) != TYPE_STRING \
				or not (world.level_order as Array).has(world.default_level_id):
			return "$.worlds[%d].default_level_id must reference level_order" % index
		if not world.levels is Array \
				or (world.levels as Array).size() != (world.level_order as Array).size():
			return "$.worlds[%d].levels must match level_order" % index
		var prior_level_ids: Array[String] = []
		for level_index: int in (world.levels as Array).size():
			var level_candidate: Variant = world.levels[level_index]
			if not level_candidate is Dictionary:
				return "$.worlds[%d].levels[%d] must be an object" % [index, level_index]
			var level := level_candidate as Dictionary
			error = _exact_keys_error(level, LEVEL_KEYS,
				"$.worlds[%d].levels[%d]" % [index, level_index])
			if not error.is_empty():
				return error
			var level_id := str(level.id)
			if level_id != str(world.level_order[level_index]) or seen_level_ids.has(level_id):
				return "level IDs must be unique and preserve level_order"
			seen_level_ids[level_id] = true
			if not _matches_registered_string(level.scene_id, registry.scene_id) \
					or not _matches_registered_string(
						level.region_id, registry.level_region_ids[level_index]) \
					or not _matches_registered_string(
						level.camera_profile_id, registry.level_camera_profile_ids[level_index]) \
					or not _matches_registered_string(level.presentation_profile_id,
						registry.level_presentation_profile_ids[level_index]):
				return "level content and presentation IDs must be registered"
			var prerequisite: Variant = level.unlock_after_level_id
			if level_index == 0:
				if prerequisite != null:
					return "first level prerequisite must be null"
			elif typeof(prerequisite) != TYPE_STRING \
					or not prior_level_ids.has(str(prerequisite)):
				return "level prerequisite must reference an earlier level"
			prior_level_ids.append(level_id)
	return ""


static func validate_world_definition_document(document: Variant, world_id: String) -> String:
	if not WORLD_REGISTRY.has(world_id):
		return "world is not registered: %s" % world_id
	if not document is Dictionary:
		return "world definition must be an object"
	var value := document as Dictionary
	var error := _exact_keys_error(value, DEFINITION_KEYS, "$world[%s]" % world_id)
	if not error.is_empty():
		return error
	if not _is_integer(value.schema_version) or int(value.schema_version) != 2:
		return "world definition schema_version must be integer 2"
	var registry := WORLD_REGISTRY[world_id] as Dictionary
	if not _matches_registered_string(value.id, world_id) \
			or not _matches_registered_string(value.definition_type, registry.definition_type) \
			or not _matches_registered_string(value.scene_id, registry.scene_id):
		return "world definition fields do not match the registry: %s" % world_id
	return ""


static func world_definition_path(world_id: String) -> String:
	return str((WORLD_REGISTRY.get(world_id, {}) as Dictionary).get("path", ""))


static func level_text_id(world_id: String, level_id: String) -> String:
	var registry := WORLD_REGISTRY.get(world_id, {}) as Dictionary
	var level_index := (registry.get("level_ids", []) as Array).find(level_id)
	if level_index < 0:
		return level_id
	return str((registry.get("level_text_ids", []) as Array)[level_index])


static func _closed_array_error(value: Variant, expected: Array, path: String) -> String:
	if not value is Array:
		return "%s must be an array" % path
	if value != expected:
		return "%s must match the closed ordered registry" % path
	return ""


static func _matches_registered_string(value: Variant, expected: Variant) -> bool:
	return typeof(value) == TYPE_STRING and typeof(expected) == TYPE_STRING \
		and str(value) == str(expected)


static func _exact_keys_error(value: Dictionary, expected: Array, path: String) -> String:
	for key: Variant in value:
		if typeof(key) != TYPE_STRING or not expected.has(str(key)):
			return "%s has unknown key: %s" % [path, key]
	for key: String in expected:
		if not value.has(key):
			return "%s has missing key: %s" % [path, key]
	return ""


static func _is_integer(value: Variant) -> bool:
	if typeof(value) == TYPE_INT:
		return true
	if typeof(value) != TYPE_FLOAT:
		return false
	var numeric := float(value)
	return is_finite(numeric) and numeric == floor(numeric)
