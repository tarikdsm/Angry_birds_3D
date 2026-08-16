extends SceneTree

const INPUT_INTENT := preload("res://scripts/input/input_intent.gd")
const INPUT_PROMPT_RESOLVER := preload("res://scripts/input/input_prompt_resolver.gd")
const INPUT_ROUTER := preload("res://scripts/input/input_router.gd")
const PRODUCT_TEXT_CATALOG := preload("res://scripts/data/product_text_catalog.gd")
const PRODUCT_TEXT_PATH := "res://data/ui/product_v2.pt-BR.json"
const SUCCESS_MARKER := "INPUT_ROUTER_SMOKE_OK"


var _router: Node
var _received: Array = []


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	_router = INPUT_ROUTER.new()
	root.add_child(_router)
	_router.intent_submitted.connect(_received.append)
	if not _router.has_method("set_context"):
		_fail("InputRouter must expose an explicit frontend/gameplay context boundary")
		return

	var submitted := INPUT_INTENT.new(
		INPUT_INTENT.KIND_NAVIGATE, &"gamepad", {"direction": Vector2.DOWN})
	_router.submit_intent(submitted)
	if _received.size() != 1 or _received[0] != submitted:
		_fail("the semantic submission boundary must forward provider intents unchanged")
		return
	if _received[0].has_method("get_raw_event"):
		_fail("InputIntent must not retain a raw InputEvent")
		return

	_received.clear()
	_router.set_context(&"frontend")
	var keyboard := InputEventKey.new()
	keyboard.physical_keycode = KEY_DOWN
	keyboard.pressed = true
	_router.route_raw_event(keyboard)
	var wheel := InputEventMouseButton.new()
	wheel.button_index = MOUSE_BUTTON_WHEEL_DOWN
	wheel.pressed = true
	_router.route_raw_event(wheel)
	if _received.size() != 2:
		_fail("keyboard and mouse navigation must both produce semantic intents")
		return
	var navigation_intents: Array = _received.filter(func(intent: Variant) -> bool:
		return intent is INPUT_INTENT and intent.kind == INPUT_INTENT.KIND_NAVIGATE)
	if navigation_intents.size() != 2:
		_fail("keyboard and mouse must both emit a logical navigate intent")
		return
	for intent: Variant in navigation_intents:
		if intent.payload.get("direction") != Vector2.DOWN:
			_fail("keyboard and mouse must agree on the logical navigate intent")
			return

	_received.clear()
	_router.set_context(&"gameplay")
	var space := InputEventKey.new()
	space.physical_keycode = KEY_SPACE
	space.pressed = true
	_router.route_raw_event(space)
	if _received.size() != 1 or _received[0].kind != INPUT_INTENT.KIND_ACTIVATE_ABILITY:
		_fail("gameplay Space must emit only activate_ability, never frontend accept")
		return
	_received.clear()
	_router.set_context(&"frontend")
	_router.route_raw_event(space)
	if _received.size() != 1 or _received[0].kind != INPUT_INTENT.KIND_ACCEPT:
		_fail("frontend Space must emit only accept, never gameplay ability")
		return

	var resolver := INPUT_PROMPT_RESOLVER.new()
	var prompt_id := resolver.resolve(INPUT_INTENT.new(INPUT_INTENT.KIND_ACCEPT, &"keyboard"))
	if prompt_id != "input.prompt.accept.keyboard" or " " in prompt_id:
		_fail("prompt resolution must return a catalog id, never visible text")
		return
	if resolver.resolve(INPUT_INTENT.new(INPUT_INTENT.KIND_ACCEPT, &"gamepad")) \
				!= "input.prompt.accept.gamepad":
		_fail("future providers must resolve prompts through the same semantic interface")
		return
	var generic_accept_id := resolver.resolve(INPUT_INTENT.new(INPUT_INTENT.KIND_ACCEPT, &"external"))
	if generic_accept_id != "input.prompt.accept.generic":
		_fail("non-keyboard accept must resolve to the closed generic accept prompt")
		return
	var catalog_result := PRODUCT_TEXT_CATALOG.load_catalog(PRODUCT_TEXT_PATH)
	if not bool(catalog_result.get("ok", false)) \
				or not (catalog_result.get("document", {}) as Dictionary).get("messages", {}).has(generic_accept_id):
		_fail("closed product catalog must include the generic accept prompt")
		return

	var expected_capabilities := {
		"frontend.keyboard": [
			INPUT_INTENT.KIND_NAVIGATE, INPUT_INTENT.KIND_ACCEPT, INPUT_INTENT.KIND_BACK,
		],
		"frontend.mouse": [INPUT_INTENT.KIND_NAVIGATE, INPUT_INTENT.KIND_ACCEPT],
		"gameplay.keyboard": [
			INPUT_INTENT.KIND_ACTIVATE_ABILITY, INPUT_INTENT.KIND_RECENTER,
			INPUT_INTENT.KIND_PAUSE, INPUT_INTENT.KIND_RESTART, INPUT_INTENT.KIND_ZOOM,
		],
		"gameplay.mouse": [
			INPUT_INTENT.KIND_ORBIT, INPUT_INTENT.KIND_ZOOM,
			INPUT_INTENT.KIND_BEGIN_GRAB, INPUT_INTENT.KIND_UPDATE_PULL,
			INPUT_INTENT.KIND_RELEASE,
		],
	}
	for capability_key: String in expected_capabilities:
		var parts := capability_key.split(".")
		if _router.supported_intent_kinds(StringName(parts[0]), StringName(parts[1])) \
				!= expected_capabilities[capability_key]:
			_fail("InputRouter capability matrix drifted: %s" % capability_key)
			return
	_assert_raw_intent(&"frontend", _key_event(KEY_DOWN), INPUT_INTENT.KIND_NAVIGATE)
	_assert_raw_intent(&"frontend", _key_event(KEY_ENTER), INPUT_INTENT.KIND_ACCEPT)
	_assert_raw_intent(&"frontend", _key_event(KEY_ESCAPE), INPUT_INTENT.KIND_BACK)
	_assert_raw_intent(&"frontend", _wheel_event(MOUSE_BUTTON_WHEEL_DOWN), INPUT_INTENT.KIND_NAVIGATE)
	_assert_raw_intent(&"frontend", _left_button_event(true), INPUT_INTENT.KIND_ACCEPT)
	_assert_raw_intent(&"gameplay", _key_event(KEY_SPACE), INPUT_INTENT.KIND_ACTIVATE_ABILITY)
	_assert_raw_intent(&"gameplay", _key_event(KEY_F), INPUT_INTENT.KIND_RECENTER)
	_assert_raw_intent(&"gameplay", _key_event(KEY_P), INPUT_INTENT.KIND_PAUSE)
	_assert_raw_intent(&"gameplay", _key_event(KEY_R), INPUT_INTENT.KIND_RESTART)
	_assert_raw_intent(&"gameplay", _key_event(KEY_EQUAL), INPUT_INTENT.KIND_ZOOM)
	_assert_raw_intent(&"gameplay", _wheel_event(MOUSE_BUTTON_WHEEL_DOWN), INPUT_INTENT.KIND_ZOOM)
	_assert_raw_intent(&"gameplay", _left_button_event(true), INPUT_INTENT.KIND_BEGIN_GRAB)
	_assert_raw_intent(&"gameplay", _left_button_event(false), INPUT_INTENT.KIND_RELEASE)
	_assert_raw_intent(&"gameplay", _motion_event(MOUSE_BUTTON_MASK_RIGHT), INPUT_INTENT.KIND_ORBIT)
	_assert_raw_intent(&"gameplay", _motion_event(MOUSE_BUTTON_MASK_LEFT), INPUT_INTENT.KIND_UPDATE_PULL)

	var kinds: Array = INPUT_INTENT.all_kinds()
	if kinds.size() != 12:
		_fail("InputIntent must expose all twelve semantic intent kinds")
		return
	for kind: StringName in kinds:
		if not INPUT_INTENT.is_valid_kind(kind):
			_fail("InputIntent rejected its declared intent kind: %s" % kind)
			return

	print(SUCCESS_MARKER)
	_finish(0)


