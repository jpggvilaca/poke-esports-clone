class_name BattleStatusPanel
extends PanelContainer

@onready var name_label: Label = $Content/Name
@onready var meta_label: Label = $Content/Meta
@onready var hp_bar: ProgressBar = $Content/HpRow/Hp
@onready var hp_value: Label = $Content/HpRow/Value
@onready var mana_bar: ProgressBar = $Content/ManaRow/Mana
@onready var mana_value: Label = $Content/ManaRow/Value
@onready var status_label: Label = $Content/Status


func set_status(data: Dictionary, fallback_name: String) -> void:
	var max_hp := int(data.get("max_hp", 1))
	var hp := int(data.get("hp", 0))
	var max_mana := int(data.get("max_mana", 1))
	var mana := int(data.get("mana", 0))
	name_label.text = "%s" % data.get("name", fallback_name)
	var identity_name := String(data.get("trait_name", ""))
	var identity_suffix := ""
	if not identity_name.is_empty():
		identity_suffix = " | %s" % identity_name
	meta_label.text = "%s%s" % [
		data.get("spec", "Spec"),
		identity_suffix,
	]
	hp_bar.max_value = max(1, max_hp)
	hp_bar.value = clamp(hp, 0, max_hp)
	hp_bar.tooltip_text = "HP %s/%s" % [hp, max_hp]
	hp_value.text = "%s/%s" % [hp, max_hp]
	mana_bar.max_value = max(1, max_mana)
	mana_bar.value = clamp(mana, 0, max_mana)
	mana_bar.tooltip_text = "Mana %s/%s" % [mana, max_mana]
	mana_value.text = "%s/%s" % [mana, max_mana]
	status_label.text = format_status_indicator(data.get("status", {}))
	status_label.tooltip_text = format_status_tooltip(data.get("status", {}))


func animate_hp(old_value: int, new_value: int) -> void:
	hp_bar.value = old_value
	hp_value.text = "%s/%s" % [new_value, int(hp_bar.max_value)]
	create_tween().tween_property(hp_bar, "value", new_value, 0.30)


func animate_mana(old_value: int, new_value: int) -> void:
	mana_bar.value = old_value
	mana_value.text = "%s/%s" % [new_value, int(mana_bar.max_value)]
	create_tween().tween_property(mana_bar, "value", new_value, 0.25)


func get_display_name() -> String:
	return name_label.text


static func compact_status_indicator(status: Dictionary) -> String:
	var full_status := format_status_indicator(status)
	if full_status == "Status: none":
		return ""
	return full_status


static func format_status_indicator(status: Dictionary) -> String:
	var entries: Array[String] = []
	var attack_percent := int(status.get("attack_modifier_percent", 0))
	var attack_turns := int(status.get("attack_modifier_turns", 0))
	var defense_percent := int(status.get("defense_modifier_percent", 0))
	var defense_turns := int(status.get("defense_modifier_turns", 0))
	var penetration_percent := int(status.get("attack_penetration_percent", 0))
	var penetration_turns := int(status.get("attack_penetration_turns", 0))
	var cooldown_percent := int(status.get("cooldown_modifier_percent", 0))
	var cooldown_turns := int(status.get("cooldown_modifier_turns", 0))
	var healing_percent := int(status.get("healing_received_modifier_percent", 0))
	var healing_turns := int(status.get("healing_received_modifier_turns", 0))

	if int(status.get("stunned_turns", 0)) > 0:
		entries.push_back("[STUN %s]" % status.get("stunned_turns", 0))
	if int(status.get("silenced_turns", 0)) > 0:
		entries.push_back("[SIL %s]" % status.get("silenced_turns", 0))
	if int(status.get("rooted_turns", 0)) > 0:
		entries.push_back("[ROOT %s]" % status.get("rooted_turns", 0))
	if int(status.get("mark_turns", 0)) > 0:
		entries.push_back("[MARK %s]" % status.get("mark_turns", 0))
	if attack_percent != 0 and attack_turns > 0:
		entries.push_back("[%s ATK %s%% %st]" % [_status_marker(attack_percent), _signed_value(attack_percent), attack_turns])
	if defense_percent != 0 and defense_turns > 0:
		entries.push_back("[%s DEF %s%% %st]" % [_status_marker(defense_percent), _signed_value(defense_percent), defense_turns])
	if penetration_percent != 0 and penetration_turns > 0:
		entries.push_back("[PEN %s%% %st]" % [_signed_value(penetration_percent), penetration_turns])
	if cooldown_percent != 0 and cooldown_turns > 0:
		entries.push_back("[CD %s%% %st]" % [_signed_value(cooldown_percent), cooldown_turns])
	if healing_percent != 0 and healing_turns > 0:
		entries.push_back("[HEAL %s%% %st]" % [_signed_value(healing_percent), healing_turns])
	if entries.is_empty():
		return "Status: none"
	return _join_strings(entries, "  ")


static func format_status_tooltip(status: Dictionary) -> String:
	var lines: Array[String] = []
	if int(status.get("stunned_turns", 0)) > 0:
		lines.push_back("Stunned: loses turns remaining: %s." % status.get("stunned_turns", 0))
	if int(status.get("silenced_turns", 0)) > 0:
		lines.push_back("Silenced: cannot use skills for %s turn(s)." % status.get("silenced_turns", 0))
	if int(status.get("rooted_turns", 0)) > 0:
		lines.push_back("Rooted: restricted by a control effect for %s turn(s)." % status.get("rooted_turns", 0))
	if int(status.get("mark_turns", 0)) > 0:
		lines.push_back("Marked: next mark trigger can deal bonus damage for %s turn(s)." % status.get("mark_turns", 0))
	_append_modifier_tooltip(lines, status, "attack_modifier_percent", "attack_modifier_turns", "Attack pressure")
	_append_modifier_tooltip(lines, status, "defense_modifier_percent", "defense_modifier_turns", "Incoming damage")
	_append_modifier_tooltip(lines, status, "attack_penetration_percent", "attack_penetration_turns", "Attack penetration")
	_append_modifier_tooltip(lines, status, "cooldown_modifier_percent", "cooldown_modifier_turns", "Cooldown speed")
	_append_modifier_tooltip(lines, status, "healing_received_modifier_percent", "healing_received_modifier_turns", "Healing received")
	if lines.is_empty():
		return "No active status effects."
	return _join_strings(lines, "\n")


static func _append_modifier_tooltip(lines: Array[String], status: Dictionary, value_key: String, turns_key: String, label: String) -> void:
	var value := int(status.get(value_key, 0))
	var turns := int(status.get(turns_key, 0))
	if value == 0 or turns <= 0:
		return
	lines.push_back("%s: %s%% for %s turn(s)." % [label, _signed_value(value), turns])


static func _status_marker(value: int) -> String:
	if value > 0:
		return "BUFF"
	return "DEBUFF"


static func _signed_value(value: int) -> String:
	if value > 0:
		return "+%s" % value
	return "%s" % value


static func _join_strings(values: Array[String], separator: String) -> String:
	var output := ""
	for index in range(values.size()):
		if index > 0:
			output += separator
		output += values[index]
	return output
