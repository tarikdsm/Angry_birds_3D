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
	return {"ok": true, "document": parser.data}
