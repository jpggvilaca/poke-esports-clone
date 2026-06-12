extends Node2D

const STORY_DIALOGUE := preload("res://story_dialogue.gd")
const DEFAULT_MAP_MIN := Vector2(0, 0)
const DEFAULT_MAP_MAX := Vector2(1600, 1100)
const MAP_BOUNDS_MARGIN := 120.0
const PLAYER_MIN_EDGE_MARGIN := Vector2(40, 50)
const PLAYER_MAX_EDGE_MARGIN := Vector2(40, 40)
const WALKABLE_PATH_NAME := "Path"
const PLAYER_SPEED := 210.0
const INTERACT_DISTANCE := 82.0
const BUILDING_INTERACT_DISTANCE := 190.0

@onready var player_body: CharacterBody2D = $HumanPlayer
@onready var player_sprite: Sprite2D = $HumanPlayer/Sprite
@onready var player_camera: Camera2D = $HumanPlayer/Camera
@onready var npc_root: Node2D = $NPCs
@onready var lan_cafe: Node2D = $Buildings/LanCafe
@onready var major_hall: Node2D = $Buildings/MajorHall
@onready var map_ui: MapUI = $MapUI

var map_bounds := Rect2(DEFAULT_MAP_MIN, DEFAULT_MAP_MAX - DEFAULT_MAP_MIN)
var walkable_path_rects: Array[Rect2] = []
var player_frame_timer := 0.0
var npc_frame_timer := 0.0
var battle_triggered := false
var nearest_battle_npc: Node2D
var nearest_interaction := {}
var npc_base_rows: Dictionary = {}


func _ready() -> void:
	_update_map_bounds()
	_cache_walkable_path_rects()
	_configure_player_camera()
	player_body.global_position = _clamp_to_map(GameState.saved_map_position)
	if not _is_walkable_position(player_body.global_position):
		player_body.global_position = _nearest_walkable_position(player_body.global_position)
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
	var movement := _get_walkable_movement(direction, delta)
	var previous_position := player_body.global_position
	player_body.velocity = movement / delta if delta > 0.0 else Vector2.ZERO
	player_body.move_and_slide()
	player_body.global_position = _clamp_to_map(player_body.global_position)
	if not _is_walkable_position(player_body.global_position):
		player_body.global_position = previous_position
		player_body.velocity = Vector2.ZERO

	_animate_player(direction if movement != Vector2.ZERO else Vector2.ZERO, delta)
	_update_nearest_battle_npc()


func _update_map_bounds() -> void:
	var bounds := Rect2(DEFAULT_MAP_MIN, DEFAULT_MAP_MAX - DEFAULT_MAP_MIN)
	for root in [$Ground, $Buildings, $Objects, $NPCs]:
		bounds = _include_node_bounds(root, bounds)
	map_bounds = bounds.grow(MAP_BOUNDS_MARGIN)


func _include_node_bounds(node: Node, bounds: Rect2) -> Rect2:
	var expanded_bounds := bounds
	var node_bounds := _get_canvas_item_bounds(node)
	if node_bounds.size != Vector2.ZERO:
		expanded_bounds = expanded_bounds.merge(node_bounds)

	for child in node.get_children():
		expanded_bounds = _include_node_bounds(child, expanded_bounds)

	return expanded_bounds


func _get_canvas_item_bounds(node: Node) -> Rect2:
	if node is Sprite2D:
		var sprite := node as Sprite2D
		return _transform_rect(sprite.get_global_transform(), sprite.get_rect())

	if node is Control:
		var control := node as Control
		return control.get_global_rect()

	return Rect2()


func _transform_rect(transform: Transform2D, rect: Rect2) -> Rect2:
	var top_left := transform * rect.position
	var top_right := transform * (rect.position + Vector2(rect.size.x, 0.0))
	var bottom_left := transform * (rect.position + Vector2(0.0, rect.size.y))
	var bottom_right := transform * (rect.position + rect.size)
	var left = min(min(top_left.x, top_right.x), min(bottom_left.x, bottom_right.x))
	var top = min(min(top_left.y, top_right.y), min(bottom_left.y, bottom_right.y))
	var right = max(max(top_left.x, top_right.x), max(bottom_left.x, bottom_right.x))
	var bottom = max(max(top_left.y, top_right.y), max(bottom_left.y, bottom_right.y))
	return Rect2(Vector2(left, top), Vector2(right - left, bottom - top))


func _configure_player_camera() -> void:
	player_camera.enabled = true
	player_camera.limit_left = int(floor(map_bounds.position.x))
	player_camera.limit_top = int(floor(map_bounds.position.y))
	player_camera.limit_right = int(ceil(map_bounds.position.x + map_bounds.size.x))
	player_camera.limit_bottom = int(ceil(map_bounds.position.y + map_bounds.size.y))


