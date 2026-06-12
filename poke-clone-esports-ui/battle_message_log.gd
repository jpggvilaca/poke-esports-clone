class_name BattleMessageLog
extends VBoxContainer

const LOG_LINES := 5
const LOG_COLOR_ATTACK := "#ff6b4a"
const LOG_COLOR_DEFENSE := "#5aa7ff"
const LOG_COLOR_DAMAGE := "#ff934f"
const LOG_COLOR_HP := "#5bd487"
const LOG_COLOR_MANA := "#57d7ff"
const LOG_COLOR_XP := "#f3d35b"

@onready var message_label: Label = $Message
@onready var log_label: RichTextLabel = $Log

var log_entries: Array[String] = []


func set_message(text: String) -> void:
	message_label.text = text


func get_message() -> String:
	return message_label.text


func set_turn_prompt(player_name: String) -> void:
	set_message("What will %s do?" % player_name)


func push_log(text: String) -> void:
	if text.is_empty():
		return

	log_entries.push_back(_colorize_log(text))
	if log_entries.size() > LOG_LINES:
		log_entries.remove_at(0)
	log_label.text = _join_strings(log_entries, "\n")


func _colorize_log(text: String) -> String:
	var output := text
	output = output.replace("attack", "[color=%s]attack[/color]" % LOG_COLOR_ATTACK)
	output = output.replace("Attack", "[color=%s]Attack[/color]" % LOG_COLOR_ATTACK)
	output = output.replace("defense", "[color=%s]defense[/color]" % LOG_COLOR_DEFENSE)
	output = output.replace("Defense", "[color=%s]Defense[/color]" % LOG_COLOR_DEFENSE)
	output = output.replace("damage", "[color=%s]damage[/color]" % LOG_COLOR_DAMAGE)
	output = output.replace("Damage", "[color=%s]Damage[/color]" % LOG_COLOR_DAMAGE)
	output = output.replace("HP", "[color=%s]HP[/color]" % LOG_COLOR_HP)
	output = output.replace("Mana", "[color=%s]Mana[/color]" % LOG_COLOR_MANA)
	output = output.replace("mana", "[color=%s]mana[/color]" % LOG_COLOR_MANA)
	output = output.replace("XP", "[color=%s]XP[/color]" % LOG_COLOR_XP)
	output = output.replace("LEVEL UP", "[color=%s]LEVEL UP[/color]" % LOG_COLOR_XP)
	return output


func _join_strings(values: Array[String], separator: String) -> String:
	var output := ""
	for index in range(values.size()):
		if index > 0:
			output += separator
		output += values[index]
	return output
