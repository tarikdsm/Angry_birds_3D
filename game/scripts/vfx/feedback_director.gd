extends Node3D

const CONFIG_PATH := "res://data/feedback/vertical_slice.feedback.json"
const ARCHETYPES_PATH := "res://data/archetypes/vertical_slice.archetypes.json"
const DEFAULT_POSITION := Vector3(0.0, 12.0, 0.0)
const SNAPSHOT_POSITION_KINDS := [
	"bird_launched", "ability_activation_requested", "ability_started",
	"ability_affected_body", "ability_pulse", "ability_ended",
]

var _config: Dictionary = {}
var _vfx_slots: Array[Dictionary] = []
var _fragment_nodes: Array[MeshInstance3D] = []
var _profile_resources: Dictionary = {}
var _directional_contract: Dictionary = {}
var _expected_resource_ids: Array = []
var _impact_latency_samples_ms: Array[float] = []
var _slot_cursor := 0
var _last_tick := -1
var _last_outcome := "none"
var _last_profile := ""
var _last_first_impact_latency_ms := -1.0
var _last_impact_overlay_screen_ratio := 0.0
var _measured_glass_screen_coverage := 0.0
var _last_emission_position := Vector3.ZERO
var _feedback_fault_count := 0
var _reduced_motion := false
var _expected_descendants := 0
var _audio_pool: Node


func _ready() -> void:
	_config = _load_config()
	_directional_contract = _load_directional_contract()
	_build_profile_resources()
	_build_vfx_pool()
	_build_fragment_pool()
	_audio_pool = get_node_or_null("FeedbackAudioPool")
	_expected_descendants = _descendant_count(self)
	_expected_resource_ids = pooled_resource_ids()
	_validate_runtime_budgets()


func apply_frame(frame: Dictionary) -> void:
	var tick := int(frame.get("tick", 0))
	if tick < _last_tick:
		reset_feedback()
	_last_tick = tick
	var snapshots: Array = frame.get("snapshots", [])
	_measured_glass_screen_coverage = _measure_glass_screen_coverage(snapshots)
	for event: Dictionary in frame.get("events", []):
		var profile := profile_for_event(event, snapshots)
		if profile.is_empty():
			continue
		_emit_profile(profile, _event_position(event, snapshots), event)
	var outcome := str(frame.get("outcome", "none"))
	if outcome != "none" and outcome != _last_outcome:
		var outcome_profiles: Dictionary = _config.get("outcome_profiles", {})
		var outcome_profile := str(outcome_profiles.get(outcome, ""))
		if not outcome_profile.is_empty():
			_emit_profile(outcome_profile, _anchor_position(snapshots), {})
	_last_outcome = outcome


func reset_feedback() -> void:
	for slot: Dictionary in _vfx_slots:
		var particles := slot.particles as GPUParticles3D
		particles.emitting = false
		var marker := slot.marker as MeshInstance3D
		marker.visible = false
		slot.expires_at_ms = 0
	for fragment: MeshInstance3D in _fragment_nodes:
		fragment.visible = false
	if _audio_pool != null and _audio_pool.has_method("reset_pool"):
		_audio_pool.reset_pool()
	_slot_cursor = 0
	_last_tick = -1
	_last_outcome = "none"
	_last_profile = ""
	_last_first_impact_latency_ms = -1.0
	_last_impact_overlay_screen_ratio = 0.0
	_measured_glass_screen_coverage = 0.0
	_last_emission_position = Vector3.ZERO
	_impact_latency_samples_ms.clear()


func set_reduced_motion(enabled: bool) -> void:
	_reduced_motion = enabled


func profile_for_event(event: Dictionary, snapshots: Array) -> String:
	var kind := str(event.get("kind", ""))
	var event_profiles: Dictionary = _config.get("event_profiles", {})
	var mapped := str(event_profiles.get(kind, ""))
	if mapped == "material" or mapped == "material_or_anchor":
		if mapped == "material_or_anchor" \
				and int(event.get("affected_entity_id", 0)) == _anchor_entity_id():
			return "helmet" if _is_protected_anchor_hit(event, snapshots) else "vulnerable"
		var material_id := int(event.get("material_id", 0))
		if material_id == 0:
			material_id = _material_for_entity(
				int(event.get("affected_entity_id", event.get("entity_id", 0))), snapshots)
		var material_profiles: Dictionary = _config.get("material_profiles", {})
		return str(material_profiles.get(str(material_id), ""))
	return mapped


