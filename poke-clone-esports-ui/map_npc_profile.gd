class_name MapNpcProfile
extends StaticBody2D

@export var display_name := ""
@export var rating := 1000
@export var opponent_spec := ""


func get_display_name(fallback_name: String) -> String:
	return display_name if not display_name.is_empty() else fallback_name


func get_battle_overrides(fallback_name: String) -> Dictionary:
	var resolved_name := get_display_name(fallback_name)
	var rating_gap = rating - 1000
	var level = max(1, 1 + int(max(0, rating - 950) / 80))
	return {
		"display_name": resolved_name,
		"opponent_name": resolved_name,
		"opponent_spec": opponent_spec if not opponent_spec.is_empty() else "Mid",
		"opponent_rating": rating,
		"opponent_level": level,
		"opponent_hp": max(70, 90 + int(rating_gap / 4)),
		"opponent_mana": 100,
		"opponent_base_power_bonus": max(0, int(rating_gap / 70)),
		"money_reward": max(25, 50 + int(rating_gap / 8)),
	}
