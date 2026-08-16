extends SceneTree

const APP_SHELL_SCENE := "res://scenes/app_shell.tscn"
const SUCCESS_MARKER := "APP_SHELL_SMOKE_OK"


var _shell: Node
var _exit_requests := 0
var _storage_root := ""


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	var packed := load(APP_SHELL_SCENE) as PackedScene
	if packed == null:
		_fail("AppShell must be loadable by explicit resource path")
		return
	_shell = packed.instantiate()
	if not _shell.has_method("set_storage_root"):
		_fail("AppShell must expose pre-tree storage injection for isolated tests")
		return
	_storage_root = "user://tests/task20-shell-%d-%d" % [
		OS.get_process_id(), Time.get_ticks_usec()]
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(_storage_root))
	_write_text(_storage_root.path_join("progress.v2.json"), "{broken progress primary")
	_write_text(_storage_root.path_join("progress.v2.json.bak"), "{broken progress backup")
	_shell.set_storage_root(_storage_root)
	root.add_child(_shell)
	await process_frame

	if _shell.get_node_or_null("InputRouter") == null \
			or _shell.get_node_or_null("ScreenRouter") == null:
		_fail("AppShell must own scene-local input and screen routers")
		return
	if _shell.find_child("VerticalSliceController", true, false) != null:
		_fail("AppShell must not load gameplay before a gameplay route is selected")
		return
	if _shell.get_viewport().gui_get_focus_owner() == null:
		_fail("main menu must expose a visible keyboard focus target")
		return
	var main_menu := _shell.get_node("ScreenRouter/ScreenHost/MainMenu")
	var recovery_notice := main_menu.find_child("RecoveryNotice", true, false) as Label
	if recovery_notice == null or not recovery_notice.visible \
			or recovery_notice.text.is_empty() or recovery_notice.text == "app.recovery.total":
		_fail("total recovery must show a non-blocking localized pt-BR notice")
		return

	var router: Node = _shell.get_node("InputRouter")
	var screen_router: Node = _shell.get_node("ScreenRouter")
	if not screen_router.has_signal("screen_changed"):
		_fail("ScreenRouter must expose route transitions to prove one viewport activation")
		return
	var input_intents: Array = []
	var screen_changes: Array = []
	router.intent_submitted.connect(input_intents.append)
	screen_router.screen_changed.connect(screen_changes.append)
	_push_key(KEY_DOWN)
	_push_key(KEY_DOWN)
	_push_key(KEY_ENTER)
	await process_frame
	if _shell.get_node_or_null("ScreenRouter/ScreenHost/OptionsMenu") == null:
		_fail("real viewport navigate and accept must reach the options screen through InputRouter")
		return
	var options := _shell.get_node("ScreenRouter/ScreenHost/OptionsMenu")
	for control_name: StringName in [
			&"ReducedMotionButton", &"ShakeButton", &"TrajectoryAssistButton",
			&"UIScaleButton", &"SensitivityDownButton", &"SensitivityUpButton",
			&"MasterVolumeButton", &"MusicVolumeButton", &"AmbienceVolumeButton",
			&"SFXVolumeButton", &"UIVolumeButton", &"BindingSemanticAcceptButton",
			&"RestoreDefaultsButton", &"BackButton"]:
		if options.find_child(str(control_name), true, false) == null:
			_fail("options must expose the closed persisted control: %s" % control_name)
			return
	(options.find_child("MasterVolumeButton", true, false) as Button).pressed.emit()
	var settings_path := _storage_root.path_join("settings.v2.json")
	var saved_options: Variant = JSON.parse_string(FileAccess.get_file_as_string(settings_path))
	if not saved_options is Dictionary \
			or not is_zero_approx(float(saved_options.volumes.master)):
		_fail("volume controls must atomically persist their closed settings field")
		return
	(options.find_child("BindingSemanticAcceptButton", true, false) as Button).pressed.emit()
	_push_key(KEY_Q)
	await process_frame
	saved_options = JSON.parse_string(FileAccess.get_file_as_string(settings_path))
	var accept_tokens: Array = []
	if saved_options is Dictionary:
		for binding: Dictionary in saved_options.get("bindings", []):
			if str(binding.get("action", "")) == "semantic_accept":
				accept_tokens = binding.get("tokens", [])
	if accept_tokens != ["key:%d" % KEY_Q]:
		_fail("binding capture UI must persist through the unique InputRouter translator")
		return
	(options.find_child("RestoreDefaultsButton", true, false) as Button).pressed.emit()
	saved_options = JSON.parse_string(FileAccess.get_file_as_string(settings_path))
	if not saved_options is Dictionary \
			or not is_equal_approx(float(saved_options.volumes.master), 1.0):
		_fail("restore defaults must atomically restore persisted options")
		return
	var sensitivity_down := options.find_child(
		"SensitivityDownButton", true, false) as Button
	if "1,00" not in sensitivity_down.text:
		_fail("camera sensitivity controls must expose the persisted current value")
		return
	if input_intents.size() != 3 or screen_changes != [&"options"]:
		_fail("viewport accept must emit once and activate the route exactly once")
		return
	if _shell.get_viewport().gui_get_focus_owner() == null:
		_fail("focus must persist after a semantic route change")
		return
	var scale_button := options.find_child("UIScaleButton", true, false) as Button
	for _step: int in range(3):
		scale_button.pressed.emit()
		await process_frame
	if not is_equal_approx(_shell.get_viewport().content_scale_factor, 2.0):
		_fail("the persisted 200 percent UI scale must reach the runtime viewport")
		return
	var effective_view := _shell.get_viewport().get_visible_rect()
	for button: Button in _visible_buttons(options):
		button.grab_focus()
		await process_frame
		if not effective_view.encloses(button.get_global_rect()):
			_fail("200 percent UI scale must keep focused option visible: %s" % button.name)
			return
	for _step: int in range(1):
		scale_button.pressed.emit()
		await process_frame

	input_intents.clear()
	(options.find_child("BindingSemanticAcceptButton", true, false) as Button).pressed.emit()
	_push_key(KEY_ESCAPE)
	await process_frame
	var conflict_confirm := options.find_child("BindingConfirmButton", true, false) as Button
	if not conflict_confirm.visible or _shell.get_viewport().gui_get_focus_owner() != conflict_confirm:
		_fail("same-context binding conflicts must require an explicit focused confirmation")
		return
	_push_key(KEY_ENTER)
	await process_frame
	var post_conflict_focus := _shell.get_viewport().gui_get_focus_owner() as Control
	if post_conflict_focus == null or not post_conflict_focus.is_visible_in_tree():
		_fail("resolving a binding conflict must restore focus to a visible option")
		return
	(options.find_child("RestoreDefaultsButton", true, false) as Button).pressed.emit()
	input_intents.clear()
	screen_changes.clear()
	_push_key(KEY_ESCAPE)
	await process_frame
	if _shell.get_node_or_null("ScreenRouter/ScreenHost/MainMenu") == null:
		_fail("real viewport back must return to the main menu through InputRouter")
		return
	if input_intents.size() != 1 or screen_changes != [&"main"]:
		_fail("viewport back must emit and activate exactly once")
		return

	_shell.set_exit_handler(_on_exit_requested)
	input_intents.clear()
	screen_changes.clear()
	_push_mouse_motion(Vector2(100.0, 360.0))
	await process_frame
	var exit_focus := _shell.get_viewport().gui_get_focus_owner()
	if exit_focus == null or exit_focus.name != &"ExitButton":
		_fail("frontend mouse hover must focus Exit before pointer validation")
		return
	_push_mouse_motion(Vector2(700.0, 600.0))
	await process_frame
	_push_left_click(Vector2(700.0, 600.0))
	await process_frame
	if _exit_requests != 0 or _shell.exit_request_count != 0 \
				or not screen_changes.is_empty():
		_fail("background click must not activate the previously focused Exit button")
		return
	if input_intents.size() != 1 or input_intents[0].kind != &"accept":
		_fail("background click must be routed as one mouse accept for target validation")
		return
	input_intents.clear()
	_push_left_click(Vector2(100.0, 360.0))
	await process_frame
	if _exit_requests != 1 or _shell.exit_request_count != 1 or input_intents.size() != 1:
		_fail("click inside Exit must request exactly one exit")
		return

	input_intents.clear()
	screen_changes.clear()
	_push_mouse_motion(Vector2(100.0, 300.0))
	await process_frame
	var hovered_focus := _shell.get_viewport().gui_get_focus_owner()
	if hovered_focus == null or hovered_focus.name != &"OptionsButton":
		_fail("frontend mouse hover must focus its button before semantic accept")
		return
	_push_left_click(Vector2(100.0, 300.0))
	await process_frame
	if _shell.get_node_or_null("ScreenRouter/ScreenHost/OptionsMenu") == null:
		_fail("real frontend mouse accept must reach its hovered button through InputRouter")
		return
	if input_intents.size() != 1 or input_intents[0].kind != &"accept" \
				or screen_changes != [&"options"]:
		_fail("frontend mouse accept must emit and activate exactly once")
		return

	_push_key(KEY_ESCAPE)
	await process_frame
	for step in range(3):
		_push_key(KEY_DOWN)
	_push_key(KEY_ENTER)
	if _exit_requests != 2 or _shell.exit_request_count != 2:
		_fail("keyboard focus exit must call the smoke handler without terminating Godot")
		return

	print(SUCCESS_MARKER)
	_finish(0)


