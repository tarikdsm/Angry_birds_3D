extends "res://scripts/app/scrollable_frontend_screen.gd"
signal back_requested
const PRODUCT_TEXT_CATALOG := preload("res://scripts/data/product_text_catalog.gd")

func _ready() -> void:
	super()
	var result := PRODUCT_TEXT_CATALOG.load_catalog("res://data/ui/product_v2.pt-BR.json")
	if bool(result.get("ok", false)):
		var messages := (result.document as Dictionary).messages as Dictionary
		for control: Node in find_children("*", "Control", true, false):
			var id := str(control.get_meta("message_id", ""))
			if not id.is_empty() and messages.has(id):
				(control as Control).text = str(messages[id])
	(find_child("BackButton", true, false) as Button).pressed.connect(func() -> void: back_requested.emit())
