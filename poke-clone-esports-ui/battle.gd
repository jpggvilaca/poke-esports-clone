extends Control

const FIELD_COLOR := Color(0.56, 0.72, 0.55)
const PANEL_COLOR := Color(0.94, 0.91, 0.78)
const PANEL_BORDER := Color(0.20, 0.20, 0.18)
const PLAYER_COLOR := Color(0.24, 0.43, 0.82)
const OPPONENT_COLOR := Color(0.78, 0.28, 0.24)
const LOG_LINES := 5
const PLAYER_AVATAR_PATH := "res://assets/players/avatars/portrait1.png"
const OPPONENT_AVATAR_PATH := "res://assets/players/avatars/portrait2.png"

var bridge: BattleBridge
var opponent_name: Label
var opponent_meta: Label
var opponent_hp: ProgressBar
var opponent_hp_value: Label
var opponent_focus: ProgressBar
var opponent_focus_value: Label
var player_name: Label
var player_meta: Label
var player_hp: ProgressBar
var player_hp_value: Label
var player_focus: ProgressBar
var player_focus_value: Label
var message_label: Label
var log_label: Label
var main_menu_grid: GridContainer
var skill_grid: GridContainer
var switch_label: Label
var switch_grid: GridContainer
var item_label: Label
var item_list: VBoxContainer
var back_button: Button
var return_button: Button
var main_menu_buttons: Array[Button] = []
var skill_buttons: Array[Button] = []
var switch_buttons: Array[Button] = []
var item_buttons: Array[Button] = []
var log_entries: Array[String] = []
var deferred_skill_xp: Dictionary = {}
var deferred_skill_levels: Dictionary = {}
var input_locked := false
var finished_result: Dictionary = {}


func _ready() -> void:
	_build_ui()
	bridge = BattleBridge.new()
	add_child(bridge)

	var result: Dictionary
	if GameState.has_pending_battle():
		result = bridge.start_battle(GameState.build_battle_setup())
	else:
		result = bridge.start_demo_battle()

	if not result.get("accepted", false):
		finished_result = result
		_update_state(result.get("state", bridge.get_battle_state()))
		_set_message(result.get("error", "Battle could not start."))
		_show_return_button()
		return

	var state: Dictionary = result.get("state", bridge.get_battle_state())
	_update_state(state)
	_refresh_skills()
	_refresh_team_switches(state)
	_show_main_menu()
	await _play_events(result.get("events", []))
	_set_message("What will your player do?")


