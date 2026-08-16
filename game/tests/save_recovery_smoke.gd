extends SceneTree

const PROGRESS_MODEL := preload("res://scripts/save/progress_model.gd")
const SAVE_STORE := preload("res://scripts/save/save_store.gd")
const WORLD_CATALOG_PATH := "res://data/worlds/world_catalog.v2.json"
const MATERIALS_PATH := "res://data/materials/vertical_slice.materials.json"
const ARCHETYPES_PATH := "res://data/archetypes/vertical_slice.archetypes.json"
const LEVEL_PATH := "res://data/levels/first_orbit.level.json"
const SUCCESS_MARKER := "SAVE_RECOVERY_SMOKE_OK"

var _interrupt_after: StringName = &""
var _storage_root := ""
var _simulation_probe: Node
var _simulation_frame_bytes := PackedByteArray()
var _simulation_frame_hash := 0


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	if SAVE_STORE.normalize_storage_root("user://") != "user://" \
			or SAVE_STORE.normalize_storage_root("user://tests/") != "user://tests":
		_fail("storage normalization must preserve the default user:// root")
		return
	var world_catalog_value: Variant = JSON.parse_string(
		FileAccess.get_file_as_string(WORLD_CATALOG_PATH))
	if not world_catalog_value is Dictionary:
		_fail("world catalog fixture must parse")
		return
	var world_catalog := world_catalog_value as Dictionary
	if not _begin_simulation_isolation_probe():
		return
	_storage_root = "user://tests/task20-save-%d-%d" % [
		OS.get_process_id(), Time.get_ticks_usec()]
	var store := SAVE_STORE.new(_storage_root)
	var fresh: Dictionary = store.load_progress(world_catalog)
	if not bool(fresh.get("ok", false)) or fresh.get("source") != &"defaults":
		_fail("a new profile must load canonical defaults")
		return
	var progress := fresh.get("document", {}) as Dictionary
	var coerced_progress := progress.duplicate(true)
	coerced_progress.schema_version = "2"
	if bool(store.save_progress(coerced_progress, world_catalog).get("ok", true)):
		_fail("SaveStore must not coerce invalid progress field types")
		return
	if not FileAccess.file_exists(_storage_root.path_join("progress.v2.json")):
		_fail("fresh defaults must be installed as a durable primary")
		return
	if progress.get("available_world_ids", []) != ["earth", "orbital"] \
			or progress.get("available_level_ids", []) != [
				"earth/farm_reaction", "orbital/first_orbit_v2"]:
		_fail("new and recovered profiles must expose both first levels")
		return
	var extended_catalog := world_catalog.duplicate(true)
	var extended_earth := (extended_catalog.worlds as Array)[0] as Dictionary
	(extended_earth.level_order as Array).append("locked_after_farm")
	(extended_earth.levels as Array).append({
		"id": "locked_after_farm",
		"region_id": "farm",
		"camera_profile_id": "CAM_Farm",
		"presentation_profile_id": "PRS_Farm",
		"scene_id": "SCN_FarmReaction",
		"unlock_after_level_id": "farm_reaction",
	})
	var extended_defaults: Dictionary = PROGRESS_MODEL.default_document(extended_catalog)
	if (extended_defaults.available_level_ids as Array).has("earth/locked_after_farm") \
			or not (extended_defaults.levels as Dictionary).has("earth/locked_after_farm"):
		_fail("locked future levels need records but must not open in a fresh profile")
		return
	var premature_victory := {
		"world_id": "earth", "level_id": "locked_after_farm", "outcome": "victory",
		"score": 1, "stars": 1, "birds_used": 1,
	}
	var premature_result: Dictionary = PROGRESS_MODEL.apply_result(
		extended_defaults, premature_victory, extended_catalog)
	if bool(premature_result.get("ok", true)) \
			or bool(premature_result.get("changed", true)):
		_fail("results for locked levels must be rejected without changing progress")
		return
	var extended_victory := {
		"world_id": "earth", "level_id": "farm_reaction", "outcome": "victory",
		"score": 1, "stars": 1, "birds_used": 1,
	}
	var unlocked_result: Dictionary = PROGRESS_MODEL.apply_result(
		extended_defaults, extended_victory, extended_catalog)
	var unlocked_progress := unlocked_result.get("document", {}) as Dictionary
	if not bool(unlocked_result.get("ok", false)) \
			or not (unlocked_progress.available_level_ids as Array).has(
				"earth/locked_after_farm") \
			or not PROGRESS_MODEL.validate_document(unlocked_progress, extended_catalog).is_empty():
		_fail("a victory must unlock its catalog successor as valid progress")
		return
	if not PROGRESS_MODEL.validate_document(progress, world_catalog).is_empty():
		_fail("fresh progress must satisfy the closed schema")
		return
	var unknown_nested := progress.duplicate(true)
	unknown_nested.levels["earth/farm_reaction"].unexpected = true
	if PROGRESS_MODEL.validate_document(unknown_nested, world_catalog).is_empty():
		_fail("progress must reject unknown nested record keys")
		return
	var impossible_incomplete := progress.duplicate(true)
	impossible_incomplete.levels["earth/farm_reaction"].best_score = 1
	if PROGRESS_MODEL.validate_document(impossible_incomplete, world_catalog).is_empty():
		_fail("incomplete levels must not carry victory records")
		return
	var impossible_complete := progress.duplicate(true)
	impossible_complete.levels["earth/farm_reaction"].completed = true
	if PROGRESS_MODEL.validate_document(impossible_complete, world_catalog).is_empty():
		_fail("completed levels must carry a positive completion record")
		return
	var tampered_locked := extended_defaults.duplicate(true)
	var tampered_record := tampered_locked.levels["earth/locked_after_farm"] as Dictionary
	tampered_record.completed = true
	tampered_record.best_score = 1
	tampered_record.best_stars = 1
	tampered_record.best_birds_used = 1
	tampered_record.completion_count = 1
	if PROGRESS_MODEL.validate_document(tampered_locked, extended_catalog).is_empty():
		_fail("a forged completion for a locked level must not unlock the chain")
		return

	var victory := {
		"world_id": "earth", "level_id": "farm_reaction", "outcome": "victory",
		"score": 42000, "stars": 2, "birds_used": 3,
	}
	var first_result: Dictionary = PROGRESS_MODEL.apply_result(progress, victory, world_catalog)
	var first_progress := first_result.get("document", {}) as Dictionary
	var weaker := victory.duplicate(true)
	weaker.score = 1000
	weaker.stars = 1
	weaker.birds_used = 4
	var second_result: Dictionary = PROGRESS_MODEL.apply_result(first_progress, weaker, world_catalog)
	var second_progress := second_result.get("document", {}) as Dictionary
	var record := (second_progress.get("levels", {}) as Dictionary).get(
		"earth/farm_reaction", {}) as Dictionary
	if int(record.get("best_score", -1)) != 42000 \
			or int(record.get("best_stars", -1)) != 2 \
			or int(record.get("best_birds_used", -1)) != 3 \
			or int(record.get("completion_count", -1)) != 2:
		_fail("victories must preserve best records and count once per application")
		return
	var zero_star_victory := victory.duplicate(true)
	zero_star_victory.stars = 0
	if bool(PROGRESS_MODEL.apply_result(progress, zero_star_victory, world_catalog).get(
			"ok", true)):
		_fail("every recorded victory must award at least one star")
		return
	var defeat := victory.duplicate(true)
	defeat.outcome = "defeat"
	defeat.score = 999999
	defeat.stars = 3
	var defeat_result: Dictionary = PROGRESS_MODEL.apply_result(second_progress, defeat, world_catalog)
	if bool(defeat_result.get("changed", true)) \
			or defeat_result.get("document", {}) != second_progress:
		_fail("defeat must not write a level record")
		return
	var unknown_outcome := defeat.duplicate(true)
	unknown_outcome.outcome = "timeout"
	if bool(PROGRESS_MODEL.apply_result(second_progress, unknown_outcome, world_catalog).get(
			"ok", true)):
		_fail("unknown result outcomes must fail closed instead of looking like defeat")
		return

	if not bool(store.save_progress(second_progress, world_catalog).get("ok", false)):
		_fail("valid progress must be persisted")
		return
	var primary := _storage_root.path_join("progress.v2.json")
	var backup := primary + ".bak"
	var canonical_bytes := FileAccess.get_file_as_string(primary)
	var parsed_canonical: Variant = JSON.parse_string(canonical_bytes)
	if not parsed_canonical is Dictionary \
			or canonical_bytes != JSON.stringify(second_progress, "", true, true) + "\n" \
			or not PROGRESS_MODEL.validate_document(parsed_canonical, world_catalog).is_empty():
		_fail("progress must use deterministic canonical JSON bytes")
		return
	if not bool(store.save_progress(second_progress, world_catalog).get("ok", false)) \
			or FileAccess.get_file_as_string(primary) != canonical_bytes:
		_fail("saving the same progress twice must be byte-idempotent")
		return
	_write_text(primary, "{broken primary")
	var recovered: Dictionary = store.load_progress(world_catalog)
	if not bool(recovered.get("ok", false)) or recovered.get("source") != &"backup" \
			or not bool(recovered.get("recovered", false)):
		_fail("a corrupt primary must recover its valid backup")
		return

	_write_text(primary, "{broken primary again")
	_write_text(backup, "{broken backup")
	var total_recovery: Dictionary = store.load_progress(world_catalog)
	if not bool(total_recovery.get("ok", false)) \
			or total_recovery.get("source") != &"defaults" \
			or total_recovery.get("warning_message_id") != &"app.recovery.total" \
			or (total_recovery.get("diagnostic_paths", []) as Array).size() != 2:
		_fail("two corrupt files must be preserved before safe defaults are installed")
		return
	for diagnostic_path: String in total_recovery.get("diagnostic_paths", []):
		if not FileAccess.file_exists(diagnostic_path) or ".corrupt." not in diagnostic_path:
			_fail("recovery diagnostic path must preserve a corrupt source")
			return
	var first_diagnostics := (total_recovery.get("diagnostic_paths", []) as Array).duplicate()
	_write_text(primary, "{third corrupt primary")
	_write_text(backup, "{fourth corrupt backup")
	var repeated_recovery: Dictionary = SAVE_STORE.new(_storage_root).load_progress(world_catalog)
	var repeated_diagnostics := repeated_recovery.get("diagnostic_paths", []) as Array
	if not bool(repeated_recovery.get("ok", false)) \
			or repeated_recovery.get("source") != &"defaults" \
			or repeated_diagnostics.size() != 2:
		_fail("a fresh SaveStore instance must preserve repeated corruption")
		return
	for diagnostic_path: String in first_diagnostics:
		if not FileAccess.file_exists(diagnostic_path) \
				or repeated_diagnostics.has(diagnostic_path):
			_fail("repeated recovery must keep every earlier diagnostic distinct")
			return

	var stable: Dictionary = repeated_recovery.get("document", {})
	var improved_result: Dictionary = PROGRESS_MODEL.apply_result(stable, victory, world_catalog)
	var improved := improved_result.get("document", {}) as Dictionary
	_interrupt_after = &"tmp_flushed"
	var interrupted_store := SAVE_STORE.new(_storage_root, _continue_write)
	var interrupted: Dictionary = interrupted_store.save_progress(improved, world_catalog)
	if bool(interrupted.get("ok", true)) or interrupted.get("error_kind") != &"interrupted":
		_fail("the temp-flush interruption seam must stop promotion")
		return
	var after_temp: Dictionary = store.load_progress(world_catalog)
	if not bool(after_temp.get("ok", false)):
		_fail("interruption after temp flush must leave the primary valid")
		return

	_interrupt_after = &"primary_backed_up"
	interrupted = interrupted_store.save_progress(improved, world_catalog)
	if bool(interrupted.get("ok", true)):
		_fail("the backup interruption seam must stop promotion")
		return
	var after_backup: Dictionary = store.load_progress(world_catalog)
	if not bool(after_backup.get("ok", false)) or after_backup.get("source") != &"backup":
		_fail("interruption after backup must leave a recoverable backup")
		return
	if not _finish_simulation_isolation_probe():
		return

	_cleanup_root(_storage_root)
	if DirAccess.dir_exists_absolute(ProjectSettings.globalize_path(_storage_root)):
		_fail("the isolated user test directory must be removable without residue")
		return
	print(SUCCESS_MARKER)
	_free_simulation_probe()
	quit(0)


