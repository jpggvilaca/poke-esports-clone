extends Control

var bridge: BattleBridge

@onready var rewards_panel: RewardsPanel = $RewardsPanel
@onready var drill_minigame: DrillMinigame = $DrillMinigame
@onready var opponent_status: BattleStatusPanel = $OpponentStatus
@onready var player_status: BattleStatusPanel = $PlayerStatus
@onready var opponent_avatar: Control = $OpponentAvatar
@onready var player_avatar: Control = $PlayerAvatar
@onready var message_log: BattleMessageLog = $BottomPanel/BottomSplit/TextBox
@onready var action_panel: BattleActionPanel = $BottomPanel/BottomSplit/ActionBox
@onready var lineup_panel: BattleLineupPanel = $LineupPanel

var objective_panel: PanelContainer
var player_objective_name: Label
var player_objective_hp: ProgressBar
var player_objective_value: Label
var opponent_objective_name: Label
var opponent_objective_hp: ProgressBar
var opponent_objective_value: Label
var neutral_objective_row: HBoxContainer
var neutral_objective_name: Label
var neutral_objective_hp: ProgressBar
var neutral_objective_value: Label
var reward_presenter := BattleRewardPresenter.new()
var event_player := BattleEventPlayer.new()
var battle_event_history: Array[Dictionary] = []
var input_locked := false
var finished_result: Dictionary = {}
var rewards_applied := false
var current_state: Dictionary = {}
var pending_skill: Dictionary = {}
var battle_setup: Dictionary = {}


func _ready() -> void:
	_setup_children()
	if not _setup_bridge():
		return

	battle_setup = GameState.build_battle_setup()
	var result := bridge.start_battle(battle_setup)
	_refresh_farm_action()

	if not result.get("accepted", false):
		finished_result = result
		_update_state(result.get("state", bridge.get_battle_state()))
		message_log.set_message(result.get("error", "Battle could not start."))
		_show_return_button()
		return

	_update_state(result.get("state", bridge.get_battle_state()))
	_refresh_skills()
	_show_main_menu()
	_record_result_events(result)
	await _play_events(result.get("events", []))
	_set_turn_prompt()
	await _maybe_auto_pass_unavailable_turn()


func _setup_children() -> void:
	_create_objective_panel()
	action_panel.setup()
	action_panel.fight_requested.connect(_on_fight_requested)
	action_panel.farm_requested.connect(_on_farm_requested)
	action_panel.push_requested.connect(_on_push_requested)
	action_panel.dragon_requested.connect(_on_dragon_requested)
	action_panel.quit_requested.connect(_on_quit_requested)
	action_panel.skill_selected.connect(_on_skill_selected)
	action_panel.target_selected.connect(_on_target_selected)
	action_panel.return_requested.connect(_on_return_pressed)
	rewards_panel.return_requested.connect(_on_return_pressed)
	drill_minigame.drill_finished.connect(_on_drill_minigame_finished)
	drill_minigame.drill_cancelled.connect(_on_drill_minigame_cancelled)
	add_child(event_player)
	event_player.setup(message_log, player_status, opponent_status, reward_presenter, player_avatar, opponent_avatar)


func _setup_bridge() -> bool:
	bridge = BattleBridge.new()
	if not is_instance_valid(bridge):
		_show_bridge_error("Battle system failed to load. Check the GDExtension build in res://bin.")
		return false

	bridge.name = "BattleBridge"
	add_child(bridge)
	return true


func _create_objective_panel() -> void:
	objective_panel = PanelContainer.new()
	objective_panel.name = "ObjectivePanel"
	objective_panel.anchor_left = 0.36
	objective_panel.anchor_top = 0.055
	objective_panel.anchor_right = 0.59
	objective_panel.anchor_bottom = 0.27
	objective_panel.offset_left = 0
	objective_panel.offset_top = 0
	objective_panel.offset_right = 0
	objective_panel.offset_bottom = 0
	objective_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	objective_panel.z_index = 5

	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color(0.055, 0.065, 0.08, 0.94)
	panel_style.border_color = Color(0.36, 0.62, 0.78, 1.0)
	panel_style.set_border_width_all(3)
	panel_style.set_corner_radius_all(8)
	panel_style.content_margin_left = 12
	panel_style.content_margin_top = 10
	panel_style.content_margin_right = 12
	panel_style.content_margin_bottom = 10
	objective_panel.add_theme_stylebox_override("panel", panel_style)
	add_child(objective_panel)

	var rows := VBoxContainer.new()
	rows.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	rows.size_flags_vertical = Control.SIZE_EXPAND_FILL
	rows.add_theme_constant_override("separation", 8)
	objective_panel.add_child(rows)
	_add_objective_column(rows, "Your", false)
	_add_objective_column(rows, "Enemy", true)
	_add_neutral_objective_row(rows)


