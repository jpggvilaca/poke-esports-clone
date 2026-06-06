extends Node

const LOSS_RATING := -10
const MAJOR_HALL_REQUIRED_RATING := 1080
const MAX_ROSTER_SIZE := 6

const NPC_BATTLES := {
	"OlderBrother": {
		"id": "OlderBrother",
		"display_name": "Older Brother",
		"opponent_name": "Older Brother",
		"opponent_spec": "Jungle",
		"trophy_id": "defeated_older_brother",
		"single_use": true,
		"money_reward": 40,
		"rating_min": 15,
		"rating_max": 20,
		"opponent_hp": 85,
		"opponent_focus": 45,
		"opponent_base_power_bonus": 0,
	},
	"Fan": {
		"id": "Fan",
		"display_name": "LAN Regular",
		"opponent_name": "LAN Regular",
		"opponent_spec": "Support",
		"trophy_id": "defeated_lan_regular",
		"single_use": true,
		"money_reward": 60,
		"rating_min": 10,
		"rating_max": 25,
		"opponent_hp": 90,
		"opponent_focus": 45,
		"opponent_base_power_bonus": 0,
	},
	"Rival": {
		"id": "Rival",
		"display_name": "Rival",
		"opponent_name": "Rival",
		"opponent_spec": "Support",
		"trophy_id": "defeated_rival",
		"single_use": true,
		"money_reward": 90,
		"rating_min": 25,
		"rating_max": 45,
		"opponent_hp": 105,
		"opponent_focus": 55,
		"opponent_base_power_bonus": 1,
	},
	"Coach": {
		"id": "Coach",
		"display_name": "Coach",
		"opponent_name": "Coach",
		"opponent_spec": "Mid",
		"trophy_id": "defeated_coach",
		"single_use": true,
		"money_reward": 130,
		"rating_min": 35,
		"rating_max": 60,
		"opponent_hp": 125,
		"opponent_focus": 60,
		"opponent_base_power_bonus": 2,
	},
}

const TOURNAMENT_BATTLE := {
	"id": "MajorHallTournament",
	"display_name": "Major Hall Tournament",
	"opponent_name": "Tournament Seed",
	"opponent_spec": "ADC",
	"trophy_id": "major_hall_tournament_win",
	"single_use": false,
	"money_reward": 250,
	"rating_min": 60,
	"rating_max": 90,
	"opponent_hp": 160,
	"opponent_focus": 80,
	"opponent_base_power_bonus": 4,
	"required_rating": MAJOR_HALL_REQUIRED_RATING,
}

const SCOUT_OFFERS := [
	{
		"id": "first-scout",
		"required_rating": 1050,
		"message": "Scout found three prospects after your first local results.",
		"candidates": [
			{"name": "Mira", "spec": "Support"},
			{"name": "Dax", "spec": "ADC"},
			{"name": "Niko", "spec": "Mid"},
		],
	},
	{
		"id": "second-scout",
		"required_rating": 1125,
		"message": "Scout sent a stronger shortlist from nearby LAN queues.",
		"candidates": [
			{"name": "Rin", "spec": "Jungle"},
			{"name": "Sol", "spec": "Top"},
			{"name": "Vexa", "spec": "ADC"},
		],
	},
	{
		"id": "third-scout",
		"required_rating": 1200,
		"message": "Scout has major-ready prospects watching your team.",
		"candidates": [
			{"name": "Kade", "spec": "Mid"},
			{"name": "Luma", "spec": "Support"},
			{"name": "Bram", "spec": "Top"},
		],
	},
]

