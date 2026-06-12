extends Control

const LOG_LINES := 5
const LOG_COLOR_ATTACK := "#ff6b4a"
const LOG_COLOR_DEFENSE := "#5aa7ff"
const LOG_COLOR_DAMAGE := "#ff934f"
const LOG_COLOR_HP := "#5bd487"
const LOG_COLOR_MANA := "#57d7ff"
const LOG_COLOR_XP := "#f3d35b"
const SKILL_COLORS := {
	"neutral": Color(0.32, 0.34, 0.38, 1.0),
	"top": Color(0.58, 0.46, 0.28, 1.0),
	"jungle": Color(0.20, 0.48, 0.30, 1.0),
	"mid": Color(0.36, 0.34, 0.70, 1.0),
	"adc": Color(0.70, 0.22, 0.20, 1.0),
	"support": Color(0.22, 0.48, 0.68, 1.0),
}

var bridge: BattleBridge
@onready var rewards_panel: RewardsPanel = $RewardsPanel
@onready var drill_minigame: DrillMinigame = $DrillMinigame
@onready var opponent_name: Label = $OpponentStatus/Content/Name
@onready var opponent_meta: Label = $OpponentStatus/Content/Meta
@onready var opponent_hp: ProgressBar = $OpponentStatus/Content/HpRow/Hp
@onready var opponent_hp_value: Label = $OpponentStatus/Content/HpRow/Value
@onready var opponent_mana: ProgressBar = $OpponentStatus/Content/ManaRow/Mana
@onready var opponent_mana_value: Label = $OpponentStatus/Content/ManaRow/Value
@onready var opponent_status: Label = $OpponentStatus/Content/Status
@onready var player_name: Label = $PlayerStatus/Content/Name
@onready var player_meta: Label = $PlayerStatus/Content/Meta
@onready var player_hp: ProgressBar = $PlayerStatus/Content/HpRow/Hp
@onready var player_hp_value: Label = $PlayerStatus/Content/HpRow/Value
@onready var player_mana: ProgressBar = $PlayerStatus/Content/ManaRow/Mana
@onready var player_mana_value: Label = $PlayerStatus/Content/ManaRow/Value
@onready var player_status: Label = $PlayerStatus/Content/Status
@onready var message_label: Label = $BottomPanel/BottomSplit/TextBox/Message
@onready var log_label: RichTextLabel = $BottomPanel/BottomSplit/TextBox/Log
@onready var main_menu_grid: GridContainer = $BottomPanel/BottomSplit/ActionBox/MainMenuGrid
@onready var skill_grid: GridContainer = $BottomPanel/BottomSplit/ActionBox/SkillGrid
@onready var target_label: Label = $BottomPanel/BottomSplit/ActionBox/TargetLabel
@onready var target_grid: GridContainer = $BottomPanel/BottomSplit/ActionBox/TargetGrid
@onready var fight_button: Button = $BottomPanel/BottomSplit/ActionBox/MainMenuGrid/FightButton
@onready var drill_button: Button = $BottomPanel/BottomSplit/ActionBox/MainMenuGrid/DrillButton
@onready var quit_button: Button = $BottomPanel/BottomSplit/ActionBox/MainMenuGrid/QuitButton
@onready var back_button: Button = $BottomPanel/BottomSplit/ActionBox/BackButton
@onready var return_button: Button = $BottomPanel/BottomSplit/ActionBox/ReturnButton
@onready var lineup_grid: GridContainer = $LineupPanel/Margin/LineupGrid
var main_menu_buttons: Array[Button] = []
var skill_buttons: Array[Button] = []
var target_buttons: Array[Button] = []
var lineup_buttons: Array[Button] = []
var log_entries: Array[String] = []
var deferred_skill_xp: Dictionary = {}
var deferred_skill_levels: Dictionary = {}
var battle_event_history: Array[Dictionary] = []
var input_locked := false
var finished_result: Dictionary = {}
var rewards_applied := false
var current_state: Dictionary = {}
var pending_skill: Dictionary = {}


