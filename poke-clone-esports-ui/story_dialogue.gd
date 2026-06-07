class_name StoryDialogue
extends RefCounted

const NPC_INTROS := {
	"OlderBrother": [
		"Older Brother: You finally made it outside.",
		"Older Brother: Show me you can handle one real match before you queue alone.",
	],
	"Fan": [
		"LAN Regular: I have been watching your games.",
		"LAN Regular: Let us see if the hype is real.",
	],
	"Rival": [
		"Rival: You are still taking basics too seriously.",
		"Rival: I am going to punish every slow decision.",
	],
	"Coach": [
		"Coach: Mechanics are only half the game.",
		"Coach: Prove you can play the map and the fight.",
	],
}

const TOURNAMENT_INTRO := [
	"Announcer: Welcome to Major Hall.",
	"Announcer: One clean set and everyone in town will know your name.",
]


static func get_npc_intro(npc_id: String) -> Array[String]:
	return _to_string_array(NPC_INTROS.get(npc_id, []))


static func get_tournament_intro() -> Array[String]:
	return _to_string_array(TOURNAMENT_INTRO)


static func _to_string_array(values: Array) -> Array[String]:
	var lines: Array[String] = []
	for value in values:
		lines.push_back(String(value))
	return lines