func _build_ui() -> void:
	var background := ColorRect.new()
	background.color = FIELD_COLOR
	background.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(background)

	var opponent_platform := ColorRect.new()
	opponent_platform.color = Color(0.78, 0.84, 0.62)
	opponent_platform.anchor_left = 0.58
	opponent_platform.anchor_top = 0.19
	opponent_platform.anchor_right = 0.91
	opponent_platform.anchor_bottom = 0.31
	add_child(opponent_platform)

	var player_platform := ColorRect.new()
	player_platform.color = Color(0.68, 0.80, 0.58)
	player_platform.anchor_left = 0.09
	player_platform.anchor_top = 0.57
	player_platform.anchor_right = 0.42
	player_platform.anchor_bottom = 0.70
	add_child(player_platform)

	var opponent_sprite := _make_avatar_block(OPPONENT_AVATAR_PATH, OPPONENT_COLOR, "OPPONENT")
	opponent_sprite.anchor_left = 0.68
	opponent_sprite.anchor_top = 0.08
	opponent_sprite.anchor_right = 0.82
	opponent_sprite.anchor_bottom = 0.25
	add_child(opponent_sprite)

	var player_sprite := _make_avatar_block(PLAYER_AVATAR_PATH, PLAYER_COLOR, "PLAYER")
	player_sprite.anchor_left = 0.18
	player_sprite.anchor_top = 0.41
	player_sprite.anchor_right = 0.32
	player_sprite.anchor_bottom = 0.61
	add_child(player_sprite)

	var opponent_panel := _make_status_panel()
	opponent_panel.anchor_left = 0.07
	opponent_panel.anchor_top = 0.08
	opponent_panel.anchor_right = 0.43
	opponent_panel.anchor_bottom = 0.25
	add_child(opponent_panel)
	opponent_name = opponent_panel.get_node("Content/Name")
	opponent_meta = opponent_panel.get_node("Content/Meta")
	opponent_hp = opponent_panel.get_node("Content/HpRow/Hp")
	opponent_hp_value = opponent_panel.get_node("Content/HpRow/Value")
	opponent_focus = opponent_panel.get_node("Content/FocusRow/Focus")
	opponent_focus_value = opponent_panel.get_node("Content/FocusRow/Value")

	var player_panel := _make_status_panel()
	player_panel.anchor_left = 0.57
	player_panel.anchor_top = 0.45
	player_panel.anchor_right = 0.93
	player_panel.anchor_bottom = 0.63
	add_child(player_panel)
	player_name = player_panel.get_node("Content/Name")
	player_meta = player_panel.get_node("Content/Meta")
	player_hp = player_panel.get_node("Content/HpRow/Hp")
	player_hp_value = player_panel.get_node("Content/HpRow/Value")
	player_focus = player_panel.get_node("Content/FocusRow/Focus")
	player_focus_value = player_panel.get_node("Content/FocusRow/Value")

	var bottom := PanelContainer.new()
	bottom.anchor_left = 0.035
	bottom.anchor_top = 0.74
	bottom.anchor_right = 0.965
	bottom.anchor_bottom = 0.965
	bottom.add_theme_stylebox_override("panel", _panel_style(Color(0.93, 0.90, 0.76), PANEL_BORDER, 10))
	add_child(bottom)

	var bottom_split := HBoxContainer.new()
	bottom_split.add_theme_constant_override("separation", 22)
	bottom.add_child(bottom_split)

	var text_box := VBoxContainer.new()
	text_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	text_box.size_flags_vertical = Control.SIZE_EXPAND_FILL
	text_box.add_theme_constant_override("separation", 12)
	bottom_split.add_child(text_box)

	message_label = Label.new()
	message_label.text = ""
	message_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	message_label.add_theme_font_size_override("font_size", 34)
	message_label.size_flags_vertical = Control.SIZE_EXPAND_FILL
	text_box.add_child(message_label)

	log_label = Label.new()
	log_label.text = ""
	log_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	log_label.add_theme_font_size_override("font_size", 20)
	log_label.modulate = Color(0.22, 0.22, 0.20)
	text_box.add_child(log_label)

	var action_box := VBoxContainer.new()
	action_box.custom_minimum_size = Vector2(560, 0)
	action_box.add_theme_constant_override("separation", 10)
	bottom_split.add_child(action_box)

	main_menu_grid = GridContainer.new()
	main_menu_grid.columns = 2
	main_menu_grid.add_theme_constant_override("h_separation", 10)
	main_menu_grid.add_theme_constant_override("v_separation", 10)
	action_box.add_child(main_menu_grid)
	_add_main_menu_button("Fight", _on_fight_menu_pressed)
	_add_main_menu_button("Player list", _on_player_list_menu_pressed)
	_add_main_menu_button("Items", _on_items_menu_pressed)
	_add_main_menu_button("Quit", _on_quit_pressed)

	skill_grid = GridContainer.new()
	skill_grid.columns = 2
	skill_grid.add_theme_constant_override("h_separation", 10)
	skill_grid.add_theme_constant_override("v_separation", 10)
	action_box.add_child(skill_grid)

	switch_label = Label.new()
	switch_label.text = "Switch Player"
	switch_label.add_theme_font_size_override("font_size", 18)
	action_box.add_child(switch_label)

	switch_grid = GridContainer.new()
	switch_grid.columns = 2
	switch_grid.add_theme_constant_override("h_separation", 10)
	switch_grid.add_theme_constant_override("v_separation", 10)
	action_box.add_child(switch_grid)

	item_label = Label.new()
	item_label.text = "Items"
	item_label.add_theme_font_size_override("font_size", 18)
	action_box.add_child(item_label)

	item_list = VBoxContainer.new()
	item_list.add_theme_constant_override("separation", 10)
	action_box.add_child(item_list)

	back_button = Button.new()
	back_button.text = "Back"
	back_button.custom_minimum_size = Vector2(0, 52)
	back_button.pressed.connect(_show_main_menu)
	action_box.add_child(back_button)

	return_button = Button.new()
	return_button.text = "Return to map"
	return_button.custom_minimum_size = Vector2(0, 76)
	return_button.visible = false
	return_button.pressed.connect(_on_return_pressed)
	action_box.add_child(return_button)


