extends Node2D

const STORY_DIALOGUE := preload("res://story_dialogue.gd")
const MAP_SIZE := Vector2(1600, 1100)
const PLAYER_SPEED := 210.0
const INTERACT_DISTANCE := 82.0
const BUILDING_INTERACT_DISTANCE := 190.0

@onready var player_body: CharacterBody2D = $HumanPlayer
@onready var player_sprite: Sprite2D = $HumanPlayer/Sprite
@onready var npc_root: Node2D = $NPCs
@onready var lan_cafe: Node2D = $Buildings/LanCafe
@onready var major_hall: Node2D = $Buildings/MajorHall
@onready var map_ui: MapUI = $MapUI

var player_frame_timer := 0.0
var npc_frame_timer := 0.0
var battle_triggered := false
var nearest_battle_npc: Node2D
var nearest_interaction := {}
var npc_base_rows: Dictionary = {}


func _ready() -> void:
	player_body.global_position = GameState.saved_map_position
	_cache_npc_directions()
	_refresh_npc_states()
	_refresh_trainer_menu()


func _physics_process(delta: float) -> void:
	if battle_triggered:
		return

	_animate_npcs(delta)

	if map_ui.is_trainer_visible() or map_ui.is_dialog_open():
		player_body.velocity = Vector2.ZERO
		_animate_player(Vector2.ZERO, delta)
		return

	var direction := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	player_body.velocity = direction * PLAYER_SPEED
	player_body.move_and_slide()
	player_body.global_position.x = clamp(player_body.global_position.x, 40.0, MAP_SIZE.x - 40.0)
	player_body.global_position.y = clamp(player_body.global_position.y, 50.0, MAP_SIZE.y - 40.0)

	_animate_player(direction, delta)
	_update_nearest_battle_npc()


func _unhandled_input(event: InputEvent) -> void:
	if map_ui.is_dialog_open():
		return

	if event.is_action_pressed("ui_accept") and not nearest_interaction.is_empty() and not map_ui.is_trainer_visible():
		_use_nearest_interaction()
		return

	if event is InputEventKey and event.pressed and not event.echo and map_ui.is_trainer_visible():
		if event.keycode == KEY_1 or event.keycode == KEY_KP_1:
			_handle_scout_candidate_choice(0)
			return
		if event.keycode == KEY_2 or event.keycode == KEY_KP_2:
			_handle_scout_candidate_choice(1)
			return
		if event.keycode == KEY_3 or event.keycode == KEY_KP_3:
			_handle_scout_candidate_choice(2)
			return
		if event.keycode == KEY_D:
			GameState.decline_scout_offer()
			_refresh_trainer_menu()
			return

	if event is InputEventKey and event.pressed and not event.echo and event.keycode == KEY_M:
		_toggle_trainer_menu()


func _animate_player(direction: Vector2, delta: float) -> void:
	var row := player_sprite.frame_coords.y
	if abs(direction.x) > abs(direction.y):
		row = 2 if direction.x > 0.0 else 1
	elif direction.y != 0.0:
		row = 0 if direction.y > 0.0 else 3

	if direction == Vector2.ZERO:
		player_frame_timer = 0.0
		player_sprite.frame_coords = Vector2i(0, row)
		return

	player_frame_timer += delta
	var frame := int(player_frame_timer * 8.0) % 4
	player_sprite.frame_coords = Vector2i(frame, row)


func _cache_npc_directions() -> void:
	for npc in npc_root.get_children():
		if not (npc is Node2D) or not npc.is_in_group("battle_npc"):
			continue
		var sprite := npc.get_node_or_null("Sprite")
		if sprite is Sprite2D:
			npc_base_rows[String(npc.name)] = sprite.frame_coords.y


func _animate_npcs(delta: float) -> void:
	npc_frame_timer += delta
	var npc_index := 0
	for npc in npc_root.get_children():
		if not (npc is Node2D) or not npc.is_in_group("battle_npc"):
			continue

		var sprite := npc.get_node_or_null("Sprite")
		if not (sprite is Sprite2D):
			continue

		var npc_id := String(npc.name)
		var row := int(npc_base_rows.get(npc_id, sprite.frame_coords.y))
		if not GameState.is_npc_defeated(npc_id):
			var to_player = player_body.global_position - npc.global_position
			if to_player.length() <= INTERACT_DISTANCE:
				row = _direction_row(to_player)
			sprite.frame_coords = Vector2i((int(npc_frame_timer * 4.0) + npc_index) % 4, row)
		else:
			sprite.frame_coords = Vector2i(0, row)

		npc_index += 1


func _direction_row(direction: Vector2) -> int:
	if abs(direction.x) > abs(direction.y):
		return 2 if direction.x > 0.0 else 1
	if direction.y != 0.0:
		return 0 if direction.y > 0.0 else 3
	return 0


