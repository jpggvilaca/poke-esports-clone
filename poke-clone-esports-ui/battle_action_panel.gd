class_name BattleActionPanel
extends VBoxContainer

signal fight_requested
signal farm_requested
signal push_requested
signal dragon_requested
signal quit_requested
signal skill_selected(skill: Dictionary)
signal target_selected(target_index: int)
signal return_requested

const SKILL_BUTTON_SCENE := preload("res://ui/components/skill_button.tscn")

@onready var main_menu_grid: GridContainer = $MainMenuGrid
@onready var skill_grid: GridContainer = $SkillGrid
@onready var target_label: Label = $TargetLabel
@onready var target_grid: GridContainer = $TargetGrid
@onready var fight_button: Button = $MainMenuGrid/FightButton
@onready var farm_button: Button = $MainMenuGrid/DrillButton
@onready var quit_button: Button = $MainMenuGrid/QuitButton
@onready var back_button: Button = $BackButton
@onready var return_button: Button = $ReturnButton

var skill_buttons: Array[BattleSkillButton] = []
var target_buttons: Array[Button] = []
var push_button: Button
var dragon_button: Button


func setup() -> void:
	_create_push_button()
	_create_dragon_button()
	fight_button.pressed.connect(_on_fight_button_pressed)
	farm_button.pressed.connect(_on_farm_button_pressed)
	push_button.pressed.connect(_on_push_button_pressed)
	dragon_button.pressed.connect(_on_dragon_button_pressed)
	quit_button.pressed.connect(_on_quit_button_pressed)
	back_button.pressed.connect(_on_back_button_pressed)
	return_button.pressed.connect(_on_return_button_pressed)


func show_main_menu(input_locked: bool) -> void:
	main_menu_grid.visible = true
	skill_grid.visible = false
	target_label.visible = false
	target_grid.visible = false
	back_button.visible = false
	return_button.visible = false
	set_buttons_disabled(input_locked)
	_grab_first_enabled([fight_button, farm_button, push_button, dragon_button, quit_button])


func show_fight_menu(skills: Array, input_locked: bool) -> void:
	refresh_skills(skills, input_locked)
	main_menu_grid.visible = false
	skill_grid.visible = true
	target_label.visible = false
	target_grid.visible = false
	back_button.visible = true
	var focus_candidates: Array[Control] = []
	for button in skill_buttons:
		if button.visible:
			focus_candidates.push_back(button)
	focus_candidates.push_back(back_button)
	_grab_first_enabled(focus_candidates)


func show_target_menu(player_team: Array, input_locked: bool) -> void:
	refresh_targets(player_team, input_locked)
	main_menu_grid.visible = false
	skill_grid.visible = false
	target_label.visible = true
	target_grid.visible = true
	back_button.visible = true
	var focus_candidates: Array[Control] = []
	for button in target_buttons:
		if button.visible:
			focus_candidates.push_back(button)
	focus_candidates.push_back(back_button)
	_grab_first_enabled(focus_candidates)


func show_return_button() -> void:
	set_buttons_disabled(true)
	main_menu_grid.visible = false
	skill_grid.visible = false
	target_label.visible = false
	target_grid.visible = false
	back_button.visible = false
	return_button.visible = true
	return_button.disabled = false
	return_button.grab_focus()


func set_return_text(text: String) -> void:
	return_button.text = text


func refresh_farm_action(input_locked: bool) -> void:
	farm_button.text = "Farm"
	farm_button.tooltip_text = "Gain mana."
	farm_button.disabled = input_locked


func refresh_fight_action(can_fight: bool, input_locked: bool) -> void:
	fight_button.text = "Fight"
	fight_button.tooltip_text = "Choose a skill." if can_fight else "No enemy champion is currently alive."
	fight_button.disabled = input_locked or not can_fight


func refresh_push_action(state: Dictionary, input_locked: bool) -> void:
	var objectives: Array = state.get("opponent_objectives", [])
	var objective_index := int(state.get("active_opponent_objective_index", 0))
	var objective_name := "Objective"
	if objective_index >= 0 and objective_index < objectives.size():
		var objective: Dictionary = objectives[objective_index]
		objective_name = String(objective.get("name", objective_name))

	var vulnerable := bool(state.get("opponent_objective_vulnerable", false))
	push_button.text = "Push %s" % objective_name
	push_button.tooltip_text = "Attack the enemy %s." % objective_name if vulnerable else "Defeat the enemy champion first to drop the backdoor shield."
	push_button.disabled = input_locked or not vulnerable


