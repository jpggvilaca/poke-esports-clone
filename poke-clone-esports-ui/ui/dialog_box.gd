class_name DialogBox
extends Control

signal dialog_finished

@export var characters_per_second := 42.0

@onready var body_label: RichTextLabel = $Panel/Margin/Content/Text
@onready var prompt_label: Label = $Panel/Margin/Content/Prompt

var lines: Array[String] = []
var line_index := 0
var typing := false
var reveal_elapsed := 0.0


func _ready() -> void:
	hide_dialog()


func start(dialog_lines: Array[String]) -> void:
	lines.clear()
	for line in dialog_lines:
		lines.push_back(line)

	if lines.is_empty():
		hide_dialog()
		dialog_finished.emit()
		return

	line_index = 0
	visible = true
	set_process(true)
	_show_current_line()


func is_open() -> bool:
	return visible


func advance() -> void:
	if not visible:
		return

	if typing:
		body_label.visible_characters = _current_line_length()
		typing = false
		prompt_label.visible = true
		return

	line_index += 1
	if line_index >= lines.size():
		hide_dialog()
		dialog_finished.emit()
		return

	_show_current_line()


func hide_dialog() -> void:
	visible = false
	set_process(false)
	typing = false
	if is_node_ready():
		body_label.text = ""
		body_label.visible_characters = 0
		prompt_label.visible = false


func _process(delta: float) -> void:
	if not typing:
		return

	reveal_elapsed += delta
	var target_count := _current_line_length()
	var next_count = min(target_count, int(reveal_elapsed * characters_per_second))
	body_label.visible_characters = next_count

	if next_count >= target_count:
		typing = false
		prompt_label.visible = true


func _gui_input(event: InputEvent) -> void:
	if _is_advance_event(event):
		accept_event()
		advance()


func _unhandled_input(event: InputEvent) -> void:
	if not visible:
		return

	if _is_advance_event(event):
		get_viewport().set_input_as_handled()
		advance()


func _show_current_line() -> void:
	body_label.text = lines[line_index]
	body_label.visible_characters = 0
	prompt_label.visible = false
	reveal_elapsed = 0.0
	typing = true

	if _current_line_length() == 0:
		typing = false
		prompt_label.visible = true


func _current_line_length() -> int:
	return body_label.text.length()


func _is_advance_event(event: InputEvent) -> bool:
	var key_event := event as InputEventKey
	if key_event != null and key_event.pressed and not key_event.echo:
		return key_event.keycode == KEY_E

	var mouse_event := event as InputEventMouseButton
	if mouse_event != null and mouse_event.pressed:
		return mouse_event.button_index == MOUSE_BUTTON_LEFT

	return false
