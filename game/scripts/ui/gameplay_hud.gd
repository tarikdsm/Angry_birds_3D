extends CanvasLayer

## Product HUD.
##
## The HUD is a pure reader of the published frame: objectives, score, chain
## multiplier, stars, bird queue and locked plane all come from the dictionary
## the kernel publishes. It never queries a solver, never advances the session
## and never derives a gameplay decision. The legacy vertical_slice_hud.gd stays
## untouched and keeps serving the v1 fixture.

const ACCESSIBILITY_SETTINGS := preload("res://scripts/ui/accessibility_settings.gd")
const FOCUS_NAVIGATION := preload("res://scripts/ui/focus_navigation.gd")
const THEME_FACTORY := preload("res://scripts/ui/theme_factory.gd")
const TRAJECTORY_RENDERER := preload("res://scripts/ui/trajectory_renderer.gd")

const QUEUE_VISIBLE_ENTRIES := 4
const HINT_ACTIONS := [&"semantic_pause", &"semantic_restart", &"semantic_recenter"]
## Below this logical viewport the HUD folds its secondary blocks so the
## objective, the score, the queue and the material legend keep fitting at the
## 150% and 200% UI scales the specification supports.
const COMPACT_VIEWPORT := Vector2(1000.0, 560.0)
## Below this one the material legend folds into a single wrapped line. It never
## disappears: the glyph and the pt-BR name are the only non-colour carrier of
## material identity, and the accessibility requirement has no exception for a
## scale the product supports.
const MINIMAL_VIEWPORT := Vector2(760.0, 420.0)
const QUEUE_SEPARATION := 8
const MINIMAL_QUEUE_SEPARATION := 2

var _messages: Dictionary = {}
var _birds: Dictionary = {}
var _abilities: Dictionary = {}
var _materials: Dictionary = {}
var _level_material_keys: Array[String] = []
var _configured := false
var _reduced_motion := false
var _root: Control
var _trajectory: Node
var _objective_label: Label
var _plane_label: Label
var _materials_host: VBoxContainer
var _materials_panel: PanelContainer
var _materials_box: VBoxContainer
var _objective_box: VBoxContainer
var _score_label: Label
var _multiplier_label: Label
var _stars_label: Label
var _ability_label: Label
var _assist_label: Label
var _queue_host: VBoxContainer
var _materials_title: Label
var _queue_title: Label
var _controls_label: Label
var _controls_panel: PanelContainer
var _materials_block: VBoxContainer
var _bindings: Array = []
var _compact := false
var _minimal := false
var _last_frame: Dictionary = {}


func _ready() -> void:
	layer = 1
	_build()


func configure(
		messages: Dictionary,
		level_document: Dictionary,
		archetypes_document: Dictionary,
		materials_document: Dictionary) -> bool:
	if _root == null:
		_build()
	_messages = messages.duplicate(true)
	_birds = {}
	_abilities = {}
	_materials = {}
	for ability: Variant in archetypes_document.get("abilities", []):
		if ability is Dictionary:
			_abilities[int((ability as Dictionary).get("id", 0))] = (ability as Dictionary).duplicate(true)
	for bird: Variant in archetypes_document.get("birds", []):
		if bird is Dictionary:
			_birds[int((bird as Dictionary).get("id", 0))] = (bird as Dictionary).duplicate(true)
	for material: Variant in materials_document.get("materials", []):
		if material is Dictionary:
			_materials[int((material as Dictionary).get("id", 0))] = (material as Dictionary).duplicate(true)
	if _birds.is_empty() or _materials.is_empty():
		return false
	_level_material_keys = _collect_material_keys(level_document)
	_render_static_text()
	_rebuild_material_legend()
	_render_controls_hint()
	_configured = true
	return true


func release() -> void:
	_configured = false
	_messages = {}
	_birds = {}
	_abilities = {}
	_materials = {}
	_level_material_keys = []
	_last_frame = {}
	if _queue_host != null:
		_clear(_queue_host)
	if _materials_host != null:
		_clear(_materials_host)
	if _trajectory != null:
		_trajectory.clear()


func configured() -> bool:
	return _configured


func set_camera(camera: Camera3D) -> void:
	if _trajectory != null:
		_trajectory.set_camera(camera)


func set_trajectory_assist(enabled: bool) -> void:
	if _trajectory != null:
		_trajectory.set_assist(enabled)
	if _assist_label != null:
		_assist_label.text = _message(
			"hud.assist.enabled" if enabled else "hud.assist.disabled")


func set_reduced_motion(enabled: bool) -> void:
	_reduced_motion = enabled


func reduced_motion() -> bool:
	return _reduced_motion


func set_bindings(bindings: Array) -> void:
	_bindings = bindings.duplicate(true)
	_render_controls_hint()


