extends Control

@export var content_size := Vector2(1280.0, 680.0)


func _ready() -> void:
	var authored_children := get_children()
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
