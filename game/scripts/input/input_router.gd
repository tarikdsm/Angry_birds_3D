extends Node

const INPUT_INTENT := preload("res://scripts/input/input_intent.gd")
const SETTINGS_MODEL := preload("res://scripts/save/settings_model.gd")

signal intent_submitted(intent: RefCounted)
signal binding_token_captured(action: StringName, token: String)

const CONTEXT_FRONTEND := &"frontend"
const CONTEXT_GAMEPLAY := &"gameplay"
const BINDING_SPECS := [
	{"action": "semantic_navigate_up", "context": "frontend", "intent": "navigate", "variant": "up"},
	{"action": "semantic_navigate_down", "context": "frontend", "intent": "navigate", "variant": "down"},
	{"action": "semantic_navigate_left", "context": "frontend", "intent": "navigate", "variant": "left"},
	{"action": "semantic_navigate_right", "context": "frontend", "intent": "navigate", "variant": "right"},
	{"action": "semantic_accept", "context": "frontend", "intent": "accept", "variant": ""},
	{"action": "semantic_back", "context": "frontend", "intent": "back", "variant": ""},
	{"action": "semantic_activate_ability", "context": "gameplay", "intent": "activate_ability", "variant": ""},
	{"action": "semantic_recenter", "context": "gameplay", "intent": "recenter", "variant": ""},
	{"action": "semantic_pause", "context": "gameplay", "intent": "pause", "variant": ""},
	{"action": "semantic_restart", "context": "gameplay", "intent": "restart", "variant": ""},
	{"action": "semantic_zoom_in", "context": "gameplay", "intent": "zoom", "variant": "positive"},
	{"action": "semantic_zoom_out", "context": "gameplay", "intent": "zoom", "variant": "negative"},
]

var _context: StringName = CONTEXT_FRONTEND
var _camera_sensitivity := 1.0
var _project_defaults: Array = []
var _binding_capture_action: StringName = &""


func _init() -> void:
	_project_defaults = _capture_official_bindings()


func _input(event: InputEvent) -> void:
	if route_raw_event(event):
		get_viewport().set_input_as_handled()


func set_context(context: StringName) -> void:
	if context not in [CONTEXT_FRONTEND, CONTEXT_GAMEPLAY]:
		push_error("InputRouter received an unsupported input context: %s" % context)
		return
	if context != CONTEXT_FRONTEND:
		# A rebind is only ever captured on the options screen. Leaving the
		# frontend disarms it, so no gameplay key can be swallowed and persisted
		# as a binding the player never asked for.
		_binding_capture_action = &""
	_context = context


func official_binding_specs() -> Array:
	return BINDING_SPECS.duplicate(true)


func project_default_bindings() -> Array:
	return _project_defaults.duplicate(true)


func capture_bindings() -> Array:
	return _capture_official_bindings()


func apply_bindings(bindings: Array) -> Dictionary:
	var validation_error := SETTINGS_MODEL.validate_bindings(bindings, BINDING_SPECS)
	if not validation_error.is_empty():
		return {"ok": false, "message": validation_error}
	var snapshot := capture_bindings()
	_apply_binding_events(bindings)
	if capture_bindings() != bindings:
		_apply_binding_events(snapshot)
		return {"ok": false, "message": "InputMap verification failed and was rolled back"}
	return {"ok": true}


func set_camera_sensitivity(value: float) -> bool:
	if not is_finite(value) or value < 0.25 or value > 2.0 \
			or not is_equal_approx(value * 20.0, round(value * 20.0)):
		return false
	_camera_sensitivity = value
	return true


func begin_binding_capture(action: StringName) -> bool:
	for spec: Dictionary in BINDING_SPECS:
		if StringName(spec.action) == action:
			_binding_capture_action = action
			return true
	return false


func cancel_binding_capture() -> void:
	_binding_capture_action = &""