func _add_objective_column(parent: BoxContainer, title: String, is_enemy: bool) -> void:
	var row := HBoxContainer.new()
	neutral_objective_row = row
	row.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.size_flags_vertical = Control.SIZE_EXPAND_FILL
	row.add_theme_constant_override("separation", 8)
	parent.add_child(row)

	var title_label := Label.new()
	title_label.text = title
	title_label.custom_minimum_size = Vector2(54, 0)
	title_label.add_theme_font_size_override("font_size", 14)
	title_label.add_theme_color_override("font_color", Color(0.68, 0.78, 0.86, 1.0))
	row.add_child(title_label)

	var name_label := Label.new()
	name_label.text = "Objective"
	name_label.custom_minimum_size = Vector2(124, 0)
	name_label.add_theme_font_size_override("font_size", 17)
	name_label.add_theme_color_override("font_color", Color(0.94, 0.97, 0.98, 1.0))
	name_label.clip_text = true
	row.add_child(name_label)

	var bar := ProgressBar.new()
	bar.custom_minimum_size = Vector2(0, 16)
	bar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bar.show_percentage = false
	row.add_child(bar)
	_apply_objective_bar_style(bar, Color(0.78, 0.24, 0.28, 1.0) if is_enemy else Color(0.14, 0.34, 0.74, 1.0))

	var value_label := Label.new()
	value_label.custom_minimum_size = Vector2(64, 0)
	value_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	value_label.add_theme_font_size_override("font_size", 14)
	value_label.add_theme_color_override("font_color", Color(0.78, 0.96, 0.84, 1.0))
	row.add_child(value_label)

	if is_enemy:
		opponent_objective_name = name_label
		opponent_objective_hp = bar
		opponent_objective_value = value_label
	else:
		player_objective_name = name_label
		player_objective_hp = bar
		player_objective_value = value_label


func _add_neutral_objective_row(parent: BoxContainer) -> void:
	var row := HBoxContainer.new()
	row.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.size_flags_vertical = Control.SIZE_EXPAND_FILL
	row.add_theme_constant_override("separation", 8)
	parent.add_child(row)

	var title_label := Label.new()
	title_label.text = "Neutral"
	title_label.custom_minimum_size = Vector2(54, 0)
	title_label.add_theme_font_size_override("font_size", 14)
	title_label.add_theme_color_override("font_color", Color(0.86, 0.78, 0.54, 1.0))
	row.add_child(title_label)

	neutral_objective_name = Label.new()
	neutral_objective_name.text = "Dragon"
	neutral_objective_name.custom_minimum_size = Vector2(96, 0)
	neutral_objective_name.add_theme_font_size_override("font_size", 17)
	neutral_objective_name.add_theme_color_override("font_color", Color(1.0, 0.93, 0.66, 1.0))
	neutral_objective_name.clip_text = true
	row.add_child(neutral_objective_name)

	neutral_objective_hp = ProgressBar.new()
	neutral_objective_hp.custom_minimum_size = Vector2(0, 16)
	neutral_objective_hp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	neutral_objective_hp.show_percentage = false
	row.add_child(neutral_objective_hp)
	_apply_objective_bar_style(neutral_objective_hp, Color(0.94, 0.55, 0.12, 1.0))

	neutral_objective_value = Label.new()
	neutral_objective_value.custom_minimum_size = Vector2(64, 0)
	neutral_objective_value.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	neutral_objective_value.add_theme_font_size_override("font_size", 14)
	neutral_objective_value.add_theme_color_override("font_color", Color(1.0, 0.93, 0.66, 1.0))
	row.add_child(neutral_objective_value)


