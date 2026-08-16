extends RefCounted


static func propose_rebind(bindings: Array, action: StringName, token: String) -> Dictionary:
	var target_index := _find_action(bindings, action)
	if target_index < 0 or not _valid_token(token):
		return {"ok": false, "message": "unknown action or invalid token"}
	var target := bindings[target_index] as Dictionary
	var conflict_index := -1
	for index: int in bindings.size():
		var candidate := bindings[index] as Dictionary
		if index != target_index and str(candidate.get("context", "")) == str(target.context) \
				and (candidate.get("tokens", []) as Array).has(token):
			conflict_index = index
			break
	return {
		"ok": true,
		"base_fingerprint": JSON.stringify(bindings, "", true, true),
		"action": action,
		"token": token,
		"target_index": target_index,
		"previous_tokens": (target.tokens as Array).duplicate(),
		"conflict": conflict_index >= 0,
		"conflict_index": conflict_index,
		"conflicting_action": StringName(str(
			(bindings[conflict_index] as Dictionary).get("action", ""))) if conflict_index >= 0 else &"",
	}


static func resolve_rebind(bindings: Array, proposal: Dictionary, confirm: bool) -> Dictionary:
	if not bool(proposal.get("ok", false)):
		return {"ok": false, "bindings": bindings.duplicate(true)}
	if str(proposal.get("base_fingerprint", "")) != JSON.stringify(bindings, "", true, true):
		return {"ok": false, "stale": true, "bindings": bindings.duplicate(true)}
	if bool(proposal.get("conflict", false)) and not confirm:
		return {"ok": false, "cancelled": true, "bindings": bindings.duplicate(true)}
	var updated := bindings.duplicate(true)
	var target_index := int(proposal.get("target_index", -1))
	if target_index < 0 or target_index >= updated.size():
		return {"ok": false, "bindings": bindings.duplicate(true)}
	var target := updated[target_index] as Dictionary
	target.tokens = [str(proposal.get("token", ""))]
	var conflict_index := int(proposal.get("conflict_index", -1))
	if conflict_index >= 0:
		var conflict := updated[conflict_index] as Dictionary
		conflict.tokens = (proposal.get("previous_tokens", []) as Array).duplicate()
	return {"ok": true, "bindings": updated}


static func _find_action(bindings: Array, action: StringName) -> int:
	for index: int in bindings.size():
		if StringName(str((bindings[index] as Dictionary).get("action", ""))) == action:
			return index
	return -1


static func _valid_token(token: String) -> bool:
	if not token.begins_with("key:"):
		return false
	var code := token.trim_prefix("key:")
	return code.is_valid_int() and int(code) > 0 and token == "key:%d" % int(code)