func _ready() -> void:
	main_menu_buttons.clear()
	main_menu_buttons.push_back(fight_button)
	main_menu_buttons.push_back(quit_button)
	fight_button.pressed.connect(_on_fight_menu_pressed)
	drill_button.pressed.connect(_on_drill_pressed)
	quit_button.pressed.connect(_on_quit_pressed)
	back_button.pressed.connect(_show_main_menu)
	return_button.pressed.connect(_on_return_pressed)
	rewards_panel.return_requested.connect(_on_return_pressed)
	drill_minigame.drill_finished.connect(_on_drill_minigame_finished)
	drill_minigame.drill_cancelled.connect(_on_drill_minigame_cancelled)
	drill_button.visible = false
	bridge = BattleBridge.new()
	add_child(bridge)

	var result := bridge.start_battle(GameState.build_battle_setup())
	_refresh_drill_action()

	if not result.get("accepted", false):
		finished_result = result
		_update_state(result.get("state", bridge.get_battle_state()))
		_set_message(result.get("error", "Battle could not start."))
		_show_return_button()
		return

	var state: Dictionary = result.get("state", bridge.get_battle_state())
	_update_state(state)
	_refresh_skills()
	_show_main_menu()
	_record_result_events(result)
	await _play_events(result.get("events", []))
	_set_turn_prompt()
	await _maybe_auto_pass_stunned_turn()


func _show_main_menu() -> void:
	main_menu_grid.visible = true
	skill_grid.visible = false
	target_label.visible = false
	target_grid.visible = false
	back_button.visible = false
	return_button.visible = false
	_set_buttons_disabled(input_locked)
	_refresh_drill_action()


func _show_fight_menu() -> void:
	_refresh_skills()
	main_menu_grid.visible = false
	skill_grid.visible = true
	target_label.visible = false
	target_grid.visible = false
	back_button.visible = true


func _show_target_menu(skill: Dictionary) -> void:
	pending_skill = skill.duplicate(true)
	_refresh_target_buttons()
	main_menu_grid.visible = false
	skill_grid.visible = false
	target_label.visible = true
	target_grid.visible = true
	back_button.visible = true


func _on_fight_menu_pressed() -> void:
	if input_locked:
		return
	_show_fight_menu()


func _on_drill_pressed() -> void:
	if input_locked:
		return

	var drill := bridge.get_drill_action()
	if not bool(drill.get("can_use", true)):
		_set_message(String(drill.get("disabled_reason", "That action is unavailable.")))
		return

	input_locked = true
	_set_buttons_disabled(true)
	drill_minigame.start(drill)


func _on_drill_minigame_finished(quality: String) -> void:
	var result = bridge.use_drill(quality)
	if not result.get("accepted", false):
		_set_message(result.get("error", "That action failed."))
		input_locked = false
		_set_buttons_disabled(false)
		return

	_record_result_events(result)
	await _play_events(result.get("events", []))
	var state: Dictionary = result.get("state", bridge.get_battle_state())
	_update_state(state)

	if result.get("battle_finished", false):
		finished_result = result
		_set_message("Battle finished. Winner: %s" % result.get("winner", "none"))
		_show_post_battle_panel(result)
	else:
		_refresh_skills()
		_refresh_drill_action()
		_set_turn_prompt()
		input_locked = false
		_set_buttons_disabled(false)
		_show_main_menu()
		await _maybe_auto_pass_stunned_turn()


func _on_drill_minigame_cancelled() -> void:
	input_locked = false
	_set_buttons_disabled(false)
	_refresh_drill_action()
	_set_turn_prompt()
	_show_main_menu()


func _maybe_auto_pass_stunned_turn() -> void:
	if input_locked or bridge == null:
		return

	var state := bridge.get_battle_state()
	if bool(state.get("finished", false)):
		return

	var player: Dictionary = state.get("player", {})
	var status: Dictionary = player.get("status", {})
	if int(status.get("stunned_turns", 0)) <= 0:
		return

	input_locked = true
	_set_buttons_disabled(true)
	_set_message("%s is stunned and loses the turn." % player.get("name", "Your player"))
	await get_tree().create_timer(0.35).timeout

	var result := bridge.pass_turn()
	if not result.get("accepted", false):
		_set_message(result.get("error", "That action failed."))
		input_locked = false
		_set_buttons_disabled(false)
		_show_main_menu()
		return

	_record_result_events(result)
	await _play_events(result.get("events", []))
	state = result.get("state", bridge.get_battle_state())
	_update_state(state)

	if result.get("battle_finished", false):
		finished_result = result
		_set_message("Battle finished. Winner: %s" % result.get("winner", "none"))
		_show_post_battle_panel(result)
	else:
		_refresh_skills()
		_refresh_drill_action()
		_set_turn_prompt()
		input_locked = false
		_set_buttons_disabled(false)
		_show_main_menu()
		await _maybe_auto_pass_stunned_turn()


