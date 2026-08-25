extends RefCounted

## Builds the single product Theme used by the HUD, the pause overlay and the
## result screen.
##
## Every colour pair shipped here is declared in text_pairs()/icon_pairs() and
## verified against the WCAG thresholds of the specification by
## game/tests/ui_accessibility_smoke.gd. Product screens consume this theme and
## containers; none of them positions a control by fixed offsets.

const SURFACE := Color("0b1522")
const PANEL := Color("132033")
const PANEL_BORDER := Color("2b4160")
const BUTTON_BACKGROUND := Color("1d2f47")
const BUTTON_HOVER := Color("2b4a6d")
const PRIMARY_TEXT := Color("f2f7fb")
const SECONDARY_TEXT := Color("ffd166")
const MUTED_TEXT := Color("9fb4c7")
const POSITIVE_TEXT := Color("7ee081")
const NEGATIVE_TEXT := Color("ff8b7b")
const FOCUS_RING := Color("ffd166")
const TRAJECTORY_DOT := Color("ffd166")
const TRAJECTORY_SHADOW := Color("0b1522")
const TRAJECTORY_IMPACT := Color("ff8b7b")

const BODY_FONT_SIZE := 18
const SMALL_FONT_SIZE := 16
const TITLE_FONT_SIZE := 34
const HEADLINE_FONT_SIZE := 26

const NORMAL_TEXT_MINIMUM := 4.5
const LARGE_TEXT_MINIMUM := 3.0

const CONTENT_MARGIN := 16
const PANEL_CORNER_RADIUS := 6


static func build() -> Theme:
	var theme := Theme.new()
	theme.default_font_size = BODY_FONT_SIZE

	theme.set_stylebox("panel", "PanelContainer", _panel_box())
	theme.set_stylebox("panel", "Panel", _panel_box())

	theme.set_color("font_color", "Label", PRIMARY_TEXT)
	theme.set_font_size("font_size", "Label", BODY_FONT_SIZE)

	theme.set_color("font_color", "Button", PRIMARY_TEXT)
	theme.set_color("font_hover_color", "Button", SECONDARY_TEXT)
	theme.set_color("font_focus_color", "Button", SECONDARY_TEXT)
	theme.set_color("font_pressed_color", "Button", SECONDARY_TEXT)
	theme.set_color("font_disabled_color", "Button", MUTED_TEXT)
	theme.set_font_size("font_size", "Button", BODY_FONT_SIZE)
	theme.set_stylebox("normal", "Button", _button_box(BUTTON_BACKGROUND, PANEL_BORDER, 1))
	theme.set_stylebox("hover", "Button", _button_box(BUTTON_HOVER, FOCUS_RING, 2))
	theme.set_stylebox("pressed", "Button", _button_box(BUTTON_HOVER, FOCUS_RING, 2))
	theme.set_stylebox("focus", "Button", _button_box(BUTTON_HOVER, FOCUS_RING, 3))
	theme.set_stylebox("disabled", "Button", _button_box(PANEL, PANEL_BORDER, 1))

	theme.set_constant("separation", "VBoxContainer", 8)
	theme.set_constant("separation", "HBoxContainer", 12)
	for margin: String in ["margin_left", "margin_right", "margin_top", "margin_bottom"]:
		theme.set_constant(margin, "MarginContainer", CONTENT_MARGIN)

	_register_label_variation(theme, "ScreenTitle", TITLE_FONT_SIZE, SECONDARY_TEXT)
	_register_label_variation(theme, "Headline", HEADLINE_FONT_SIZE, PRIMARY_TEXT)
	_register_label_variation(theme, "Victory", HEADLINE_FONT_SIZE, POSITIVE_TEXT)
	_register_label_variation(theme, "Defeat", HEADLINE_FONT_SIZE, NEGATIVE_TEXT)
	_register_label_variation(theme, "Value", BODY_FONT_SIZE, SECONDARY_TEXT)
	_register_label_variation(theme, "Muted", SMALL_FONT_SIZE, MUTED_TEXT)
	_register_label_variation(theme, "Record", BODY_FONT_SIZE, POSITIVE_TEXT)
	return theme


