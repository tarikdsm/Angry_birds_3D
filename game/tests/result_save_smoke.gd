extends SceneTree

## End-to-end product flow and terminal save contract.
##
## The smoke walks the shipped route — fan notice, menu, worlds, levels,
## briefing, gameplay, result, back to the menu — with real viewport input, and
## then asserts the save rules of the specification: a victory records once, a
## defeat records nothing and pause never advances the session.

const APP_SHELL_SCENE := "res://scenes/app_shell.tscn"
const MARKER := "RESULT_SAVE_SMOKE_OK"
## Frozen Orbital routes of the Task 18 playthrough fixtures. Neither needs an
## ability activation, so the replay stays independent from activation timing.
const VICTORY_ROUTE := [
	{"camera_right": Vector3(0.0, 0.9961947202682495, 0.08715574443340302),
		"pull_horizontal_m": -2.127, "pull_vertical_m": 0.0},
	{"camera_right": Vector3(0.0, 0.9998477101325989, 0.017452405765652657),
		"pull_horizontal_m": -2.127, "pull_vertical_m": 0.0},
]
const DEFEAT_ROUTE := [
	{"camera_right": Vector3(0.0, 0.0, 1.0),
		"pull_horizontal_m": -2.127, "pull_vertical_m": 0.0},
	{"camera_right": Vector3(0.0, 0.0, 1.0),
		"pull_horizontal_m": -2.127, "pull_vertical_m": 0.0},
	{"camera_right": Vector3(0.0, 0.0, 1.0),
		"pull_horizontal_m": -2.127, "pull_vertical_m": 0.0},
]
const RESOLVED_LEVEL_ID := "orbital/first_orbit_v2"
const INSPECTION_GUARD_FRAMES := 600
const RESOLUTION_GUARD_FRAMES := 2400
const SETTLE_FRAMES := 6

var _errors: Array[String] = []
var _shell: Node
var _storage_root := ""
var _resolutions: Array = []


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	if not ClassDB.class_exists("GameplaySessionNode"):
		_fail("GameplaySessionNode must be registered before the product flow")
		return
	if not await _boot_shell():
		_fail(" | ".join(_errors))
		return
	if not await _walk_to_gameplay():
		_fail(" | ".join(_errors))
		return
	await _check_pause()
	if _errors.is_empty():
		await _check_victory()
	if _errors.is_empty():
		await _check_defeat()
	if _errors.is_empty():
		await _check_return_to_menu()
	if not _errors.is_empty():
		_fail(" | ".join(_errors))
		return
	print(MARKER)
	_finish(0)


func _boot_shell() -> bool:
	var packed := load(APP_SHELL_SCENE) as PackedScene
	_shell = packed.instantiate()
	_storage_root = "user://tests/task24-result-%d-%d" % [
		OS.get_process_id(), Time.get_ticks_usec()]
	_shell.set_storage_root(_storage_root)
	root.add_child(_shell)
	await process_frame
	var notice := _shell.get_node_or_null(
		"ScreenRouter/ScreenHost/FanProjectNotice") as Control
	if notice == null:
		_check(false, "a new profile must open on the fan project notice")
		return false
	_push_key(KEY_ENTER)
	await process_frame
	if _shell.get_node_or_null("ScreenRouter/ScreenHost/MainMenu") == null:
		_check(false, "accepting the notice must reach the main menu")
		return false
	return true


func _walk_to_gameplay() -> bool:
	_push_key(KEY_DOWN)
	_push_key(KEY_ENTER)
	await process_frame
	var carousel := _shell.get_node_or_null(
		"ScreenRouter/ScreenHost/WorldCarousel") as Control
	if carousel == null:
		_check(false, "the menu must route to the world carousel")
		return false
	if carousel.current_world_id() != "orbital":
		_push_key(KEY_RIGHT)
		await process_frame
	if carousel.current_world_id() != "orbital":
		_check(false, "the carousel must be able to centre Orbital")
		return false
	_push_key(KEY_ENTER)
	await process_frame
	if _shell.get_node_or_null("ScreenRouter/ScreenHost/LevelSelect") == null:
		_check(false, "the carousel must route to the level list")
		return false
	_push_key(KEY_ENTER)
	await process_frame
	var briefing := _shell.get_node_or_null(
		"ScreenRouter/ScreenHost/NarrativeBrief") as Control
	if briefing == null:
		_check(false, "the level list must route to the briefing")
		return false
	_push_key(KEY_ENTER)
	await process_frame
	var router: Node = _shell.get_node("ScreenRouter")
	var session: Node3D = router.gameplay()
	if session == null:
		var notice := briefing.find_child("RetryNotice", true, false) as Label
		_check(false, "accepting the briefing must open the level: %s" % (
			notice.text if notice != null else ""))
		return false
	_check(router.current_route() == &"gameplay",
		"the shell must leave the frontend route for gameplay")
	_check(_shell.get_node_or_null("ScreenRouter/ScreenHost").get_child_count() == 0,
		"the gameplay route must leave the overlay host empty")
	_check(bool(session.session_configured),
		"the mounted level must be configured by the launch request")
	_check(str(session.current_request.get("level_id", "")) == "first_orbit_v2",
		"the mounted level must be the one the briefing announced")
	session.level_resolved.connect(_resolutions.append)
	var hud: CanvasLayer = session.hud()
	_check(hud != null and bool(hud.configured()),
		"the mounted level must bring its HUD up")
	for _frame: int in 4:
		await physics_frame
	_check(str((session.current_frame as Dictionary).get("phase", "")) == "inspection",
		"the mounted level must reach inspection")
	var objective := hud.root_control().find_child("ObjectiveLabel", true, false) as Label
	_check(objective != null and objective.text.contains("0/1"),
		"the HUD must publish the Orbital objective: %s" % (
			objective.text if objective != null else ""))
	return true


