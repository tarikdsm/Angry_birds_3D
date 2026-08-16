extends SceneTree

const BINDING_STORE := preload("res://scripts/input/binding_store.gd")
const INPUT_ROUTER := preload("res://scripts/input/input_router.gd")
const SAVE_STORE := preload("res://scripts/save/save_store.gd")
const SETTINGS_MODEL := preload("res://scripts/save/settings_model.gd")
const WORLD_CATALOG_PATH := "res://data/worlds/world_catalog.v2.json"
const SUCCESS_MARKER := "INPUT_REMAPPING_SMOKE_OK"

var _router: Node
var _received: Array = []
var _interrupt_after: StringName = &""
var _storage_root := ""


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	_router = INPUT_ROUTER.new()
	root.add_child(_router)
	_router.intent_submitted.connect(_received.append)
	var specs: Array = _router.official_binding_specs()
	var defaults: Array = _router.project_default_bindings()
	if specs.size() != 12 or defaults.size() != specs.size():
		_fail("all twelve discrete semantic actions must expose default bindings")
		return
	var captured_tokens: Array = []
	_router.binding_token_captured.connect(func(action: StringName, token: String) -> void:
		captured_tokens.append([action, token]))
	if not _router.begin_binding_capture(&"semantic_accept"):
		_fail("InputRouter must own the raw-to-token capture boundary")
		return
	_received.clear()
	var capture_key := InputEventKey.new()
	capture_key.physical_keycode = KEY_Q
	capture_key.pressed = true
	_router.route_raw_event(capture_key)
	if captured_tokens != [[&"semantic_accept", "key:%d" % KEY_Q]] \
			or not _received.is_empty():
		_fail("a captured token must be consumed without emitting its previous intent")
		return
	var malformed: Dictionary = BINDING_STORE.propose_rebind(
		defaults, &"semantic_accept", "key:not-a-number")
	if bool(malformed.get("ok", true)):
		_fail("malformed physical tokens must fail before mutation")
		return
	var cross_context: Dictionary = BINDING_STORE.propose_rebind(
		defaults, &"semantic_accept", "key:%d" % KEY_F)
	if not bool(cross_context.get("ok", false)) or bool(cross_context.get("conflict", true)):
		_fail("the same key must remain legal across frontend and gameplay contexts")
		return
	var before := defaults.duplicate(true)
	var proposal: Dictionary = BINDING_STORE.propose_rebind(
		before, &"semantic_accept", "key:%d" % KEY_ESCAPE)
	if not bool(proposal.get("conflict", false)) \
			or proposal.get("conflicting_action") != &"semantic_back" \
			or before != defaults:
		_fail("same-context conflicts must be reported before any mutation")
		return
	var cancelled: Dictionary = BINDING_STORE.resolve_rebind(before, proposal, false)
	if cancelled.get("bindings", []) != defaults:
		_fail("cancelling a conflict must preserve both previous bindings")
		return
	var confirmed: Dictionary = BINDING_STORE.resolve_rebind(before, proposal, true)
	if not bool(confirmed.get("ok", false)):
		_fail("confirmed same-context rebind must produce a candidate")
		return
	var rebound: Array = confirmed.get("bindings", [])
	for binding: Dictionary in rebound:
		if (binding.get("tokens", []) as Array).is_empty():
			_fail("conflict confirmation must swap bindings without unbinding an action")
			return
	var stale_base := defaults.duplicate(true)
	stale_base[0].tokens = ["key:%d" % KEY_W]
	var stale: Dictionary = BINDING_STORE.resolve_rebind(stale_base, proposal, true)
	if bool(stale.get("ok", true)) or stale.get("bindings", []) != stale_base:
		_fail("a proposal based on stale bindings must fail without mutation")
		return
	if not bool(_router.apply_bindings(rebound).get("ok", false)):
		_fail("a fully validated binding candidate must apply transactionally")
		return
	_received.clear()
	_router.set_context(&"frontend")
	var escape := InputEventKey.new()
	escape.physical_keycode = KEY_ESCAPE
	escape.pressed = true
	_router.route_raw_event(escape)
	if _received.size() != 1 or _received[0].kind != &"accept":
		_fail("confirmed remapping must change the emitted semantic intent")
		return
	_received.clear()
	var enter := InputEventKey.new()
	enter.physical_keycode = KEY_ENTER
	enter.pressed = true
	_router.route_raw_event(enter)
	if _received.size() != 1 or _received[0].kind != &"back":
		_fail("the consumed previous token must emit only its swapped intent")
		return

	var settings := SETTINGS_MODEL.default_document(rebound)
	_storage_root = "user://tests/task20-bindings-%d-%d" % [
		OS.get_process_id(), Time.get_ticks_usec()]
	var store := SAVE_STORE.new(_storage_root)
	var coerced_settings := settings.duplicate(true)
	coerced_settings.volumes.master = "1.0"
	if bool(store.save_settings(coerced_settings, specs).get("ok", true)):
		_fail("SaveStore must not coerce invalid settings field types")
		return
	if not bool(store.save_settings(settings, specs).get("ok", false)):
		_fail("remapped bindings must persist inside settings.v2")
		return
	var loaded: Dictionary = store.load_settings(defaults, specs)
	if not bool(loaded.get("ok", false)) \
			or (loaded.get("document", {}) as Dictionary).get("bindings", []) != rebound:
		_fail("persisted bindings must round-trip by semantic action")
		return
	if not bool(store.save_settings(settings, specs).get("ok", false)):
		_fail("a second settings save must establish a valid backup")
		return
	var primary := _storage_root.path_join("settings.v2.json")
	var backup := primary + ".bak"
	_write_text(primary, "{broken primary")
	_write_text(backup, "{broken backup")
	var recovered: Dictionary = store.load_settings(defaults, specs)
	var recovered_settings := recovered.get("document", {}) as Dictionary
	if not bool(recovered.get("ok", false)) or recovered.get("source") != &"defaults" \
			or recovered_settings.get("bindings", []) != defaults \
			or not is_equal_approx(float(recovered_settings.get("camera_sensitivity", 0.0)), 1.0):
		_fail("total settings recovery must restore default bindings and sensitivity")
		return
	if not bool(_router.apply_bindings(recovered_settings.bindings).get("ok", false)) \
			or not _router.set_camera_sensitivity(float(recovered_settings.camera_sensitivity)):
		_fail("recovered settings must be applicable to the runtime as one validated unit")
		return
	if not bool(store.save_settings(settings, specs).get("ok", false)):
		_fail("valid remapping must be restorable after total recovery")
		return
	_interrupt_after = &"primary_backed_up"
	var interrupted_store := SAVE_STORE.new(_storage_root, _continue_write)
	var changed_settings := settings.duplicate(true)
	changed_settings.camera_sensitivity = 1.5
	var interrupted: Dictionary = interrupted_store.save_settings(changed_settings, specs)
	var after_interruption: Dictionary = store.load_settings(defaults, specs)
	if bool(interrupted.get("ok", true)) \
			or not bool(after_interruption.get("ok", false)) \
			or (after_interruption.get("document", {}) as Dictionary) != settings:
		_fail("failed settings promotion must roll back to the last complete document")
		return

	var invalid := rebound.duplicate(true)
	invalid[0].action = "semantic_unknown"
	var runtime_before: Array = _router.capture_bindings()
	var rejected: Dictionary = _router.apply_bindings(invalid)
	if bool(rejected.get("ok", true)) or _router.capture_bindings() != runtime_before:
		_fail("invalid binding documents must leave InputMap unchanged")
		return

	_router.set_camera_sensitivity(2.0)
	_received.clear()
	var orbit := InputEventMouseMotion.new()
	orbit.button_mask = MOUSE_BUTTON_MASK_RIGHT
	orbit.relative = Vector2(2.0, 1.0)
	_router.set_context(&"gameplay")
	_router.route_raw_event(orbit)
	var pull := InputEventMouseMotion.new()
	pull.button_mask = MOUSE_BUTTON_MASK_LEFT
	pull.relative = Vector2(2.0, 1.0)
	_router.route_raw_event(pull)
	if _received.size() != 2 \
			or _received[0].payload.get("delta") != Vector2(4.0, 2.0) \
			or _received[1].payload.get("delta") != Vector2(2.0, 1.0):
		_fail("camera sensitivity must alter only orbit gain")
		return

	if not bool(_router.apply_bindings(defaults).get("ok", false)):
		_fail("restoring project defaults must be a valid transaction")
		return
	var restored := SETTINGS_MODEL.default_document(defaults)
	if not bool(store.save_settings(restored, specs).get("ok", false)):
		_fail("restoring defaults must persist atomically")
		return
	print(SUCCESS_MARKER)
	_finish(0)


func _fail(message: String) -> void:
	push_error("input remapping smoke: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	if is_instance_valid(_router):
		root.remove_child(_router)
		_router.free()
	_cleanup_root()
	quit(exit_code)


func _continue_write(phase: StringName) -> bool:
	return phase != _interrupt_after


func _write_text(path: String, value: String) -> void:
	var file := FileAccess.open(path, FileAccess.WRITE)
	if file == null:
		_fail("could not corrupt settings fixture")
		return
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
