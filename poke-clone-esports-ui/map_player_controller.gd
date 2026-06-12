class_name MapPlayerController
extends CharacterBody2D

const PLAYER_SPEED := 210.0
const SPRINT_MULTIPLIER := 1.55

@onready var player_sprite: Sprite2D = $Sprite
@onready var player_camera: Camera2D = $Camera

var world_bounds: MapWorldBounds
var input_enabled := true
var player_frame_timer := 0.0


func setup(next_world_bounds: MapWorldBounds, saved_position: Vector2) -> void:
	world_bounds = next_world_bounds
	world_bounds.configure_camera(player_camera)
	global_position = world_bounds.clamp_to_map(saved_position)
	if not world_bounds.is_walkable_position(global_position):
		global_position = world_bounds.nearest_walkable_position(global_position)


func set_input_enabled(enabled: bool) -> void:
	input_enabled = enabled
	if not input_enabled:
		velocity = Vector2.ZERO


func tick_movement(delta: float) -> void:
	if not input_enabled:
		velocity = Vector2.ZERO
		_animate_player(Vector2.ZERO, delta)
		return

	var direction := Input.get_vector("move_left", "move_right", "move_up", "move_down")
	var movement := _get_walkable_movement(direction, delta)
	var previous_position := global_position
	velocity = movement / delta if delta > 0.0 else Vector2.ZERO
	move_and_slide()
	global_position = world_bounds.clamp_to_map(global_position)
	if not world_bounds.is_walkable_position(global_position):
		global_position = previous_position
		velocity = Vector2.ZERO

	_animate_player(direction if movement != Vector2.ZERO else Vector2.ZERO, delta)


func get_player_position() -> Vector2:
	return global_position


func _get_walkable_movement(direction: Vector2, delta: float) -> Vector2:
	if direction == Vector2.ZERO:
		return Vector2.ZERO

	var move_speed := _current_move_speed()
	var direct_movement := direction * move_speed * delta
	if world_bounds.is_walkable_position(world_bounds.clamp_to_map(global_position + direct_movement)):
		return direct_movement

	if direction.x != 0.0:
		var horizontal_movement := Vector2(sign(direction.x), 0.0) * move_speed * delta
		if world_bounds.is_walkable_position(world_bounds.clamp_to_map(global_position + horizontal_movement)):
			return horizontal_movement

	if direction.y != 0.0:
		var vertical_movement := Vector2(0.0, sign(direction.y)) * move_speed * delta
		if world_bounds.is_walkable_position(world_bounds.clamp_to_map(global_position + vertical_movement)):
			return vertical_movement

	return Vector2.ZERO


func _current_move_speed() -> float:
	return PLAYER_SPEED * SPRINT_MULTIPLIER if Input.is_key_pressed(KEY_SHIFT) else PLAYER_SPEED


func _animate_player(direction: Vector2, delta: float) -> void:
	var row := player_sprite.frame_coords.y
	if abs(direction.x) > abs(direction.y):
		row = 2 if direction.x > 0.0 else 1
	elif direction.y != 0.0:
		row = 0 if direction.y > 0.0 else 3

	if direction == Vector2.ZERO:
		player_frame_timer = 0.0
		player_sprite.frame_coords = Vector2i(0, row)
		return

	player_frame_timer += delta
	var frame := int(player_frame_timer * 8.0) % 4
	player_sprite.frame_coords = Vector2i(frame, row)