func _on_exit_requested() -> void:
	_exit_requests += 1


func _push_key(keycode: Key) -> void:
	var event := InputEventKey.new()
	event.physical_keycode = keycode
	event.pressed = true
	root.get_viewport().push_input(event, true)


func _push_mouse_motion(position: Vector2) -> void:
	var event := InputEventMouseMotion.new()
	event.position = position
	event.global_position = position
	root.get_viewport().push_input(event, true)


func _push_left_click(position: Vector2) -> void:
	var event := InputEventMouseButton.new()
	event.button_index = MOUSE_BUTTON_LEFT
	event.pressed = true
	event.position = position
	event.global_position = position
	root.get_viewport().push_input(event, true)


func _visible_buttons(node: Node) -> Array[Button]:
	var result: Array[Button] = []
	for child: Node in node.find_children("*", "Button", true, false):
		var button := child as Button
		if button.visible:
			result.append(button)
	return result


func _fail(message: String) -> void:
	push_error("app shell smoke: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	if is_instance_valid(_shell):
		if _shell.get_parent() == root:
			root.remove_child(_shell)
		_shell.free()
	_cleanup_root()
	quit(exit_code)


func _write_text(path: String, value: String) -> void:
	var file := FileAccess.open(path, FileAccess.WRITE)
	if file != null:
		file.store_string(value)
		file.flush()


func _cleanup_root() -> void:
	if _storage_root.is_empty():
		return
	var absolute := ProjectSettings.globalize_path(_storage_root)
	var directory := DirAccess.open(absolute)
	if directory == null:
		return
	for file_name: String in directory.get_files():
		DirAccess.remove_absolute(absolute.path_join(file_name))
	DirAccess.remove_absolute(absolute)
