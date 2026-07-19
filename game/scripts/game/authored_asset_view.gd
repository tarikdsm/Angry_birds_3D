extends Node3D

func _ready() -> void:
	_prepare_descendants(self)


func _prepare_descendants(node: Node) -> void:
	for child: Node in node.get_children():
		var child_name := str(child.name)
		if child is Node3D and (
				child_name.begins_with("COL_")
				or child_name.begins_with("FRAG_")
				or child_name.begins_with("SOCKET_")
				or child_name.begins_with("RIG_")
				or child_name.ends_with("_LOD1")):
			(child as Node3D).visible = false
		elif child is GeometryInstance3D:
			(child as GeometryInstance3D).cast_shadow = \
				GeometryInstance3D.SHADOW_CASTING_SETTING_ON
		_prepare_descendants(child)
