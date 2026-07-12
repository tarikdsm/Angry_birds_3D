extends CanvasLayer

@onready var phase_label: Label = %PhaseLabel
@onready var birds_label: Label = %BirdsLabel
@onready var integrity_label: Label = %IntegrityLabel
@onready var controls_label: Label = %ControlsLabel
@onready var result_label: Label = %ResultLabel
@onready var fault_label: Label = %FaultLabel

var _anchor_integrity := 100.0
var _last_tick := -1
var _last_phase := "loading"
var _reduced_motion := false

const SPACE_COLOR := Color("07111f")
const PRIMARY_TEXT_COLOR := Color("bef9e8")
const SECONDARY_TEXT_COLOR := Color("f6c95c")


func apply_frame(frame: Dictionary) -> void:
	var tick := int(frame.get("tick", 0))
	if tick < _last_tick:
		_anchor_integrity = 100.0
	_last_tick = tick
	for event: Dictionary in frame.get("events", []):
		var kind := str(event.get("kind", ""))
		if kind == "damage_applied" \
				and int(event.get("affected_entity_id", 0)) == 200:
			_anchor_integrity = maxf(0.0, _anchor_integrity - float(event.get("damage", 0.0)))
		elif kind == "entity_neutralized" \
				and int(event.get("affected_entity_id", 0)) == 200:
			_anchor_integrity = 0.0
	var phase := str(frame.get("phase", "loading"))
	_last_phase = phase
	phase_label.text = "FASE  %s" % phase.to_upper().replace("_", " ")
	birds_label.text = "VIRELAS  %d" % int(frame.get("birds_remaining", 0))
	integrity_label.text = "INTEGRIDADE DA ÂNCORA  %03d%%" % roundi(_anchor_integrity)
	controls_label.text = _controls_with_accessibility(phase)
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
	controls_label.text = _controls_with_accessibility(_last_phase)


func minimum_contrast_ratio() -> float:
	return minf(
		_contrast_ratio(PRIMARY_TEXT_COLOR, SPACE_COLOR),
		_contrast_ratio(SECONDARY_TEXT_COLOR, SPACE_COLOR))


func _controls_for_phase(phase: String) -> String:
	match phase:
		"inspection":
			return "CLIQUE  MIRAR    RMB  ÓRBITA    F  RECENTRAR"
		"aim":
			return "Q/E  ARCO    A/D  POTÊNCIA    ESPAÇO  LANÇAR    ESC  CANCELAR"
		"flight_ability":
			return "ESPAÇO  ATIVAR VIRELA    F  RECENTRAR"
		"result":
			return "SEGURE R 0,5 s  REINICIAR"
		_:
			return "ESC  PAUSA    SEGURE R 0,5 s  REINICIAR"


func _controls_with_accessibility(phase: String) -> String:
	return "%s    M  MOVIMENTO %s" % [
		_controls_for_phase(phase),
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
