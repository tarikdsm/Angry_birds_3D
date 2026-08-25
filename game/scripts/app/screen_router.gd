extends Node

const INPUT_INTENT := preload("res://scripts/input/input_intent.gd")
const FOCUS_NAVIGATION := preload("res://scripts/ui/focus_navigation.gd")
const MAIN_MENU := preload("res://scenes/frontend/main_menu.tscn")
const OPTIONS_MENU := preload("res://scenes/frontend/options_menu.tscn")
const FAN_PROJECT_NOTICE := preload("res://scenes/frontend/fan_project_notice.tscn")
const ABOUT_SCREEN := preload("res://scenes/frontend/about_screen.tscn")
const WORLD_CAROUSEL := preload("res://scenes/frontend/world_carousel.tscn")
const LEVEL_SELECT := preload("res://scenes/frontend/level_select.tscn")
const NARRATIVE_BRIEF := preload("res://scenes/frontend/narrative_brief.tscn")
const PAUSE_MENU := preload("res://scenes/frontend/pause_menu.tscn")
const RESULT_SCREEN := preload("res://scenes/frontend/result_screen.tscn")
const GAMEPLAY_SESSION := preload("res://scenes/gameplay/gameplay_session.tscn")

signal exit_requested
signal screen_changed(route: StringName)
## Emitted when a launch request is refused. The route never changes, so the
## player stays on the briefing with a localized diagnostic.
signal gameplay_launch_failed(message: String)

@onready var _screen_host: Node = $ScreenHost
@onready var _gameplay_host: Node = $GameplayHost
var _messages: Dictionary = {}
var _current_screen: Control
var _gameplay: Node3D
var _route: StringName = &""


func set_messages(messages: Dictionary) -> void:
	_messages = messages.duplicate(true)


func show_main_menu() -> void:
	release_gameplay()
	_show_screen(MAIN_MENU, &"main")


func show_options_menu() -> void:
	_show_screen(OPTIONS_MENU, &"options")

func show_fan_project_notice() -> void:
	release_gameplay()
	_show_screen(FAN_PROJECT_NOTICE, &"fan_notice")

func show_about_screen() -> void:
	_show_screen(ABOUT_SCREEN, &"about")

func show_world_carousel() -> void:
	release_gameplay()
	_show_screen(WORLD_CAROUSEL, &"worlds")

func show_level_select() -> void:
	release_gameplay()
	_show_screen(LEVEL_SELECT, &"levels")

func show_narrative_brief() -> void:
	release_gameplay()
	_show_screen(NARRATIVE_BRIEF, &"briefing")


## Mounts the product gameplay scene and hands it the validated launch request.
## The overlay host stays empty so the world is visible, and every failure is
## fail-closed: the scene is released and the caller keeps its route.
func show_gameplay(request: Dictionary) -> bool:
	release_gameplay()
	_gameplay = GAMEPLAY_SESSION.instantiate() as Node3D
	if _gameplay == null:
		gameplay_launch_failed.emit("the product gameplay scene root is not a Node3D")
		return false
	_gameplay_host.add_child(_gameplay)
	if not _gameplay.configure_launch(request):
		var message := str(_gameplay.last_error)
		release_gameplay()
		gameplay_launch_failed.emit(message)
		return false
	_clear_overlay()
	_route = &"gameplay"
	screen_changed.emit(_route)
	return true


func show_pause_menu() -> void:
	if _gameplay == null:
		return
	_gameplay.set_paused(true)
	_show_screen(PAUSE_MENU, &"pause")


func show_result_screen(summary: Dictionary) -> void:
	_show_screen(RESULT_SCREEN, &"result")
	if _current_screen != null:
		_current_screen.configure(summary)
		FOCUS_NAVIGATION.focus_first(_current_screen)


func resume_gameplay() -> void:
	if _gameplay == null:
		show_main_menu()
		return
	_clear_overlay()
	_gameplay.set_paused(false)
	_route = &"gameplay"
	screen_changed.emit(_route)


func restart_gameplay() -> void:
	if _gameplay == null or not _gameplay.restart_level():
		return
	resume_gameplay()


func release_gameplay() -> void:
	if _gameplay == null:
		return
	var released := _gameplay
	_gameplay = null
	released.release_level()
	_gameplay_host.remove_child(released)
	released.queue_free()


func gameplay() -> Node3D:
	return _gameplay


func current_route() -> StringName:
	return _route


func current_screen() -> Control:
	return _current_screen


func handle_intent(intent: RefCounted) -> void:
	if intent == null or not intent is INPUT_INTENT:
		return
	if _route == &"gameplay" and _gameplay != null:
		_handle_gameplay_intent(intent)
		return
	if _route == &"pause" and intent.kind == INPUT_INTENT.KIND_PAUSE:
		resume_gameplay()
		return
	if intent.kind == INPUT_INTENT.KIND_NAVIGATE:
		_navigate(intent.payload.get("direction", Vector2.ZERO) as Vector2)
	elif intent.kind == INPUT_INTENT.KIND_ACCEPT:
		_activate_accept(intent)
	elif intent.kind == INPUT_INTENT.KIND_BACK:
		_match_back()


