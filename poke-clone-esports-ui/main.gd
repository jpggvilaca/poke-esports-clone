extends Control

# This scene is a temporary combat lab. It deliberately contains presentation
# logic only: BattleBridge owns the real C++ simulation and turn order.

@onready var bridge: BattleBridge = $BattleBridge
@onready var player_spec: OptionButton = $Margin/Root/Setup/PlayerSpec
@onready var player_style: OptionButton = $Margin/Root/Setup/PlayerStyle
@onready var opponent_spec: OptionButton = $Margin/Root/Setup/OpponentSpec
@onready var opponent_style: OptionButton = $Margin/Root/Setup/OpponentStyle
@onready var restart_button: Button = $Margin/Root/StartButtons/Restart
@onready var style_buttons: HBoxContainer = $Margin/Root/StyleButtons
@onready var skill_buttons: VBoxContainer = $Margin/Root/SkillButtons
@onready var battle_log: RichTextLabel = $Margin/Root/BattleLog

var styles: Array[Dictionary] = []


func _ready() -> void:
	$Margin/Root/StartButtons/Start.pressed.connect(_start_battle)
	restart_button.pressed.connect(_start_battle)

	_populate_option(player_spec, bridge.get_specs())
	_populate_option(opponent_spec, bridge.get_specs())
	styles.assign(bridge.get_styles())
	_populate_option(player_style, styles)
	_populate_option(opponent_style, styles)

	player_style.select(2)
	opponent_spec.select(1)
	opponent_style.select(2)
	_render()


func _populate_option(button: OptionButton, rows: Array) -> void:
	for row: Dictionary in rows:
		button.add_item(row.name, row.id)


func _start_battle() -> void:
	battle_log.clear()
	_append_events(bridge.start_battle(
		player_spec.get_selected_id(),
		player_style.get_selected_id(),
		opponent_spec.get_selected_id(),
		opponent_style.get_selected_id()))
	_render()


func _use_skill(skill_id: String) -> void:
	_append_events(bridge.use_skill(skill_id))
	_render()


func _change_style(style_id: int) -> void:
	_append_events(bridge.change_style(style_id))
	_render()


func _render() -> void:
	var state: Dictionary = bridge.get_battle_state()
	_render_competitor($Margin/Root/Status/Player, state.player)
	_render_competitor($Margin/Root/Status/Opponent, state.opponent)
	restart_button.disabled = not state.started
	_rebuild_style_buttons()
	_rebuild_skill_buttons()


func _render_competitor(panel: VBoxContainer, competitor: Dictionary) -> void:
	panel.get_node("SpecStyle").text = "Spec: %s | Style: %s" % [
		competitor.spec_name,
		competitor.style_name,
	]
	panel.get_node("Hp").text = "HP: %d / %d" % [competitor.hp, competitor.max_hp]
	panel.get_node("Focus").text = "Focus: %d / %d" % [competitor.focus, competitor.max_focus]
	panel.get_node("Effects").text = "Effects: " + _format_status(competitor.status)


func _format_status(status: Dictionary) -> String:
	var parts: Array[String] = []
	if status.attack_modifier_hits > 0:
		parts.append("attack %+d%% (%d hit)" % [
			status.attack_modifier_percent,
			status.attack_modifier_hits,
		])
	if status.defense_modifier_hits > 0:
		parts.append("defense %+d%% (%d hit)" % [
			status.defense_modifier_percent,
			status.defense_modifier_hits,
		])
	return "none" if parts.is_empty() else ", ".join(parts)


func _rebuild_style_buttons() -> void:
	_free_children(style_buttons)
	var state: Dictionary = bridge.get_battle_state()
	for style: Dictionary in styles:
		var button := Button.new()
		button.text = style.name
		button.disabled = not state.started or state.finished or state.player.style == style.id
		button.pressed.connect(_change_style.bind(style.id))
		style_buttons.add_child(button)


func _rebuild_skill_buttons() -> void:
	_free_children(skill_buttons)
	var state: Dictionary = bridge.get_battle_state()
	if not state.started:
		var hint := Label.new()
		hint.text = "Start a battle to choose skills."
		skill_buttons.add_child(hint)
		return

	for skill: Dictionary in bridge.get_available_skills():
		var button := Button.new()
		button.text = _format_skill(skill)
		button.disabled = state.finished or not skill.affordable
		button.pressed.connect(_use_skill.bind(skill.id))
		skill_buttons.add_child(button)


func _format_skill(skill: Dictionary) -> String:
	var text := "%s | Power %d | Focus %d | Accuracy %d%% | Lv %d (%d XP)" % [
		skill.name,
		skill.power,
		skill.focus_cost,
		roundi(skill.accuracy * 100.0),
		skill.level,
		skill.xp,
	]
	if skill.effect_type == "heal":
		text += " | Heal %d HP" % skill.effect_value
	elif skill.effect_type != "none":
		text += " | %s %s%+d%% for %d hit(s)" % [
			skill.effect_target,
			"attack" if skill.effect_type == "attack_modifier" else "defense",
			skill.effect_value,
			skill.effect_uses,
		]
	return text


func _append_events(events: Array) -> void:
	for event: Dictionary in events:
		battle_log.append_text(_format_event(event) + "\n")


func _format_event(event: Dictionary) -> String:
	match event.type:
		"battle_started":
			return "[b]Battle started.[/b]"
		"skill_used":
			return "%s uses %s." % [_actor(event.actor), event.skill_name]
		"missed":
			return "The skill misses."
		"damage_dealt":
			return "It deals %d damage." % event.value
		"healed":
			return "%s recovers %d HP." % [_actor(event.target), event.value]
		"super_effective":
			return "[color=green]Super effective![/color]"
		"not_very_effective":
			return "[color=gray]Not very effective...[/color]"
		"attack_modified":
			return "%s attack changes by %+d%% for %d hit(s)." % [
				_actor(event.target),
				event.value,
				event.duration,
			]
		"defense_modified":
			return "%s defense changes by %+d%% for %d hit(s)." % [
				_actor(event.target),
				event.value,
				event.duration,
			]
		"style_changed":
			return "Player switches to %s and spends the turn." % _style_name(event.value)
		"skill_leveled_up":
			return "%s reaches skill level %d!" % [event.skill_name, event.value]
		"battle_finished":
			return "[b]%s wins.[/b]" % _actor(event.actor)
		"action_rejected":
			return "[color=red]%s[/color]" % event.message
	return "Unknown battle event: %s" % event.type


func _actor(actor: String) -> String:
	return actor.capitalize()


func _style_name(style_id: int) -> String:
	for style: Dictionary in styles:
		if style.id == style_id:
			return style.name
	return "Unknown"


func _free_children(parent: Node) -> void:
	for child: Node in parent.get_children():
		child.queue_free()
