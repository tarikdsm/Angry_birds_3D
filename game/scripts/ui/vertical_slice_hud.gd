extends CanvasLayer

const HUD_TEXT_CATALOG := preload("res://scripts/data/hud_text_catalog.gd")
const DEFAULT_TEXT_CATALOG_PATH := "res://data/ui/vertical_slice.pt-BR.json"
const PHASE_MESSAGE_IDS := {
	"loading": "hud.phase.loading",
	"inspection": "hud.phase.inspection",
	"aim": "hud.phase.aim",
	"flight_ability": "hud.phase.flight_ability",
	"resolution": "hud.phase.resolution",
	"evaluation": "hud.phase.evaluation",
	"result": "hud.phase.result",
	"faulted": "hud.phase.faulted",
}
const CONTROL_MESSAGE_IDS := {
	"loading": "hud.controls.loading",
	"inspection": "hud.controls.inspection",
	"aim": "hud.controls.aim",
	"resolution": "hud.controls.resolution",
	"evaluation": "hud.controls.evaluation",
	"result": "hud.controls.result",
	"faulted": "hud.controls.faulted",
}
const OUTCOME_MESSAGE_IDS := {
	"none": "hud.outcome.none",
	"victory": "hud.outcome.victory",
	"defeat": "hud.outcome.defeat",
}

@export_file("*.json") var text_catalog_path := DEFAULT_TEXT_CATALOG_PATH
@onready var phase_label: Label = %PhaseLabel
@onready var birds_label: Label = %BirdsLabel
@onready var integrity_label: Label = %IntegrityLabel
@onready var controls_label: Label = %ControlsLabel
@onready var result_label: Label = %ResultLabel
@onready var fault_label: Label = %FaultLabel

var _last_phase := "loading"
var _last_ability_readiness := "unavailable"
var _last_ability_armed := false
var _ability_rejected_early := false
var _reduced_motion := false
var _label_write_count := 0
var _text_catalog_load_count := 0
var _text_catalog_status := {"ok": false, "error_kind": "not_loaded"}
var _messages: Dictionary = HUD_TEXT_CATALOG.safe_fallback_messages()

const SPACE_COLOR := Color("07111f")
const PRIMARY_TEXT_COLOR := Color("bef9e8")
const SECONDARY_TEXT_COLOR := Color("f6c95c")

func _ready() -> void:
	_text_catalog_load_count += 1
	_text_catalog_status = HUD_TEXT_CATALOG.load_catalog(text_catalog_path)
	if bool(_text_catalog_status.get("ok", false)):
		var document := _text_catalog_status.get("document", {}) as Dictionary
		_messages = (document.get("messages", {}) as Dictionary).duplicate(true)
	_render_initial_state()


func apply_frame(frame: Dictionary) -> void:
	var phase := str(frame.get("phase", "loading"))
	var readiness := str(frame.get("ability_readiness", "unavailable"))
	var ability_armed := bool(frame.get("ability_armed", false))
	if phase != "flight_ability" or readiness != "arming":
		_ability_rejected_early = false
	for event: Dictionary in frame.get("events", []):
		if readiness == "arming" \
				and str(event.get("kind", "")) == "command_rejected" \
				and str(event.get("rejection_reason_name", "")) == "not_armed":
			_ability_rejected_early = true
	_last_phase = phase
	_last_ability_readiness = readiness
	_last_ability_armed = ability_armed
	_set_label_text(phase_label, _message("hud.format.phase") % _phase_label_pt_br(phase))
	_set_label_text(
		birds_label, _message("hud.format.birds") % int(frame.get("birds_remaining", 0)))
	_set_label_text(
		integrity_label, _objective_integrity_text(frame.get("objective_targets", [])))
	_set_label_text(
		controls_label, _controls_with_accessibility(phase, readiness, ability_armed))
	var outcome := str(frame.get("outcome", "none"))
	var outcome_message_id := str(OUTCOME_MESSAGE_IDS.get(outcome, "hud.outcome.unknown"))
	_set_label_text(result_label, _message(outcome_message_id))


func show_fault(code: String, message: String) -> void:
	_set_label_text(fault_label, _message("hud.format.fault") % [code, message])