func _cache_walkable_path_rects() -> void:
	walkable_path_rects.clear()
	_cache_walkable_path_rects_from($Ground)


func _cache_walkable_path_rects_from(node: Node) -> void:
	if node is ColorRect and String(node.name).contains(WALKABLE_PATH_NAME):
		var rect := (node as ColorRect).get_global_rect()
		if rect.size != Vector2.ZERO:
			walkable_path_rects.push_back(rect)

	for child in node.get_children():
		_cache_walkable_path_rects_from(child)


func _get_walkable_movement(direction: Vector2, delta: float) -> Vector2:
	if direction == Vector2.ZERO:
		return Vector2.ZERO

	var current_position := player_body.global_position
	var direct_movement := direction * PLAYER_SPEED * delta
	if _is_walkable_position(_clamp_to_map(current_position + direct_movement)):
		return direct_movement

	if direction.x != 0.0:
		var horizontal_movement := Vector2(sign(direction.x), 0.0) * PLAYER_SPEED * delta
		if _is_walkable_position(_clamp_to_map(current_position + horizontal_movement)):
			return horizontal_movement

	if direction.y != 0.0:
		var vertical_movement := Vector2(0.0, sign(direction.y)) * PLAYER_SPEED * delta
		if _is_walkable_position(_clamp_to_map(current_position + vertical_movement)):
			return vertical_movement

	return Vector2.ZERO


func _is_walkable_position(position: Vector2) -> bool:
	for rect in walkable_path_rects:
		if rect.has_point(position):
			return true
	return false


func _nearest_walkable_position(position: Vector2) -> Vector2:
	if walkable_path_rects.is_empty():
		return position

	var nearest_position := position
	var nearest_distance := INF
	for rect in walkable_path_rects:
		var candidate := Vector2(
			clamp(position.x, rect.position.x, rect.position.x + rect.size.x),
			clamp(position.y, rect.position.y, rect.position.y + rect.size.y)
		)
		var distance := position.distance_squared_to(candidate)
		if distance < nearest_distance:
			nearest_distance = distance
			nearest_position = candidate

	return _clamp_to_map(nearest_position)


func _clamp_to_map(position: Vector2) -> Vector2:
	var min_position := map_bounds.position + PLAYER_MIN_EDGE_MARGIN
	var max_position := map_bounds.position + map_bounds.size - PLAYER_MAX_EDGE_MARGIN
	if max_position.x < min_position.x:
		position.x = map_bounds.get_center().x
	else:
		position.x = clamp(position.x, min_position.x, max_position.x)
	if max_position.y < min_position.y:
		position.y = map_bounds.get_center().y
	else:
		position.y = clamp(position.y, min_position.y, max_position.y)
	return position


func _unhandled_input(event: InputEvent) -> void:
	if map_ui.is_dialog_open():
		return

	if event.is_action_pressed("ui_accept") and not nearest_interaction.is_empty() and not map_ui.is_trainer_visible():
		_use_nearest_interaction()
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
	_append_lineup_lines(lines, state)
	lines.push_back("Roster:")

	var active_index := int(state.get("active_player_index", 0))
	var lineup_indices: Array = state.get("lineup_indices", [])
	var roster: Array = state.get("roster", [])
	for index in range(roster.size()):
		var player: Dictionary = roster[index]
		var lineup_slot := lineup_indices.find(index)
		var lineup_marker := "   "
		if lineup_slot >= 0:
			lineup_marker = "[%s]" % (lineup_slot + 1)
		var first_turn_marker := ""
		if index == active_index and lineup_slot == 0:
			first_turn_marker = " first turn"
		lines.push_back("%s %s. %s - %s %s%s" % [
			lineup_marker,
			index + 1,
			player.get("name", "Player"),
			player.get("spec", "Spec"),
			player.get("rank", "Rookie"),
			first_turn_marker,
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


func _append_lineup_lines(lines: Array[String], state: Dictionary) -> void:
	var roster: Array = state.get("roster", [])
	var lineup_indices: Array = state.get("lineup_indices", [])
	lines.push_back("Battle lineup:")
	if lineup_indices.is_empty():
		lines.push_back("None selected.")
	else:
		var names: Array[String] = []
		for value in lineup_indices:
			var index := int(value)
			if index >= 0 and index < roster.size():
				var player: Dictionary = roster[index]
				names.push_back("%s. %s (%s)" % [
					index + 1,
					player.get("name", "Player"),
					player.get("spec", "Spec"),
				])
		lines.push_back(_join_strings(names, " | "))
	lines.push_back("Press 1-6 to toggle roster slots into the lineup (max 3).")
	lines.push_back("[1]-[3] = party turn order")
	lines.push_back("")


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
			lines.push_back("Dismiss scout to edit lineup slots 1-3.")

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
