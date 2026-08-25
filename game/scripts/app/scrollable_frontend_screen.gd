extends Control

## Frontend screen shell.
##
## Every Task 19/21 screen shares this shell, so it is the single place that
## gives them the audited product theme and the surface the contrast pairs were
## measured against. A screen left on the engine default palette would be a
## screen no contrast probe ever measured, and these are the first screens the
## player sees.

const THEME_FACTORY := preload("res://scripts/ui/theme_factory.gd")

@export var content_size := Vector2(1280.0, 680.0)


func _ready() -> void:
	var authored_children := get_children()
	theme = THEME_FACTORY.build()
	var backdrop := ColorRect.new()
	backdrop.name = &"Backdrop"
	backdrop.color = THEME_FACTORY.SURFACE
	backdrop.mouse_filter = Control.MOUSE_FILTER_IGNORE
	backdrop.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(backdrop)
	var scroll := ScrollContainer.new()
	scroll.name = &"ContentScroll"
	scroll.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	scroll.follow_focus = true
	add_child(scroll)
	var content := Control.new()
	content.name = &"Content"
	content.custom_minimum_size = content_size
	scroll.add_child(content)
	for child: Node in authored_children:
		remove_child(child)
		content.add_child(child)
