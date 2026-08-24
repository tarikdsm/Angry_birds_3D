extends SceneTree

const APP_SHELL_SCENE := "res://scenes/app_shell.tscn"
const MARKER := "FAN_PROJECT_NOTICE_SMOKE_OK"
const NOTICE_ID := "FAN_PROJECT_NOTICE"
const RETRY_COPY := "NÃO FOI POSSÍVEL SALVAR. TENTE NOVAMENTE."

class FailingSaveStore extends RefCounted:
	var calls := 0

	func save_progress(_document: Dictionary, _catalog: Dictionary) -> Dictionary:
		calls += 1
		return {"ok": false, "error_kind": "test", "message": "forced failure"}

var _shell: Node
var _storage_root := ""


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	var packed := load(APP_SHELL_SCENE) as PackedScene
	if packed == null:
		_fail("AppShell must load the first-run notice")
		return
	_storage_root = "user://tests/task21-notice-%d-%d" % [
		OS.get_process_id(), Time.get_ticks_usec()]
	_shell = packed.instantiate()
	_shell.set_storage_root(_storage_root)
	root.add_child(_shell)
	await process_frame
	var notice := _shell.get_node_or_null(
		"ScreenRouter/ScreenHost/FanProjectNotice") as Control
	if notice == null:
		_fail("first run must show the fan notice before the main menu")
		return
	var legal := notice.find_child("LegalNotice", true, false) as Label
	var accept := notice.find_child("AcceptButton", true, false) as Button
	if legal == null or accept == null \
			or "fan game não oficial, sem afiliação ou endosso" not in legal.text.to_lower():
		_fail("notice must resolve the mandatory pt-BR disclaimer")
		return
	_push_key(KEY_ESCAPE)
	await process_frame
	if _shell.get_node_or_null("ScreenRouter/ScreenHost/FanProjectNotice") == null:
		_fail("semantic Back must be absorbed while the first-run notice is pending")
		return
	var before_accept := _read_progress()
	if before_accept.is_empty() or (before_accept.get("seen_tutorial_ids", []) as Array).has(NOTICE_ID):
		_fail("Back must not mark or persist the unseen fan notice")
		return
	var real_store: RefCounted = _shell.get("_save_store") as RefCounted
	var before_memory := (_shell.get("_progress") as Dictionary).duplicate(true)
	var progress_path := _storage_root.path_join("progress.v2.json")
	var before_disk := FileAccess.get_file_as_string(progress_path)
	var failing_store := FailingSaveStore.new()
	_shell.set("_save_store", failing_store)
	accept.pressed.emit()
	await process_frame
	notice = _shell.get_node_or_null(
		"ScreenRouter/ScreenHost/FanProjectNotice") as Control
	if notice == null:
		_fail("failed notice persistence must keep the current route")
		return
	var retry := notice.find_child("RetryNotice", true, false) as Label
	if failing_store.calls != 1 \
			or (_shell.get("_progress") as Dictionary) != before_memory \
			or FileAccess.get_file_as_string(progress_path) != before_disk:
		_fail("failed notice persistence must not mutate memory or disk")
		return
	if retry == null or not retry.visible or retry.text != RETRY_COPY:
		_fail("failed notice persistence must show the closed localized pt-BR retry message")
		return
	_shell.set("_save_store", real_store)
	accept = notice.find_child("AcceptButton", true, false) as Button
	accept.pressed.emit()
	await process_frame
	if _shell.get_node_or_null("ScreenRouter/ScreenHost/MainMenu") == null:
		_fail("explicit notice acceptance must route to the main menu")
		return
	var after_accept := _read_progress()
	if not (after_accept.get("seen_tutorial_ids", []) as Array).has(NOTICE_ID):
		_fail("explicit acceptance must atomically persist the notice ID")
		return
	print(MARKER)
	_finish(0)


func _read_progress() -> Dictionary:
	var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(
		_storage_root.path_join("progress.v2.json")))
	return parsed as Dictionary if parsed is Dictionary else {}


func _push_key(keycode: Key) -> void:
	var event := InputEventKey.new()
	event.physical_keycode = keycode
	event.pressed = true
	root.get_viewport().push_input(event, true)


func _fail(message: String) -> void:
	push_error("fan project notice smoke: %s" % message)
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
