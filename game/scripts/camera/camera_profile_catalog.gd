extends RefCounted

## Closed, fail-closed catalog of the product camera profiles.
##
## The profiles are content: the rigs never hardcode a field of view, a safe
## margin or a transition length. Composition bounds are not stored here; they
## come from the level manifest so a rig can never be pinned to one fortification.

const CONTENT_FILE_LOADER := preload("res://scripts/data/content_file_loader.gd")
const CATALOG_PATH := "res://data/camera/product_v2.camera.json"
const ROOT_KEYS := ["schema_version", "default_profile_id", "profiles"]
const PROFILE_KEYS := [
	"id", "rig", "fov_degrees", "transition_seconds",
	"reduced_motion_transition_seconds", "safe_margin_ratio",
	"impact_focus_seconds", "impact_fov_kick_degrees", "occlusion_policy",
	"distance_range_m", "inclination_range_deg", "orbit_sensitivity_deg",
	"zoom_step_m",
]
const PROFILE_ORDER := ["CAM_Carousel", "CAM_Farm", "CAM_Orbital"]
const RIG_BY_PROFILE := {
	"CAM_Carousel": "carousel",
	"CAM_Farm": "terrestrial",
	"CAM_Orbital": "orbital",
}
const FOV_BY_RIG := {"carousel": 35.0, "terrestrial": 44.0, "orbital": 48.0}
const OCCLUSION_BY_RIG := {
	"carousel": "none", "terrestrial": "none", "orbital": "radial_body",
}
const CAROUSEL_TRANSITION_SECONDS := 0.45
const REDUCED_MOTION_TRANSITION_SECONDS := 0.1
const SAFE_MARGIN_RATIO := 0.08
const MAXIMUM_IMPACT_FOCUS_SECONDS := 0.6
const MAXIMUM_IMPACT_KICK_DEGREES := 2.0


static func load_catalog() -> Dictionary:
	var result := CONTENT_FILE_LOADER.load_json(CATALOG_PATH)
	if not bool(result.get("ok", false)):
		return result
	var error := validate_catalog_document(result.get("document"))
	if not error.is_empty():
		return {"ok": false, "error_kind": "schema", "message": error}
	return result


static func profile_of(document: Dictionary, profile_id: String) -> Dictionary:
	for profile: Variant in document.get("profiles", []):
		if profile is Dictionary \
				and str((profile as Dictionary).get("id", "")) == profile_id:
			return (profile as Dictionary).duplicate(true)
	return {}


static func validate_catalog_document(document: Variant) -> String:
	if not document is Dictionary:
		return "$ must be an object"
	var value := document as Dictionary
	var error := _exact_keys_error(value, ROOT_KEYS, "$")
	if not error.is_empty():
		return error
	if not _is_integer(value.schema_version) or int(value.schema_version) != 2:
		return "$.schema_version must be integer 2"
	if typeof(value.default_profile_id) != TYPE_STRING \
			or not PROFILE_ORDER.has(str(value.default_profile_id)):
		return "$.default_profile_id must be a registered camera profile"
	if not value.profiles is Array \
			or (value.profiles as Array).size() != PROFILE_ORDER.size():
		return "$.profiles must contain every registered camera profile"
	for index: int in (value.profiles as Array).size():
		error = _profile_error(value.profiles[index], index)
		if not error.is_empty():
			return error
	return ""


