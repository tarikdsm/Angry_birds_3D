extends SceneTree

const APP_SHELL_SCENE := "res://scenes/app_shell.tscn"
const SUCCESS_MARKER := "APP_SHELL_SMOKE_OK"


var _shell: Node
var _exit_requests := 0


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	var packed := load(APP_SHELL_SCENE) as PackedScene
	if packed == null:
		_fail("AppShell must be loadable by explicit resource path")
		return
	_shell = packed.instantiate()
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
	if input_intents.size() != 3 or screen_changes != [&"options"]:
		_fail("viewport accept must emit once and activate the route exactly once")
		return
	if _shell.get_viewport().gui_get_focus_owner() == null:
		_fail("focus must persist after a semantic route change")
		return

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


func _fail(message: String) -> void:
	push_error("app shell smoke: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	if is_instance_valid(_shell):
		root.remove_child(_shell)
		_shell.free()
	quit(exit_code)
