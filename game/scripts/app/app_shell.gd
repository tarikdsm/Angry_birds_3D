extends Node

const PRODUCT_TEXT_CATALOG := preload("res://scripts/data/product_text_catalog.gd")
const CONTENT_FILE_LOADER := preload("res://scripts/data/content_file_loader.gd")
const WORLD_CATALOG := preload("res://scripts/data/world_catalog.gd")
const SETTINGS_MODEL := preload("res://scripts/save/settings_model.gd")
const SAVE_STORE := preload("res://scripts/save/save_store.gd")
const BINDING_STORE := preload("res://scripts/input/binding_store.gd")
const INPUT_ROUTER := preload("res://scripts/input/input_router.gd")
const ACCESSIBILITY_SETTINGS := preload("res://scripts/ui/accessibility_settings.gd")
const GAMEPLAY_SESSION_CONTROLLER := preload(
	"res://scripts/game/gameplay_session_controller.gd")
const CATALOG_PATH := "res://data/ui/product_v2.pt-BR.json"
const WORLD_CATALOG_PATH := "res://data/worlds/world_catalog.v2.json"
const UI_SCALES := [100, 125, 150, 200]

@onready var _input_router: Node = $InputRouter
@onready var _screen_router: Node = $ScreenRouter
var _exit_handler := Callable()
var _storage_root := "user://"
var _save_store: RefCounted
var _messages: Dictionary = {}
var _settings: Dictionary = {}
var _progress: Dictionary = {}
var _world_catalog: Dictionary = {}
var _narrative: Dictionary = {}
var _selected_world_id := "earth"
var _selected_level_id := ""
var _default_bindings: Array = []
var _binding_specs: Array = []
var _show_recovery_notice := false
var _recovery_message_id := ""
var _pending_binding_proposal: Dictionary = {}
var exit_request_count := 0


func _ready() -> void:
	var catalog_result := PRODUCT_TEXT_CATALOG.load_catalog(CATALOG_PATH)
	if not bool(catalog_result.get("ok", false)):
		push_error("AppShell failed closed while loading product text: %s" % catalog_result.get("message", ""))
		return
	_messages = (catalog_result.get("document", {}) as Dictionary).get("messages", {}).duplicate(true)
	var world_result := WORLD_CATALOG.load_catalog()
	if not bool(world_result.get("ok", false)):
		push_error("AppShell failed closed while loading the world catalog")
		return
	_default_bindings = _input_router.project_default_bindings()
	_binding_specs = _input_router.official_binding_specs()
	_save_store = SAVE_STORE.new(_storage_root)
	var progress_result: Dictionary = _save_store.load_progress(world_result.document)
	var settings_result: Dictionary = _save_store.load_settings(_default_bindings, _binding_specs)
	if not bool(progress_result.get("ok", false)) or not bool(settings_result.get("ok", false)):
		push_error("AppShell failed closed while loading recoverable profile data")
		return
	_settings = (settings_result.get("document", {}) as Dictionary).duplicate(true)
	_progress = (progress_result.get("document", {}) as Dictionary).duplicate(true)
	_world_catalog = (world_result.get("document", {}) as Dictionary).duplicate(true)
	var narrative_result := PRODUCT_TEXT_CATALOG.load_narrative_catalog(
		"res://data/narrative/product_v2.pt-BR.json")
	if not bool(narrative_result.get("ok", false)):
		push_error("AppShell failed closed while loading product narrative")
		return
	_narrative = (narrative_result.document as Dictionary).duplicate(true)
	if not _apply_runtime(_settings):
		push_error("AppShell rejected validated runtime settings")
		return
	# The notice must say which recovery actually happened: a migrated profile
	# and a discarded one are different events for the player, and both texts
	# come from the closed pt-BR catalog.
	_recovery_message_id = str(progress_result.get("warning_message_id",
		settings_result.get("warning_message_id", "")))
	_show_recovery_notice = not _recovery_message_id.is_empty()
	_screen_router.set_messages(_messages)
	_input_router.intent_submitted.connect(_screen_router.handle_intent)
	_input_router.binding_token_captured.connect(_on_binding_token_captured)
	_screen_router.exit_requested.connect(request_exit)
	_screen_router.screen_changed.connect(_on_screen_changed)
	_screen_router.gameplay_launch_failed.connect(_on_gameplay_launch_failed)
	if (_progress.get("seen_tutorial_ids", []) as Array).has("FAN_PROJECT_NOTICE"):
		_screen_router.show_main_menu()
	else:
		_screen_router.show_fan_project_notice()


