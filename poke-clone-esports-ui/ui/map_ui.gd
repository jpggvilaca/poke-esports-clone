class_name MapUI
extends CanvasLayer

signal play_requested
signal spec_chosen

@onready var main_menu_panel: PanelContainer = $MainMenuPanel
@onready var play_button: Button = $MainMenuPanel/Margin/Content/PlayButton
@onready var quit_button: Button = $MainMenuPanel/Margin/Content/QuitButton
@onready var rating_label: Label = $RatingPanel/RatingLabel
@onready var spec_choice_panel: PanelContainer = $SpecChoicePanel
@onready var spec_buttons: VBoxContainer = $SpecChoicePanel/Margin/Content/SpecButtons
@onready var prompt_panel: PanelContainer = $PromptPanel
@onready var prompt_label: Label = $PromptPanel/PromptLabel
@onready var trainer_panel: PanelContainer = $TrainerPanel
@onready var trainer_tabs: TabContainer = $TrainerPanel/Margin/Tabs
@onready var overview_text: Label = $TrainerPanel/Margin/Tabs/Overview/Scroll/Text
@onready var players_text: Label = $TrainerPanel/Margin/Tabs/Players/Scroll/Text
@onready var lineup_text: Label = $TrainerPanel/Margin/Tabs/Lineup/Scroll/Text
@onready var scout_text: Label = $TrainerPanel/Margin/Tabs/Scout/Scroll/Text
@onready var history_text: Label = $TrainerPanel/Margin/Tabs/History/Scroll/Text
@onready var dialog_box: DialogBox = $DialogBox

var pending_spec_choice := ""


func _ready() -> void:
	play_button.pressed.connect(_on_play_pressed)
	quit_button.pressed.connect(_on_quit_pressed)
	trainer_tabs.focus_mode = Control.FOCUS_ALL
	main_menu_panel.visible = false
	spec_choice_panel.visible = false
	hide_prompt()
	set_trainer_visible(false)
	set_rating(0)


func show_main_menu() -> void:
	hide_prompt()
	set_trainer_visible(false)
	main_menu_panel.visible = true
	play_button.grab_focus()


func hide_main_menu() -> void:
	main_menu_panel.visible = false


func show_spec_choice(options: Array[String]) -> String:
	hide_prompt()
	set_trainer_visible(false)
	pending_spec_choice = ""
	for child in spec_buttons.get_children():
		child.queue_free()

	for spec in options:
		var button := Button.new()
		button.text = spec
		button.custom_minimum_size = Vector2(220, 44)
		button.pressed.connect(_choose_spec.bind(spec))
		spec_buttons.add_child(button)

	spec_choice_panel.visible = true
	var first_button := spec_buttons.get_child(0) if spec_buttons.get_child_count() > 0 else null
	if first_button is Control:
		first_button.grab_focus()
	await spec_chosen
	spec_choice_panel.visible = false
	return pending_spec_choice


func is_blocking_overlay_open() -> bool:
	return main_menu_panel.visible or spec_choice_panel.visible


func set_rating(value: int) -> void:
	rating_label.text = "Rating %s" % value


func show_prompt(text: String) -> void:
	prompt_label.text = text
	prompt_panel.visible = true


func hide_prompt() -> void:
	prompt_panel.visible = false


func set_trainer_text(text: String) -> void:
	overview_text.text = text


func set_trainer_tabs(tabs: Dictionary) -> void:
	overview_text.text = String(tabs.get("Overview", ""))
	players_text.text = String(tabs.get("Players", ""))
	lineup_text.text = String(tabs.get("Lineup", ""))
	scout_text.text = String(tabs.get("Scout", ""))
	history_text.text = String(tabs.get("History", ""))


func is_trainer_visible() -> bool:
	return trainer_panel.visible


func is_dialog_open() -> bool:
	return dialog_box.is_open()


func show_dialog(lines: Array[String]) -> void:
	if lines.is_empty():
		return

	hide_prompt()
	set_trainer_visible(false)
	dialog_box.start(lines)
	await dialog_box.dialog_finished


func set_trainer_visible(_is_visible: bool) -> void:
	trainer_panel.visible = _is_visible
	
	if _is_visible:
		hide_prompt()
		trainer_tabs.grab_focus()


func toggle_trainer() -> void:
	set_trainer_visible(not trainer_panel.visible)


func _unhandled_input(event: InputEvent) -> void:
	if not trainer_panel.visible:
		return

	var key_event := event as InputEventKey
	if key_event == null or not key_event.pressed or key_event.echo:
		return

	if key_event.keycode == KEY_LEFT:
		_cycle_trainer_tab(-1)
		get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_RIGHT:
		_cycle_trainer_tab(1)
		get_viewport().set_input_as_handled()


func _on_play_pressed() -> void:
	play_requested.emit()


func _on_quit_pressed() -> void:
	get_tree().quit()


func _choose_spec(spec: String) -> void:
	pending_spec_choice = spec
	spec_chosen.emit()


func _cycle_trainer_tab(direction: int) -> void:
	var tab_count := trainer_tabs.get_tab_count()
	if tab_count <= 0:
		return
	trainer_tabs.current_tab = wrapi(trainer_tabs.current_tab + direction, 0, tab_count)
