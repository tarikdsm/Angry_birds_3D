extends Node

const INPUT_INTENT := preload("res://scripts/input/input_intent.gd")
const MAIN_MENU := preload("res://scenes/frontend/main_menu.tscn")
const OPTIONS_MENU := preload("res://scenes/frontend/options_menu.tscn")
const FAN_PROJECT_NOTICE := preload("res://scenes/frontend/fan_project_notice.tscn")
const ABOUT_SCREEN := preload("res://scenes/frontend/about_screen.tscn")
const WORLD_CAROUSEL := preload("res://scenes/frontend/world_carousel.tscn")
const LEVEL_SELECT := preload("res://scenes/frontend/level_select.tscn")
const NARRATIVE_BRIEF := preload("res://scenes/frontend/narrative_brief.tscn")

signal exit_requested
signal screen_changed(route: StringName)

@onready var _screen_host: Node = $ScreenHost
var _messages: Dictionary = {}
var _current_screen: Control


func set_messages(messages: Dictionary) -> void:
	_messages = messages.duplicate(true)


func show_main_menu() -> void:
	_show_screen(MAIN_MENU, &"main")


func show_options_menu() -> void:
	_show_screen(OPTIONS_MENU, &"options")

func show_fan_project_notice() -> void:
	_show_screen(FAN_PROJECT_NOTICE, &"fan_notice")

func show_about_screen() -> void:
	_show_screen(ABOUT_SCREEN, &"about")

func show_world_carousel() -> void:
	_show_screen(WORLD_CAROUSEL, &"worlds")

func show_level_select() -> void:
	_show_screen(LEVEL_SELECT, &"levels")

func show_narrative_brief() -> void:
	_show_screen(NARRATIVE_BRIEF, &"briefing")


func current_screen() -> Control:
	return _current_screen


func handle_intent(intent: RefCounted) -> void:
	if intent == null or not intent is INPUT_INTENT:
		return
	if intent.kind == INPUT_INTENT.KIND_NAVIGATE:
		_navigate(intent.payload.get("direction", Vector2.ZERO) as Vector2)
	elif intent.kind == INPUT_INTENT.KIND_ACCEPT:
		_activate_accept(intent)
	elif intent.kind == INPUT_INTENT.KIND_BACK:
		_match_back()


func _show_screen(scene: PackedScene, route: StringName) -> void:
	if _current_screen != null:
		_screen_host.remove_child(_current_screen)
		_current_screen.queue_free()
	_current_screen = scene.instantiate() as Control
	_screen_host.add_child(_current_screen)
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
			show_main_menu)

func _match_back() -> void:
	if _current_screen == null:
		return
	match _current_screen.name:
		&"OptionsMenu", &"AboutScreen": show_main_menu()
		&"FanProjectNotice": return
		&"WorldCarousel": show_main_menu()
		&"LevelSelect": show_world_carousel()
		&"NarrativeBrief": show_level_select()


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