func set_storage_root(storage_root: String) -> bool:
	if is_inside_tree() or not storage_root.begins_with("user://") \
			or ".." in storage_root or "\\" in storage_root:
		return false
	_storage_root = SAVE_STORE.normalize_storage_root(storage_root)
	return true


func set_exit_handler(handler: Callable) -> void:
	_exit_handler = handler


func request_exit() -> void:
	exit_request_count += 1
	if _exit_handler.is_valid():
		_exit_handler.call()
		return
	get_tree().quit()


func _on_screen_changed(route: StringName) -> void:
	_input_router.set_context(
		INPUT_ROUTER.CONTEXT_GAMEPLAY if route == &"gameplay"
		else INPUT_ROUTER.CONTEXT_FRONTEND)
	if route != &"options":
		# The rebind capture and its pending proposal belong to the options
		# screen alone. Every other route — the pause menu and gameplay
		# included — disarms them, so a key pressed elsewhere is never consumed
		# and never rewrites a persisted binding.
		_input_router.cancel_binding_capture()
		_pending_binding_proposal.clear()
	if route == &"gameplay":
		_bind_gameplay()
		return
	var screen := _screen_router.current_screen() as Control
	if screen == null:
		return
	if route == &"main":
		var recovery_notice := screen.find_child("RecoveryNotice", true, false) as Label
		if recovery_notice != null:
			recovery_notice.visible = _show_recovery_notice
			if _show_recovery_notice:
				recovery_notice.text = _message(_recovery_message_id)
		var continue_button := screen.find_child("ContinueButton", true, false) as Button
		if continue_button != null:
			continue_button.pressed.connect(_continue_progress)
		return
	if route == &"fan_notice":
		screen.accepted.connect(_accept_fan_notice)
		return
	if route == &"about":
		screen.back_requested.connect(_screen_router.show_main_menu)
		return
	if route == &"worlds":
		screen.configure(_world_catalog, _progress)
		screen.world_selected.connect(_open_levels)
		screen.back_requested.connect(_screen_router.show_main_menu)
		return
	if route == &"levels":
		screen.configure(_find_world(_selected_world_id), _progress)
		screen.level_selected.connect(_open_briefing)
		screen.back_requested.connect(_screen_router.show_world_carousel)
		return
	if route == &"briefing":
		var briefing_id := "BRF_Farm" if _selected_world_id == "earth" else "BRF_Orbital"
		var lines: Array[String] = []
		for line: Variant in ((_narrative.briefings as Dictionary)[briefing_id] as Dictionary).lines:
			lines.append(str(line))
		screen.configure(lines)
		screen.accepted.connect(_accept_briefing)
		screen.back_requested.connect(_screen_router.show_level_select)
		return
	if route in [&"pause", &"result"]:
		return
	if route != &"options":
		return
	(_screen_control(screen, "ReducedMotionButton") as Button).pressed.connect(
		_toggle_bool.bind("reduced_motion"))
	(_screen_control(screen, "ShakeButton") as Button).pressed.connect(_toggle_bool.bind("shake"))
	(_screen_control(screen, "TrajectoryAssistButton") as Button).pressed.connect(
		_toggle_bool.bind("trajectory_assist"))
	(_screen_control(screen, "UIScaleButton") as Button).pressed.connect(_cycle_ui_scale)
	(_screen_control(screen, "SensitivityDownButton") as Button).pressed.connect(
		_adjust_sensitivity.bind(-0.05))
	(_screen_control(screen, "SensitivityUpButton") as Button).pressed.connect(
		_adjust_sensitivity.bind(0.05))
	(_screen_control(screen, "RestoreDefaultsButton") as Button).pressed.connect(_restore_defaults)
	for control: Node in screen.find_children("*", "Button", true, false):
		if not control is Button:
			continue
		var button := control as Button
		var volume_bus := str(button.get_meta("volume_bus", ""))
		if not volume_bus.is_empty():
			button.pressed.connect(_cycle_volume.bind(volume_bus))
		var binding_action := StringName(str(button.get_meta("binding_action", "")))
		if not binding_action.is_empty():
			button.pressed.connect(_begin_binding_capture.bind(binding_action, button))
	(_screen_control(screen, "BindingConfirmButton") as Button).pressed.connect(
		_confirm_binding_conflict)
	(_screen_control(screen, "BindingCancelButton") as Button).pressed.connect(
		_cancel_binding_conflict)
	_refresh_options(screen)


