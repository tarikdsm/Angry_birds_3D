extends RefCounted

const CONTENT_FILE_LOADER := preload("res://scripts/data/content_file_loader.gd")
const ROOT_KEYS := ["schema_version", "locale", "messages"]
const REQUIRED_MESSAGE_IDS := [
	"hud.accessibility.format",
	"hud.accessibility.full",
	"hud.accessibility.reduced",
	"hud.controls.aim",
	"hud.controls.evaluation",
	"hud.controls.faulted",
	"hud.controls.flight_ability.active",
	"hud.controls.flight_ability.armed",
	"hud.controls.flight_ability.arming",
	"hud.controls.flight_ability.rejected_not_armed",
	"hud.controls.flight_ability.spent",
	"hud.controls.flight_ability.unavailable",
	"hud.controls.flight_ability.unknown",
	"hud.controls.inspection",
	"hud.controls.loading",
	"hud.controls.resolution",
	"hud.controls.result",
	"hud.controls.unknown",
	"hud.format.birds",
	"hud.format.birds_empty",
	"hud.format.fault",
	"hud.format.objective_empty",
	"hud.format.objective_percent",
	"hud.format.phase",
	"hud.outcome.defeat",
	"hud.outcome.none",
	"hud.outcome.unknown",
	"hud.outcome.victory",
	"hud.phase.aim",
	"hud.phase.evaluation",
	"hud.phase.faulted",
	"hud.phase.flight_ability",
	"hud.phase.inspection",
	"hud.phase.loading",
	"hud.phase.resolution",
	"hud.phase.result",
	"hud.phase.unknown",
]
const FORMAT_TOKENS := {
	"hud.accessibility.format": ["%s", "%s"],
	"hud.format.birds": ["%d"],
	"hud.format.fault": ["%s", "%s"],
	"hud.format.objective_percent": ["%03d"],
	"hud.format.phase": ["%s"],
}


static func required_message_ids() -> Array[String]:
	var ids: Array[String] = []
	ids.assign(REQUIRED_MESSAGE_IDS)
	return ids


static func load_catalog(path: String) -> Dictionary:
	var result := CONTENT_FILE_LOADER.load_json(path)
	if not bool(result.get("ok", false)):
		return result
	var validation_error := validate_catalog_document(result.get("document"))
	if not validation_error.is_empty():
		return {
			"ok": false,
			"error_kind": "schema",
			"message": "%s violates the HUD text schema: %s" % [path, validation_error],
		}
	return result


static func validate_catalog_document(document: Variant) -> String:
	if not document is Dictionary:
		return "$ must be an object"
	var catalog := document as Dictionary
	var error := _exact_keys_error(catalog, ROOT_KEYS, "$")
	if not error.is_empty():
		return error
	if not _is_integer(catalog["schema_version"]) \
			or int(catalog["schema_version"]) != 1:
		return "$.schema_version must be integer 1"
	if typeof(catalog["locale"]) != TYPE_STRING or catalog["locale"] != "pt-BR":
		return "$.locale must be pt-BR"
	if not catalog["messages"] is Dictionary:
		return "$.messages must be an object"
	var messages := catalog["messages"] as Dictionary
	error = _exact_keys_error(messages, REQUIRED_MESSAGE_IDS, "$.messages")
	if not error.is_empty():
		return error
	for message_id: String in REQUIRED_MESSAGE_IDS:
		if typeof(messages[message_id]) != TYPE_STRING:
			return "$.messages.%s must be a string" % message_id
		if message_id != "hud.outcome.none" and str(messages[message_id]).is_empty():
			return "$.messages.%s must not be empty" % message_id
	for message_id: String in FORMAT_TOKENS:
		var actual_tokens := _format_tokens(str(messages[message_id]))
		var expected_tokens: Array = FORMAT_TOKENS[message_id]
		if actual_tokens != expected_tokens:
			return "$.messages.%s format tokens must be %s" % [
				message_id, JSON.stringify(expected_tokens),
			]
	return ""


static func safe_fallback_messages() -> Dictionary:
	var messages := {}
	for message_id: String in REQUIRED_MESSAGE_IDS:
		messages[message_id] = "[%s]" % message_id
		if message_id.begins_with("hud.controls."):
			messages[message_id] = "ESC / R"
	messages["hud.accessibility.format"] = "[hud.controls:%s:%s]"
	messages["hud.format.birds"] = "[hud.birds:%d]"
	messages["hud.format.birds_empty"] = "[hud.birds]"
	messages["hud.format.fault"] = "[hud.fault:%s:%s]"
	messages["hud.format.objective_empty"] = "[hud.objective]"
	messages["hud.format.objective_percent"] = "[hud.objective:%03d%%]"
	messages["hud.format.phase"] = "[hud.phase:%s]"
	messages["hud.outcome.none"] = ""
	return messages


static func _exact_keys_error(value: Dictionary, expected: Array, path: String) -> String:
	for key: Variant in value:
		if typeof(key) != TYPE_STRING or not expected.has(str(key)):
			return "%s has unknown key: %s" % [path, key]
	for key: String in expected:
		if not value.has(key):
			return "%s has missing key: %s" % [path, key]
	return ""


static func _format_tokens(value: String) -> Array[String]:
	var tokens: Array[String] = []
	var index := 0
	while index < value.length():
		if value[index] != "%":
			index += 1
			continue
		if index + 1 < value.length() and value[index + 1] == "%":
			index += 2
			continue
		if value.substr(index, 4) == "%03d":
			tokens.append("%03d")
			index += 4
			continue
		if index + 1 < value.length() and value[index + 1] in ["s", "d"]:
			tokens.append(value.substr(index, 2))
			index += 2
			continue
		tokens.append("%invalid")
		index += 1
	return tokens


static func _is_integer(value: Variant) -> bool:
	if typeof(value) == TYPE_INT:
		return true
	if typeof(value) != TYPE_FLOAT:
		return false
	var numeric := float(value)
	return is_finite(numeric) and numeric == floor(numeric)
