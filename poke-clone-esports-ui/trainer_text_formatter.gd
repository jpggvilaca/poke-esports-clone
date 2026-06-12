class_name TrainerTextFormatter
extends RefCounted


func format_trainer_text(state: Dictionary) -> String:
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