func _update_nearest_battle_npc() -> void:
	nearest_battle_npc = null
	nearest_interaction.clear()
	var nearest_distance := BUILDING_INTERACT_DISTANCE + 1.0
	for npc in npc_root.get_children():
		if not (npc is Node2D) or not npc.is_in_group("battle_npc"):
			continue
		if GameState.is_npc_defeated(String(npc.name)):
			continue

		var distance := player_body.global_position.distance_to(npc.global_position)
		if distance <= INTERACT_DISTANCE and distance < nearest_distance:
			nearest_distance = distance
			nearest_battle_npc = npc
			nearest_interaction = {"type": "battle_npc", "node": npc, "label": "Press Enter to battle %s" % GameState.get_npc_display_name(String(npc.name))}

	var recovery_distance := player_body.global_position.distance_to(lan_cafe.global_position)
	if recovery_distance <= BUILDING_INTERACT_DISTANCE and recovery_distance < nearest_distance:
		nearest_distance = recovery_distance
		nearest_interaction = {"type": "recovery", "label": "Press Enter to recover at LAN CAFE"}

	var tournament_distance := player_body.global_position.distance_to(major_hall.global_position)
	if tournament_distance <= BUILDING_INTERACT_DISTANCE and tournament_distance < nearest_distance:
		nearest_distance = tournament_distance
		nearest_interaction = {"type": "tournament", "label": GameState.get_tournament_prompt()}

	if nearest_interaction.is_empty():
		map_ui.hide_prompt()
		return

	map_ui.show_prompt(String(nearest_interaction.get("label", "Press Enter")))


func _use_nearest_interaction() -> void:
	match String(nearest_interaction.get("type", "")):
		"battle_npc":
			var npc = nearest_interaction.get("node")
			if npc is Node2D:
				_start_npc_battle(npc)
		"recovery":
			map_ui.show_prompt(GameState.recover_roster())
			_refresh_trainer_menu()
		"tournament":
			battle_triggered = true
			if GameState.prepare_tournament_battle(player_body.global_position):
				await map_ui.show_dialog(STORY_DIALOGUE.get_tournament_intro())
				get_tree().change_scene_to_file("res://battle.tscn")
			else:
				map_ui.show_prompt(GameState.get_tournament_locked_message())
				battle_triggered = false


func _start_npc_battle(npc: Node2D) -> void:
	battle_triggered = true
	await map_ui.show_dialog(STORY_DIALOGUE.get_npc_intro(String(npc.name)))
	if GameState.prepare_npc_battle(String(npc.name), player_body.global_position):
		get_tree().change_scene_to_file("res://battle.tscn")
		return

	battle_triggered = false
	_refresh_npc_states()
	_update_nearest_battle_npc()


func _refresh_npc_states() -> void:
	for npc in npc_root.get_children():
		if not (npc is Node2D) or not npc.is_in_group("battle_npc"):
			continue

		var npc_id := String(npc.name)
		var defeated := GameState.is_npc_defeated(npc_id)
		var sprite := npc.get_node_or_null("Sprite")
		if sprite is CanvasItem:
			sprite.modulate = Color(0.55, 0.55, 0.55, 0.85) if defeated else Color.WHITE

		var label := npc.get_node_or_null("%sLabel" % npc.name)
		if label is Label:
			label.text = "%s\nDefeated" % GameState.get_npc_display_name(npc_id) if defeated else GameState.get_npc_display_name(npc_id)


func _toggle_trainer_menu() -> void:
	map_ui.toggle_trainer()
	_refresh_trainer_menu()


func _refresh_trainer_menu() -> void:
	map_ui.set_trainer_text(_format_trainer_text())


func _handle_scout_candidate_choice(candidate_index: int) -> void:
	GameState.accept_scout_candidate(candidate_index)
	_refresh_trainer_menu()


