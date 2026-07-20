extends SceneTree

var _session: Node
var _faults: Array[String] = []

func _materials_json() -> String:
	return JSON.stringify({
		"schema_version": 2,
		"materials": [{
			"id": 1, "key": "wood", "response": "fibrous",
			"density_kg_m3": 520.0, "friction": 0.6,
			"restitution": 0.1, "toughness": 0.5
		}],
		"surfaces": [
			{"id": 1001, "key": "bird", "density_kg_m3": 400.0,
			 "friction": 0.35, "restitution": 0.25},
			{"id": 1002, "key": "pig", "density_kg_m3": 480.0,
			 "friction": 0.45, "restitution": 0.1}
		]
	})

func _archetypes_json() -> String:
	return JSON.stringify({
		"schema_version": 2,
		"presentation_ids": ["visual_bird", "icon_bird", "anim_bird"],
		"score_ids": ["unused_bird"],
		"abilities": [{
			"id": 1, "key": "gravity_ability", "kind": "gravity_field",
			"payload": {"arm_ticks": 9, "duration_ticks": 60, "radius_m": 5.0,
				"max_body_mass_kg": 1000.0, "max_bodies": 16,
				"max_acceleration_m_s2": 20.0, "pulse_speed_m_s": 30.0}
		}],
		"birds": [{
			"id": 1, "key": "bird", "projectile_visual_id": "visual_bird",
			"ability_id": 1, "surface_id": 1001, "mass_kg": 5.0,
			"radius_m": 0.25, "friction": 0.35, "restitution": 0.25,
			"bullet": true, "launch_speed_cap_m_s": 40.0,
			"score_id": "unused_bird", "icon_id": "icon_bird",
			"animation_id": "anim_bird"
		}],
		"weakpoints": [{
			"id": 1, "key": "pig_front", "protected_direction": [-1.0, 0.0, 0.0],
			"protected_cone_deg": 30.0, "protected_multiplier": 0.5,
			"exposed_multiplier": 1.25
		}],
		"enemies": [{
			"id": 1, "key": "pig", "weakpoint_id": 1, "surface_id": 1002,
			"mass_kg": 480.0, "integrity": 100.0,
			"damage_energy_j_per_kg": 2.5, "max_damage": 50.0
		}]
	})

func _level_json() -> String:
	return JSON.stringify({
		"schema_version": 2, "id": "uniform_minimum", "world_id": "earth",
		"region_id": "test_region", "camera_profile_id": "test_camera",
		"presentation_profile_id": "test_presentation",
		"world": {"kind": "uniform", "acceleration_m_s2": [0.0, -9.81, 0.0],
			"bounds": {"min_m": [-24.0, -12.0, -12.0],
				"max_m": [48.0, 32.0, 12.0]}},
		"slingshot": {"asset_id": "slingshot", "rest_position_m": [-4.0, 2.0, 0.0],
			"rest_rotation_xyzw": [0.0, 0.0, 0.0, 1.0],
			"spring_constant_n_m": 5200.0, "energy_efficiency": 0.9,
			"minimum_extension_m": 0.2, "maximum_extension_m": 4.25,
			"plane_policy": "gravity_vertical_camera_yaw",
			"projectile_clearance_m": 0.0, "speed_ceiling_m_s": 40.0},
		"bird_queue": [1, 1],
		"scoring": {"pig_points": 5000, "unused_bird_points": 10000,
			"star_thresholds": [10000, 20000, 30000], "chain_window_ticks": 45,
			"chain_multiplier_step": 0.25, "max_chain_multiplier": 3.0},
		"free_body_ids": [1],
		"bodies": [{
			"body_id": 1, "entity_id": 100, "part_id": 1, "body_type": "dynamic",
			"affected_by_world_gravity": true, "material_id": null,
			"surface_id": 1002, "enemy_archetype_id": 1, "density_kg_m3": 480.0,
			"transform": {"position_m": [8.0, 0.0, 0.0],
				"rotation_xyzw": [0.0, 0.0, 0.0, 1.0]},
			"shape": {"type": "box", "half_extents_m": [0.5, 0.5, 0.5]},
			"visual": {"asset_id": "pig", "bounds_m": [1.0, 1.0, 1.0]}
		}],
		"joints": [], "assemblies": [], "triggers": [],
		"objectives": [{"id": 1, "kind": "neutralize_entity", "target_entity_id": 100}],
		"settle_policy": {"linear_speed_m_s": 0.05,
			"angular_speed_rad_s": 0.05, "rest_ticks": 60},
		"watchdog_ticks": 1500
	})

