extends SceneTree

const FEEDBACK_DIRECTOR := preload("res://scripts/vfx/feedback_director.gd")
const HUD_TEXT_CATALOG := preload("res://scripts/data/hud_text_catalog.gd")
const HUD_SCENE := preload("res://scripts/ui/vertical_slice_hud.tscn")
const CONFIG_PATH := "res://data/feedback/vertical_slice.feedback.json"
const HUD_TEXT_PATH := "res://data/ui/vertical_slice.pt-BR.json"


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(CONFIG_PATH))
	if not parsed is Dictionary:
		_fail("canonical feedback config must parse as an object")
		return
	var validator := FEEDBACK_DIRECTOR.new()
	if not validator.has_method("validate_config_document"):
		validator.free()
		_fail("FeedbackDirector must expose strict feedback config validation")
		return
	var config := parsed as Dictionary
	if (config.get("directional_anchor", {}) as Dictionary).has("entity_id"):
		validator.free()
		_fail("directional anchor must not duplicate the objective target entity id")
		return
	var canonical_error := str(validator.validate_config_document(config))
	if not canonical_error.is_empty():
		validator.free()
		_fail("canonical feedback config must satisfy its schema: %s" % canonical_error)
		return
	var with_unknown_key := config.duplicate(true)
	with_unknown_key["unexpected"] = true
	if "unknown key" not in str(validator.validate_config_document(with_unknown_key)):
		validator.free()
		_fail("feedback config must reject unknown keys")
		return
	var with_missing_profile := config.duplicate(true)
	with_missing_profile["event_profiles"]["bird_launched"] = "missing_profile"
	if "missing_profile" not in str(validator.validate_config_document(with_missing_profile)):
		validator.free()
		_fail("feedback config must reject mappings to missing profiles")
		return
	var with_misplaced_selector := config.duplicate(true)
	with_misplaced_selector["event_profiles"]["bird_launched"] = "material"
	if "bird_launched" not in str(validator.validate_config_document(with_misplaced_selector)):
		validator.free()
		_fail("feedback config must restrict material selectors to compatible events")
		return
	if not validator.has_method("load_config_document"):
		validator.free()
		_fail("FeedbackDirector must expose diagnosed config loading")
		return
	var missing_result: Dictionary = validator.load_config_document(
		"res://data/feedback/missing.feedback.json")
	if bool(missing_result.get("ok", true)) \
			or str(missing_result.get("error_kind", "")) != "open":
		validator.free()
		_fail("missing feedback config must report an open error")
		return
	var malformed_result: Dictionary = validator.load_config_document("res://project.godot")
	if bool(malformed_result.get("ok", true)) \
			or str(malformed_result.get("error_kind", "")) != "json" \
			or malformed_result.get("message") == missing_result.get("message"):
		validator.free()
		_fail("malformed feedback config must report a distinct JSON error")
		return
	if not validator.has_method("_resolve_directional_contract"):
		validator.free()
		_fail("FeedbackDirector must validate the directional cross-reference")
		return
	var archetypes: Variant = JSON.parse_string(FileAccess.get_file_as_string(
		"res://data/archetypes/vertical_slice.archetypes.json"))
	var canonical_contract: Dictionary = validator.call(
		"_resolve_directional_contract", config, archetypes)
	if canonical_contract.is_empty():
		validator.free()
		_fail("canonical directional contract must resolve")
		return
	var with_missing_enemy := config.duplicate(true)
	with_missing_enemy["directional_anchor"]["enemy_archetype_id"] = 999
	var unresolved_contract: Dictionary = validator.call(
		"_resolve_directional_contract", with_missing_enemy, archetypes)
	if not unresolved_contract.is_empty():
		validator.free()
		_fail("missing directional enemy reference must fail before runtime")
		return
	validator.free()
	var hud_catalog_error := _validate_hud_text_catalog()
	if not hud_catalog_error.is_empty():
		_fail(hud_catalog_error)
		return
	var hud_fallback_error := await _validate_hud_fallback()
	if not hud_fallback_error.is_empty():
		_fail(hud_fallback_error)
		return
	print("feedback config validation: PASS")
	quit(0)


