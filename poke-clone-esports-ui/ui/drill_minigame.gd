class_name DrillMinigame
extends Control

signal drill_finished(quality: String)
signal drill_cancelled

const TARGET_WIDTH := 0.72
const PERFECT_WIDTH := 0.22
const NEEDLE_SPEED := TAU * 1
const MAX_DURATION := 4.0

@onready var panel: PanelContainer = $Panel
@onready var title_label: Label = $Panel/Margin/Content/Title
@onready var dial: DrillDial = $Panel/Margin/Content/Dial
@onready var feedback_label: Label = $Panel/Margin/Content/Feedback
@onready var reward_label: Label = $Panel/Margin/Content/Reward
@onready var cancel_button: Button = $Panel/Margin/Content/CancelButton

var rng := RandomNumberGenerator.new()
var needle_angle := 0.0
var target_angle := 0.0
var elapsed := 0.0
var running := false


func _ready() -> void:
	rng.randomize()
	cancel_button.pressed.connect(_cancel)
	hide_minigame()


func start(drill: Dictionary) -> void:
	var display_name := String(drill.get("display_name", "Drill"))
	title_label.text = display_name
	feedback_label.text = "Hit the highlighted window."
	reward_label.text = "Perfect +%s mana   Good +%s mana   Miss +%s mana" % [
		int(drill.get("perfect_mana_gain", 0)),
		int(drill.get("good_mana_gain", 0)),
		int(drill.get("miss_mana_gain", 0)),
	]

	needle_angle = rng.randf_range(0.0, TAU)
	target_angle = rng.randf_range(0.0, TAU)
	elapsed = 0.0
	running = true
	visible = true
	set_process(true)
	dial.set_state(needle_angle, target_angle, TARGET_WIDTH, PERFECT_WIDTH)
	grab_focus()

	panel.modulate = Color(1, 1, 1, 0)
	panel.scale = Vector2(0.92, 0.92)
	var tween := create_tween()
	tween.set_parallel(true)
	tween.tween_property(panel, "modulate", Color.WHITE, 0.14)
	tween.tween_property(panel, "scale", Vector2.ONE, 0.18).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)


func hide_minigame() -> void:
	visible = false
	running = false
	set_process(false)


func _process(delta: float) -> void:
	if not running:
		return

	elapsed += delta
	needle_angle = wrapf(needle_angle + NEEDLE_SPEED * delta, 0.0, TAU)
	dial.set_state(needle_angle, target_angle, TARGET_WIDTH, PERFECT_WIDTH)

	if elapsed >= MAX_DURATION:
		_finish("miss")


func _gui_input(event: InputEvent) -> void:
	if not visible:
		return

	if _is_commit_event(event):
		accept_event()
		_commit()


func _unhandled_input(event: InputEvent) -> void:
	if not visible:
		return

	if event.is_action_pressed("ui_cancel"):
		get_viewport().set_input_as_handled()
		_cancel()
		return

	if _is_commit_event(event):
		get_viewport().set_input_as_handled()
		_commit()


func _commit() -> void:
	if not running:
		return

	var distance := _angular_distance(needle_angle, target_angle)
	if distance <= PERFECT_WIDTH * 0.5:
		_finish("perfect")
	elif distance <= TARGET_WIDTH * 0.5:
		_finish("good")
	else:
		_finish("miss")


func _finish(quality: String) -> void:
	if not running:
		return

	hide_minigame()
	drill_finished.emit(quality)


func _cancel() -> void:
	if not visible:
		return

	hide_minigame()
	drill_cancelled.emit()


func _is_commit_event(event: InputEvent) -> bool:
	if event.is_action_pressed("ui_accept"):
		return true

	var key_event := event as InputEventKey
	if key_event != null and key_event.pressed and not key_event.echo:
		return key_event.keycode == KEY_SPACE or key_event.keycode == KEY_ENTER or key_event.keycode == KEY_KP_ENTER

	var mouse_event := event as InputEventMouseButton
	if mouse_event != null and mouse_event.pressed:
		return mouse_event.button_index == MOUSE_BUTTON_LEFT

	return false


func _angular_distance(first: float, second: float) -> float:
	return abs(wrapf(first - second + PI, 0.0, TAU) - PI)
