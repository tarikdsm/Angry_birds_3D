extends Node

const PRODUCT_TEXT_CATALOG := preload("res://scripts/data/product_text_catalog.gd")
const CATALOG_PATH := "res://data/ui/product_v2.pt-BR.json"

@onready var _input_router: Node = $InputRouter
@onready var _screen_router: Node = $ScreenRouter
var _exit_handler := Callable()
var exit_request_count := 0


func _ready() -> void:
	var result := PRODUCT_TEXT_CATALOG.load_catalog(CATALOG_PATH)
	if not bool(result.get("ok", false)):
		push_error("AppShell failed closed while loading product text: %s" % result.get("message", ""))
		return
	_screen_router.set_messages((result.get("document", {}) as Dictionary).get("messages", {}))
	_input_router.intent_submitted.connect(_screen_router.handle_intent)
	_screen_router.exit_requested.connect(request_exit)
	_screen_router.show_main_menu()


func set_exit_handler(handler: Callable) -> void:
	_exit_handler = handler


func request_exit() -> void:
	exit_request_count += 1
	if _exit_handler.is_valid():
		_exit_handler.call()
		return
	get_tree().quit()