func _begin_simulation_isolation_probe() -> bool:
	if not ClassDB.class_exists("OrbitalSessionNode"):
		_fail("OrbitalSessionNode must be available for the frame/hash isolation proof")
		return false
	_simulation_probe = ClassDB.instantiate("OrbitalSessionNode")
	root.add_child(_simulation_probe)
	if not _simulation_probe.configure_session(
		FileAccess.get_file_as_string(MATERIALS_PATH),
		FileAccess.get_file_as_string(ARCHETYPES_PATH),
		FileAccess.get_file_as_string(LEVEL_PATH)):
		_fail("frame/hash isolation probe could not configure the legacy simulation")
		return false
	_simulation_probe.consume_frame()
	var frame_before: Dictionary = _simulation_probe.consume_frame()
	_simulation_frame_bytes = var_to_bytes(frame_before)
	_simulation_frame_hash = hash(_simulation_frame_bytes)
	return true


func _finish_simulation_isolation_probe() -> bool:
	var frame_after: Dictionary = _simulation_probe.consume_frame()
	var frame_after_bytes := var_to_bytes(frame_after)
	if frame_after_bytes != _simulation_frame_bytes \
			or hash(frame_after_bytes) != _simulation_frame_hash:
		_fail("save and recovery operations must not alter the simulation frame or hash")
		return false
	return true


func _free_simulation_probe() -> void:
	if not is_instance_valid(_simulation_probe):
		return
	if _simulation_probe.get_parent() == root:
		root.remove_child(_simulation_probe)
	_simulation_probe.free()


func _continue_write(phase: StringName) -> bool:
	return phase != _interrupt_after


func _write_text(path: String, text: String) -> void:
	var file := FileAccess.open(path, FileAccess.WRITE)
	if file == null:
		_fail("test fixture could not open %s" % path)
		return
	file.store_string(text)
	file.flush()


func _cleanup_root(path: String) -> void:
	var absolute := ProjectSettings.globalize_path(path)
	var directory := DirAccess.open(absolute)
	if directory == null:
		return
	for file_name: String in directory.get_files():
		DirAccess.remove_absolute(absolute.path_join(file_name))
	DirAccess.remove_absolute(absolute)


func _fail(message: String) -> void:
	push_error("save recovery smoke: %s" % message)
	_free_simulation_probe()
	_cleanup_root(_storage_root)
	quit(1)
