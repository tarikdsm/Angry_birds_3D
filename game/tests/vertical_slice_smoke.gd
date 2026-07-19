extends SceneTree

const SCENE_PATH := "res://scenes/vertical_slice.tscn"
const SUCCESS_MARKER := "VERTICAL_SLICE_SMOKE_OK"
const AIM_COALESCING_MARKER := "AIM_COALESCING_SMOKE_OK"
const MAX_FRAMES_PER_SHOT := 2102
const ROUTE_THETA_DEGREES := [-2.0, 0.0, 0.0]
const ASPECT_VIEWPORTS := [Vector2i(1280, 720), Vector2i(1280, 800), Vector2i(1680, 720)]
const AIM_MATCH_EPSILON := 0.002

var _scene: Node
var _controller: Node
var _launch: Node


class AimQueueProbe:
	extends Node

	var aim_calls: Array[Dictionary] = []
	var launch_calls := 0
	var call_order: Array[String] = []


	func queue_aim(origin: Vector3, tangent: Vector3, speed: float) -> bool:
		aim_calls.append({
			"origin": origin,
			"tangent": tangent,
			"speed": speed,
		})
		call_order.append("aim")
		return true


	func queue_launch() -> bool:
		launch_calls += 1
		call_order.append("launch")
		return true


func _find_glass_material(node: Node) -> ShaderMaterial:
	if node is GeometryInstance3D:
		var material := (node as GeometryInstance3D).material_override
		if material is ShaderMaterial:
			return material as ShaderMaterial
	for child in node.get_children():
		var found := _find_glass_material(child)
		if found != null:
			return found
	return null


func _expected_aim_for_input_sample(
		sample: Dictionary, aim_envelope: Dictionary) -> Dictionary:
	var theta := deg_to_rad(float(sample.theta_degrees))
	var phase := deg_to_rad(float(sample.phase_degrees))
	var shell_radius := float(aim_envelope.shell_radius_m)
	var origin := Vector3(
		-shell_radius * cos(theta), 0.0, shell_radius * sin(theta))
	var azimuth := Vector3(sin(theta), 0.0, cos(theta))
	return {
		"origin": origin,
		"tangent_direction": (Vector3.UP * cos(phase) + azimuth * sin(phase)).normalized(),
		"speed": float(sample.speed),
	}


