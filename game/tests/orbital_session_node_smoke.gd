extends SceneTree

class FrameController extends Node:
	var session: Node
	var consumed_frames := 0
	var latest: Dictionary = {}
	var first: Dictionary = {}

	func _physics_process(_delta: float) -> void:
		latest = session.consume_frame()
		if consumed_frames == 0:
			first = latest
		consumed_frames += 1

var _session: Node
var _controller: FrameController
var _faults: Array[String] = []

func _initialize() -> void:
	if not ClassDB.class_exists("Box3DWorldNode"):
		_fail("Box3DWorldNode registration regressed")
		return
	if not ClassDB.class_exists("OrbitalSessionNode"):
		_fail("OrbitalSessionNode is not registered")
		return

	_session = ClassDB.instantiate("OrbitalSessionNode")
	if _session == null:
		_fail("OrbitalSessionNode could not be instantiated")
		return
	root.add_child(_session)
	_session.gameplay_fault.connect(_on_gameplay_fault)

	for method_name: String in [
		"configure_session", "queue_begin_aim", "queue_aim", "queue_launch",
		"queue_activate_ability", "queue_cancel_aim", "restart_level", "consume_frame"
	]:
		if not _session.has_method(method_name):
			_fail("missing binding: %s" % method_name)
			return
	if _session.process_physics_priority != -100:
		_fail("OrbitalSessionNode physics priority must be -100")
		return
	if _session.process_priority != -100:
		_fail("OrbitalSessionNode priority must be -100")
		return

	_controller = FrameController.new()
	_controller.session = _session
	_controller.process_priority = 0
	root.add_child(_controller)

	var materials := FileAccess.get_file_as_string(
		"res://data/materials/vertical_slice.materials.json")
	var archetypes := FileAccess.get_file_as_string(
		"res://data/archetypes/vertical_slice.archetypes.json")
	var level := FileAccess.get_file_as_string(
		"res://data/levels/first_orbit.level.json")
	if not _session.configure_session(materials, archetypes, level):
		_fail("configure_session failed")
		return
	if not _session.queue_begin_aim():
		_fail("queue_begin_aim failed")
		return

	await physics_frame
	await physics_frame
	if _controller.consumed_frames < 1:
		_fail("controller did not consume a physics frame")
		return
	if int(_controller.first.ticks_executed) < 1 or int(_controller.first.tick) < 1:
		_fail("OrbitalSessionNode did not process before priority-zero controller")
		return
	if not _controller.latest.has_all([
		"tick", "ticks_executed", "phase", "outcome", "birds_remaining",
		"snapshots", "events", "objectives_complete", "objective_targets",
		"ability_readiness", "ability_armed", "trajectory_preview",
		"aim_envelope", "metrics", "discarded_time_seconds"
	]):
		_fail("consume_frame dictionary contract is incomplete")
		return
	var aim_envelope: Dictionary = _controller.latest.aim_envelope
	if not aim_envelope.has_all([
		"shell_radius_m", "theta_min_deg", "theta_max_deg",
		"speed_min_m_s", "speed_max_m_s", "default_speed_m_s"
	]) or not is_equal_approx(float(aim_envelope.shell_radius_m), 13.0) \
			or not is_equal_approx(float(aim_envelope.theta_min_deg), -50.0) \
			or not is_equal_approx(float(aim_envelope.theta_max_deg), 50.0) \
			or not is_equal_approx(float(aim_envelope.speed_min_m_s), 8.0) \
			or not is_equal_approx(float(aim_envelope.speed_max_m_s), 16.0) \
			or not is_equal_approx(float(aim_envelope.default_speed_m_s), 10.5):
		_fail("aim envelope does not match the configured manifest")
		return
	var objective_targets: Array = _controller.latest.objective_targets
	if objective_targets.size() != 1 \
			or not (objective_targets[0] as Dictionary).has_all([
				"entity_id", "current_integrity", "maximum_integrity", "neutralized"]):
		_fail("objective target frame state is incomplete")
		return
	if float((objective_targets[0] as Dictionary).current_integrity) != 100.0 \
			or float((objective_targets[0] as Dictionary).maximum_integrity) != 100.0:
		_fail("initial objective integrity is not authoritative")
		return
	for snapshot: Dictionary in _controller.latest.snapshots:
		if not snapshot.has("is_projectile") or bool(snapshot.is_projectile):
			_fail("initial snapshots must publish a false projectile role")
			return
	if str(_controller.latest.ability_readiness) != "unavailable" \
			or bool(_controller.latest.ability_armed):
		_fail("inspection must expose unavailable unarmed ability state")
		return

	if _session.queue_aim(Vector3(INF, 0.0, 0.0), Vector3.UP, 10.5):
		_fail("nonfinite aim should fail")
		return
	if not _faults.is_empty():
		_fail("nonfinite aim must not fault the session")
		return
	var rejected_tick := int(_controller.latest.tick)
	await physics_frame
	if int(_controller.latest.tick) <= rejected_tick:
		_fail("session did not advance after rejected nonfinite aim")
		return

	print("ORBITAL_SESSION_NODE_SMOKE_OK")
	_finish(0)

func _on_gameplay_fault(code: String, message: String) -> void:
	_faults.append("%s: %s" % [code, message])

func _fail(message: String) -> void:
	push_error("orbital session smoke: %s" % message)
	_finish(1)

func _finish(exit_code: int) -> void:
	if is_instance_valid(_controller):
		root.remove_child(_controller)
		_controller.free()
	if is_instance_valid(_session):
		root.remove_child(_session)
		_session.free()
	quit(exit_code)
