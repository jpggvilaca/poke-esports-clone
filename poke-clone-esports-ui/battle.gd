extends Control

var bridge: BattleBridge

@onready var rewards_panel: RewardsPanel = $RewardsPanel
@onready var drill_minigame: DrillMinigame = $DrillMinigame
@onready var opponent_status: BattleStatusPanel = $OpponentStatus
@onready var player_status: BattleStatusPanel = $PlayerStatus
@onready var message_log: BattleMessageLog = $BottomPanel/BottomSplit/TextBox
@onready var action_panel: BattleActionPanel = $BottomPanel/BottomSplit/ActionBox
@onready var lineup_panel: BattleLineupPanel = $LineupPanel

var reward_presenter := BattleRewardPresenter.new()
var event_player := BattleEventPlayer.new()
var battle_event_history: Array[Dictionary] = []
var input_locked := false
var finished_result: Dictionary = {}
var rewards_applied := false
var current_state: Dictionary = {}
var pending_skill: Dictionary = {}


func _ready() -> void:
	_setup_children()
	if not _setup_bridge():
		return

	var result := bridge.start_battle(GameState.build_battle_setup())
	_refresh_drill_action()

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
	await _maybe_auto_pass_stunned_turn()


func _setup_children() -> void:
	action_panel.setup()
	action_panel.fight_requested.connect(_on_fight_requested)
	action_panel.drill_requested.connect(_on_drill_requested)
	action_panel.quit_requested.connect(_on_quit_requested)
	action_panel.skill_selected.connect(_on_skill_selected)
	action_panel.target_selected.connect(_on_target_selected)
	action_panel.return_requested.connect(_on_return_pressed)
	rewards_panel.return_requested.connect(_on_return_pressed)
	drill_minigame.drill_finished.connect(_on_drill_minigame_finished)
	drill_minigame.drill_cancelled.connect(_on_drill_minigame_cancelled)
	add_child(event_player)
	event_player.setup(message_log, player_status, opponent_status, reward_presenter)


func _setup_bridge() -> bool:
	bridge = BattleBridge.new()
	if not is_instance_valid(bridge):
		_show_bridge_error("Battle system failed to load. Check the GDExtension build in res://bin.")
		return false

	bridge.name = "BattleBridge"
	add_child(bridge)
	return true


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
	_refresh_drill_action()


func _show_fight_menu() -> void:
	action_panel.show_fight_menu(bridge.get_available_skills(), input_locked)


func _show_target_menu(skill: Dictionary) -> void:
	pending_skill = skill.duplicate(true)
	var state := bridge.get_battle_state()
	action_panel.show_target_menu(state.get("player_team", []), input_locked)


func _on_fight_requested() -> void:
	if input_locked:
		return
	_show_fight_menu()


func _on_drill_requested() -> void:
	if input_locked:
		return

	var drill := bridge.get_drill_action()
	if not bool(drill.get("can_use", true)):
		message_log.set_message(String(drill.get("disabled_reason", "That action is unavailable.")))
		return

	_lock_input()
	drill_minigame.start(drill)


func _on_drill_minigame_finished(quality: String) -> void:
	var result = bridge.use_drill(quality)
	if not result.get("accepted", false):
		message_log.set_message(result.get("error", "That action failed."))
		_unlock_input()
		return

	_record_result_events(result)
	await _play_events(result.get("events", []))
	_update_state(result.get("state", bridge.get_battle_state()))
	await _continue_or_finish(result)


func _on_drill_minigame_cancelled() -> void:
	_unlock_input()
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

	_lock_input()
	message_log.set_message("%s is stunned and loses the turn." % player.get("name", "Your player"))
	await get_tree().create_timer(0.35).timeout

	var result := bridge.pass_turn()
	if not result.get("accepted", false):
		message_log.set_message(result.get("error", "That action failed."))
		_unlock_input()
		_show_main_menu()
		return

	_record_result_events(result)
	await _play_events(result.get("events", []))
	_update_state(result.get("state", bridge.get_battle_state()))
	await _continue_or_finish(result)


func _refresh_drill_action() -> void:
	if bridge == null:
		return
	action_panel.refresh_drill_action(bridge.get_drill_action(), input_locked)


func _refresh_skills() -> void:
	action_panel.refresh_skills(bridge.get_available_skills(), input_locked)


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


func _use_skill(skill_id: String, target_index: int) -> void:
	_lock_input()
	var result := bridge.use_skill(skill_id, target_index)
	if not result.get("accepted", false):
		message_log.set_message(result.get("error", "That action failed."))
		_unlock_input()
		return

	_record_result_events(result)
	await _play_events(result.get("events", []))
	_update_state(result.get("state", bridge.get_battle_state()))
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
	_refresh_drill_action()
	_set_turn_prompt()
	_unlock_input()
	_show_main_menu()
	await _maybe_auto_pass_stunned_turn()


func _play_events(events: Array) -> void:
	await event_player.play_events(events, current_state)


func _update_state(state: Dictionary) -> void:
	current_state = state.duplicate(true)
	player_status.set_status(current_state.get("player", {}), "Your Player")
	opponent_status.set_status(current_state.get("opponent", {}), "Opponent")
	lineup_panel.refresh(current_state)


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
