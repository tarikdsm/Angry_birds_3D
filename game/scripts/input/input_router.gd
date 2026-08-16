extends Node

const INPUT_INTENT := preload("res://scripts/input/input_intent.gd")

signal intent_submitted(intent: RefCounted)

const CONTEXT_FRONTEND := &"frontend"
const CONTEXT_GAMEPLAY := &"gameplay"

var _context: StringName = CONTEXT_FRONTEND


func _input(event: InputEvent) -> void:
	if route_raw_event(event):
		get_viewport().set_input_as_handled()


func set_context(context: StringName) -> void:
	if context not in [CONTEXT_FRONTEND, CONTEXT_GAMEPLAY]:
		push_error("InputRouter received an unsupported input context: %s" % context)
		return
	_context = context


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
		submit_semantic(INPUT_INTENT.KIND_ORBIT, {"delta": event.relative}, &"mouse")
		handled = true
	if event.button_mask & MOUSE_BUTTON_MASK_LEFT:
		submit_semantic(INPUT_INTENT.KIND_UPDATE_PULL, {
			"position": event.position,
			"delta": event.relative,
		}, &"mouse")
		handled = true
	return handled