func _refresh_drill_action() -> void:
	if drill_button == null or bridge == null:
		return

	var drill := bridge.get_drill_action()
	var display_name := String(drill.get("display_name", "Drill"))
	drill_button.text = display_name
	drill_button.tooltip_text = String(drill.get("description", ""))
	drill_button.disabled = input_locked or not bool(drill.get("can_use", true))


func _refresh_skills() -> void:
	for child in skill_grid.get_children():
		child.queue_free()
	skill_buttons.clear()

	for skill in bridge.get_available_skills():
		var button := Button.new()
		button.text = _format_skill_button(skill)
		button.custom_minimum_size = Vector2(270, 112)
		button.disabled = input_locked or not bool(skill.get("can_use", true))
		_apply_skill_button_style(button, String(skill.get("skill_color_id", "neutral")))
		button.pressed.connect(_on_skill_pressed.bind(skill.duplicate(true)))
		skill_grid.add_child(button)
		skill_buttons.push_back(button)


func _format_skill_button(skill: Dictionary) -> String:
	var description := String(skill.get("description", ""))
	if description.length() > 58:
		description = description.substr(0, 55) + "..."
	var mana_cost := int(skill.get("mana_cost", 0))
	var mana_gain := int(skill.get("mana_gain", 0))
	var cooldown := int(skill.get("cooldown_turns", 0))
	var cooldown_remaining := int(skill.get("cooldown_remaining", 0))
	var availability := String(skill.get("disabled_reason", ""))
	var cost_text := "Gain %s mana" % mana_gain if mana_cost == 0 and mana_gain > 0 else "Mana %s" % mana_cost
	var cooldown_text := "CD %s" % cooldown
	if cooldown_remaining > 0:
		cooldown_text = "CD %s/%s" % [cooldown_remaining, cooldown]
	if not availability.is_empty():
		cooldown_text += " | %s" % availability
	return "%s\n%s | %s\n%s" % [
		skill.get("name", "Skill"),
		cost_text,
		cooldown_text,
		description,
	]


func _apply_skill_button_style(button: Button, color_id: String) -> void:
	var base_color: Color = SKILL_COLORS.get(color_id, SKILL_COLORS["neutral"])
	var normal := StyleBoxFlat.new()
	normal.bg_color = base_color.darkened(0.32)
	normal.border_color = base_color.lightened(0.24)
	normal.set_border_width_all(2)
	normal.set_corner_radius_all(6)

	var hover := normal.duplicate()
	hover.bg_color = base_color.darkened(0.18)
	hover.border_color = base_color.lightened(0.36)

	var pressed := normal.duplicate()
	pressed.bg_color = base_color.darkened(0.42)

	var disabled := normal.duplicate()
	disabled.bg_color = Color(0.12, 0.12, 0.14, 0.86)
	disabled.border_color = Color(0.28, 0.28, 0.32, 0.8)

	button.add_theme_stylebox_override("normal", normal)
	button.add_theme_stylebox_override("hover", hover)
	button.add_theme_stylebox_override("pressed", pressed)
	button.add_theme_stylebox_override("focus", hover)
	button.add_theme_stylebox_override("disabled", disabled)
	button.add_theme_color_override("font_color", Color(0.95, 0.97, 1.0, 1.0))
	button.add_theme_color_override("font_disabled_color", Color(0.58, 0.60, 0.64, 1.0))