func _fail(message: String) -> void:
	push_error("input router smoke: %s" % message)
	_finish(1)


func _assert_raw_intent(context: StringName, event: InputEvent, expected_kind: StringName) -> void:
	_received.clear()
	_router.set_context(context)
	if not _router.route_raw_event(event) or _received.size() != 1 \
				or _received[0].kind != expected_kind:
		_fail("%s must reach %s exactly once" % [context, expected_kind])


func _key_event(keycode: Key) -> InputEventKey:
	var event := InputEventKey.new()
	event.physical_keycode = keycode
	event.pressed = true
	return event


func _wheel_event(button: MouseButton) -> InputEventMouseButton:
	var event := InputEventMouseButton.new()
	event.button_index = button
	event.pressed = true
	return event


func _left_button_event(pressed: bool) -> InputEventMouseButton:
	var event := InputEventMouseButton.new()
	event.button_index = MOUSE_BUTTON_LEFT
	event.pressed = pressed
	return event


func _motion_event(mask: int) -> InputEventMouseMotion:
	var event := InputEventMouseMotion.new()
	event.button_mask = mask
	event.relative = Vector2(2.0, 1.0)
	return event


func _finish(exit_code: int) -> void:
	if is_instance_valid(_router):
		root.remove_child(_router)
		_router.free()
	quit(exit_code)
