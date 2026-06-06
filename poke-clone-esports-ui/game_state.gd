extends Node

const WIN_MONEY := 100
const WIN_RATING := 20
const LOSS_RATING := -10
const BASE_XP_FOR_NEXT_LEVEL := 100
const XP_GROWTH_PER_LEVEL := 50

const NPC_BATTLES := {
	"Rival": {
		"id": "Rival",
		"display_name": "Rival",
		"opponent_name": "Rival",
		"opponent_spec": "Jungle",
		"opponent_style": "Balanced",
		"trophy_id": "defeated_rival",
	},
	"Coach": {
		"id": "Coach",
		"display_name": "Coach",
		"opponent_name": "Coach",
		"opponent_spec": "Mid",
		"opponent_style": "Defensive",
		"trophy_id": "defeated_coach",
	},
	"Fan": {
		"id": "Fan",
		"display_name": "Fan",
		"opponent_name": "Fan",
		"opponent_spec": "Support",
		"opponent_style": "Aggressive",
		"trophy_id": "defeated_fan",
	},
}

var trainer_name := "Human Trainer"
var rating := 1000
var money := 0
var active_player_index := 0
var roster: Array = []
var items: Array[Dictionary] = [
	{"id": "energy_drink", "name": "Energy Drink", "quantity": 2, "description": "Restores focus between long sets."},
	{"id": "first_aid", "name": "First Aid Kit", "quantity": 1, "description": "A basic recovery item for the road."},
]
var trophies: Array[String] = []
var defeated_npcs: Dictionary = {}
var saved_map_position := Vector2(760, 690)
var pending_battle: Dictionary = {}
var last_battle_summary := {
	"text": "No battles yet.",
	"winner": "none",
	"npc_id": "",
}


func _ready() -> void:
	_ensure_started()


func prepare_npc_battle(npc_id: String, player_position: Vector2) -> bool:
	_ensure_started()
	if is_npc_defeated(npc_id):
		return false

	var config: Dictionary = NPC_BATTLES.get(npc_id, {})
	if config.is_empty():
		return false

	saved_map_position = player_position
	pending_battle = config.duplicate(true)
	pending_battle["seed"] = int(Time.get_ticks_msec() % 2147483647)
	return true


func has_pending_battle() -> bool:
	return not pending_battle.is_empty()


func build_battle_setup() -> Dictionary:
	_ensure_started()
	var battle: Dictionary = {}
	if has_pending_battle():
		battle = pending_battle
	else:
		battle = {
			"opponent_name": "Opponent",
			"opponent_spec": "Jungle",
			"opponent_style": "Balanced",
			"seed": 20260606,
		}

	var player_team: Array = []
	var setup_active_index := _first_playable_player_index()
	for index in range(roster.size()):
		var player: Dictionary = roster[index]
		var skills: Array = []
		for skill_id in player.get("active_skill_ids", []):
			var progress: Dictionary = player.get("skill_progress", {}).get(skill_id, {})
			skills.push_back({
				"id": skill_id,
				"level": int(progress.get("level", 1)),
				"xp": int(progress.get("xp", 0)),
			})

		player_team.push_back({
			"profile_index": index,
			"name": player.get("name", "Player"),
			"spec": player.get("spec", "Top"),
			"style": player.get("style", "Balanced"),
			"current_hp": int(player.get("current_hp", player.get("max_hp", 100))),
			"current_focus": int(player.get("current_focus", player.get("max_focus", 50))),
			"passive_bonuses": player.get("passive_bonuses", {}),
			"skills": skills,
		})

	return {
		"game_type": "League of Legends",
		"active_player_index": setup_active_index,
		"player_team": player_team,
		"opponent_name": battle.get("opponent_name", "Opponent"),
		"opponent_spec": battle.get("opponent_spec", "Jungle"),
		"opponent_style": battle.get("opponent_style", "Balanced"),
		"seed": int(battle.get("seed", 20260606)),
	}


