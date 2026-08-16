extends RefCounted

const SCHEMA_VERSION := 2
const ROOT_KEYS := [
	"schema_version", "profile_id", "available_world_ids", "available_level_ids",
	"levels", "seen_briefing_ids", "seen_tutorial_ids", "last_world_id", "last_level_id",
]
const RECORD_KEYS := [
	"completed", "best_score", "best_stars", "best_birds_used", "completion_count",
]


static func default_document(world_catalog: Dictionary) -> Dictionary:
	var world_ids: Array = []
	var records := {}
	for world_id: Variant in world_catalog.get("world_order", []):
		world_ids.append(str(world_id))
		var world := _find_world(world_catalog, str(world_id))
		for level_id: Variant in world.get("level_order", []):
			var qualified_id := "%s/%s" % [world_id, level_id]
			records[qualified_id] = {
				"completed": false,
				"best_score": 0,
				"best_stars": 0,
				"best_birds_used": 0,
				"completion_count": 0,
			}
	var level_ids := _available_level_ids(world_catalog, records)
	var default_world_id := str(world_catalog.get("default_world_id", ""))
	var default_world := _find_world(world_catalog, default_world_id)
	return {
		"schema_version": SCHEMA_VERSION,
		"profile_id": "default",
		"available_world_ids": world_ids,
		"available_level_ids": level_ids,
		"levels": records,
		"seen_briefing_ids": [],
		"seen_tutorial_ids": [],
		"last_world_id": default_world_id,
		"last_level_id": str(default_world.get("default_level_id", "")),
	}


static func validate_document(document: Variant, world_catalog: Dictionary) -> String:
	if not document is Dictionary:
		return "$ must be an object"
	var value := document as Dictionary
	var error := _exact_keys_error(value, ROOT_KEYS, "$")
	if not error.is_empty():
		return error
	if not _is_int(value.schema_version) or int(value.schema_version) != SCHEMA_VERSION:
		return "$.schema_version must be integer 2"
	if typeof(value.profile_id) != TYPE_STRING or str(value.profile_id).is_empty():
		return "$.profile_id must be a non-empty string"
	var canonical := default_document(world_catalog)
	if not value.available_world_ids is Array \
			or value.available_world_ids != canonical.available_world_ids:
		return "$.available_world_ids must match the world catalog"
	if not value.available_level_ids is Array:
		return "$.available_level_ids must be an array"
	if not value.levels is Dictionary:
		return "$.levels must be an object"
	error = _exact_keys_error(value.levels, canonical.levels.keys(), "$.levels")
	if not error.is_empty():
		return error
	for level_id: Variant in canonical.levels:
		var record: Variant = value.levels[level_id]
		if not record is Dictionary:
			return "$.levels.%s must be an object" % level_id
		error = _exact_keys_error(record, RECORD_KEYS, "$.levels.%s" % level_id)
		if not error.is_empty():
			return error
		if typeof(record.completed) != TYPE_BOOL:
			return "$.levels.%s.completed must be boolean" % level_id
		for field: String in ["best_score", "best_stars", "best_birds_used", "completion_count"]:
			if not _is_int(record[field]) or int(record[field]) < 0:
				return "$.levels.%s.%s must be a non-negative integer" % [level_id, field]
		if int(record.best_stars) > 3:
			return "$.levels.%s.best_stars must be at most 3" % level_id
		if bool(record.completed):
			if int(record.best_stars) < 1 or int(record.best_birds_used) < 1 \
					or int(record.completion_count) < 1:
				return "$.levels.%s completed records must be positive" % level_id
		elif int(record.best_score) != 0 or int(record.best_stars) != 0 \
				or int(record.best_birds_used) != 0 or int(record.completion_count) != 0:
			return "$.levels.%s incomplete records must be empty" % level_id
	var expected_available := _available_level_ids(world_catalog, value.levels)
	if value.available_level_ids != expected_available:
		return "$.available_level_ids must match completed catalog prerequisites"
	for level_id: Variant in value.levels:
		if bool((value.levels[level_id] as Dictionary).completed) \
				and not expected_available.has(str(level_id)):
			return "$.levels.%s cannot be completed while locked" % level_id
	for array_key: String in ["seen_briefing_ids", "seen_tutorial_ids"]:
		if not value[array_key] is Array:
			return "$.%s must be an array" % array_key
		for item: Variant in value[array_key]:
			if typeof(item) != TYPE_STRING or str(item).is_empty():
				return "$.%s entries must be non-empty strings" % array_key
	if typeof(value.last_world_id) != TYPE_STRING \
			or not (value.available_world_ids as Array).has(value.last_world_id):
		return "$.last_world_id must reference an available world"
	if typeof(value.last_level_id) != TYPE_STRING \
			or not (value.available_level_ids as Array).has(
				"%s/%s" % [value.last_world_id, value.last_level_id]):
		return "$.last_level_id must reference a level in last_world_id"
	return ""


