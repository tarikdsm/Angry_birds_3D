extends RefCounted

## Presentation view of the persisted accessibility options.
##
## The persisted document stays owned by settings_model.gd; this helper only
## normalizes the fields the product presentation consumes and publishes the
## glyph/text badges that keep material, bird and ability state readable without
## relying on colour alone.

const THEME_FACTORY := preload("res://scripts/ui/theme_factory.gd")

const FIELDS := ["reduced_motion", "shake", "trajectory_assist", "ui_scale_percent"]

const BIRD_BADGES := {
	"virela": {"glyph": "[VI]", "message_id": "hud.bird.virela"},
	"red": {"glyph": "[VM]", "message_id": "hud.bird.red"},
	"yellow": {"glyph": "[AM]", "message_id": "hud.bird.yellow"},
	"blue": {"glyph": "[AZ]", "message_id": "hud.bird.blue"},
	"black": {"glyph": "[PR]", "message_id": "hud.bird.black"},
}
const ABILITY_BADGES := {
	"gravity_field": {"glyph": "[G]", "message_id": "hud.ability.gravity_field"},
	"mass_boost": {"glyph": "[M]", "message_id": "hud.ability.mass_boost"},
	"speed_boost": {"glyph": "[V]", "message_id": "hud.ability.speed_boost"},
	"split": {"glyph": "[D]", "message_id": "hud.ability.split"},
	"explosion": {"glyph": "[X]", "message_id": "hud.ability.explosion"},
}
const MATERIAL_BADGES := {
	"pine": {"glyph": "[MD]", "message_id": "hud.material.pine", "color": "c9a26b"},
	"brick": {"glyph": "[TJ]", "message_id": "hud.material.brick", "color": "d8825f"},
	"glass": {"glyph": "[VD]", "message_id": "hud.material.glass", "color": "7fd8e8"},
	"pressed_straw": {"glyph": "[PL]", "message_id": "hud.material.pressed_straw",
		"color": "e3cf7a"},
	"sheet_steel": {"glyph": "[AC]", "message_id": "hud.material.sheet_steel",
		"color": "b8c4d2"},
}
const READINESS_MESSAGE_IDS := {
	"unavailable": "hud.state.waiting",
	"arming": "hud.state.loading",
	"armed": "hud.state.ready",
	"active": "hud.state.active",
	"spent": "hud.state.spent",
}


static func defaults() -> Dictionary:
	return {
		"reduced_motion": false,
		"shake": true,
		"trajectory_assist": false,
		"ui_scale_percent": 100,
	}


## Reads the persisted settings document without ever widening it. Unknown or
## malformed fields fall back to the safe default instead of failing the run:
## the persisted document itself is validated by settings_model.gd.
static func from_document(document: Variant) -> Dictionary:
	var value := defaults()
	if not document is Dictionary:
		return value
	var source := document as Dictionary
	for field: String in ["reduced_motion", "shake", "trajectory_assist"]:
		if typeof(source.get(field)) == TYPE_BOOL:
			value[field] = bool(source[field])
	var scale: Variant = source.get("ui_scale_percent")
	if typeof(scale) == TYPE_INT or typeof(scale) == TYPE_FLOAT:
		var percent := int(scale)
		if percent in [100, 125, 150, 200]:
			value.ui_scale_percent = percent
	return value


static func bird_badge(key: String) -> Dictionary:
	return (BIRD_BADGES.get(key, {}) as Dictionary).duplicate(true)


static func ability_badge(kind: String) -> Dictionary:
	return (ABILITY_BADGES.get(kind, {}) as Dictionary).duplicate(true)


static func material_badge(key: String) -> Dictionary:
	return (MATERIAL_BADGES.get(key, {}) as Dictionary).duplicate(true)


static func readiness_message_id(readiness: String) -> String:
	return str(READINESS_MESSAGE_IDS.get(readiness, "hud.state.waiting"))


## Every badge must survive a colour-blind reading: a glyph and a localized
## name, and a swatch that still separates from the panel by at least 3:1.
static func badge_contrast_failures() -> Array[String]:
	var failures: Array[String] = []
	for key: Variant in MATERIAL_BADGES:
		var badge := MATERIAL_BADGES[key] as Dictionary
		var swatch := Color(str(badge.color))
		if THEME_FACTORY.contrast_ratio(swatch, THEME_FACTORY.PANEL) \
				< THEME_FACTORY.LARGE_TEXT_MINIMUM:
			failures.append("material swatch below 3:1: %s" % key)
		if str(badge.glyph).is_empty() or str(badge.message_id).is_empty():
			failures.append("material badge is colour-only: %s" % key)
	for maps: Dictionary in [BIRD_BADGES, ABILITY_BADGES]:
		for key: Variant in maps:
			var badge := maps[key] as Dictionary
			if str(badge.get("glyph", "")).is_empty() \
					or str(badge.get("message_id", "")).is_empty():
				failures.append("badge is colour-only: %s" % key)
	return failures
