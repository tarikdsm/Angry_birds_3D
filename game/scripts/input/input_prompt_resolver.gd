extends RefCounted

const INPUT_INTENT := preload("res://scripts/input/input_intent.gd")


func resolve(intent: RefCounted) -> String:
	if intent == null or not intent is INPUT_INTENT:
		return "input.prompt.unknown.generic"
	if intent.kind == INPUT_INTENT.KIND_ACCEPT and intent.source == &"keyboard":
		return "input.prompt.accept.keyboard"
	if intent.kind == INPUT_INTENT.KIND_ACCEPT and intent.source == &"gamepad":
		return "input.prompt.accept.gamepad"
	return "input.prompt.%s.generic" % intent.kind
