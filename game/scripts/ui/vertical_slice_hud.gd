extends CanvasLayer

@onready var phase_label: Label = %PhaseLabel
@onready var birds_label: Label = %BirdsLabel
@onready var integrity_label: Label = %IntegrityLabel
@onready var controls_label: Label = %ControlsLabel
@onready var result_label: Label = %ResultLabel
@onready var fault_label: Label = %FaultLabel

var _anchor_integrity := 100.0
var _last_tick := -1


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
	phase_label.text = "FASE  %s" % phase.to_upper().replace("_", " ")
	birds_label.text = "VIRELAS  %d" % int(frame.get("birds_remaining", 0))
	integrity_label.text = "INTEGRIDADE DA ÂNCORA  %03d%%" % roundi(_anchor_integrity)
	controls_label.text = _controls_for_phase(phase)
	var outcome := str(frame.get("outcome", "none"))
	if outcome == "victory":
		result_label.text = "ÓRBITA CONQUISTADA"
	elif outcome == "defeat":
		result_label.text = "ÓRBITA PERDIDA"
	else:
		result_label.text = ""


func show_fault(code: String, message: String) -> void:
	fault_label.text = "FALHA [%s] %s" % [code, message]


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
