extends Control

# This scene is a temporary combat lab. It deliberately contains presentation
# logic only: BattleBridge owns the real C++ simulation and turn order.

@onready var bridge: BattleBridge = $BattleBridge
@onready var player_spec: OptionButton = $Margin/Root/Setup/PlayerSpec
@onready var player_style: OptionButton = $Margin/Root/Setup/PlayerStyle
@onready var opponent_spec: OptionButton = $Margin/Root/Setup/OpponentSpec
@onready var opponent_style: OptionButton = $Margin/Root/Setup/OpponentStyle
@onready var restart_button: Button = $Margin/Root/StartButtons/Restart
@onready var profile_summary: Label = $Margin/Root/Profile/ProfileSummary
@onready var profile_roster: Label = $Margin/Root/Profile/ProfileRoster
@onready var profile_skills: Label = $Margin/Root/Profile/ProfileSkills
@onready var profile_trophies: Label = $Margin/Root/Profile/ProfileTrophies
@onready var style_buttons: HBoxContainer = $Margin/Root/StyleButtons
@onready var skill_buttons: VBoxContainer = $Margin/Root/SkillButtons
@onready var battle_log: RichTextLabel = $Margin/Root/BattleLog

var styles: Array[Dictionary] = []


func _ready() -> void:
	$Margin/Root/StartButtons/Start.pressed.connect(_start_battle)
	restart_button.pressed.connect(_start_battle)
	$Margin/Root/Profile/ProfileActions/NewTrainer.pressed.connect(_create_trainer_from_setup)
	$Margin/Root/Profile/ProfileActions/AddXp.pressed.connect(_profile_command.bind("Award 125 XP", Callable(bridge, "active_player_award_xp"), [125]))
	$Margin/Root/Profile/ProfileActions/AddRating.pressed.connect(_profile_command.bind("Award 30 rating", Callable(bridge, "trainer_award_rating"), [30]))
	$Margin/Root/Profile/ProfileActions/AddMoney.pressed.connect(_profile_command.bind("Award 100 money", Callable(bridge, "trainer_award_money"), [100]))
	$Margin/Root/Profile/ProfileActions/AddTrophy.pressed.connect(_profile_command.bind("Add first major trophy", Callable(bridge, "trainer_add_trophy"), ["first-major"]))
	$Margin/Root/Profile/ProfileActions/LearnRecover.pressed.connect(_profile_command.bind("Learn recover", Callable(bridge, "active_player_learn_skill"), ["top-recover"]))
	$Margin/Root/Profile/ProfileActions/UnequipBasic.pressed.connect(_profile_command.bind("Unequip basic", Callable(bridge, "active_player_unequip_skill"), ["top-basic"]))
	$Margin/Root/Profile/ProfileActions/EquipRecover.pressed.connect(_profile_command.bind("Equip recover", Callable(bridge, "active_player_equip_skill"), ["top-recover"]))
	$Margin/Root/Profile/RatingActions/NormalWin.pressed.connect(_rating_command.bind("Normal win vs same level", 0, 1, true))
	$Margin/Root/Profile/RatingActions/LowLevelWin.pressed.connect(_rating_command.bind("Normal win vs low level", -1, 1, true))
	$Margin/Root/Profile/RatingActions/NemesisWin.pressed.connect(_rating_command.bind("Nemesis win vs +2 levels", 3, 2, true))
	$Margin/Root/Profile/RatingActions/MajorWin.pressed.connect(_rating_command.bind("Major win vs +3 levels", 4, 3, true))

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
	_render_profile()
	var state: Dictionary = bridge.get_battle_state()
	_render_competitor($Margin/Root/Status/Player, state.player)
	_render_competitor($Margin/Root/Status/Opponent, state.opponent)
	restart_button.disabled = not state.started
	_rebuild_style_buttons()
	_rebuild_skill_buttons()


