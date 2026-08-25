extends RefCounted

## Keyboard focus service for the product screens.
##
## The router owns the semantic intents; this helper only guarantees that every
## interactive control of a screen is reachable, that the ring is closed in both
## directions and that the focused control is always visible.


static func focusable_controls(host: Node) -> Array[Control]:
	var controls: Array[Control] = []
	if host == null:
		return controls
	for child: Node in host.find_children("*", "Button", true, false):
		var button := child as Button
		if button != null and button.is_visible_in_tree() and not button.disabled:
			controls.append(button)
	return controls


## Closes the focus ring so that every control is reachable with the semantic
## navigate intent and with the engine focus_next/focus_previous chain.
static func wire(host: Node) -> int:
	var controls := focusable_controls(host)
	var count := controls.size()
	for index: int in count:
		var control := controls[index]
		control.focus_mode = Control.FOCUS_ALL
		var previous := controls[posmod(index - 1, count)]
		var next := controls[posmod(index + 1, count)]
		control.focus_neighbor_top = control.get_path_to(previous)
		control.focus_neighbor_bottom = control.get_path_to(next)
		control.focus_neighbor_left = control.get_path_to(previous)
		control.focus_neighbor_right = control.get_path_to(next)
		control.focus_previous = control.get_path_to(previous)
		control.focus_next = control.get_path_to(next)
	return count


static func focus_first(host: Node) -> Control:
	var controls := focusable_controls(host)
	if controls.is_empty():
		return null
	controls.front().grab_focus()
	return controls.front()


## Moves the focus one step inside the ring. Returns the control that received
## the focus, or null when the screen has none.
static func move(host: Node, focused: Control, step: int) -> Control:
	var controls := focusable_controls(host)
	if controls.is_empty() or step == 0:
		return null
	var index := controls.find(focused)
	if index < 0:
		index = 0 if step > 0 else controls.size() - 1
		controls[index].grab_focus()
		return controls[index]
	var target := controls[posmod(index + step, controls.size())]
	target.grab_focus()
	return target


static func is_ring_closed(host: Node) -> bool:
	var controls := focusable_controls(host)
	if controls.is_empty():
		return false
	for control: Control in controls:
		for path: NodePath in [
				control.focus_neighbor_top, control.focus_neighbor_bottom,
				control.focus_previous, control.focus_next]:
			if path.is_empty() or control.get_node_or_null(path) == null:
				return false
	return true
