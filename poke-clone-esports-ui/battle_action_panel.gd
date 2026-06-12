class_name BattleActionPanel
extends VBoxContainer

signal fight_requested
signal drill_requested
signal quit_requested
signal skill_selected(skill: Dictionary)
signal target_selected(target_index: int)
signal return_requested

const SKILL_COLORS := {
	"neutral": Color(0.32, 0.34, 0.38, 1.0),
	"top": Color(0.58, 0.46, 0.28, 1.0),
	"jungle": Color(0.20, 0.48, 0.30, 1.0),
	"mid": Color(0.36, 0.34, 0.70, 1.0),
	"adc": Color(0.70, 0.22, 0.20, 1.0),
	"support": Color(0.22, 0.48, 0.68, 1.0),
}

@onready var main_menu_grid: GridContainer = $MainMenuGrid
@onready var skill_grid: GridContainer = $SkillGrid
@onready var target_label: Label = $TargetLabel
@onready var target_grid: GridContainer = $TargetGrid
@onready var fight_button: Button = $MainMenuGrid/FightButton
@onready var drill_button: Button = $MainMenuGrid/DrillButton
@onready var quit_button: Button = $MainMenuGrid/QuitButton
@onready var back_button: Button = $BackButton
@onready var return_button: Button = $ReturnButton

var skill_buttons: Array[Button] = []
var target_buttons: Array[Button] = []


func setup() -> void:
	fight_button.pressed.connect(_on_fight_button_pressed)
	drill_button.pressed.connect(_on_drill_button_pressed)
	quit_button.pressed.connect(_on_quit_button_pressed)
	back_button.pressed.connect(_on_back_button_pressed)
	return_button.pressed.connect(_on_return_button_pressed)
	drill_button.visible = false


func show_main_menu(input_locked: bool) -> void:
	main_menu_grid.visible = true
	skill_grid.visible = false
	target_label.visible = false
	target_grid.visible = false
	back_button.visible = false
	return_button.visible = false
	set_buttons_disabled(input_locked)


func show_fight_menu(skills: Array, input_locked: bool) -> void:
	refresh_skills(skills, input_locked)
	main_menu_grid.visible = false
	skill_grid.visible = true
	target_label.visible = false
	target_grid.visible = false
	back_button.visible = true


func show_target_menu(player_team: Array, input_locked: bool) -> void:
	refresh_targets(player_team, input_locked)
	main_menu_grid.visible = false
	skill_grid.visible = false
	target_label.visible = true
	target_grid.visible = true
	back_button.visible = true


func show_return_button() -> void:
	set_buttons_disabled(true)
	main_menu_grid.visible = false
	skill_grid.visible = false
	target_label.visible = false
	target_grid.visible = false
	back_button.visible = false
	return_button.visible = true


func set_return_text(text: String) -> void:
	return_button.text = text


func refresh_drill_action(drill: Dictionary, input_locked: bool) -> void:
	var display_name := String(drill.get("display_name", "Drill"))
	drill_button.text = display_name
	drill_button.tooltip_text = String(drill.get("description", ""))
	drill_button.disabled = input_locked or not bool(drill.get("can_use", true))


func refresh_skills(skills: Array, input_locked: bool) -> void:
	for child in skill_grid.get_children():
		child.queue_free()
	skill_buttons.clear()

	for skill in skills:
		var button := Button.new()
		button.text = _format_skill_button(skill)
		button.custom_minimum_size = Vector2(270, 112)
		button.disabled = input_locked or not bool(skill.get("can_use", true))
		_apply_skill_button_style(button, String(skill.get("skill_color_id", "neutral")))
		button.pressed.connect(_on_skill_button_pressed.bind(skill.duplicate(true)))
		skill_grid.add_child(button)
		skill_buttons.push_back(button)


func refresh_targets(player_team: Array, input_locked: bool) -> void:
	for child in target_grid.get_children():
		child.queue_free()
	target_buttons.clear()

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
		button.pressed.connect(_on_target_button_pressed.bind(index))
		target_grid.add_child(button)
		target_buttons.push_back(button)


func set_buttons_disabled(disabled: bool) -> void:
	fight_button.disabled = disabled
	quit_button.disabled = disabled
	back_button.disabled = disabled
	for button in skill_buttons:
		button.disabled = disabled
	for button in target_buttons:
		button.disabled = disabled


func _on_skill_button_pressed(skill: Dictionary) -> void:
	skill_selected.emit(skill)


func _on_target_button_pressed(target_index: int) -> void:
	target_selected.emit(target_index)


func _on_fight_button_pressed() -> void:
	fight_requested.emit()


func _on_drill_button_pressed() -> void:
	drill_requested.emit()


func _on_quit_button_pressed() -> void:
	quit_requested.emit()


func _on_back_button_pressed() -> void:
	show_main_menu(false)


func _on_return_button_pressed() -> void:
	return_requested.emit()


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
