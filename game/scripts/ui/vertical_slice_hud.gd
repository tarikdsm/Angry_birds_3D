extends CanvasLayer

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

const SPACE_COLOR := Color("07111f")
const PRIMARY_TEXT_COLOR := Color("bef9e8")
const SECONDARY_TEXT_COLOR := Color("f6c95c")


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
	phase_label.text = "FASE  %s" % phase.to_upper().replace("_", " ")
	birds_label.text = "VIRELAS  %d" % int(frame.get("birds_remaining", 0))
	integrity_label.text = _objective_integrity_text(frame.get("objective_targets", []))
	controls_label.text = _controls_with_accessibility(phase, readiness, ability_armed)
	var outcome := str(frame.get("outcome", "none"))
	if outcome == "victory":
		result_label.text = "ÓRBITA CONQUISTADA"
	elif outcome == "defeat":
		result_label.text = "ÓRBITA PERDIDA"
	else:
		result_label.text = ""


func show_fault(code: String, message: String) -> void:
	fault_label.text = "FALHA [%s] %s" % [code, message]


func set_reduced_motion(enabled: bool) -> void:
	_reduced_motion = enabled
	controls_label.text = _controls_with_accessibility(
		_last_phase, _last_ability_readiness, _last_ability_armed)


func minimum_contrast_ratio() -> float:
	return minf(
		_contrast_ratio(PRIMARY_TEXT_COLOR, SPACE_COLOR),
		_contrast_ratio(SECONDARY_TEXT_COLOR, SPACE_COLOR))


func _objective_integrity_text(targets_value: Variant) -> String:
	if targets_value == null or not targets_value is Array or targets_value.is_empty():
		return "INTEGRIDADE DO ALVO  ---"
	var target_value: Variant = targets_value.front()
	if target_value == null or not target_value is Dictionary:
		return "INTEGRIDADE DO ALVO  ---"
	var target := target_value as Dictionary
	var maximum := float(target.get("maximum_integrity", 0.0))
	if maximum <= 0.0:
		return "INTEGRIDADE DO ALVO  ---"
	var current := clampf(float(target.get("current_integrity", 0.0)), 0.0, maximum)
	return "INTEGRIDADE DO ALVO  %03d%%" % roundi(100.0 * current / maximum)


func _controls_for_phase(phase: String, readiness: String, ability_armed: bool) -> String:
	match phase:
		"inspection":
			return "CLIQUE  MIRAR    RMB  ÓRBITA    F  RECENTRAR"
		"aim":
			return "Q/E  ARCO    A/D  POTÊNCIA    ESPAÇO  LANÇAR    ESC  CANCELAR"
		"flight_ability":
			if _ability_rejected_early:
				return "VIRELA AINDA NÃO ARMADA    AGUARDE    F  RECENTRAR"
			if ability_armed and readiness == "armed":
				return "ESPAÇO  ATIVAR VIRELA    F  RECENTRAR"
			if readiness == "arming":
				return "VIRELA CARREGANDO    F  RECENTRAR"
			if readiness == "active":
				return "VIRELA ATIVA    F  RECENTRAR"
			return "F  RECENTRAR"
		"result":
			return "SEGURE R 0,5 s  REINICIAR"
		_:
			return "ESC  PAUSA    SEGURE R 0,5 s  REINICIAR"


func _controls_with_accessibility(
	phase: String, readiness: String, ability_armed: bool) -> String:
	return "%s    M  MOVIMENTO %s" % [
		_controls_for_phase(phase, readiness, ability_armed),
		"REDUZIDO" if _reduced_motion else "COMPLETO",
	]


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
