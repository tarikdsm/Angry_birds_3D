extends RefCounted

const CONTENT_FILE_LOADER := preload("res://scripts/data/content_file_loader.gd")
const CATALOG_PATH := "res://data/assets/product_v2.assets.json"
const RESOURCE_ROOT := "res://assets/"
const ROOT_KEYS := ["schema_version", "assets"]
const ASSET_KEYS := [
	"id", "kind", "status", "resource_path", "node_path", "presentation_only",
]
const ASSET_KINDS := [
	"animation", "character", "character_subasset", "device", "enemy",
	"fragment_subasset", "icon", "kit", "prop", "world",
]
const ASSET_STATUSES := ["planned", "required"]
const RESOURCE_EXTENSIONS := ["glb", "png", "res"]
const ID_PATTERN := "^[A-Z]{3,4}_[A-Za-z0-9_]+$"
const SEGMENT_PATTERN := "^[A-Za-z0-9_]+$"
const PRESENTATION_ARCHETYPE_FIELDS := ["projectile_visual_id", "icon_id", "animation_id"]


static func load_catalog() -> Dictionary:
	var result := CONTENT_FILE_LOADER.load_json(CATALOG_PATH)
	if not bool(result.get("ok", false)):
		return result
	var error := validate_catalog_document(result.get("document"))
	if not error.is_empty():
		return {"ok": false, "error_kind": "schema", "message": error}
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
	if not value.assets is Array or (value.assets as Array).is_empty():
		return "$.assets must be a non-empty array"
	var id_matcher := RegEx.create_from_string(ID_PATTERN)
	var segment_matcher := RegEx.create_from_string(SEGMENT_PATTERN)
	var seen_ids := {}
	for index: int in (value.assets as Array).size():
		var path := "$.assets[%d]" % index
		var candidate: Variant = value.assets[index]
		if not candidate is Dictionary:
			return "%s must be an object" % path
		var asset := candidate as Dictionary
		error = _exact_keys_error(asset, ASSET_KEYS, path)
		if not error.is_empty():
			return error
		if typeof(asset.id) != TYPE_STRING or id_matcher.search(str(asset.id)) == null:
			return "%s.id must be a registered asset identifier" % path
		if seen_ids.has(str(asset.id)):
			return "%s.id is duplicated: %s" % [path, asset.id]
		seen_ids[str(asset.id)] = true
		if typeof(asset.kind) != TYPE_STRING or not ASSET_KINDS.has(str(asset.kind)):
			return "%s.kind must be one of the registered kinds" % path
		if typeof(asset.status) != TYPE_STRING or not ASSET_STATUSES.has(str(asset.status)):
			return "%s.status must be one of the registered statuses" % path
		error = _resource_path_error(asset.resource_path, "%s.resource_path" % path)
		if not error.is_empty():
			return error
		error = _node_path_error(asset.node_path, "%s.node_path" % path, segment_matcher)
		if not error.is_empty():
			return error
		if typeof(asset.presentation_only) != TYPE_BOOL:
			return "%s.presentation_only must be a boolean" % path
	return ""


static func index_by_id(document: Dictionary) -> Dictionary:
	var index := {}
	for asset: Variant in document.get("assets", []):
		if asset is Dictionary:
			index[str((asset as Dictionary).get("id", ""))] = (asset as Dictionary).duplicate(true)
	return index


static func unavailable_required_asset_ids(document: Dictionary) -> Array[String]:
	var unavailable: Array[String] = []
	for asset: Variant in document.get("assets", []):
		if not asset is Dictionary:
			continue
		var entry := asset as Dictionary
		if str(entry.get("status", "")) != "required":
			continue
		if not ResourceLoader.exists(str(entry.get("resource_path", ""))):
			unavailable.append(str(entry.get("id", "")))
	unavailable.sort()
	return unavailable


static func level_visual_asset_ids(level_document: Dictionary) -> Array[String]:
	var referenced := {}
	var slingshot := level_document.get("slingshot", {}) as Dictionary
	var slingshot_asset := str(slingshot.get("asset_id", ""))
	if not slingshot_asset.is_empty():
		referenced[slingshot_asset] = true
	for body: Variant in level_document.get("bodies", []):
		if not body is Dictionary:
			continue
		var visual := (body as Dictionary).get("visual", {}) as Dictionary
		var asset_id := str(visual.get("asset_id", ""))
		if not asset_id.is_empty():
			referenced[asset_id] = true
		# Fracture fragments become real bodies in flight and carry their own
		# visual. Leaving them out of the referenced set lets an unregistered
		# fragment pass the gate and the launch, and show up only as invisible
		# debris mid-shot.
		var pattern := (body as Dictionary).get("fracture_pattern", {}) as Dictionary
		for fragment: Variant in pattern.get("physical_fragments", []):
			if not fragment is Dictionary:
				continue
			var fragment_id := str((fragment as Dictionary).get("visual_id", ""))
			if not fragment_id.is_empty():
				referenced[fragment_id] = true
	return _sorted_ids(referenced)


static func missing_visual_asset_ids(
		document: Dictionary, level_document: Dictionary) -> Array[String]:
	var index := index_by_id(document)
	var missing := {}
	for asset_id: String in level_visual_asset_ids(level_document):
		if not index.has(asset_id):
			missing[asset_id] = true
	return _sorted_ids(missing)


static func missing_presentation_asset_ids(
		document: Dictionary, archetypes_document: Dictionary) -> Array[String]:
	var index := index_by_id(document)
	var missing := {}
	for declared: Variant in archetypes_document.get("presentation_ids", []):
		if not index.has(str(declared)):
			missing[str(declared)] = true
	for bird: Variant in archetypes_document.get("birds", []):
		if not bird is Dictionary:
			continue
		for field: String in PRESENTATION_ARCHETYPE_FIELDS:
			var asset_id := str((bird as Dictionary).get(field, ""))
			if not asset_id.is_empty() and not index.has(asset_id):
				missing[asset_id] = true
	return _sorted_ids(missing)


static func _sorted_ids(unique: Dictionary) -> Array[String]:
	var result: Array[String] = []
	for asset_id: Variant in unique.keys():
		result.append(str(asset_id))
	result.sort()
	return result


static func _resource_path_error(value: Variant, path: String) -> String:
	if typeof(value) != TYPE_STRING:
		return "%s must be a string" % path
	var resource_path := str(value)
	if not resource_path.begins_with(RESOURCE_ROOT):
		return "%s must stay inside %s" % [path, RESOURCE_ROOT]
	var relative := resource_path.substr(RESOURCE_ROOT.length())
	if relative.is_empty() or relative.contains("//") or relative.begins_with("/"):
		return "%s must be a canonical resource path" % path
	for segment: String in relative.split("/"):
		if segment.is_empty() or segment == "." or segment == "..":
			return "%s must not contain relative segments" % path
	if not RESOURCE_EXTENSIONS.has(resource_path.get_extension().to_lower()):
		return "%s must use a registered resource extension" % path
	return ""


static func _node_path_error(value: Variant, path: String, matcher: RegEx) -> String:
	if typeof(value) != TYPE_STRING:
		return "%s must be a string" % path
	var node_path := str(value)
	if node_path.is_empty() or node_path.begins_with("/"):
		return "%s must be a relative node path" % path
	for segment: String in node_path.split("/"):
		if matcher.search(segment) == null:
			return "%s must only contain simple node names" % path
	return ""


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
