extends "res://scripts/app/scrollable_frontend_screen.gd"

const WORLD_CATALOG := preload("res://scripts/data/world_catalog.gd")

signal level_selected(world_id: String, level_id: String)
signal back_requested

var _messages: Dictionary = {}
var _world: Dictionary = {}


func _ready() -> void:
	super()
	(find_child("BackButton", true, false) as Button).pressed.connect(
		func() -> void: back_requested.emit())


func set_messages(messages: Dictionary) -> void:
	_messages = messages.duplicate(true)


func configure(world: Dictionary, _progress: Dictionary) -> void:
	_world = world.duplicate(true)
	var host := find_child("LevelHost", true, false)
	for child: Node in host.get_children():
		host.remove_child(child)
		child.free()
	var first_button: Button
	for level: Dictionary in _world.get("levels", []):
		var level_id := str(level.get("id", "unknown-level"))
		var text_id := WORLD_CATALOG.level_text_id(str(_world.get("id", "")), level_id)
		var button := Button.new()
		button.name = StringName("LevelButton_%s" % level_id)
		button.set_meta("world_id", str(_world.get("id", "")))
		button.set_meta("level_id", level_id)
		button.text = str(_messages.get(text_id, text_id))
		button.mouse_entered.connect(button.grab_focus)
		button.pressed.connect(_select_level.bind(level_id))
		host.add_child(button)
		if first_button == null:
			first_button = button
	if first_button != null:
		first_button.grab_focus()


func _select_level(level_id: String) -> void:
	level_selected.emit(str(_world.get("id", "")), level_id)
