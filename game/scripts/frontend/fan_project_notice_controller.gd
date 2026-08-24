extends "res://scripts/app/scrollable_frontend_screen.gd"
signal accepted
const PRODUCT_TEXT_CATALOG := preload("res://scripts/data/product_text_catalog.gd")

func _ready() -> void:
	super()
	_apply_catalog()
	(find_child("AcceptButton", true, false) as Button).pressed.connect(func() -> void: accepted.emit())

func _apply_catalog() -> void:
	var result := PRODUCT_TEXT_CATALOG.load_catalog("res://data/ui/product_v2.pt-BR.json")
	if bool(result.get("ok", false)):
		var messages := (result.document as Dictionary).messages as Dictionary
		for control: Node in find_children("*", "Control", true, false):
			var id := str(control.get_meta("message_id", ""))
			if not id.is_empty() and messages.has(id):
				(control as Control).text = str(messages[id])

func show_save_retry(value: bool) -> void:
	(find_child("RetryNotice", true, false) as Label).visible = value
