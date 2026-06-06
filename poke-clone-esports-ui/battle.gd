extends Control

const FIELD_COLOR := Color(0.045, 0.055, 0.075)
const PANEL_COLOR := Color(0.13, 0.15, 0.18)
const PANEL_BORDER := Color(0.36, 0.62, 0.78)
const PLAYER_COLOR := Color(0.14, 0.34, 0.74)
const OPPONENT_COLOR := Color(0.78, 0.24, 0.28)
const LOG_LINES := 5
const PLAYER_AVATAR_PATH := "res://assets/players/avatars/portrait1.png"
const OPPONENT_AVATAR_PATH := "res://assets/players/avatars/portrait2.png"
const LOG_COLOR_ATTACK := "#ff6b4a"
const LOG_COLOR_DEFENSE := "#5aa7ff"
const LOG_COLOR_DAMAGE := "#ff934f"
const LOG_COLOR_HP := "#5bd487"
const LOG_COLOR_FOCUS := "#57d7ff"
const LOG_COLOR_XP := "#f3d35b"

var bridge: BattleBridge
var opponent_name: Label
var opponent_meta: Label
var opponent_hp: ProgressBar
var opponent_hp_value: Label
var opponent_focus: ProgressBar
var opponent_focus_value: Label
var opponent_status: Label
var player_name: Label
var player_meta: Label
var player_hp: ProgressBar
var player_hp_value: Label
var player_focus: ProgressBar
var player_focus_value: Label
var player_status: Label
var message_label: Label
var log_label: RichTextLabel
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
var battle_event_history: Array = []
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
	_record_result_events(result)
	await _play_events(result.get("events", []))
	_set_message("What will your player do?")