func _refresh_lineup_strip(state: Dictionary = {}) -> void:
	if lineup_grid == null:
		return
	if state.is_empty():
		state = current_state

	for child in lineup_grid.get_children():
		child.queue_free()
	lineup_buttons.clear()

	var active_index := int(state.get("active_player_index", 0))
	var player_team: Array = state.get("player_team", [])
	for index in range(player_team.size()):
		var player: Dictionary = player_team[index]
		var hp := int(player.get("hp", 0))
		var max_hp := int(player.get("max_hp", 1))
		var mana := int(player.get("mana", 0))
		var max_mana := int(player.get("max_mana", 1))
		var status_text := _compact_status_indicator(player.get("status", {}))
		var state_text := "TURN" if index == active_index else "WAITING"
		if hp <= 0:
			state_text = "DOWN"

		var button := Button.new()
		button.custom_minimum_size = Vector2(190, 118)
		button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		button.text = "%s\n%s | %s\nHP %s/%s\nMana %s/%s%s" % [
			state_text,
			player.get("name", "Player"),
			player.get("spec", "Spec"),
			hp,
			max_hp,
			mana,
			max_mana,
			"\n%s" % status_text if not status_text.is_empty() else "",
		]
		button.mouse_filter = Control.MOUSE_FILTER_IGNORE
		button.focus_mode = Control.FOCUS_NONE
		if hp <= 0:
			_apply_lineup_card_style(button, false, true)
			button.tooltip_text = "Down"
		elif index == active_index:
			_apply_lineup_card_style(button, true, false)
			button.tooltip_text = "Current party turn"
		else:
			_apply_lineup_card_style(button, false, false)
			button.tooltip_text = "Waiting in party order"
		lineup_grid.add_child(button)
		lineup_buttons.push_back(button)


func _apply_lineup_card_style(button: Button, is_active: bool, is_down: bool) -> void:
	var style := StyleBoxFlat.new()
	style.set_corner_radius_all(6)
	if is_down:
		style.bg_color = Color(0.10, 0.10, 0.12, 0.82)
		style.border_color = Color(0.32, 0.32, 0.36, 0.95)
		style.set_border_width_all(1)
		button.add_theme_color_override("font_color", Color(0.62, 0.62, 0.66, 1.0))
	elif is_active:
		style.bg_color = Color(0.15, 0.20, 0.32, 0.96)
		style.border_color = Color(0.95, 0.77, 0.23, 1.0)
		style.set_border_width_all(4)
		button.add_theme_color_override("font_color", Color(1.0, 0.95, 0.76, 1.0))
	else:
		style.bg_color = Color(0.08, 0.11, 0.18, 0.84)
		style.border_color = Color(0.28, 0.42, 0.67, 0.72)
		style.set_border_width_all(1)
		button.add_theme_color_override("font_color", Color(0.82, 0.88, 1.0, 1.0))

	for state_name in ["normal", "hover", "pressed", "focus"]:
		button.add_theme_stylebox_override(state_name, style)


func _refresh_target_buttons() -> void:
	for child in target_grid.get_children():
		child.queue_free()
	target_buttons.clear()

	var state := bridge.get_battle_state()
	var player_team: Array = state.get("player_team", [])
	for index in range(player_team.size()):
		var player: Dictionary = player_team[index]
		var hp := int(player.get("hp", 0))
		var max_hp := int(player.get("max_hp", 1))
		var mana := int(player.get("mana", 0))
		var max_mana := int(player.get("max_mana", 1))
		var button := Button.new()
		button.text = "%s\nHP %s/%s | Mana %s/%s" % [
			player.get("name", "Player"),
			hp,
			max_hp,
			mana,
			max_mana,
		]
		button.custom_minimum_size = Vector2(270, 72)
		button.disabled = input_locked or hp <= 0
		button.pressed.connect(_on_target_pressed.bind(index))
		target_grid.add_child(button)
		target_buttons.push_back(button)


func _on_skill_pressed(skill: Dictionary) -> void:
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


func _on_target_pressed(target_index: int) -> void:
	if input_locked:
		return
	await _use_skill(String(pending_skill.get("id", "")), target_index)


