extends RefCounted


static func read_text(path: String) -> Dictionary:
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		var open_error := FileAccess.get_open_error()
		return {
			"ok": false,
			"error_kind": "open",
			"message": "%s could not be opened (error %d)" % [path, open_error],
		}
	var content := file.get_as_text()
	var read_error := file.get_error()
	if read_error != OK:
		return {
			"ok": false,
			"error_kind": "read",
			"message": "%s could not be read (error %d)" % [path, read_error],
		}
	return {"ok": true, "text": content}


static func load_json(path: String) -> Dictionary:
	var read_result := read_text(path)
	if not bool(read_result.get("ok", false)):
		return read_result
	var parser := JSON.new()
	var parse_error := parser.parse(str(read_result.get("text", "")))
	if parse_error != OK:
		return {
			"ok": false,
			"error_kind": "json",
			"message": "%s contains malformed JSON at line %d: %s" % [
				path, parser.get_error_line(), parser.get_error_message()],
		}
	var duplicate := duplicate_key_error(str(read_result.get("text", "")))
	if not duplicate.is_empty():
		return {
			"ok": false,
			"error_kind": "json",
			"message": "%s has a %s" % [path, duplicate],
		}
	return {"ok": true, "document": parser.data}


## Godot's JSON parser collapses a repeated object key, keeping the last value,
## while the C++ reader rejects it as an explicit DuplicateKey. Product loading
## is strict, atomic and fail-closed, and the documents that never reach C++ --
## the world catalog, the asset and camera catalogs and the pt-BR text catalogs
## -- must get the same answer instead of silently taking the last value.
##
## The scanner reads keys as they are written in the file. Two keys that differ
## only by escaping are not treated as equal, which keeps the check strictly
## conservative: it never rejects a document the C++ reader would accept.
static func duplicate_key_error(text: String) -> String:
	var is_object: Array[bool] = []
	var keys_per_object: Array = []
	var awaiting_key: Array[bool] = []
	var length := text.length()
	var index := 0
	while index < length:
		var code := text.unicode_at(index)
		if code == 34:
			var start := index + 1
			index += 1
			var closed := false
			while index < length:
				var inner := text.unicode_at(index)
				if inner == 92:
					index += 2
					continue
				if inner == 34:
					closed = true
					break
				index += 1
			if not closed:
				return ""
			if not is_object.is_empty() and is_object[-1] and awaiting_key[-1]:
				var token := text.substr(start, index - start)
				var keys := keys_per_object[-1] as Dictionary
				if keys.has(token):
					return "duplicate JSON key: %s" % token
				keys[token] = true
				awaiting_key[-1] = false
			index += 1
			continue
		if code == 123 or code == 91:
			is_object.append(code == 123)
			keys_per_object.append({})
			awaiting_key.append(code == 123)
		elif code == 125 or code == 93:
			if is_object.is_empty():
				return ""
			is_object.resize(is_object.size() - 1)
			keys_per_object.resize(keys_per_object.size() - 1)
			awaiting_key.resize(awaiting_key.size() - 1)
		elif code == 44 and not is_object.is_empty() and is_object[-1]:
			awaiting_key[-1] = true
		index += 1
	return ""
