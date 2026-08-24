extends SceneTree

const WORLD_CATALOG := preload("res://scripts/data/world_catalog.gd")
const PRODUCT_TEXT := preload("res://scripts/data/product_text_catalog.gd")
const CAROUSEL_SCENE := "res://scenes/frontend/world_carousel.tscn"
const LEVEL_SCENE := "res://scenes/frontend/level_select.tscn"
const MARKER := "WORLD_CAROUSEL_SMOKE_OK"

var _errors: Array[String] = []


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	var loaded := WORLD_CATALOG.load_catalog()
	var text_result := PRODUCT_TEXT.load_catalog("res://data/ui/product_v2.pt-BR.json")
	if not bool(loaded.get("ok", false)) or not bool(text_result.get("ok", false)):
		_fail("registered content must load before exercising the carousel")
		return
	var catalog := loaded.document as Dictionary
	var messages := (text_result.document as Dictionary).messages as Dictionary
	var progress := {
		"last_world_id": "earth",
		"levels": {
			"earth/farm_reaction": {"best_score": 43210, "best_stars": 3},
			"orbital/first_orbit_v2": {"best_score": 12000, "best_stars": 2},
		},
	}
	var packed := load(CAROUSEL_SCENE) as PackedScene
	var carousel := packed.instantiate() as Control
	root.add_child(carousel)
	await process_frame
	_check(carousel.has_method("set_messages"),
		"dynamic carousel content must receive the closed localized catalog")
	if carousel.has_method("set_messages"):
		carousel.set_messages(messages)
	carousel.configure(catalog, progress)
	for offset: int in [1, -1, 1, -1, 1]:
		carousel.select_offset(offset)
		_check(_actual_card_count(carousel) <= 3,
			"the live tree must never retain more than three card Buttons during rebuild")
		_check(_actual_rig_count(carousel) <= 2,
			"the live tree must never retain more than two diorama rigs during rebuild")
	await process_frame
	_check(_actual_card_count(carousel) <= 3 and _actual_rig_count(carousel) <= 2,
		"card/rig budgets must remain true after the deletion frame")
	carousel.select_offset(-1)
	await process_frame
	_check(carousel.current_world_id() == "earth", "Earth must remain index zero/default")
	var cards := carousel.find_child("CardHost", true, false).get_children()
	var earth_card: Button
	var orbital_neighbor: Button
	for child: Node in cards:
		if child is Button and str(child.get_meta("world_id", "")) == "earth":
			earth_card = child as Button
		elif child is Button and str(child.get_meta("world_id", "")) == "orbital":
			orbital_neighbor = child as Button
	_check(earth_card != null, "live cards must expose their registered world_id")
	_check(orbital_neighbor != null,
		"Earth-centered carousel must expose a real Orbital neighbor card")
	if earth_card != null:
		_check(earth_card.text == "TERRA — MELHOR: 43210 PONTOS • 3 ESTRELAS",
			"normal lookup must resolve world text_id and the localized closed record format")
	if orbital_neighbor != null:
		orbital_neighbor.mouse_entered.emit()
		_check(root.get_viewport().gui_get_focus_owner() == orbital_neighbor,
			"dynamically created cards must acquire visible focus on hover")
		orbital_neighbor.pressed.emit()
		await process_frame
		var rebuilt_focus := root.get_viewport().gui_get_focus_owner() as Control
		_check(carousel.current_world_id() == "orbital",
			"pressing the real neighbor card must center Orbital without signal-time errors")
		_check(_actual_card_count(carousel) <= 3 and _actual_rig_count(carousel) <= 2,
			"neighbor activation must keep the live tree within card and rig budgets")
		_check(rebuilt_focus != null and rebuilt_focus.is_visible_in_tree() \
				and str(rebuilt_focus.get_meta("world_id", "")) == "orbital",
			"card activation/rebuild must restore visible focus to the centered card")
	var rigs := carousel.find_child("RigHost", true, false).get_children()
	_check(rigs.size() == 2, "two-world carousel must expose exactly two placeholder rigs")
	for rig: Node in rigs:
		var world_id := str(rig.get_meta("world_id", ""))
		_check(world_id in ["earth", "orbital"],
			"placeholder rigs must expose their registered world_id")
		var expected_diorama := "WRD_EarthFarm_Diorama" if world_id == "earth" \
			else "WRD_Aster_Diorama" if world_id == "orbital" else "__invalid__"
		_check(rig.name == StringName("DioramaRig_%s" % world_id),
			"placeholder rigs must have stable registered names")
		_check(str(rig.get_meta("diorama_id", "")) == expected_diorama,
			"placeholder rigs must expose their registered final diorama_id")
		var expected_lod := "center_high" if world_id == "orbital" else "neighbor_lod1"
		_check(str(rig.get_meta("lod", "")) == expected_lod,
			"placeholder rigs must distinguish center high LOD from neighbor LOD1")

	var level_packed := load(LEVEL_SCENE) as PackedScene
	var level_screen := level_packed.instantiate() as Control
	root.add_child(level_screen)
	await process_frame
	_check(level_screen.has_method("set_messages"),
		"dynamic phase controls must receive the closed localized catalog")
	if level_screen.has_method("set_messages"):
		level_screen.set_messages(messages)
	level_screen.configure(catalog.worlds[0], progress)
	await process_frame
	var phase_buttons := level_screen.find_child("LevelHost", true, false).get_children()
	_check(phase_buttons.size() == 1 \
			and (phase_buttons[0] as Button).text == "Fazenda — Reação em Cadeia",
		"normal phase lookup must resolve the closed localized phase token")
	level_screen.configure(catalog.worlds[1], progress)
	await process_frame
	phase_buttons = level_screen.find_child("LevelHost", true, false).get_children()
	_check(phase_buttons.size() == 1 and (phase_buttons[0] as Button).text \
			== "Primeira Órbita — Contrapeso de Aster",
		"Orbital phase lookup must resolve its closed frontend registry token")

	var fallback_carousel := packed.instantiate() as Control
	root.add_child(fallback_carousel)
	await process_frame
	var missing_messages := messages.duplicate(true)
	missing_messages.erase("TXT_WORLD_EARTH")
	if fallback_carousel.has_method("set_messages"):
		fallback_carousel.set_messages(missing_messages)
	fallback_carousel.configure(catalog, progress)
	await process_frame
	var fallback_text := ""
	for child: Node in fallback_carousel.find_child("CardHost", true, false).get_children():
		if child is Button and str(child.get_meta("world_id", "")) == "earth":
			fallback_text = (child as Button).text
	_check(fallback_text.begins_with("TXT_WORLD_EARTH"),
		"only a missing lookup may fall back to the safe technical text ID")

	if not _errors.is_empty():
		_fail(" | ".join(_errors))
		return
	print(MARKER)
	quit(0)


func _actual_card_count(carousel: Control) -> int:
	var count := 0
	for child: Node in carousel.find_child("CardHost", true, false).get_children():
		if child is Button:
			count += 1
	return count


func _actual_rig_count(carousel: Control) -> int:
	return carousel.find_child("RigHost", true, false).get_child_count()


func _check(condition: bool, message: String) -> void:
	if not condition:
		_errors.append(message)


func _fail(message: String) -> void:
	push_error("world carousel smoke: %s" % message)
	quit(1)
