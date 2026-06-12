class_name BattleSkillButton
extends Button

signal selected(skill: Dictionary)

const SKILL_COLORS := {
	"neutral": Color(0.32, 0.34, 0.38, 1.0),
	"top": Color(0.58, 0.46, 0.28, 1.0),
	"jungle": Color(0.20, 0.48, 0.30, 1.0),
	"mid": Color(0.36, 0.34, 0.70, 1.0),
	"adc": Color(0.70, 0.22, 0.20, 1.0),
	"support": Color(0.22, 0.48, 0.68, 1.0),
}

var skill_data: Dictionary = {}


func _ready() -> void:
	pressed.connect(_on_pressed)


func setup(skill: Dictionary, input_locked: bool) -> void:
	skill_data = skill.duplicate(true)
	text = _format_skill_button(skill_data)
	disabled = input_locked or not bool(skill_data.get("can_use", true))
	tooltip_text = ""
	_apply_skill_button_style(String(skill_data.get("skill_color_id", "neutral")))
	visible = true


func _on_pressed() -> void:
	selected.emit(skill_data.duplicate(true))


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


func _apply_skill_button_style(color_id: String) -> void:
	var base_color: Color = SKILL_COLORS.get(color_id, SKILL_COLORS["neutral"])
	var normal := StyleBoxFlat.new()
	normal.bg_color = base_color.darkened(0.32)
	normal.border_color = base_color.lightened(0.24)
	normal.set_border_width_all(2)
	normal.set_corner_radius_all(6)

	var hover := normal.duplicate()
	hover.bg_color = base_color.darkened(0.18)
	hover.border_color = base_color.lightened(0.36)

	var pressed_style := normal.duplicate()
	pressed_style.bg_color = base_color.darkened(0.42)

	var disabled_style := normal.duplicate()
	disabled_style.bg_color = Color(0.12, 0.12, 0.14, 0.86)
	disabled_style.border_color = Color(0.28, 0.28, 0.32, 0.8)

	add_theme_stylebox_override("normal", normal)
	add_theme_stylebox_override("hover", hover)
	add_theme_stylebox_override("pressed", pressed_style)
	add_theme_stylebox_override("focus", hover)
	add_theme_stylebox_override("disabled", disabled_style)
	add_theme_color_override("font_color", Color(0.95, 0.97, 1.0, 1.0))
	add_theme_color_override("font_disabled_color", Color(0.58, 0.60, 0.64, 1.0))