func _toggle_bool(field: String) -> void:
	var candidate := _settings.duplicate(true)
	candidate[field] = not bool(candidate.get(field, false))
	_persist_settings(candidate)


func _cycle_ui_scale() -> void:
	var candidate := _settings.duplicate(true)
	var index := UI_SCALES.find(int(candidate.ui_scale_percent))
	candidate.ui_scale_percent = UI_SCALES[(index + 1) % UI_SCALES.size()]
	_persist_settings(candidate)


func _adjust_sensitivity(delta: float) -> void:
	var candidate := _settings.duplicate(true)
	var value := clampf(float(candidate.camera_sensitivity) + delta, 0.25, 2.0)
	candidate.camera_sensitivity = snappedf(value, 0.05)
	_persist_settings(candidate)


func _cycle_volume(bus: String) -> void:
	var candidate := _settings.duplicate(true)
	var value := float(candidate.volumes.get(bus, 1.0)) + 0.25
	candidate.volumes[bus] = 0.0 if value > 1.0 else value
	_persist_settings(candidate)


func _begin_binding_capture(action: StringName, button: Button) -> void:
	_pending_binding_proposal.clear()
	_hide_binding_conflict()
	if _input_router.begin_binding_capture(action):
		button.text = _message("app.options.binding_waiting") % [
			_message("app.options.binding.%s" % action)]


func _on_binding_token_captured(action: StringName, token: String) -> void:
	var proposal: Dictionary = BINDING_STORE.propose_rebind(_settings.bindings, action, token)
	if not bool(proposal.get("ok", false)):
		_refresh_current_options()
		return
	if bool(proposal.get("conflict", false)):
		_pending_binding_proposal = proposal
		_show_binding_conflict()
		return
	_apply_binding_proposal(proposal)


func _confirm_binding_conflict() -> void:
	if not _pending_binding_proposal.is_empty():
		_apply_binding_proposal(_pending_binding_proposal)


func _cancel_binding_conflict() -> void:
	var action := StringName(str(_pending_binding_proposal.get("action", "")))
	_pending_binding_proposal.clear()
	_hide_binding_conflict()
	_refresh_current_options()
	_focus_binding_action(action)


func _apply_binding_proposal(proposal: Dictionary) -> void:
	var action := StringName(str(proposal.get("action", "")))
	var resolution: Dictionary = BINDING_STORE.resolve_rebind(_settings.bindings, proposal, true)
	if bool(resolution.get("ok", false)):
		var candidate := _settings.duplicate(true)
		candidate.bindings = (resolution.get("bindings", []) as Array).duplicate(true)
		_persist_settings(candidate)
	_pending_binding_proposal.clear()
	_hide_binding_conflict()
	_refresh_current_options()
	_focus_binding_action(action)


func _restore_defaults() -> void:
	_persist_settings(SETTINGS_MODEL.default_document(_default_bindings))


func _persist_settings(candidate: Dictionary) -> bool:
	var validation_error := SETTINGS_MODEL.validate_document(candidate, _binding_specs)
	if not validation_error.is_empty():
		return false
	var previous := _settings.duplicate(true)
	if not _apply_runtime(candidate):
		_apply_runtime(previous)
		return false
	var save_result: Dictionary = _save_store.save_settings(candidate, _binding_specs)
	if not bool(save_result.get("ok", false)):
		_apply_runtime(previous)
		return false
	_settings = candidate.duplicate(true)
	_publish_runtime_settings()
	var screen := _screen_router.current_screen() as Control
	if screen != null and screen.name == &"OptionsMenu":
		_refresh_options(screen)
	return true


## Pushes the persisted accessibility options to a mounted level. They only
## reach the presentation, so they can never change score or trajectory.
func _publish_runtime_settings() -> void:
	var session: Node3D = _screen_router.gameplay()
	if session == null:
		return
	session.set_bindings(_settings.get("bindings", []) as Array)
	session.apply_accessibility(ACCESSIBILITY_SETTINGS.from_document(_settings))


func _apply_runtime(value: Dictionary) -> bool:
	var bindings_result: Dictionary = _input_router.apply_bindings(value.get("bindings", []))
	if not bool(bindings_result.get("ok", false)):
		return false
	if not _input_router.set_camera_sensitivity(float(value.get("camera_sensitivity", 1.0))):
		return false
	get_viewport().content_scale_factor = float(value.get("ui_scale_percent", 100)) / 100.0
	return true


