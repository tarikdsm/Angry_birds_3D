extends SceneTree

const APP_SHELL_SCENE := "res://scenes/app_shell.tscn"
const SUCCESS_MARKER := "APP_SHELL_SMOKE_OK"
const RETRY_COPY := "NÃO FOI POSSÍVEL SALVAR. TENTE NOVAMENTE."

class FailingSaveStore extends RefCounted:
	var calls := 0

	func save_progress(_document: Dictionary, _catalog: Dictionary) -> Dictionary:
		calls += 1
		return {"ok": false, "error_kind": "test", "message": "forced failure"}


var _shell: Node
var _exit_requests := 0
var _storage_root := ""
var _task21_errors: Array[String] = []


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	# The shipped window is 1280x720; headless starts at 64x64 and the product
	# stretch mode follows the real aspect, so the probe pins the shipped size.
	root.size = Vector2i(1280, 720)
	await process_frame
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
	_write_text(_storage_root.path_join("progress.v2.json"),
		JSON.stringify(_progress_fixture(), "", true, true) + "\n")
	_write_text(_storage_root.path_join("settings.v2.json"), "{broken settings primary")
	_write_text(_storage_root.path_join("settings.v2.json.bak"), "{broken settings backup")
	_shell.set_storage_root(_storage_root)
	root.add_child(_shell)
	await process_frame
	var fan_notice := _shell.get_node_or_null("ScreenRouter/ScreenHost/FanProjectNotice") as Control
	if fan_notice == null:
		_fail("first-run app shell must route through the fan project notice")
		return
	await _assert_controls_visible(fan_notice, "fan notice at 100 percent")
	(fan_notice.find_child("AcceptButton", true, false) as Button).pressed.emit()
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
	await _exercise_task21_routes_at_scale(100)
	root.get_viewport().content_scale_factor = 1.5
	await _exercise_task21_routes_at_scale(150)
	root.get_viewport().content_scale_factor = 2.0
	await _exercise_task21_routes_at_scale(200)
	root.get_viewport().content_scale_factor = 1.0

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
	var second_packed := load(APP_SHELL_SCENE) as PackedScene
	var second_shell := second_packed.instantiate()
	second_shell.set_storage_root(_storage_root)
	root.add_child(second_shell)
	await process_frame
	_record(second_shell.get_node_or_null("ScreenRouter/ScreenHost/FanProjectNotice") == null \
			and second_shell.get_node_or_null("ScreenRouter/ScreenHost/MainMenu") != null,
		"saved tutorial acceptance must bypass the first-run notice on the next launch")
	var relaunched_progress := _read_progress()
	_record(str(relaunched_progress.get("last_world_id", "")) == "orbital" \
			and str(relaunched_progress.get("last_level_id", "")) == "first_orbit_v2",
		"relaunch must retain Orbital with its matching last level")
	var second_main := second_shell.get_node_or_null("ScreenRouter/ScreenHost/MainMenu") as Control
	if second_main != null:
		(second_main.find_child("WorldsButton", true, false) as Button).pressed.emit()
		await process_frame
		var relaunched_carousel := second_shell.get_node_or_null(
			"ScreenRouter/ScreenHost/WorldCarousel") as Control
		_record(relaunched_carousel != null \
				and relaunched_carousel.current_world_id() == "orbital",
			"relaunch must center the persisted Orbital world")
	root.remove_child(second_shell)
	second_shell.free()
	if not _task21_errors.is_empty():
		_fail(" | ".join(_task21_errors))
		return

	print(SUCCESS_MARKER)
	_finish(0)