func _format_trainer_text() -> String:
	var state := GameState.get_trainer_state()
	var lines: Array[String] = []
	lines.push_back("%s" % state.get("trainer_name", "Trainer"))
	lines.push_back("Rating: %s" % state.get("rating", 0))
	lines.push_back("Major Hall entry: %s rating" % state.get("major_hall_required_rating", 0))
	lines.push_back("Money: %s" % state.get("money", 0))
	lines.push_back("Trophies: %s" % _format_trophies(state.get("trophies", [])))
	lines.push_back("")
	lines.push_back("Last battle:")
	lines.push_back("%s" % state.get("last_battle_summary", {}).get("text", "No battles yet."))
	for message in state.get("last_battle_summary", {}).get("level_up_messages", []):
		lines.push_back("%s" % message)
	lines.push_back("")
	_append_scout_lines(lines, state)
	lines.push_back("Roster:")

	var active_index := int(state.get("active_player_index", 0))
	var roster: Array = state.get("roster", [])
	for index in range(roster.size()):
		var player: Dictionary = roster[index]
		var marker := ">" if index == active_index else " "
		lines.push_back("%s %s - %s %s" % [
			marker,
			player.get("name", "Player"),
			player.get("spec", "Spec"),
			player.get("rank", "Rookie"),
		])
		var identity_name := String(player.get("trait_name", ""))
		if not identity_name.is_empty():
			lines.push_back("  Trait: %s" % identity_name)
		lines.push_back("  Level %s | XP %s/%s" % [
			player.get("level", 1),
			player.get("xp", 0),
			player.get("xp_required_for_next_level", 100),
		])
		lines.push_back("  HP %s/%s | Mana %s/%s" % [
			player.get("current_hp", player.get("max_hp", 100)),
			player.get("max_hp", 100),
			player.get("current_mana", 0),
			player.get("max_mana", 100),
		])
		var passive_bonuses: Dictionary = player.get("passive_bonuses", {})
		lines.push_back("  Bonuses: Max HP +%s | Base Power +%s | Counter +%s%%" % [
			passive_bonuses.get("max_hp_bonus", 0),
			passive_bonuses.get("base_power_bonus", 0),
			passive_bonuses.get("counter_damage_bonus_percent", 0),
		])
		lines.push_back("  Active skills:")
		var progress_by_skill: Dictionary = player.get("skill_progress", {})
		for skill_id in player.get("active_skill_ids", []):
			var progress: Dictionary = progress_by_skill.get(skill_id, {})
			var skill_summary: Dictionary = GameState.get_skill_progress_summary(player, String(skill_id), progress)
			lines.push_back("   - %s Lv%s XP %s | Mana %s CD %s" % [
				skill_summary.get("name", GameState.format_skill_name(String(skill_id))),
				skill_summary.get("level", 1),
				skill_summary.get("xp", 0),
				skill_summary.get("mana_cost", 0),
				skill_summary.get("cooldown_turns", 0),
			])
		lines.push_back("")

	lines.push_back("Press M to close.")
	return _join_strings(lines, "\n")


func _append_scout_lines(lines: Array[String], state: Dictionary) -> void:
	var scout_message := String(state.get("last_scout_message", ""))
	var scout_offer: Dictionary = state.get("pending_scout_offer", {})
	if scout_message.is_empty() and scout_offer.is_empty():
		return

	lines.push_back("Scout:")
	if not scout_message.is_empty():
		lines.push_back(scout_message)

	if not scout_offer.is_empty():
		lines.push_back("Shortlist at %s rating:" % scout_offer.get("required_rating", 0))
		var candidates: Array = scout_offer.get("candidates", [])
		for index in range(candidates.size()):
			var candidate: Dictionary = candidates[index]
			var spec_text := String(candidate.get("spec", "Spec"))
			var identity_name := String(candidate.get("trait_name", ""))
			if not identity_name.is_empty():
				spec_text += " | %s" % identity_name

			lines.push_back("%s. %s - %s" % [
				index + 1,
				candidate.get("name", "Prospect"),
				spec_text,
			])
			lines.push_back("   Level %s | Skills: %s" % [
				candidate.get("level", 1),
				_format_candidate_skills(candidate),
			])

		var roster: Array = state.get("roster", [])
		var max_roster_size := int(state.get("max_roster_size", 6))
		if roster.size() >= max_roster_size:
			lines.push_back("Roster full. Free space before choosing.")
		else:
			lines.push_back("Press 1-3 to recruit. Press D to dismiss.")

	lines.push_back("")


func _format_candidate_skills(candidate: Dictionary) -> String:
	var skill_names: Array[String] = []
	var progress_by_skill: Dictionary = candidate.get("skill_progress", {})
	for skill_id in candidate.get("active_skill_ids", []):
		var progress: Dictionary = progress_by_skill.get(skill_id, {})
		var skill_summary: Dictionary = GameState.get_skill_progress_summary(candidate, String(skill_id), progress)
		skill_names.push_back("%s M%s/CD%s" % [
			skill_summary.get("name", GameState.format_skill_name(String(skill_id))),
			skill_summary.get("mana_cost", 0),
			skill_summary.get("cooldown_turns", 0),
		])
	if skill_names.is_empty():
		return "None"
	return _join_strings(skill_names, ", ")


func _format_trophies(values: Array) -> String:
	if values.is_empty():
		return "None"
	var trophies_text: Array[String] = []
	for value in values:
		trophies_text.push_back(String(value))
	return _join_strings(trophies_text, ", ")


func _join_strings(values: Array[String], separator: String) -> String:
	var output := ""
	for index in range(values.size()):
		if index > 0:
			output += separator
		output += values[index]
	return output