func _use_skill(skill_id: String, target_index: int) -> void:
	input_locked = true
	_set_buttons_disabled(true)
	var result := bridge.use_skill(skill_id, target_index)
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
		_show_post_battle_panel(result)
	else:
		_set_turn_prompt()
		input_locked = false
		_set_buttons_disabled(false)
		_show_main_menu()
		await _maybe_auto_pass_stunned_turn()


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
	_show_post_battle_panel(finished_result)


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
			if String(event.get("actor", "")) == "opponent":
				_set_message("Opponent sends out %s." % event.get("player_name", "opponent"))
			elif String(event.get("reason", "")) == "turn":
				_set_message("Next up: %s." % event.get("player_name", "player"))
			else:
				_set_message("Switched to %s." % event.get("player_name", "player"))
			_push_log(message_label.text)
		"skill_started":
			var actor := _display_event_actor(event)
			_set_message("%s used %s." % [actor, _format_skill_id(String(event.get("skill_id", "a skill")))])
			_push_log(message_label.text)
		"drill_started":
			var actor := _display_actor(event.get("actor", "none"))
			var drill_name := String(event.get("reason", "Drill"))
			_set_message("%s chose %s." % [actor, drill_name])
			_push_log(message_label.text)
		"drill_completed":
			var actor := _display_actor(event.get("actor", "none"))
			_push_log("%s drill result: %s (+%s mana)." % [
				actor,
				String(event.get("reason", "good")).capitalize(),
				event.get("amount", 0),
			])
		"mana_changed":
			_animate_mana(event.get("actor", "none"), event.get("old_value", 0), event.get("new_value", 0), int(event.get("actor_player_index", -1)))
			_push_log("%s mana %s -> %s." % [_display_event_actor(event), event.get("old_value", 0), event.get("new_value", 0)])
		"cooldown_started":
			_push_log("%s cooldown: %s turn(s)." % [_format_skill_id(String(event.get("skill_id", ""))), event.get("new_value", 0)])
		"cooldown_ready":
			_push_log("%s is ready." % _format_skill_id(String(event.get("skill_id", ""))))
		"action_blocked":
			_push_log("%s could not act: %s." % [_display_actor(event.get("actor", "none")), event.get("reason", "blocked")])
		"attack_missed":
			_set_message("It missed.")
			_push_log("The play missed.")
		"damage_applied":
			_animate_hp(event.get("target", "none"), event.get("old_value", 0), event.get("new_value", 0), int(event.get("target_player_index", -1)))
			_push_log("%s took %s damage." % [_display_event_target(event), event.get("amount", 0)])
		"healing_applied":
			_animate_hp(event.get("target", "none"), event.get("old_value", 0), event.get("new_value", 0), int(event.get("target_player_index", -1)))
			_push_log("%s recovered %s HP." % [_display_event_target(event), event.get("amount", 0)])
		"status_applied":
			_push_log(_format_status_log(event))
		"status_expired":
			_push_log("%s status expired." % _display_event_target(event))
		"mark_applied":
			_push_log("%s was marked." % _display_event_target(event))
		"mark_triggered":
			_push_log("Mark triggered for %s bonus damage." % event.get("amount", 0))
		"mark_expired":
			_push_log("%s mark expired." % _display_event_target(event))
		"farming_triggered":
			_push_log("%s: lineup gained up to %s mana." % [event.get("reason", "Farm secured"), event.get("amount", 0)])
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
	current_state = state.duplicate(true)
	var player = state.get("player", {})
	var opponent = state.get("opponent", {})
	_update_status(player_name, player_meta, player_hp, player_hp_value, player_mana, player_mana_value, player_status, player, "Your Player")
	_update_status(opponent_name, opponent_meta, opponent_hp, opponent_hp_value, opponent_mana, opponent_mana_value, opponent_status, opponent, "Opponent")
	_refresh_lineup_strip(state)


func _update_status(name_label: Label, meta_label: Label, hp_bar: ProgressBar, hp_value: Label, mana_bar: ProgressBar, mana_value: Label, status_label: Label, data: Dictionary, fallback_name: String) -> void:
	var max_hp := int(data.get("max_hp", 1))
	var hp := int(data.get("hp", 0))
	var max_mana := int(data.get("max_mana", 1))
	var mana := int(data.get("mana", 0))
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
	mana_bar.max_value = max(1, max_mana)
	mana_bar.value = clamp(mana, 0, max_mana)
	mana_bar.tooltip_text = "Mana %s/%s" % [mana, max_mana]
	mana_value.text = "%s/%s" % [mana, max_mana]
	status_label.text = _format_status_indicator(data.get("status", {}))