func _build_ui() -> void:
	var background := ColorRect.new()
	background.color = FIELD_COLOR
	background.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(background)

	var stage_glow := ColorRect.new()
	stage_glow.color = Color(0.08, 0.12, 0.16)
	stage_glow.anchor_left = 0.07
	stage_glow.anchor_top = 0.12
	stage_glow.anchor_right = 0.93
	stage_glow.anchor_bottom = 0.68
	add_child(stage_glow)

	var desk := ColorRect.new()
	desk.color = Color(0.09, 0.095, 0.105)
	desk.anchor_left = 0.31
	desk.anchor_top = 0.44
	desk.anchor_right = 0.69
	desk.anchor_bottom = 0.58
	add_child(desk)

	var player_monitor := _make_monitor_block("PLAYER PC", PLAYER_COLOR)
	player_monitor.anchor_left = 0.37
	player_monitor.anchor_top = 0.25
	player_monitor.anchor_right = 0.49
	player_monitor.anchor_bottom = 0.48
	add_child(player_monitor)

	var opponent_monitor := _make_monitor_block("ENEMY PC", OPPONENT_COLOR)
	opponent_monitor.anchor_left = 0.51
	opponent_monitor.anchor_top = 0.25
	opponent_monitor.anchor_right = 0.63
	opponent_monitor.anchor_bottom = 0.48
	add_child(opponent_monitor)

	var opponent_sprite := _make_avatar_block(OPPONENT_AVATAR_PATH, OPPONENT_COLOR, "OPPONENT")
	opponent_sprite.anchor_left = 0.73
	opponent_sprite.anchor_top = 0.22
	opponent_sprite.anchor_right = 0.88
	opponent_sprite.anchor_bottom = 0.53
	add_child(opponent_sprite)

	var player_sprite := _make_avatar_block(PLAYER_AVATAR_PATH, PLAYER_COLOR, "PLAYER")
	player_sprite.anchor_left = 0.12
	player_sprite.anchor_top = 0.22
	player_sprite.anchor_right = 0.27
	player_sprite.anchor_bottom = 0.53
	add_child(player_sprite)

	var opponent_panel := _make_status_panel()
	opponent_panel.anchor_left = 0.60
	opponent_panel.anchor_top = 0.08
	opponent_panel.anchor_right = 0.94
	opponent_panel.anchor_bottom = 0.24
	add_child(opponent_panel)
	opponent_name = opponent_panel.get_node("Content/Name")
	opponent_meta = opponent_panel.get_node("Content/Meta")
	opponent_hp = opponent_panel.get_node("Content/HpRow/Hp")
	opponent_hp_value = opponent_panel.get_node("Content/HpRow/Value")
	opponent_focus = opponent_panel.get_node("Content/FocusRow/Focus")
	opponent_focus_value = opponent_panel.get_node("Content/FocusRow/Value")
	opponent_status = opponent_panel.get_node("Content/Status")

	var player_panel := _make_status_panel()
	player_panel.anchor_left = 0.06
	player_panel.anchor_top = 0.55
	player_panel.anchor_right = 0.40
	player_panel.anchor_bottom = 0.71
	add_child(player_panel)
	player_name = player_panel.get_node("Content/Name")
	player_meta = player_panel.get_node("Content/Meta")
	player_hp = player_panel.get_node("Content/HpRow/Hp")
	player_hp_value = player_panel.get_node("Content/HpRow/Value")
	player_focus = player_panel.get_node("Content/FocusRow/Focus")
	player_focus_value = player_panel.get_node("Content/FocusRow/Value")
	player_status = player_panel.get_node("Content/Status")

	var bottom := PanelContainer.new()
	bottom.anchor_left = 0.035
	bottom.anchor_top = 0.74
	bottom.anchor_right = 0.965
	bottom.anchor_bottom = 0.965
	bottom.add_theme_stylebox_override("panel", _panel_style(Color(0.10, 0.11, 0.13), PANEL_BORDER, 10))
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
	message_label.add_theme_color_override("font_color", Color(0.92, 0.95, 0.96))
	message_label.size_flags_vertical = Control.SIZE_EXPAND_FILL
	text_box.add_child(message_label)

	log_label = RichTextLabel.new()
	log_label.text = ""
	log_label.bbcode_enabled = true
	log_label.fit_content = true
	log_label.scroll_active = false
	log_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	log_label.add_theme_font_size_override("font_size", 20)
	log_label.add_theme_color_override("default_color", Color(0.84, 0.88, 0.90))
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
	name_label.add_theme_color_override("font_color", Color(0.94, 0.97, 0.98))
	content.add_child(name_label)

	var meta_label := Label.new()
	meta_label.name = "Meta"
	meta_label.add_theme_font_size_override("font_size", 18)
	meta_label.add_theme_color_override("font_color", Color(0.62, 0.72, 0.80))
	content.add_child(meta_label)

	var hp_row := HBoxContainer.new()
	hp_row.name = "HpRow"
	hp_row.add_theme_constant_override("separation", 8)
	content.add_child(hp_row)

	var hp_label := Label.new()
	hp_label.text = "HP"
	hp_label.custom_minimum_size = Vector2(58, 0)
	hp_label.add_theme_font_size_override("font_size", 16)
	hp_label.add_theme_color_override("font_color", Color(0.78, 0.96, 0.84))
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
	hp_value.add_theme_color_override("font_color", Color(0.78, 0.96, 0.84))
	hp_row.add_child(hp_value)

	var focus_row := HBoxContainer.new()
	focus_row.name = "FocusRow"
	focus_row.add_theme_constant_override("separation", 8)
	content.add_child(focus_row)

	var focus_label := Label.new()
	focus_label.text = "Focus"
	focus_label.custom_minimum_size = Vector2(58, 0)
	focus_label.add_theme_font_size_override("font_size", 16)
	focus_label.add_theme_color_override("font_color", Color(0.72, 0.90, 1.0))
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
	focus_value.add_theme_color_override("font_color", Color(0.72, 0.90, 1.0))
	focus_row.add_child(focus_value)

	var status_label := Label.new()
	status_label.name = "Status"
	status_label.text = "Status: none"
	status_label.add_theme_font_size_override("font_size", 15)
	status_label.add_theme_color_override("font_color", Color(0.93, 0.82, 0.45))
	content.add_child(status_label)
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