func complete_battle(result: Dictionary) -> void:
	_ensure_started()
	var npc_id := String(pending_battle.get("id", ""))
	var display_name := String(pending_battle.get("display_name", "Opponent"))
	var winner := String(result.get("winner", "none"))
	var won := winner == "player"

	_apply_battle_vitals(result)
	_apply_skill_progress(result)

	if won:
		if not npc_id.is_empty():
			defeated_npcs[npc_id] = true

		var reward: Dictionary = result.get("reward", {})
		var battle_xp := int(reward.get("xp_per_participant", 0))
		for profile_index in reward.get("participant_player_indices", []):
			_award_player_xp(int(profile_index), battle_xp)

		money += WIN_MONEY
		rating += WIN_RATING
		var trophy_id := String(pending_battle.get("trophy_id", ""))
		if not trophy_id.is_empty() and not trophies.has(trophy_id):
			trophies.push_back(trophy_id)

		last_battle_summary = {
			"text": "Won against %s (+%s money, +%s rating)." % [display_name, WIN_MONEY, WIN_RATING],
			"winner": winner,
			"npc_id": npc_id,
		}
	else:
		rating = max(0, rating + LOSS_RATING)
		last_battle_summary = {
			"text": "Lost against %s (%s rating)." % [display_name, LOSS_RATING],
			"winner": winner,
			"npc_id": npc_id,
		}

	pending_battle.clear()


func get_trainer_state() -> Dictionary:
	_ensure_started()
	return {
		"trainer_name": trainer_name,
		"rating": rating,
		"money": money,
		"active_player_index": active_player_index,
		"roster": roster.duplicate(true),
		"items": items.duplicate(true),
		"trophies": trophies.duplicate(),
		"defeated_npcs": defeated_npcs.duplicate(),
		"last_battle_summary": last_battle_summary.duplicate(),
	}


func is_npc_defeated(npc_id: String) -> bool:
	return defeated_npcs.get(npc_id, false)


func get_npc_display_name(npc_id: String) -> String:
	var config: Dictionary = NPC_BATTLES.get(npc_id, {})
	return String(config.get("display_name", npc_id))


func format_skill_name(skill_id: String) -> String:
	var parts := skill_id.split("-")
	var formatted: Array[String] = []
	for index in range(parts.size()):
		formatted.push_back(String(parts[index]).capitalize())
	return _join_strings(formatted, " ")


func _ensure_started() -> void:
	if roster.is_empty():
		roster.push_back(_create_player("Starter", "Top"))
	if roster.size() == 1:
		roster.push_back(_create_player("Sub Jungler", "Jungle"))
	for index in range(roster.size()):
		var player: Dictionary = roster[index]
		if not player.has("max_hp"):
			player["max_hp"] = 100 + int(player.get("passive_bonuses", {}).get("max_hp_bonus", 0))
		if not player.has("current_hp"):
			player["current_hp"] = player["max_hp"]
		if not player.has("max_focus"):
			player["max_focus"] = 50
		if not player.has("current_focus"):
			player["current_focus"] = player["max_focus"]
		roster[index] = player


func _create_player(player_name: String, spec: String) -> Dictionary:
	var active_skills := _starter_active_skill_ids(spec)
	var progress := {}
	for skill_id in active_skills:
		progress[skill_id] = {
			"skill_id": skill_id,
			"level": 1,
			"xp": 0,
		}

	return {
		"name": player_name,
		"spec": spec,
		"style": "Balanced",
		"rank": "Rookie",
		"level": 1,
		"xp": 0,
		"xp_required_for_next_level": BASE_XP_FOR_NEXT_LEVEL,
		"passive_bonuses": _passive_bonuses_for_rank("Rookie"),
		"max_hp": 100,
		"current_hp": 100,
		"max_focus": 50,
		"current_focus": 50,
		"learned_skill_ids": active_skills.duplicate(),
		"active_skill_ids": active_skills,
		"skill_progress": progress,
	}


func _starter_active_skill_ids(spec: String) -> Array[String]:
	var prefix := spec.to_lower()
	return [
		"%s-basic" % prefix,
		"%s-consistent" % prefix,
		"%s-disrupt" % prefix,
		"%s-setup" % prefix,
	]


func _first_playable_player_index() -> int:
	if active_player_index >= 0 and active_player_index < roster.size():
		var active_player: Dictionary = roster[active_player_index]
		if int(active_player.get("current_hp", active_player.get("max_hp", 100))) > 0:
			return active_player_index

	for index in range(roster.size()):
		var player: Dictionary = roster[index]
		if int(player.get("current_hp", player.get("max_hp", 100))) > 0:
			active_player_index = index
			return index

	return clamp(active_player_index, 0, max(0, roster.size() - 1))