func profile_color(profile: String) -> Color:
	var definition := _profile(profile)
	return Color.from_string(str(definition.get("color", "#FFFFFF")), Color.WHITE)


func audio_cues() -> Array:
	if _audio_pool != null and _audio_pool.has_method("available_cues"):
		return _audio_pool.available_cues()
	return []


func mapped_event_kinds() -> Array:
	var result: Array[String] = []
	for kind: String in (_config.get("event_profiles", {}) as Dictionary):
		result.append(kind)
	result.sort()
	return result


func pooled_resource_ids() -> Array:
	var ids: Array[int] = []
	for resources: Dictionary in _profile_resources.values():
		for key: String in ["marker_mesh", "process_normal", "process_reduced", "draw_mesh", "fragment_material"]:
			var resource := resources.get(key) as Resource
			if resource != null:
				ids.append(resource.get_instance_id())
		var marker_mesh := resources.marker_mesh as PrimitiveMesh
		var draw_mesh := resources.draw_mesh as PrimitiveMesh
		if marker_mesh.material != null:
			ids.append(marker_mesh.material.get_instance_id())
		if draw_mesh.material != null:
			ids.append(draw_mesh.material.get_instance_id())
	for fragment: MeshInstance3D in _fragment_nodes:
		if fragment.mesh != null:
			ids.append(fragment.mesh.get_instance_id())
	if _audio_pool != null and _audio_pool.has_method("pooled_stream_resource_ids"):
		ids.append_array(_audio_pool.pooled_stream_resource_ids())
	ids.sort()
	return ids


func directional_anchor_contract() -> Dictionary:
	return _directional_contract.duplicate(true)


func feedback_metrics() -> Dictionary:
	var budgets: Dictionary = _config.get("budgets", {})
	var particle_capacity := 0
	for slot: Dictionary in _vfx_slots:
		particle_capacity += (slot.particles as GPUParticles3D).amount
	var audio_metrics := _audio_metrics()
	var resource_ids := pooled_resource_ids()
	return {
		"fixed_pools": _descendant_count(self) == _expected_descendants \
			and resource_ids == _expected_resource_ids,
		"vfx_pool_size": _vfx_slots.size(),
		"audio_voice_pool_size": int(audio_metrics.get("voice_capacity", 0)),
		"fragment_budget": _fragment_nodes.size(),
		"particle_budget": particle_capacity,
		"particle_capacity_runtime": particle_capacity,
		"fragment_capacity_runtime": _fragment_nodes.size(),
		"audio_voice_capacity_runtime": int(audio_metrics.get("voice_capacity", 0)),
		"audio_loaded_cues": int(audio_metrics.get("loaded_cues", 0)),
		"audio_load_failures": int(audio_metrics.get("load_failures", 0)),
		"audio_playback_requests": int(audio_metrics.get("playback_requests", 0)),
		"audio_playback_successes": int(audio_metrics.get("playback_successes", 0)),
		"audio_active_voices": int(audio_metrics.get("active_voices", 0)),
		"audio_assigned_wav_voices": int(audio_metrics.get("assigned_wav_voices", 0)),
		"audio_all_streams_wav": bool(audio_metrics.get("all_streams_wav", false)),
		"glass_screen_coverage_limit": float(budgets.get("glass_screen_coverage_limit", 1.0)),
		"measured_glass_screen_coverage": _measured_glass_screen_coverage,
		"orphan_count": maxi(0, _descendant_count(self) - _expected_descendants),
		"visible_marker_count": _visible_marker_count(),
		"last_profile": _last_profile,
		"last_first_impact_latency_ms": _last_first_impact_latency_ms,
		"impact_latency_p95_ms": _latency_p95_ms(),
		"last_impact_overlay_screen_ratio": _last_impact_overlay_screen_ratio,
		"reduced_motion": _reduced_motion,
		"camera_kick_degrees": 0.0 if _reduced_motion else 2.0,
		"last_emission_position": _last_emission_position,
		"feedback_fault_count": _feedback_fault_count,
	}