static func _profile_error(candidate: Variant, index: int) -> String:
	var path := "$.profiles[%d]" % index
	if not candidate is Dictionary:
		return "%s must be an object" % path
	var profile := candidate as Dictionary
	var error := _exact_keys_error(profile, PROFILE_KEYS, path)
	if not error.is_empty():
		return error
	var profile_id := str(profile.id)
	if typeof(profile.id) != TYPE_STRING or profile_id != str(PROFILE_ORDER[index]):
		return "%s.id must preserve the canonical profile order" % path
	var rig := str(RIG_BY_PROFILE[profile_id])
	if typeof(profile.rig) != TYPE_STRING or str(profile.rig) != rig:
		return "%s.rig must be %s" % [path, rig]
	if not _is_number(profile.fov_degrees) \
			or not is_equal_approx(float(profile.fov_degrees), float(FOV_BY_RIG[rig])):
		return "%s.fov_degrees must be %s" % [path, FOV_BY_RIG[rig]]
	if typeof(profile.occlusion_policy) != TYPE_STRING \
			or str(profile.occlusion_policy) != str(OCCLUSION_BY_RIG[rig]):
		return "%s.occlusion_policy must be %s" % [path, OCCLUSION_BY_RIG[rig]]
	if not _is_number(profile.transition_seconds) \
			or float(profile.transition_seconds) <= 0.0 \
			or float(profile.transition_seconds) > CAROUSEL_TRANSITION_SECONDS:
		return "%s.transition_seconds must be positive and at most 0.45" % path
	if rig == "carousel" and not is_equal_approx(
			float(profile.transition_seconds), CAROUSEL_TRANSITION_SECONDS):
		return "%s.transition_seconds must be 0.45 for the carousel" % path
	if not _is_number(profile.reduced_motion_transition_seconds) \
			or not is_equal_approx(
				float(profile.reduced_motion_transition_seconds),
				REDUCED_MOTION_TRANSITION_SECONDS):
		return "%s.reduced_motion_transition_seconds must be 0.1" % path
	if not _is_number(profile.safe_margin_ratio) \
			or not is_equal_approx(float(profile.safe_margin_ratio), SAFE_MARGIN_RATIO):
		return "%s.safe_margin_ratio must be 0.08" % path
	if not _is_number(profile.impact_focus_seconds) \
			or float(profile.impact_focus_seconds) < 0.0 \
			or float(profile.impact_focus_seconds) > MAXIMUM_IMPACT_FOCUS_SECONDS:
		return "%s.impact_focus_seconds must be between 0 and 0.6" % path
	if not _is_number(profile.impact_fov_kick_degrees) \
			or float(profile.impact_fov_kick_degrees) < 0.0 \
			or float(profile.impact_fov_kick_degrees) > MAXIMUM_IMPACT_KICK_DEGREES:
		return "%s.impact_fov_kick_degrees must be between 0 and 2" % path
	if not _is_number(profile.zoom_step_m) or float(profile.zoom_step_m) < 0.0:
		return "%s.zoom_step_m must be a non-negative number" % path
	error = _ordered_pair_error(profile.distance_range_m, "%s.distance_range_m" % path, 0.5)
	if not error.is_empty():
		return error
	error = _ordered_pair_error(
		profile.inclination_range_deg, "%s.inclination_range_deg" % path, 0.0)
	if not error.is_empty():
		return error
	return _pair_error(profile.orbit_sensitivity_deg, "%s.orbit_sensitivity_deg" % path)


static func _ordered_pair_error(value: Variant, path: String, minimum: float) -> String:
	var error := _pair_error(value, path)
	if not error.is_empty():
		return error
	var pair := value as Array
	if float(pair[0]) < minimum or float(pair[1]) <= float(pair[0]):
		return "%s must be an increasing range" % path
	return ""


static func _pair_error(value: Variant, path: String) -> String:
	if not value is Array or (value as Array).size() != 2:
		return "%s must be a pair" % path
	for entry: Variant in value as Array:
		if not _is_number(entry) or float(entry) < 0.0:
			return "%s must contain non-negative numbers" % path
	return ""


static func _exact_keys_error(value: Dictionary, expected: Array, path: String) -> String:
	for key: Variant in value:
		if typeof(key) != TYPE_STRING or not expected.has(str(key)):
			return "%s has unknown key: %s" % [path, key]
	for key: String in expected:
		if not value.has(key):
			return "%s has missing key: %s" % [path, key]
	return ""


static func _is_number(value: Variant) -> bool:
	return typeof(value) in [TYPE_INT, TYPE_FLOAT] and is_finite(float(value))


static func _is_integer(value: Variant) -> bool:
	if typeof(value) == TYPE_INT:
		return true
	if typeof(value) != TYPE_FLOAT:
		return false
	return is_finite(float(value)) and float(value) == floor(float(value))