func _render_profile() -> void:
	var profile: Dictionary = bridge.get_trainer_state()
	var active: Dictionary = profile.active_player_profile
	profile_summary.text = "Trainer %s | %s | Rating %d | Money %d" % [
		profile.trainer_name,
		profile.game_type_name,
		profile.rating,
		profile.money,
	]
	profile_roster.text = "Active player profile: %s | %s %s | Lv %d (%d / %d XP) | Bonus HP %+d | Counter dmg +%d%% | Roster %d / 6" % [
		active.name,
		active.rank_name,
		active.spec_name,
		active.level,
		active.xp,
		active.xp_required,
		active.bonus_max_hp,
		active.bonus_counter_damage_percent,
		profile.roster.size(),
	]
	profile_skills.text = "Active skills: " + _format_skill_names(active.active_skills)
	profile_trophies.text = "Trophies: " + _format_strings(profile.trophies)


func _format_skill_names(skills: Array) -> String:
	var names: Array[String] = []
	for skill: Dictionary in skills:
		names.append(skill.name)
	return "none" if names.is_empty() else ", ".join(names)


func _format_strings(values: Array) -> String:
	var text_values: Array[String] = []
	for value: Variant in values:
		text_values.append(str(value))
	return "none" if text_values.is_empty() else ", ".join(text_values)


func _create_trainer_from_setup() -> void:
	var result: Dictionary = bridge.create_trainer("Trainer", player_spec.get_selected_id())
	_append_profile_result("Create trainer", result)
	_render()


func _profile_command(label: String, callable: Callable, args: Array) -> void:
	var result: Dictionary = callable.callv(args)
	_append_profile_result(label, result)
	_render()


func _rating_command(label: String, opponent_level_offset: int, context: int, won: bool) -> void:
	var profile: Dictionary = bridge.get_trainer_state()
	var active: Dictionary = profile.active_player_profile
	var opponent_level: int = max(1, active.level + opponent_level_offset)
	var result: Dictionary = bridge.trainer_apply_match_result(opponent_level, context, won)
	_append_rating_result(label, result)
	_render()


func _append_profile_result(label: String, result: Dictionary) -> void:
	if result.accepted:
		if result.leveled_up:
			battle_log.append_text("[color=cyan]%s accepted. Level %d -> %d.[/color]\n" % [
				label,
				result.old_level,
				result.new_level,
			])
		elif result.old_value != result.new_value:
			battle_log.append_text("[color=cyan]%s accepted. %d -> %d.[/color]\n" % [
				label,
				result.old_value,
				result.new_value,
			])
		else:
			battle_log.append_text("[color=cyan]%s accepted.[/color]\n" % label)
	else:
		battle_log.append_text("[color=red]%s rejected: %s[/color]\n" % [label, result.error])


func _append_rating_result(label: String, result: Dictionary) -> void:
	if result.accepted:
		battle_log.append_text("[color=yellow]%s (%s): rating %+d, %d -> %d.[/color]\n" % [
			label,
			result.context_name,
			result.rating_change,
			result.old_rating,
			result.new_rating,
		])
	else:
		battle_log.append_text("[color=red]%s rejected: %s[/color]\n" % [label, result.error])


func _render_competitor(panel: VBoxContainer, competitor: Dictionary) -> void:
	panel.get_node("SpecStyle").text = "Spec: %s | Style: %s" % [
		competitor.spec_name,
		competitor.style_name,
	]
	panel.get_node("Hp").text = "HP: %d / %d" % [competitor.hp, competitor.max_hp]
	panel.get_node("Focus").text = "Focus: %d / %d" % [competitor.focus, competitor.max_focus]
	panel.get_node("Effects").text = "Effects: %s | Counter dmg bonus: +%d%%" % [
		_format_status(competitor.status),
		competitor.counter_damage_bonus_percent,
	]


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
	var text := "[%s] %s | Power %d | Focus %d | Accuracy %d%% | Lv %d (%d XP)" % [
		skill.tone.capitalize(),
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
	text += "\n  %s" % skill.description
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
