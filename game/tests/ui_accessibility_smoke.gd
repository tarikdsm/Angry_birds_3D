extends SceneTree

## Accessibility contract of the 2.0 product UI: contrast, non-colour-only
## state, keyboard reachability, responsive layout and persisted options.

const ACCESSIBILITY_SETTINGS := preload("res://scripts/ui/accessibility_settings.gd")
const CONTROLLER := preload("res://scripts/game/gameplay_session_controller.gd")
const PRODUCT_TEXT_CATALOG := preload("res://scripts/data/product_text_catalog.gd")
const FOCUS_NAVIGATION := preload("res://scripts/ui/focus_navigation.gd")
const INPUT_ROUTER := preload("res://scripts/input/input_router.gd")
const SAVE_STORE := preload("res://scripts/save/save_store.gd")
const SETTINGS_MODEL := preload("res://scripts/save/settings_model.gd")
const THEME_FACTORY := preload("res://scripts/ui/theme_factory.gd")
const WORLD_CATALOG := preload("res://scripts/data/world_catalog.gd")
const GAMEPLAY_SCENE := "res://scenes/gameplay/gameplay_session.tscn"
const PAUSE_MENU := "res://scenes/frontend/pause_menu.tscn"
const RESULT_SCREEN := "res://scenes/frontend/result_screen.tscn"
const VIEWPORT_SIZES := [
	{"id": "16:9", "size": Vector2i(1280, 720)},
	{"id": "16:10", "size": Vector2i(1280, 800)},
	{"id": "4:3", "size": Vector2i(1024, 768)},
]
const UI_SCALES := [1.0, 1.5, 2.0]
const FRONTEND_SCREENS := [
	"res://scenes/frontend/main_menu.tscn",
	"res://scenes/frontend/options_menu.tscn",
	"res://scenes/frontend/world_carousel.tscn",
	"res://scenes/frontend/level_select.tscn",
	"res://scenes/frontend/narrative_brief.tscn",
	"res://scenes/frontend/about_screen.tscn",
	"res://scenes/frontend/fan_project_notice.tscn",
]
const MARKER := "UI_ACCESSIBILITY_SMOKE_OK"

var _errors: Array[String] = []
var _gameplay: Node3D
var _overlay: Control
var _router: Node
var _storage_root := ""


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	_check_contrast()
	_check_badges()
	await _check_frontend_screens()
	await _check_hud_layout()
	if _errors.is_empty():
		await _check_overlay_focus(PAUSE_MENU, "PauseMenu")
	if _errors.is_empty():
		await _check_overlay_focus(RESULT_SCREEN, "ResultScreen")
	if _errors.is_empty():
		_check_persisted_options()
	if not _errors.is_empty():
		_fail(" | ".join(_errors))
		return
	print(MARKER)
	_finish(0)


func _check_contrast() -> void:
	var pairs: Array = THEME_FACTORY.text_pairs()
	_check(pairs.size() >= 8, "the theme must declare every shipped text pair")
	for entry: Dictionary in pairs + (THEME_FACTORY.icon_pairs() as Array):
		var ratio: float = THEME_FACTORY.contrast_ratio(
			entry.foreground as Color, entry.background as Color)
		_check(ratio >= float(entry.minimum),
			"contrast below the requirement for %s: %.2f < %.2f" % [
				entry.id, ratio, float(entry.minimum)])
	_check(is_equal_approx(
			THEME_FACTORY.contrast_ratio(Color.WHITE, Color.BLACK), 21.0),
		"the contrast formula must reproduce the WCAG extreme")
	_check(THEME_FACTORY.NORMAL_TEXT_MINIMUM >= 4.5 \
			and THEME_FACTORY.LARGE_TEXT_MINIMUM >= 3.0,
		"the shipped thresholds must never be relaxed")
	var theme := THEME_FACTORY.build()
	_check(theme.get_color("font_color", "Label") == THEME_FACTORY.PRIMARY_TEXT \
			and theme.has_stylebox("panel", "PanelContainer"),
		"the theme must ship the label and panel entries the screens consume")


