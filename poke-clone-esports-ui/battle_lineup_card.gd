class_name BattleLineupCard
extends Button


func setup(player: Dictionary, index: int, active_index: int) -> void:
	var hp := int(player.get("hp", 0))
	var max_hp := int(player.get("max_hp", 1))
	var mana := int(player.get("mana", 0))
	var max_mana := int(player.get("max_mana", 1))
	var status_text := BattleStatusPanel.compact_status_indicator(player.get("status", {}))
	var state_text := "TURN" if index == active_index else "WAITING"
	if hp <= 0:
		state_text = "DOWN"

	text = "%s\n%s | %s\nHP %s/%s\nMana %s/%s%s" % [
		state_text,
		player.get("name", "Player"),
		player.get("spec", "Spec"),
		hp,
		max_hp,
		mana,
		max_mana,
		"\n%s" % status_text if not status_text.is_empty() else "",
	]
	tooltip_text = _build_tooltip(player, hp, max_hp, mana, max_mana)
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	focus_mode = Control.FOCUS_NONE
	if hp <= 0:
		_apply_card_style(false, true)
	elif index == active_index:
		_apply_card_style(true, false)
	else:
		_apply_card_style(false, false)
	visible = true


func _build_tooltip(player: Dictionary, hp: int, max_hp: int, mana: int, max_mana: int) -> String:
	var status_tooltip := BattleStatusPanel.format_status_tooltip(player.get("status", {}))
	return "%s\nHP %s/%s\nMana %s/%s\n%s" % [
		player.get("name", "Player"),
		hp,
		max_hp,
		mana,
		max_mana,
		status_tooltip,
	]


func _apply_card_style(is_active: bool, is_down: bool) -> void:
	var style := StyleBoxFlat.new()
	style.set_corner_radius_all(6)
	if is_down:
		style.bg_color = Color(0.10, 0.10, 0.12, 0.82)
		style.border_color = Color(0.32, 0.32, 0.36, 0.95)
		style.set_border_width_all(1)
		add_theme_color_override("font_color", Color(0.62, 0.62, 0.66, 1.0))
	elif is_active:
		style.bg_color = Color(0.15, 0.20, 0.32, 0.96)
		style.border_color = Color(0.95, 0.77, 0.23, 1.0)
		style.set_border_width_all(4)
		add_theme_color_override("font_color", Color(1.0, 0.95, 0.76, 1.0))
	else:
		style.bg_color = Color(0.08, 0.11, 0.18, 0.84)
		style.border_color = Color(0.28, 0.42, 0.67, 0.72)
		style.set_border_width_all(1)
		add_theme_color_override("font_color", Color(0.82, 0.88, 1.0, 1.0))

	for state_name in ["normal", "hover", "pressed", "focus"]:
		add_theme_stylebox_override(state_name, style)
