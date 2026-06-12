class_name StoryDialogue
extends RefCounted

const NPC_INTROS := {
	"OlderBrother": [
		"Older Brother: You finally made it outside.",
		"Older Brother: Show me you can handle one real match before you queue alone.",
	],
	"Rival": [
		"Rival: You are still taking basics too seriously.",
		"Rival: I am going to punish every slow decision.",
	],
}

const TOURNAMENT_INTRO := [
	"Announcer: Welcome to Major Hall.",
	"Announcer: One clean set and everyone in town will know your name.",
]

const OPENING_INTRO := [
	"Older Brother: Hey. You made it.",
	"Older Brother: Before you queue, pick the role you want to build around.",
	"Older Brother: No pressure. Your first spec just decides your starting kit.",
]

const SPEC_FOLLOWUPS := {
	"Top": "Older Brother: Top it is. Durable, stubborn, and hard to push around.",
	"Jungle": "Older Brother: Jungle. Good. You will learn the whole map fast.",
	"Mid": "Older Brother: Mid lane. Flashy, flexible, and always under pressure.",
	"ADC": "Older Brother: ADC. Keep your spacing clean and you can carry fights.",
	"Support": "Older Brother: Support. Vision, tempo, and making everyone else look smart.",
}


static func get_npc_intro(npc_id: String, display_name := "") -> Array[String]:
	if npc_id.begins_with("Casual"):
		var casual_name := display_name if not display_name.is_empty() else "Casual"
		return _to_string_array([
			"%s: Looking for a quick scrim?" % casual_name,
			"%s: Let us see how your lineup handles this rating." % casual_name,
		])
	return _to_string_array(NPC_INTROS.get(npc_id, []))


static func get_tournament_intro() -> Array[String]:
	return _to_string_array(TOURNAMENT_INTRO)


static func get_opening_intro() -> Array[String]:
	return _to_string_array(OPENING_INTRO)


static func get_spec_choice_followup(spec: String) -> Array[String]:
	return _to_string_array([
		String(SPEC_FOLLOWUPS.get(spec, "Older Brother: That works. Let us build around it.")),
		"Older Brother: Walk the paths, take a few casual matches, and come back stronger.",
	])


static func _to_string_array(values: Array) -> Array[String]:
	var lines: Array[String] = []
	for value in values:
		lines.push_back(String(value))
	return lines