func _animate_hp(actor: String, old_value: int, new_value: int, player_index: int = -1) -> void:
	if actor == "player" and player_index != int(current_state.get("active_player_index", 0)):
		return
	var bar := opponent_hp if actor == "opponent" else player_hp
	var value_label := opponent_hp_value if actor == "opponent" else player_hp_value
	bar.value = old_value
	value_label.text = "%s/%s" % [new_value, int(bar.max_value)]
	create_tween().tween_property(bar, "value", new_value, 0.30)


func _animate_mana(actor: String, old_value: int, new_value: int, player_index: int = -1) -> void:
	if actor == "player" and player_index != int(current_state.get("active_player_index", 0)):
		return
	var bar := opponent_mana if actor == "opponent" else player_mana
	var value_label := opponent_mana_value if actor == "opponent" else player_mana_value
	bar.value = old_value
	value_label.text = "%s/%s" % [new_value, int(bar.max_value)]
	create_tween().tween_property(bar, "value", new_value, 0.25)


func _set_message(text: String) -> void:
	message_label.text = text


func _set_turn_prompt() -> void:
	var player: Dictionary = current_state.get("player", {})
	var player_display_name := String(player.get("name", "your player"))
	_set_message("What will %s do?" % player_display_name)


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
	output = output.replace("Mana", "[color=%s]Mana[/color]" % LOG_COLOR_MANA)
	output = output.replace("mana", "[color=%s]mana[/color]" % LOG_COLOR_MANA)
	output = output.replace("XP", "[color=%s]XP[/color]" % LOG_COLOR_XP)
	output = output.replace("LEVEL UP", "[color=%s]LEVEL UP[/color]" % LOG_COLOR_XP)
	return output


func _display_actor(actor: String) -> String:
	if actor == "player":
		return "Your player"
	if actor == "opponent":
		return "Opponent"
	return "Someone"


func _display_event_actor(event: Dictionary) -> String:
	var actor_name := String(event.get("actor_name", ""))
	if not actor_name.is_empty():
		return actor_name
	return _display_actor(String(event.get("actor", "none")))


func _display_event_target(event: Dictionary) -> String:
	var target_name := String(event.get("target_name", ""))
	if not target_name.is_empty():
		return target_name
	return _display_actor(String(event.get("target", "none")))


func _format_status_log(event: Dictionary) -> String:
	var actor := _display_event_actor(event)
	var target := _display_event_target(event)
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
		"attack_penetration_modifier":
			return "%s gained %s%% attack penetration." % [target, amount]
		"cooldown_modifier":
			if amount >= 0:
				return "%s cooldowns increased by %s%%." % [target, amount]
			return "%s cooldowns reduced by %s%%." % [target, abs(amount)]
		"healing_received_modifier":
			return "%s healing changed by %s%%." % [target, amount]
		"stunned":
			return "%s is stunned." % target
		"silenced":
			return "%s is silenced." % target
		"rooted":
			return "%s is rooted." % target
	return "%s changed %s's battle state." % [actor, target]


func _format_status_indicator(status: Dictionary) -> String:
	var entries: Array[String] = []
	var attack_percent := int(status.get("attack_modifier_percent", 0))
	var attack_turns := int(status.get("attack_modifier_turns", 0))
	var defense_percent := int(status.get("defense_modifier_percent", 0))
	var defense_turns := int(status.get("defense_modifier_turns", 0))
	var penetration_percent := int(status.get("attack_penetration_percent", 0))
	var penetration_turns := int(status.get("attack_penetration_turns", 0))
	var cooldown_percent := int(status.get("cooldown_modifier_percent", 0))
	var cooldown_turns := int(status.get("cooldown_modifier_turns", 0))
	var healing_percent := int(status.get("healing_received_modifier_percent", 0))
	var healing_turns := int(status.get("healing_received_modifier_turns", 0))

	if int(status.get("stunned_turns", 0)) > 0:
		entries.push_back("STUN %s" % status.get("stunned_turns", 0))
	if int(status.get("silenced_turns", 0)) > 0:
		entries.push_back("SILENCE %s" % status.get("silenced_turns", 0))
	if int(status.get("rooted_turns", 0)) > 0:
		entries.push_back("ROOT %s" % status.get("rooted_turns", 0))
	if int(status.get("mark_turns", 0)) > 0:
		entries.push_back("MARK %s" % status.get("mark_turns", 0))
	if attack_percent != 0 and attack_turns > 0:
		entries.push_back("%s ATK %s%% %st" % [_status_marker(attack_percent), _signed_value(attack_percent), attack_turns])
	if defense_percent != 0 and defense_turns > 0:
		entries.push_back("%s DEF %s%% %st" % [_status_marker(defense_percent), _signed_value(defense_percent), defense_turns])
	if penetration_percent != 0 and penetration_turns > 0:
		entries.push_back("PEN %s%% %st" % [_signed_value(penetration_percent), penetration_turns])
	if cooldown_percent != 0 and cooldown_turns > 0:
		entries.push_back("CD %s%% %st" % [_signed_value(cooldown_percent), cooldown_turns])
	if healing_percent != 0 and healing_turns > 0:
		entries.push_back("HEAL %s%% %st" % [_signed_value(healing_percent), healing_turns])
	if entries.is_empty():
		return "Status: none"
	return _join_strings(entries, "  ")


