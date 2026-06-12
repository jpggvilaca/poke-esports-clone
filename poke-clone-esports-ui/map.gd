extends Node2D

const STORY_DIALOGUE := preload("res://story_dialogue.gd")

@onready var world_bounds: MapWorldBounds = $Ground
@onready var player_controller: MapPlayerController = $HumanPlayer
@onready var npc_controller: MapNpcController = $NPCs
@onready var lan_cafe: Node2D = $Buildings/LanCafe
@onready var major_hall: Node2D = $Buildings/MajorHall
@onready var map_ui: MapUI = $MapUI

var interaction_controller := MapInteractionController.new()
var trainer_formatter := TrainerTextFormatter.new()
var battle_triggered := false


func _ready() -> void:
	world_bounds.setup([$Ground, $Buildings, $Objects, $NPCs])
	player_controller.setup(world_bounds, GameState.saved_map_position)
	npc_controller.setup()
	interaction_controller.setup(npc_controller, lan_cafe, major_hall)
	_refresh_trainer_menu()
	_update_nearest_interaction()


func _physics_process(delta: float) -> void:
	if battle_triggered:
		return

	var player_position := player_controller.get_player_position()
	npc_controller.tick_animation(delta, player_position)

	var ui_blocking := map_ui.is_trainer_visible() or map_ui.is_dialog_open()
	player_controller.set_input_enabled(not ui_blocking)
	player_controller.tick_movement(delta)
	if ui_blocking:
		map_ui.hide_prompt()
		return
	_update_nearest_interaction()


func _unhandled_input(event: InputEvent) -> void:
	if map_ui.is_dialog_open():
		return

	if event.is_action_pressed("ui_accept") and not map_ui.is_trainer_visible():
		var interaction := interaction_controller.get_current_interaction()
		if not interaction.is_empty():
			_use_interaction(interaction)
			return

	if event is InputEventKey and event.pressed and not event.echo and map_ui.is_trainer_visible():
		var number_key := _number_key_to_index(event)
		if number_key >= 0:
			_handle_trainer_number_key(number_key)
			return
		if event.keycode == KEY_D:
			GameState.decline_scout_offer()
			_refresh_trainer_menu()
			return

	if event is InputEventKey and event.pressed and not event.echo and event.keycode == KEY_M:
		_toggle_trainer_menu()


func _update_nearest_interaction() -> void:
	var interaction := interaction_controller.update(player_controller.get_player_position())
	if interaction.is_empty():
		map_ui.hide_prompt()
		return

	map_ui.show_prompt(String(interaction.get("label", "Press Enter")))


func _use_interaction(interaction: Dictionary) -> void:
	match String(interaction.get("type", "")):
		"battle_npc":
			var npc = interaction.get("node")
			if npc is Node2D:
				_start_npc_battle(npc)
		"recovery":
			map_ui.show_prompt(GameState.recover_roster())
			_refresh_trainer_menu()
		"tournament":
			_start_tournament_battle()


func _start_tournament_battle() -> void:
	battle_triggered = true
	if GameState.prepare_tournament_battle(player_controller.get_player_position()):
		await map_ui.show_dialog(STORY_DIALOGUE.get_tournament_intro())
		get_tree().change_scene_to_file("res://battle.tscn")
		return

	map_ui.show_prompt(GameState.get_tournament_locked_message())
	battle_triggered = false


func _start_npc_battle(npc: Node2D) -> void:
	battle_triggered = true
	await map_ui.show_dialog(STORY_DIALOGUE.get_npc_intro(String(npc.name)))
	if GameState.prepare_npc_battle(String(npc.name), player_controller.get_player_position()):
		get_tree().change_scene_to_file("res://battle.tscn")
		return

	battle_triggered = false
	npc_controller.refresh_defeated_states()
	_update_nearest_interaction()


func _toggle_trainer_menu() -> void:
	map_ui.toggle_trainer()
	_refresh_trainer_menu()


func _refresh_trainer_menu() -> void:
	map_ui.set_trainer_text(trainer_formatter.format_trainer_text(GameState.get_trainer_state()))


func _handle_scout_candidate_choice(candidate_index: int) -> void:
	GameState.accept_scout_candidate(candidate_index)
	_refresh_trainer_menu()


func _handle_trainer_number_key(number_index: int) -> void:
	var state := GameState.get_trainer_state()
	var scout_offer: Dictionary = state.get("pending_scout_offer", {})
	if not scout_offer.is_empty() and number_index >= 0 and number_index < 3:
		_handle_scout_candidate_choice(number_index)
		return

	map_ui.show_prompt(GameState.toggle_lineup_member(number_index))
	_refresh_trainer_menu()


func _number_key_to_index(event: InputEventKey) -> int:
	match event.keycode:
		KEY_1, KEY_KP_1:
			return 0
		KEY_2, KEY_KP_2:
			return 1
		KEY_3, KEY_KP_3:
			return 2
		KEY_4, KEY_KP_4:
			return 3
		KEY_5, KEY_KP_5:
			return 4
		KEY_6, KEY_KP_6:
			return 5
	return -1