func _process(_delta: float) -> void:
	var now := Time.get_ticks_msec()
	for slot: Dictionary in _vfx_slots:
		if int(slot.expires_at_ms) > 0 and now >= int(slot.expires_at_ms):
			var marker := slot.marker as MeshInstance3D
			marker.visible = false
			slot.expires_at_ms = 0


func _emit_profile(profile: String, position: Vector3, event: Dictionary) -> void:
	var started_us := Time.get_ticks_usec()
	var definition := _profile(profile)
	if definition.is_empty() or _vfx_slots.is_empty():
		return
	var slot: Dictionary = _vfx_slots[_slot_cursor]
	_slot_cursor = (_slot_cursor + 1) % _vfx_slots.size()
	var root := slot.root as Node3D
	var marker := slot.marker as MeshInstance3D
	var particles := slot.particles as GPUParticles3D
	var size := float(definition.get("size_m", 0.1))
	root.global_position = position
	_configure_marker(marker, profile)
	_configure_particles(particles, profile, definition)
	marker.visible = true
	_last_emission_position = position
	if str(event.get("kind", "")) == "damage_applied":
		_last_first_impact_latency_ms = float(Time.get_ticks_usec() - started_us) / 1000.0
		_impact_latency_samples_ms.append(_last_first_impact_latency_ms)
	particles.restart()
	particles.emitting = true
	var marker_duration_ms := mini(140, roundi(float(definition.get("lifetime_s", 0.3)) * 1000.0))
	slot.expires_at_ms = Time.get_ticks_msec() + marker_duration_ms
	_last_profile = profile
	if str(event.get("kind", "")) == "damage_applied":
		_last_impact_overlay_screen_ratio = _screen_ratio_at(position, size)
	var audio := str(definition.get("audio", ""))
	if not audio.is_empty() and _audio_pool != null and _audio_pool.has_method("play_cue"):
		if not _audio_pool.play_cue(audio, position):
			_feedback_fault_count += 1
			push_error("feedback audio playback failed: %s" % audio)
	if str(event.get("kind", "")) in ["piece_fracture_triggered", "piece_fractured"]:
		_show_fragments(position, profile, mini(8, int(definition.get("particles", 8))))


func _build_vfx_pool() -> void:
	var budgets: Dictionary = _config.get("budgets", {})
	var count := int(budgets.get("vfx_pool_size", 0))
	for index in range(count):
		var root := Node3D.new()
		root.name = "BurstSlot%02d" % index
		var marker := MeshInstance3D.new()
		marker.name = "ImmediateMarker"
		marker.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		marker.visible = false
		root.add_child(marker)
		var particles := GPUParticles3D.new()
		particles.name = "Particles"
		particles.one_shot = true
		particles.emitting = false
		particles.amount = int(budgets.get("max_particles_per_slot", 96))
		particles.lifetime = 0.9
		particles.visibility_aabb = AABB(Vector3(-4.0, -4.0, -4.0), Vector3(8.0, 8.0, 8.0))
		root.add_child(particles)
		add_child(root)
		_vfx_slots.append({
			"root": root,
			"marker": marker,
			"particles": particles,
			"expires_at_ms": 0,
		})


func _build_fragment_pool() -> void:
	var budgets: Dictionary = _config.get("budgets", {})
	for index in range(int(budgets.get("fragment_pool_size", 0))):
		var fragment := MeshInstance3D.new()
		fragment.name = "Fragment%03d" % index
		fragment.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		fragment.visible = false
		var mesh := BoxMesh.new()
		mesh.size = Vector3(0.07, 0.04, 0.03)
		fragment.mesh = mesh
		add_child(fragment)
		_fragment_nodes.append(fragment)


