extends SceneTree

const INPUT_ROUTER := preload("res://scripts/input/input_router.gd")
const PRODUCT_TEXT_CATALOG := preload("res://scripts/data/product_text_catalog.gd")
const SETTINGS_MODEL := preload("res://scripts/save/settings_model.gd")
const PRODUCT_TEXT_PATH := "res://data/ui/product_v2.pt-BR.json"
const SUCCESS_MARKER := "SETTINGS_MODEL_SMOKE_OK"


func _initialize() -> void:
	if ProjectSettings.get_setting("application/config/name") != "Ninho Orbital" \
			or not bool(ProjectSettings.get_setting("application/config/use_custom_user_dir", false)) \
			or ProjectSettings.get_setting("application/config/custom_user_dir_name") != "NinhoOrbital":
		_fail("application identity must keep production user:// saves stable")
		return
	var catalog_result: Dictionary = PRODUCT_TEXT_CATALOG.load_catalog(PRODUCT_TEXT_PATH)
	if not bool(catalog_result.get("ok", false)):
		_fail("the extended pt-BR catalog must satisfy its closed schema")
		return
	var malformed_catalog := (catalog_result.document as Dictionary).duplicate(true)
	malformed_catalog.messages["app.options.toggle_format"] = "%s"
	if PRODUCT_TEXT_CATALOG.validate_catalog_document(malformed_catalog).is_empty():
		_fail("formatted option messages must preserve their exact placeholder contract")
		return
	var router := INPUT_ROUTER.new()
	var specs: Array = router.official_binding_specs()
	var defaults: Array = router.project_default_bindings()
	var settings := SETTINGS_MODEL.default_document(defaults)
	if not SETTINGS_MODEL.validate_document(settings, specs).is_empty():
		_fail("default settings must satisfy their own closed schema")
		return
	if not is_equal_approx(float(settings.get("camera_sensitivity", 0.0)), 1.0):
		_fail("camera sensitivity default must be 1.00")
		return
	for scale: int in [100, 125, 150, 200]:
		var candidate := settings.duplicate(true)
		candidate.ui_scale_percent = scale
		if not SETTINGS_MODEL.validate_document(candidate, specs).is_empty():
			_fail("supported UI scale was rejected: %d" % scale)
			return
	var invalid_scale := settings.duplicate(true)
	invalid_scale.ui_scale_percent = 110
	if SETTINGS_MODEL.validate_document(invalid_scale, specs).is_empty():
		_fail("unsupported UI scale must be rejected")
		return
	for sensitivity: float in [0.25, 0.30, 1.0, 1.95, 2.0]:
		var candidate := settings.duplicate(true)
		candidate.camera_sensitivity = sensitivity
		if not SETTINGS_MODEL.validate_document(candidate, specs).is_empty():
			_fail("valid camera sensitivity was rejected: %.2f" % sensitivity)
			return
	for sensitivity: float in [0.20, 0.26, 2.05, INF, NAN]:
		var candidate := settings.duplicate(true)
		candidate.camera_sensitivity = sensitivity
		if SETTINGS_MODEL.validate_document(candidate, specs).is_empty():
			_fail("invalid camera sensitivity was accepted: %s" % sensitivity)
			return
	for bus: String in ["master", "music", "ambience", "sfx", "ui"]:
		for volume: float in [0.0, 0.5, 1.0]:
			var candidate := settings.duplicate(true)
			candidate.volumes[bus] = volume
			if not SETTINGS_MODEL.validate_document(candidate, specs).is_empty():
				_fail("valid %s volume was rejected: %.2f" % [bus, volume])
				return
		for volume: float in [-0.01, 1.01, INF, NAN]:
			var candidate := settings.duplicate(true)
			candidate.volumes[bus] = volume
			if SETTINGS_MODEL.validate_document(candidate, specs).is_empty():
				_fail("invalid %s volume was accepted: %s" % [bus, volume])
				return
	var with_unknown := settings.duplicate(true)
	with_unknown.unexpected = true
	if SETTINGS_MODEL.validate_document(with_unknown, specs).is_empty():
		_fail("settings must reject unknown root keys")
		return
	var unknown_volume := settings.duplicate(true)
	unknown_volume.volumes.voice = 0.5
	if SETTINGS_MODEL.validate_document(unknown_volume, specs).is_empty():
		_fail("settings must reject unknown nested volume keys")
		return
	var unknown_binding := settings.duplicate(true)
	unknown_binding.bindings[0].device_guid = "keyboard"
	if SETTINGS_MODEL.validate_document(unknown_binding, specs).is_empty():
		_fail("settings must reject unknown nested binding keys")
		return
	var noncanonical_token := settings.duplicate(true)
	var canonical_token := str(noncanonical_token.bindings[0].tokens[0])
	noncanonical_token.bindings[0].tokens[0] = "key:0%s" % canonical_token.trim_prefix("key:")
	if SETTINGS_MODEL.validate_document(noncanonical_token, specs).is_empty():
		_fail("equivalent physical keys must have one canonical token spelling")
		return
	var reordered_bindings := settings.duplicate(true)
	var first_binding: Dictionary = reordered_bindings.bindings[0]
	reordered_bindings.bindings[0] = reordered_bindings.bindings[1]
	reordered_bindings.bindings[1] = first_binding
	if SETTINGS_MODEL.validate_document(reordered_bindings, specs).is_empty():
		_fail("validated bindings must preserve the official deterministic action order")
		return
	var missing_bindings := settings.duplicate(true)
	missing_bindings.erase("bindings")
	if SETTINGS_MODEL.validate_document(missing_bindings, specs).is_empty():
		_fail("settings must reject missing bindings")
		return
	print(SUCCESS_MARKER)
	router.free()
	quit(0)


func _fail(message: String) -> void:
	push_error("settings model smoke: %s" % message)
	quit(1)