func _check_pause() -> void:
	var router: Node = _shell.get_node("ScreenRouter")
	var session: Node3D = router.gameplay()
	_push_key(KEY_P)
	await process_frame
	var pause := _shell.get_node_or_null("ScreenRouter/ScreenHost/PauseMenu") as Control
	_check(pause != null, "the pause intent must open the overlay")
	if pause == null:
		return
	_check(bool(session.is_paused()) and not bool(session.is_advancing()),
		"the pause overlay must stop the presentation from advancing the kernel")
	var consumed_before: int = session.consume_calls
	var tick_before := int((session.current_frame as Dictionary).get("tick", -1))
	for _frame: int in 30:
		await physics_frame
	_check(session.consume_calls == consumed_before \
			and int((session.current_frame as Dictionary).get("tick", -1)) == tick_before,
		"pause must never advance the session: %d vs %d" % [
			session.consume_calls, consumed_before])
	_check(router.gameplay() == session,
		"pause must keep the mounted level alive behind the overlay")
	_push_key(KEY_ESCAPE)
	await process_frame
	_check(_shell.get_node_or_null("ScreenRouter/ScreenHost/PauseMenu") == null \
			and not bool(session.is_paused()),
		"the semantic back must resume from the pause overlay")
	await physics_frame
	await physics_frame
	_check(session.consume_calls > consumed_before,
		"resuming must let the kernel advance again")


func _check_victory() -> void:
	var router: Node = _shell.get_node("ScreenRouter")
	var session: Node3D = router.gameplay()
	if not await _replay_route(session, VICTORY_ROUTE, "victory"):
		return
	for _frame: int in 4:
		await process_frame
	_check(_resolutions.size() == 1,
		"a terminal frame must publish exactly one resolution: %d" % _resolutions.size())
	if _resolutions.is_empty():
		return
	var summary := _resolutions[0] as Dictionary
	_check(str(summary.outcome) == "victory" and int(summary.stars) >= 1 \
			and int(summary.score) > 0 and int(summary.birds_used) >= 1,
		"the frozen Orbital route must resolve as a scored victory: %s" % summary)
	var result := _shell.get_node_or_null("ScreenRouter/ScreenHost/ResultScreen") as Control
	_check(result != null, "the terminal frame must open the result screen")
	if result == null:
		return
	var outcome_label := result.find_child("OutcomeLabel", true, false) as Label
	var score_label := result.find_child("ScoreLabel", true, false) as Label
	var record_label := result.find_child("RecordLabel", true, false) as Label
	_check(outcome_label != null and outcome_label.text == "VITÓRIA",
		"the result screen must announce the victory in pt-BR")
	_check(score_label != null and score_label.text.contains(str(int(summary.score))),
		"the result screen must publish the terminal score")
	_check(record_label != null and record_label.visible,
		"a first victory must be announced as a new record")
	var record := _saved_record()
	_check(bool(record.get("completed", false)) \
			and int(record.get("completion_count", 0)) == 1 \
			and int(record.get("best_score", 0)) == int(summary.score) \
			and int(record.get("best_stars", 0)) == int(summary.stars) \
			and int(record.get("best_birds_used", 0)) == int(summary.birds_used),
		"the victory must be recorded once with its terminal values: %s" % record)
	for _frame: int in 120:
		await physics_frame
	_check(_resolutions.size() == 1,
		"an already resolved level must never publish a second resolution")
	_check(int(_saved_record().get("completion_count", 0)) == 1,
		"an already resolved level must never write the record twice")