func supported_intent_kinds(context: StringName, source: StringName) -> Array[StringName]:
	if context == CONTEXT_FRONTEND and source == &"keyboard":
		return [
			INPUT_INTENT.KIND_NAVIGATE,
			INPUT_INTENT.KIND_ACCEPT,
			INPUT_INTENT.KIND_BACK,
		]
	if context == CONTEXT_FRONTEND and source == &"mouse":
		return [INPUT_INTENT.KIND_NAVIGATE, INPUT_INTENT.KIND_ACCEPT]
	if context == CONTEXT_GAMEPLAY and source == &"keyboard":
		return [
			INPUT_INTENT.KIND_ACTIVATE_ABILITY,
			INPUT_INTENT.KIND_RECENTER,
			INPUT_INTENT.KIND_PAUSE,
			INPUT_INTENT.KIND_RESTART,
			INPUT_INTENT.KIND_ZOOM,
			INPUT_INTENT.KIND_BACK,
		]
	if context == CONTEXT_GAMEPLAY and source == &"mouse":
		return [
			INPUT_INTENT.KIND_ORBIT,
			INPUT_INTENT.KIND_ZOOM,
			INPUT_INTENT.KIND_BEGIN_GRAB,
			INPUT_INTENT.KIND_UPDATE_PULL,
			INPUT_INTENT.KIND_RELEASE,
		]
	return []


func submit_intent(intent: RefCounted) -> void:
	if intent == null or not intent is INPUT_INTENT:
		push_error("InputRouter accepts only InputIntent values")
		return
	intent_submitted.emit(intent)


func submit_semantic(kind: StringName, payload: Dictionary = {}, source: StringName = &"external") -> void:
	submit_intent(INPUT_INTENT.new(kind, source, payload))


func route_raw_event(event: InputEvent) -> bool:
	if event is InputEventKey:
		var key_event := event as InputEventKey
		var capturing := _context == CONTEXT_FRONTEND \
			and not _binding_capture_action.is_empty()
		if capturing and key_event.pressed and not key_event.echo:
			var keycode := key_event.physical_keycode
			if keycode == 0:
				keycode = key_event.keycode
			if keycode <= 0:
				return false
			var action := _binding_capture_action
			_binding_capture_action = &""
			binding_token_captured.emit(action, "key:%d" % keycode)
			return true
		return _route_key(event as InputEventKey)
	elif event is InputEventMouseButton:
		return _route_mouse_button(event as InputEventMouseButton)
	elif event is InputEventMouseMotion:
		return _route_mouse_motion(event as InputEventMouseMotion)
	return false


func _route_key(event: InputEventKey) -> bool:
	if not event.pressed or event.echo:
		return false
	if _context == CONTEXT_FRONTEND:
		return _route_frontend_key(event)
	return _route_gameplay_key(event)


func _route_frontend_key(event: InputEventKey) -> bool:
	if event.is_action_pressed(&"semantic_navigate_up"):
		submit_semantic(INPUT_INTENT.KIND_NAVIGATE, {"direction": Vector2.UP}, &"keyboard")
	elif event.is_action_pressed(&"semantic_navigate_down"):
		submit_semantic(INPUT_INTENT.KIND_NAVIGATE, {"direction": Vector2.DOWN}, &"keyboard")
	elif event.is_action_pressed(&"semantic_navigate_left"):
		submit_semantic(INPUT_INTENT.KIND_NAVIGATE, {"direction": Vector2.LEFT}, &"keyboard")
	elif event.is_action_pressed(&"semantic_navigate_right"):
		submit_semantic(INPUT_INTENT.KIND_NAVIGATE, {"direction": Vector2.RIGHT}, &"keyboard")
	elif event.is_action_pressed(&"semantic_accept"):
		submit_semantic(INPUT_INTENT.KIND_ACCEPT, {}, &"keyboard")
	elif event.is_action_pressed(&"semantic_back"):
		submit_semantic(INPUT_INTENT.KIND_BACK, {}, &"keyboard")
	else:
		return false
	return true