## In gameplay the semantic Back first belongs to the gesture: it cancels an
## open grab. Only when no gesture consumes it does it open the pause overlay,
## which is why the 2.0 never needed a thirteenth intent kind.
func _handle_gameplay_intent(intent: RefCounted) -> void:
	if intent.kind == INPUT_INTENT.KIND_PAUSE:
		show_pause_menu()
		return
	if intent.kind == INPUT_INTENT.KIND_BACK:
		if not _gameplay.handle_intent(intent):
			show_pause_menu()
		return
	_gameplay.handle_intent(intent)


func _clear_overlay() -> void:
	if _current_screen != null:
		_screen_host.remove_child(_current_screen)
		_current_screen.queue_free()
		_current_screen = null


func _show_screen(scene: PackedScene, route: StringName) -> void:
	_clear_overlay()
	_current_screen = scene.instantiate() as Control
	_screen_host.add_child(_current_screen)
	_route = route
	_apply_messages()
	_connect_actions(route)
	_focus_first_action()
	screen_changed.emit(route)


func _apply_messages() -> void:
	if _current_screen.has_method("set_messages"):
		_current_screen.set_messages(_messages)
	for control: Node in _current_screen.find_children("*", "Control", true, false):
		if control is Label or control is Button:
			var message_id := str(control.get_meta("message_id", ""))
			if not message_id.is_empty() and _messages.has(message_id):
				(control as Control).text = str(_messages[message_id])


func _connect_actions(route: StringName) -> void:
	for button: Button in _action_buttons():
		button.mouse_entered.connect(button.grab_focus)
	if route == &"main":
		(_current_screen.find_child("OptionsButton", true, false) as Button).pressed.connect(
			show_options_menu)
		(_current_screen.find_child("WorldsButton", true, false) as Button).pressed.connect(
			show_world_carousel)
		(_current_screen.find_child("ExitButton", true, false) as Button).pressed.connect(
			exit_requested.emit)
		var about := _current_screen.find_child("AboutButton", true, false) as Button
		if about != null:
			about.pressed.connect(show_about_screen)
	elif route == &"options":
		(_current_screen.find_child("BackButton", true, false) as Button).pressed.connect(
			_leave_options)
	elif route == &"pause":
		_connect_button("ResumeButton", resume_gameplay)
		_connect_button("RestartButton", restart_gameplay)
		_connect_button("OptionsButton", show_options_menu)
		_connect_button("LevelSelectButton", show_level_select)
		_connect_button("MainMenuButton", show_main_menu)
	elif route == &"result":
		_connect_button("RestartButton", restart_gameplay)
		_connect_button("LevelSelectButton", show_level_select)
		_connect_button("ContinueButton", show_main_menu)


func _connect_button(node_name: String, handler: Callable) -> void:
	var button := _current_screen.find_child(node_name, true, false) as Button
	if button != null:
		button.pressed.connect(handler)


## Leaving the options screen returns to whatever opened it: the pause overlay
## when a level is mounted, the main menu otherwise.
func _leave_options() -> void:
	if _gameplay != null:
		show_pause_menu()
		return
	show_main_menu()


func _match_back() -> void:
	if _current_screen == null:
		return
	match _current_screen.name:
		&"OptionsMenu": _leave_options()
		&"AboutScreen": show_main_menu()
		&"FanProjectNotice": return
		&"WorldCarousel": show_main_menu()
		&"LevelSelect": show_world_carousel()
		&"NarrativeBrief": show_level_select()
		&"PauseMenu": resume_gameplay()
		&"ResultScreen": return


func _action_buttons() -> Array[Button]:
	var buttons: Array[Button] = []
	if _current_screen == null:
		return buttons
	for control: Node in _current_screen.find_children("*", "Button", true, false):
		if control is Button and (control as Button).visible:
			buttons.append(control as Button)
	return buttons


func _focus_first_action() -> void:
	var buttons := _action_buttons()
	if not buttons.is_empty():
		buttons.front().grab_focus()


func _navigate(direction: Vector2) -> void:
	if _current_screen != null and _current_screen.name == &"WorldCarousel" \
			and not is_zero_approx(direction.x) and _current_screen.has_method("select_offset"):
		_current_screen.select_offset(1 if direction.x > 0.0 else -1)
		return
	_move_focus(direction.y)


func _move_focus(direction: float) -> void:
	if is_zero_approx(direction):
		return
	var buttons := _action_buttons()
	if buttons.is_empty():
		return
	var focused := get_viewport().gui_get_focus_owner() as Button
	var index := buttons.find(focused)
	if index < 0:
		index = 0
	var step := 1 if direction > 0.0 else -1
	buttons[posmod(index + step, buttons.size())].grab_focus()


func _activate_focus() -> void:
	var focused := get_viewport().gui_get_focus_owner()
	if focused is Button and _current_screen != null and _current_screen.is_ancestor_of(focused):
		(focused as Button).pressed.emit()


func _activate_accept(intent: RefCounted) -> void:
	if intent.source != &"mouse":
		_activate_focus()
		return
	var position: Variant = intent.payload.get("position")
	if not position is Vector2:
		return
	for button: Button in _action_buttons():
		if button.get_global_rect().has_point(position as Vector2):
			button.grab_focus()
			button.pressed.emit()
			return