func _exercise_task21_routes_at_scale(scale_percent: int) -> void:
	var screen_router := _shell.get_node("ScreenRouter")
	if _shell.get_node_or_null("ScreenRouter/ScreenHost/MainMenu") == null:
		screen_router.show_main_menu()
		await process_frame

	for _step: int in range(4):
		_push_key(KEY_DOWN)
	_push_key(KEY_ENTER)
	await process_frame
	var about := _shell.get_node_or_null("ScreenRouter/ScreenHost/AboutScreen") as Control
	_record(about != null,
		"real semantic navigation must route About from the menu at %d percent" % scale_percent)
	if about != null:
		await _assert_controls_visible(about, "About at %d percent" % scale_percent)
		_push_key(KEY_ESCAPE)
		await process_frame
	_record(_shell.get_node_or_null("ScreenRouter/ScreenHost/MainMenu") != null,
		"real semantic Back must return from About at %d percent" % scale_percent)
	if _shell.get_node_or_null("ScreenRouter/ScreenHost/MainMenu") == null:
		screen_router.show_main_menu()
		await process_frame

	_push_key(KEY_DOWN)
	_push_key(KEY_ENTER)
	await process_frame
	var carousel := _shell.get_node_or_null("ScreenRouter/ScreenHost/WorldCarousel") as Control
	_record(carousel != null,
		"real semantic navigation must route Worlds at %d percent" % scale_percent)
	if carousel == null:
		screen_router.show_world_carousel()
		await process_frame
		carousel = _shell.get_node_or_null("ScreenRouter/ScreenHost/WorldCarousel") as Control
	if carousel == null:
		return
	await _assert_controls_visible(carousel, "World carousel at %d percent" % scale_percent)
	var rendered_record := ""
	for child: Node in carousel.find_child("CardHost", true, false).get_children():
		if child is Button and str(child.get_meta("world_id", "")) == "earth":
			rendered_record = (child as Button).text
	_record("54321" in rendered_record and "3" in rendered_record,
		"nonzero saved score/stars must render in the localized Earth card")
	if carousel.current_world_id() == "orbital":
		_push_key(KEY_LEFT)
		await process_frame
	_record(carousel.current_world_id() == "earth",
		"real semantic Left must select Earth in the carousel")

	var orbital_neighbor := _find_world_card(carousel, "orbital")
	_record(orbital_neighbor != null,
		"Earth-centered carousel must expose a live Orbital neighbor at %d percent" \
			% scale_percent)
	if orbital_neighbor == null:
		return
	var neighbor_center := orbital_neighbor.get_global_rect().get_center()
	_push_mouse_motion(neighbor_center)
	await process_frame
	_record(root.get_viewport().gui_get_focus_owner() == orbital_neighbor,
		"real mouse motion must focus the live Orbital neighbor at %d percent" \
			% scale_percent)
	_push_left_click(neighbor_center)
	await process_frame
	carousel = _shell.get_node_or_null("ScreenRouter/ScreenHost/WorldCarousel") as Control
	_record(carousel != null and carousel.current_world_id() == "orbital",
		"real LMB on the neighbor must keep the carousel and center Orbital at %d percent" \
			% scale_percent)
	if carousel == null:
		return
	var card_host := carousel.find_child("CardHost", true, false)
	var rig_host := carousel.find_child("RigHost", true, false)
	_record(card_host != null and card_host.get_child_count() <= 3 \
			and rig_host != null and rig_host.get_child_count() <= 2,
		"real neighbor click must preserve live card/rig budgets at %d percent" \
			% scale_percent)
	var mouse_rebuild_focus := root.get_viewport().gui_get_focus_owner() as Control
	_record(is_instance_valid(mouse_rebuild_focus) \
			and mouse_rebuild_focus.is_visible_in_tree() \
			and str(mouse_rebuild_focus.get_meta("world_id", "")) == "orbital",
		"real neighbor click must focus the live rebuilt center at %d percent" \
			% scale_percent)

	var central_orbital := _find_world_card(carousel, "orbital")
	_record(central_orbital != null,
		"rebuilt carousel must expose a newly acquired central Orbital card")
	if central_orbital == null:
		return
	var central_center := central_orbital.get_global_rect().get_center()
	_push_mouse_motion(central_center)
	await process_frame
	_record(root.get_viewport().gui_get_focus_owner() == central_orbital,
		"real mouse motion must focus the reacquired central Orbital card at %d percent" \
			% scale_percent)
	_push_left_click(central_center)
	await process_frame
	var levels := _shell.get_node_or_null("ScreenRouter/ScreenHost/LevelSelect") as Control
	_record(levels != null,
		"real LMB on the reacquired center must route to LevelSelect at %d percent" \
			% scale_percent)
	if levels == null:
		return
	var saved_after_mouse := _read_progress()
	_record(str(saved_after_mouse.get("last_world_id", "")) == "orbital" \
			and str(saved_after_mouse.get("last_level_id", "")) == "first_orbit_v2",
		"real central click must persist Orbital and its default phase")
	var mouse_phase_focus := root.get_viewport().gui_get_focus_owner() as Control
	_record(is_instance_valid(mouse_phase_focus) and mouse_phase_focus.is_visible_in_tree() \
			and str(mouse_phase_focus.get_meta("level_id", "")) == "first_orbit_v2",
		"real central click must focus the live Orbital phase at %d percent" % scale_percent)
	_push_key(KEY_ESCAPE)
	await process_frame
	carousel = _shell.get_node_or_null("ScreenRouter/ScreenHost/WorldCarousel") as Control
	_record(carousel != null and carousel.current_world_id() == "orbital",
		"real Back after the mouse route must restore the Orbital carousel")
	if carousel == null:
		return

	_push_key(KEY_LEFT)
	await process_frame
	_record(carousel.current_world_id() == "earth",
		"real semantic Left must still select Earth after the mouse route")
	_push_key(KEY_RIGHT)
	await process_frame
	_record(carousel.current_world_id() == "orbital",
		"real semantic Right must still select Orbital after the mouse route")
	var horizontal_focus := root.get_viewport().gui_get_focus_owner() as Control
	_record(horizontal_focus != null and horizontal_focus.is_visible_in_tree() \
			and str(horizontal_focus.get_meta("world_id", "")) == "orbital",
		"horizontal rebuild must leave visible focus on the Orbital card")
	_push_key(KEY_ENTER)
	await process_frame
	levels = _shell.get_node_or_null("ScreenRouter/ScreenHost/LevelSelect") as Control
	_record(levels != null,
		"accepting the focused centered Orbital card must route to its level")
	if levels == null:
		return
	await _assert_controls_visible(levels, "Orbital level select at %d percent" % scale_percent)
	var phase_buttons := levels.find_child("LevelHost", true, false).get_children()
	_record(phase_buttons.size() == 1 \
			and (phase_buttons[0] as Button).text \
				== "Primeira Órbita — Contrapeso de Aster",
		"Orbital phase must resolve its closed pt-BR text token")
	var saved := _read_progress()
	_record(str(saved.get("last_world_id", "")) == "orbital" \
			and str(saved.get("last_level_id", "")) == "first_orbit_v2",
		"world selection must atomically persist Orbital and its default level")
	_push_key(KEY_ESCAPE)
	await process_frame
	carousel = _shell.get_node_or_null("ScreenRouter/ScreenHost/WorldCarousel") as Control
	_record(carousel != null and carousel.current_world_id() == "orbital",
		"Level Back must rebuild the carousel with Orbital retained")
	if carousel == null:
		return
	_push_key(KEY_ENTER)
	await process_frame
	levels = _shell.get_node_or_null("ScreenRouter/ScreenHost/LevelSelect") as Control
	_record(levels != null,
		"real semantic Enter must reopen the retained Orbital phase")
	if levels == null:
		return
	var focused_level := root.get_viewport().gui_get_focus_owner() as Control
	_record(focused_level != null and focused_level.is_visible_in_tree() \
			and str(focused_level.get_meta("level_id", "")) == "first_orbit_v2",
		"dynamic Orbital phase must receive visible focus after configure")
	_push_key(KEY_ENTER)
	await process_frame
	var briefing := _shell.get_node_or_null("ScreenRouter/ScreenHost/NarrativeBrief") as Control
	_record(briefing != null,
		"real semantic accept on the dynamic phase must route to briefing")
	if briefing == null:
		return
	await _assert_controls_visible(briefing, "Orbital briefing at %d percent" % scale_percent)
	var briefing_text := briefing.find_child("BriefingText", true, false) as Label
	_record(briefing_text != null and briefing_text.text.split("\n").size() in [2, 3],
		"Orbital briefing must contain 2-3 localized lines")
	(briefing.find_child("ContinueButton", true, false) as Button).grab_focus()
	if scale_percent == 100:
		var real_store: RefCounted = _shell.get("_save_store") as RefCounted
		var before_memory := (_shell.get("_progress") as Dictionary).duplicate(true)
		var progress_path := _storage_root.path_join("progress.v2.json")
		var before_disk := FileAccess.get_file_as_string(progress_path)
		var failing_store := FailingSaveStore.new()
		_shell.set("_save_store", failing_store)
		_push_key(KEY_ENTER)
		await process_frame
		briefing = _shell.get_node_or_null(
			"ScreenRouter/ScreenHost/NarrativeBrief") as Control
		_record(briefing != null,
			"failed briefing persistence must keep the current route")
		_record(failing_store.calls == 1 \
				and (_shell.get("_progress") as Dictionary) == before_memory \
				and FileAccess.get_file_as_string(progress_path) == before_disk,
			"failed briefing persistence must not mutate memory or disk")
		if briefing != null:
			var retry := briefing.find_child("RetryNotice", true, false) as Label
			_record(retry != null and retry.visible and retry.text == RETRY_COPY,
				"failed briefing persistence must show the closed localized pt-BR retry message")
		_shell.set("_save_store", real_store)
		if briefing == null:
			screen_router.show_narrative_brief()
			await process_frame
			briefing = _shell.get_node_or_null(
				"ScreenRouter/ScreenHost/NarrativeBrief") as Control
		if briefing != null:
			(briefing.find_child("ContinueButton", true, false) as Button).grab_focus()
			_push_key(KEY_ENTER)
			await process_frame
	else:
		_push_key(KEY_ENTER)
		await process_frame
	saved = _read_progress()
	_record((saved.get("seen_briefing_ids", []) as Array).has("orbital/first_orbit_v2"),
		"briefing accept/skip must atomically persist the Orbital briefing ID")
	if _shell.get_node_or_null("ScreenRouter/ScreenHost/MainMenu") == null:
		screen_router.show_main_menu()
		await process_frame

	screen_router.show_fan_project_notice()
	await process_frame
	var notice := _shell.get_node_or_null("ScreenRouter/ScreenHost/FanProjectNotice") as Control
	if notice != null:
		await _assert_controls_visible(notice, "fan notice at %d percent" % scale_percent)
		_push_key(KEY_ENTER)
		await process_frame
	_record(_shell.get_node_or_null("ScreenRouter/ScreenHost/MainMenu") != null,
		"semantic accept must leave the notice route at %d percent" % scale_percent)


