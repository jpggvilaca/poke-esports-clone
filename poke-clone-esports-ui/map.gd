extends Node2D

const MAP_SIZE := Vector2(1600, 1100)
const PLAYER_SPEED := 210.0
const INTERACT_DISTANCE := 82.0

@onready var player_body: CharacterBody2D = $HumanPlayer
@onready var player_sprite: Sprite2D = $HumanPlayer/Sprite
@onready var npc_root: Node2D = $NPCs

var player_frame_timer := 0.0
var npc_frame_timer := 0.0
var battle_triggered := false
var nearest_battle_npc: Node2D
var npc_base_rows: Dictionary = {}
var prompt_panel: PanelContainer
var prompt_label: Label
var trainer_panel: PanelContainer
var trainer_text: Label


func _ready() -> void:
	player_body.global_position = GameState.saved_map_position
	_cache_npc_directions()
	_build_map_ui()
	_refresh_npc_states()
	_refresh_trainer_menu()


func _physics_process(delta: float) -> void:
	if battle_triggered:
		return

	_animate_npcs(delta)

	if trainer_panel.visible:
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
	if event.is_action_pressed("ui_accept") and nearest_battle_npc != null and not trainer_panel.visible:
		_start_npc_battle(nearest_battle_npc)
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


func _build_map_ui() -> void:
	var canvas := CanvasLayer.new()
	canvas.name = "MapUI"
	add_child(canvas)

	prompt_panel = PanelContainer.new()
	prompt_panel.visible = false
	prompt_panel.anchor_left = 0.5
	prompt_panel.anchor_top = 1.0
	prompt_panel.anchor_right = 0.5
	prompt_panel.anchor_bottom = 1.0
	prompt_panel.offset_left = -260
	prompt_panel.offset_top = -128
	prompt_panel.offset_right = 260
	prompt_panel.offset_bottom = -68
	prompt_panel.add_theme_stylebox_override("panel", _panel_style(Color(0.95, 0.92, 0.78), Color(0.18, 0.18, 0.16), 8))
	canvas.add_child(prompt_panel)

	prompt_label = Label.new()
	prompt_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	prompt_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	prompt_label.add_theme_font_size_override("font_size", 24)
	prompt_panel.add_child(prompt_label)

	trainer_panel = PanelContainer.new()
	trainer_panel.visible = false
	trainer_panel.anchor_left = 0.61
	trainer_panel.anchor_top = 0.04
	trainer_panel.anchor_right = 0.97
	trainer_panel.anchor_bottom = 0.88
	trainer_panel.add_theme_stylebox_override("panel", _panel_style(Color(0.94, 0.91, 0.79), Color(0.18, 0.18, 0.16), 8))
	canvas.add_child(trainer_panel)

	var margin := MarginContainer.new()
	margin.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	margin.size_flags_vertical = Control.SIZE_EXPAND_FILL
	margin.add_theme_constant_override("margin_left", 18)
	margin.add_theme_constant_override("margin_top", 16)
	margin.add_theme_constant_override("margin_right", 18)
	margin.add_theme_constant_override("margin_bottom", 16)
	trainer_panel.add_child(margin)

	var scroll := ScrollContainer.new()
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	margin.add_child(scroll)

	trainer_text = Label.new()
	trainer_text.custom_minimum_size = Vector2(560, 0)
	trainer_text.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	trainer_text.size_flags_vertical = Control.SIZE_EXPAND_FILL
	trainer_text.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	trainer_text.add_theme_font_size_override("font_size", 20)
	trainer_text.modulate = Color(0.10, 0.10, 0.09)
	scroll.add_child(trainer_text)


func _panel_style(color: Color, border: Color, radius: int) -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = color
	style.border_color = border
	style.set_border_width_all(4)
	style.set_corner_radius_all(radius)
	style.content_margin_left = 16
	style.content_margin_right = 16
	style.content_margin_top = 10
	style.content_margin_bottom = 10
	return style


func _update_nearest_battle_npc() -> void:
	nearest_battle_npc = null
	var nearest_distance := INTERACT_DISTANCE
	for npc in npc_root.get_children():
		if not (npc is Node2D) or not npc.is_in_group("battle_npc"):
			continue
		if GameState.is_npc_defeated(String(npc.name)):
			continue

		var distance := player_body.global_position.distance_to(npc.global_position)
		if distance < nearest_distance:
			nearest_distance = distance
			nearest_battle_npc = npc

	if nearest_battle_npc == null:
		prompt_panel.visible = false
		return

	prompt_label.text = "Press Enter to battle %s" % GameState.get_npc_display_name(String(nearest_battle_npc.name))
	prompt_panel.visible = true


func _start_npc_battle(npc: Node2D) -> void:
	battle_triggered = true
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
	trainer_panel.visible = not trainer_panel.visible
	prompt_panel.visible = false if trainer_panel.visible else prompt_panel.visible
	_refresh_trainer_menu()


func _refresh_trainer_menu() -> void:
	trainer_text.text = _format_trainer_text()


func _format_trainer_text() -> String:
	var state := GameState.get_trainer_state()
	var lines: Array[String] = []
	lines.push_back("%s" % state.get("trainer_name", "Trainer"))
	lines.push_back("Rating: %s" % state.get("rating", 0))
	lines.push_back("Money: %s" % state.get("money", 0))
	lines.push_back("Trophies: %s" % _format_trophies(state.get("trophies", [])))
	lines.push_back("")
	lines.push_back("Last battle:")
	lines.push_back("%s" % state.get("last_battle_summary", {}).get("text", "No battles yet."))
	lines.push_back("")
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
		lines.push_back("  Level %s | XP %s/%s | Style %s" % [
			player.get("level", 1),
			player.get("xp", 0),
			player.get("xp_required_for_next_level", 100),
			player.get("style", "Balanced"),
		])
		lines.push_back("  HP %s/%s | Focus %s/%s" % [
			player.get("current_hp", player.get("max_hp", 100)),
			player.get("max_hp", 100),
			player.get("current_focus", player.get("max_focus", 50)),
			player.get("max_focus", 50),
		])
		lines.push_back("  Active skills:")
		var progress_by_skill: Dictionary = player.get("skill_progress", {})
		for skill_id in player.get("active_skill_ids", []):
			var progress: Dictionary = progress_by_skill.get(skill_id, {})
			lines.push_back("   - %s Lv%s XP %s" % [
				GameState.format_skill_name(String(skill_id)),
				progress.get("level", 1),
				progress.get("xp", 0),
			])
		lines.push_back("")

	lines.push_back("Press M to close.")
	return _join_strings(lines, "\n")


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