func _build_profile_resources() -> void:
	for profile: String in (_config.get("profiles", {}) as Dictionary):
		var definition := _profile(profile)
		var color := profile_color(profile)
		var size := float(definition.get("size_m", 0.1))
		var marker_mesh := SphereMesh.new()
		marker_mesh.radius = size
		marker_mesh.height = size * 2.0
		marker_mesh.radial_segments = 12
		marker_mesh.rings = 6
		var marker_material := StandardMaterial3D.new()
		marker_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		marker_material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		marker_material.albedo_color = Color(color, 0.86)
		marker_material.emission_enabled = true
		marker_material.emission = color
		marker_material.emission_energy_multiplier = 2.4
		marker_mesh.material = marker_material
		var draw_mesh := SphereMesh.new()
		draw_mesh.radius = size * 0.22
		draw_mesh.height = draw_mesh.radius * 2.0
		draw_mesh.radial_segments = 6
		draw_mesh.rings = 3
		var draw_material := StandardMaterial3D.new()
		draw_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		draw_material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		draw_material.albedo_color = Color(color, 0.72)
		draw_material.emission_enabled = true
		draw_material.emission = color
		draw_material.emission_energy_multiplier = 1.8
		draw_mesh.material = draw_material
		var fragment_material := StandardMaterial3D.new()
		fragment_material.albedo_color = color
		_profile_resources[profile] = {
			"marker_mesh": marker_mesh,
			"process_normal": _create_particle_process(definition, color, false),
			"process_reduced": _create_particle_process(definition, color, true),
			"draw_mesh": draw_mesh,
			"fragment_material": fragment_material,
		}


func _create_particle_process(
		definition: Dictionary, color: Color, reduced: bool) -> ParticleProcessMaterial:
	var process_material := ParticleProcessMaterial.new()
	process_material.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_SPHERE
	process_material.emission_sphere_radius = float(definition.get("size_m", 0.1)) * 1.5
	process_material.direction = Vector3.UP
	process_material.spread = 180.0
	process_material.initial_velocity_min = 0.45 if reduced else 1.0
	process_material.initial_velocity_max = 0.9 if reduced else 2.2
	process_material.gravity = Vector3(0.0, -0.2, 0.0)
	process_material.scale_min = 0.45
	process_material.scale_max = 1.25
	process_material.color = Color(color, 0.78)
	return process_material


func _configure_marker(marker: MeshInstance3D, profile: String) -> void:
	marker.mesh = (_profile_resources[profile] as Dictionary).marker_mesh


func _configure_particles(
		particles: GPUParticles3D, profile: String, definition: Dictionary) -> void:
	var resources := _profile_resources[profile] as Dictionary
	var budgets := _config.get("budgets", {}) as Dictionary
	var requested := int(definition.get("particles", 24))
	particles.amount = mini(requested, int(budgets.get("max_particles_per_slot", 96)))
	particles.lifetime = float(definition.get("lifetime_s", 0.4))
	particles.process_material = resources.process_reduced if _reduced_motion \
		else resources.process_normal
	particles.draw_pass_1 = resources.draw_mesh


func _show_fragments(position: Vector3, profile: String, count: int) -> void:
	var fragment_material := (_profile_resources[profile] as Dictionary).fragment_material as Material
	var safe_count := mini(count, _fragment_nodes.size())
	for index in range(_fragment_nodes.size()):
		var fragment := _fragment_nodes[index]
		fragment.visible = index < safe_count
		if index >= safe_count:
			continue
		var angle := float(index) * TAU / float(maxi(1, safe_count))
		fragment.global_position = position + Vector3(cos(angle), 0.25, sin(angle)) * 0.18
		fragment.material_override = fragment_material