func trajectory_renderer() -> Node:
	return _trajectory


func root_control() -> Control:
	return _root


func is_compact() -> bool:
	return _compact


func is_minimal() -> bool:
	return _minimal


## The queue is never shortened. The specification asks for the current bird and
## the next three, without an exception for a supported UI scale, so compaction
## folds other blocks instead of dropping shots the player has to plan around.
func visible_queue_entries() -> int:
	return QUEUE_VISIBLE_ENTRIES


## Progressive disclosure: the objective, the score, the bird queue and the
## material legend are the blocks a player needs to act and to read state
## without relying on colour, so none of them is ever hidden. What steps aside
## is the control hint panel and the assist label, and the legend moves into the
## space they vacate.
func _apply_responsive_layout() -> void:
	if _root == null:
		return
	var view := _root.size
	var compact := view.x < COMPACT_VIEWPORT.x or view.y < COMPACT_VIEWPORT.y
	var minimal := view.x < MINIMAL_VIEWPORT.x or view.y < MINIMAL_VIEWPORT.y
	var changed := compact != _compact or minimal != _minimal
	_compact = compact
	_minimal = minimal
	if _controls_panel != null:
		_controls_panel.visible = not _compact
	if _assist_label != null:
		_assist_label.visible = not _compact
	# At the smallest supported logical viewport the queue gives up its header
	# and its row spacing, never an entry: the header is decoration, the four
	# entries are the shot plan the player plays around.
	if _queue_title != null:
		_queue_title.visible = not _minimal
	if _queue_host != null:
		_queue_host.add_theme_constant_override(
			"separation", MINIMAL_QUEUE_SEPARATION if _minimal else QUEUE_SEPARATION)
	# The legend gives up the same decoration for the same reason: each row still
	# names its material by glyph and by pt-BR copy, which is the guarantee.
	if _materials_title != null:
		_materials_title.visible = not _minimal
	if _materials_host != null:
		_materials_host.add_theme_constant_override(
			"separation", MINIMAL_QUEUE_SEPARATION if _minimal else QUEUE_SEPARATION)
	if not changed:
		return
	_relocate_material_legend()
	if _configured and not _last_frame.is_empty():
		_render_queue(_last_frame)


## The legend is never dropped: it moves into the bottom row, taking exactly the
## space the control hints vacate. The glyph and the pt-BR name of every
## material therefore stay on screen at 150% and 200% as well, and the material
## state never has to be read from colour alone.
func _relocate_material_legend() -> void:
	if _materials_block == null or _materials_panel == null or _objective_box == null:
		return
	var host: Node = _materials_box if _compact else _objective_box
	if _materials_block.get_parent() != host:
		_materials_block.reparent(host)
	_materials_panel.visible = _compact
	_materials_block.visible = true


func material_legend_keys() -> Array[String]:
	return _level_material_keys.duplicate()


func queue_entry_texts() -> Array[String]:
	var texts: Array[String] = []
	if _queue_host == null:
		return texts
	for child: Node in _queue_host.get_children():
		if child is Label:
			texts.append((child as Label).text)
	return texts


## Consumes exactly one published frame.
func apply_frame(frame: Dictionary) -> void:
	if not _configured:
		return
	var objectives := frame.get("objectives", {}) as Dictionary
	var targets := objectives.get("targets", []) as Array
	var neutralized := 0
	for target: Variant in targets:
		if target is Dictionary and bool((target as Dictionary).get("neutralized", false)):
			neutralized += 1
	_objective_label.text = _message("hud.objectives") % [neutralized, targets.size()]
	_score_label.text = _message("hud.score") % int(frame.get("score", 0))
	_multiplier_label.text = _message("hud.multiplier") % _multiplier_text(
		int(frame.get("multiplier_percent", 100)))
	_stars_label.text = _message("hud.stars") % int(frame.get("stars", 0))
	_plane_label.text = _message(
		"hud.plane.locked" if frame.get("locked_plane") is Dictionary else "hud.plane.free")
	_last_frame = frame
	_render_queue(frame)
	if _trajectory != null:
		_trajectory.apply_frame(frame)


## The chain multiplier is published as an integer percentage; the HUD only
## formats it in pt-BR.
func _multiplier_text(percent: int) -> String:
	return ("%.2f" % (float(percent) / 100.0)).replace(".", ",") + "×"