func _apply_objective_bar_style(bar: ProgressBar, fill_color: Color) -> void:
	var fill := StyleBoxFlat.new()
	fill.bg_color = fill_color
	fill.set_corner_radius_all(4)
	var background := StyleBoxFlat.new()
	background.bg_color = Color(0.03, 0.035, 0.045, 1.0)
	background.set_corner_radius_all(4)
	bar.add_theme_stylebox_override("fill", fill)
	bar.add_theme_stylebox_override("background", background)


func _show_bridge_error(text: String) -> void:
	push_error(text)
	message_log.set_message(text)
	finished_result = {
		"accepted": false,
		"battle_finished": true,
		"winner": "none",
		"events": [],
	}
	_show_return_button()


func _show_main_menu() -> void:
	action_panel.show_main_menu(input_locked)
	_refresh_farm_action()


func _show_fight_menu() -> void:
	action_panel.show_fight_menu(bridge.get_available_skills(), input_locked)


func _show_target_menu(skill: Dictionary) -> void:
	pending_skill = skill.duplicate(true)
	var state := bridge.get_battle_state()
	action_panel.show_target_menu(state.get("player_team", []), input_locked)


func _on_fight_requested() -> void:
	if input_locked:
		return
	if not _enemy_team_has_living_member(bridge.get_battle_state()):
		message_log.set_message("No enemy champion is alive. Push the objective.")
		_refresh_farm_action()
		return
	_show_fight_menu()


func _on_push_requested() -> void:
	if input_locked:
		return
	_lock_input()
	var result = bridge.use_push_objective()
	if not result.get("accepted", false):
		message_log.set_message(result.get("error", "That action failed."))
		_unlock_input()
		return

	_record_result_events(result)
	_update_state(result.get("state", bridge.get_battle_state()))
	await _play_events(result.get("events", []))
	await _continue_or_finish(result)


func _on_dragon_requested() -> void:
	if input_locked:
		return
	_lock_input()
	var result = bridge.use_attack_dragon()
	if not result.get("accepted", false):
		message_log.set_message(result.get("error", "That action failed."))
		_unlock_input()
		_refresh_farm_action()
		return

	_record_result_events(result)
	_update_state(result.get("state", bridge.get_battle_state()))
	await _play_events(result.get("events", []))
	await _continue_or_finish(result)


func _on_farm_requested() -> void:
	if input_locked:
		return

	_lock_input()
	var result = bridge.use_farm()
	if not result.get("accepted", false):
		message_log.set_message(result.get("error", "That action failed."))
		_unlock_input()
		return

	_record_result_events(result)
	_update_state(result.get("state", bridge.get_battle_state()))
	await _play_events(result.get("events", []))
	await _continue_or_finish(result)


func _on_drill_minigame_finished(quality: String) -> void:
	var result = bridge.use_drill(quality)
	if not result.get("accepted", false):
		message_log.set_message(result.get("error", "That action failed."))
		_unlock_input()
		return

	_record_result_events(result)
	_update_state(result.get("state", bridge.get_battle_state()))
	await _play_events(result.get("events", []))
	await _continue_or_finish(result)


func _on_drill_minigame_cancelled() -> void:
	_unlock_input()
	_refresh_farm_action()
	_set_turn_prompt()
	_show_main_menu()


func _maybe_auto_pass_unavailable_turn() -> void:
	if input_locked or bridge == null:
		return

	var state := bridge.get_battle_state()
	if bool(state.get("finished", false)):
		return

	var player: Dictionary = state.get("player", {})
	if int(player.get("hp", 0)) <= 0 and int(player.get("reinforcement_timer", 0)) > 0:
		_lock_input()
		message_log.set_message("%s is waiting to respawn (%s turn(s))." % [
			player.get("name", "Your player"),
			player.get("reinforcement_timer", 0),
		])
		await get_tree().create_timer(0.35).timeout
		await _auto_pass_turn()
		return

	var status: Dictionary = player.get("status", {})
	if int(status.get("stunned_turns", 0)) <= 0:
		return

	_lock_input()
	message_log.set_message("%s is stunned and loses the turn." % player.get("name", "Your player"))
	await get_tree().create_timer(0.35).timeout
	await _auto_pass_turn()


