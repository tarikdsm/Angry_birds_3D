extends RefCounted

const KIND_NAVIGATE := &"navigate"
const KIND_ACCEPT := &"accept"
const KIND_BACK := &"back"
const KIND_ORBIT := &"orbit"
const KIND_ZOOM := &"zoom"
const KIND_BEGIN_GRAB := &"begin_grab"
const KIND_UPDATE_PULL := &"update_pull"
const KIND_RELEASE := &"release"
const KIND_ACTIVATE_ABILITY := &"activate_ability"
const KIND_RECENTER := &"recenter"
const KIND_PAUSE := &"pause"
const KIND_RESTART := &"restart"

var kind: StringName
var source: StringName
var payload: Dictionary


func _init(intent_kind: StringName, intent_source: StringName = &"unknown", intent_payload: Dictionary = {}) -> void:
	if not is_valid_kind(intent_kind):
		push_error("InputIntent received an unknown kind: %s" % intent_kind)
	kind = intent_kind
	source = intent_source
	payload = intent_payload.duplicate(true)


static func all_kinds() -> Array[StringName]:
	return [
		KIND_NAVIGATE, KIND_ACCEPT, KIND_BACK, KIND_ORBIT, KIND_ZOOM,
		KIND_BEGIN_GRAB, KIND_UPDATE_PULL, KIND_RELEASE,
		KIND_ACTIVATE_ABILITY, KIND_RECENTER, KIND_PAUSE, KIND_RESTART,
	]


static func is_valid_kind(value: StringName) -> bool:
	return value in all_kinds()
