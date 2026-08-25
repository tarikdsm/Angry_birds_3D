extends RefCounted

const PROGRESS_MODEL := preload("res://scripts/save/progress_model.gd")
const SETTINGS_MODEL := preload("res://scripts/save/settings_model.gd")
const PROGRESS_FILE := "progress.v2.json"
const SETTINGS_FILE := "settings.v2.json"
const TOTAL_RECOVERY_MESSAGE_ID := &"app.recovery.total"
const MIGRATED_MESSAGE_ID := &"app.recovery.migrated"

var _storage_root: String
var _phase_hook: Callable
var _diagnostic_counter := 0


func _init(storage_root: String = "user://", phase_hook: Callable = Callable()) -> void:
	_storage_root = normalize_storage_root(storage_root)
	_phase_hook = phase_hook


static func normalize_storage_root(storage_root: String) -> String:
	return "user://" if storage_root == "user://" else storage_root.trim_suffix("/")


func load_progress(world_catalog: Dictionary) -> Dictionary:
	var defaults := PROGRESS_MODEL.default_document(world_catalog)
	return _load_document(PROGRESS_FILE, defaults, &"progress", world_catalog)


func save_progress(document: Dictionary, world_catalog: Dictionary) -> Dictionary:
	return _save_document(PROGRESS_FILE, document, &"progress", world_catalog)


## Records one confirmed terminal result.
##
## The write only happens for a victory that actually improves or creates a
## record; a defeat and a repeated identical victory leave the file untouched.
## Idempotency at the level of the frame belongs to the caller, which is only
## allowed to call this once per confirmed terminal frame.
func save_level_result(
		progress: Dictionary, result: Dictionary, world_catalog: Dictionary) -> Dictionary:
	var previous := ((progress.get("levels", {}) as Dictionary).get(
		"%s/%s" % [result.get("world_id", ""), result.get("level_id", "")], {})) as Dictionary
	var applied: Dictionary = PROGRESS_MODEL.apply_result(progress, result, world_catalog)
	if not bool(applied.get("ok", false)):
		return {
			"ok": false,
			"changed": false,
			"new_record": false,
			"message": str(applied.get("message", "result could not be applied")),
			"document": progress.duplicate(true),
		}
	if not bool(applied.get("changed", false)):
		return {"ok": true, "changed": false, "new_record": false,
			"document": progress.duplicate(true)}
	var candidate := applied.document as Dictionary
	var saved := _save_document(PROGRESS_FILE, candidate, &"progress", world_catalog)
	if not bool(saved.get("ok", false)):
		saved.changed = false
		saved.new_record = false
		saved.document = progress.duplicate(true)
		return saved
	return {
		"ok": true,
		"changed": true,
		"new_record": int(result.get("score", 0)) > int(previous.get("best_score", 0)),
		"document": candidate,
	}


func load_settings(default_bindings: Array, specs: Array) -> Dictionary:
	var defaults := SETTINGS_MODEL.default_document(default_bindings)
	return _load_document(SETTINGS_FILE, defaults, &"settings", specs)


func save_settings(document: Dictionary, specs: Array) -> Dictionary:
	return _save_document(SETTINGS_FILE, document, &"settings", specs)