func _make_status_panel() -> PanelContainer:
	var panel := PanelContainer.new()
	panel.add_theme_stylebox_override("panel", _panel_style(PANEL_COLOR, PANEL_BORDER, 8))

	var content := VBoxContainer.new()
	content.name = "Content"
	content.add_theme_constant_override("separation", 5)
	panel.add_child(content)

	var name_label := Label.new()
	name_label.name = "Name"
	name_label.add_theme_font_size_override("font_size", 26)
	content.add_child(name_label)

	var meta_label := Label.new()
	meta_label.name = "Meta"
	meta_label.add_theme_font_size_override("font_size", 18)
	meta_label.modulate = Color(0.25, 0.25, 0.23)
	content.add_child(meta_label)

	var hp_row := HBoxContainer.new()
	hp_row.name = "HpRow"
	hp_row.add_theme_constant_override("separation", 8)
	content.add_child(hp_row)

	var hp_label := Label.new()
	hp_label.text = "HP"
	hp_label.custom_minimum_size = Vector2(58, 0)
	hp_label.add_theme_font_size_override("font_size", 16)
	hp_row.add_child(hp_label)

	var hp_bar := ProgressBar.new()
	hp_bar.name = "Hp"
	hp_bar.show_percentage = false
	hp_bar.custom_minimum_size = Vector2(0, 24)
	hp_bar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hp_row.add_child(hp_bar)

	var hp_value := Label.new()
	hp_value.name = "Value"
	hp_value.custom_minimum_size = Vector2(88, 0)
	hp_value.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	hp_value.add_theme_font_size_override("font_size", 16)
	hp_row.add_child(hp_value)

	var focus_row := HBoxContainer.new()
	focus_row.name = "FocusRow"
	focus_row.add_theme_constant_override("separation", 8)
	content.add_child(focus_row)

	var focus_label := Label.new()
	focus_label.text = "Focus"
	focus_label.custom_minimum_size = Vector2(58, 0)
	focus_label.add_theme_font_size_override("font_size", 16)
	focus_row.add_child(focus_label)

	var focus_bar := ProgressBar.new()
	focus_bar.name = "Focus"
	focus_bar.show_percentage = false
	focus_bar.custom_minimum_size = Vector2(0, 18)
	focus_bar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	focus_row.add_child(focus_bar)

	var focus_value := Label.new()
	focus_value.name = "Value"
	focus_value.custom_minimum_size = Vector2(88, 0)
	focus_value.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	focus_value.add_theme_font_size_override("font_size", 16)
	focus_row.add_child(focus_value)
	return panel


func _make_avatar_block(texture_path: String, color: Color, label_text: String) -> PanelContainer:
	var panel := PanelContainer.new()
	panel.add_theme_stylebox_override("panel", _panel_style(color, Color(0.12, 0.12, 0.12), 12))
	panel.custom_minimum_size = Vector2(220, 220)

	var texture := load(texture_path)
	if texture is Texture2D:
		var avatar := TextureRect.new()
		avatar.texture = texture
		avatar.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		avatar.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		avatar.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		avatar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		avatar.size_flags_vertical = Control.SIZE_EXPAND_FILL
		panel.add_child(avatar)
	else:
		var label := Label.new()
		label.text = label_text
		label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
		label.add_theme_font_size_override("font_size", 24)
		label.add_theme_color_override("font_color", Color.WHITE)
		panel.add_child(label)
	return panel


func _panel_style(color: Color, border: Color, radius: int) -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = color
	style.border_color = border
	style.set_border_width_all(4)
	style.set_corner_radius_all(radius)
	style.content_margin_left = 18
	style.content_margin_right = 18
	style.content_margin_top = 12
	style.content_margin_bottom = 12
	return style


func _add_main_menu_button(text: String, callback: Callable) -> void:
	var button := Button.new()
	button.text = text
	button.custom_minimum_size = Vector2(270, 76)
	button.pressed.connect(callback)
	main_menu_grid.add_child(button)
	main_menu_buttons.push_back(button)


func _show_main_menu() -> void:
	main_menu_grid.visible = true
	skill_grid.visible = false
	switch_label.visible = false
	switch_grid.visible = false
	item_label.visible = false
	item_list.visible = false
	back_button.visible = false
	return_button.visible = false
	_set_buttons_disabled(input_locked)