## Foreground/background pairs that carry readable copy. Normal-sized text must
## reach 4,5:1 and large text 3:1; the smoke asserts both without exception.
static func text_pairs() -> Array:
	return [
		{"id": "label_on_panel", "foreground": PRIMARY_TEXT, "background": PANEL,
			"minimum": NORMAL_TEXT_MINIMUM},
		{"id": "label_on_surface", "foreground": PRIMARY_TEXT, "background": SURFACE,
			"minimum": NORMAL_TEXT_MINIMUM},
		{"id": "value_on_panel", "foreground": SECONDARY_TEXT, "background": PANEL,
			"minimum": NORMAL_TEXT_MINIMUM},
		{"id": "muted_on_panel", "foreground": MUTED_TEXT, "background": PANEL,
			"minimum": NORMAL_TEXT_MINIMUM},
		{"id": "victory_on_panel", "foreground": POSITIVE_TEXT, "background": PANEL,
			"minimum": LARGE_TEXT_MINIMUM},
		{"id": "defeat_on_panel", "foreground": NEGATIVE_TEXT, "background": PANEL,
			"minimum": LARGE_TEXT_MINIMUM},
		{"id": "title_on_surface", "foreground": SECONDARY_TEXT, "background": SURFACE,
			"minimum": LARGE_TEXT_MINIMUM},
		{"id": "button_label", "foreground": PRIMARY_TEXT, "background": BUTTON_BACKGROUND,
			"minimum": NORMAL_TEXT_MINIMUM},
		{"id": "button_label_hover", "foreground": SECONDARY_TEXT,
			"background": BUTTON_HOVER, "minimum": NORMAL_TEXT_MINIMUM},
	]


## Non-text carriers: focus ring and the material swatches. The swatch never
## carries meaning alone; the smoke also asserts that each one ships a glyph
## and a localized name.
static func icon_pairs() -> Array:
	var pairs: Array = [
		{"id": "focus_ring", "foreground": FOCUS_RING, "background": BUTTON_HOVER,
			"minimum": LARGE_TEXT_MINIMUM},
		{"id": "panel_border", "foreground": PANEL_BORDER, "background": SURFACE,
			"minimum": 1.2},
	]
	return pairs


static func relative_luminance(value: Color) -> float:
	return 0.2126 * _linear_channel(value.r) \
		+ 0.7152 * _linear_channel(value.g) \
		+ 0.0722 * _linear_channel(value.b)


static func contrast_ratio(foreground: Color, background: Color) -> float:
	var first := relative_luminance(foreground)
	var second := relative_luminance(background)
	var brighter := maxf(first, second)
	var darker := minf(first, second)
	return (brighter + 0.05) / (darker + 0.05)


static func _linear_channel(channel: float) -> float:
	var value := clampf(channel, 0.0, 1.0)
	if value <= 0.04045:
		return value / 12.92
	return pow((value + 0.055) / 1.055, 2.4)


static func _register_label_variation(
		theme: Theme, variation: String, font_size: int, color: Color) -> void:
	theme.add_type(variation)
	theme.set_type_variation(variation, "Label")
	theme.set_font_size("font_size", variation, font_size)
	theme.set_color("font_color", variation, color)


static func _panel_box() -> StyleBoxFlat:
	var box := StyleBoxFlat.new()
	box.bg_color = PANEL
	box.border_color = PANEL_BORDER
	box.set_border_width_all(1)
	box.set_corner_radius_all(PANEL_CORNER_RADIUS)
	box.content_margin_left = CONTENT_MARGIN
	box.content_margin_right = CONTENT_MARGIN
	box.content_margin_top = 12
	box.content_margin_bottom = 12
	return box


static func _button_box(background: Color, border: Color, border_width: int) -> StyleBoxFlat:
	var box := StyleBoxFlat.new()
	box.bg_color = background
	box.border_color = border
	box.set_border_width_all(border_width)
	box.set_corner_radius_all(PANEL_CORNER_RADIUS)
	box.content_margin_left = CONTENT_MARGIN
	box.content_margin_right = CONTENT_MARGIN
	box.content_margin_top = 10
	box.content_margin_bottom = 10
	return box