func _check_badges() -> void:
	var failures: Array[String] = ACCESSIBILITY_SETTINGS.badge_contrast_failures()
	_check(failures.is_empty(), "colour-only or low-contrast badge: %s" % ", ".join(failures))
	for readiness: String in ["unavailable", "arming", "armed", "active", "spent"]:
		_check(not ACCESSIBILITY_SETTINGS.readiness_message_id(readiness).is_empty(),
			"every ability state must carry localized text: %s" % readiness)


## The contrast proof measures the ThemeFactory palette, so it says something
## about a screen only if that screen actually renders with the palette. Task 24
## proved three surfaces; the seven screens the player meets first were left on
## the engine default theme, therefore unmeasured. This probe pins that every
## registered frontend screen resolves the audited colours, paints the audited
## surface behind them, and keeps every action reachable without clipping at
## every supported aspect ratio and UI scale.
func _check_frontend_screens() -> void:
	var messages := _product_messages()
	var audited: Array[Color] = []
	for entry: Dictionary in THEME_FACTORY.text_pairs():
		var foreground := entry.foreground as Color
		if not audited.has(foreground):
			audited.append(foreground)
	var original_size := root.size
	for scene_path: String in FRONTEND_SCREENS:
		var packed := load(scene_path) as PackedScene
		if packed == null:
			_check(false, "the frontend screen must be loadable: %s" % scene_path)
			continue
		_overlay = packed.instantiate() as Control
		root.add_child(_overlay)
		await process_frame
		if _overlay.has_method("set_messages"):
			_overlay.set_messages(messages)
		var screen_name := str(_overlay.name)
		var screen_theme: Theme = _overlay.theme
		_check(screen_theme != null \
				and screen_theme.get_color("font_color", "Label") == THEME_FACTORY.PRIMARY_TEXT \
				and screen_theme.has_stylebox("normal", "Button"),
			"%s must render with the audited product theme" % screen_name)
		var backdrop := _overlay.find_child("Backdrop", true, false) as ColorRect
		_check(backdrop != null and backdrop.color == THEME_FACTORY.SURFACE,
			"%s must paint the audited surface behind its copy" % screen_name)
		for control: Node in _overlay.find_children("*", "Control", true, false):
			if control is Button:
				_check(audited.has((control as Button).get_theme_color("font_color")) \
						and audited.has(
							(control as Button).get_theme_color("font_focus_color")),
					"%s renders %s in an unaudited colour" % [screen_name, control.name])
			elif control is Label:
				_check(audited.has((control as Label).get_theme_color("font_color")),
					"%s renders %s in an unaudited colour" % [screen_name, control.name])
		for viewport: Dictionary in VIEWPORT_SIZES:
			for scale: float in UI_SCALES:
				root.size = viewport.size as Vector2i
				root.content_scale_factor = scale
				await process_frame
				await process_frame
				var view := root.get_visible_rect()
				for button: Button in _visible_buttons(_overlay):
					button.grab_focus()
					await process_frame
					_check(button.has_focus() and button.is_visible_in_tree() \
							and view.encloses(button.get_global_rect()),
						"%s must keep %s reachable without clipping at %s %d%%: view=%s rect=%s" % [
							screen_name, button.name, viewport.id,
							roundi(scale * 100.0), view, button.get_global_rect()])
		root.remove_child(_overlay)
		_overlay.free()
		_overlay = null
	root.content_scale_factor = 1.0
	root.size = original_size
	await process_frame


func _visible_buttons(screen: Control) -> Array[Button]:
	var buttons: Array[Button] = []
	for control: Node in screen.find_children("*", "Button", true, false):
		if control is Button and (control as Button).is_visible_in_tree():
			buttons.append(control as Button)
	return buttons


