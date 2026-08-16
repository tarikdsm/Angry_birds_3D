extends RefCounted

const SCHEMA_VERSION := 2
const ROOT_KEYS := [
	"schema_version", "volumes", "reduced_motion", "shake", "trajectory_assist",
	"ui_scale_percent", "camera_sensitivity", "bindings",
]
const VOLUME_KEYS := ["master", "music", "ambience", "sfx", "ui"]
const BINDING_KEYS := ["action", "context", "intent", "variant", "tokens"]
const UI_SCALES := [100, 125, 150, 200]


static func default_document(default_bindings: Array) -> Dictionary:
	return {
		"schema_version": SCHEMA_VERSION,
		"volumes": {"master": 1.0, "music": 1.0, "ambience": 1.0, "sfx": 1.0, "ui": 1.0},
		"reduced_motion": false,
		"shake": true,
		"trajectory_assist": false,
		"ui_scale_percent": 100,
		"camera_sensitivity": 1.0,
		"bindings": default_bindings.duplicate(true),
	}


static func validate_document(document: Variant, specs: Array) -> String:
	if not document is Dictionary:
		return "$ must be an object"
	var value := document as Dictionary
	var error := _exact_keys_error(value, ROOT_KEYS, "$")
	if not error.is_empty():
		return error
	if not _is_int(value.schema_version) or int(value.schema_version) != SCHEMA_VERSION:
		return "$.schema_version must be integer 2"
	if not value.volumes is Dictionary:
		return "$.volumes must be an object"
	error = _exact_keys_error(value.volumes, VOLUME_KEYS, "$.volumes")
	if not error.is_empty():
		return error
	for bus: String in VOLUME_KEYS:
		if not _finite_number(value.volumes[bus]) or float(value.volumes[bus]) < 0.0 \
				or float(value.volumes[bus]) > 1.0:
			return "$.volumes.%s must be between 0 and 1" % bus
	for flag: String in ["reduced_motion", "shake", "trajectory_assist"]:
		if typeof(value[flag]) != TYPE_BOOL:
			return "$.%s must be boolean" % flag
	if not _is_int(value.ui_scale_percent) or int(value.ui_scale_percent) not in UI_SCALES:
		return "$.ui_scale_percent must be one of 100, 125, 150, 200"
	if not _finite_number(value.camera_sensitivity):
		return "$.camera_sensitivity must be finite"
	var sensitivity := float(value.camera_sensitivity)
	if sensitivity < 0.25 or sensitivity > 2.0 \
			or not is_equal_approx(sensitivity * 20.0, round(sensitivity * 20.0)):
		return "$.camera_sensitivity must be 0.25..2.00 in 0.05 steps"
	return validate_bindings(value.bindings, specs)


static func validate_bindings(bindings: Variant, specs: Array) -> String:
	if not bindings is Array:
		return "$.bindings must be an array"
	if bindings.size() != specs.size():
		return "$.bindings must contain every official action exactly once"
	var spec_by_action := {}
	for spec: Variant in specs:
		if not spec is Dictionary:
			return "binding specs must be objects"
		spec_by_action[str(spec.get("action", ""))] = spec
	var seen := {}
	var tokens_by_context := {}
	for index: int in bindings.size():
		var item: Variant = bindings[index]
		if not item is Dictionary:
			return "$.bindings[%d] must be an object" % index
		var binding := item as Dictionary
		var error := _exact_keys_error(binding, BINDING_KEYS, "$.bindings[%d]" % index)
		if not error.is_empty():
			return error
		var action := str(binding.action)
		if not spec_by_action.has(action) or seen.has(action):
			return "$.bindings[%d].action must be a unique official action" % index
		if action != str((specs[index] as Dictionary).get("action", "")):
			return "$.bindings[%d].action must preserve official order" % index
		seen[action] = true
		var spec := spec_by_action[action] as Dictionary
		for field: String in ["context", "intent", "variant"]:
			if typeof(binding[field]) != TYPE_STRING \
					or str(binding[field]) != str(spec.get(field, "")):
				return "$.bindings[%d].%s must match its official action" % [index, field]
		if not binding.tokens is Array or binding.tokens.is_empty():
			return "$.bindings[%d].tokens must be a non-empty array" % index
		var context := str(binding.context)
		if not tokens_by_context.has(context):
			tokens_by_context[context] = {}
		for token: Variant in binding.tokens:
			if typeof(token) != TYPE_STRING or not _valid_token(str(token)):
				return "$.bindings[%d] has an invalid token" % index
			if (tokens_by_context[context] as Dictionary).has(token):
				return "$.bindings has a conflict in context %s" % context
			(tokens_by_context[context] as Dictionary)[token] = action
	return ""


static func _valid_token(token: String) -> bool:
	if not token.begins_with("key:"):
		return false
	var code := token.trim_prefix("key:")
	if not code.is_valid_int() or int(code) <= 0:
		return false
	return token == "key:%d" % int(code)


static func _exact_keys_error(value: Dictionary, expected: Array, path: String) -> String:
	for key: Variant in value:
		if typeof(key) != TYPE_STRING or not expected.has(str(key)):
			return "%s has unknown key: %s" % [path, key]
	for key: Variant in expected:
		if not value.has(str(key)):
			return "%s has missing key: %s" % [path, key]
	return ""


static func _finite_number(value: Variant) -> bool:
	return typeof(value) in [TYPE_INT, TYPE_FLOAT] and is_finite(float(value))


static func _is_int(value: Variant) -> bool:
	return typeof(value) == TYPE_INT or (typeof(value) == TYPE_FLOAT \
		and is_finite(float(value)) and float(value) == floor(float(value)))
