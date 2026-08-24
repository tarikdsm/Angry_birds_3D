extends SceneTree

const SCENE := "res://scenes/frontend/about_screen.tscn"
const MARKER := "ABOUT_SCREEN_SMOKE_OK"

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var packed := load(SCENE) as PackedScene
	if packed == null:
		_fail("the About route must be a loadable closed scene")
		return
	var screen := packed.instantiate() as Control
	root.add_child(screen)
	await process_frame
	var legal := screen.find_child("LegalNotice", true, false) as Label
	var back := screen.find_child("BackButton", true, false) as Button
	if legal == null or back == null or legal.text.is_empty():
		_fail("About must expose the same localized disclaimer and a return action")
		return
	if "fan game não oficial, sem afiliação ou endosso" not in legal.text.to_lower():
		_fail("About must never imply authorization or endorsement")
		return
	if not screen.has_signal("back_requested"):
		_fail("About must provide a semantic back route")
		return
	print(MARKER)
	quit(0)

func _fail(message: String) -> void:
	push_error("about screen smoke: %s" % message)
	quit(1)