func _initialize() -> void:
	if not ClassDB.class_exists("GameplaySessionNode"):
		_fail("GameplaySessionNode is not registered")
		return
	if not ClassDB.class_exists("OrbitalSessionNode"):
		_fail("OrbitalSessionNode registration regressed")
		return

	_session = ClassDB.instantiate("GameplaySessionNode")
	if _session == null:
		_fail("GameplaySessionNode could not be instantiated")
		return
	root.add_child(_session)
	_session.gameplay_fault.connect(_on_gameplay_fault)

	for method_name: String in [
		"configure_session", "queue_begin_grab", "queue_pull", "queue_release",
		"queue_activate_ability", "queue_cancel_grab", "restart_level", "consume_frame"
	]:
		if not _session.has_method(method_name):
			_fail("missing binding: %s" % method_name)
			return
	for legacy_method: String in ["queue_begin_aim", "queue_aim", "queue_launch", "queue_cancel_aim"]:
		if _session.has_method(legacy_method):
			_fail("GameplaySessionNode leaked v1 binding: %s" % legacy_method)
			return
	if _session.process_priority != -100:
		_fail("GameplaySessionNode priority must be -100")
		return

	if not _session.configure_session(_materials_json(), _archetypes_json(), _level_json()):
		_fail("configure_session failed")
		return
	var initial: Dictionary = _session.consume_frame()
	var expected_keys := [
		"frame_schema_version", "tick", "ticks_executed", "phase", "outcome",
		"launcher", "locked_plane", "bird_queue", "current_bird", "shot",
		"projectiles", "snapshots", "events", "objectives", "ability_readiness",
		"ability_armed", "trajectory_preview", "score", "stars", "gravity_kind",
		"local_gravity", "metrics", "discarded_time_seconds"
	]
	if initial.size() != expected_keys.size() or not initial.has_all(expected_keys):
		_fail("frame v2 top-level keys drifted")
		return
	if int(initial.frame_schema_version) != 2 or str(initial.phase) != "inspection":
		_fail("initial frame version or phase is invalid")
		return
	if str(initial.gravity_kind) != "uniform" \
			or not (initial.local_gravity as Vector3).is_equal_approx(Vector3(0.0, -9.81, 0.0)):
		_fail("uniform local gravity was not published")
		return
	if (initial.bird_queue as Array) != [1, 1] or int(initial.current_bird) != 1:
		_fail("ordered bird queue was not published")
		return
	if initial.launcher != null or initial.locked_plane != null or initial.shot != null:
		_fail("initial launcher and shot state must be null")
		return

	if _session.queue_pull(INF, 0.0):
		_fail("nonfinite pull should fail")
		return
	if not _faults.is_empty():
		_fail("nonfinite pull must not fault the session")
		return
	if not _session.queue_begin_grab(Vector3.RIGHT):
		_fail("queue_begin_grab failed")
		return
	await physics_frame
	await physics_frame
	var grabbed: Dictionary = _session.consume_frame()
	if str(grabbed.phase) != "grabbed" or not (grabbed.locked_plane is Dictionary):
		_fail("grabbed frame did not publish locked plane")
		return
	if not _session.queue_pull(-2.0, 1.0):
		_fail("queue_pull failed")
		return
	await physics_frame
	await physics_frame
	var pulled: Dictionary = _session.consume_frame()
	if not (pulled.launcher is Dictionary) or not (pulled.trajectory_preview is Dictionary):
		_fail("pull did not publish launcher and trajectory")
		return
	if not _session.queue_release():
		_fail("queue_release failed")
		return
	await physics_frame
	await physics_frame
	var released: Dictionary = _session.consume_frame()
	if str(released.phase) != "flight_ability" or not (released.shot is Dictionary) \
			or (released.projectiles as Array).size() != 1:
		_fail("release did not publish one authoritative projectile")
		return
	var released_shot := released.shot as Dictionary
	if int(released_shot.shot_id) != 1 or int(released_shot.bird_archetype_id) != 1 \
			or int(released_shot.ability_id) != 1 \
			or (released_shot.projectile_ids as Array).size() != 1 \
			or released.locked_plane == null:
		_fail("release did not publish authoritative shot identity and plane")
		return
	var acknowledged: Dictionary = _session.consume_frame()
	if not (acknowledged.events as Array).is_empty() \
			or (acknowledged.snapshots as Array).size() != (released.snapshots as Array).size():
		_fail("consume_frame did not acknowledge events exactly once")
		return
	if not _session.restart_level():
		_fail("restart_level failed")
		return
	var restarted: Dictionary = _session.consume_frame()
	if int(restarted.tick) != 0 or not (restarted.events as Array).is_empty() \
			or not (restarted.projectiles as Array).is_empty():
		_fail("restart was not atomic and clean")
		return

	print("GAMEPLAY_SESSION_NODE_SMOKE_OK")
	_finish(0)

func _on_gameplay_fault(code: String, message: String) -> void:
	_faults.append("%s: %s" % [code, message])

func _fail(message: String) -> void:
	push_error("gameplay session smoke: %s" % message)
	_finish(1)

func _finish(exit_code: int) -> void:
	if is_instance_valid(_session):
		root.remove_child(_session)
		_session.free()
	quit(exit_code)
