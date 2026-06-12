class_name MapWorldBounds
extends Node2D

const DEFAULT_MAP_MIN := Vector2(0, 0)
const DEFAULT_MAP_MAX := Vector2(1600, 1100)
const MAP_BOUNDS_MARGIN := 120.0
const PLAYER_MIN_EDGE_MARGIN := Vector2(40, 50)
const PLAYER_MAX_EDGE_MARGIN := Vector2(40, 40)
const WALKABLE_PATH_NAME := "Path"

var map_bounds := Rect2(DEFAULT_MAP_MIN, DEFAULT_MAP_MAX - DEFAULT_MAP_MIN)
var walkable_path_rects: Array[Rect2] = []


func setup(bounds_roots: Array) -> void:
	_update_map_bounds(bounds_roots)
	_cache_walkable_path_rects()


func configure_camera(camera: Camera2D) -> void:
	camera.enabled = true
	camera.limit_left = int(floor(map_bounds.position.x))
	camera.limit_top = int(floor(map_bounds.position.y))
	camera.limit_right = int(ceil(map_bounds.position.x + map_bounds.size.x))
	camera.limit_bottom = int(ceil(map_bounds.position.y + map_bounds.size.y))


func clamp_to_map(position: Vector2) -> Vector2:
	var next_position := position
	var min_position := map_bounds.position + PLAYER_MIN_EDGE_MARGIN
	var max_position := map_bounds.position + map_bounds.size - PLAYER_MAX_EDGE_MARGIN
	if max_position.x < min_position.x:
		next_position.x = map_bounds.get_center().x
	else:
		next_position.x = clamp(next_position.x, min_position.x, max_position.x)
	if max_position.y < min_position.y:
		next_position.y = map_bounds.get_center().y
	else:
		next_position.y = clamp(next_position.y, min_position.y, max_position.y)
	return next_position


func is_walkable_position(_position: Vector2) -> bool:
	for rect in walkable_path_rects:
		if rect.has_point(_position):
			return true
	return false


func nearest_walkable_position(_position: Vector2) -> Vector2:
	if walkable_path_rects.is_empty():
		return _position

	var nearest_position := _position
	var nearest_distance := INF
	for rect in walkable_path_rects:
		var candidate := Vector2(
			clamp(_position.x, rect.position.x, rect.position.x + rect.size.x),
			clamp(_position.y, rect.position.y, rect.position.y + rect.size.y)
		)
		var distance := _position.distance_squared_to(candidate)
		if distance < nearest_distance:
			nearest_distance = distance
			nearest_position = candidate

	return clamp_to_map(nearest_position)


func _update_map_bounds(bounds_roots: Array) -> void:
	var bounds := Rect2(DEFAULT_MAP_MIN, DEFAULT_MAP_MAX - DEFAULT_MAP_MIN)
	for root in bounds_roots:
		bounds = _include_node_bounds(root, bounds)
	map_bounds = bounds.grow(MAP_BOUNDS_MARGIN)


func _include_node_bounds(node: Node, bounds: Rect2) -> Rect2:
	var expanded_bounds := bounds
	var node_bounds := _get_canvas_item_bounds(node)
	if node_bounds.size != Vector2.ZERO:
		expanded_bounds = expanded_bounds.merge(node_bounds)

	for child in node.get_children():
		expanded_bounds = _include_node_bounds(child, expanded_bounds)

	return expanded_bounds


func _get_canvas_item_bounds(node: Node) -> Rect2:
	if node is Sprite2D:
		var sprite := node as Sprite2D
		return _transform_rect(sprite.get_global_transform(), sprite.get_rect())

	if node is Control:
		var control := node as Control
		return control.get_global_rect()

	return Rect2()


func _transform_rect(_transform: Transform2D, rect: Rect2) -> Rect2:
	var top_left := _transform * rect.position
	var top_right := _transform * (rect.position + Vector2(rect.size.x, 0.0))
	var bottom_left := _transform * (rect.position + Vector2(0.0, rect.size.y))
	var bottom_right := _transform * (rect.position + rect.size)
	var left = min(min(top_left.x, top_right.x), min(bottom_left.x, bottom_right.x))
	var top = min(min(top_left.y, top_right.y), min(bottom_left.y, bottom_right.y))
	var right = max(max(top_left.x, top_right.x), max(bottom_left.x, bottom_right.x))
	var bottom = max(max(top_left.y, top_right.y), max(bottom_left.y, bottom_right.y))
	
	return Rect2(Vector2(left, top), Vector2(right - left, bottom - top))


func _cache_walkable_path_rects() -> void:
	walkable_path_rects.clear()
	_cache_walkable_path_rects_from(self)


func _cache_walkable_path_rects_from(node: Node) -> void:
	if node is ColorRect and String(node.name).contains(WALKABLE_PATH_NAME):
		var rect := (node as ColorRect).get_global_rect()
		if rect.size != Vector2.ZERO:
			walkable_path_rects.push_back(rect)

	for child in node.get_children():
		_cache_walkable_path_rects_from(child)