func _is_protected_anchor_hit(event: Dictionary, snapshots: Array) -> bool:
	var anchor_transform := Transform3D.IDENTITY
	for snapshot: Dictionary in snapshots:
		if int(snapshot.get("entity_id", 0)) == _anchor_entity_id():
			anchor_transform = snapshot.get("transform", Transform3D.IDENTITY)
			break
	var direction_values: Array = _directional_contract.local_protected_direction
	var local_front := Vector3(
		float(direction_values[0]), float(direction_values[1]), float(direction_values[2]))
	var world_front := (anchor_transform.basis * local_front).normalized()
	var cause_to_target: Vector3 = event.get("normal", Vector3.ZERO)
	var target_to_source := -cause_to_target.normalized()
	var cone_cosine := cos(deg_to_rad(float(_directional_contract.protected_cone_degrees)))
	return world_front.dot(target_to_source) >= cone_cosine


func _event_position(event: Dictionary, snapshots: Array) -> Vector3:
	var affected_id := int(event.get("affected_entity_id", 0))
	var preferred_id := affected_id if affected_id != 0 else int(event.get("entity_id", 0))
	if str(event.get("kind", "")) in SNAPSHOT_POSITION_KINDS:
		for snapshot: Dictionary in snapshots:
			if int(snapshot.get("entity_id", 0)) == preferred_id:
				var transform: Transform3D = snapshot.get("transform", Transform3D.IDENTITY)
				return transform.origin
	if event.has("position"):
		return event.position
	for snapshot: Dictionary in snapshots:
		if int(snapshot.get("entity_id", 0)) == preferred_id:
			var transform: Transform3D = snapshot.get("transform", Transform3D.IDENTITY)
			return transform.origin
	return _anchor_position(snapshots)


func _anchor_position(snapshots: Array) -> Vector3:
	for snapshot: Dictionary in snapshots:
		if int(snapshot.get("entity_id", 0)) == _anchor_entity_id():
			var transform: Transform3D = snapshot.get("transform", Transform3D.IDENTITY)
			return transform.origin
	return DEFAULT_POSITION


func _material_for_entity(entity_id: int, snapshots: Array) -> int:
	for snapshot: Dictionary in snapshots:
		if int(snapshot.get("entity_id", 0)) == entity_id:
			return int(snapshot.get("material_id", 0))
	return 0


func _measure_glass_screen_coverage(snapshots: Array) -> float:
	var camera := get_node_or_null("../OrbitalCamera") as Camera3D
	if camera == null:
		return 0.0
	var viewport_size := get_viewport().get_visible_rect().size
	var viewport_area := viewport_size.x * viewport_size.y
	if viewport_area <= 1.0:
		return 0.0
	var covered_area := 0.0
	for snapshot: Dictionary in snapshots:
		if int(snapshot.get("material_id", 0)) != 9:
			continue
		var shape: Dictionary = snapshot.get("shape", {})
		var half_extents: Vector3 = shape.get("half_extents", Vector3.ZERO)
		var transform: Transform3D = snapshot.get("transform", Transform3D.IDENTITY)
		var screen_min := Vector2(1000000.0, 1000000.0)
		var screen_max := Vector2(-1000000.0, -1000000.0)
		var visible_corner_count := 0
		for x_sign in [-1.0, 1.0]:
			for y_sign in [-1.0, 1.0]:
				for z_sign in [-1.0, 1.0]:
					var point := transform * Vector3(
						half_extents.x * x_sign,
						half_extents.y * y_sign,
						half_extents.z * z_sign)
					if camera.is_position_behind(point):
						continue
					var projected := camera.unproject_position(point)
					screen_min = screen_min.min(projected)
					screen_max = screen_max.max(projected)
					visible_corner_count += 1
		if visible_corner_count == 0:
			continue
		screen_min = screen_min.clamp(Vector2.ZERO, viewport_size)
		screen_max = screen_max.clamp(Vector2.ZERO, viewport_size)
		var size := (screen_max - screen_min).max(Vector2.ZERO)
		covered_area += size.x * size.y
	return clampf(covered_area / viewport_area, 0.0, 1.0)