func _check_defeat() -> void:
	var record_before := _saved_record()
	_push_key(KEY_ENTER)
	await process_frame
	var router: Node = _shell.get_node("ScreenRouter")
	var session: Node3D = router.gameplay()
	_check(router.current_route() == &"gameplay" and session != null \
			and not bool(session.is_resolved()),
		"the result screen must be able to restart the level")
	if session == null:
		return
	_resolutions.clear()
	if not await _replay_route(session, DEFEAT_ROUTE, "defeat"):
		return
	for _frame: int in 4:
		await process_frame
	_check(_resolutions.size() == 1,
		"the defeat route must publish exactly one resolution: %d" % _resolutions.size())
	if _resolutions.is_empty():
		return
	_check(str((_resolutions[0] as Dictionary).outcome) == "defeat",
		"the frozen defeat route must resolve as a defeat")
	var result := _shell.get_node_or_null("ScreenRouter/ScreenHost/ResultScreen") as Control
	_check(result != null, "the defeat must also open the result screen")
	if result == null:
		return
	var outcome_label := result.find_child("OutcomeLabel", true, false) as Label
	_check(outcome_label != null and outcome_label.text == "DERROTA",
		"the result screen must announce the defeat in pt-BR")
	_check((result.find_child("RecordLabel", true, false) as Label).visible == false,
		"a defeat must never announce a record")
	_check(_saved_record() == record_before,
		"a defeat must leave the saved record untouched: %s" % _saved_record())


func _check_return_to_menu() -> void:
	var router: Node = _shell.get_node("ScreenRouter")
	_push_key(KEY_DOWN)
	_push_key(KEY_ENTER)
	await process_frame
	_check(_shell.get_node_or_null("ScreenRouter/ScreenHost/LevelSelect") != null,
		"the result screen must route back to the level list")
	_check(router.gameplay() == null,
		"leaving gameplay must release the mounted level")
	_check(_shell.get_node_or_null("ScreenRouter/GameplayHost").get_child_count() == 0,
		"leaving gameplay must leave the gameplay host empty")
	_push_key(KEY_ESCAPE)
	await process_frame
	_push_key(KEY_ESCAPE)
	await process_frame
	_check(_shell.get_node_or_null("ScreenRouter/ScreenHost/MainMenu") != null,
		"the semantic back must walk the player home")


func _replay_route(session: Node3D, route: Array, label: String) -> bool:
	var kernel: Node = session.get_node("Session")
	for index: int in route.size():
		var shot := route[index] as Dictionary
		if not await _wait_for_phase(session, ["inspection"], INSPECTION_GUARD_FRAMES):
			_check(false, "%s shot %d must begin in inspection: %s" % [
				label, index, _phase_of(session)])
			return false
		# A level that has just been reinstalled needs a few published frames
		# before the launcher accepts a gesture again.
		for _settle: int in SETTLE_FRAMES:
			await physics_frame
		if not kernel.queue_begin_grab(shot.camera_right as Vector3) \
				or not kernel.queue_pull(
					float(shot.pull_horizontal_m), float(shot.pull_vertical_m)):
			_check(false, "%s shot %d must be accepted by the kernel" % [label, index])
			return false
		await physics_frame
		if _phase_of(session) != "grabbed":
			var reasons: Array[String] = []
			for event: Variant in (session.current_frame as Dictionary).get("events", []):
				if event is Dictionary:
					reasons.append("%s/%s" % [
						(event as Dictionary).get("kind", ""),
						(event as Dictionary).get("rejection_reason_name", "")])
			_check(false, "%s shot %d must solve the gesture: %s %s" % [
				label, index, _phase_of(session), reasons])
			return false
		if not kernel.queue_release():
			_check(false, "%s shot %d must be releasable" % [label, index])
			return false
		await physics_frame
		if not await _wait_for_phase(
				session, ["inspection", "result"], RESOLUTION_GUARD_FRAMES):
			_check(false, "%s shot %d must resolve inside the watchdog: %s" % [
				label, index, _phase_of(session)])
			return false
	return _phase_of(session) == "result"


func _wait_for_phase(session: Node3D, phases: Array, guard: int) -> bool:
	for _frame: int in guard:
		if phases.has(_phase_of(session)):
			return true
		if not bool(session.is_advancing()):
			return phases.has(_phase_of(session))
		await physics_frame
	return phases.has(_phase_of(session))


func _phase_of(session: Node3D) -> String:
	return str((session.current_frame as Dictionary).get("phase", ""))


func _saved_record() -> Dictionary:
	var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(
		_storage_root.path_join("progress.v2.json")))
	if not parsed is Dictionary:
		return {}
	var levels := (parsed as Dictionary).get("levels", {}) as Dictionary
	return (levels.get(RESOLVED_LEVEL_ID, {}) as Dictionary).duplicate(true)


func _push_key(keycode: Key) -> void:
	var event := InputEventKey.new()
	event.physical_keycode = keycode
	event.pressed = true
	root.get_viewport().push_input(event, true)


func _check(condition: bool, message: String) -> void:
	if not condition:
		_errors.append(message)


func _fail(message: String) -> void:
	push_error("result save smoke: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	if is_instance_valid(_shell):
		if _shell.get_parent() == root:
			root.remove_child(_shell)
		_shell.free()
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