func _apply_skill_progress(result: Dictionary) -> void:
	if active_player_index < 0 or active_player_index >= roster.size():
		return

	var player: Dictionary = roster[active_player_index]
	var progress_by_skill: Dictionary = player.get("skill_progress", {})
	for event in result.get("events", []):
		if not (event is Dictionary):
			continue
		if String(event.get("actor", "none")) != "player":
			continue

		var skill_id := String(event.get("skill_id", ""))
		if skill_id.is_empty():
			continue

		var progress: Dictionary = progress_by_skill.get(skill_id, {
			"skill_id": skill_id,
			"level": 1,
			"xp": 0,
		})

		match String(event.get("type", "none")):
			"skill_xp_gained":
				progress["xp"] = int(event.get("new_value", progress.get("xp", 0)))
			"skill_leveled_up":
				progress["level"] = int(event.get("new_level", progress.get("level", 1)))

		progress_by_skill[skill_id] = progress

	player["skill_progress"] = progress_by_skill
	roster[active_player_index] = player


func _apply_battle_vitals(result: Dictionary) -> void:
	var state: Dictionary = result.get("state", {})
	active_player_index = int(state.get("active_player_index", active_player_index))
	for player_state in state.get("player_team", []):
		if not (player_state is Dictionary):
			continue

		var profile_index := int(player_state.get("profile_index", -1))
		if profile_index < 0 or profile_index >= roster.size():
			continue

		var player: Dictionary = roster[profile_index]
		player["current_hp"] = int(player_state.get("hp", player.get("current_hp", 100)))
		player["max_hp"] = int(player_state.get("max_hp", player.get("max_hp", 100)))
		player["current_focus"] = int(player_state.get("focus", player.get("current_focus", 50)))
		player["max_focus"] = int(player_state.get("max_focus", player.get("max_focus", 50)))
		roster[profile_index] = player


func _award_player_xp(profile_index: int, amount: int) -> void:
	if amount <= 0 or profile_index < 0 or profile_index >= roster.size():
		return

	var player: Dictionary = roster[profile_index]
	player["xp"] = int(player.get("xp", 0)) + amount
	while int(player["xp"]) >= _xp_required_for_level(int(player.get("level", 1))):
		player["xp"] = int(player["xp"]) - _xp_required_for_level(int(player.get("level", 1)))
		player["level"] = int(player.get("level", 1)) + 1

	player["xp_required_for_next_level"] = _xp_required_for_level(int(player.get("level", 1)))
	player["rank"] = _rank_for_level(int(player.get("level", 1)))
	player["passive_bonuses"] = _passive_bonuses_for_rank(String(player["rank"]))
	var bonuses: Dictionary = player["passive_bonuses"]
	player["max_hp"] = 100 + int(bonuses.get("max_hp_bonus", 0))
	player["current_hp"] = min(int(player.get("current_hp", player["max_hp"])), int(player["max_hp"]))
	player["max_focus"] = int(player.get("max_focus", 50))
	player["current_focus"] = min(int(player.get("current_focus", player["max_focus"])), int(player["max_focus"]))
	roster[profile_index] = player


func _xp_required_for_level(level: int) -> int:
	return BASE_XP_FOR_NEXT_LEVEL + (max(1, level) - 1) * XP_GROWTH_PER_LEVEL


func _rank_for_level(level: int) -> String:
	if level >= 20:
		return "World-Class"
	if level >= 15:
		return "Elite"
	if level >= 10:
		return "Pro"
	if level >= 5:
		return "Ladder"
	return "Rookie"


func _passive_bonuses_for_rank(rank: String) -> Dictionary:
	match rank:
		"Ladder":
			return {"max_hp_bonus": 10, "base_power_bonus": 0, "counter_damage_bonus_percent": 0}
		"Pro":
			return {"max_hp_bonus": 10, "base_power_bonus": 0, "counter_damage_bonus_percent": 5}
		"Elite":
			return {"max_hp_bonus": 20, "base_power_bonus": 0, "counter_damage_bonus_percent": 10}
		"World-Class":
			return {"max_hp_bonus": 30, "base_power_bonus": 0, "counter_damage_bonus_percent": 15}
		_:
			return {"max_hp_bonus": 0, "base_power_bonus": 0, "counter_damage_bonus_percent": 0}


func _join_strings(values: Array[String], separator: String) -> String:
	var output := ""
	for index in range(values.size()):
		if index > 0:
			output += separator
		output += values[index]
	return output
