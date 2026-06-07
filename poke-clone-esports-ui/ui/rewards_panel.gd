class_name RewardsPanel
extends Control

@onready var overlay: ColorRect = $Overlay
@onready var panel: PanelContainer = $Panel
@onready var title_label: Label = $Panel/Margin/Content/Title
@onready var body_label: RichTextLabel = $Panel/Margin/Content/Body


func _ready() -> void:
	hide_rewards()


func show_rewards(title: String, lines: Array[String]) -> void:
	title_label.text = title
	body_label.text = _join_lines(lines)
	visible = true
	overlay.visible = true
	panel.visible = true
	panel.modulate = Color(1, 1, 1, 0)
	panel.scale = Vector2(0.92, 0.92)

	var tween := create_tween()
	tween.set_parallel(true)
	tween.tween_property(panel, "modulate", Color.WHITE, 0.18)
	tween.tween_property(panel, "scale", Vector2.ONE, 0.22).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)


func hide_rewards() -> void:
	visible = false
	if is_node_ready():
		overlay.visible = false
		panel.visible = false


func _join_lines(lines: Array[String]) -> String:
	var output := ""
	for index in range(lines.size()):
		if index > 0:
			output += "\n"
		output += lines[index]
	return output
