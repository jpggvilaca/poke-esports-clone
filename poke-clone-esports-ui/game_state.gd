extends Node

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
		"match_context": "tutorial",
		"opponent_level": 1,
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
		"match_context": "normal",
		"opponent_level": 1,
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
		"match_context": "nemesis",
		"opponent_level": 2,
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
		"match_context": "normal",
		"opponent_level": 3,
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
	"match_context": "major",
	"opponent_level": 5,
	"opponent_hp": 160,
	"opponent_focus": 80,
	"opponent_base_power_bonus": 4,
	"required_rating": MAJOR_HALL_REQUIRED_RATING,
}

var trainer_name := "Human Trainer"
var rating := 1000
var money := 0
var active_player_index := 0
var roster: Array = []
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


func complete_battle(result: Dictionary) -> Dictionary:
	_ensure_started()
	var bridge := _ensure_profile_bridge()
	var completion := bridge.complete_trainer_battle(get_trainer_state(), pending_battle, result)
	if not completion.get("accepted", false):
		last_battle_summary = {
			"text": "Battle rewards could not be applied.",
			"winner": result.get("winner", "none"),
			"npc_id": pending_battle.get("id", ""),
			"level_up_messages": [],
		}
		pending_battle.clear()
		return last_battle_summary.duplicate(true)

	var trainer_state: Dictionary = completion.get("trainer_state", {})
	trainer_name = String(trainer_state.get("trainer_name", trainer_name))
	rating = int(trainer_state.get("rating", rating))
	money = int(trainer_state.get("money", money))
	active_player_index = int(trainer_state.get("active_player_index", active_player_index))
	roster = trainer_state.get("roster", []).duplicate(true)

	trophies.clear()
	for trophy_id in trainer_state.get("trophies", []):
		trophies.push_back(String(trophy_id))

	var npc_id := String(completion.get("npc_id", ""))
	if bool(completion.get("mark_npc_defeated", false)) and not npc_id.is_empty():
		defeated_npcs[npc_id] = true

	var level_up_messages: Array[String] = []
	for message in completion.get("level_up_messages", []):
		level_up_messages.push_back(String(message))

	last_battle_summary = {
		"text": String(completion.get("summary_text", "Battle complete.")),
		"winner": String(completion.get("winner", result.get("winner", "none"))),
		"npc_id": npc_id,
		"level_up_messages": level_up_messages,
	}
	_refresh_pending_scout_offer()

	pending_battle.clear()
	return last_battle_summary.duplicate(true)


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

	var result = _ensure_profile_bridge().accept_scout_candidate(roster.duplicate(true), pending_scout_offer, candidate_index)
	last_scout_message = String(result.get("message", result.get("error", "Scout choice failed.")))
	if not result.get("accepted", false):
		return last_scout_message

	roster = result.get("roster", []).duplicate(true)

	var offer_id := String(result.get("offer_id", pending_scout_offer.get("id", "")))
	if not offer_id.is_empty():
		completed_scout_offers[offer_id] = true
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


func get_skill_progress_summary(player: Dictionary, skill_id: String, progress: Dictionary) -> Dictionary:
	return _ensure_profile_bridge().get_player_skill_summary(player, skill_id, progress)


func _refresh_pending_scout_offer() -> void:
	if not pending_scout_offer.is_empty():
		return

	var offer = _ensure_profile_bridge().get_pending_scout_offer(
		rating,
		completed_scout_offers.keys(),
		declined_scout_offers.keys())
	if offer.is_empty():
		return

	pending_scout_offer = offer
	last_scout_message = String(offer.get("message", "Scout found prospects."))


func _ensure_started() -> void:
	if roster.is_empty():
		roster.push_back(_create_player("Starter", "Top"))
	if roster.size() == 1:
		roster.push_back(_create_player("Sub Jungler", "Jungle"))
	for index in range(roster.size()):
		var player: Dictionary = roster[index]
		player = _ensure_player_identity_fields(player)
		if not player.has("passive_bonuses"):
			player["passive_bonuses"] = {}
		if not player.has("max_hp"):
			player["max_hp"] = 100
		if not player.has("current_hp"):
			player["current_hp"] = player["max_hp"]
		if not player.has("max_focus"):
			player["max_focus"] = 50
		if not player.has("current_focus"):
			player["current_focus"] = player["max_focus"]
		roster[index] = player


func _create_player(player_name: String, spec: String) -> Dictionary:
	var bridge := _ensure_profile_bridge()
	var player: Dictionary = bridge.create_player_profile(player_name, spec)
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


func _ensure_player_identity_fields(player: Dictionary) -> Dictionary:
	if player.has("trait_id") and player.has("trait_name") and player.has("trait_description"):
		return player

	var bridge := _ensure_profile_bridge()
	var snapshot: Dictionary = bridge.create_player_profile(
		String(player.get("name", "Player")),
		String(player.get("spec", "Top")))
	for key in ["trait_id", "trait_name", "trait_description"]:
		if snapshot.has(key):
			player[key] = snapshot[key]
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


func _ensure_profile_bridge() -> BattleBridge:
	if not is_instance_valid(profile_bridge):
		profile_bridge = BattleBridge.new()
		profile_bridge.name = "ProfileBridge"
		add_child(profile_bridge)
	return profile_bridge


func _join_strings(values: Array[String], separator: String) -> String:
	var output := ""
	for index in range(values.size()):
		if index > 0:
			output += separator
		output += values[index]
	return output