func _validate_hud_text_catalog() -> String:
	var canonical_result: Dictionary = HUD_TEXT_CATALOG.load_catalog(HUD_TEXT_PATH)
	if not bool(canonical_result.get("ok", false)):
		return "canonical HUD text catalog must load: %s" % canonical_result.get("message", "")
	var canonical := canonical_result.get("document", {}) as Dictionary
	var canonical_error := str(HUD_TEXT_CATALOG.validate_catalog_document(canonical))
	if not canonical_error.is_empty():
		return "canonical HUD text catalog must satisfy its schema: %s" % canonical_error

	var with_unknown_root := canonical.duplicate(true)
	with_unknown_root["unexpected"] = true
	if "unknown key" not in str(HUD_TEXT_CATALOG.validate_catalog_document(with_unknown_root)):
		return "HUD text catalog must reject unknown root keys"

	var required_ids: Array[String] = HUD_TEXT_CATALOG.required_message_ids()
	if required_ids.is_empty():
		return "HUD text catalog must declare canonical message IDs"
	var authored_messages := canonical.get("messages", {}) as Dictionary
	for runtime_path: String in [
		"res://scripts/ui/vertical_slice_hud.gd",
		"res://scripts/ui/vertical_slice_hud.tscn",
	]:
		var runtime_text := FileAccess.get_file_as_string(runtime_path)
		for message_id: String in required_ids:
			var authored_text := str(authored_messages[message_id])
			if not authored_text.is_empty() and authored_text in runtime_text:
				return "localized HUD text leaked into %s: %s" % [runtime_path, message_id]
	var with_missing_message := canonical.duplicate(true)
	(with_missing_message["messages"] as Dictionary).erase(required_ids[0])
	if "missing key" not in str(HUD_TEXT_CATALOG.validate_catalog_document(with_missing_message)):
		return "HUD text catalog must reject missing canonical messages"

	var with_unknown_message := canonical.duplicate(true)
	(with_unknown_message["messages"] as Dictionary)["hud.future"] = "future"
	if "unknown key" not in str(HUD_TEXT_CATALOG.validate_catalog_document(with_unknown_message)):
		return "HUD text catalog must reject unknown message IDs"

	var with_invalid_format := canonical.duplicate(true)
	(with_invalid_format["messages"] as Dictionary)["hud.format.birds"] = "VIRELAS"
	if "format tokens" not in str(HUD_TEXT_CATALOG.validate_catalog_document(with_invalid_format)):
		return "HUD text catalog must reject incompatible format tokens"

	var missing_result: Dictionary = HUD_TEXT_CATALOG.load_catalog(
		"res://data/ui/missing.pt-BR.json")
	if bool(missing_result.get("ok", true)) \
			or str(missing_result.get("error_kind", "")) != "open":
		return "missing HUD text catalog must report an open error"
	var malformed_result: Dictionary = HUD_TEXT_CATALOG.load_catalog("res://project.godot")
	if bool(malformed_result.get("ok", true)) \
			or str(malformed_result.get("error_kind", "")) != "json" \
			or malformed_result.get("message") == missing_result.get("message"):
		return "malformed HUD text catalog must report a distinct JSON error"

	var fallback: Dictionary = HUD_TEXT_CATALOG.safe_fallback_messages()
	if fallback.size() != required_ids.size():
		return "safe HUD fallback must cover every canonical message ID"
	for message_id: String in required_ids:
		if not fallback.has(message_id) \
				or (message_id != "hud.outcome.none" and str(fallback[message_id]).is_empty()):
			return "safe HUD fallback is missing %s" % message_id
	return ""


func _validate_hud_fallback() -> String:
	var hud := HUD_SCENE.instantiate()
	hud.text_catalog_path = "res://data/ui/missing.pt-BR.json"
	root.add_child(hud)
	await process_frame
	var error := ""
	var status: Dictionary = hud.text_catalog_status()
	if bool(status.get("ok", true)) or str(status.get("error_kind", "")) != "open" \
			or int(hud.text_catalog_load_count()) != 1:
		error = "HUD must diagnose a missing catalog after exactly one load"
	else:
		hud.apply_frame({
			"phase": "future_phase", "outcome": "future_outcome", "events": [],
		})
		var phase := hud.get_node("Root/TopBar/Margin/Readout/PhaseLabel") as Label
		var controls := hud.get_node("Root/ControlsLabel") as Label
		var result := hud.get_node("Root/ResultLabel") as Label
		if "hud.phase.unknown" not in phase.text \
				or "hud.outcome.unknown" not in result.text:
			error = "HUD fallback must expose stable unknown phase/outcome IDs"
		elif "ESC" not in controls.text or "R" not in controls.text:
			error = "HUD fallback must preserve safe pause/restart controls"
	root.remove_child(hud)
	hud.free()
	return error


func _fail(message: String) -> void:
	push_error("feedback config validation: %s" % message)
	quit(1)
