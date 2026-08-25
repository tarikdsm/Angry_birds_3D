extends Control

## Pause overlay.
##
## Pause is presentation only: the overlay never touches the kernel. The
## gameplay controller is the single place that stops advancing the session, and
## the specification keeps Pause out of the authoritative FSM.

const FOCUS_NAVIGATION := preload("res://scripts/ui/focus_navigation.gd")
const THEME_FACTORY := preload("res://scripts/ui/theme_factory.gd")

var _messages: Dictionary = {}


func _ready() -> void:
	theme = THEME_FACTORY.build()
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	FOCUS_NAVIGATION.wire(self)


func set_messages(messages: Dictionary) -> void:
	_messages = messages.duplicate(true)


func focus_ring_size() -> int:
	return FOCUS_NAVIGATION.wire(self)