func _show_fight_menu() -> void:
	_refresh_skills()
	main_menu_grid.visible = false
	skill_grid.visible = true
	switch_label.visible = false
	switch_grid.visible = false
	item_label.visible = false
	item_list.visible = false
	back_button.visible = true


func _show_player_list_menu() -> void:
	_refresh_team_switches()
	main_menu_grid.visible = false
	skill_grid.visible = false
	switch_label.visible = true
	switch_grid.visible = true
	item_label.visible = false
	item_list.visible = false
	back_button.visible = true


func _show_items_menu() -> void:
	_refresh_items()
	main_menu_grid.visible = false
	skill_grid.visible = false
	switch_label.visible = false
	switch_grid.visible = false
	item_label.visible = true
	item_list.visible = true
	back_button.visible = true


func _on_fight_menu_pressed() -> void:
	if input_locked:
		return
	_show_fight_menu()


func _on_player_list_menu_pressed() -> void:
	if input_locked:
		return
	_show_player_list_menu()


func _on_items_menu_pressed() -> void:
	if input_locked:
		return
	_show_items_menu()


func _refresh_skills() -> void:
	for child in skill_grid.get_children():
		child.queue_free()
	skill_buttons.clear()

	for skill in bridge.get_available_skills():
		var button := Button.new()
		button.text = "%s\nPWR %s / FOCUS %s" % [skill.get("name", "Skill"), skill.get("power", 0), skill.get("focus_cost", 0)]
		button.custom_minimum_size = Vector2(270, 86)
		button.disabled = input_locked
		button.pressed.connect(_on_skill_pressed.bind(skill.get("id", "")))
		skill_grid.add_child(button)
		skill_buttons.push_back(button)


func _refresh_items() -> void:
	for child in item_list.get_children():
		child.queue_free()
	item_buttons.clear()

	var state := GameState.get_trainer_state()
	var items: Array = state.get("items", [])
	if items.is_empty():
		var empty_label := Label.new()
		empty_label.text = "No items."
		empty_label.add_theme_font_size_override("font_size", 20)
		item_list.add_child(empty_label)
		return

	for item in items:
		var data: Dictionary = item
		var button := Button.new()
		button.text = "%s x%s\n%s" % [
			data.get("name", "Item"),
			data.get("quantity", 0),
			data.get("description", ""),
		]
		button.custom_minimum_size = Vector2(0, 76)
		button.disabled = input_locked
		button.pressed.connect(_on_item_pressed.bind(String(data.get("name", "Item"))))
		item_list.add_child(button)
		item_buttons.push_back(button)


func _refresh_team_switches(state: Dictionary = {}) -> void:
	if state.is_empty():
		state = bridge.get_battle_state()

	for child in switch_grid.get_children():
		child.queue_free()
	switch_buttons.clear()

	var active_index := int(state.get("active_player_index", 0))
	var player_team: Array = state.get("player_team", [])
	for index in range(player_team.size()):
		var player: Dictionary = player_team[index]
		var hp := int(player.get("hp", 0))
		var max_hp := int(player.get("max_hp", 1))
		var focus := int(player.get("focus", 0))
		var max_focus := int(player.get("max_focus", 1))
		var button := Button.new()
		button.text = "%s\nHP %s/%s | Focus %s/%s" % [
			player.get("name", "Player"),
			hp,
			max_hp,
			focus,
			max_focus,
		]
		button.custom_minimum_size = Vector2(270, 72)
		var switch_disabled := index == active_index or hp <= 0
		button.set_meta("switch_disabled", switch_disabled)
		button.disabled = input_locked or switch_disabled
		button.pressed.connect(_on_switch_pressed.bind(index))
		switch_grid.add_child(button)
		switch_buttons.push_back(button)


func _on_skill_pressed(skill_id: String) -> void:
	if input_locked:
		return

	input_locked = true
	_set_buttons_disabled(true)
	var result := bridge.use_skill(skill_id)
	if not result.get("accepted", false):
		_set_message(result.get("error", "That action failed."))
		input_locked = false
		_set_buttons_disabled(false)
		return

	await _play_events(result.get("events", []))
	_update_state(result.get("state", bridge.get_battle_state()))

	if result.get("battle_finished", false):
		finished_result = result
		_set_message("Battle finished. Winner: %s" % result.get("winner", "none"))
		_flush_deferred_progress_log()
		_show_return_button()
	else:
		_refresh_team_switches(result.get("state", bridge.get_battle_state()))
		_set_message("What will your player do?")
		input_locked = false
		_set_buttons_disabled(false)
		_show_main_menu()