static func apply_result(
		document: Dictionary, result: Dictionary, world_catalog: Dictionary) -> Dictionary:
	var validation_error := validate_document(document, world_catalog)
	if not validation_error.is_empty():
		return {"ok": false, "changed": false, "message": validation_error,
			"document": document.duplicate(true)}
	var outcome := str(result.get("outcome", ""))
	if outcome not in ["victory", "defeat"]:
		return {"ok": false, "changed": false, "message": "unknown result outcome",
			"document": document.duplicate(true)}
	if outcome == "defeat":
		return {"ok": true, "changed": false, "document": document.duplicate(true)}
	var qualified_id := "%s/%s" % [result.get("world_id", ""), result.get("level_id", "")]
	if not (document.levels as Dictionary).has(qualified_id):
		return {"ok": false, "changed": false, "message": "unknown level result",
			"document": document.duplicate(true)}
	if not (document.available_level_ids as Array).has(qualified_id):
		return {"ok": false, "changed": false, "message": "locked level result",
			"document": document.duplicate(true)}
	for field: String in ["score", "stars", "birds_used"]:
		if not result.has(field) or not _is_int(result[field]) or int(result[field]) < 0:
			return {"ok": false, "changed": false, "message": "invalid %s" % field,
				"document": document.duplicate(true)}
	if int(result.stars) < 1 or int(result.stars) > 3 or int(result.birds_used) < 1:
		return {"ok": false, "changed": false, "message": "invalid victory record",
			"document": document.duplicate(true)}
	var updated := document.duplicate(true)
	var record := (updated.levels as Dictionary)[qualified_id] as Dictionary
	record.completed = true
	record.best_score = maxi(int(record.best_score), int(result.score))
	record.best_stars = maxi(int(record.best_stars), int(result.stars))
	var old_birds := int(record.best_birds_used)
	record.best_birds_used = int(result.birds_used) if old_birds == 0 else mini(
		old_birds, int(result.birds_used))
	record.completion_count = int(record.completion_count) + 1
	updated.available_level_ids = _available_level_ids(world_catalog, updated.levels)
	updated.last_world_id = str(result.world_id)
	updated.last_level_id = str(result.level_id)
	return {"ok": true, "changed": true, "document": updated}


static func _find_world(world_catalog: Dictionary, world_id: String) -> Dictionary:
	for candidate: Variant in world_catalog.get("worlds", []):
		if candidate is Dictionary and str(candidate.get("id", "")) == world_id:
			return candidate as Dictionary
	return {}


static func _find_level(world: Dictionary, level_id: String) -> Dictionary:
	for candidate: Variant in world.get("levels", []):
		if candidate is Dictionary and str(candidate.get("id", "")) == level_id:
			return candidate as Dictionary
	return {}


static func _available_level_ids(world_catalog: Dictionary, records: Dictionary) -> Array:
	var available: Array = []
	var available_set := {}
	for world_id: Variant in world_catalog.get("world_order", []):
		var world := _find_world(world_catalog, str(world_id))
		for level_id: Variant in world.get("level_order", []):
			var level := _find_level(world, str(level_id))
			if level.is_empty():
				continue
			var prerequisite: Variant = level.get("unlock_after_level_id")
			if prerequisite == null:
				var qualified_id := "%s/%s" % [world_id, level_id]
				available.append(qualified_id)
				available_set[qualified_id] = true
				continue
			var prerequisite_id := "%s/%s" % [world_id, prerequisite]
			var prerequisite_record: Variant = records.get(prerequisite_id)
			if available_set.has(prerequisite_id) and prerequisite_record is Dictionary \
					and bool(prerequisite_record.get("completed", false)):
				var qualified_id := "%s/%s" % [world_id, level_id]
				available.append(qualified_id)
				available_set[qualified_id] = true
	return available


static func _exact_keys_error(value: Dictionary, expected: Array, path: String) -> String:
	for key: Variant in value:
		if typeof(key) != TYPE_STRING or not expected.has(str(key)):
			return "%s has unknown key: %s" % [path, key]
	for key: Variant in expected:
		if not value.has(str(key)):
			return "%s has missing key: %s" % [path, key]
	return ""


static func _is_int(value: Variant) -> bool:
	if typeof(value) == TYPE_INT:
		return true
	return typeof(value) == TYPE_FLOAT and is_finite(float(value)) \
		and float(value) == floor(float(value))
