class_name BattleRewardPresenter
extends RefCounted

const LOG_COLOR_XP := "#f3d35b"
const MAX_SKILL_XP_PER_BATTLE := 50

var deferred_skill_xp: Dictionary = {}
var deferred_skill_levels: Dictionary = {}


func defer_progress_event(event: Dictionary) -> void:
	if String(event.get("actor", "none")) != "player":
		return

	var skill_name := format_skill_id(String(event.get("skill_id", "skill")))
	match String(event.get("type", "none")):
		BattleEventTypes.SKILL_XP_GAINED:
			var current_xp := int(deferred_skill_xp.get(skill_name, 0))
			deferred_skill_xp[skill_name] = min(MAX_SKILL_XP_PER_BATTLE, current_xp + int(event.get("amount", 0)))
		BattleEventTypes.SKILL_LEVELED_UP:
			deferred_skill_levels[skill_name] = int(event.get("new_level", 1))


func build_reward_lines(result: Dictionary, summary: Dictionary) -> Array[String]:
	var lines: Array[String] = []
	var summary_text := String(summary.get("text", "Battle complete."))
	if not summary_text.is_empty():
		lines.push_back("[center]%s[/center]" % summary_text)

	var reward: Dictionary = result.get("reward", {})
	if bool(reward.get("awarded", false)):
		lines.push_back("[color=%s]+%s team XP[/color]" % [LOG_COLOR_XP, reward.get("total_xp", 0)])
		lines.push_back("Split across %s participant(s): %s XP each." % [
			reward.get("participant_player_indices", []).size(),
			reward.get("xp_per_participant", 0),
		])

	for message in _collect_deferred_progress_lines():
		lines.push_back("[color=%s]%s[/color]" % [LOG_COLOR_XP, message])

	for message in summary.get("level_up_messages", []):
		var text := String(message)
		if text.begins_with("LEVEL UP"):
			lines.push_back("[font_size=34][color=%s]%s[/color][/font_size]" % [LOG_COLOR_XP, text])
		else:
			lines.push_back("[color=%s]%s[/color]" % [LOG_COLOR_XP, text])

	if lines.size() == 1:
		lines.push_back("No progression rewards.")
	return lines


static func format_skill_id(skill_id: String) -> String:
	var parts := skill_id.split("-")
	var formatted: Array[String] = []
	for index in range(parts.size()):
		formatted.push_back(String(parts[index]).capitalize())
	return _join_strings(formatted, " ")


func _collect_deferred_progress_lines() -> Array[String]:
	var lines: Array[String] = []
	for skill_name in deferred_skill_xp.keys():
		lines.push_back("%s gained %s skill XP." % [skill_name, deferred_skill_xp[skill_name]])
	for skill_name in deferred_skill_levels.keys():
		lines.push_back("LEVEL UP: %s reached Lv%s." % [skill_name, deferred_skill_levels[skill_name]])
	deferred_skill_xp.clear()
	deferred_skill_levels.clear()
	return lines


static func _join_strings(values: Array[String], separator: String) -> String:
	var output := ""
	for index in range(values.size()):
		if index > 0:
			output += separator
		output += values[index]
	return output