func refresh_dragon_action(state: Dictionary, input_locked: bool) -> void:
	var dragon: Dictionary = state.get("neutral_objective", {})
	var active := bool(dragon.get("active", false))
	var dragon_name := String(dragon.get("name", "Dragon"))

	if active:
		dragon_button.visible = true
		var hp := int(dragon.get("hp", 0))
		var max_hp := int(dragon.get("max_hp", 1))
		dragon_button.text = "%s %s/%s" % [dragon_name, hp, max_hp]
		dragon_button.tooltip_text = "Attack %s for a permanent 15%% power buff." % dragon_name
		dragon_button.disabled = input_locked
	else:
		dragon_button.visible = false
		dragon_button.text = dragon_name
		dragon_button.tooltip_text = ""
		dragon_button.disabled = true


func refresh_skills(skills: Array, input_locked: bool) -> void:
	_ensure_skill_button_count(skills.size())
	for index in range(skill_buttons.size()):
		if index >= skills.size():
			skill_buttons[index].visible = false
			continue
		skill_buttons[index].setup(skills[index], input_locked)


func refresh_targets(player_team: Array, input_locked: bool) -> void:
	_ensure_target_button_count(player_team.size())
	for index in range(target_buttons.size()):
		if index >= player_team.size():
			target_buttons[index].visible = false
			continue
		var player: Dictionary = player_team[index]
		var hp := int(player.get("hp", 0))
		var max_hp := int(player.get("max_hp", 1))
		var mana := int(player.get("mana", 0))
		var max_mana := int(player.get("max_mana", 1))
		var button := target_buttons[index]
		button.text = "%s\nHP %s/%s | Mana %s/%s" % [
			player.get("name", "Player"),
			hp,
			max_hp,
			mana,
			max_mana,
		]
		button.custom_minimum_size = Vector2(270, 72)
		button.disabled = input_locked or hp <= 0
		button.set_meta("target_index", index)
		button.visible = true


func _ensure_skill_button_count(count: int) -> void:
	while skill_buttons.size() < count:
		var button := SKILL_BUTTON_SCENE.instantiate() as BattleSkillButton
		button.selected.connect(_on_skill_button_selected)
		skill_grid.add_child(button)
		skill_buttons.push_back(button)


func _ensure_target_button_count(count: int) -> void:
	while target_buttons.size() < count:
		var button := Button.new()
		button.pressed.connect(_on_target_pool_button_pressed.bind(button))
		target_grid.add_child(button)
		target_buttons.push_back(button)


func set_buttons_disabled(disabled: bool) -> void:
	fight_button.disabled = disabled
	farm_button.disabled = disabled
	push_button.disabled = disabled
	dragon_button.disabled = disabled
	quit_button.disabled = disabled
	back_button.disabled = disabled
	for button in skill_buttons:
		if button.visible:
			button.disabled = disabled
	for button in target_buttons:
		if button.visible:
			button.disabled = disabled


func _grab_first_enabled(controls: Array) -> void:
	for control in controls:
		var button := control as BaseButton
		if button != null and button.visible and not button.disabled:
			button.grab_focus()
			return


func _create_push_button() -> void:
	push_button = Button.new()
	push_button.name = "PushButton"
	push_button.text = "Push Objective"
	push_button.custom_minimum_size = Vector2(270, 76)
	main_menu_grid.add_child(push_button)
	main_menu_grid.move_child(push_button, 2)


func _create_dragon_button() -> void:
	dragon_button = Button.new()
	dragon_button.name = "DragonButton"
	dragon_button.text = "Dragon"
	dragon_button.custom_minimum_size = Vector2(270, 76)
	main_menu_grid.add_child(dragon_button)
	main_menu_grid.move_child(dragon_button, 3)


func _unhandled_input(event: InputEvent) -> void:
	if not event.is_action_pressed("ui_cancel"):
		return
	if back_button.visible and not back_button.disabled:
		get_viewport().set_input_as_handled()
		show_main_menu(false)


func _on_skill_button_selected(skill: Dictionary) -> void:
	skill_selected.emit(skill)


func _on_target_pool_button_pressed(button: Button) -> void:
	target_selected.emit(int(button.get_meta("target_index", -1)))


func _on_fight_button_pressed() -> void:
	fight_requested.emit()


func _on_farm_button_pressed() -> void:
	farm_requested.emit()


func _on_push_button_pressed() -> void:
	push_requested.emit()


func _on_dragon_button_pressed() -> void:
	dragon_requested.emit()


func _on_quit_button_pressed() -> void:
	quit_requested.emit()


func _on_back_button_pressed() -> void:
	show_main_menu(false)


func _on_return_button_pressed() -> void:
	return_requested.emit()