func _refresh_options(screen: Control) -> void:
	_set_toggle_text(screen, "ReducedMotionButton", "app.options.reduced_motion", "reduced_motion")
	_set_toggle_text(screen, "ShakeButton", "app.options.shake", "shake")
	_set_toggle_text(
		screen, "TrajectoryAssistButton", "app.options.trajectory_assist", "trajectory_assist")
	var scale_value := "%d%%" % int(_settings.ui_scale_percent)
	(_screen_control(screen, "UIScaleButton") as Button).text = _format_value(
		"app.options.ui_scale", scale_value)
	var sensitivity_value := ("%.2f" % float(_settings.camera_sensitivity)).replace(".", ",")
	(_screen_control(screen, "SensitivityDownButton") as Button).text = _message(
		"app.options.camera_sensitivity_down") % [sensitivity_value]
	(_screen_control(screen, "SensitivityUpButton") as Button).text = _message(
		"app.options.camera_sensitivity_up") % [sensitivity_value]
	for control: Node in screen.find_children("*", "Button", true, false):
		if not control is Button:
			continue
		var button := control as Button
		var volume_bus := str(button.get_meta("volume_bus", ""))
		if not volume_bus.is_empty():
			button.text = _format_value(
				"app.options.volume.%s" % volume_bus,
				"%d%%" % roundi(float(_settings.volumes[volume_bus]) * 100.0))
		var binding_action := str(button.get_meta("binding_action", ""))
		if not binding_action.is_empty():
			button.text = _format_value(
				"app.options.binding.%s" % binding_action,
				_binding_token_text(binding_action))


func _set_toggle_text(screen: Control, node_name: String, label_id: String, field: String) -> void:
	var state_id := "app.options.enabled" if bool(_settings[field]) else "app.options.disabled"
	(_screen_control(screen, node_name) as Button).text = _message("app.options.toggle_format") % [
		_message(label_id), _message(state_id)]


func _format_value(label_id: String, value: String) -> String:
	return _message("app.options.value_format") % [_message(label_id), value]


func _message(message_id: String) -> String:
	return str(_messages.get(message_id, message_id))


func _binding_token_text(action: String) -> String:
	for binding: Dictionary in _settings.get("bindings", []):
		if str(binding.action) == action:
			var tokens := binding.tokens as Array
			if not tokens.is_empty():
				return OS.get_keycode_string(int(str(tokens.front()).trim_prefix("key:")))
	return ""


func _show_binding_conflict() -> void:
	var screen := _screen_router.current_screen() as Control
	if screen == null or screen.name != &"OptionsMenu":
		return
	(_screen_control(screen, "BindingConflictLabel") as Label).visible = true
	(_screen_control(screen, "BindingConfirmButton") as Button).visible = true
	(_screen_control(screen, "BindingCancelButton") as Button).visible = true
	(_screen_control(screen, "BindingConfirmButton") as Button).grab_focus()


func _hide_binding_conflict() -> void:
	var screen := _screen_router.current_screen() as Control
	if screen == null or screen.name != &"OptionsMenu":
		return
	(_screen_control(screen, "BindingConflictLabel") as Label).visible = false
	(_screen_control(screen, "BindingConfirmButton") as Button).visible = false
	(_screen_control(screen, "BindingCancelButton") as Button).visible = false


func _refresh_current_options() -> void:
	var screen := _screen_router.current_screen() as Control
	if screen != null and screen.name == &"OptionsMenu":
		_refresh_options(screen)


func _focus_binding_action(action: StringName) -> void:
	var screen := _screen_router.current_screen() as Control
	if screen == null or screen.name != &"OptionsMenu":
		return
	for control: Node in screen.find_children("*", "Button", true, false):
		if control is Button \
				and StringName(str(control.get_meta("binding_action", ""))) == action:
			(control as Button).grab_focus()
			return
	(_screen_control(screen, "RestoreDefaultsButton") as Button).grab_focus()


func _screen_control(screen: Control, node_name: String) -> Control:
	return screen.find_child(node_name, true, false) as Control


func _accept_fan_notice() -> void:
	var screen := _screen_router.current_screen() as Control
	var persisted := _mark_seen("seen_tutorial_ids", "FAN_PROJECT_NOTICE")
	if screen != null and screen.has_method("show_save_retry"):
		screen.show_save_retry(not persisted)
	if not persisted:
		return
	_screen_router.show_main_menu()