func _on_switch_pressed(player_index: int) -> void:
	if input_locked:
		return

	input_locked = true
	_set_buttons_disabled(true)
	var result := bridge.switch_player(player_index)
	if not result.get("accepted", false):
		_set_message(result.get("error", "That switch failed."))
		input_locked = false
		_set_buttons_disabled(false)
		return

	var state: Dictionary = result.get("state", bridge.get_battle_state())
	_update_state(state)
	_refresh_team_switches(state)
	await _play_events(result.get("events", []))
	_update_state(state)
	_refresh_skills()

	if result.get("battle_finished", false):
		finished_result = result
		_set_message("Battle finished. Winner: %s" % result.get("winner", "none"))
		_flush_deferred_progress_log()
		_show_return_button()
	else:
		_set_message("What will your player do?")
		input_locked = false
		_set_buttons_disabled(false)
		_show_main_menu()


func _on_item_pressed(item_name: String) -> void:
	_set_message("%s cannot be used yet." % item_name)


func _on_quit_pressed() -> void:
	if input_locked:
		return

	input_locked = true
	_set_buttons_disabled(true)
	finished_result = {
		"accepted": true,
		"battle_finished": true,
		"winner": "opponent",
		"state": bridge.get_battle_state(),
		"reward": {
			"awarded": false,
			"total_xp": 0,
			"xp_per_participant": 0,
			"participant_player_indices": [],
		},
		"events": [],
	}
	_set_message("You quit the battle.")
	_push_log("You quit the battle.")
	_show_return_button()


func _play_events(events: Array) -> void:
	for event in events:
		_apply_event(event)
		await get_tree().create_timer(0.45).timeout


func _apply_event(event: Dictionary) -> void:
	var type := String(event.get("type", "none"))
	match type:
		"battle_started":
			_push_log("Battle start!")
			_set_message("%s wants to battle." % opponent_name.text)
		"player_switched":
			_set_message("Switched to %s." % event.get("player_name", "player"))
			_push_log(message_label.text)
		"skill_started":
			var actor := _display_actor(event.get("actor", "none"))
			_set_message("%s used %s." % [actor, event.get("skill_id", "a skill")])
			_push_log(message_label.text)
		"focus_changed":
			_animate_focus(event.get("actor", "none"), event.get("old_value", 0), event.get("new_value", 0))
		"attack_missed":
			_set_message("It missed.")
			_push_log("The play missed.")
		"damage_applied":
			_animate_hp(event.get("target", "none"), event.get("old_value", 0), event.get("new_value", 0))
			_push_log("%s took %s damage." % [_display_actor(event.get("target", "none")), event.get("amount", 0)])
		"healing_applied":
			_animate_hp(event.get("actor", "none"), event.get("old_value", 0), event.get("new_value", 0))
			_push_log("%s recovered %s HP." % [_display_actor(event.get("actor", "none")), event.get("amount", 0)])
		"status_applied":
			_push_log("A status effect changed momentum.")
		"skill_xp_gained":
			_defer_progress_event(event)
		"skill_leveled_up":
			_defer_progress_event(event)
		"battle_finished":
			_set_message("The battle is over.")
			_push_log("Winner: %s" % event.get("winner", "none"))
		"reward_granted":
			_push_log("Battle XP awarded.")


func _update_state(state: Dictionary) -> void:
	var player = state.get("player", {})
	var opponent = state.get("opponent", {})
	_update_status(player_name, player_meta, player_hp, player_hp_value, player_focus, player_focus_value, player, "Your Player")
	_update_status(opponent_name, opponent_meta, opponent_hp, opponent_hp_value, opponent_focus, opponent_focus_value, opponent, "Opponent")


