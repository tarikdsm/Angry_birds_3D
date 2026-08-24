extends "res://scripts/app/scrollable_frontend_screen.gd"
signal accepted
signal back_requested

func _ready() -> void:
	super()
	(find_child("ContinueButton", true, false) as Button).pressed.connect(func() -> void: accepted.emit())
	(find_child("BackButton", true, false) as Button).pressed.connect(func() -> void: back_requested.emit())

func configure(lines: Array[String]) -> void:
	(find_child("BriefingText", true, false) as Label).text = "\n".join(lines.slice(0, 3))

func show_save_retry(value: bool) -> void:
	(find_child("RetryNotice", true, false) as Label).visible = value