func _make_monitor_block(label_text: String, accent: Color) -> PanelContainer:
	var panel := PanelContainer.new()
	panel.add_theme_stylebox_override("panel", _panel_style(Color(0.035, 0.04, 0.05), accent, 8))

	var content := VBoxContainer.new()
	content.add_theme_constant_override("separation", 6)
	panel.add_child(content)

	var screen := ColorRect.new()
	screen.color = Color(0.05, 0.10, 0.13)
	screen.custom_minimum_size = Vector2(160, 96)
	screen.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	screen.size_flags_vertical = Control.SIZE_EXPAND_FILL
	content.add_child(screen)

	var label := Label.new()
	label.text = label_text
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	label.add_theme_font_size_override("font_size", 15)
	label.add_theme_color_override("font_color", Color(0.85, 0.95, 1.0))
	content.add_child(label)
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
		button.text = _format_skill_button(skill)
		button.custom_minimum_size = Vector2(270, 112)
		button.disabled = input_locked
		button.pressed.connect(_on_skill_pressed.bind(skill.get("id", "")))
		skill_grid.add_child(button)
		skill_buttons.push_back(button)


func _format_skill_button(skill: Dictionary) -> String:
	var description := String(skill.get("description", ""))
	if description.length() > 58:
		description = description.substr(0, 55) + "..."
	return "%s\nPWR %s | Focus %s\n%s" % [
		skill.get("name", "Skill"),
		skill.get("power", 0),
		skill.get("focus_cost", 0),
		description,
	]


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
		var identity_name := String(player.get("trait_name", ""))
		var identity_suffix := ""
		if not identity_name.is_empty():
			identity_suffix = " | %s" % identity_name
		var button := Button.new()
		button.text = "%s%s\nHP %s/%s | Focus %s/%s" % [
			player.get("name", "Player"),
			identity_suffix,
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

	_record_result_events(result)
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

	_record_result_events(result)
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
			_set_message("%s used %s." % [actor, _format_skill_id(String(event.get("skill_id", "a skill")))])
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
			_push_log(_format_status_log(event))
		"skill_xp_gained":
			_defer_progress_event(event)
		"skill_leveled_up":
			_defer_progress_event(event)
		"battle_finished":
			_set_message("The battle is over.")
			_push_log("Winner: %s" % event.get("winner", "none"))
		"reward_granted":
			var reward: Dictionary = event.get("reward", {})
			_push_log("Team earned %s battle XP." % reward.get("total_xp", 0))


func _update_state(state: Dictionary) -> void:
	var player = state.get("player", {})
	var opponent = state.get("opponent", {})
	_update_status(player_name, player_meta, player_hp, player_hp_value, player_focus, player_focus_value, player_status, player, "Your Player")
	_update_status(opponent_name, opponent_meta, opponent_hp, opponent_hp_value, opponent_focus, opponent_focus_value, opponent_status, opponent, "Opponent")


func _update_status(name_label: Label, meta_label: Label, hp_bar: ProgressBar, hp_value: Label, focus_bar: ProgressBar, focus_value: Label, status_label: Label, data: Dictionary, fallback_name: String) -> void:
	var max_hp := int(data.get("max_hp", 1))
	var hp := int(data.get("hp", 0))
	var max_focus := int(data.get("max_focus", 1))
	var focus := int(data.get("focus", 0))
	name_label.text = "%s" % data.get("name", fallback_name)
	var identity_name := String(data.get("trait_name", ""))
	var identity_suffix := ""
	if not identity_name.is_empty():
		identity_suffix = " | %s" % identity_name
	meta_label.text = "%s%s" % [
		data.get("spec", "Spec"),
		identity_suffix,
	]
	hp_bar.max_value = max(1, max_hp)
	hp_bar.value = clamp(hp, 0, max_hp)
	hp_bar.tooltip_text = "HP %s/%s" % [hp, max_hp]
	hp_value.text = "%s/%s" % [hp, max_hp]
	focus_bar.max_value = max(1, max_focus)
	focus_bar.value = clamp(focus, 0, max_focus)
	focus_bar.tooltip_text = "Focus %s/%s" % [focus, max_focus]
	focus_value.text = "%s/%s" % [focus, max_focus]
	status_label.text = _format_status_indicator(data.get("status", {}))


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

	log_entries.push_back(_colorize_log(text))
	if log_entries.size() > LOG_LINES:
		log_entries.remove_at(0)
	log_label.text = _join_strings(log_entries, "\n")


func _colorize_log(text: String) -> String:
	var output := text
	output = output.replace("attack", "[color=%s]attack[/color]" % LOG_COLOR_ATTACK)
	output = output.replace("Attack", "[color=%s]Attack[/color]" % LOG_COLOR_ATTACK)
	output = output.replace("defense", "[color=%s]defense[/color]" % LOG_COLOR_DEFENSE)
	output = output.replace("Defense", "[color=%s]Defense[/color]" % LOG_COLOR_DEFENSE)
	output = output.replace("damage", "[color=%s]damage[/color]" % LOG_COLOR_DAMAGE)
	output = output.replace("Damage", "[color=%s]Damage[/color]" % LOG_COLOR_DAMAGE)
	output = output.replace("HP", "[color=%s]HP[/color]" % LOG_COLOR_HP)
	output = output.replace("Focus", "[color=%s]Focus[/color]" % LOG_COLOR_FOCUS)
	output = output.replace("XP", "[color=%s]XP[/color]" % LOG_COLOR_XP)
	output = output.replace("LEVEL UP", "[color=%s]LEVEL UP[/color]" % LOG_COLOR_XP)
	return output


func _display_actor(actor: String) -> String:
	if actor == "player":
		return "Your player"
	if actor == "opponent":
		return "Opponent"
	return "Someone"


func _format_status_log(event: Dictionary) -> String:
	var actor := _display_actor(String(event.get("actor", "none")))
	var target := _display_actor(String(event.get("target", "none")))
	var amount := int(event.get("amount", 0))
	match String(event.get("effect", "none")):
		"attack_modifier":
			if amount >= 0:
				return "%s boosted attack pressure by %s%%." % [target, amount]
			return "%s attack pressure fell by %s%%." % [target, abs(amount)]
		"defense_modifier":
			if amount >= 0:
				return "%s reduced incoming damage by %s%%." % [target, amount]
			return "%s became exposed and takes %s%% more damage." % [target, abs(amount)]
	return "%s changed %s's battle state." % [actor, target]


func _format_status_indicator(status: Dictionary) -> String:
	var entries: Array[String] = []
	var attack_percent := int(status.get("attack_modifier_percent", 0))
	var attack_hits := int(status.get("attack_modifier_hits", 0))
	var defense_percent := int(status.get("defense_modifier_percent", 0))
	var defense_hits := int(status.get("defense_modifier_hits", 0))

	if attack_percent != 0 and attack_hits > 0:
		entries.push_back("%s ATK %s%% x%s" % [_status_marker(attack_percent), _signed_value(attack_percent), attack_hits])
	if defense_percent != 0 and defense_hits > 0:
		entries.push_back("%s DEF %s%% x%s" % [_status_marker(defense_percent), _signed_value(defense_percent), defense_hits])
	if entries.is_empty():
		return "Status: none"
	return _join_strings(entries, "  ")


func _status_marker(value: int) -> String:
	if value > 0:
		return "BUFF"
	return "DEBUFF"


func _signed_value(value: int) -> String:
	if value > 0:
		return "+%s" % value
	return "%s" % value


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


func _record_result_events(result: Dictionary) -> void:
	for event in result.get("events", []):
		if event is Dictionary:
			battle_event_history.push_back(event.duplicate(true))


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
		_push_log("LEVEL UP: %s reached Lv%s." % [skill_name, deferred_skill_levels[skill_name]])
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
		var complete_result := finished_result.duplicate(true)
		complete_result["events"] = battle_event_history.duplicate(true)
		GameState.complete_battle(complete_result)
	get_tree().change_scene_to_file("res://map.tscn")


func _join_strings(values: Array[String], separator: String) -> String:
	var output := ""
	for index in range(values.size()):
		if index > 0:
			output += separator
		output += values[index]
	return output