func set_reduced_motion(enabled: bool) -> void:
	_reduced_motion = enabled
	_set_label_text(
		controls_label,
		_controls_with_accessibility(
			_last_phase, _last_ability_readiness, _last_ability_armed))


func label_write_count() -> int:
	return _label_write_count


func text_catalog_load_count() -> int:
	return _text_catalog_load_count


func text_catalog_status() -> Dictionary:
	return _text_catalog_status.duplicate(true)


func _set_label_text(label: Label, value: String) -> void:
	if label.text == value:
		return
	label.text = value
	_label_write_count += 1


func minimum_contrast_ratio() -> float:
	return minf(
		_contrast_ratio(PRIMARY_TEXT_COLOR, SPACE_COLOR),
		_contrast_ratio(SECONDARY_TEXT_COLOR, SPACE_COLOR))


func _objective_integrity_text(targets_value: Variant) -> String:
	if targets_value == null or not targets_value is Array or targets_value.is_empty():
		return _message("hud.format.objective_empty")
	var target_value: Variant = targets_value.front()
	if target_value == null or not target_value is Dictionary:
		return _message("hud.format.objective_empty")
	var target := target_value as Dictionary
	var maximum := float(target.get("maximum_integrity", 0.0))
	if maximum <= 0.0:
		return _message("hud.format.objective_empty")
	var current := clampf(float(target.get("current_integrity", 0.0)), 0.0, maximum)
	return _message("hud.format.objective_percent") % roundi(100.0 * current / maximum)


func _phase_label_pt_br(phase_id: String) -> String:
	var message_id := str(PHASE_MESSAGE_IDS.get(phase_id, "hud.phase.unknown"))
	return _message(message_id)


func _controls_for_phase(phase: String, readiness: String, ability_armed: bool) -> String:
	match phase:
		"flight_ability":
			if _ability_rejected_early:
				return _message("hud.controls.flight_ability.rejected_not_armed")
			if ability_armed and readiness == "armed":
				return _message("hud.controls.flight_ability.armed")
			if readiness == "arming":
				return _message("hud.controls.flight_ability.arming")
			if readiness == "active":
				return _message("hud.controls.flight_ability.active")
			if readiness == "spent":
				return _message("hud.controls.flight_ability.spent")
			if readiness == "unavailable":
				return _message("hud.controls.flight_ability.unavailable")
			return _message("hud.controls.flight_ability.unknown")
		_:
			var message_id := str(CONTROL_MESSAGE_IDS.get(phase, "hud.controls.unknown"))
			return _message(message_id)


func _controls_with_accessibility(
	phase: String, readiness: String, ability_armed: bool) -> String:
	return _message("hud.accessibility.format") % [
		_controls_for_phase(phase, readiness, ability_armed),
		_message("hud.accessibility.reduced") if _reduced_motion \
			else _message("hud.accessibility.full"),
	]


func _render_initial_state() -> void:
	_set_label_text(
		phase_label, _message("hud.format.phase") % _message("hud.phase.loading"))
	_set_label_text(birds_label, _message("hud.format.birds_empty"))
	_set_label_text(integrity_label, _message("hud.format.objective_empty"))
	_set_label_text(
		controls_label, _controls_with_accessibility("loading", "unavailable", false))
	_set_label_text(result_label, _message("hud.outcome.none"))
	_set_label_text(fault_label, "")


func _message(message_id: String) -> String:
	return str(_messages.get(message_id, "[%s]" % message_id))


func _contrast_ratio(foreground: Color, background: Color) -> float:
	var lighter := maxf(_relative_luminance(foreground), _relative_luminance(background))
	var darker := minf(_relative_luminance(foreground), _relative_luminance(background))
	return (lighter + 0.05) / (darker + 0.05)


func _relative_luminance(color: Color) -> float:
	return 0.2126 * _linear_channel(color.r) \
		+ 0.7152 * _linear_channel(color.g) \
		+ 0.0722 * _linear_channel(color.b)


func _linear_channel(value: float) -> float:
	return value / 12.92 if value <= 0.04045 else pow((value + 0.055) / 1.055, 2.4)