func _route_gameplay_key(event: InputEventKey) -> bool:
	if event.is_action_pressed(&"semantic_activate_ability"):
		submit_semantic(INPUT_INTENT.KIND_ACTIVATE_ABILITY, {}, &"keyboard")
	elif event.is_action_pressed(&"semantic_recenter"):
		submit_semantic(INPUT_INTENT.KIND_RECENTER, {}, &"keyboard")
	elif event.is_action_pressed(&"semantic_pause"):
		submit_semantic(INPUT_INTENT.KIND_PAUSE, {}, &"keyboard")
	elif event.is_action_pressed(&"semantic_restart"):
		submit_semantic(INPUT_INTENT.KIND_RESTART, {}, &"keyboard")
	elif event.is_action_pressed(&"semantic_zoom_in"):
		submit_semantic(INPUT_INTENT.KIND_ZOOM, {"amount": 1.0}, &"keyboard")
	elif event.is_action_pressed(&"semantic_zoom_out"):
		submit_semantic(INPUT_INTENT.KIND_ZOOM, {"amount": -1.0}, &"keyboard")
	elif event.is_action_pressed(&"semantic_back"):
		# Esc cancels an open grab; the gameplay layer decides what an
		# unstarted grab means. The router never inspects gameplay state.
		submit_semantic(INPUT_INTENT.KIND_BACK, {}, &"keyboard")
	else:
		return false
	return true


func _route_mouse_button(event: InputEventMouseButton) -> bool:
	if _context == CONTEXT_FRONTEND:
		if event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
			submit_semantic(INPUT_INTENT.KIND_ACCEPT, {"position": event.position}, &"mouse")
			return true
		if event.button_index == MOUSE_BUTTON_WHEEL_DOWN and event.pressed:
			submit_semantic(INPUT_INTENT.KIND_NAVIGATE, {"direction": Vector2.DOWN}, &"mouse")
			return true
		if event.button_index == MOUSE_BUTTON_WHEEL_UP and event.pressed:
			submit_semantic(INPUT_INTENT.KIND_NAVIGATE, {"direction": Vector2.UP}, &"mouse")
			return true
		return false
	if event.button_index == MOUSE_BUTTON_WHEEL_DOWN and event.pressed:
		submit_semantic(INPUT_INTENT.KIND_ZOOM, {"amount": -event.factor}, &"mouse")
		return true
	elif event.button_index == MOUSE_BUTTON_WHEEL_UP and event.pressed:
		submit_semantic(INPUT_INTENT.KIND_ZOOM, {"amount": event.factor}, &"mouse")
		return true
	elif event.button_index == MOUSE_BUTTON_LEFT:
		if event.pressed:
			submit_semantic(INPUT_INTENT.KIND_BEGIN_GRAB, {"position": event.position}, &"mouse")
		else:
			submit_semantic(INPUT_INTENT.KIND_RELEASE, {"position": event.position}, &"mouse")
		return true
	return false


func _route_mouse_motion(event: InputEventMouseMotion) -> bool:
	if _context != CONTEXT_GAMEPLAY:
		return false
	var handled := false
	if event.button_mask & MOUSE_BUTTON_MASK_RIGHT:
		submit_semantic(INPUT_INTENT.KIND_ORBIT, {
			"delta": event.relative * _camera_sensitivity,
		}, &"mouse")
		handled = true
	if event.button_mask & MOUSE_BUTTON_MASK_LEFT:
		submit_semantic(INPUT_INTENT.KIND_UPDATE_PULL, {
			"position": event.position,
			"delta": event.relative,
		}, &"mouse")
		handled = true
	return handled


func _capture_official_bindings() -> Array:
	var bindings: Array = []
	for spec: Dictionary in BINDING_SPECS:
		var tokens: Array[String] = []
		for event: InputEvent in InputMap.action_get_events(StringName(spec.action)):
			if event is InputEventKey:
				var key_event := event as InputEventKey
				var keycode := key_event.physical_keycode
				if keycode == 0:
					keycode = key_event.keycode
				var token := "key:%d" % keycode
				if keycode > 0 and token not in tokens:
					tokens.append(token)
		bindings.append({
			"action": str(spec.action),
			"context": str(spec.context),
			"intent": str(spec.intent),
			"variant": str(spec.variant),
			"tokens": tokens,
		})
	return bindings


func _apply_binding_events(bindings: Array) -> void:
	for binding: Dictionary in bindings:
		var action := StringName(str(binding.action))
		InputMap.action_erase_events(action)
		for token: String in binding.tokens:
			var event := InputEventKey.new()
			event.physical_keycode = int(token.trim_prefix("key:")) as Key
			InputMap.action_add_event(action, event)