func _auto_pass_turn() -> void:
	var result := bridge.pass_turn()
	if not result.get("accepted", false):
		message_log.set_message(result.get("error", "That action failed."))
		_unlock_input()
		_show_main_menu()
		return

	_record_result_events(result)
	_update_state(result.get("state", bridge.get_battle_state()))
	await _play_events(result.get("events", []))
	await _continue_or_finish(result)


func _refresh_farm_action() -> void:
	if bridge == null:
		return
	var state := bridge.get_battle_state()
	action_panel.refresh_fight_action(_enemy_team_has_living_member(state), input_locked)
	action_panel.refresh_farm_action(input_locked)
	action_panel.refresh_push_action(state, input_locked)
	action_panel.refresh_dragon_action(state, input_locked)


func _refresh_skills() -> void:
	action_panel.refresh_skills(bridge.get_available_skills(), input_locked)


func _enemy_team_has_living_member(state: Dictionary) -> bool:
	var opponents: Array = state.get("opponent_team", [])
	for opponent in opponents:
		if opponent is Dictionary and int(opponent.get("hp", 0)) > 0:
			return true
	if opponents.is_empty():
		var active_opponent: Dictionary = state.get("opponent", {})
		return int(active_opponent.get("hp", 0)) > 0
	return false


func _on_skill_selected(skill: Dictionary) -> void:
	if input_locked:
		return

	var target_scope := String(skill.get("effect_target", "enemy"))
	if target_scope == "ally":
		_show_target_menu(skill)
		return

	var target_index := -1
	if target_scope == "self" or target_scope == "player_lineup":
		target_index = int(bridge.get_battle_state().get("active_player_index", 0))
	await _use_skill(String(skill.get("id", "")), target_index)


func _on_target_selected(target_index: int) -> void:
	if input_locked:
		return
	await _use_skill(String(pending_skill.get("id", "")), target_index)


func _use_skill_on_objective(skill_id: String) -> void:
	_lock_input()
	var result = bridge.use_skill_on_objective(skill_id)
	if not result.get("accepted", false):
		message_log.set_message(result.get("error", "That action failed."))
		_unlock_input()
		return

	_record_result_events(result)
	_update_state(result.get("state", bridge.get_battle_state()))
	await _play_events(result.get("events", []))
	await _continue_or_finish(result)


func _use_skill(skill_id: String, target_index: int) -> void:
	_lock_input()
	var result := bridge.use_skill(skill_id, target_index)
	if not result.get("accepted", false):
		message_log.set_message(result.get("error", "That action failed."))
		_unlock_input()
		return

	_record_result_events(result)
	_update_state(result.get("state", bridge.get_battle_state()))
	await _play_events(result.get("events", []))
	await _continue_or_finish(result)


func _on_quit_requested() -> void:
	if input_locked:
		return

	_lock_input()
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
	message_log.set_message("You quit the battle.")
	message_log.push_log("You quit the battle.")
	_show_post_battle_panel(finished_result)


func _continue_or_finish(result: Dictionary) -> void:
	if result.get("battle_finished", false):
		finished_result = result
		message_log.set_message("Battle finished. Winner: %s" % result.get("winner", "none"))
		_show_post_battle_panel(result)
		return

	_refresh_skills()
	_refresh_farm_action()
	_set_turn_prompt()
	_unlock_input()
	_show_main_menu()
	await _maybe_auto_pass_unavailable_turn()


func _play_events(events: Array) -> void:
	await event_player.play_events(events, current_state)


func _update_state(state: Dictionary) -> void:
	current_state = state.duplicate(true)
	var opponent: Dictionary = current_state.get("opponent", {})
	if not opponent.has("rating") and battle_setup.has("opponent_rating"):
		opponent["rating"] = int(battle_setup.get("opponent_rating", 0))
		current_state["opponent"] = opponent
	player_status.set_status(current_state.get("player", {}), "Your Player")
	opponent_status.set_status(current_state.get("opponent", {}), "Opponent")
	lineup_panel.refresh(current_state)
	_update_objective_panel()


