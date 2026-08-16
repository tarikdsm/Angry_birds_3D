extends RefCounted

const CONTENT_FILE_LOADER := preload("res://scripts/data/content_file_loader.gd")
const ROOT_KEYS := ["schema_version", "locale", "messages"]
const FORMAT_TOKENS := {
	"app.options.toggle_format": ["%s", "%s"],
	"app.options.value_format": ["%s", "%s"],
	"app.options.binding_waiting": ["%s"],
	"app.options.camera_sensitivity_down": ["%s"],
	"app.options.camera_sensitivity_up": ["%s"],
}
const REQUIRED_MESSAGE_IDS := [
	"app.main_menu.title",
	"app.main_menu.continue",
	"app.main_menu.worlds",
	"app.main_menu.options",
	"app.main_menu.exit",
	"app.options.title",
	"app.options.back",
	"app.options.reduced_motion",
	"app.options.shake",
	"app.options.trajectory_assist",
	"app.options.ui_scale",
	"app.options.camera_sensitivity_down",
	"app.options.camera_sensitivity_up",
	"app.options.restore_defaults",
	"app.options.enabled",
	"app.options.disabled",
	"app.options.toggle_format",
	"app.options.value_format",
	"app.options.volume.master",
	"app.options.volume.music",
	"app.options.volume.ambience",
	"app.options.volume.sfx",
	"app.options.volume.ui",
	"app.options.binding.semantic_navigate_up",
	"app.options.binding.semantic_navigate_down",
	"app.options.binding.semantic_navigate_left",
	"app.options.binding.semantic_navigate_right",
	"app.options.binding.semantic_accept",
	"app.options.binding.semantic_back",
	"app.options.binding.semantic_activate_ability",
	"app.options.binding.semantic_recenter",
	"app.options.binding.semantic_pause",
	"app.options.binding.semantic_restart",
	"app.options.binding.semantic_zoom_in",
	"app.options.binding.semantic_zoom_out",
	"app.options.binding_waiting",
	"app.options.binding_conflict",
	"app.options.confirm",
	"app.options.cancel",
	"app.recovery.total",
	"input.prompt.accept.keyboard",
	"input.prompt.accept.gamepad",
	"input.prompt.accept.generic",
	"input.prompt.navigate.generic",
	"input.prompt.back.generic",
	"input.prompt.orbit.generic",
	"input.prompt.zoom.generic",
	"input.prompt.begin_grab.generic",
	"input.prompt.update_pull.generic",
	"input.prompt.release.generic",
	"input.prompt.activate_ability.generic",
	"input.prompt.recenter.generic",
	"input.prompt.pause.generic",
	"input.prompt.restart.generic",
	"input.prompt.unknown.generic",
]


static func load_catalog(path: String) -> Dictionary:
	var result := CONTENT_FILE_LOADER.load_json(path)
	if not bool(result.get("ok", false)):
		return result
	var validation_error := validate_catalog_document(result.get("document"))
	if not validation_error.is_empty():
		return {
			"ok": false,
			"error_kind": "schema",
			"message": "%s violates the product text schema: %s" % [path, validation_error],
		}
	return result


static func validate_catalog_document(document: Variant) -> String:
	if not document is Dictionary:
		return "$ must be an object"
	var catalog := document as Dictionary
	var error := _exact_keys_error(catalog, ROOT_KEYS, "$")
	if not error.is_empty():
		return error
	if not _is_integer(catalog["schema_version"]) or int(catalog["schema_version"]) != 1:
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
		if typeof(messages[message_id]) != TYPE_STRING or str(messages[message_id]).is_empty():
			return "$.messages.%s must be a non-empty string" % message_id
		if FORMAT_TOKENS.has(message_id) \
				and _format_tokens(str(messages[message_id])) != FORMAT_TOKENS[message_id]:
			return "$.messages.%s has invalid format placeholders" % message_id
	return ""


static func _format_tokens(value: String) -> Array[String]:
	var tokens: Array[String] = []
	var index := 0
	while index < value.length():
		var marker_index := value.find("%", index)
		if marker_index < 0:
			break
		if marker_index + 1 >= value.length():
			return ["invalid"]
		var token := value.substr(marker_index, 2)
		if token == "%%":
			index = marker_index + 2
			continue
		if token not in ["%s", "%d"]:
			return ["invalid"]
		tokens.append(token)
		index = marker_index + 2
	return tokens


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
