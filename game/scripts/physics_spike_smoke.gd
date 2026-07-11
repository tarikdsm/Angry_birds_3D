extends SceneTree

var _world: Node
var _faults: Array[String] = []


func _initialize() -> void:
	var fallback_enabled := bool(ProjectSettings.get_setting(
		"rendering/rendering_device/fallback_to_opengl3",
		false
	))
	if not fallback_enabled:
		_fail("OpenGL fallback project setting is not enabled")
		return

	if not ClassDB.class_exists("Box3DWorldNode"):
		_fail("Box3DWorldNode is not registered")
		return

	_world = ClassDB.instantiate("Box3DWorldNode")
	if _world == null:
		_fail("Box3DWorldNode could not be instantiated")
		return

	root.add_child(_world)
	_world.physics_fault.connect(_on_physics_fault)
	if not _world.configure_planet(10.0, 9.0):
		_fail("configure_planet failed")
		return

	var projectile: int = _world.spawn_projectile(
		0.4,
		Transform3D(Basis.IDENTITY, Vector3(0.0, 15.0, 0.0)),
		Vector3.ZERO
	)
	if projectile == 0:
		_fail("spawn_projectile failed")
		return

	for tick in range(120):
		if not _world.step_fixed():
			_fail("step_fixed failed at tick %d" % tick)
			return

	var states: Array = _world.get_body_states()
	if states.size() != 2:
		_fail("expected 2 body states, got %d" % states.size())
		return

	var projectile_position := Vector3.ZERO
	var found_projectile := false
	for state: Dictionary in states:
		if int(state.handle) == projectile:
			projectile_position = state.position
			found_projectile = true
			break
	if not found_projectile:
		_fail("projectile state was not returned")
		return
	if projectile_position.length() >= 15.0:
		_fail("projectile did not fall toward the planet")
		return

	if not _world.reset_world():
		_fail("reset_world failed")
		return
	if not _world.get_body_states().is_empty():
		_fail("reset snapshots must stay empty until the queued planet is stepped")
		return
	if not _faults.is_empty():
		_fail("physics_fault emitted: %s" % "; ".join(_faults))
		return

	_finish(0)


func _on_physics_fault(code: String, message: String) -> void:
	_faults.append("%s: %s" % [code, message])


func _fail(message: String) -> void:
	push_error("physics spike smoke: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	if is_instance_valid(_world):
		root.remove_child(_world)
		_world.free()
	quit(exit_code)
