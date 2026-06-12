class_name BattleLineupPanel
extends PanelContainer

@onready var lineup_grid: GridContainer = $Margin/LineupGrid

var lineup_buttons: Array[Button] = []


func refresh(state: Dictionary) -> void:
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
		var status_text := BattleStatusPanel.compact_status_indicator(player.get("status", {}))
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
			_apply_card_style(button, false, true)
			button.tooltip_text = "Down"
		elif index == active_index:
			_apply_card_style(button, true, false)
			button.tooltip_text = "Current party turn"
		else:
			_apply_card_style(button, false, false)
			button.tooltip_text = "Waiting in party order"
		lineup_grid.add_child(button)
		lineup_buttons.push_back(button)


func _apply_card_style(button: Button, is_active: bool, is_down: bool) -> void:
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