func _update_status(name_label: Label, meta_label: Label, hp_bar: ProgressBar, hp_value: Label, focus_bar: ProgressBar, focus_value: Label, data: Dictionary, fallback_name: String) -> void:
	var max_hp := int(data.get("max_hp", 1))
	var hp := int(data.get("hp", 0))
	var max_focus := int(data.get("max_focus", 1))
	var focus := int(data.get("focus", 0))
	name_label.text = "%s" % data.get("name", fallback_name)
	meta_label.text = "%s / %s" % [data.get("spec", "Spec"), data.get("style", "Style")]
	hp_bar.max_value = max(1, max_hp)
	hp_bar.value = clamp(hp, 0, max_hp)
	hp_bar.tooltip_text = "HP %s/%s" % [hp, max_hp]
	hp_value.text = "%s/%s" % [hp, max_hp]
	focus_bar.max_value = max(1, max_focus)
	focus_bar.value = clamp(focus, 0, max_focus)
	focus_bar.tooltip_text = "Focus %s/%s" % [focus, max_focus]
	focus_value.text = "%s/%s" % [focus, max_focus]


func _animate_hp(actor: String, old_value: int, new_value: int) -> void:
	var bar := opponent_hp if actor == "opponent" else player_hp
	var value_label := opponent_hp_value if actor == "opponent" else player_hp_value
	bar.value = old_value
	value_label.text = "%s/%s" % [new_value, int(bar.max_value)]
	create_tween().tween_property(bar, "value", new_value, 0.30)


func _animate_focus(actor: String, old_value: int, new_value: int) -> void:
	var bar := opponent_focus if actor == "opponent" else player_focus
	var value_label := opponent_focus_value if actor == "opponent" else player_focus_value
	bar.value = old_value
	value_label.text = "%s/%s" % [new_value, int(bar.max_value)]
	create_tween().tween_property(bar, "value", new_value, 0.25)


func _set_message(text: String) -> void:
	message_label.text = text


func _push_log(text: String) -> void:
	if text.is_empty():
		return

	log_entries.push_back(text)
	if log_entries.size() > LOG_LINES:
		log_entries.remove_at(0)
	log_label.text = _join_strings(log_entries, "\n")


func _display_actor(actor: String) -> String:
	if actor == "player":
		return "Your player"
	if actor == "opponent":
		return "Opponent"
	return "Someone"


func _set_buttons_disabled(disabled: bool) -> void:
	for button in main_menu_buttons:
		button.disabled = disabled
	for button in skill_buttons:
		button.disabled = disabled
	for button in switch_buttons:
		button.disabled = disabled or bool(button.get_meta("switch_disabled", false))
	for button in item_buttons:
		button.disabled = disabled
	back_button.disabled = disabled


func _show_return_button() -> void:
	input_locked = true
	_set_buttons_disabled(true)
	main_menu_grid.visible = false
	skill_grid.visible = false
	switch_label.visible = false
	switch_grid.visible = false
	item_label.visible = false
	item_list.visible = false
	back_button.visible = false
	return_button.visible = true


func _defer_progress_event(event: Dictionary) -> void:
	if String(event.get("actor", "none")) != "player":
		return

	var skill_name := _format_skill_id(String(event.get("skill_id", "skill")))
	match String(event.get("type", "none")):
		"skill_xp_gained":
			deferred_skill_xp[skill_name] = int(deferred_skill_xp.get(skill_name, 0)) + int(event.get("amount", 0))
		"skill_leveled_up":
			deferred_skill_levels[skill_name] = int(event.get("new_level", 1))


func _flush_deferred_progress_log() -> void:
	for skill_name in deferred_skill_xp.keys():
		_push_log("%s gained %s skill XP." % [skill_name, deferred_skill_xp[skill_name]])
	for skill_name in deferred_skill_levels.keys():
		_push_log("%s reached Lv%s." % [skill_name, deferred_skill_levels[skill_name]])
	deferred_skill_xp.clear()
	deferred_skill_levels.clear()


func _format_skill_id(skill_id: String) -> String:
	var parts := skill_id.split("-")
	var formatted: Array[String] = []
	for index in range(parts.size()):
		formatted.push_back(String(parts[index]).capitalize())
	return _join_strings(formatted, " ")


func _on_return_pressed() -> void:
	if not finished_result.is_empty():
		GameState.complete_battle(finished_result)
	get_tree().change_scene_to_file("res://map.tscn")


func _join_strings(values: Array[String], separator: String) -> String:
	var output := ""
	for index in range(values.size()):
		if index > 0:
			output += separator
		output += values[index]
	return output