func _render_queue(frame: Dictionary) -> void:
	_clear(_queue_host)
	var queue := frame.get("bird_queue", []) as Array
	if queue.is_empty():
		_queue_host.add_child(_label(_message("hud.queue.empty"), "Muted"))
		_ability_label.text = ""
		return
	var readiness := str(frame.get("ability_readiness", "unavailable"))
	for slot: int in mini(visible_queue_entries(), queue.size()):
		var bird := _birds.get(int(queue[slot]), {}) as Dictionary
		var badge := ACCESSIBILITY_SETTINGS.bird_badge(str(bird.get("key", "")))
		var ability := _abilities.get(int(bird.get("ability_id", 0)), {}) as Dictionary
		var ability_badge := ACCESSIBILITY_SETTINGS.ability_badge(str(ability.get("kind", "")))
		var state_id := ACCESSIBILITY_SETTINGS.readiness_message_id(readiness) if slot == 0 \
			else "hud.state.waiting"
		var entry := _message("hud.queue.entry") % [
			str(badge.get("glyph", "[--]")),
			_message(str(badge.get("message_id", "hud.state.waiting"))),
			_message(state_id)]
		_queue_host.add_child(_label(entry, "Value" if slot == 0 else "Muted"))
		if slot == 0:
			_ability_label.text = _message("hud.ability.entry") % [
				str(ability_badge.get("glyph", "[-]")),
				_message(str(ability_badge.get("message_id", "hud.state.waiting")))]


func _rebuild_material_legend() -> void:
	_clear(_materials_host)
	for key: String in _level_material_keys:
		var badge := ACCESSIBILITY_SETTINGS.material_badge(key)
		if badge.is_empty():
			continue
		var row := HBoxContainer.new()
		row.name = StringName("MaterialRow_%s" % key)
		row.mouse_filter = Control.MOUSE_FILTER_IGNORE
		var swatch := ColorRect.new()
		swatch.name = &"Swatch"
		swatch.color = Color(str(badge.color))
		swatch.custom_minimum_size = Vector2(14.0, 14.0)
		swatch.mouse_filter = Control.MOUSE_FILTER_IGNORE
		swatch.size_flags_vertical = Control.SIZE_SHRINK_CENTER
		row.add_child(swatch)
		row.add_child(_label(_message("hud.material.entry") % [
			str(badge.glyph), _message(str(badge.message_id))], "Muted"))
		_materials_host.add_child(row)


func _collect_material_keys(level_document: Dictionary) -> Array[String]:
	var ordered: Array[String] = []
	for body: Variant in level_document.get("bodies", []):
		if not body is Dictionary:
			continue
		var material_id: Variant = (body as Dictionary).get("material_id")
		if material_id == null:
			continue
		var material := _materials.get(int(material_id), {}) as Dictionary
		var key := str(material.get("key", ""))
		if key.is_empty() or ordered.has(key):
			continue
		ordered.append(key)
	ordered.sort()
	return ordered


func _render_static_text() -> void:
	if _materials_title != null:
		_materials_title.text = _message("hud.material.title")
	if _queue_title != null:
		_queue_title.text = _message("hud.queue.title")
	if _assist_label != null and _trajectory != null:
		_assist_label.text = _message(
			"hud.assist.enabled" if _trajectory.assist_enabled() else "hud.assist.disabled")


func _render_controls_hint() -> void:
	if _controls_label == null:
		return
	var tokens: Array = []
	for action: StringName in HINT_ACTIONS:
		tokens.append(_binding_text(action))
	_controls_label.text = _message("hud.controls.hint") % tokens


func _binding_text(action: StringName) -> String:
	for binding: Variant in _bindings:
		if not binding is Dictionary or StringName(str((binding as Dictionary).get(
				"action", ""))) != action:
			continue
		var keys := (binding as Dictionary).get("tokens", []) as Array
		if not keys.is_empty():
			return OS.get_keycode_string(int(str(keys.front()).trim_prefix("key:")))
	for event: InputEvent in InputMap.action_get_events(action):
		if event is InputEventKey:
			var key_event := event as InputEventKey
			var keycode := key_event.physical_keycode
			if keycode == 0:
				keycode = key_event.keycode
			if keycode > 0:
				return OS.get_keycode_string(keycode)
	return "?"


func _message(message_id: String) -> String:
	return str(_messages.get(message_id, message_id))


func _clear(host: Node) -> void:
	if host == null:
		return
	for child: Node in host.get_children():
		host.remove_child(child)
		child.queue_free()


func _label(text: String, variation: String) -> Label:
	var label := Label.new()
	label.text = text
	label.theme_type_variation = StringName(variation)
	label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	label.autowrap_mode = TextServer.AUTOWRAP_OFF
	return label