func _assert_controls_visible(screen: Control, label: String) -> void:
	var view := root.get_viewport().get_visible_rect()
	for button: Button in _visible_buttons(screen):
		button.grab_focus()
		await process_frame
		_record(button.is_visible_in_tree() and view.encloses(button.get_global_rect()),
			"%s must keep focused control visible without clipping: %s view=%s rect=%s" % [
				label, button.name, view, button.get_global_rect()])


func _find_world_card(carousel: Control, world_id: String) -> Button:
	for child: Node in carousel.find_child("CardHost", true, false).get_children():
		if child is Button and str(child.get_meta("world_id", "")) == world_id:
			return child as Button
	return null


func _read_progress() -> Dictionary:
	var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(
		_storage_root.path_join("progress.v2.json")))
	return parsed as Dictionary if parsed is Dictionary else {}


func _record(condition: bool, message: String) -> void:
	if not condition:
		_task21_errors.append(message)


func _progress_fixture() -> Dictionary:
	return {
		"schema_version": 2,
		"profile_id": "default",
		"available_world_ids": ["earth", "orbital"],
		"available_level_ids": ["earth/farm_reaction", "orbital/first_orbit_v2"],
		"levels": {
			"earth/farm_reaction": {
				"completed": true, "best_score": 54321, "best_stars": 3,
				"best_birds_used": 2, "completion_count": 1,
			},
			"orbital/first_orbit_v2": {
				"completed": false, "best_score": 0, "best_stars": 0,
				"best_birds_used": 0, "completion_count": 0,
			},
		},
		"seen_briefing_ids": [],
		"seen_tutorial_ids": [],
		"last_world_id": "earth",
		"last_level_id": "farm_reaction",
	}


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