func _check_hud_layout() -> void:
	if not ClassDB.class_exists("GameplaySessionNode"):
		_check(false, "GameplaySessionNode must be registered before the layout probe")
		return
	var request_result: Dictionary = CONTROLLER.make_launch_request("earth", "farm_reaction")
	var packed := load(GAMEPLAY_SCENE) as PackedScene
	_gameplay = packed.instantiate() as Node3D
	root.add_child(_gameplay)
	await process_frame
	if not _gameplay.configure_launch(request_result.request as Dictionary):
		_check(false, "the Earth launch must configure: %s" % _gameplay.last_error)
		return
	await physics_frame
	await physics_frame
	var hud: CanvasLayer = _gameplay.hud()
	var legend: Array[String] = hud.material_legend_keys()
	_check(legend.size() >= 3,
		"the Fazenda HUD must publish its material legend: %d" % legend.size())
	for key: String in legend:
		var row: Node = hud.root_control().find_child("MaterialRow_%s" % key, true, false)
		_check(row != null and row.get_child_count() == 2,
			"each material row must carry a swatch and a localized glyph: %s" % key)
		if row == null:
			continue
		var label := row.get_child(1) as Label
		_check(label != null and label.text.contains("[") \
				and label.text.length() > 5 and not label.text.begins_with("hud."),
			"the material row must resolve glyph and pt-BR copy: %s" % key)

	# The queue is fed only by the published bird_queue; projectiles, and thus
	# the clones a split creates, can never reach it.
	hud.apply_frame({
		"objectives": {"complete": false, "targets": []},
		"score": 1234, "stars": 2, "multiplier_percent": 130,
		"bird_queue": [2, 3, 4, 5, 2, 2],
		"current_bird": 2,
		"ability_readiness": "armed",
		"projectiles": [
			{"entity_id": 10}, {"entity_id": 11}, {"entity_id": 12}, {"entity_id": 13},
		],
		"trajectory_preview": null,
	})
	var entries: Array[String] = hud.queue_entry_texts()
	_check(entries.size() == 4,
		"the queue must show the current bird and the next three: %d" % entries.size())
	_check(entries[0].contains("PRONTA") and entries[0].contains("[VM]"),
		"the current bird must carry state text and a glyph: %s" % entries[0])
	_check(entries[1].contains("AGUARDANDO"),
		"queued birds must declare that they are waiting: %s" % entries[1])
	var ability_label: Label = hud.root_control().find_child("AbilityLabel", true, false) as Label
	_check(ability_label != null and ability_label.text.contains("[M]") \
			and ability_label.text.contains("GANHO"),
		"the current power must be readable by glyph and text")
	var score_label: Label = hud.root_control().find_child("ScoreLabel", true, false) as Label
	var multiplier_label: Label = hud.root_control().find_child(
		"MultiplierLabel", true, false) as Label
	var stars_label: Label = hud.root_control().find_child("StarsLabel", true, false) as Label
	_check(score_label.text.contains("1234") and multiplier_label.text.contains("1,30") \
			and stars_label.text.contains("2"),
		"score, multiplier and stars must reflect the published frame")

	var original_size := root.size
	for viewport: Dictionary in VIEWPORT_SIZES:
		for scale: float in UI_SCALES:
			root.size = viewport.size as Vector2i
			root.content_scale_factor = scale
			await process_frame
			await process_frame
			_check_hud_fits(hud, "%s at %d%%" % [viewport.id, roundi(scale * 100.0)])
	root.content_scale_factor = 1.0
	root.size = original_size
	await process_frame