func _build() -> void:
	if _root != null:
		return
	_root = Control.new()
	_root.name = &"HudRoot"
	_root.theme = THEME_FACTORY.build()
	_root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(_root)

	_trajectory = TRAJECTORY_RENDERER.new()
	_trajectory.name = &"TrajectoryRenderer"
	_root.add_child(_trajectory)

	var margins := MarginContainer.new()
	margins.name = &"HudMargins"
	margins.mouse_filter = Control.MOUSE_FILTER_IGNORE
	margins.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	for margin: String in ["margin_left", "margin_right", "margin_top", "margin_bottom"]:
		margins.add_theme_constant_override(margin, 24)
	_root.add_child(margins)

	var rows := VBoxContainer.new()
	rows.name = &"HudRows"
	rows.mouse_filter = Control.MOUSE_FILTER_IGNORE
	margins.add_child(rows)

	var top := HBoxContainer.new()
	top.name = &"TopRow"
	top.mouse_filter = Control.MOUSE_FILTER_IGNORE
	rows.add_child(top)

	var objective_panel := _panel(&"ObjectivePanel")
	_objective_box = _column(&"ObjectiveBox")
	objective_panel.add_child(_objective_box)
	_objective_label = _label("", "Headline")
	_objective_label.name = &"ObjectiveLabel"
	_objective_box.add_child(_objective_label)
	_plane_label = _label("", "Value")
	_plane_label.name = &"PlaneLabel"
	_objective_box.add_child(_plane_label)
	_materials_block = _column(&"MaterialsBlock")
	_objective_box.add_child(_materials_block)
	_materials_title = _label(_message("hud.material.title"), "Muted")
	_materials_title.name = &"MaterialsTitle"
	_materials_block.add_child(_materials_title)
	_materials_host = _column(&"MaterialsHost")
	_materials_block.add_child(_materials_host)
	top.add_child(objective_panel)

	top.add_child(_spacer(&"TopSpacer", true))

	var score_panel := _panel(&"ScorePanel")
	var score_box := _column(&"ScoreBox")
	score_panel.add_child(score_box)
	_score_label = _label("", "Headline")
	_score_label.name = &"ScoreLabel"
	score_box.add_child(_score_label)
	_multiplier_label = _label("", "Value")
	_multiplier_label.name = &"MultiplierLabel"
	score_box.add_child(_multiplier_label)
	_stars_label = _label("", "Value")
	_stars_label.name = &"StarsLabel"
	score_box.add_child(_stars_label)
	_ability_label = _label("", "Muted")
	_ability_label.name = &"AbilityLabel"
	score_box.add_child(_ability_label)
	_assist_label = _label(_message("hud.assist.disabled"), "Muted")
	_assist_label.name = &"AssistLabel"
	score_box.add_child(_assist_label)
	top.add_child(score_panel)

	rows.add_child(_spacer(&"MiddleSpacer", false))

	var bottom := HBoxContainer.new()
	bottom.name = &"BottomRow"
	bottom.mouse_filter = Control.MOUSE_FILTER_IGNORE
	rows.add_child(bottom)

	var queue_panel := _panel(&"QueuePanel")
	var queue_box := _column(&"QueueBox")
	queue_panel.add_child(queue_box)
	_queue_title = _label(_message("hud.queue.title"), "Value")
	_queue_title.name = &"QueueTitle"
	queue_box.add_child(_queue_title)
	_queue_host = _column(&"QueueHost")
	queue_box.add_child(_queue_host)
	bottom.add_child(queue_panel)

	bottom.add_child(_spacer(&"BottomSpacer", true))

	# Second home of the material legend. It only becomes visible when the
	# compact layout pulls the legend out of the objective panel, and it occupies
	# exactly the room the control hints give up.
	_materials_panel = _panel(&"MaterialsPanel")
	_materials_panel.visible = false
	_materials_box = _column(&"MaterialsBox")
	_materials_panel.add_child(_materials_box)
	bottom.add_child(_materials_panel)

	_controls_panel = _panel(&"ControlsPanel")
	var controls_box := _column(&"ControlsBox")
	_controls_panel.add_child(controls_box)
	_controls_label = _label("", "Muted")
	_controls_label.name = &"ControlsLabel"
	controls_box.add_child(_controls_label)
	bottom.add_child(_controls_panel)
	_root.resized.connect(_apply_responsive_layout)
	_apply_responsive_layout()


func _panel(panel_name: StringName) -> PanelContainer:
	var panel := PanelContainer.new()
	panel.name = panel_name
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	panel.size_flags_vertical = Control.SIZE_SHRINK_BEGIN
	return panel


func _column(column_name: StringName) -> VBoxContainer:
	var column := VBoxContainer.new()
	column.name = column_name
	column.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return column


func _spacer(spacer_name: StringName, horizontal: bool) -> Control:
	var spacer := Control.new()
	spacer.name = spacer_name
	spacer.mouse_filter = Control.MOUSE_FILTER_IGNORE
	if horizontal:
		spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	else:
		spacer.size_flags_vertical = Control.SIZE_EXPAND_FILL
	return spacer