func _open_levels(world_id: String) -> void:
	var world := _find_world(world_id)
	var default_level_id := str(world.get("default_level_id", ""))
	if world.is_empty() or not _persist_position(world_id, default_level_id):
		return
	_selected_world_id = world_id
	_selected_level_id = default_level_id
	_screen_router.show_level_select()


func _open_briefing(world_id: String, level_id: String) -> void:
	if not _persist_position(world_id, level_id):
		return
	_selected_world_id = world_id
	_selected_level_id = level_id
	_screen_router.show_narrative_brief()


func _accept_briefing() -> void:
	var screen := _screen_router.current_screen() as Control
	var persisted := _mark_seen(
		"seen_briefing_ids", "%s/%s" % [_selected_world_id, _selected_level_id])
	if screen != null and screen.has_method("show_save_retry"):
		screen.show_save_retry(not persisted)
	if not persisted:
		return
	open_gameplay()


## Opens the selected level. The launch request is built by the gameplay
## controller from its closed campaign registry, so an unregistered pair can
## never reach the kernel.
func open_gameplay() -> bool:
	var request: Dictionary = GAMEPLAY_SESSION_CONTROLLER.make_launch_request(
		_selected_world_id, _selected_level_id)
	if not bool(request.get("ok", false)):
		_on_gameplay_launch_failed(str(request.get("message", "")))
		return false
	return _screen_router.show_gameplay(request.request as Dictionary)


func _continue_progress() -> void:
	_selected_world_id = str(_progress.get("last_world_id", ""))
	_selected_level_id = str(_progress.get("last_level_id", ""))
	if (_progress.get("seen_briefing_ids", []) as Array).has(
			"%s/%s" % [_selected_world_id, _selected_level_id]):
		open_gameplay()
		return
	_screen_router.show_narrative_brief()


func _bind_gameplay() -> void:
	var session: Node3D = _screen_router.gameplay()
	if session == null:
		return
	if not session.level_resolved.is_connected(_on_level_resolved):
		session.level_resolved.connect(_on_level_resolved)
	if not session.session_faulted.is_connected(_on_session_faulted):
		session.session_faulted.connect(_on_session_faulted)
	_publish_runtime_settings()


## Runs on the single confirmed terminal frame published by the controller.
## Only a victory reaches the save, and it reaches it exactly once.
func _on_level_resolved(summary: Dictionary) -> void:
	var payload := summary.duplicate(true)
	payload.new_record = false
	payload.save_failed = false
	if str(summary.get("outcome", "")) == "victory":
		var saved: Dictionary = _save_store.save_level_result(
			_progress, summary, _world_catalog)
		if bool(saved.get("ok", false)):
			_progress = (saved.get("document", _progress) as Dictionary).duplicate(true)
			payload.new_record = bool(saved.get("new_record", false))
		else:
			payload.save_failed = true
	_screen_router.show_result_screen(payload)


## A content or physics fault is never a defeat: nothing is recorded and the
## player returns to the level list.
func _on_session_faulted(_code: String, _fault_message: String) -> void:
	_screen_router.show_level_select()


func _on_gameplay_launch_failed(_reason: String) -> void:
	var screen := _screen_router.current_screen() as Control
	if screen == null:
		return
	var notice := screen.find_child("RetryNotice", true, false) as Label
	if notice == null:
		return
	notice.text = _message("app.gameplay.launch_failed")
	notice.visible = true


func _mark_seen(field: String, identifier: String) -> bool:
	var candidate := _progress.duplicate(true)
	var seen := candidate.get(field, []) as Array
	if seen.has(identifier):
		return true
	seen.append(identifier)
	candidate[field] = seen
	var saved: Dictionary = _save_store.save_progress(candidate, _world_catalog)
	if not bool(saved.get("ok", false)):
		return false
	_progress = candidate
	return true


func _persist_position(world_id: String, level_id: String) -> bool:
	var candidate := _progress.duplicate(true)
	candidate.last_world_id = world_id
	candidate.last_level_id = level_id
	var saved: Dictionary = _save_store.save_progress(candidate, _world_catalog)
	if not bool(saved.get("ok", false)):
		return false
	_progress = candidate
	return true


func _find_world(world_id: String) -> Dictionary:
	for world: Variant in _world_catalog.get("worlds", []):
		if world is Dictionary and str((world as Dictionary).get("id", "")) == world_id:
			return world as Dictionary
	return {}
