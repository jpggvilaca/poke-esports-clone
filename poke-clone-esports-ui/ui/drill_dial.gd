class_name DrillDial
extends Control

const RING_COLOR := Color(0.12, 0.16, 0.20)
const RING_BORDER := Color(0.36, 0.62, 0.78)
const GOOD_COLOR := Color(0.27, 0.78, 0.46)
const PERFECT_COLOR := Color(1.0, 0.84, 0.28)
const NEEDLE_COLOR := Color(1.0, 0.95, 0.82)
const NEEDLE_SHADOW := Color(0.02, 0.03, 0.04, 0.74)

var needle_angle := 0.0
var target_angle := 0.0
var target_width := 0.72
var perfect_width := 0.22


func set_state(next_needle_angle: float, next_target_angle: float, next_target_width: float, next_perfect_width: float) -> void:
	needle_angle = next_needle_angle
	target_angle = next_target_angle
	target_width = next_target_width
	perfect_width = next_perfect_width
	queue_redraw()


func _draw() -> void:
	var center := size * 0.5
	var radius = min(size.x, size.y) * 0.38
	if radius <= 0:
		return

	draw_circle(center, radius + 18.0, Color(0.035, 0.045, 0.055, 1.0))
	draw_arc(center, radius, 0.0, TAU, 96, RING_COLOR, 34.0, true)
	draw_arc(center, radius, 0.0, TAU, 96, RING_BORDER, 3.0, true)
	draw_arc(center, radius, target_angle - target_width * 0.5, target_angle + target_width * 0.5, 32, GOOD_COLOR, 34.0, true)
	draw_arc(center, radius, target_angle - perfect_width * 0.5, target_angle + perfect_width * 0.5, 16, PERFECT_COLOR, 38.0, true)

	var needle_end = center + Vector2(cos(needle_angle), sin(needle_angle)) * (radius + 8.0)
	draw_line(center + Vector2(2, 3), needle_end + Vector2(2, 3), NEEDLE_SHADOW, 8.0, true)
	draw_line(center, needle_end, NEEDLE_COLOR, 5.0, true)
	draw_circle(center, 13.0, Color(0.01, 0.015, 0.02, 1.0))
	draw_circle(center, 8.0, NEEDLE_COLOR)
