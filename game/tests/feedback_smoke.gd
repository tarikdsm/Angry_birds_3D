extends SceneTree

const SCENE_PATH := "res://scenes/vertical_slice.tscn"
const SUCCESS_MARKER := "FEEDBACK_SMOKE_OK"
const EXPECTED_AUDIO := [
	"launch", "vortex", "pine", "glass", "brick",
	"helmet", "vulnerable", "victory", "defeat",
]

var _scene: Node


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	var packed := load(SCENE_PATH) as PackedScene
	if packed == null:
		_fail("vertical slice scene could not be loaded")
		return
	_scene = packed.instantiate()
	root.add_child(_scene)
	await physics_frame
	await physics_frame

	var feedback := _scene.get_node_or_null("FeedbackDirector")
	var controller := _scene.get_node_or_null("VerticalSliceController")
	var camera := _scene.get_node_or_null("OrbitalCamera")
	var hud := _scene.get_node_or_null("VerticalSliceHUD")
	if feedback == null or controller == null or camera == null or hud == null:
		_fail("scene must expose feedback, controller, camera and HUD")
		return
	for method: String in [
		"apply_frame", "reset_feedback", "feedback_metrics",
		"profile_for_event", "profile_color", "audio_cues", "mapped_event_kinds",
		"pooled_resource_ids", "directional_anchor_contract",
	]:
		if not feedback.has_method(method):
			_fail("feedback director is missing %s" % method)
			return
	var expected_event_kinds := [
		"bird_launched", "ability_activation_requested", "command_rejected",
		"ability_started", "ability_affected_body", "ability_pulse", "ability_ended",
		"damage_applied", "entity_neutralized", "joint_overloaded",
		"piece_fracture_triggered", "joint_broken", "piece_fractured",
	]
	expected_event_kinds.sort()
	if feedback.mapped_event_kinds() != expected_event_kinds:
		_fail("data-driven mapping must cover every canonical domain event")
		return
	var resource_ids_before: Array = feedback.pooled_resource_ids()
	if resource_ids_before.is_empty():
		_fail("fixed pool must preallocate its resources")
		return

	var snapshots := [{
		"entity_id": 200,
		"enemy_archetype_id": 1,
		"transform": Transform3D.IDENTITY,
	}]
	var cases := [
		[{"kind": "bird_launched"}, "launch"],
		[{"kind": "ability_started"}, "vortex"],
		[{"kind": "ability_pulse"}, "virela_pulse"],
		[{"kind": "piece_fractured", "material_id": 1}, "pine"],
		[{"kind": "piece_fractured", "material_id": 9}, "glass"],
		[{"kind": "piece_fractured", "material_id": 5}, "brick"],
		[{
			"kind": "damage_applied", "affected_entity_id": 200,
			"normal": Vector3(0.0, 0.0, 1.0),
		}, "helmet"],
		[{
			"kind": "damage_applied", "affected_entity_id": 200,
			"normal": Vector3(1.0, 0.0, 0.0),
		}, "vulnerable"],
	]
	for pair: Array in cases:
		var actual := str(feedback.profile_for_event(pair[0], snapshots))
		if actual != pair[1]:
			_fail("event profile expected %s, got %s" % [pair[1], actual])
			return
	var archetypes: Dictionary = JSON.parse_string(FileAccess.get_file_as_string(
		"res://data/archetypes/vertical_slice.archetypes.json"))
	var weakpoint: Dictionary = archetypes.weakpoints[0]
	var directional: Dictionary = feedback.directional_anchor_contract()
	if directional.local_protected_direction != weakpoint.protected_direction \
			or not is_equal_approx(
				float(directional.protected_cone_degrees), float(weakpoint.protected_cone_deg)):
		_fail("feedback directional contract drifted from authored weakpoint")
		return
	var rotated_transform := Transform3D(Basis(Vector3.UP, deg_to_rad(90.0)), Vector3.ZERO)
	var rotated_front := (rotated_transform.basis * Vector3(0.0, 0.0, -1.0)).normalized()
	if str(feedback.profile_for_event({
		"kind": "damage_applied", "affected_entity_id": 200,
		"normal": -rotated_front,
	}, [{"entity_id": 200, "transform": rotated_transform}])) != "helmet":
		_fail("rotated Anchor front must remain protected")
		return
	for angle_case: Array in [[45.0, "helmet"], [46.0, "vulnerable"]]:
		var source_direction := Vector3(
			sin(deg_to_rad(float(angle_case[0]))), 0.0,
			-cos(deg_to_rad(float(angle_case[0]))))
		if str(feedback.profile_for_event({
			"kind": "damage_applied", "affected_entity_id": 200,
			"normal": -source_direction,
		}, snapshots)) != angle_case[1]:
			_fail("Anchor cone boundary %.1f must map to %s" % angle_case)
			return

	var pine: Color = feedback.profile_color("pine")
	var glass: Color = feedback.profile_color("glass")
	var brick: Color = feedback.profile_color("brick")
	var vortex: Color = feedback.profile_color("vortex")
	if pine.is_equal_approx(glass) or pine.is_equal_approx(brick) \
			or glass.is_equal_approx(brick):
		_fail("pine, glass and brick feedback must use distinct material colors")
		return
	if vortex.g < 0.75 or vortex.b < 0.65 or vortex.r > 0.4:
		_fail("Virela vortex must read as turquoise")
		return

	var metrics: Dictionary = feedback.feedback_metrics()
	if int(metrics.get("particle_budget", 999999)) > 30000:
		_fail("particle budget exceeds 30000")
		return
	if int(metrics.get("particle_capacity_runtime", -1)) != int(metrics.particle_budget) \
			or int(metrics.get("fragment_capacity_runtime", -1)) != int(metrics.fragment_budget) \
			or int(metrics.get("audio_voice_capacity_runtime", -1)) != 12:
		_fail("budgets must be measured from runtime pools")
		return
	if int(metrics.get("fragment_budget", 999999)) > 120:
		_fail("fragment budget exceeds 120")
		return
	if float(metrics.get("glass_screen_coverage_limit", 1.0)) > 0.15:
		_fail("glass screen coverage exceeds 15 percent")
		return
	if not metrics.has("measured_glass_screen_coverage") \
			or float(metrics.measured_glass_screen_coverage) > 0.15:
		_fail("measured glass geometry exceeds 15 percent of the viewport")
		return
	var measured_glass_coverage := float(metrics.measured_glass_screen_coverage)
	if not bool(metrics.get("fixed_pools", false)):
		_fail("feedback pools must be fixed")
		return
	var cues: Array = feedback.audio_cues()
	for cue: String in EXPECTED_AUDIO:
		if cue not in cues:
			_fail("missing procedural audio cue %s" % cue)
			return
	if int(metrics.get("audio_loaded_cues", -1)) != EXPECTED_AUDIO.size() \
			or int(metrics.get("audio_load_failures", -1)) != 0 \
			or not bool(metrics.get("audio_all_streams_wav", false)):
		_fail("all procedural cues must import before the smoke")
		return

	var live_position := Vector3(4.0, 12.0, 2.0)
	feedback.apply_frame({
		"tick": 8, "phase": "flight_ability", "outcome": "none",
		"snapshots": [{
			"entity_id": 900, "transform": Transform3D(Basis.IDENTITY, live_position),
		}],
		"events": [{
			"kind": "ability_started", "tick": 8, "entity_id": 900,
			"affected_entity_id": 0, "position": Vector3.ZERO,
		}],
	})
	metrics = feedback.feedback_metrics()
	if not (metrics.get("last_emission_position", Vector3.ZERO) as Vector3).is_equal_approx(live_position):
		_fail("live zero-valued event position must resolve from its entity snapshot")
		return
	if int(metrics.get("audio_playback_successes", 0)) <= 0 \
			or int(metrics.get("audio_assigned_wav_voices", 0)) <= 0 \
			or int(metrics.get("audio_active_voices", 0)) <= 0 \
			or int(metrics.get("feedback_fault_count", -1)) != 0:
		_fail("feedback smoke must exercise successful imported audio playback")
		return

	var initial_nodes := _descendant_count(feedback)
	for cycle in range(20):
		feedback.apply_frame({
			"tick": 100,
			"phase": "flight_ability",
			"outcome": "none",
			"snapshots": snapshots,
			"events": [{
				"kind": "damage_applied", "tick": 100,
				"affected_entity_id": 200, "normal": Vector3.RIGHT,
				"position": Vector3(0.0, 12.0, 0.0),
			}],
		})
		feedback.apply_frame({"tick": 0, "phase": "inspection", "outcome": "none", "events": []})
	if _descendant_count(feedback) != initial_nodes:
		_fail("20 restarts changed the fixed feedback node count")
		return
	if feedback.pooled_resource_ids() != resource_ids_before:
		_fail("20 restarts changed preallocated VFX resource identities")
		return
	metrics = feedback.feedback_metrics()
	if int(metrics.get("orphan_count", -1)) != 0:
		_fail("feedback pools reported orphan nodes after 20 restarts")
		return

	feedback.apply_frame({
		"tick": 100,
		"phase": "flight_ability",
		"outcome": "none",
		"snapshots": snapshots,
		"events": [{
			"kind": "damage_applied", "tick": 100,
			"affected_entity_id": 200, "normal": Vector3.RIGHT,
			"position": Vector3(0.0, 12.0, 0.0),
		}],
	})
	metrics = feedback.feedback_metrics()
	if float(metrics.get("last_first_impact_latency_ms", 9999.0)) >= 150.0 \
			or int(metrics.get("visible_marker_count", 0)) <= 0:
		_fail("first impact was not visibly acknowledged inside 150 ms")
		return
	if not metrics.has("last_impact_overlay_screen_ratio") \
			or float(metrics.last_impact_overlay_screen_ratio) > 0.02:
		_fail("impact feedback obscures too much of the screen")
		return
	var measured_impact_latency := float(metrics.last_first_impact_latency_ms)
	if float(metrics.get("impact_latency_p95_ms", 9999.0)) >= 150.0:
		_fail("measured impact emission p95 exceeds 150 ms")
		return
	var measured_impact_overlay := float(metrics.last_impact_overlay_screen_ratio)

	if not controller.has_method("set_reduced_motion"):
		_fail("controller must expose reduced motion")
		return
	controller.set_reduced_motion(true)
	await process_frame
	feedback.apply_frame({
		"tick": 101,
		"phase": "flight_ability",
		"outcome": "none",
		"snapshots": snapshots,
		"events": [{
			"kind": "damage_applied", "tick": 101,
			"affected_entity_id": 200, "normal": Vector3.RIGHT,
			"position": Vector3(0.0, 12.0, 0.0),
		}],
	})
	metrics = feedback.feedback_metrics()
	if not camera.reduced_motion or not bool(metrics.get("reduced_motion", false)) \
			or float(metrics.get("camera_kick_degrees", -1.0)) != 0.0 \
			or not is_equal_approx(float(camera.fov), 48.0):
		_fail("reduced motion must remove camera shake and FOV kick")
		return
	if not hud.has_method("minimum_contrast_ratio") \
			or float(hud.minimum_contrast_ratio()) < 4.5:
		_fail("astral HUD contrast must be at least 4.5:1")
		return
	var measured_contrast := float(hud.minimum_contrast_ratio())

	feedback.apply_frame({"tick": 102, "phase": "result", "outcome": "victory", "events": []})
	if str(feedback.feedback_metrics().get("last_profile", "")) != "victory":
		_fail("victory outcome did not produce feedback")
		return
	feedback.reset_feedback()
	feedback.apply_frame({"tick": 1, "phase": "result", "outcome": "defeat", "events": []})
	if str(feedback.feedback_metrics().get("last_profile", "")) != "defeat":
		_fail("defeat outcome did not produce feedback")
		return

	print("FEEDBACK_METRICS particles=%d fragments=%d glass=%.6f impact_latency_ms=%.3f impact_overlay=%.6f contrast=%.3f reduced_motion=%s" % [
		int(metrics.get("particle_budget", 0)),
		int(metrics.get("fragment_budget", 0)),
		measured_glass_coverage,
		measured_impact_latency,
		measured_impact_overlay,
		measured_contrast,
		str(camera.reduced_motion),
	])
	print(SUCCESS_MARKER)
	_finish(0)


func _descendant_count(node: Node) -> int:
	var count := 0
	for child: Node in node.get_children():
		count += 1 + _descendant_count(child)
	return count


func _fail(message: String) -> void:
	push_error("feedback smoke: %s" % message)
	_finish(1)


func _finish(code: int) -> void:
	if is_instance_valid(_scene):
		root.remove_child(_scene)
		_scene.free()
	quit(code)