## The objective, the score and the queue are the blocks a player needs to act:
## they must stay visible, inside the viewport and apart at every supported
## aspect ratio and UI scale. The secondary blocks may step aside, but never
## clip or overlap while they are shown.
func _check_hud_fits(hud: CanvasLayer, label: String) -> void:
	var view := root.get_visible_rect()
	var rects: Dictionary = {}
	for panel_name: String in [
			"ObjectivePanel", "ScorePanel", "QueuePanel", "ControlsPanel", "MaterialsPanel"]:
		var panel: Control = hud.root_control().find_child(panel_name, true, false) as Control
		if panel == null:
			_check(false, "the HUD must keep %s mounted" % panel_name)
			return
		if panel_name not in ["ControlsPanel", "MaterialsPanel"]:
			_check(panel.is_visible_in_tree(),
				"%s must never hide %s" % [label, panel_name])
		if not panel.is_visible_in_tree():
			continue
		var rect := panel.get_global_rect()
		rects[panel_name] = rect
		_check(view.encloses(rect),
			"%s must keep %s inside the viewport: view=%s rect=%s" % [
				label, panel_name, view, rect])
	for pair: Array in [["ObjectivePanel", "ScorePanel"], ["QueuePanel", "ControlsPanel"],
			["ObjectivePanel", "QueuePanel"], ["ScorePanel", "ControlsPanel"],
			["QueuePanel", "MaterialsPanel"], ["ObjectivePanel", "MaterialsPanel"],
			["ScorePanel", "MaterialsPanel"]]:
		if not rects.has(pair[0]) or not rects.has(pair[1]):
			continue
		var first := rects[pair[0]] as Rect2
		var second := rects[pair[1]] as Rect2
		_check(not first.intersects(second),
			"%s must never overlap %s and %s" % [label, pair[0], pair[1]])
	# The queue and the material legend are specification guarantees, not
	# progressive-disclosure candidates: the current bird plus the next three, and
	# a non-colour carrier for every material of the level, at every supported
	# aspect ratio and UI scale. Compaction may fold the header, the row spacing
	# and the control hints; it may never fold these two.
	var queue_host: Control = hud.root_control().find_child(
		"QueueHost", true, false) as Control
	_check(queue_host != null and queue_host.get_child_count() == 4,
		"%s must always show the current bird and the next three: %d" % [
			label, -1 if queue_host == null else queue_host.get_child_count()])
	if queue_host != null:
		_check(view.encloses(queue_host.get_global_rect()),
			"%s must keep the whole queue inside the viewport: view=%s rect=%s" % [
				label, view, queue_host.get_global_rect()])
	var legend: Control = hud.root_control().find_child(
		"MaterialsBlock", true, false) as Control
	_check(legend != null and legend.is_visible_in_tree(),
		"%s must never hide the material legend" % label)
	for key: String in hud.material_legend_keys():
		var row: Control = hud.root_control().find_child(
			"MaterialRow_%s" % key, true, false) as Control
		_check(row != null and row.is_visible_in_tree() \
				and view.encloses(row.get_global_rect()),
			"%s must keep the %s legend row readable inside the viewport" % [label, key])


func _check_overlay_focus(scene_path: String, expected_name: String) -> void:
	var packed := load(scene_path) as PackedScene
	_overlay = packed.instantiate() as Control
	root.add_child(_overlay)
	await process_frame
	_check(str(_overlay.name) == expected_name,
		"the overlay scene must keep its route name: %s" % _overlay.name)
	if _overlay.has_method("set_messages"):
		_overlay.set_messages(_product_messages())
	if _overlay.has_method("configure"):
		_overlay.configure({
			"outcome": "victory", "score": 50054, "stars": 3, "birds_used": 2,
			"new_record": true, "save_failed": false,
		})
		await process_frame
	var controls: Array[Control] = FOCUS_NAVIGATION.focusable_controls(_overlay)
	_check(controls.size() >= 3,
		"%s must expose its full action set: %d" % [expected_name, controls.size()])
	_check(FOCUS_NAVIGATION.is_ring_closed(_overlay),
		"%s must close the keyboard focus ring" % expected_name)
	var original_size := root.size
	for viewport: Dictionary in VIEWPORT_SIZES:
		for scale: float in UI_SCALES:
			root.size = viewport.size as Vector2i
			root.content_scale_factor = scale
			await process_frame
			await process_frame
			var view := root.get_visible_rect()
			var visited: Array[String] = []
			var focused: Control = FOCUS_NAVIGATION.focus_first(_overlay)
			for step: int in controls.size():
				await process_frame
				_check(focused != null and focused.is_visible_in_tree(),
					"%s must keep a visible focus at %s %d%%" % [
						expected_name, viewport.id, roundi(scale * 100.0)])
				if focused == null:
					break
				visited.append(str(focused.name))
				_check(view.encloses(focused.get_global_rect()),
					"%s must keep %s reachable at %s %d%%: view=%s rect=%s" % [
						expected_name, focused.name, viewport.id,
						roundi(scale * 100.0), view, focused.get_global_rect()])
				focused = FOCUS_NAVIGATION.move(_overlay, focused, 1)
			_check(visited.size() == controls.size(),
				"%s must reach every action by keyboard at %s %d%%" % [
					expected_name, viewport.id, roundi(scale * 100.0)])
	root.content_scale_factor = 1.0
	root.size = original_size
	await process_frame
	root.remove_child(_overlay)
	_overlay.free()
	_overlay = null