func _screen_ratio_at(position: Vector3, radius_m: float) -> float:
	var camera := get_node_or_null("../OrbitalCamera") as Camera3D
	if camera == null or camera.is_position_behind(position):
		return 0.0
	var viewport_size := get_viewport().get_visible_rect().size
	var viewport_area := viewport_size.x * viewport_size.y
	if viewport_area <= 1.0:
		return 0.0
	var center := camera.unproject_position(position)
	var camera_right := camera.global_transform.basis.x.normalized()
	var edge := camera.unproject_position(position + camera_right * radius_m)
	var radius_pixels := center.distance_to(edge)
	return clampf(PI * radius_pixels * radius_pixels / viewport_area, 0.0, 1.0)


func _anchor_entity_id() -> int:
	return int((_config.get("directional_anchor", {}) as Dictionary).get("entity_id", 200))


func _audio_metrics() -> Dictionary:
	if _audio_pool != null and _audio_pool.has_method("pool_metrics"):
		return _audio_pool.pool_metrics()
	return {}


func _latency_p95_ms() -> float:
	if _impact_latency_samples_ms.is_empty():
		return -1.0
	var sorted := _impact_latency_samples_ms.duplicate()
	sorted.sort()
	var index := clampi(ceili(float(sorted.size()) * 0.95) - 1, 0, sorted.size() - 1)
	return sorted[index]


func _validate_runtime_budgets() -> void:
	var budgets := _config.get("budgets", {}) as Dictionary
	var particle_capacity := 0
	for slot: Dictionary in _vfx_slots:
		particle_capacity += (slot.particles as GPUParticles3D).amount
	var configured_particle_max := int(budgets.get("max_particles_total", 0))
	var configured_fragment_max := int(budgets.get("max_fragments_total", 0))
	var configured_voice_max := int(budgets.get("audio_voice_pool_size", 0))
	var voice_capacity := int(_audio_metrics().get("voice_capacity", 0))
	var valid := particle_capacity <= configured_particle_max \
		and configured_particle_max <= 30000 \
		and _fragment_nodes.size() <= configured_fragment_max \
		and configured_fragment_max <= 120 \
		and voice_capacity == configured_voice_max \
		and float(budgets.get("glass_screen_coverage_limit", 1.0)) <= 0.15
	var per_slot_max := int(budgets.get("max_particles_per_slot", 0))
	for definition: Dictionary in (_config.get("profiles", {}) as Dictionary).values():
		valid = valid and int(definition.get("particles", 0)) <= per_slot_max
	if not valid:
		_feedback_fault_count += 1
		push_error("feedback runtime pool budgets are invalid")


func _profile(profile: String) -> Dictionary:
	return (_config.get("profiles", {}) as Dictionary).get(profile, {})


func _load_config() -> Dictionary:
	var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(CONFIG_PATH))
	if parsed is Dictionary:
		return parsed
	push_error("feedback config could not be parsed: %s" % CONFIG_PATH)
	return {}


func _load_directional_contract() -> Dictionary:
	var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(ARCHETYPES_PATH))
	if not parsed is Dictionary:
		push_error("archetype config could not be parsed: %s" % ARCHETYPES_PATH)
		return {}
	var catalog := parsed as Dictionary
	var enemy_id := int((_config.get("directional_anchor", {}) as Dictionary).get(
		"enemy_archetype_id", 0))
	var weakpoint_id := 0
	for enemy: Dictionary in catalog.get("enemies", []):
		if int(enemy.get("id", 0)) == enemy_id:
			weakpoint_id = int(enemy.get("weakpoint_id", 0))
			break
	for weakpoint: Dictionary in catalog.get("weakpoints", []):
		if int(weakpoint.get("id", 0)) == weakpoint_id:
			return {
				"local_protected_direction": weakpoint.get("protected_direction", []),
				"protected_cone_degrees": float(weakpoint.get("protected_cone_deg", 0.0)),
			}
	push_error("Anchor weakpoint contract is unavailable")
	return {}


func _visible_marker_count() -> int:
	var count := 0
	for slot: Dictionary in _vfx_slots:
		if (slot.marker as MeshInstance3D).visible:
			count += 1
	return count


func _descendant_count(node: Node) -> int:
	var count := 0
	for child: Node in node.get_children():
		count += 1 + _descendant_count(child)
	return count
