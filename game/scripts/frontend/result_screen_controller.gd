extends Control

## Result overlay.
##
## The screen only renders the terminal frame the gameplay controller published
## and the record decision the save layer already took. It never scores, never
## recomputes stars and never writes a save by itself.

const FOCUS_NAVIGATION := preload("res://scripts/ui/focus_navigation.gd")
const THEME_FACTORY := preload("res://scripts/ui/theme_factory.gd")

var _messages: Dictionary = {}
var _summary: Dictionary = {}


func _ready() -> void:
	theme = THEME_FACTORY.build()
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	FOCUS_NAVIGATION.wire(self)


func set_messages(messages: Dictionary) -> void:
	_messages = messages.duplicate(true)
	if not _summary.is_empty():
		configure(_summary)


## summary: {outcome, score, stars, birds_used, new_record, saved}
func configure(summary: Dictionary) -> void:
	_summary = summary.duplicate(true)
	var victory := str(_summary.get("outcome", "")) == "victory"
	var headline := _label("OutcomeLabel")
	headline.text = _message("app.result.victory" if victory else "app.result.defeat")
	headline.theme_type_variation = &"Victory" if victory else &"Defeat"
	_label("ScoreLabel").text = _format("app.result.score", int(_summary.get("score", 0)))
	_label("StarsLabel").text = _format("app.result.stars", int(_summary.get("stars", 0)))
	_label("BirdsLabel").text = _format(
		"app.result.birds_used", int(_summary.get("birds_used", 0)))
	var record := _label("RecordLabel")
	record.text = _message("app.result.new_record")
	record.visible = bool(_summary.get("new_record", false))
	_label("RetryNotice").visible = bool(_summary.get("save_failed", false))
	FOCUS_NAVIGATION.wire(self)


func summary() -> Dictionary:
	return _summary.duplicate(true)


func focus_ring_size() -> int:
	return FOCUS_NAVIGATION.wire(self)


func _label(node_name: String) -> Label:
	return find_child(node_name, true, false) as Label


func _message(message_id: String) -> String:
	return str(_messages.get(message_id, message_id))


## The catalog is closed and fail-closed, so a missing entry can only mean the
## screen was mounted outside the shell. It then renders the identifier instead
## of raising a formatting failure.
func _format(message_id: String, value: Variant) -> String:
	var pattern := _message(message_id)
	return (pattern % value) if pattern.contains("%") else pattern
