extends "res://scripts/app/scrollable_frontend_screen.gd"

signal world_selected(world_id: String)
signal back_requested

const RECORD_FORMAT_ID := "app.worlds.record_format"

var _catalog: Dictionary = {}
var _messages: Dictionary = {}
var _progress: Dictionary = {}
var _index := 0
var _cards: Array[Button] = []
var _rigs: Array[Node] = []


func _ready() -> void:
	super()
	(find_child("PreviousButton", true, false) as Button).pressed.connect(select_offset.bind(-1))
	(find_child("NextButton", true, false) as Button).pressed.connect(select_offset.bind(1))
	(find_child("SelectButton", true, false) as Button).pressed.connect(_select_current)
	(find_child("BackButton", true, false) as Button).pressed.connect(
		func() -> void: back_requested.emit())


func set_messages(messages: Dictionary) -> void:
	_messages = messages.duplicate(true)


func configure(catalog: Dictionary, progress: Dictionary) -> void:
	_catalog = catalog.duplicate(true)
	_progress = progress.duplicate(true)
	var requested_id := str(progress.get(
		"last_world_id", _catalog.get("default_world_id", "earth")))
	_index = (_catalog.get("world_order", []) as Array).find(requested_id)
	if _index < 0:
		_index = 0
	_rebuild()


func current_world_id() -> String:
	var order := _catalog.get("world_order", []) as Array
	return str(order[_index]) if not order.is_empty() else "earth"


func card_count() -> int:
	return _cards.size()


func rig_count() -> int:
	return _rigs.size()


func select_offset(offset: int) -> void:
	var order := _catalog.get("world_order", []) as Array
	if order.is_empty() or offset == 0:
		return
	_index = posmod(_index + offset, order.size())
	_rebuild()


func _rebuild() -> void:
	_clear_host(find_child("CardHost", true, false))
	_clear_host(find_child("RigHost", true, false))
	_cards.clear()
	_rigs.clear()
	var order := _catalog.get("world_order", []) as Array
	var centered_card: Button
	for candidate: int in order.size():
		if absi(candidate - _index) > 1:
			continue
		var world_id := str(order[candidate])
		var world := _find_world(world_id)
		var card := Button.new()
		card.name = StringName("WorldCard_%s" % world_id)
		card.set_meta("world_id", world_id)
		card.text = _card_text(world)
		card.mouse_entered.connect(card.grab_focus)
		card.pressed.connect(_on_card_pressed.bind(world_id))
		(find_child("CardHost", true, false) as Control).add_child(card)
		_cards.append(card)
		if candidate == _index:
			centered_card = card
		var rig := Node.new()
		rig.name = StringName("DioramaRig_%s" % world_id)
		rig.set_meta("world_id", world_id)
		rig.set_meta("diorama_id", str(world.get("diorama_id", "")))
		rig.set_meta("lod", "center_high" if candidate == _index else "neighbor_lod1")
		find_child("RigHost", true, false).add_child(rig)
		_rigs.append(rig)
	(find_child("CurrentWorld", true, false) as Label).text = _world_name(
		_find_world(current_world_id()))
	if centered_card != null:
		centered_card.grab_focus()


func _clear_host(host: Node) -> void:
	for child: Node in host.get_children():
		host.remove_child(child)
		child.queue_free()


func _on_card_pressed(world_id: String) -> void:
	if world_id == current_world_id():
		world_selected.emit(world_id)
		return
	var candidate := (_catalog.get("world_order", []) as Array).find(world_id)
	if candidate >= 0:
		_index = candidate
		_rebuild()


func _select_current() -> void:
	world_selected.emit(current_world_id())


func _card_text(world: Dictionary) -> String:
	var world_id := str(world.get("id", ""))
	var level_id := str(world.get("default_level_id", ""))
	var record := (_progress.get("levels", {}) as Dictionary).get(
		"%s/%s" % [world_id, level_id], {}) as Dictionary
	if not _messages.has(RECORD_FORMAT_ID):
		return RECORD_FORMAT_ID
	return str(_messages[RECORD_FORMAT_ID]) % [
		_world_name(world), int(record.get("best_score", 0)), int(record.get("best_stars", 0))]


func _world_name(world: Dictionary) -> String:
	var text_id := str(world.get("text_id", "unknown-world-text"))
	return str(_messages.get(text_id, text_id))


func _find_world(world_id: String) -> Dictionary:
	for world: Variant in _catalog.get("worlds", []):
		if world is Dictionary and str((world as Dictionary).get("id", "")) == world_id:
			return world as Dictionary
	return {}