func _compact_status_indicator(status: Dictionary) -> String:
	var full_status := _format_status_indicator(status)
	if full_status == "Status: none":
		return ""
	return full_status


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
	for button in target_buttons:
		button.disabled = disabled
	back_button.disabled = disabled


func _show_return_button() -> void:
	input_locked = true
	_set_buttons_disabled(true)
	main_menu_grid.visible = false
	skill_grid.visible = false
	target_label.visible = false
	target_grid.visible = false
	back_button.visible = false
	return_button.visible = true


func _show_post_battle_panel(result: Dictionary) -> void:
	var summary := _apply_finished_battle_once(result)
	var lines := _build_reward_lines(result, summary)
	_show_return_button()
	return_button.text = "Return to map"
	var title := "VICTORY" if String(result.get("winner", "none")) == "player" else "BATTLE OVER"
	rewards_panel.show_rewards(title, lines)


func _apply_finished_battle_once(result: Dictionary) -> Dictionary:
	if rewards_applied:
		return GameState.get_trainer_state().get("last_battle_summary", {}).duplicate(true)

	var complete_result := result.duplicate(true)
	complete_result["events"] = battle_event_history.duplicate(true)
	rewards_applied = true
	return GameState.complete_battle(complete_result)


func _build_reward_lines(result: Dictionary, summary: Dictionary) -> Array[String]:
	var lines: Array[String] = []
	var summary_text := String(summary.get("text", "Battle complete."))
	if not summary_text.is_empty():
		lines.push_back("[center]%s[/center]" % summary_text)

	var reward: Dictionary = result.get("reward", {})
	if bool(reward.get("awarded", false)):
		lines.push_back("[color=%s]+%s team XP[/color]" % [LOG_COLOR_XP, reward.get("total_xp", 0)])
		lines.push_back("Split across %s participant(s): %s XP each." % [
			reward.get("participant_player_indices", []).size(),
			reward.get("xp_per_participant", 0),
		])

	for message in _collect_deferred_progress_lines():
		lines.push_back("[color=%s]%s[/color]" % [LOG_COLOR_XP, message])

	for message in summary.get("level_up_messages", []):
		var text := String(message)
		if text.begins_with("LEVEL UP"):
			lines.push_back("[font_size=34][color=%s]%s[/color][/font_size]" % [LOG_COLOR_XP, text])
		else:
			lines.push_back("[color=%s]%s[/color]" % [LOG_COLOR_XP, text])

	if lines.size() == 1:
		lines.push_back("No progression rewards.")
	return lines


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


func _collect_deferred_progress_lines() -> Array[String]:
	var lines: Array[String] = []
	for skill_name in deferred_skill_xp.keys():
		lines.push_back("%s gained %s skill XP." % [skill_name, deferred_skill_xp[skill_name]])
	for skill_name in deferred_skill_levels.keys():
		lines.push_back("LEVEL UP: %s reached Lv%s." % [skill_name, deferred_skill_levels[skill_name]])
	deferred_skill_xp.clear()
	deferred_skill_levels.clear()
	return lines


func _format_skill_id(skill_id: String) -> String:
	var parts := skill_id.split("-")
	var formatted: Array[String] = []
	for index in range(parts.size()):
		formatted.push_back(String(parts[index]).capitalize())
	return _join_strings(formatted, " ")


func _on_return_pressed() -> void:
	if not rewards_applied and bool(finished_result.get("battle_finished", false)):
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