func _check_persisted_options() -> void:
	_router = INPUT_ROUTER.new()
	root.add_child(_router)
	var specs: Array = _router.official_binding_specs()
	var defaults: Array = _router.project_default_bindings()
	_storage_root = "user://tests/task24-accessibility-%d-%d" % [
		OS.get_process_id(), Time.get_ticks_usec()]
	var store: RefCounted = SAVE_STORE.new(_storage_root)
	var loaded: Dictionary = store.load_settings(defaults, specs)
	if not bool(loaded.get("ok", false)):
		_check(false, "the isolated profile must load its default settings")
		return
	var document := (loaded.document as Dictionary).duplicate(true)
	var initial: Dictionary = ACCESSIBILITY_SETTINGS.from_document(document)
	_check(not bool(initial.reduced_motion) and bool(initial.shake) \
			and not bool(initial.trajectory_assist) and int(initial.ui_scale_percent) == 100,
		"a new profile must start with the documented accessibility defaults")
	document.reduced_motion = true
	document.shake = false
	document.trajectory_assist = true
	document.ui_scale_percent = 150
	_check(SETTINGS_MODEL.validate_document(document, specs).is_empty(),
		"the accessibility options must stay inside the persisted schema")
	var saved: Dictionary = store.save_settings(document, specs)
	if not bool(saved.get("ok", false)):
		_check(false, "the accessibility options must persist atomically")
		return
	var reopened: RefCounted = SAVE_STORE.new(_storage_root)
	var reloaded: Dictionary = reopened.load_settings(defaults, specs)
	var restored: Dictionary = ACCESSIBILITY_SETTINGS.from_document(
		reloaded.get("document", {}))
	_check(bool(restored.reduced_motion) and not bool(restored.shake) \
			and bool(restored.trajectory_assist) and int(restored.ui_scale_percent) == 150,
		"reduced motion, shake and assist must survive a relaunch")

	if _gameplay == null or not _gameplay.session_configured:
		_check(false, "the mounted level must still be available for the runtime probe")
		return
	_gameplay.apply_accessibility(restored)
	var director: Node3D = _gameplay.camera_director()
	_check(bool(_gameplay.reduced_motion()) and bool(director.reduced_motion()),
		"reduced motion must reach the camera director")
	_check(not bool(_gameplay.shake_enabled()) and not bool(director.shake_enabled()),
		"the shake switch must reach the camera director")
	var rig: Camera3D = director.active_rig()
	_check(rig != null and is_zero_approx(float(rig.pending_kick_degrees())),
		"a disabled shake must leave no pending camera kick")
	_check(bool(_gameplay.trajectory_assist()) \
			and bool((_gameplay.hud().trajectory_renderer() as Node).assist_enabled()),
		"the trajectory assist must reach the renderer")
	_gameplay.apply_accessibility(ACCESSIBILITY_SETTINGS.defaults())
	_check(not bool(_gameplay.reduced_motion()) and bool(_gameplay.shake_enabled()),
		"restoring the defaults must reach the presentation as well")


func _product_messages() -> Dictionary:
	var catalog: Dictionary = PRODUCT_TEXT_CATALOG.load_catalog(
		"res://data/ui/product_v2.pt-BR.json")
	if not bool(catalog.get("ok", false)):
		_check(false, "the closed pt-BR catalog must load for the overlay probe")
		return {}
	return (catalog.document as Dictionary).messages as Dictionary


func _check(condition: bool, message: String) -> void:
	if not condition:
		_errors.append(message)


func _fail(message: String) -> void:
	push_error("ui accessibility smoke: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	root.content_scale_factor = 1.0
	for node: Node in [_overlay, _gameplay, _router]:
		if is_instance_valid(node):
			if node.get_parent() == root:
				root.remove_child(node)
			node.free()
	_cleanup_root()
	quit(exit_code)


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
