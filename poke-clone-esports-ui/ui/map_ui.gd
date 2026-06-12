class_name MapUI
extends CanvasLayer

@onready var prompt_panel: PanelContainer = $PromptPanel
@onready var prompt_label: Label = $PromptPanel/PromptLabel
@onready var trainer_panel: PanelContainer = $TrainerPanel
@onready var trainer_text: Label = $TrainerPanel/Margin/Scroll/TrainerText
@onready var dialog_box: DialogBox = $DialogBox


func _ready() -> void:
	hide_prompt()
	set_trainer_visible(false)


func show_prompt(text: String) -> void:
	prompt_label.text = text
	prompt_panel.visible = true


func hide_prompt() -> void:
	prompt_panel.visible = false


func set_trainer_text(text: String) -> void:
	trainer_text.text = text


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


func toggle_trainer() -> void:
	set_trainer_visible(not trainer_panel.visible)
