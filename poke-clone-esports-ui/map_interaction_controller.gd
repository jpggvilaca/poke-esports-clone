class_name MapInteractionController
extends RefCounted

const INTERACT_DISTANCE := 82.0
const BUILDING_INTERACT_DISTANCE := 190.0

var npc_controller: MapNpcController
var lan_cafe: Node2D
var major_hall: Node2D
var current_interaction := {}


func setup(next_npc_controller: MapNpcController, next_lan_cafe: Node2D, next_major_hall: Node2D) -> void:
	npc_controller = next_npc_controller
	lan_cafe = next_lan_cafe
	major_hall = next_major_hall


func update(player_position: Vector2) -> Dictionary:
	current_interaction.clear()
	var nearest_distance := BUILDING_INTERACT_DISTANCE + 1.0
	var nearest_npc := npc_controller.find_nearest_battle_npc(player_position, INTERACT_DISTANCE)
	if nearest_npc != null:
		nearest_distance = player_position.distance_to(nearest_npc.global_position)
		current_interaction = {
			"type": "battle_npc",
			"node": nearest_npc,
			"label": "E - Battle %s" % npc_controller.get_npc_display_name(nearest_npc),
		}

	var recovery_distance := player_position.distance_to(lan_cafe.global_position)
	if recovery_distance <= BUILDING_INTERACT_DISTANCE and recovery_distance < nearest_distance:
		nearest_distance = recovery_distance
		current_interaction = {"type": "recovery", "label": "E - Recover at LAN CAFE"}

	var tournament_distance := player_position.distance_to(major_hall.global_position)
	if tournament_distance <= BUILDING_INTERACT_DISTANCE and tournament_distance < nearest_distance:
		current_interaction = {"type": "tournament", "label": GameState.get_tournament_prompt()}

	return current_interaction.duplicate()


func get_current_interaction() -> Dictionary:
	return current_interaction.duplicate()
