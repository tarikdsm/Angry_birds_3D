extends Node

const INPUT_INTENT := preload("res://scripts/input/input_intent.gd")
const MAIN_MENU := preload("res://scenes/frontend/main_menu.tscn")
const OPTIONS_MENU := preload("res://scenes/frontend/options_menu.tscn")

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


func handle_intent(intent: RefCounted) -> void:
	if intent == null or not intent is INPUT_INTENT:
		return
	if intent.kind == INPUT_INTENT.KIND_NAVIGATE:
		_move_focus((intent.payload.get("direction", Vector2.ZERO) as Vector2).y)
	elif intent.kind == INPUT_INTENT.KIND_ACCEPT:
		_activate_accept(intent)
	elif intent.kind == INPUT_INTENT.KIND_BACK and _current_screen != null \
			and _current_screen.name == &"OptionsMenu":
		show_main_menu()


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
	for control: Node in _current_screen.get_children():
		if control is Label or control is Button:
			var message_id := str(control.get_meta("message_id", ""))
			if not message_id.is_empty() and _messages.has(message_id):
				(control as Control).text = str(_messages[message_id])


func _connect_actions(route: StringName) -> void:
	for button: Button in _action_buttons():
		button.mouse_entered.connect(button.grab_focus)
	if route == &"main":
		(_current_screen.get_node("OptionsButton") as Button).pressed.connect(show_options_menu)
		(_current_screen.get_node("ExitButton") as Button).pressed.connect(exit_requested.emit)
	elif route == &"options":
		(_current_screen.get_node("BackButton") as Button).pressed.connect(show_main_menu)


func _action_buttons() -> Array[Button]:
	var buttons: Array[Button] = []
	if _current_screen == null:
		return buttons
	for control: Node in _current_screen.get_children():
		if control is Button:
			buttons.append(control as Button)
	return buttons


func _focus_first_action() -> void:
	var buttons := _action_buttons()
	if not buttons.is_empty():
		buttons.front().grab_focus()


func _move_focus(vertical_direction: float) -> void:
	if is_zero_approx(vertical_direction):
		return
	var buttons := _action_buttons()
	if buttons.is_empty():
		return
	var focused := get_viewport().gui_get_focus_owner() as Button
	var index := buttons.find(focused)
	if index < 0:
		index = 0
	var step := 1 if vertical_direction > 0.0 else -1
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