const SPEC_IDENTITY_PASSIVES := {
	"Top": {
		"id": "clutch-player",
		"name": "Clutch Player",
		"description": "Below 35% HP, paid skills cost 25% less focus.",
	},
	"Jungle": {
		"id": "shotcaller",
		"name": "Shotcaller",
		"description": "Setup and disruption effects are 20% stronger.",
	},
	"Mid": {
		"id": "lane-bully",
		"name": "Lane Bully",
		"description": "Super-effective hits deal 15% more damage.",
	},
	"ADC": {
		"id": "precision-carry",
		"name": "Precision Carry",
		"description": "Damaging skills gain 5% accuracy.",
	},
	"Support": {
		"id": "stabilizer",
		"name": "Stabilizer",
		"description": "Healing and defensive effects are 20% stronger.",
	},
}

const STARTER_SKILL_SUFFIXES := ["basic", "consistent", "disrupt", "setup"]
const SKILL_BASE_STATS := {
	"basic": {"power": 16, "focus_cost": 0},
	"pressure": {"power": 26, "focus_cost": 8},
	"all-in": {"power": 36, "focus_cost": 15},
	"reckless": {"power": 46, "focus_cost": 18},
	"recover": {"power": 0, "focus_cost": 14},
	"guard": {"power": 0, "focus_cost": 7},
	"fortify": {"power": 0, "focus_cost": 12},
	"consistent": {"power": 24, "focus_cost": 7},
	"disrupt": {"power": 18, "focus_cost": 10},
	"setup": {"power": 0, "focus_cost": 8},
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
var completed_scout_offers: Dictionary = {}
var declined_scout_offers: Dictionary = {}
var pending_scout_offer: Dictionary = {}
var last_scout_message := ""
var saved_map_position := Vector2(760, 690)
var pending_battle: Dictionary = {}
var last_battle_summary := {
	"text": "No battles yet.",
	"winner": "none",
	"npc_id": "",
}
var profile_bridge: BattleBridge


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


func prepare_tournament_battle(player_position: Vector2) -> bool:
	_ensure_started()
	if rating < int(TOURNAMENT_BATTLE.get("required_rating", MAJOR_HALL_REQUIRED_RATING)):
		return false

	saved_map_position = player_position
	pending_battle = TOURNAMENT_BATTLE.duplicate(true)
	pending_battle["seed"] = int(Time.get_ticks_msec() % 2147483647)
	return true


func recover_roster() -> String:
	_ensure_started()
	for index in range(roster.size()):
		var player: Dictionary = roster[index]
		player["current_hp"] = int(player.get("max_hp", 100))
		player["current_focus"] = int(player.get("max_focus", 50))
		roster[index] = player
	return "LAN Cafe restored all HP and Focus."


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
			"trait_id": player.get("trait_id", ""),
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
		"opponent_trait_id": battle.get("opponent_trait_id", ""),
		"opponent_hp": int(battle.get("opponent_hp", 100)),
		"opponent_focus": int(battle.get("opponent_focus", 50)),
		"opponent_base_power_bonus": int(battle.get("opponent_base_power_bonus", 0)),
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
		var level_up_messages: Array[String] = []
		if not npc_id.is_empty() and bool(pending_battle.get("single_use", true)):
			defeated_npcs[npc_id] = true

		var reward: Dictionary = result.get("reward", {})
		var battle_xp := int(reward.get("xp_per_participant", 0))
		for profile_index in reward.get("participant_player_indices", []):
			level_up_messages.append_array(_award_player_xp(int(profile_index), battle_xp))

		var money_reward := int(pending_battle.get("money_reward", 100))
		var rating_reward := _roll_rating_reward(pending_battle)
		money += money_reward
		rating += rating_reward
		_refresh_pending_scout_offer()
		var trophy_id := String(pending_battle.get("trophy_id", ""))
		if not trophy_id.is_empty() and not trophies.has(trophy_id):
			trophies.push_back(trophy_id)

		last_battle_summary = {
			"text": "Won against %s (+%s money, +%s rating)." % [display_name, money_reward, rating_reward],
			"winner": winner,
			"npc_id": npc_id,
			"level_up_messages": level_up_messages,
		}
	else:
		rating = max(0, rating + LOSS_RATING)
		last_battle_summary = {
			"text": "Lost against %s (%s rating)." % [display_name, LOSS_RATING],
			"winner": winner,
			"npc_id": npc_id,
			"level_up_messages": [],
		}

	pending_battle.clear()


func get_trainer_state() -> Dictionary:
	_ensure_started()
	_refresh_pending_scout_offer()
	return {
		"trainer_name": trainer_name,
		"rating": rating,
		"money": money,
		"max_roster_size": MAX_ROSTER_SIZE,
		"active_player_index": active_player_index,
		"roster": roster.duplicate(true),
		"items": items.duplicate(true),
		"trophies": trophies.duplicate(),
		"defeated_npcs": defeated_npcs.duplicate(),
		"pending_scout_offer": pending_scout_offer.duplicate(true),
		"last_scout_message": last_scout_message,
		"last_battle_summary": last_battle_summary.duplicate(),
		"major_hall_required_rating": int(TOURNAMENT_BATTLE.get("required_rating", MAJOR_HALL_REQUIRED_RATING)),
	}


func is_npc_defeated(npc_id: String) -> bool:
	return defeated_npcs.get(npc_id, false)


func get_npc_display_name(npc_id: String) -> String:
	var config: Dictionary = NPC_BATTLES.get(npc_id, {})
	return String(config.get("display_name", npc_id))


func get_tournament_prompt() -> String:
	var required_rating := int(TOURNAMENT_BATTLE.get("required_rating", MAJOR_HALL_REQUIRED_RATING))
	if rating < required_rating:
		return "Major Hall requires %s rating (you: %s)" % [required_rating, rating]
	return "Press Enter to enter MAJOR HALL tournament"


func get_tournament_locked_message() -> String:
	var required_rating := int(TOURNAMENT_BATTLE.get("required_rating", MAJOR_HALL_REQUIRED_RATING))
	return "Major Hall requires %s rating. Current rating: %s." % [required_rating, rating]


func has_pending_scout_offer() -> bool:
	_ensure_started()
	_refresh_pending_scout_offer()
	return not pending_scout_offer.is_empty()


func accept_scout_candidate(candidate_index: int) -> String:
	_ensure_started()
	_refresh_pending_scout_offer()
	if pending_scout_offer.is_empty():
		return "No scout offer is pending."
	if roster.size() >= MAX_ROSTER_SIZE:
		last_scout_message = "Roster full. Scout offer remains pending."
		return last_scout_message

	var candidates: Array = pending_scout_offer.get("candidates", [])
	if candidate_index < 0 or candidate_index >= candidates.size():
		return "Unknown scout candidate."

	var candidate: Dictionary = candidates[candidate_index]
	var profile := candidate.duplicate(true)
	profile["current_hp"] = int(profile.get("max_hp", 100))
	profile["current_focus"] = int(profile.get("max_focus", 50))
	roster.push_back(profile)

	var offer_id := String(pending_scout_offer.get("id", ""))
	if not offer_id.is_empty():
		completed_scout_offers[offer_id] = true
	last_scout_message = "%s joined your roster." % profile.get("name", "Prospect")
	pending_scout_offer.clear()
	return last_scout_message


func decline_scout_offer() -> String:
	_ensure_started()
	_refresh_pending_scout_offer()
	if pending_scout_offer.is_empty():
		return "No scout offer is pending."

	var offer_id := String(pending_scout_offer.get("id", ""))
	if not offer_id.is_empty():
		declined_scout_offers[offer_id] = true
	last_scout_message = "Scout shortlist dismissed."
	pending_scout_offer.clear()
	return last_scout_message


func format_skill_name(skill_id: String) -> String:
	var parts := skill_id.split("-")
	var formatted: Array[String] = []
	for index in range(parts.size()):
		formatted.push_back(String(parts[index]).capitalize())
	return _join_strings(formatted, " ")


func get_skill_progress_summary(skill_id: String, progress: Dictionary) -> Dictionary:
	var level = max(1, int(progress.get("level", 1)))
	var suffix := _skill_suffix(skill_id)
	var base_stats: Dictionary = SKILL_BASE_STATS.get(suffix, {"power": 0, "focus_cost": 0})
	var base_power := int(base_stats.get("power", 0))
	var base_focus := int(base_stats.get("focus_cost", 0))
	var focus_cost = 0 if base_focus == 0 else base_focus + (level - 1) * 2
	return {
		"name": format_skill_name(skill_id),
		"level": level,
		"xp": int(progress.get("xp", 0)),
		"power": base_power + (level - 1) * 4,
		"focus_cost": focus_cost,
	}


func _refresh_pending_scout_offer() -> void:
	if not pending_scout_offer.is_empty():
		return

	for offer in SCOUT_OFFERS:
		var offer_id := String(offer.get("id", ""))
		if rating < int(offer.get("required_rating", 0)):
			continue
		if bool(completed_scout_offers.get(offer_id, false)) or bool(declined_scout_offers.get(offer_id, false)):
			continue

		var hydrated_offer: Dictionary = offer.duplicate(true)
		var candidates: Array = []
		for candidate_config in offer.get("candidates", []):
			if not (candidate_config is Dictionary):
				continue
			candidates.push_back(_create_scout_candidate(candidate_config))
		hydrated_offer["candidates"] = candidates
		pending_scout_offer = hydrated_offer
		last_scout_message = String(hydrated_offer.get("message", "Scout found prospects."))
		return


func _create_scout_candidate(candidate_config: Dictionary) -> Dictionary:
	return _create_player(
		String(candidate_config.get("name", "Prospect")),
		String(candidate_config.get("spec", "Top")))


func _ensure_started() -> void:
	if roster.is_empty():
		roster.push_back(_create_player("Starter", "Top"))
	if roster.size() == 1:
		roster.push_back(_create_player("Sub Jungler", "Jungle"))
	for index in range(roster.size()):
		var player: Dictionary = roster[index]
		player = _ensure_player_identity_fields(player)
		player = _refresh_player_level_passives(player)
		if not player.has("max_hp"):
			player["max_hp"] = _max_hp_from_passives(player.get("passive_bonuses", {}))
		if not player.has("current_hp"):
			player["current_hp"] = player["max_hp"]
		if not player.has("max_focus"):
			player["max_focus"] = 50
		if not player.has("current_focus"):
			player["current_focus"] = player["max_focus"]
		roster[index] = player


func _create_player(player_name: String, spec: String) -> Dictionary:
	var bridge := _ensure_profile_bridge()
	var player: Dictionary = {}
	if bridge.has_method("create_player_profile"):
		player = bridge.create_player_profile(player_name, spec)
	else:
		player = _create_local_player_profile(player_name, spec)
	player["current_hp"] = int(player.get("max_hp", 100))
	player["current_focus"] = int(player.get("max_focus", 50))

	var progress := {}
	for skill_id in player.get("active_skill_ids", []):
		progress[skill_id] = {
			"skill_id": skill_id,
			"level": 1,
			"xp": 0,
		}
	player["skill_progress"] = progress

	return player


func _create_local_player_profile(player_name: String, spec: String) -> Dictionary:
	var normalized_spec := _normalize_spec(spec)
	var identity_data: Dictionary = SPEC_IDENTITY_PASSIVES.get(normalized_spec, SPEC_IDENTITY_PASSIVES["Top"])
	var active_skill_ids := _starter_skill_ids_for_spec(normalized_spec)
	var passive_bonuses := _passive_bonuses_for_level(1)
	return {
		"name": player_name,
		"spec": normalized_spec,
		"rank": "Rookie",
		"trait_id": identity_data.get("id", ""),
		"trait_name": identity_data.get("name", ""),
		"trait_description": identity_data.get("description", ""),
		"level": 1,
		"xp": 0,
		"xp_required_for_next_level": _xp_required_for_level(1),
		"passive_bonuses": passive_bonuses,
		"max_hp": _max_hp_from_passives(passive_bonuses),
		"max_focus": 50,
		"learned_skill_ids": active_skill_ids.duplicate(),
		"active_skill_ids": active_skill_ids.duplicate(),
	}


func _award_player_profile_xp_local(player: Dictionary, amount: int) -> Dictionary:
	if amount < 0:
		return {
			"accepted": false,
			"error_code": "negative_xp_award",
			"error": "XP award cannot be negative.",
		}

	var profile := player.duplicate(true)
	var old_xp := int(profile.get("xp", 0))
	var old_level := int(profile.get("level", 1))
	var xp := old_xp + amount
	var level := old_level
	var leveled_up := false
	while xp >= _xp_required_for_level(level):
		xp -= _xp_required_for_level(level)
		level += 1
		leveled_up = true

	var rank := _rank_for_level(level)
	var passive_bonuses := _passive_bonuses_for_level(level)
	profile["level"] = level
	profile["xp"] = xp
	profile["rank"] = rank
	profile["xp_required_for_next_level"] = _xp_required_for_level(level)
	profile["passive_bonuses"] = passive_bonuses
	profile["max_hp"] = _max_hp_from_passives(passive_bonuses)
	profile["max_focus"] = 50

	return {
		"accepted": true,
		"leveled_up": leveled_up,
		"old_value": old_xp,
		"new_value": xp,
		"old_level": old_level,
		"new_level": level,
		"player_profile": profile,
	}


func _normalize_spec(spec: String) -> String:
	var cleaned := spec.strip_edges().to_lower()
	match cleaned:
		"jungle":
			return "Jungle"
		"mid":
			return "Mid"
		"adc":
			return "ADC"
		"support":
			return "Support"
		_:
			return "Top"


func _starter_skill_ids_for_spec(spec: String) -> Array[String]:
	var prefix := spec.to_lower()
	var skill_ids: Array[String] = []
	for suffix in STARTER_SKILL_SUFFIXES:
		skill_ids.push_back("%s-%s" % [prefix, suffix])
	return skill_ids


func _skill_suffix(skill_id: String) -> String:
	var parts := skill_id.split("-", false)
	if parts.size() <= 1:
		return skill_id
	var suffix_parts: Array[String] = []
	for index in range(1, parts.size()):
		suffix_parts.push_back(String(parts[index]))
	return _join_strings(suffix_parts, "-")


func _xp_required_for_level(level: int) -> int:
	return 100 + (max(1, level) - 1) * 50


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
			return {"max_hp_bonus": 10}
		"Pro":
			return {"max_hp_bonus": 10, "counter_damage_bonus_percent": 5}
		"Elite":
			return {"max_hp_bonus": 20, "counter_damage_bonus_percent": 10}
		"World-Class":
			return {"max_hp_bonus": 30, "counter_damage_bonus_percent": 15}
		_:
			return {}


func _passive_bonuses_for_level(level: int) -> Dictionary:
	var safe_level = max(1, level)
	var passive_bonuses := _passive_bonuses_for_rank(_rank_for_level(safe_level))
	passive_bonuses["max_hp_bonus"] = int(passive_bonuses.get("max_hp_bonus", 0)) + (safe_level - 1) * 2
	passive_bonuses["base_power_bonus"] = int(passive_bonuses.get("base_power_bonus", 0)) + int((safe_level - 1) / 3)
	return passive_bonuses


func _max_hp_from_passives(passive_bonuses: Dictionary) -> int:
	return 100 + int(passive_bonuses.get("max_hp_bonus", 0))


func _ensure_player_identity_fields(player: Dictionary) -> Dictionary:
	if player.has("trait_id") and player.has("trait_name") and player.has("trait_description"):
		return player

	var bridge := _ensure_profile_bridge()
	var snapshot: Dictionary = {}
	if bridge.has_method("create_player_profile"):
		snapshot = bridge.create_player_profile(
			String(player.get("name", "Player")),
			String(player.get("spec", "Top")))
	else:
		snapshot = _create_local_player_profile(
			String(player.get("name", "Player")),
			String(player.get("spec", "Top")))
	for key in ["trait_id", "trait_name", "trait_description"]:
		if snapshot.has(key):
			player[key] = snapshot[key]
	return player


func _refresh_player_level_passives(player: Dictionary) -> Dictionary:
	var level = max(1, int(player.get("level", 1)))
	player["rank"] = _rank_for_level(level)
	player["passive_bonuses"] = _passive_bonuses_for_level(level)
	player["max_hp"] = _max_hp_from_passives(player.get("passive_bonuses", {}))
	player["max_focus"] = int(player.get("max_focus", 50))
	return player


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
	var current_profile_index := active_player_index
	for event in result.get("events", []):
		if not (event is Dictionary):
			continue

		match String(event.get("type", "none")):
			"battle_started", "player_switched":
				current_profile_index = int(event.get("new_player_index", current_profile_index))

		if String(event.get("actor", "none")) != "player":
			continue

		var skill_id := String(event.get("skill_id", ""))
		if skill_id.is_empty():
			continue

		var profile_index := int(event.get("profile_index", current_profile_index))
		if profile_index < 0 or profile_index >= roster.size():
			continue

		var player: Dictionary = roster[profile_index]
		var progress_by_skill: Dictionary = player.get("skill_progress", {})
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
		roster[profile_index] = player


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


func _award_player_xp(profile_index: int, amount: int) -> Array[String]:
	if amount <= 0 or profile_index < 0 or profile_index >= roster.size():
		return []

	var player: Dictionary = roster[profile_index]
	var player_name := String(player.get("name", "Player"))
	var bridge := _ensure_profile_bridge()
	var result: Dictionary = {}
	if bridge.has_method("award_player_profile_xp"):
		result = bridge.award_player_profile_xp(player, amount)
	else:
		result = _award_player_profile_xp_local(player, amount)
	if not result.get("accepted", false):
		return []

	player = _merge_profile_snapshot(player, result.get("player_profile", {}))
	roster[profile_index] = player
	var messages: Array[String] = ["XP: %s gained %s player XP." % [player_name, amount]]
	if bool(result.get("leveled_up", false)):
		messages.push_back("LEVEL UP: %s reached Lv%s." % [player.get("name", "Player"), player.get("level", 1)])
	return messages


func _ensure_profile_bridge() -> BattleBridge:
	if not is_instance_valid(profile_bridge):
		profile_bridge = BattleBridge.new()
		profile_bridge.name = "ProfileBridge"
		add_child(profile_bridge)
	return profile_bridge


func _merge_profile_snapshot(player: Dictionary, snapshot: Dictionary) -> Dictionary:
	for key in [
		"name",
		"spec",
		"rank",
		"trait_id",
		"trait_name",
		"trait_description",
		"level",
		"xp",
		"xp_required_for_next_level",
		"passive_bonuses",
		"max_hp",
		"max_focus",
		"learned_skill_ids",
		"active_skill_ids",
	]:
		if snapshot.has(key):
			player[key] = snapshot[key]

	player = _refresh_player_level_passives(player)
	player["current_hp"] = min(int(player.get("current_hp", player.get("max_hp", 100))), int(player.get("max_hp", 100)))
	player["current_focus"] = min(int(player.get("current_focus", player.get("max_focus", 50))), int(player.get("max_focus", 50)))
	return player


func _roll_rating_reward(config: Dictionary) -> int:
	var min_value := int(config.get("rating_min", 20))
	var max_value := int(config.get("rating_max", min_value))
	if max_value < min_value:
		max_value = min_value
	return randi_range(min_value, max_value)


func _join_strings(values: Array[String], separator: String) -> String:
	var output := ""
	for index in range(values.size()):
		if index > 0:
			output += separator
		output += values[index]
	return output