func _preview_matches_input_sample(
		preview: Dictionary, sample: Dictionary, aim_envelope: Dictionary) -> bool:
	if int(preview.get("canonical_hash", 0)) == 0:
		return false
	var aim_value: Variant = preview.get("aim")
	if aim_value == null or not aim_value is Dictionary:
		return false
	var aim := aim_value as Dictionary
	var expected := _expected_aim_for_input_sample(sample, aim_envelope)
	return (aim.get("origin", Vector3.ZERO) as Vector3).distance_to(expected.origin) \
			<= AIM_MATCH_EPSILON \
		and (aim.get("tangent_direction", Vector3.ZERO) as Vector3).distance_to(
			expected.tangent_direction) <= AIM_MATCH_EPSILON \
		and absf(float(aim.get("speed", 0.0)) - float(expected.speed)) \
			<= AIM_MATCH_EPSILON


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	var packed := load(SCENE_PATH) as PackedScene
	if packed == null:
		_fail("playable scene could not be loaded")
		return
	_scene = packed.instantiate()
	root.add_child(_scene)

	_controller = _scene.get_node_or_null("VerticalSliceController")
	_launch = _scene.get_node_or_null("LaunchController")
	if _controller == null or _launch == null:
		_fail("scene is missing its gameplay controllers")
		return
	if _controller.process_priority != 0:
		_fail("vertical slice controller priority must be zero")
		return
	var camera := _scene.get_node_or_null("OrbitalCamera")
	if camera == null or not camera.has_method("is_planet_occluded"):
		_fail("orbital camera must expose its presentation-only occlusion check")
		return
	if not camera.has_method("is_safe_framing"):
		_fail("orbital camera must expose projected safe-margin framing")
		return
	# Intentional white-box regression seam: GDScript's leading underscore is a
	# naming convention, and publishing this geometry helper solely for the test
	# would unnecessarily enlarge the gameplay API.
	var clamp_focus := Vector3(0.0, 10.15, 0.0)
	var clamped_camera: Vector3 = camera._safe_camera_position(
		Vector3(0.0, -30.0, 0.0), clamp_focus, 24.0)
	if clamped_camera.distance_to(clamp_focus) > 24.001:
		_fail("occlusion correction exceeded the 24 m camera limit")
		return

	await physics_frame
	await physics_frame
	var body_views := _scene.get_node_or_null("BodyViews")
	if body_views == null or not body_views.has_method("missing_asset_ids"):
		_fail("body views must expose missing authored asset diagnostics")
		return
	if not body_views.missing_asset_ids().is_empty():
		_fail("authored visuals missing: %s" % body_views.missing_asset_ids())
		return
	var glass_material := _find_glass_material(body_views)
	if glass_material == null:
		_fail("effective glass override material was not found")
		return
	if glass_material.get_shader_parameter("albedo_texture") == null \
			or not is_equal_approx(float(glass_material.get_shader_parameter("authored_roughness")), 0.18) \
			or not is_equal_approx(float(glass_material.get_shader_parameter("authored_metallic")), 0.0) \
			or not is_equal_approx(float(glass_material.get_shader_parameter("transmission_weight")), 0.78) \
			or not is_equal_approx(float(glass_material.get_shader_parameter("coat_weight")), 0.24):
		_fail("effective glass material differs from authored PBR semantics")
		return
	if not _controller.session_configured:
		_fail("session did not configure from the shipped catalogs")
		return
	var before_calls: int = _controller.consume_calls
	await physics_frame
	if _controller.consume_calls != before_calls + 1:
		_fail("controller must consume exactly one batch per physics frame")
		return
	if str(_controller.current_frame.get("phase", "")) != "inspection":
		_fail("configured scene did not enter inspection")
		return
	var aim_envelope: Dictionary = _controller.current_frame.get("aim_envelope", {})
	if not aim_envelope.has_all([
		"shell_radius_m", "theta_min_deg", "theta_max_deg",
		"speed_min_m_s", "speed_max_m_s", "default_speed_m_s"
	]):
		_fail("configured scene did not receive its authoritative aim envelope")
		return
	var consumed_envelope: Dictionary = _launch.get("_aim_envelope")
	if not is_equal_approx(
			float(consumed_envelope.get("shell_radius_m", 0.0)),
			float(aim_envelope.shell_radius_m)) \
			or not is_equal_approx(
				float(consumed_envelope.get("theta_min_deg", 0.0)),
				float(aim_envelope.theta_min_deg)) \
			or not is_equal_approx(
				float(consumed_envelope.get("theta_max_deg", 0.0)),
				float(aim_envelope.theta_max_deg)) \
			or not is_equal_approx(
				float(consumed_envelope.get("speed_min_m_s", 0.0)),
				float(aim_envelope.speed_min_m_s)) \
			or not is_equal_approx(
				float(consumed_envelope.get("speed_max_m_s", 0.0)),
				float(aim_envelope.speed_max_m_s)) \
			or not is_equal_approx(
				float(consumed_envelope.get("default_speed_m_s", 0.0)),
				float(aim_envelope.default_speed_m_s)):
		_fail("launch input did not consume the authoritative aim envelope")
		return
	var camera_ring_point: Vector3 = camera.get("_ring_point")
	if not is_equal_approx(camera_ring_point.length(), float(aim_envelope.shell_radius_m)):
		_fail("camera did not consume the authoritative launch shell radius")
		return
	var real_session: Node = _launch.get("_session")
	var aim_probe := AimQueueProbe.new()
	_scene.add_child(aim_probe)
	_launch.set_process(false)
	_launch.bind_session(aim_probe)
	_launch.observe_frame({"phase": "aim", "aim_envelope": aim_envelope})
	for sample in [
		{"theta": -1.0, "phase": 0.0, "speed": 8.5},
		{"theta": 0.0, "phase": 2.0, "speed": 9.0},
		{"theta": 1.0, "phase": 4.0, "speed": 9.5},
		{"theta": 2.0, "phase": 6.0, "speed": 10.0},
	]:
		if not _launch.set_aim_degrees(sample.theta, sample.phase, sample.speed):
			_fail("coalesced aim probe rejected an input sample")
			return
	if not aim_probe.aim_calls.is_empty():
		_fail("aim updates reached native preview before the frame flush")
		return
	_launch._process(0.0)
	if aim_probe.aim_calls.size() != 1 \
			or not is_equal_approx(float(aim_probe.aim_calls[0].speed), 10.0):
		_fail("four aim updates did not coalesce to the final frame value")
		return
	_launch._process(0.0)
	if aim_probe.aim_calls.size() != 1:
		_fail("an unchanged aim was submitted again on the next frame")
		return
	if not _launch.set_aim_degrees(1.5, 3.0, 9.75) \
			or not _launch.launch_or_activate():
		_fail("launch barrier rejected a staged final aim")
		return
	if aim_probe.aim_calls.size() != 2 or aim_probe.launch_calls != 1 \
			or aim_probe.call_order.slice(-2) != ["aim", "launch"] \
			or not is_equal_approx(float(aim_probe.aim_calls[1].speed), 9.75):
		_fail("launch did not flush the final staged aim before launch")
		return
	_launch._process(0.0)
	if aim_probe.aim_calls.size() != 2:
		_fail("launch barrier left a duplicate aim pending")
		return
	_launch.bind_session(real_session)
	_launch.observe_frame(_controller.current_frame)
	_launch.set_process(true)
	aim_probe.queue_free()
	if "--finding64-only" in OS.get_cmdline_user_args():
		print(AIM_COALESCING_MARKER)
		_scene.get_tree().quit(0)
		return
	if not _launch.begin_aim():
		_fail("causal input probe could not begin aim")
		return
	await physics_frame
	var causal_samples: Array[Dictionary] = [
		{"command_id": "set_aim_center", "theta_degrees": 0.0,
			"phase_degrees": 0.0, "speed": 10.5},
		{"command_id": "set_aim_right", "theta_degrees": 2.0,
			"phase_degrees": 0.0, "speed": 8.0},
		{"command_id": "set_aim_left", "theta_degrees": -2.0,
			"phase_degrees": 0.0, "speed": 8.0},
	]
	var causal_hashes := {}
	for sample: Dictionary in causal_samples:
		var consume_calls_before_sample: int = _controller.consume_calls
		if not _launch.set_aim_degrees(
				float(sample.theta_degrees), float(sample.phase_degrees), float(sample.speed)):
			_fail("causal input probe rejected %s" % sample.command_id)
			return
		await process_frame
		await physics_frame
		await process_frame
		if _controller.consume_calls <= consume_calls_before_sample:
			_fail("causal input preview was read before the controller consumed a frame")
			return
		var observed_preview: Variant = _controller.current_frame.get("trajectory_preview")
		if observed_preview == null or not observed_preview is Dictionary \
				or not _preview_matches_input_sample(observed_preview, sample, aim_envelope):
			_fail("causal input preview mismatch command=%s observed=%s expected=%s" % [
				sample.command_id,
				observed_preview,
				_expected_aim_for_input_sample(sample, aim_envelope),
			])
			return
		var observed_hash := str(int(observed_preview.canonical_hash))
		if causal_hashes.has(observed_hash):
			_fail("causal input previews reused hash %s" % observed_hash)
			return
		causal_hashes[observed_hash] = true
	_launch.cancel_aim_or_toggle_pause()
	await physics_frame
	await physics_frame
	if str(_controller.current_frame.get("phase", "")) != "inspection":
		_fail("causal input probe did not return to inspection")
		return
	var controls := _scene.get_node_or_null("VerticalSliceHUD/Root/ControlsLabel") as Label
	var hud_root := _scene.get_node_or_null("VerticalSliceHUD/Root") as Control
	var integrity_label := _scene.get_node("VerticalSliceHUD/Root/TopBar/Margin/Readout/IntegrityLabel") as Label
	if controls == null:
		_fail("HUD controls must live under a viewport-sized Control root")
		return
	var viewport_rect := root.get_visible_rect()
	if not hud_root.get_global_rect().is_equal_approx(viewport_rect):
		_fail("HUD root must cover the visible viewport")
		return
	var visible_controls := controls.get_global_rect().intersection(root.get_visible_rect())
	if visible_controls.size.x <= 1.0 or visible_controls.size.y <= 1.0 \
			or controls.get_global_rect().position.y < 0.0 \
			or controls.get_global_rect().size.y > 64.0:
		_fail("HUD controls are outside the visible viewport")
		return
	var ring: Node3D = _scene.get_node("ImpulseRingVisual")
	if not _launch.has_method("can_begin_aim_at"):
		_fail("launch input must expose its presentation-only ring hit test")
		return
	var ring_screen: Vector2 = camera.unproject_position(ring.global_position)
	if _launch.can_begin_aim_at(ring_screen):
		_fail("empty center of the impulse ring must not be clickable")
		return
	var ring_band_world := ring.global_transform * Vector3(0.7, 0.0, 0.0)
	var ring_band_screen: Vector2 = camera.unproject_position(ring_band_world)
	if not _launch.can_begin_aim_at(ring_band_screen):
		_fail("visible annulus band of the impulse ring must be clickable")
		return
	if _launch.can_begin_aim_at(ring_screen + Vector2(500.0, 500.0)):
		_fail("clicks outside the impulse ring must not begin aim")
		return
	if not _launch.has_method("solve_aim_from_screen"):
		_fail("aim authorship must expose its camera ray-shell tangent solve")
		return
	var solved_aim: Dictionary = _launch.solve_aim_from_screen(ring_screen)
	if solved_aim.is_empty():
		_fail("camera ray did not solve an aim at the projected ring")
		return
	var solved_origin: Vector3 = solved_aim.origin
	var solved_tangent: Vector3 = solved_aim.tangent
	if absf(solved_origin.length() - float(aim_envelope.shell_radius_m)) > 0.001 \
			or absf(solved_origin.y) > 0.001 \
			or absf(solved_origin.normalized().dot(solved_tangent)) > 0.0001:
		_fail("screen aim must lie on the launch shell and its tangent plane")
		return
	var fault_label := _scene.get_node("VerticalSliceHUD/Root/FaultLabel") as Label
	paused = true
	Input.action_press("aim_left")
	for paused_frame in range(140):
		var paused_motion := InputEventMouseMotion.new()
		paused_motion.position = ring_screen
		paused_motion.relative = Vector2(1.0, 1.0)
		paused_motion.alt_pressed = true
		paused_motion.button_mask = MOUSE_BUTTON_MASK_LEFT
		Input.parse_input_event(paused_motion)
		await process_frame
	Input.action_release("aim_left")
	paused = false
	await physics_frame
	if not fault_label.text.is_empty():
		_fail("paused aim input must not enqueue commands: %s" % fault_label.text)
		return

	var saw_ability_started := false
	var saw_early_ability_rejection := false
	var saw_objective_damage_below_full := false
	var saw_objective_neutralized_at_zero := false
	var saw_semantic_damage_classification := false
	var saw_projectile_role := false
	for shot in range(ROUTE_THETA_DEGREES.size()):
		root.size = ASPECT_VIEWPORTS[shot]
		for settle_frame in range(20):
			await process_frame
		if not hud_root.get_global_rect().is_equal_approx(root.get_visible_rect()):
			_fail("HUD layout did not adapt to aspect viewport %s" % ASPECT_VIEWPORTS[shot])
			return
		if not camera.is_safe_framing():
			_fail("inspection framing failed at aspect viewport %s" % ASPECT_VIEWPORTS[shot])
			return
		if shot == 1:
			var birds_before_cancel := int(_controller.current_frame.get("birds_remaining", -1))
			if not _launch.begin_aim():
				_fail("cancel regression could not enter aim")
				return
			await physics_frame
			await physics_frame
			_launch.cancel_aim_or_toggle_pause()
			await physics_frame
			await physics_frame
			if str(_controller.current_frame.get("phase", "")) != "inspection" \
					or int(_controller.current_frame.get("birds_remaining", -1)) != birds_before_cancel:
				_fail("Esc cancel must preserve progress and the remaining roster")
				return
			if _controller.current_frame.get("trajectory_preview") != null:
				_fail("Esc cancel must clear the authoritative trajectory preview")
				return
		if not _launch.begin_aim():
			_fail("shot %d could not begin aim" % shot)
			return
		await physics_frame
		var consume_calls_before_shot_aim: int = _controller.consume_calls
		if not _launch.set_aim_degrees(ROUTE_THETA_DEGREES[shot], 0.0, 8.0):
			_fail("shot %d rejected the verified aim" % shot)
			return
		await process_frame
		await physics_frame
		await process_frame
		if _controller.consume_calls <= consume_calls_before_shot_aim:
			_fail("shot aim preview was read before the controller consumed a frame")
			return
		var preview: Variant = _controller.current_frame.get("trajectory_preview")
		if preview == null or not preview is Dictionary or preview.samples.is_empty():
			_fail("kernel trajectory preview was not received")
			return
		for settle_frame in range(20):
			await process_frame
		if not camera.is_safe_framing():
			_fail("aim framing failed at aspect viewport %s" % ASPECT_VIEWPORTS[shot])
			return
		if not _launch.launch_or_activate():
			_fail("shot %d could not launch" % shot)
			return

		var launch_tick := -1
		var ability_requested := false
		var early_ability_requested := false
		var event_trace: Array[String] = []
		for frame_index in range(MAX_FRAMES_PER_SHOT):
			await physics_frame
			var frame: Dictionary = _controller.current_frame
			if camera.is_planet_occluded():
				_fail("camera crossed planet shot=%d frame=%d phase=%s position=%s focus=%s" % [
					shot,
					frame_index,
					frame.get("phase", ""),
					camera.global_position,
					camera.get("_focus"),
				])
				return
			if frame_index > 15 and not camera.is_safe_framing():
				_fail("camera safe framing failed during shot %d frame %d" % [shot, frame_index])
				return
			if frame_index > 15 \
					and camera.global_position.distance_to(camera.get("_focus")) > 24.001:
				_fail("runtime camera exceeded 24 m during shot %d frame %d" % [shot, frame_index])
				return
			var objective_targets: Array = frame.get("objective_targets", [])
			if objective_targets.size() != 1:
				_fail("frame must expose exactly one authoritative objective target")
				return
			var objective := objective_targets.front() as Dictionary
			var objective_entity_id := int(objective.get("entity_id", 0))
			for event: Dictionary in frame.get("events", []):
				var kind := str(event.get("kind", ""))
				if kind in ["bird_launched", "ability_started", "ability_ended", "entity_neutralized", "command_rejected"]:
					event_trace.append("%s@%d" % [kind, int(event.get("tick", 0))])
				if kind == "bird_launched":
					launch_tick = int(event.get("tick", frame.get("tick", 0)))
				elif kind == "ability_started":
					saw_ability_started = true
				elif kind == "command_rejected" \
						and str(event.get("rejection_reason_name", "")) == "not_armed":
					saw_early_ability_rejection = true
					if "AINDA NÃO ARMADA" not in controls.text \
							or "AGUARDE" not in controls.text:
						_fail("early Virela rejection was not legible in the HUD")
						return
				elif kind == "damage_applied" \
						and int(event.get("affected_entity_id", 0)) == objective_entity_id:
					var classification := str(event.get("damage_classification", ""))
					if classification not in ["protected", "vulnerable"]:
						_fail("Anchor damage event lacks native semantic classification")
						return
					saw_semantic_damage_classification = true
			var projectile_snapshot_count := 0
			for snapshot: Dictionary in frame.get("snapshots", []):
				if not snapshot.has("is_projectile"):
					_fail("snapshot omitted the native projectile role")
					return
				if bool(snapshot.is_projectile):
					projectile_snapshot_count += 1
			if projectile_snapshot_count > 1:
				_fail("frame published more than one projectile role")
				return
			if launch_tick >= 0 and str(frame.get("phase", "")) == "flight_ability":
				if projectile_snapshot_count != 1:
					_fail("flight frame did not publish exactly one projectile role")
					return
				saw_projectile_role = true
			var current_integrity := float(objective.get("current_integrity", -1.0))
			var maximum_integrity := float(objective.get("maximum_integrity", 0.0))
			if maximum_integrity <= 0.0 or current_integrity < 0.0 \
					or current_integrity > maximum_integrity:
				_fail("objective integrity state is invalid: %s" % objective)
				return
			var expected_percent := roundi(100.0 * current_integrity / maximum_integrity)
			var displayed_integrity := _hud_integrity_percent(integrity_label)
			if displayed_integrity != expected_percent:
				_fail("HUD=%d differs from authoritative objective integrity=%d" % [
					displayed_integrity, expected_percent])
				return
			if current_integrity < maximum_integrity:
				saw_objective_damage_below_full = true
			if bool(objective.get("neutralized", false)) and displayed_integrity == 0:
				saw_objective_neutralized_at_zero = true
			if shot == 0 and not early_ability_requested and launch_tick >= 0 \
					and int(frame.get("tick", 0)) >= launch_tick + 1:
				if not _launch.launch_or_activate():
					_fail("controller did not delegate the early Virela request")
					return
				early_ability_requested = true
			if not ability_requested and launch_tick >= 0 \
					and int(frame.get("tick", 0)) >= launch_tick + 39:
				if not _launch.launch_or_activate():
					_fail("shot %d could not request Virela" % shot)
					return
				ability_requested = true
			var phase := str(frame.get("phase", ""))
			if phase == "result" or (phase == "inspection" and ability_requested):
				break
			if frame_index == MAX_FRAMES_PER_SHOT - 1:
				_fail(
					"shot %d timed out: tick=%d phase=%s birds=%d launch_tick=%d ability=%s" % [
						shot,
						int(frame.get("tick", 0)),
						phase,
						int(frame.get("birds_remaining", -1)),
						launch_tick,
						"%s events=%s objective=%s" % [
							ability_requested,
							", ".join(event_trace),
							frame.get("objectives_complete", false),
						],
					])
				return
		if str(_controller.current_frame.get("phase", "")) == "result":
			break

	if str(_controller.current_frame.get("phase", "")) != "result":
		_fail("verified route did not produce a result")
		return
	if str(_controller.current_frame.get("outcome", "")) != "victory":
		_fail("verified route did not produce victory")
		return
	if not saw_ability_started:
		_fail("verified route did not publish ability_started")
		return
	if not saw_early_ability_rejection:
		_fail("verified route did not expose the authoritative early ability rejection")
		return
	if not saw_objective_damage_below_full:
		_fail("real Anchor damage did not reduce the HUD below 100%")
		return
	if not saw_objective_neutralized_at_zero:
		_fail("Anchor neutralization did not set the HUD to 0%")
		return
	if not saw_semantic_damage_classification:
		_fail("verified route did not expose native Anchor damage classification")
		return
	if not saw_projectile_role:
		_fail("verified route did not expose the native projectile role")
		return
	if not _launch.restart_now():
		_fail("restart was rejected after result")
		return
	await physics_frame
	await physics_frame
	if str(_controller.current_frame.get("phase", "")) != "inspection":
		_fail("restart did not restore inspection")
		return
	var restarted_targets: Array = _controller.current_frame.get("objective_targets", [])
	if restarted_targets.size() != 1:
		_fail("restart did not restore authoritative objective state")
		return
	var restarted_target := restarted_targets.front() as Dictionary
	if not is_equal_approx(
			float(restarted_target.get("current_integrity", -1.0)),
			float(restarted_target.get("maximum_integrity", 0.0))) \
			or _hud_integrity_percent(integrity_label) != 100 \
			or str(_controller.current_frame.get("ability_readiness", "")) != "unavailable" \
			or bool(_controller.current_frame.get("ability_armed", true)) \
			or _controller.current_frame.get("trajectory_preview") != null:
		_fail("restart did not reset HUD, ability readiness and preview authoritatively")
		return

	# Prove the loss path through the same Godot/GDExtension public controls.
	# A steep tangential shot misses the fortification; no ability is requested.
	for defeat_shot in range(3):
		if not _launch.begin_aim():
			_fail("defeat shot %d could not begin aim" % defeat_shot)
			return
		await physics_frame
		if not _launch.set_aim_degrees(0.0, 80.0, 8.0):
			_fail("defeat shot %d rejected miss aim" % defeat_shot)
			return
		await physics_frame
		if not _launch.launch_or_activate():
			_fail("defeat shot %d could not launch" % defeat_shot)
			return
		for defeat_frame in range(MAX_FRAMES_PER_SHOT):
			await physics_frame
			var defeat_phase := str(_controller.current_frame.get("phase", ""))
			if defeat_phase == "result" or defeat_phase == "inspection":
				break
			if defeat_frame == MAX_FRAMES_PER_SHOT - 1:
				_fail("defeat shot %d timed out" % defeat_shot)
				return
		if str(_controller.current_frame.get("phase", "")) == "result":
			break
	if str(_controller.current_frame.get("phase", "")) != "result" \
			or str(_controller.current_frame.get("outcome", "")) != "defeat":
		_fail("three public miss shots did not produce defeat")
		return

	print(SUCCESS_MARKER)
	_finish(0)


func _hud_integrity_percent(label: Label) -> int:
	var digits := ""
	for character: String in label.text:
		if character >= "0" and character <= "9":
			digits += character
	return int(digits)


func _fail(message: String) -> void:
	push_error("vertical slice smoke: %s" % message)
	_finish(1)


func _finish(exit_code: int) -> void:
	if is_instance_valid(_scene):
		root.remove_child(_scene)
		_scene.free()
	quit(exit_code)