func _load_document(
		file_name: String, defaults: Dictionary, kind: StringName, context: Variant) -> Dictionary:
	var root_error := _ensure_storage_root()
	if not root_error.is_empty():
		return _failure(&"path", root_error)
	var primary := _storage_root.path_join(file_name)
	var backup := primary + ".bak"
	var primary_result := _read_valid(primary, kind, context)
	if bool(primary_result.get("ok", false)):
		return {"ok": true, "source": &"primary", "recovered": false,
			"document": primary_result.document}
	var backup_result := _read_valid(backup, kind, context)
	if bool(backup_result.get("ok", false)):
		var diagnostics: Array[String] = []
		if FileAccess.file_exists(primary):
			var preserved := _preserve_corrupt(primary)
			if preserved.is_empty():
				return _failure(&"filesystem", "could not preserve corrupt primary")
			diagnostics.append(preserved)
		var restore_result := _install_without_rotation(primary, backup_result.document, kind, context)
		if not bool(restore_result.get("ok", false)):
			return restore_result
		return {"ok": true, "source": &"backup", "recovered": true,
			"document": backup_result.document, "diagnostic_paths": diagnostics}
	# A profile whose level set no longer matches the world catalog is migrated,
	# never discarded. Only what migration cannot restore — invalid JSON or a
	# broken internal invariant — reaches the corruption branch below.
	if kind == &"progress":
		for candidate_path: String in [primary, backup]:
			var migrated := _read_migrated(candidate_path, context)
			if not bool(migrated.get("ok", false)):
				continue
			var installed := _install_without_rotation(
				primary, migrated.document as Dictionary, kind, context)
			if not bool(installed.get("ok", false)):
				return installed
			return {
				"ok": true,
				"source": &"migration",
				"recovered": false,
				"migrated": true,
				"warning_message_id": MIGRATED_MESSAGE_ID,
				"document": (migrated.document as Dictionary).duplicate(true),
			}
	var primary_exists := FileAccess.file_exists(primary)
	var backup_exists := FileAccess.file_exists(backup)
	if not primary_exists and not backup_exists:
		var fresh_result := _save_document(file_name, defaults, kind, context)
		if not bool(fresh_result.get("ok", false)):
			return fresh_result
		return {"ok": true, "source": &"defaults", "recovered": false,
			"document": defaults.duplicate(true)}
	var diagnostic_paths: Array[String] = []
	for corrupt_path: String in [primary, backup]:
		if FileAccess.file_exists(corrupt_path):
			var preserved := _preserve_corrupt(corrupt_path)
			if preserved.is_empty():
				return _failure(&"filesystem", "could not preserve corrupt save")
			diagnostic_paths.append(preserved)
	var defaults_result := _save_document(file_name, defaults, kind, context)
	if not bool(defaults_result.get("ok", false)):
		return defaults_result
	return {
		"ok": true,
		"source": &"defaults",
		"recovered": true,
		"warning_message_id": TOTAL_RECOVERY_MESSAGE_ID,
		"diagnostic_paths": diagnostic_paths,
		"document": defaults.duplicate(true),
	}


func _save_document(
		file_name: String, document: Dictionary, kind: StringName, context: Variant) -> Dictionary:
	var validation_error := _validate(kind, document, context)
	if not validation_error.is_empty():
		return _failure(&"schema", validation_error)
	var normalized := _normalize(kind, document)
	var root_error := _ensure_storage_root()
	if not root_error.is_empty():
		return _failure(&"path", root_error)
	var primary := _storage_root.path_join(file_name)
	var backup := primary + ".bak"
	var temporary := primary + ".tmp"
	var write_error := _write_canonical(temporary, normalized)
	if not write_error.is_empty():
		return _failure(&"filesystem", write_error)
	var temp_result := _read_valid(temporary, kind, context)
	if not bool(temp_result.get("ok", false)):
		return _failure(&"verification", "temporary document failed round-trip validation")
	if not _continue_after(&"tmp_flushed"):
		return _failure(&"interrupted", "interrupted after temporary flush")
	if FileAccess.file_exists(primary):
		var primary_result := _read_valid(primary, kind, context)
		if bool(primary_result.get("ok", false)):
			if FileAccess.file_exists(backup) and not _remove(backup):
				return _failure(&"filesystem", "could not replace previous backup")
			if not _rename(primary, backup):
				return _failure(&"filesystem", "could not back up primary")
		else:
			var preserved := _preserve_corrupt(primary)
			if preserved.is_empty():
				return _failure(&"filesystem", "could not preserve corrupt primary")
	if not _continue_after(&"primary_backed_up"):
		return _failure(&"interrupted", "interrupted after primary backup")
	if not _rename(temporary, primary):
		return _failure(&"filesystem", "could not promote temporary document")
	return {"ok": true}


func _install_without_rotation(
		primary: String, document: Dictionary, kind: StringName, context: Variant) -> Dictionary:
	var temporary := primary + ".tmp"
	var write_error := _write_canonical(temporary, _normalize(kind, document))
	if not write_error.is_empty():
		return _failure(&"filesystem", write_error)
	var verification := _read_valid(temporary, kind, context)
	if not bool(verification.get("ok", false)):
		return _failure(&"verification", "recovery copy failed validation")
	if FileAccess.file_exists(primary) and not _remove(primary):
		return _failure(&"filesystem", "could not clear recovery target")
	if not _rename(temporary, primary):
		return _failure(&"filesystem", "could not install recovery copy")
	return {"ok": true}