func _update_objective_panel() -> void:
	if objective_panel == null:
		return

	_update_objective_side(
		current_state.get("player_objectives", []),
		int(current_state.get("active_player_objective_index", 0)),
		player_objective_name,
		player_objective_hp,
		player_objective_value)
	_update_objective_side(
		current_state.get("opponent_objectives", []),
		int(current_state.get("active_opponent_objective_index", 0)),
		opponent_objective_name,
		opponent_objective_hp,
		opponent_objective_value)
	_update_neutral_objective(current_state.get("neutral_objective", {}))


func _update_objective_side(objectives: Array, objective_index: int, name_label: Label, hp_bar: ProgressBar, value_label: Label) -> void:
	if name_label == null or hp_bar == null or value_label == null:
		return

	if objectives.is_empty():
		name_label.text = "Objective"
		hp_bar.max_value = 1
		hp_bar.value = 0
		value_label.text = "0/0"
		return

	var safe_index = clamp(objective_index, 0, objectives.size() - 1)
	var objective: Dictionary = objectives[safe_index]
	var hp := int(objective.get("hp", 0))
	var max_hp := int(objective.get("max_hp", 1))
	name_label.text = String(objective.get("name", "Objective"))
	hp_bar.max_value = max(1, max_hp)
	hp_bar.value = clamp(hp, 0, max_hp)
	hp_bar.tooltip_text = "%s HP %s/%s" % [name_label.text, hp, max_hp]
	value_label.text = "%s/%s" % [hp, max_hp]


func _update_neutral_objective(objective: Dictionary) -> void:
	if neutral_objective_row == null or neutral_objective_name == null or neutral_objective_hp == null or neutral_objective_value == null:
		return

	var name := String(objective.get("name", "Dragon"))
	var hp := int(objective.get("hp", 0))
	var max_hp := int(objective.get("max_hp", 1))
	var respawn_timer := int(objective.get("respawn_timer", 0))
	var active := bool(objective.get("active", false))
	neutral_objective_row.visible = active
	neutral_objective_name.text = name
	neutral_objective_hp.max_value = max(1, max_hp)
	neutral_objective_hp.value = clamp(hp, 0, max_hp)
	if active:
		neutral_objective_value.text = "%s/%s" % [hp, max_hp]
		neutral_objective_hp.tooltip_text = "%s HP %s/%s" % [name, hp, max_hp]
	elif respawn_timer > 0:
		neutral_objective_value.text = "in %s" % respawn_timer
		neutral_objective_hp.tooltip_text = "%s respawns in %s turn(s)." % [name, respawn_timer]
	else:
		neutral_objective_value.text = "soon"
		neutral_objective_hp.tooltip_text = "%s has not spawned yet." % name


func _set_turn_prompt() -> void:
	var player: Dictionary = current_state.get("player", {})
	message_log.set_turn_prompt(String(player.get("name", "your player")))


func _lock_input() -> void:
	input_locked = true
	action_panel.set_buttons_disabled(true)


func _unlock_input() -> void:
	input_locked = false
	action_panel.set_buttons_disabled(false)


func _show_return_button() -> void:
	input_locked = true
	action_panel.show_return_button()


func _show_post_battle_panel(result: Dictionary) -> void:
	var summary := _apply_finished_battle_once(result)
	var lines := reward_presenter.build_reward_lines(result, summary)
	_show_return_button()
	action_panel.set_return_text("Return to map")
	var title := "VICTORY" if String(result.get("winner", "none")) == "player" else "BATTLE OVER"
	rewards_panel.show_rewards(title, lines)


func _apply_finished_battle_once(result: Dictionary) -> Dictionary:
	if rewards_applied:
		return GameState.get_trainer_state().get("last_battle_summary", {}).duplicate(true)

	var complete_result := result.duplicate(true)
	complete_result["events"] = battle_event_history.duplicate(true)
	rewards_applied = true
	return GameState.complete_battle(complete_result)


func _record_result_events(result: Dictionary) -> void:
	for event in result.get("events", []):
		if event is Dictionary:
			battle_event_history.push_back(event.duplicate(true))


func _on_return_pressed() -> void:
	if not rewards_applied and bool(finished_result.get("battle_finished", false)):
		var complete_result := finished_result.duplicate(true)
		complete_result["events"] = battle_event_history.duplicate(true)
		GameState.complete_battle(complete_result)
	get_tree().change_scene_to_file("res://map.tscn")