func _read_valid(path: String, kind: StringName, context: Variant) -> Dictionary:
	if not FileAccess.file_exists(path):
		return _failure(&"missing", "file does not exist")
	var text := FileAccess.get_file_as_string(path)
	var parser := JSON.new()
	if parser.parse(text) != OK or not parser.data is Dictionary:
		return _failure(&"parse", "file is not valid JSON object")
	var parsed: Variant = parser.data
	var validation_error := _validate(kind, parsed, context)
	if not validation_error.is_empty():
		return _failure(&"schema", validation_error)
	return {"ok": true, "document": _normalize(kind, parsed as Dictionary)}


## Reads a progress file that failed validation and tries to bring it forward to
## the current world catalog. It never repairs a record: it only rebuilds the
## catalog-derived shape and keeps everything the player earned.
func _read_migrated(path: String, context: Variant) -> Dictionary:
	if not FileAccess.file_exists(path):
		return _failure(&"missing", "file does not exist")
	var parser := JSON.new()
	if parser.parse(FileAccess.get_file_as_string(path)) != OK \
			or not parser.data is Dictionary:
		return _failure(&"parse", "file is not valid JSON object")
	var migrated: Dictionary = PROGRESS_MODEL.migrate_document(
		parser.data, context as Dictionary)
	if not bool(migrated.get("ok", false)):
		return _failure(&"schema", str(migrated.get("message", "migration failed")))
	return {"ok": true, "document": _normalize(&"progress", migrated.document as Dictionary)}


func _validate(kind: StringName, document: Variant, context: Variant) -> String:
	if kind == &"progress":
		return PROGRESS_MODEL.validate_document(document, context as Dictionary)
	return SETTINGS_MODEL.validate_document(document, context as Array)


func _normalize(kind: StringName, document: Dictionary) -> Dictionary:
	var normalized := document.duplicate(true)
	normalized.schema_version = int(normalized.get("schema_version", 0))
	if kind == &"progress":
		for record_value: Variant in (normalized.get("levels", {}) as Dictionary).values():
			if record_value is Dictionary:
				var record := record_value as Dictionary
				for field: String in ["best_score", "best_stars", "best_birds_used", "completion_count"]:
					record[field] = int(record.get(field, 0))
	else:
		normalized.ui_scale_percent = int(normalized.get("ui_scale_percent", 0))
		normalized.camera_sensitivity = float(normalized.get("camera_sensitivity", 0.0))
		for bus: String in ["master", "music", "ambience", "sfx", "ui"]:
			if normalized.get("volumes", {}) is Dictionary:
				normalized.volumes[bus] = float(normalized.volumes.get(bus, 0.0))
	return normalized


func _write_canonical(path: String, document: Dictionary) -> String:
	if FileAccess.file_exists(path) and not _remove(path):
		return "could not clear stale temporary document"
	var file := FileAccess.open(path, FileAccess.WRITE)
	if file == null:
		return "could not open temporary document: %s" % error_string(FileAccess.get_open_error())
	file.store_string(JSON.stringify(document, "", true, true) + "\n")
	file.flush()
	var write_status := file.get_error()
	file = null
	if write_status != OK:
		return "could not flush temporary document: %s" % error_string(write_status)
	return ""


func _ensure_storage_root() -> String:
	if not _storage_root.begins_with("user://") or ".." in _storage_root \
			or "\\" in _storage_root:
		return "storage_root must be an isolated user:// path"
	var absolute := ProjectSettings.globalize_path(_storage_root)
	var result := DirAccess.make_dir_recursive_absolute(absolute)
	if result != OK:
		return "could not create storage root: %s" % error_string(result)
	return ""


func _continue_after(phase: StringName) -> bool:
	return not _phase_hook.is_valid() or bool(_phase_hook.call(phase))


func _preserve_corrupt(path: String) -> String:
	_diagnostic_counter += 1
	var suffix := "%d.%d.%d" % [
		Time.get_unix_time_from_system(), OS.get_process_id(), Time.get_ticks_usec()]
	var diagnostic := "%s.corrupt.%s.%d" % [path, suffix, _diagnostic_counter]
	while FileAccess.file_exists(diagnostic):
		_diagnostic_counter += 1
		diagnostic = "%s.corrupt.%s.%d" % [path, suffix, _diagnostic_counter]
	return diagnostic if _rename(path, diagnostic) else ""


func _rename(from_path: String, to_path: String) -> bool:
	return DirAccess.rename_absolute(
		ProjectSettings.globalize_path(from_path), ProjectSettings.globalize_path(to_path)) == OK


func _remove(path: String) -> bool:
	return DirAccess.remove_absolute(ProjectSettings.globalize_path(path)) == OK


func _failure(kind: StringName, message: String) -> Dictionary:
	return {"ok": false, "error_kind": kind, "message": message}
